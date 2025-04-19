#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <random>
#include <algorithm>
#include <thread>
#include <chrono>
#include <memory>
#include <iomanip>
#include "bench.hpp"
#include "chess.hpp"
#include "search.hpp"
#include "tunable_params.hpp"
#include "tt.hpp"

// Global TranspositionTable pointer needed by the engine
TranspositionTable* table = nullptr;

// Test positions and expected best moves from bench.cpp
const std::string positions[] = {
    "r2r2k1/pp2bppp/4b3/8/4N3/8/PPP1BPPP/R4RK1 w - - 0 1",                     // b2b3
    "3k4/8/q7/5r2/8/3B4/PPP5/2KN4 b - - 0 1",                                  // a6h6
    "3k4/8/7q/5r2/8/3B4/PPP5/1K1N4 b - - 0 1",                                 // h6d2
    "3k4/8/8/5r2/8/3B4/PPPq4/1K1N4 w - - 0 1",                                 // a2a3
    "r1bqk2r/ppp1bppp/2n1p3/2Pp4/3P4/2PBPN2/P4PPP/R1BQK2R b KQkq - 2 8",       // e8g8
    "4b1k1/p5p1/7p/2qp1Q2/6P1/7P/5RK1/4rB2 b - - 2 39",                        // c5d6,
    "r2q1rk1/ppp1nppp/3p1n2/3Pp1B1/1b2P1b1/2NB1N2/PPP2PPP/R2Q1RK1 w - - 4 10", // g5f6,
    "r3r3/5pk1/3p3p/3Pp1p1/1Pp1n1q1/2Q1B3/1PB1NK2/3R2R1 w - - 0 41",           // c2e4
    "r3r3/5pk1/3p3p/3Pp1p1/1Pp1q3/2Q1B3/1P2NK2/3R2R1 w - - 0 42",              // e3d2
    "8/r5k1/3p4/3P2pp/1P3p2/1K6/1P1r4/4RR2 w - - 4 52",                        // e1e6
    "1r1qkb1r/p1pb1ppp/2n2n2/3p4/Q3p3/2P2N2/PP1PPPPP/RNB1KB1R w KQk - 0 8",    // f3d4
    "2b2b1r/r3kp2/pqB1p2p/1p2P1p1/3pQ1n1/6P1/PP2P2P/RNB2RK1 b - - 6 17",       // a7c7
    "rnb1kb1r/pppp1ppp/5n2/8/4q3/5N2/PPPPBPPP/RNBQK2R b KQkq - 1 5",           // e4e7
    "3r4/2R5/1k3pp1/p7/B6p/2b2P1P/5P1B/5K2 b - - 5 57",                        // d8d4
    "r3r1k1/1p1bn1pp/4p3/pPPp1p2/P2P1q2/3B1N2/2Q2RPP/4R1K1 b - - 1 22",        // f4c7
    "r3b1k1/1p4p1/7p/pPP1Q3/P2P4/1q6/2B2RPP/6K1 b - - 6 35",                   // b3f7
    "r5k1/1p3bp1/7p/pPP5/P2PQ3/8/2B3PP/6K1 b - - 1 37",                         // f7c4
    "rn2Rb1r/2k2ppp/p1P2q2/1p6/2p1Q3/2P2B1P/P4PP1/1RB3K1 b - - 1 19",
    "r1bq1rk1/ppp2ppp/5n2/3pb3/8/P1NBP3/1PP2PPP/R1BQ1RK1 w - - 0 10",
    "r3r1k1/pppq1ppp/8/3p2P1/2b1nP2/P1P1PB2/1BP4P/R2QR1K1 w - - 3 18",
    "r3r1k1/ppp2ppp/8/3p2P1/2b1nP2/P1P1PB1q/1BP4P/1R1QR1K1 w - - 5 19"

};

const std::string targetMoves[] = {
    "b2b3", "a6h6", "h6d2", "a2a3", "e8g8", "c5d6", "g5f6", 
    "c2e4", "e3d2", "e1e6", "f3d4", "a7c7", "e4e7", 
    "d8d4", "f4c7", "b3f7", "f7c4","g7g5","f2f4",
    "f3g2","f2g2"
};

// Configuration for tuning
struct TuningConfig {
    int iterations;       // Total number of iterations 
    int population_size;  // Number of parameter sets to evaluate
    int tournament_size;  // Size of tournament for selection
    double mutation_rate; // Probability of mutation for each parameter
    int search_depth;     // Search depth for evaluating moves
};

