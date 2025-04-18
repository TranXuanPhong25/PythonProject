#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <random>
#include <chrono>
#include <cmath>
#include <memory>
#include <iomanip>
#include "chess.hpp"
#include "search.hpp"
#include "tunable_params.hpp"
#include "tt.hpp"

// Global TranspositionTable pointer needed by the engine
TranspositionTable* table = nullptr;

using namespace Chess;

// Test positions for starting the games (variety of openings)
const std::vector<std::string> startPositions = {
    // Standard chess start position
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    
    // Sicilian Defense
    "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2",
    
    // French Defense
    "rnbqkbnr/pppp1ppp/4p3/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
    
    // Caro-Kann Defense
    "rnbqkbnr/pp1ppppp/2p5/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
    
    // Queen's Gambit
    "rnbqkbnr/ppp1pppp/8/3p4/2PP4/8/PP2PPPP/RNBQKBNR b KQkq d3 0 2"
};

// Game result constants
enum GameResult {
    WHITE_WIN = 1,
    BLACK_WIN = -1,
    DRAW = 0,
    IN_PROGRESS = 2
};

// Structure to hold a parameter set
struct ParamSet {
    std::string name;
    std::map<std::string, int> params;
    
    ParamSet(std::string _name) : name(_name) {}
    
    void loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return;
        }
        
        std::string name;
        int value;
        while (file >> name >> value) {
            params[name] = value;
        }
    }
    
    void applyParams() {
        // Apply these parameters to the engine
        for (const auto& [name, value] : params) {
            if (name == "RFP_MARGIN") TunableParams::RFP_MARGIN = value;
            else if (name == "RFP_DEPTH") TunableParams::RFP_DEPTH = value;
            else if (name == "RFP_IMPROVING_BONUS") TunableParams::RFP_IMPROVING_BONUS = value;
            else if (name == "LMR_BASE") TunableParams::LMR_BASE = value;
            else if (name == "LMR_DIVISION") TunableParams::LMR_DIVISION = value;
            else if (name == "NMP_BASE") TunableParams::NMP_BASE = value;
            else if (name == "NMP_DIVISION") TunableParams::NMP_DIVISION = value;
            else if (name == "NMP_MARGIN") TunableParams::NMP_MARGIN = value;
            else if (name == "LMP_DEPTH_THRESHOLD") TunableParams::LMP_DEPTH_THRESHOLD = value;
            else if (name == "FUTILITY_MARGIN") TunableParams::FUTILITY_MARGIN = value;
            else if (name == "FUTILITY_DEPTH") TunableParams::FUTILITY_DEPTH = value;
            else if (name == "FUTILITY_IMPROVING") TunableParams::FUTILITY_IMPROVING = value;
            else if (name == "QS_FUTILITY_MARGIN") TunableParams::QS_FUTILITY_MARGIN = value;
            else if (name == "SEE_QUIET_MARGIN_BASE") TunableParams::SEE_QUIET_MARGIN_BASE = value;
            else if (name == "SEE_NOISY_MARGIN_BASE") TunableParams::SEE_NOISY_MARGIN_BASE = value;
            else if (name == "ASPIRATION_DELTA") TunableParams::ASPIRATION_DELTA = value;
            else if (name == "HISTORY_PRUNING_THRESHOLD") TunableParams::HISTORY_PRUNING_THRESHOLD = value;
        }
        
        // Reinitialize tables that depend on these parameters
        initLateMoveTable();
    }
};

