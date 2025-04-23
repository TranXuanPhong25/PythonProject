#include <iostream>
 #include <fstream>
 #include <vector>
 #include <string>
 #include <cmath>
 #include <random>
 #include <algorithm>
 #include <thread>
 #include <chrono>
 #include "bench.hpp"
 #include "tunable_params.hpp"
 #include "chess.hpp"
 #include "search.hpp"
 #include "tt.hpp"
 
 
 
 // Global TranspositionTable pointer
 TranspositionTable* table = nullptr;
 
 // Configuration for tuning
 struct TuningConfig {
     int iterations;      // Total number of iterations 
     int population_size; // Number of parameter sets to evaluate
     int tournament_size; // Size of tournament for selection
     double mutation_rate;// Probability of mutation for each parameter
     int threads;         // Number of threads to use for evaluation
 };
 
 // Individual parameter set and its fitness
 struct Individual {
     std::vector<int> params;  // Parameter values
     double fitness;           // Lower is better (nodes searched or time taken)
 
     Individual() : fitness(std::numeric_limits<double>::max()) {}
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
         {"RFP_MARGIN", TunableParams::RFP_MARGIN, 50, 100, 5},
         {"RFP_DEPTH", TunableParams::RFP_DEPTH, 3, 8, 1},
         {"RFP_IMPROVING_BONUS", TunableParams::RFP_IMPROVING_BONUS, 30, 90, 5},
         {"LMR_BASE", TunableParams::LMR_BASE, 50, 100, 5},
         {"LMR_DIVISION", TunableParams::LMR_DIVISION, 150, 300, 10},
         {"NMP_BASE", TunableParams::NMP_BASE, 1, 5, 1},
         {"NMP_DIVISION", TunableParams::NMP_DIVISION, 1, 5, 1},
         {"NMP_MARGIN", TunableParams::NMP_MARGIN, 100, 250, 10},
         {"LMP_DEPTH_THRESHOLD", TunableParams::LMP_DEPTH_THRESHOLD, 5, 10, 1},
         {"FUTILITY_MARGIN", TunableParams::FUTILITY_MARGIN, 100, 250, 10},
         {"FUTILITY_DEPTH", TunableParams::FUTILITY_DEPTH, 4, 8, 1},
         {"FUTILITY_IMPROVING", TunableParams::FUTILITY_IMPROVING, 15, 35, 2},
         {"QS_FUTILITY_MARGIN", TunableParams::QS_FUTILITY_MARGIN, 120, 230, 10},
         {"SEE_QUIET_MARGIN_BASE", TunableParams::SEE_QUIET_MARGIN_BASE, -100, -50, 5},
         {"SEE_NOISY_MARGIN_BASE", TunableParams::SEE_NOISY_MARGIN_BASE, -25, -5, 1},
         {"ASPIRATION_DELTA", TunableParams::ASPIRATION_DELTA, 8, 18, 1},
         {"HISTORY_PRUNING_THRESHOLD", TunableParams::HISTORY_PRUNING_THRESHOLD, 3000, 5000, 250}
     };
 }
 
 // Custom positions for evaluation
 std::vector<std::string> testPositions = {
     "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
     "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3",
     "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
     "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
     "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
     "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9",
     "2rq1rk1/pp1bppbp/2np1np1/8/2BNP3/2N1BP2/PPPQ2PP/1K1R3R b - - 8 11"
 };
 
 // Function to evaluate a set of parameters
 double evaluateParameters(const std::vector<int>& params, const std::vector<Parameter>& paramDefs, int searchDepth) {
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
 
     // Measure the performance by processing test positions
     double totalNodes = 0;
     auto startTime = std::chrono::high_resolution_clock::now();
 
     for (const auto& fen : testPositions) {
         SearchThread st;
         st.applyFen(fen);
         st.clear();
         
         // Search to a fixed depth
         SearchStack stack[MAXPLY + 10], *ss = stack + 7;
         auto nodes_before = st.nodes;
         negamax(-INF_BOUND, INF_BOUND, searchDepth, st, ss, false);
         totalNodes += (st.nodes - nodes_before);
     }
 
     auto endTime = std::chrono::high_resolution_clock::now();
     auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
 
     // We want to minimize total nodes searched while maintaining strength
     return totalNodes;
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
         ind.params[i] = dist(rng) / params[i].step_size * params[i].step_size;
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
             ind.params[i] = value_dist(rng) / params[i].step_size * params[i].step_size;
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
     best.fitness = std::numeric_limits<double>::max();
     
     for (int i = 0; i < tournamentSize; i++) {
         int idx = dist(rng);
         if (population[idx].fitness < best.fitness) {
             best = population[idx];
         }
     }
     
     return best;
 }
 
 // Run the genetic algorithm for parameter tuning
 void tuneParameters(const TuningConfig& config) {
     // Initialize parameters
     auto paramDefs = createParameters();
     
     // Setup random number generator
     std::random_device rd;
     std::mt19937 rng(rd());
     
     // Create initial population
     std::vector<Individual> population(config.population_size);
     std::cout << "Creating initial population..." << std::endl;
     for (int i = 0; i < config.population_size; i++) {
         population[i] = createRandomIndividual(paramDefs, rng);
     }
     
     // Evaluate initial population
     std::cout << "Evaluating initial population..." << std::endl;
     for (auto& ind : population) {
         ind.fitness = evaluateParameters(ind.params, paramDefs, 6); // Search depth of 6
     }
     
     // Sort by fitness (lower is better)
     std::sort(population.begin(), population.end(), [](const Individual& a, const Individual& b) {
         return a.fitness < b.fitness;
     });
 
     // Best individual so far
     Individual bestEver = population[0];
     std::cout << "Initial best fitness: " << bestEver.fitness << std::endl;
     
     // Main optimization loop
     for (int gen = 0; gen < config.iterations; gen++) {
         std::vector<Individual> newPopulation;
         
         // Elitism - keep best individual
         newPopulation.push_back(population[0]);
         
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
         
         // Evaluate new population
         for (auto& ind : newPopulation) {
             if (ind.fitness == std::numeric_limits<double>::max()) {
                 ind.fitness = evaluateParameters(ind.params, paramDefs, 6); // Search depth of 6
             }
         }
         
         // Replace old population
         population = newPopulation;
         
         // Sort by fitness
         std::sort(population.begin(), population.end(), [](const Individual& a, const Individual& b) {
             return a.fitness < b.fitness;
         });
         
         // Update best individual
         if (population[0].fitness < bestEver.fitness) {
             bestEver = population[0];
         }
         
         std::cout << "Generation " << gen + 1 << "/" << config.iterations 
                   << ", Best fitness: " << population[0].fitness 
                   << ", All-time best: " << bestEver.fitness << std::endl;
         
         // Output current best parameters
         std::cout << "Current best parameters:" << std::endl;
         for (size_t i = 0; i < paramDefs.size(); i++) {
             std::cout << paramDefs[i].name << ": " << population[0].params[i] << std::endl;
         }
         
         // Save current best to file
         std::ofstream outFile("best_params_gen_" + std::to_string(gen + 1) + ".txt");
         for (size_t i = 0; i < paramDefs.size(); i++) {
             outFile << paramDefs[i].name << " " << population[0].params[i] << std::endl;
         }
         outFile.close();
     }
     
     // Final output
     std::cout << "\nOptimization complete!\n";
     std::cout << "Best parameters found:" << std::endl;
     for (size_t i = 0; i < paramDefs.size(); i++) {
         std::cout << paramDefs[i].name << ": " << bestEver.params[i] << std::endl;
     }
     
     // Save best parameters to file
     std::ofstream bestFile("best_params.txt");
     for (size_t i = 0; i < paramDefs.size(); i++) {
         bestFile << paramDefs[i].name << " " << bestEver.params[i] << std::endl;
     }
     bestFile.close();
     
     std::cout << "Parameters saved to best_params.txt" << std::endl;
 }
 
 int main(int argc, char* argv[]) {
     // Initialize the transposition table
     table = new TranspositionTable();
     table->Initialize(16); // 16MB default size
 
     // Default configuration
     TuningConfig config = {
         .iterations = 10,         // Just do 10 generations by default
         .population_size = 10,    // Small population for easier testing
         .tournament_size = 3,     // Tournament selection size
         .mutation_rate = 0.2,     // 20% chance to mutate each parameter
         .threads = 1              // Single threaded by default
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
         } else if (arg == "--threads") {
             if (i + 1 < argc) config.threads = std::stoi(argv[++i]);
         } else if (arg == "-h" || arg == "--help") {
             std::cout << "Usage: tune [options]\n"
                       << "Options:\n"
                       << "  -i, --iterations N    Set number of iterations (default: 10)\n"
                       << "  -p, --population N    Set population size (default: 10)\n"
                       << "  -t, --tournament N    Set tournament size (default: 3)\n"
                       << "  -m, --mutation N      Set mutation rate (default: 0.2)\n"
                       << "      --threads N       Set number of threads (default: 1)\n"
                       << "  -h, --help            Show this help message\n";
             return 0;
         }
     }
     
     // Initialize LMR tables
     initLateMoveTable();
     
     // Start parameter tuning
     std::cout << "Starting parameter tuning with:\n"
               << "  Iterations:      " << config.iterations << "\n"
               << "  Population size: " << config.population_size << "\n"
               << "  Tournament size: " << config.tournament_size << "\n"
               << "  Mutation rate:   " << config.mutation_rate << "\n"
               << "  Threads:         " << config.threads << "\n\n";
     
     tuneParameters(config);
     
     // Cleanup
     delete table;
     
     return 0;
 }