// Individual parameter set and its fitness
struct Individual {
    std::vector<int> params;  // Parameter values
    double fitness;          // Higher is better (number of correct moves found)
    double nodes;            // Total nodes searched (tiebreaker for equal fitness)

    Individual() : fitness(0), nodes(0) {}
};

// Parameter definition
struct Parameter {
    std::string name;
    int current_value;
    int min_value;
    int max_value;
    int step_size;
};

// Parameter bounds for tuning
std::vector<Parameter> createParameters() {
    return {
        {"RFP_MARGIN", 75, 50, 100, 5},
        {"RFP_DEPTH", 5, 3, 8, 1},
        {"RFP_IMPROVING_BONUS", 62, 30, 90, 5},
        {"LMR_BASE", 75, 50, 100, 5},
        {"LMR_DIVISION", 225, 150, 300, 10},
        {"NMP_BASE", 3, 1, 5, 1},
        {"NMP_DIVISION", 3, 1, 5, 1},
        {"NMP_MARGIN", 180, 100, 250, 10},
        {"LMP_DEPTH_THRESHOLD", 7, 5, 10, 1},
        {"FUTILITY_MARGIN", 150, 100, 250, 10},
        {"FUTILITY_DEPTH", 6, 4, 8, 1},
        {"FUTILITY_IMPROVING", 24, 15, 35, 2},
        {"QS_FUTILITY_MARGIN", 177, 120, 230, 10},
        {"SEE_QUIET_MARGIN_BASE", -70, -100, -50, 5},
        {"SEE_NOISY_MARGIN_BASE", -15, -25, -5, 1},
        {"ASPIRATION_DELTA", 12, 8, 18, 1},
        {"HISTORY_PRUNING_THRESHOLD", 4000, 3000, 5000, 250}
    };
}

// Function to evaluate a set of parameters by testing benchmark positions
std::pair<double, double> evaluateParameters(const std::vector<int>& params, 
                                            const std::vector<Parameter>& paramDefs, 
                                            int searchDepth) {
    // Set the parameters
    for (size_t i = 0; i < params.size(); i++) {
        if (paramDefs[i].name == "RFP_MARGIN") TunableParams::RFP_MARGIN = params[i];
        else if (paramDefs[i].name == "RFP_DEPTH") TunableParams::RFP_DEPTH = params[i];
        else if (paramDefs[i].name == "RFP_IMPROVING_BONUS") TunableParams::RFP_IMPROVING_BONUS = params[i];
        else if (paramDefs[i].name == "LMR_BASE") TunableParams::LMR_BASE = params[i];
        else if (paramDefs[i].name == "LMR_DIVISION") TunableParams::LMR_DIVISION = params[i];
        else if (paramDefs[i].name == "NMP_BASE") TunableParams::NMP_BASE = params[i];
        else if (paramDefs[i].name == "NMP_DIVISION") TunableParams::NMP_DIVISION = params[i];
        else if (paramDefs[i].name == "NMP_MARGIN") TunableParams::NMP_MARGIN = params[i];
        else if (paramDefs[i].name == "LMP_DEPTH_THRESHOLD") TunableParams::LMP_DEPTH_THRESHOLD = params[i];
        else if (paramDefs[i].name == "FUTILITY_MARGIN") TunableParams::FUTILITY_MARGIN = params[i];
        else if (paramDefs[i].name == "FUTILITY_DEPTH") TunableParams::FUTILITY_DEPTH = params[i];
        else if (paramDefs[i].name == "FUTILITY_IMPROVING") TunableParams::FUTILITY_IMPROVING = params[i];
        else if (paramDefs[i].name == "QS_FUTILITY_MARGIN") TunableParams::QS_FUTILITY_MARGIN = params[i];
        else if (paramDefs[i].name == "SEE_QUIET_MARGIN_BASE") TunableParams::SEE_QUIET_MARGIN_BASE = params[i];
        else if (paramDefs[i].name == "SEE_NOISY_MARGIN_BASE") TunableParams::SEE_NOISY_MARGIN_BASE = params[i];
        else if (paramDefs[i].name == "ASPIRATION_DELTA") TunableParams::ASPIRATION_DELTA = params[i];
        else if (paramDefs[i].name == "HISTORY_PRUNING_THRESHOLD") TunableParams::HISTORY_PRUNING_THRESHOLD = params[i];
    }

    // Initialize the engine
    initLateMoveTable();

    int correct = 0;
    uint64_t totalNodes = 0;
    int totalTests = sizeof(positions) / sizeof(positions[0]);

    for (int i = 0; i < totalTests; i++) {
        SearchThread st;
        st.applyFen(positions[i]);
        st.clear();
        
        // Search the position
        SearchStack stack[MAXPLY + 10], *ss = stack + 7;
        Move bestMove = NO_MOVE;
        
        iterativeDeepening<false>(st, searchDepth);
        bestMove = st.bestMove;
        
        // Convert found move to UCI format and check if it matches expected
        std::string foundMove = convertMoveToUci(bestMove);
        if (foundMove == targetMoves[i]) {
            correct++;
        }
        
        totalNodes += st.nodes;
    }

    return std::make_pair(correct, totalNodes);
}