// Check for game termination
GameResult checkGameResult(Board& board, int moveCounter, int halfMoveClock) {
    // Check for checkmate
    bool inCheck = board.isSquareAttacked(~board.sideToMove, board.KingSQ(board.sideToMove));
    Movelist moves;
    if (board.sideToMove == White) {
        Movegen::legalmoves<White, ALL>(board, moves);
    } else {
        Movegen::legalmoves<Black, ALL>(board, moves);
    }
    
    if (moves.size == 0) {
        if (inCheck) {
            return (board.sideToMove == White) ? BLACK_WIN : WHITE_WIN;
        } else {
            return DRAW; // Stalemate
        }
    }
    
    // 50-move rule
    if (halfMoveClock >= 100) {
        return DRAW;
    }
    
    // Move limit (to prevent endless games)
    if (moveCounter >= 200) {
        return DRAW;
    }
    
    // Basic draw check - simplify for our needs
    // Just check if there are only kings left or insufficient material
    if (board.nonPawnMat(White) == 0 && board.nonPawnMat(Black) == 0) {
        // Count pawns for each side
        int whitePawns = popcount(board.pieces( PAWN,White));
        int blackPawns = popcount(board.pieces( PAWN,Black));
        
        if (whitePawns == 0 && blackPawns == 0) {
            return DRAW; // Only kings left
        }
    }
    
    return IN_PROGRESS;
}

// Play a game between two parameter sets
GameResult playGame(ParamSet& white, ParamSet& black, int searchDepth, std::string& pgn, std::string startFen = "") {
    // Reset the transposition table
    table->clear();
    
    // Create a new board, optionally from a starting position
    Board board = startFen.empty() ? Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") : Board(startFen);
    
    // Game state
    int moveCounter = 1;
    int halfMoveClock = 0;
    std::vector<std::string> moves;
    
    // Initialize PGN
    pgn = "[Event \"Parameter Test Match\"]\n";
    pgn += "[Site \"Local Engine\"]\n";
    pgn += "[Date \"" + std::to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())) + "\"]\n";
    pgn += "[Round \"1\"]\n";
    pgn += "[White \"" + white.name + "\"]\n";
    pgn += "[Black \"" + black.name + "\"]\n";
    pgn += "[Result \"*\"]\n\n";
    
    // Start the game
    while (true) {
        // Check for game termination
        GameResult result = checkGameResult(board, moveCounter, halfMoveClock);
        if (result != IN_PROGRESS) {
            // Update PGN result
            if (result == WHITE_WIN) {
                pgn.replace(pgn.find("[Result \"*\"]"), 12, "[Result \"1-0\"]");
                pgn += " 1-0";
            } else if (result == BLACK_WIN) {
                pgn.replace(pgn.find("[Result \"*\"]"), 12, "[Result \"0-1\"]");
                pgn += " 0-1";
            } else {
                pgn.replace(pgn.find("[Result \"*\"]"), 12, "[Result \"1/2-1/2\"]");
                pgn += " 1/2-1/2";
            }
            return result;
        }
        
        // Apply correct parameter set based on side to move
        if (board.sideToMove == White) {
            white.applyParams();
        } else {
            black.applyParams();
        }
        
        // Search for best move
        SearchThread st;
        st.board = board;
        
        SearchStack stack[MAXPLY + 10], *ss = stack + 7;
        negamax(-INF_BOUND, INF_BOUND, searchDepth, st, ss, false);
        Move bestMove = st.bestMove;
        
        // Convert move to algebraic notation for PGN
        std::string moveStr = convertMoveToUci(bestMove);
        moves.push_back(moveStr);
        
        // Format PGN move
        if (board.sideToMove == White) {
            pgn += std::to_string(moveCounter) + ". " + moveStr + " ";
        } else {
            pgn += moveStr + " ";
            moveCounter++;
        }
        
        // Update half-move clock (for 50-move rule)
        Piece capturedPiece = board.pieceAtB(to(bestMove));
        Piece movedPiece = board.pieceAtB(from(bestMove));
        bool isPawn = (movedPiece == WhitePawn) || (movedPiece == BlackPawn);
        
        if (capturedPiece != None || isPawn) {
            halfMoveClock = 0;  // Reset on capture or pawn move
        } else {
            halfMoveClock++;
        }
        
        // Make the move
        board.makeMove(bestMove);
        
        // Add some newlines in PGN for readability
        if (moves.size() % 10 == 0) {
            pgn += "\n";
        }
    }
    
    return IN_PROGRESS;  // Should never reach here
}

// Calculate ELO difference based on score
double calculateElo(double score, int games) {
    if (score == 0) return -400;  // Complete loss
    if (score == games) return 400;  // Complete win
    
    double percentage = score / games;
    return -400.0 * log10(1.0 / percentage - 1.0);
}

