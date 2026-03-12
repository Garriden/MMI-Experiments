#include <iostream>
#include <random>
#include <string>
#include <fstream> // Required for CSV output
#include <filesystem> // Required to check if file exists

namespace fs = std::filesystem;

/**
 * Replicates the 3-stage Markov chain from the Radin MMI paper.
 * Stage 1: 50% probability of Target (1) or Miss (0).
 * Stages 2 & 3: 80% probability to stay in current state, 20% to switch.
 * https://noetic.org/wp-content/uploads/2021/11/Experiments_Testing_Models_of_Mind-Matter_Interaction.pdf
 * 
 * Using a new random number at every stage is actually a mechanical requirement of the Markov chain model used in the paper.
 * Radin's argument is that if the hit rate stays high despite this added noise at each stage,
 *  it proves the effect isn't just a "push" from the beginning, but a "pull" from the end.
 */

std::string get_next_filename() {
    int counter = 0;
    while(true) {
        std::string name = "mmi_data_mock_" + std::to_string(counter) + ".csv";
        if(!fs::exists(name)) {
            return name;
        }
        ++counter;
    }
}


int main() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 9);


    // Find the first available sequential filename
    std::string filename = get_next_filename();
    std::ofstream csvFile(filename);

    if (!csvFile.is_open()) {
        std::cerr << "Error: Could not create " << filename << std::endl;
        return 1;
    }

    csvFile << "Trial,Stage1,Stage2,Stage3\n"; // Wwire csv header.



    int total_trials = 100;
    int hits_stage1 = 0;
    int hits_stage2 = 0;
    int hits_stage3 = 0;

    std::cout << "--- Mind-Matter Interaction: Markov Chain Experiment ---\n";
    std::cout << "Focus on reaching the 'Target' state at Stage 3.\n";
    std::cout << "Press ENTER to start a trial, or type 'q' to quit and see results.\n\n";

    for(int ii = 1; ii <= total_trials; ++ii) {
        int s1, s2, s3; // To store individual stage results

        // --- STAGE 1: The Initial Selection ---
        // (Even = Target, Odd = Miss)
        int r1 = distrib(gen);
        s1 = (r1 % 2 == 0) ? 1 : 0;
        if(s1 == 1) ++hits_stage1;

        // --- STAGE 2: Transition ---
        // (0-7 = Stay, 8-9 = Switch)
        int r2 = distrib(gen);
        if(r2 >= 8) {
            s2 = 1 - s1; // Switch
        } else {
            s2 = s1;     // Stay
        }

        if(s2 == 1) ++hits_stage2;

        // --- STAGE 3: Transition ---
        int r3 = distrib(gen);
        if(r3 >= 8) {
            s3 = 1 - s2; // Switch
        } else {
            s3 = s2;     // Stay
        }
        if(s3 == 1) ++hits_stage3;


        // Write this trial's data to the CSV
        csvFile << ii << "," << s1 << "," << s2 << "," << s3 << "\n";
        
        std::cout << "  Path: " << s1 << " -> " << s2 << " -> " << s3 << "   | "  << (s3 == 1 ? " [WIN!\a]" : " [loss]") << std::endl;
    }

    csvFile.close(); // Ensure data is saved

    // Final Statistics
    if(total_trials > 0) {
        double p1 = (hits_stage1 * 100.0) / total_trials;
        double p2 = (hits_stage2 * 100.0) / total_trials;
        double p3 = (hits_stage3 * 100.0) / total_trials;

        std::cout << "\n================ SUMMARY ================\n";
        std::cout << "Total Trials:   " << total_trials << "\n";
        std::cout << "Stage 1 Hits:   " << p1 << "%\n";
        std::cout << "Stage 2 Hits:   " << p2 << "%\n";
        std::cout << "Stage 3 Hits:   " << p3 << "%\n";
        std::cout << "=========================================\n";
        
        if(p3 >= 55.0) {
            std::cout << "¡¡¡¡¡ Final hit rate is notably above chance !!!!!\n";
        }
    }

    return 0;
}