// Initialize a random individual
Individual createRandomIndividual(const std::vector<Parameter>& params, std::mt19937& rng) {
    Individual ind;
    ind.params.resize(params.size());
    
    for (size_t i = 0; i < params.size(); i++) {
        std::uniform_int_distribution<int> dist(
            params[i].min_value, 
            params[i].max_value
        );
        ind.params[i] = (dist(rng) / params[i].step_size) * params[i].step_size;
    }
    
    return ind;
}

// Mutate an individual
void mutate(Individual& ind, const std::vector<Parameter>& params, double mutation_rate, std::mt19937& rng) {
    for (size_t i = 0; i < ind.params.size(); i++) {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        if (dist(rng) < mutation_rate) {
            std::uniform_int_distribution<int> value_dist(
                params[i].min_value, 
                params[i].max_value
            );
            ind.params[i] = (value_dist(rng) / params[i].step_size) * params[i].step_size;
        }
    }
}

// Crossover two individuals
Individual crossover(const Individual& parent1, const Individual& parent2, std::mt19937& rng) {
    Individual child;
    child.params.resize(parent1.params.size());
    
    for (size_t i = 0; i < parent1.params.size(); i++) {
        std::uniform_int_distribution<int> dist(0, 1);
        child.params[i] = dist(rng) ? parent1.params[i] : parent2.params[i];
    }
    
    return child;
}

// Tournament selection
Individual tournamentSelect(const std::vector<Individual>& population, int tournamentSize, std::mt19937& rng) {
    std::uniform_int_distribution<int> dist(0, population.size() - 1);
    
    Individual best;
    best.fitness = -1;
    
    for (int i = 0; i < tournamentSize; i++) {
        int idx = dist(rng);
        if (population[idx].fitness > best.fitness || 
            (population[idx].fitness == best.fitness && population[idx].nodes < best.nodes)) {
            best = population[idx];
        }
    }
    
    return best;
}