// Main bot match function
void runMatch(const std::string& paramFile1, const std::string& paramFile2, int numGames, int searchDepth) {
    // Load parameter sets
    ParamSet params1(paramFile1);
    params1.loadFromFile(paramFile1);
    
    ParamSet params2(paramFile2);
    params2.loadFromFile(paramFile2);
    
    // Game statistics
    int whiteWins = 0, blackWins = 0, draws = 0;
    double score1 = 0, score2 = 0;
    
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> dist(0, startPositions.size() - 1);
    
    // Create a log file for the match
    std::ofstream logFile("bot_match_results.txt");
    logFile << "Bot Match Results" << std::endl;
    logFile << "================" << std::endl;
    logFile << "Parameter Set 1: " << paramFile1 << std::endl;
    logFile << "Parameter Set 2: " << paramFile2 << std::endl;
    logFile << "Search Depth: " << searchDepth << std::endl;
    logFile << "Number of Games: " << numGames << std::endl << std::endl;
    
    // Start the match
    std::cout << "Starting " << numGames << " games at depth " << searchDepth << "..." << std::endl;
    
    for (int i = 0; i < numGames; i++) {
        std::cout << "Playing game " << (i + 1) << "..." << std::endl;
        
        // Choose a random starting position
        std::string startFen = startPositions[dist(rng)];
        
        // Alternate who plays white and black
        ParamSet &white = (i % 2 == 0) ? params1 : params2;
        ParamSet &black = (i % 2 == 0) ? params2 : params1;
        
        std::string pgn;
        GameResult result = playGame(white, black, searchDepth, pgn, startFen);
        
        // Update statistics
        if (result == WHITE_WIN) {
            whiteWins++;
            if (i % 2 == 0) score1 += 1.0;
            else score2 += 1.0;
            std::cout << "Game " << (i + 1) << ": White (" << white.name << ") wins" << std::endl;
            logFile << "Game " << (i + 1) << ": White (" << white.name << ") wins" << std::endl;
        } else if (result == BLACK_WIN) {
            blackWins++;
            if (i % 2 == 0) score2 += 1.0;
            else score1 += 1.0;
            std::cout << "Game " << (i + 1) << ": Black (" << black.name << ") wins" << std::endl;
            logFile << "Game " << (i + 1) << ": Black (" << black.name << ") wins" << std::endl;
        } else {
            draws++;
            score1 += 0.5;
            score2 += 0.5;
            std::cout << "Game " << (i + 1) << ": Draw" << std::endl;
            logFile << "Game " << (i + 1) << ": Draw" << std::endl;
        }
        
        // Save PGN to a file
        std::ofstream pgnFile("game_" + std::to_string(i + 1) + ".pgn");
        pgnFile << pgn;
        pgnFile.close();
        
        // Log the full PGN
        logFile << "PGN: " << std::endl << pgn << std::endl << std::endl;
    }
    
    // Calculate ELO difference
    double eloDiff = calculateElo(score1, numGames);
    
    // Report results
    std::cout << "\nMatch Results:" << std::endl;
    std::cout << "=============" << std::endl;
    std::cout << paramFile1 << ": " << score1 << " points" << std::endl;
    std::cout << paramFile2 << ": " << score2 << " points" << std::endl;
    std::cout << "White wins: " << whiteWins << ", Black wins: " << blackWins << ", Draws: " << draws << std::endl;
    std::cout << "ELO difference: " << std::fixed << std::setprecision(2) << eloDiff
              << " in favor of " << (eloDiff > 0 ? paramFile1 : paramFile2) << std::endl;
    
    logFile << "\nMatch Results:" << std::endl;
    logFile << "=============" << std::endl;
    logFile << paramFile1 << ": " << score1 << " points" << std::endl;
    logFile << paramFile2 << ": " << score2 << " points" << std::endl;
    logFile << "White wins: " << whiteWins << ", Black wins: " << blackWins << ", Draws: " << draws << std::endl;
    logFile << "ELO difference: " << std::fixed << std::setprecision(2) << eloDiff
            << " in favor of " << (eloDiff > 0 ? paramFile1 : paramFile2) << std::endl;
    
    logFile.close();
}

