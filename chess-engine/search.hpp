#pragma once

#include "chess.hpp"
#include "types.hpp"
#include "evaluate.hpp"
#include "score_move.hpp"
#include "tt.hpp"
using namespace Chess;

// Simple evaluation function - just counts material
int evaluate(const Board &board);

// Minimax search with alpha-beta pruning
int negamax(Board &board, int depth, int alpha, int beta,TranspositionTable *table, int ply =0);

// Function to get the best move at a given depth
Move getBestMove(Board &board, int depth, TranspositionTable *table);

