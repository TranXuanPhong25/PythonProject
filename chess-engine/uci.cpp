#include "uci.h"
#include "search.h"
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
    std::cout << "Rice Copyright (C) 2023  Rafid Ahsan" << std::endl;

    auto ttable = std::make_unique<TranspositionTable>();
    table = ttable.get();
    table->Initialize(DefaultHashSize);



    std::string command;
    std::string token;

    while (std::getline(std::cin, command)) {
        std::istringstream is(command);

        token.clear();
        is >> std::skipws >> token;

        if (token == "stop") {

        } else if (token == "quit") {
 
            break;
        } else if (token == "isready") {

            std::cout << "readyok" << std::endl;
            continue;

        } else if (token == "ucinewgame") {
            table->Initialize(CurrentHashSize);
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
                // searchThread->applyFen(DEFAULT_POS);
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

                // record fifty move conter
                final_fen.push_back(' ');
                is >> std::skipws >> option;
                final_fen += option;

                final_fen.push_back(' ');
                is >> std::skipws >> option;
                final_fen += option;

                // Finally, apply the fen.
                // searchThread->applyFen(final_fen);
            }
            is >> std::skipws >> option;
            if (option == "moves") {
                std::string moveString;

                while (is >> moveString) {
                    // searchThread->makeMove(moveString);
                }
            }
            continue;
        }

        /* Handle UCI go command */
        else if (token == "go") {
            is >> std::skipws >> token;

            // Initialize variables
            int depth = -1;

            uint64_t nodes = -1;

            while (token != "none") {
                if (token == "infinite") {
                    depth = -1;
                    break;
                }
                if (token == "movestogo") {
                    is >> std::skipws >> token;
                    // searchThread->tm.movestogo = stoi(token);
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

                // Time
                if (token == "wtime") {
                    is >> std::skipws >> token;
                    // searchThread->tm.wtime = std::stod(token);
                    is >> std::skipws >> token;
                    continue;
                }
                if (token == "btime") {
                    is >> std::skipws >> token;
                    // searchThread->tm.btime = std::stod(token);
                    is >> std::skipws >> token;
                    continue;
                }

                // Increment
                if (token == "winc") {
                    is >> std::skipws >> token;
                    // searchThread->tm.winc = std::stod(token);
                    is >> std::skipws >> token;
                    continue;
                }
                if (token == "binc") {
                    is >> std::skipws >> token;
                    // searchThread->tm.binc = std::stod(token);
                    is >> std::skipws >> token;
                    continue;
                }

                if (token == "movetime") {
                    is >> std::skipws >> token;
                    // searchThread->tm.movetime = stod(token);
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

            if (nodes != -1) {
                // info.nodes = nodes;
                // info.nodeset = true;
            }

            // info.depth = depth;
            // if (searchThread->tm.wtime != -1 || searchThread->tm.btime != -1 || searchThread->tm.movetime != -1) {
                // info.timeset = true;
            // }

            // if (depth == -1) {
                // info.depth = MAXPLY;
            // }

            // info.stopped = false;
            // info.uci = IsUci;

            // threadHandle.start(*searchThread);
        }

        else if (token == "setoption") {
        
        }

        /* Debugging Commands */
        else if (token == "print") {
            // std::cout << searchThread->board << std::endl;
            continue;
        } else if (token == "eval") {
            
            // std::cout << "Eval: " << evaluate(*searchThread) << std::endl;

        } else if (token == "repetition") {

            // std::cout << searchThread->board.isRepetition() << std::endl;

        } else if (token == "side") {

            // std::cout << (searchThread->board.sideToMove == White ? "White" : "Black") << std::endl;

        }
    }

    table->clear();

    std::cout << std::endl;
    // if (!info.uci) {
    //     std::cout << "\u001b[0m";
    // }
}