int main(int argc, char* argv[]) {
    // Initialize the transposition table
    table = new TranspositionTable();
    table->Initialize(128); // 128MB for matches
    
    // Default settings
    std::string paramFile1 = "benchmark_best_params.txt";
    std::string paramFile2 = "benchmark_initial_params.txt";
    int numGames = 5;
    int searchDepth = 13;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-p1" || arg == "--params1") {
            if (i + 1 < argc) paramFile1 = argv[++i];
        } else if (arg == "-p2" || arg == "--params2") {
            if (i + 1 < argc) paramFile2 = argv[++i];
        } else if (arg == "-g" || arg == "--games") {
            if (i + 1 < argc) numGames = std::stoi(argv[++i]);
        } else if (arg == "-d" || arg == "--depth") {
            if (i + 1 < argc) searchDepth = std::stoi(argv[++i]);
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: bot_match [options]\n"
                      << "Options:\n"
                      << "  -p1, --params1 FILE   Parameter set 1 (default: benchmark_best_params.txt)\n"
                      << "  -p2, --params2 FILE   Parameter set 2 (default: tunable_params_default.txt)\n"
                      << "  -g, --games N         Number of games to play (default: 5)\n"
                      << "  -d, --depth N         Search depth (default: 13)\n"
                      << "  -h, --help            Show this help message\n";
            return 0;
        }
    }
    
    // Create default parameter file if it doesn't exist
    std::ofstream defaultFile("tunable_params_default.txt");
    defaultFile << "RFP_MARGIN " << TunableParams::RFP_MARGIN << std::endl;
    defaultFile << "RFP_DEPTH " << TunableParams::RFP_DEPTH << std::endl;
    defaultFile << "RFP_IMPROVING_BONUS " << TunableParams::RFP_IMPROVING_BONUS << std::endl;
    defaultFile << "LMR_BASE " << TunableParams::LMR_BASE << std::endl;
    defaultFile << "LMR_DIVISION " << TunableParams::LMR_DIVISION << std::endl;
    defaultFile << "NMP_BASE " << TunableParams::NMP_BASE << std::endl;
    defaultFile << "NMP_DIVISION " << TunableParams::NMP_DIVISION << std::endl;
    defaultFile << "NMP_MARGIN " << TunableParams::NMP_MARGIN << std::endl;
    defaultFile << "LMP_DEPTH_THRESHOLD " << TunableParams::LMP_DEPTH_THRESHOLD << std::endl;
    defaultFile << "FUTILITY_MARGIN " << TunableParams::FUTILITY_MARGIN << std::endl;
    defaultFile << "FUTILITY_DEPTH " << TunableParams::FUTILITY_DEPTH << std::endl;
    defaultFile << "FUTILITY_IMPROVING " << TunableParams::FUTILITY_IMPROVING << std::endl;
    defaultFile << "QS_FUTILITY_MARGIN " << TunableParams::QS_FUTILITY_MARGIN << std::endl;
    defaultFile << "SEE_QUIET_MARGIN_BASE " << TunableParams::SEE_QUIET_MARGIN_BASE << std::endl;
    defaultFile << "SEE_NOISY_MARGIN_BASE " << TunableParams::SEE_NOISY_MARGIN_BASE << std::endl;
    defaultFile << "ASPIRATION_DELTA " << TunableParams::ASPIRATION_DELTA << std::endl;
    defaultFile << "HISTORY_PRUNING_THRESHOLD " << TunableParams::HISTORY_PRUNING_THRESHOLD << std::endl;
    defaultFile.close();
    
    std::cout << "Starting bot match:" << std::endl;
    std::cout << "Parameter Set 1: " << paramFile1 << std::endl;
    std::cout << "Parameter Set 2: " << paramFile2 << std::endl;
    std::cout << "Games: " << numGames << ", Depth: " << searchDepth << std::endl;
    
    // Initialize LMR tables
    initLateMoveTable();
    
    // Run the match
    runMatch(paramFile1, paramFile2, numGames, searchDepth);
    
    // Cleanup
    delete table;
    
    return 0;
}