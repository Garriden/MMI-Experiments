/**
 * Replicates the 3-stage Markov chain from the Radin MMI paper.
 * Stage 1: 50% probability of Target (1) or Miss (0).
 * Stages 2 & 3: 80% probability to stay, 20% to switch.
 *
 * https://noetic.org/wp-content/uploads/2021/11/Experiments_Testing_Models_of_Mind-Matter_Interaction.pdf
 * 
 * Using a new random number at every stage is actually a
 * mechanical requirement of the Markov chain model used in the paper.
 * Radin's argument is that if the hit rate stays high 
 * despite this added noise at each stage,
 *  it proves the effect isn't just a "push" from the beginning, 
 * but a "pull" from the end.

 * * Instructions:
 * 1. Upload to Arduino.
 * 2. Open Serial Monitor (set to 9600 baud).
 * 3. Send any character (or press Enter) to run a trial.
 * 4. Send 'q' to see the final summary statistics.
 
  Test randomness with Diehard test.
 */

#include <stdint.h>
#include "TouchScreen.h"
#include "Adafruit_GFX.h"    // Core graphics library
#include "MCUFRIEND_kbv.h"   // Hardware-specific library
#include <SPI.h>
#include <SD.h>
#include <avr/wdt.h>

const int chipSelect = 10;
File dataFile;
MCUFRIEND_kbv tft;

const int XP=6, XM=A2, YP=A1, YM=7; 
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
// Calibration
#define TS_MINX 120
#define TS_MAXX 900
#define TS_MINY 70
#define TS_MAXY 920
#define MINPRESSURE 10
#define MAXPRESSURE 1000

// Button.
int btnX = 40, btnY = 150, btnW = 160, btnH = 50;
int sumBtnX = 10, sumBtnY = 10, sumBtnW = 20, sumBtnH = 20;
int resBtnX = 60, resBtnY = 10, resBtnW = 120, resBtnH = 40; // restart button

bool isFinished = false;
bool wasTouched = false;
bool lastPressState = false; // Prevents the "too quick" flickering
bool finalResultsAlreadyPrinted = false;

long total_trials = 0;
long hits_stage1 = 0;
long hits_stage2 = 0;
long hits_stage3 = 0;
long releaseCount = 0;

void setup() {
  //Serial.begin(9600); // Debug.



  /////////////////////
  // Initialize TFT
  /////////////////////
  uint16_t ID = tft.readID();
  tft.begin(ID);

  // 1. Set to Vertical Mode (try 0 or 2)
  tft.setRotation(2); 

  // 2. Clear the screen
  tft.fillScreen(TFT_BLACK);

  // 3. Print text (Note: X max is now 240, Y max is 320)
  tft.setCursor(10, 10);
  tft.setTextColor(TFT_GREENYELLOW);
  tft.setTextSize(2);
  tft.println("Mind-Matter");
  tft.setCursor(10, 30);
  tft.println("Interaction");

  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(30, 40);
  tft.setTextSize(3);
  tft.setCursor(30, 70);
  tft.println("PRNG Markov");
  tft.setCursor(30, 100);
  tft.println("Chain-3");

  tft.setTextColor(TFT_DARKGREY);
  tft.setTextSize(2);
  tft.setCursor(10, 300);
  tft.println("v0.0");



  /////////////////////
  // Initialize SD
  /////////////////////
  if (!SD.begin(chipSelect)) {
    tft.setCursor(30, 260);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.println("SD Init Failed!");
  } else {
    // Create header if file doesn't exist
    if (!SD.exists("trials.csv")) {
      dataFile = SD.open("trials.csv", FILE_WRITE);
      if (dataFile) {
        //dataFile.println("Trial,Stage1,Stage2,Stage3");
        dataFile.close();
      }
    }
  }



  // Draw buttons.
  drawButton(false); // Draw the initial button
  drawSummaryButton(); // Draw the final, summary button


  
  // TODO: QRNG
  // Seed the random generator using noise from an unconnected analog pin
  randomSeed(analogRead(0)); // PRNG

  //Serial.println("--- Mind-Matter Interaction: Markov Chain ---");
  //Serial.println("Focus on reaching 'Target' (1) at Stage 3.");
  //Serial.println("Send any character to start a trial, or 'q' for results.\n");
  //Serial.println("Trial,Stage1,Stage2,Stage3");


}