// Run the genetic algorithm for parameter tuning based on benchmark accuracy
void tuneParameters(const TuningConfig& config) {
    // Initialize parameters
    auto paramDefs = createParameters();
    
    // Setup random number generator
    std::random_device rd;
    std::mt19937 rng(rd());
    
    // First, try to load and evaluate best parameters from benchmark_best_params.txt
    Individual bestFromFile;
    bestFromFile.params.resize(paramDefs.size());
    bool bestFileLoaded = false;
    
    std::ifstream bestFile("test/params/benchmark_best_params.txt");
    if (bestFile.is_open()) {
        std::cout << "Loading parameters from benchmark_best_params.txt..." << std::endl;
        std::string param;
        int value;
        int foundParams = 0;
        
        while (bestFile >> param >> value) {
            for (size_t i = 0; i < paramDefs.size(); i++) {
                if (paramDefs[i].name == param) {
                    bestFromFile.params[i] = value;
                    foundParams++;
                    break;
                }
            }
        }
        bestFile.close();
        
        // Only consider file loaded if all parameters were found
        if (foundParams == static_cast<int>(paramDefs.size())) {
            std::cout << "Evaluating parameters from benchmark_best_params.txt..." << std::endl;
            auto [correct, nodes] = evaluateParameters(bestFromFile.params, paramDefs, config.search_depth);
            bestFromFile.fitness = correct;
            bestFromFile.nodes = nodes;
            int totalTests = sizeof(positions) / sizeof(positions[0]);
            std::cout << "Fitness from file: " << bestFromFile.fitness << "/" << totalTests 
                     << " correct moves, " << bestFromFile.nodes << " nodes" << std::endl;
            bestFileLoaded = true;
        } else {
            std::cout << "Warning: benchmark_best_params.txt missing some parameters, found " 
                     << foundParams << "/" << paramDefs.size() << std::endl;
        }
    } else {
        std::cout << "Could not open benchmark_best_params.txt, starting fresh." << std::endl;
    }
    
    // Create initial population
    std::vector<Individual> population(config.population_size);
    std::cout << "Creating initial population..." << std::endl;
    
    // If best file was loaded, use it as the first individual
    if (bestFileLoaded) {
        population[0] = bestFromFile;
        for (int i = 1; i < config.population_size; i++) {
            population[i] = createRandomIndividual(paramDefs, rng);
        }
    } else {
        for (int i = 0; i < config.population_size; i++) {
            population[i] = createRandomIndividual(paramDefs, rng);
        }
    }
    
    // Evaluate initial population
    std::cout << "Evaluating initial population..." << std::endl;
    int totalTests = sizeof(positions) / sizeof(positions[0]);
    
    for (auto& ind : population) {
        // Skip evaluation if it's the loaded best (already evaluated)
        if (bestFileLoaded && &ind == &population[0]) {
            continue;
        }
        
        auto [correct, nodes] = evaluateParameters(ind.params, paramDefs, config.search_depth);
        ind.fitness = correct;
        ind.nodes = nodes;
        std::cout << "Individual evaluated: " << correct << "/" << totalTests 
                 << " correct moves, " << nodes << " nodes" << std::endl;
    }
    
    // Sort by fitness (higher is better) and then by nodes (lower is better)
    std::sort(population.begin(), population.end(), [](const Individual& a, const Individual& b) {
        if (a.fitness != b.fitness)
            return a.fitness > b.fitness;
        return a.nodes < b.nodes;
    });

    // Best individual so far (either from file or from initial population)
    Individual bestEver = population[0];
    std::cout << "Initial best fitness: " << bestEver.fitness << "/" << totalTests 
             << " correct moves, " << bestEver.nodes << " nodes" << std::endl;
    
    // Save initial parameters
    std::ofstream initialFile("test/params/benchmark_initial_params.txt");
    for (size_t i = 0; i < paramDefs.size(); i++) {
        initialFile << paramDefs[i].name << " " << paramDefs[i].current_value << std::endl;
    }
    initialFile.close();
    std::cout<< "Start Optimization loop..." << std::endl;
    // Main optimization loop
    for (int gen = 0; gen < config.iterations; gen++) {
        std::vector<Individual> newPopulation;
        table->clear();
        // Elitism - keep best individuals (10% of population)
        int eliteCount = std::max(1, config.population_size / 10);
        for (int i = 0; i < eliteCount && i < (int)population.size(); i++) {
            newPopulation.push_back(population[i]);
        }
        
        // Create new individuals
        while (newPopulation.size() < population.size()) {
            // Select parents
            Individual parent1 = tournamentSelect(population, config.tournament_size, rng);
            Individual parent2 = tournamentSelect(population, config.tournament_size, rng);
            
            // Create child through crossover
            Individual child = crossover(parent1, parent2, rng);
            
            // Mutate child
            mutate(child, paramDefs, config.mutation_rate, rng);
            
            // Add to new population
            newPopulation.push_back(child);
        }
        
        // Evaluate new individuals
        for (auto& ind : newPopulation) {
            if (ind.fitness == 0) { // Only evaluate if not from elites
                auto [correct, nodes] = evaluateParameters(ind.params, paramDefs, config.search_depth);
                ind.fitness = correct;
                ind.nodes = nodes;
            }
            
        }
        
        // Replace old population
        population = newPopulation;
        
        // Sort by fitness
        std::sort(population.begin(), population.end(), [](const Individual& a, const Individual& b) {
            if (a.fitness != b.fitness)
                return a.fitness > b.fitness;
            return a.nodes < b.nodes;
        });
        
        // Update best individual
        if (population[0].fitness > bestEver.fitness || 
            (population[0].fitness == bestEver.fitness && population[0].nodes < bestEver.nodes)) {
            bestEver = population[0];
        }
        
        std::cout << "Generation " << gen + 1 << "/" << config.iterations 
                  << ", Best fitness: " << population[0].fitness << "/" << totalTests
                  << " correct moves, " << population[0].nodes << " nodes"
                  << ", All-time best: " << bestEver.fitness << "/" << totalTests 
                  << " correct moves, " << bestEver.nodes << " nodes" << std::endl;
        
        // Output current best parameters
        std::cout << "Current best parameters:" << std::endl;
        for (size_t i = 0; i < paramDefs.size(); i++) {
            std::cout << paramDefs[i].name << ": " << population[0].params[i] << std::endl;
        }
        
        // Save current best to file
        std::ofstream outFile("test/params/benchmark_params_gen_" + std::to_string(gen + 1) + ".txt");
        for (size_t i = 0; i < paramDefs.size(); i++) {
            outFile << paramDefs[i].name << " " << population[0].params[i] << std::endl;
        }
        outFile.close();
    }
    
    // Final output
    std::cout << "\nOptimization complete!\n";
    std::cout << "Best parameters found (" << bestEver.fitness << "/" << totalTests << " correct):" << std::endl;
    for (size_t i = 0; i < paramDefs.size(); i++) {
        std::cout << paramDefs[i].name << ": " << bestEver.params[i] << std::endl;
    }
    
    // Save best parameters to file
    std::ofstream bestFileO("test/params/benchmark_best_params.txt");
    for (size_t i = 0; i < paramDefs.size(); i++) {
        bestFileO << paramDefs[i].name << " " << bestEver.params[i] << std::endl;
    }
    bestFileO.close();
    
    // Also save to benchmark_initial_params.txt for future tuning sessions
    std::ofstream iF("test/params/benchmark_initial_params.txt");
    for (size_t i = 0; i < paramDefs.size(); i++) {
        iF << paramDefs[i].name << " " << bestEver.params[i] << std::endl;
    }
    iF.close();
    
    // Copy the best parameters to the root directory for immediate use with make bench
    std::ofstream rootFile("benchmark_best_params.txt");
    for (size_t i = 0; i < paramDefs.size(); i++) {
        rootFile << paramDefs[i].name << " " << bestEver.params[i] << std::endl;
    }
    rootFile.close();
    
    std::cout << "Parameters saved to test/params/benchmark_best_params.txt" << std::endl;
    std::cout << "Parameters also saved to test/params/benchmark_initial_params.txt for future tuning" << std::endl;
    std::cout << "Parameters copied to benchmark_best_params.txt in root directory for immediate use with make bench" << std::endl;
}

