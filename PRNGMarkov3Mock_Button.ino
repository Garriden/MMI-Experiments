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

long total_trials = 0;
long hits_stage1 = 0;
long hits_stage2 = 0;
long hits_stage3 = 0;

void setup() {
  Serial.begin(9600);
  
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
  if (Serial.available() > 0) {
    char input = Serial.read();

    // If 'q', print summary and stop
    if (input == 'q' || input == 'Q') {
      printSummary();
      while(true); // Halt execution
    }

    // Clear any extra characters in the buffer (like newline/carriage return)
    while(Serial.available() > 0) Serial.read();

    runTrial();
  }
}

void runTrial() {
  total_trials++;
  int s1, s2, s3;

  // --- STAGE 1: The Initial Selection (50/50) ---
  s1 = (random(0, 10) % 2 == 0) ? 1 : 0;
  if (s1 == 1) hits_stage1++;

  // --- STAGE 2: Transition (80% Stay, 20% Switch) ---
  int r2 = random(0, 10); 
  if (r2 >= 8) {
    s2 = 1 - s1; // Switch
  } else {
    s2 = s1;     // Stay
  }
  if (s2 == 1) hits_stage2++;

  // --- STAGE 3: Transition (80% Stay, 20% Switch) ---
  int r3 = random(0, 10);
  if (r3 >= 8) {
    s3 = 1 - s2; // Switch
  } else {
    s3 = s2;     // Stay
  }
  if (s3 == 1) hits_stage3++;

  // Output CSV row
  Serial.print(total_trials);
  Serial.print(",");
  Serial.print(s1);
  Serial.print(",");
  Serial.print(s2);
  Serial.print(",");
  Serial.print(s3);
  
  // User-friendly feedback
  Serial.print("  | Path: ");
  Serial.print(s1);
  Serial.print("->");
  Serial.print(s2);
  Serial.print("->");
  Serial.print(s3);
  if (s3 == 1) {
    Serial.println(" [WIN!]");
  } else {
    Serial.println(" [loss]");
  }
}

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