void loop() {
  TSPoint p = ts.getPoint();
  pinMode(YP, OUTPUT);
  pinMode(XM, OUTPUT);

  bool isPressing = (p.z > MINPRESSURE && p.z < MAXPRESSURE);
  //Serial.print("isPressing: ");
  //Serial.println((int)p.z);
  
  // Convert raw touch to screen pixels
  int pixel_x = map(p.x, TS_MINX, TS_MAXX, 0, 240);
  int pixel_y = map(p.y, TS_MINY, TS_MAXY, 0, 320);

  if(isFinished) { // Stop everything if we've ended the trial.
      printSummary();
        
      // Check if the "RESTART" button area is touched
      if (pixel_x >= 190 && pixel_x <= 243 && 
          pixel_y >= 80 && pixel_y <= 250)
      {
          reboot(); // Trigger the hardware reset
      }

  } else {

    if(isPressing) {

      // --- PRINT COORDINATES ---
      // Clear a small area at the top to show coords
      tft.fillRect(0, 290, 240, 30, TFT_BLACK); 
      tft.setCursor(50, 300);
      tft.setTextColor(TFT_WHITE);
      tft.setTextSize(2);
      tft.print("X:"); tft.print(pixel_x);
      tft.print(" Y:"); tft.print(pixel_y);

      // Also print RAW values to Serial for your calibration
      //Serial.print("Raw X: "); Serial.print(p.x);
      //Serial.print(" | Raw Y: "); Serial.println(p.y);

      // --- CHECK FINISH BUTTON ---
      if(pixel_x >= 190 && pixel_x <= 243   && 
        pixel_y >= 32 && pixel_y <= 90     &&
        total_trials > 0) 
      {
          isFinished = true;
          //return;
      } else { // keep going.

          // --- PRESS BUTTON LOGIC ---
          bool insideButton = (pixel_x >= 75 && pixel_x <= 150) && 
                              (pixel_y >= 50 && pixel_y <= 290);

          if(insideButton) {
            releaseCount = 0;
            if(!lastPressState) {
              drawButton(true);   // Change to Green ONLY ONCE
              lastPressState = true;
              wasTouched = true;
            }
          }


      }





      
    } else { // NOT touching

      // Cover false releases.
      ++releaseCount;
      if(releaseCount > 10) releaseCount = 10; // Cover overflow.
    
      if(releaseCount > 2) {

        if(lastPressState) { // last time was touched.
          // This happens the exact moment you RELEASE
          drawButton(false);  // Change back to Red
          lastPressState = false;
          
          if (wasTouched) {
            runTrial();
            //Serial.println("Release Action Triggered!");
            wasTouched = false; 
          }
        }

      }
    }
  }

  delay(20);
}



void drawButton(bool pressed) {
  int color = pressed ? TFT_NAVY : TFT_BLUE;
  tft.fillRect(btnX, btnY, btnW, btnH, color);
  tft.drawRect(btnX, btnY, btnW, btnH, TFT_WHITE); // Border
  tft.setCursor(btnX + 25, btnY + 15);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.print("PRESS ME");
}

// Add this function to draw the small button to finish.
void drawSummaryButton() {
  if(total_trials > 0) {
    tft.fillRect(sumBtnX, sumBtnY, sumBtnW, sumBtnH, TFT_ORANGE);
    //tft.drawRect(sumBtnX, sumBtnY, sumBtnW, sumBtnH, TFT_ORANGE);
    tft.setCursor(sumBtnX + 8, sumBtnY + 8);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.print(" ");
  }
}