int main(int argc, char* argv[]) {
    // Initialize the transposition table
    table = new TranspositionTable();
    table->Initialize(64); // Use 64MB for tuning
    
    // Default configuration
    TuningConfig config = {
        .iterations = 15,        // 15 generations
        .population_size = 10,   // 20 individuals per generation
        .tournament_size = 3,    // Tournament selection size
        .mutation_rate = 0.25,   // 25% chance to mutate each parameter
        .search_depth = 15       // Search depth for evaluating positions
    };
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-i" || arg == "--iterations") {
            if (i + 1 < argc) config.iterations = std::stoi(argv[++i]);
        } else if (arg == "-p" || arg == "--population") {
            if (i + 1 < argc) config.population_size = std::stoi(argv[++i]);
        } else if (arg == "-t" || arg == "--tournament") {
            if (i + 1 < argc) config.tournament_size = std::stoi(argv[++i]);
        } else if (arg == "-m" || arg == "--mutation") {
            if (i + 1 < argc) config.mutation_rate = std::stod(argv[++i]);
        } else if (arg == "-d" || arg == "--depth") {
            if (i + 1 < argc) config.search_depth = std::stoi(argv[++i]);
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: tune_bench [options]\n"
                      << "Options:\n"
                      << "  -i, --iterations N    Set number of iterations (default: 15)\n"
                      << "  -p, --population N    Set population size (default: 20)\n"
                      << "  -t, --tournament N    Set tournament size (default: 3)\n"
                      << "  -m, --mutation N      Set mutation rate (default: 0.25)\n"
                      << "  -d, --depth N         Set search depth (default: 10)\n"
                      << "  -h, --help            Show this help message\n";
            return 0;
        }
    }
    
    // Start parameter tuning
    std::cout << "Starting benchmark-based parameter tuning with:\n"
              << "  Iterations:      " << config.iterations << "\n"
              << "  Population size: " << config.population_size << "\n"
              << "  Tournament size: " << config.tournament_size << "\n"
              << "  Mutation rate:   " << config.mutation_rate << "\n"
              << "  Search depth:    " << config.search_depth << "\n\n";
    
    tuneParameters(config);
    
    // Cleanup
    delete table;
    
    return 0;
}