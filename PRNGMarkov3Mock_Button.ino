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

MCUFRIEND_kbv tft;

const int XP=6, XM=A2, YP=A1, YM=7; 
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
// Calibration
#define TS_MINX 120
#define TS_MAXX 900
#define TS_MINY 70
#define TS_MAXY 920
#define MINPRESSURE 100
#define MAXPRESSURE 10000

// Button.
int btnX = 40, btnY = 150, btnW = 160, btnH = 50;
bool wasTouched = false;
bool lastPressState = false; // Prevents the "too quick" flickering

long total_trials = 0;
long hits_stage1 = 0;
long hits_stage2 = 0;
long hits_stage3 = 0;

void setup() {
  Serial.begin(9600);

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
  tft.setCursor(30, 60);           // Moved in slightly from the left
  tft.setTextColor(TFT_YELLOW);    // Changed color just for fun
  tft.setTextSize(3);
  tft.println("Holiiiiiii");

  drawButton(false); // Draw the initial button



  
  // TODO: QRNG
  // Seed the random generator using noise from an unconnected analog pin
  randomSeed(analogRead(0)); // PRNG

  Serial.println("--- Mind-Matter Interaction: Markov Chain ---");
  Serial.println("Focus on reaching 'Target' (1) at Stage 3.");
  Serial.println("Send any character to start a trial, or 'q' for results.\n");
  
  // Print CSV Header to Serial
  Serial.println("Trial,Stage1,Stage2,Stage3");
}


void loop() {
  TSPoint p = ts.getPoint();
  pinMode(YP, OUTPUT);
  pinMode(XM, OUTPUT);

  bool isPressing = (p.z > MINPRESSURE && p.z < MAXPRESSURE);

  if (isPressing) {
    // Convert raw touch to screen pixels
    int pixel_x = map(p.x, TS_MINX, TS_MAXX, 0, 240);
    int pixel_y = map(p.y, TS_MINY, TS_MAXY, 0, 320);

    // --- PRINT COORDINATES ---
    // Clear a small area at the top to show coords
    tft.fillRect(0, 290, 240, 30, TFT_BLACK); 
    tft.setCursor(50, 300);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.print("X:"); tft.print(pixel_x);
    tft.print(" Y:"); tft.print(pixel_y);

    // Also print RAW values to Serial for your calibration
    Serial.print("Raw X: "); Serial.print(p.x);
    Serial.print(" | Raw Y: "); Serial.println(p.y);

    // --- BUTTON LOGIC ---
    bool insideButton = (pixel_x >= 75 && pixel_x <= 150) && 
                        (pixel_y >= 50 && pixel_y <= 290);

    if (insideButton && !lastPressState) {
      drawButton(true);   // Change to Green ONLY ONCE
      lastPressState = true;
      wasTouched = true;
    }
  } 
  else {
    if (lastPressState) {
      // This happens the exact moment you RELEASE
      drawButton(false);  // Change back to Red
      lastPressState = false;
      
      if (wasTouched) {
        runTrial();
        Serial.println("Release Action Triggered!");
        wasTouched = false; 
      }
    }
  }
}



void drawButton(bool pressed) {
  int color = pressed ? TFT_GREEN : TFT_RED;
  tft.fillRect(btnX, btnY, btnW, btnH, color);
  tft.drawRect(btnX, btnY, btnW, btnH, TFT_WHITE); // Border
  tft.setCursor(btnX + 25, btnY + 15);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.print("PRESS ME");
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
  
  tft.setCursor(60, 40);
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.print("Trial: "); tft.println(total_trials);

  tft.setCursor(100, 80);
  tft.setTextSize(6);
  if (s3 == 1) {
    tft.setTextColor(TFT_GREEN);
    tft.print("1"); // WIN
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("0"); // LOSS
  }

  // Log the path to Serial for your data collection
  Serial.print("Trial "); Serial.print(total_trials);
  Serial.print(": "); Serial.print(s1); 
  Serial.print("->"); Serial.print(s2); 
  Serial.print("->"); Serial.println(s3);
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
  if (total_trials == 0) return;

  float p1 = (hits_stage1 * 100.0) / total_trials;
  float p2 = (hits_stage2 * 100.0) / total_trials;
  float p3 = (hits_stage3 * 100.0) / total_trials;

  Serial.println("\n================ SUMMARY ================");
  Serial.print("Total Trials:   "); Serial.println(total_trials);
  Serial.print("Stage 1 Hits:   "); Serial.print(p1); Serial.println("%");
  Serial.print("Stage 2 Hits:   "); Serial.print(p2); Serial.println("%");
  Serial.print("Stage 3 Hits:   "); Serial.print(p3); Serial.println("%");
  Serial.println("=========================================");
  
  if (p3 >= 55.0) {
    Serial.println("!!!!! Hit rate is notably above chance !!!!!");
  }
}
