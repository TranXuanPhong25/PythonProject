#pragma once

#include "chess.hpp"
#include "types.hpp"
#include "evaluate.hpp"
#include "tt.hpp"
#include "see.hpp"
#include <memory.h>
#include <algorithm>
#include <math.h>
using namespace Chess;
using HistoryTable = std::array<std::array<int16_t, NSQUARES>, NPIECES>;

const int RFPMargin = 75;
const int RFPDepth = 5;
const int RFPImprovingBonus = 62;
const int LMRBase = 75;
const int LMRDivision = 225;

const int NMPBase = 3;
const int NMPDivision = 3;
const int NMPMargin = 180;

extern TranspositionTable *table;
struct SearchStack
{
   int16_t staticEval{};
   uint8_t ply{};

   Move move{NO_MOVE};
   Move killers[2] = {NO_MOVE, NO_MOVE};

   Piece movedPice{None};

   int staticScore = 0;
   HistoryTable *continuationHistory;
};

// A struct to hold the search data
struct SearchThread
{
   Board board;
   HistoryTable searchHistory;
   HistoryTable continuationHistory[NPIECES][NSQUARES];

   uint64_t nodes = 0;
   Move bestMove = NO_MOVE;

   SearchThread() : board(DEFAULT_POS)
   {
      clear();
   }

   // Applying fen on board
   inline void applyFen(std::string fen)
   {
      board.applyFen(fen);
   }
   inline void clear()
   {

      nodes = 0;
      memset(searchHistory.data(), 0, sizeof(searchHistory));
      memset(continuationHistory, 0, sizeof(continuationHistory));
   }
};

// Global search stats object
void initLateMoveTable();
int negamax(int alpha, int beta, int depth, SearchThread &st, SearchStack *ss, bool cutnode);
int quiescence(int alpha, int beta, SearchThread &st, SearchStack *ss);
void iterativeDeepening(SearchThread &st, const int &maxDepth);
int aspirationWindow(int prevEval, int depth, SearchThread& st, Move& bestmove);