#pragma once

#include "chess.hpp"
#include "types.h"
#include "evaluate.hpp"
#include "score_move.hpp"
using namespace Chess;

// Simple evaluation function - just counts material
int evaluate(const Board &board);

// Minimax search with alpha-beta pruning
int negamax(Board &board, int depth, int alpha, int beta);

// Function to get the best move at a given depth
Move getBestMove(Board &board, int depth);