void runTrial() {
  total_trials++;
  int s1, s2, s3;

  // --- MARKOV CHAIN LOGIC ---
  s1 = (random(2) == 1) ? 1 : 0;
  if (s1 == 1) hits_stage1++;

  s2 = (random(10) < 8) ? s1 : (1 - s1); // 80% Stay, 20% Switch
  if (s2 == 1) hits_stage2++;

  s3 = (random(10) < 8) ? s2 : (1 - s2); // 80% Stay, 20% Switch
  if (s3 == 1) hits_stage3++;

  // --- SCREEN FEEDBACK ---
  // Instead of clearing the whole screen, we just clear the top result area
  tft.fillRect(0, 0, 240, 140, TFT_BLACK);
  drawSummaryButton(); // RE-DRAW BUTTON HERE
  
  tft.setCursor(60, 20);
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.print("Trial: "); tft.println(total_trials);

  tft.setCursor(100, 75);
  tft.setTextSize(6);
  if (s3 == 1) {
    tft.setTextColor(TFT_GREEN);
    tft.print("1"); // WIN
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("0"); // LOSS
  }


  // --- SD CARD LOGGING ---
  dataFile = SD.open("trials.csv", FILE_WRITE);
  if(dataFile) {
    dataFile.print(total_trials);
    dataFile.print(",");
    dataFile.print(s1);
    dataFile.print(",");
    dataFile.print(s2);
    dataFile.print(",");
    dataFile.println(s3);
    dataFile.close(); // Save and close
  }

  // Log the path to Serial for your data collection
  //Serial.print("Trial "); Serial.print(total_trials);
  //Serial.print(": "); Serial.print(s1); 
  //Serial.print("->"); Serial.print(s2); 
  //Serial.print("->"); Serial.println(s3);
}

/*
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(100, 60);
    tft.setTextColor(TFT_GREEN);
    tft.setTextSize(5);
    tft.println("1");
    //tft.println("O");
*/
void printSummary() {
  if(total_trials == 0) return;

  if(finalResultsAlreadyPrinted) return;

  finalResultsAlreadyPrinted = true;

  tft.fillScreen(TFT_BLACK);
  tft.setCursor(20, 80);
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(2);
  tft.println("FINAL RESULTS");
  tft.drawLine(20, 105, 200, 105, TFT_WHITE);

  float p1 = (hits_stage1 * 100.0) / total_trials;
  float p2 = (hits_stage2 * 100.0) / total_trials;
  float p3 = (hits_stage3 * 100.0) / total_trials;

  //Serial.println("\n================ SUMMARY ================");
  //Serial.print("Total Trials:   "); Serial.println(total_trials);
  //Serial.print("Stage 1 Hits:   "); Serial.print(p1); Serial.println("%");
  //Serial.print("Stage 2 Hits:   "); Serial.print(p2); Serial.println("%");
  //Serial.print("Stage 3 Hits:   "); Serial.print(p3); Serial.println("%");
  //Serial.println("=========================================");
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  
  tft.setCursor(20, 130);
  tft.print("1s: "); 
  if(p3 > 50.0) tft.setTextColor(TFT_GREEN);
  else if(p3 < 50.0) tft.setTextColor(TFT_RED);
  else tft.setTextColor(TFT_WHITE);
  tft.println(p3);

  tft.drawLine(60, 150, 130, 150, TFT_WHITE);
  tft.setTextColor(TFT_WHITE);

  tft.setCursor(20, 170);
  tft.print("Trials: "); tft.println(total_trials);

  tft.setCursor(20, 200);
  tft.print("Stage 1: "); tft.print(p1, 1); tft.println("%");
  
  tft.setCursor(20, 230);
  tft.print("Stage 2: "); tft.print(p2, 1); tft.println("%");
  
  tft.setCursor(20, 260);
  tft.print("Stage 3: "); 
  
  // Highlight if significant
  if (p3 >= 55.0) tft.setTextColor(TFT_GREEN);
  tft.print(p3, 1); tft.println("%");

  tft.setCursor(20, 290);
  tft.setTextColor(TFT_BLUE);
  tft.setTextSize(1);
  tft.print("Data saved to SD. Reset to restart.");



  // Draw the Restart Button
  tft.fillRect(resBtnX, resBtnY, resBtnW, resBtnH, TFT_DARKCYAN);
  tft.drawRect(resBtnX, resBtnY, resBtnW, resBtnH, TFT_WHITE);
  tft.setCursor(resBtnX + 15, resBtnY + 12);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.print("RESTART");

}

void reboot() {
  wdt_enable(WDTO_15MS); // Set watchdog to 15ms
  while (1); // Wait for the "bite" (reset)
}