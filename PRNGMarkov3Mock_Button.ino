#include <stdint.h>
#include "TouchScreen.h"
#include "Adafruit_GFX.h"
#include "MCUFRIEND_kbv.h"
#include <SPI.h>
#include <SD.h>
#include <avr/wdt.h>

const int chipSelect = 10;
File dataFile;
MCUFRIEND_kbv tft;

const int XP=6, XM=A2, YP=A1, YM=7; 
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
#define TS_MINX 120
#define TS_MAXX 900
#define TS_MINY 70
#define TS_MAXY 920
#define MINPRESSURE 10
#define MAXPRESSURE 1000

int btnX = 40, btnY = 150, btnW = 160, btnH = 50;
int sumBtnX = 10, sumBtnY = 10, sumBtnW = 20, sumBtnH = 20;
int resBtnX = 60, resBtnY = 10, resBtnW = 120, resBtnH = 40;

bool isFinished = false;
bool wasTouched = false;
bool lastPressState = false;
bool finalResultsAlreadyPrinted = false;

long total_trials = 0;
long hits_stage1 = 0;
long hits_stage2 = 0;
long hits_stage3 = 0;
long releaseCount = 0;

char fileName[] = "TR000.CSV"; // Placeholder

void setup() {
  uint16_t ID = tft.readID();
  tft.begin(ID);
  tft.setRotation(2); 
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_GREENYELLOW);
  tft.setTextSize(2);
  tft.setCursor(10, 10); tft.println(F("Mind-Matter"));
  tft.setCursor(10, 30); tft.println(F("Interaction"));

  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(3);
  tft.setCursor(30, 70); tft.println(F("PRNG Markov"));
  tft.setCursor(30, 100); tft.println(F("Chain-3"));

  if (!SD.begin(chipSelect)) {
    tft.setCursor(30, 260);
    tft.setTextColor(TFT_WHITE);
    tft.println(F("SD Fail"));
  } else {
    // Manual Filename Generation (Saves ~2KB vs sprintf)
    for (int i = 0; i < 1000; i++) {
      fileName[2] = (i / 100) + '0';
      fileName[3] = ((i / 10) % 10) + '0';
      fileName[4] = (i % 10) + '0';
      if (!SD.exists(fileName)) break;
    }
    // Create file
    dataFile = SD.open(fileName, FILE_WRITE);
    if(dataFile) dataFile.close();
  }

  drawButton(false);
  drawSummaryButton();

  randomSeed(analogRead(0));

  tft.setTextColor(TFT_DARKGREY);
  tft.setTextSize(2);
  tft.setCursor(10, 300); tft.println(F("v0.1"));
}

void loop() {
  TSPoint p = ts.getPoint();
  pinMode(YP, OUTPUT);
  pinMode(XM, OUTPUT);

  bool isPressing = (p.z > MINPRESSURE && p.z < MAXPRESSURE);
  int pixel_x = map(p.x, TS_MINX, TS_MAXX, 0, 240);
  int pixel_y = map(p.y, TS_MINY, TS_MAXY, 0, 320);

  if(isFinished) {
      printSummary();
      if (pixel_x >= 190 && pixel_x <= 243 && pixel_y >= 80 && pixel_y <= 250) reboot();
  } else {
    if(isPressing) {
      // Logic for summary button check
      if(pixel_x >= 190 && pixel_x <= 243 && pixel_y >= 32 && pixel_y <= 90 && total_trials > 0) {
          isFinished = true;
      } else {
          bool insideButton = (pixel_x >= 75 && pixel_x <= 150) && (pixel_y >= 50 && pixel_y <= 290);
          if(insideButton) {
            releaseCount = 0;
            if(!lastPressState) {
              drawButton(true);
              lastPressState = true;
              wasTouched = true;
            }
          }
      }
    } else {
      if(++releaseCount > 10) releaseCount = 10;
      if(releaseCount > 2 && lastPressState) {
          drawButton(false);
          lastPressState = false;
          if (wasTouched) { runTrial(); wasTouched = false; }
      }
    }
  }
  delay(20);
}

void drawButton(bool pressed) {
  tft.fillRect(btnX, btnY, btnW, btnH, pressed ? TFT_NAVY : TFT_BLUE);
  tft.drawRect(btnX, btnY, btnW, btnH, TFT_WHITE);
  tft.setCursor(btnX + 25, btnY + 15);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.print(F("PRESS ME"));
}

void drawSummaryButton() {
  if(total_trials > 0) tft.fillRect(sumBtnX, sumBtnY, sumBtnW, sumBtnH, TFT_ORANGE);
}

void runTrial() {
  total_trials++;
  int s1 = (random(2) == 1) ? 1 : 0;
  if (s1 == 1) hits_stage1++;
  int s2 = (random(10) < 8) ? s1 : (1 - s1);
  if (s2 == 1) hits_stage2++;
  int s3 = (random(10) < 8) ? s2 : (1 - s2);
  if (s3 == 1) hits_stage3++;

  tft.fillRect(0, 0, 240, 140, TFT_BLACK);
  drawSummaryButton();
  
  tft.setCursor(50, 20);
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.print(F("Trials: ")); tft.println(total_trials);

  tft.setCursor(100, 75);
  tft.setTextSize(6);
  tft.setTextColor(s3 == 1 ? TFT_GREEN : TFT_RED);
  tft.print(s3);

  dataFile = SD.open(fileName, FILE_WRITE);
  if(dataFile) {
    dataFile.print(total_trials); dataFile.print(',');
    dataFile.print(s1); dataFile.print(',');
    dataFile.print(s2); dataFile.print(',');
    dataFile.println(s3);
    dataFile.close();
  }
}

void printSummary() {
  if(total_trials == 0 || finalResultsAlreadyPrinted) return;
  finalResultsAlreadyPrinted = true;

  tft.fillScreen(TFT_BLACK);
  tft.setCursor(20, 80);
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(2);
  tft.println(F("FINAL RESULTS"));
  tft.drawLine(20, 105, 200, 105, TFT_WHITE);

  float p1 = (hits_stage1 * 100.0) / total_trials;
  float p2 = (hits_stage2 * 100.0) / total_trials;
  float p3 = (hits_stage3 * 100.0) / total_trials;

  tft.setTextColor(TFT_WHITE);
  tft.setCursor(20, 130);
  tft.print(F("Hits: ")); 
  tft.setTextColor(p3 >= 50.0 ? TFT_GREEN : TFT_RED);
  tft.print(p3, 1); tft.println('%');

  tft.setTextColor(TFT_WHITE);
  tft.setCursor(20, 170);
  tft.print(F("Trials: ")); tft.println(total_trials);
  tft.setCursor(20, 200); tft.print(F("S1: ")); tft.print(p1, 1);
  tft.setCursor(20, 230); tft.print(F("S2: ")); tft.print(p2, 1);
  tft.setCursor(20, 260); tft.print(F("S3: ")); tft.print(p3, 1);

  tft.fillRect(resBtnX, resBtnY, resBtnW, resBtnH, TFT_DARKCYAN);
  tft.drawRect(resBtnX, resBtnY, resBtnW, resBtnH, TFT_WHITE);
  tft.setCursor(resBtnX + 15, resBtnY + 12);
  tft.print(F("RESTART"));

  tft.setCursor(100, 310);
  tft.setTextColor(TFT_BLUE);
  tft.setTextSize(1);
  tft.print(F("File saved: "));
  tft.print(fileName);
}

void reboot() {
  wdt_enable(WDTO_15MS);
  while (1);
}