#pragma once

#include "chess.hpp"
#include "types.hpp"
#include "evaluate.hpp"
#include "score_move.hpp"
#include "tt.hpp"
#include "see.hpp"
using namespace Chess;

// Simple evaluation function - just counts material

// Minimax search with alpha-beta pruning
int negamax(Board &board, int depth, int alpha, int beta,TranspositionTable *table, int ply =0);

// Function to get the best move at a given depth
Move getBestMoveIterative(Board &board, int depth, TranspositionTable *table, 
   int alpha = -INF_BOUND, int beta = INF_BOUND);
   Move getBestMove(Board &board, int maxDepth, TranspositionTable *table);
// Function to get the best move at a given depth with score


Move getBestMoveIterativeWithScore(Board &board, int depth, TranspositionTable *table, 
   int alpha, int beta, int* score);