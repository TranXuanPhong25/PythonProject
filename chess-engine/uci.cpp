#include "uci.h"
#include "search.hpp"
#include "tt.h"
#include "types.h"

#include <chrono>
#include <iostream>
#include <sstream>
#include <bits/unique_ptr.h>

static void uci_send_id() {
    std::cout << "id name " << NAME << std::endl;
    std::cout << "id author " << AUTHOR << std::endl;
    std::cout << "option name Hash type spin default 64 min 4 max " << MAXHASH << std::endl;
    std::cout << "option name Threads type spin default 1 min 1 max 1" << std::endl;
    std::cout << "uciok" << std::endl;
}

static void set_option(std::istream &is, std::string &token, std::string name, int &value) {
    if (token == name) {
        is >> std::skipws >> token;
        is >> std::skipws >> token;
        value = std::stoi(token);
    }
}

int DefaultHashSize = 64;
int CurrentHashSize = DefaultHashSize;
int LastHashSize = CurrentHashSize;

bool IsUci = false;

TranspositionTable *table;

void uci_loop(int argv, char **argc) {
    std::cout << "Chess Engine Copyright (C) 2023" << std::endl;

    auto ttable = std::make_unique<TranspositionTable>();
    table = ttable.get();
    table->Initialize(DefaultHashSize);

    // Create our board instance
    Board board(DEFAULT_POS);
    int default_depth = 4; // Default search depth

    std::string command;
    std::string token;

    while (std::getline(std::cin, command)) {
        std::istringstream is(command);

        token.clear();
        is >> std::skipws >> token;

        if (token == "stop") {
            // For a simple non-threaded implementation, this does nothing
        } else if (token == "quit") {
            break;
        } else if (token == "isready") {
            std::cout << "readyok" << std::endl;
            continue;
        } else if (token == "ucinewgame") {
            table->Initialize(CurrentHashSize);
            board = Board(DEFAULT_POS); // Reset the board
            continue;
        } else if (token == "uci") {
            IsUci = true;
            uci_send_id();
            continue;
        }

        /* Handle UCI position command */
        else if (token == "position") {
            std::string option;
            is >> std::skipws >> option;
            if (option == "startpos") {
                board = Board(DEFAULT_POS);
            } else if (option == "fen") {
                std::string final_fen;
                is >> std::skipws >> option;
                final_fen = option;

                // Record side to move
                final_fen.push_back(' ');
                is >> std::skipws >> option;
                final_fen += option;

                // Record castling
                final_fen.push_back(' ');
                is >> std::skipws >> option;
                final_fen += option;

                // record enpassant square
                final_fen.push_back(' ');
                is >> std::skipws >> option;
                final_fen += option;

                // record fifty move counter
                final_fen.push_back(' ');
                is >> std::skipws >> option;
                final_fen += option;

                final_fen.push_back(' ');
                is >> std::skipws >> option;
                final_fen += option;

                // Apply the FEN
                board = Board(final_fen);
            }
            
            is >> std::skipws >> option;
            if (option == "moves") {
                std::string moveString;

                while (is >> moveString) {
                    Move move = convertUciToMove(board, moveString);
                    if (move != NO_MOVE) {
                        board.makeMove( move);
                    }
                }
            }
            continue;
        }

        /* Handle UCI go command */
        else if (token == "go") {
            is >> std::skipws >> token;

            // Initialize variables
            int depth = default_depth; // Default to depth 4 if not specified

            uint64_t nodes = -1;

            while (token != "none") {
                if (token == "infinite") {
                    depth = 6; // Use depth 6 for infinite (can be adjusted)
                    break;
                }
                if (token == "movestogo") {
                    is >> std::skipws >> token;
                    is >> std::skipws >> token;
                    continue;
                }

                // Depth
                if (token == "depth") {
                    is >> std::skipws >> token;
                    depth = std::stoi(token);
                    is >> std::skipws >> token;
                    continue;
                }

                // Time management (simplified - we ignore time controls for now)
                if (token == "wtime" || token == "btime" || token == "winc" || token == "binc" || token == "movetime") {
                    is >> std::skipws >> token;
                    is >> std::skipws >> token;
                    continue;
                }
                
                if (token == "nodes") {
                    is >> std::skipws >> token;
                    nodes = stoi(token);
                    is >> std::skipws >> token;
                }
                
                token = "none";
            }

            // Start search for best move
            auto start_time = std::chrono::high_resolution_clock::now();

            // Perform the search with proper error handling
            Move bestMove = NO_MOVE;
            try {
                bestMove = getBestMove(board, depth);
            } catch (const std::exception& e) {
                std::cerr << "Error during search: " << e.what() << std::endl;
                bestMove = NO_MOVE;
            } catch (...) {
                std::cerr << "Unknown error during search" << std::endl;
                bestMove = NO_MOVE;
            }

            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

            // Output the best move
            if (bestMove != NO_MOVE) {
                std::cout << "info depth " << depth << " time " << duration << " nodes 0 score cp 0" << std::endl;
                std::cout << "bestmove " << convertMoveToUci(bestMove) << std::endl;
            } else {
                std::cout << "bestmove (none)" << std::endl;
            }
        }

        else if (token == "setoption") {
            is >> std::skipws >> token;
            if (token == "name") {
                is >> std::skipws >> token;
                if (token == "Hash") {
                    is >> std::skipws >> token; // Skip "value"
                    is >> std::skipws >> token;
                    CurrentHashSize = std::stoi(token);
                    table->Initialize(CurrentHashSize);
                }
            }
        }

        /* Debugging Commands */
        else if (token == "print") {
            std::cout << board << std::endl;
            continue;
        } else if (token == "eval") {
            std::cout << "Eval: " << evaluate(board) << std::endl;
        } else if (token == "side") {
            std::cout << (board.sideToMove == White ? "White" : "Black") << std::endl;
        }
    }

    table->clear();
    std::cout << std::endl;
}