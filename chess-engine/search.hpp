#pragma once

#include "chess.hpp"
#include "types.hpp"
#include "evaluate.hpp"
#include "score_move.hpp"
#include "tt.hpp"
#include "see.hpp"
using namespace Chess;

// Node counting statistics
struct SearchStats
{
   uint64_t nodes;       // Regular search nodes
   uint64_t qnodes;      // Quiescence search nodes
   uint64_t nullCutoffs; // Successful null move cutoffs
   uint64_t ttCutoffs;   // Transposition table cutoffs
   std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

   SearchStats() { clear(); }

   void clear()
   {
      nodes = 0;
      qnodes = 0;
      nullCutoffs = 0;
      ttCutoffs = 0;
      startTime = std::chrono::high_resolution_clock::now();
   }

   uint64_t totalNodes() const { return nodes + qnodes; }

   double elapsedSeconds() const
   {
      auto now = std::chrono::high_resolution_clock::now();
      return std::chrono::duration<double>(now - startTime).count();
   }

   uint64_t nodesPerSecond() const
   {
      double seconds = elapsedSeconds();
      return seconds > 0 ? static_cast<uint64_t>(totalNodes() / seconds) : 0;
   }

   void printStats() const
   {
      std::cout << "Nodes: " << totalNodes()
                << " (" << nodes << " regular, " << qnodes << " qnodes)\n";
      std::cout << "NPS: " << nodesPerSecond() << " nodes/sec\n";
      std::cout << "TT cutoffs: " << ttCutoffs << " ("
                << (100.0 * ttCutoffs / totalNodes()) << "%)\n";
      std::cout << "Null cutoffs: " << nullCutoffs << " ("
                << (100.0 * nullCutoffs / nodes) << "% of regular nodes)\n";
   }
};

// Global search stats object
extern SearchStats searchStats;

int negamax(Board &board, int depth, int alpha, int beta, TranspositionTable *table, int ply = 0);

Move getBestMove(Board &board, int maxDepth, TranspositionTable *table);

Move getBestMoveIterativeWithScore(Board &board, int depth, TranspositionTable *table,
                                   int alpha, int beta, int *score);