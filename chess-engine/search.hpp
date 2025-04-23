#pragma once

#include "chess.hpp"
#include "types.hpp"
#include "evaluate.hpp"
#include "tt.hpp"
#include "see.hpp"
#include <memory.h>
#include <algorithm>
#include <math.h>
#include <algorithm>
#include <cmath>
#include <math.h>
using namespace Chess;

// Maximum size for move history stack
constexpr int MAX_MOVE_HISTORY = 512;

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

// Move history entry for keeping track of previous moves
struct MoveHistoryEntry {
    Move move;
    U64 hashKey;

    MoveHistoryEntry() : move(NO_MOVE), hashKey(0) {}
    MoveHistoryEntry(Move m, U64 key) : move(m), hashKey(key) {}
};

// Continuation history - tracks moves played after other moves
// This is indexed by [piece][to_square][piece][to_square]
extern int continuationHistory[12][64][12][64];

// Add move to move history
void addMoveToHistory(Move move, U64 hashKey);

// Get last move from history
Move getLastMove();

// Clear move history
void clearMoveHistory();

// Global search stats object
void initLateMoveTable();
int negamax(int alpha, int beta, int depth, SearchThread &st, SearchStack *ss, bool cutnode);
int quiescence(int alpha, int beta, SearchThread &st, SearchStack *ss);

// Original non-templated function (kept for backward compatibility)
void iterativeDeepening(SearchThread &st, const int &maxDepth);

// New templated version with printInfo parameter
template<bool printInfo>
void iterativeDeepening(SearchThread &st, const int &maxDepth);

int aspirationWindow(int prevEval, int depth, SearchThread& st, Move& bestmove);