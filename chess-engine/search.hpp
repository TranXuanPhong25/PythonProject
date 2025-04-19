#pragma once

#include "chess.hpp"
#include "types.hpp"
#include "evaluate.hpp"
#include "tt.hpp"
#include "see.hpp"
#include <memory.h>
#include <algorithm>
#include <math.h>
#include "timeman.hpp"
using namespace Chess;
using HistoryTable = std::array<std::array<int16_t, 64>, 13>;

const int RFPMargin = 75;
const int RFPDepth = 5;
const int RFPImprovingBonus = 62;
const int LMRBase = 75;
const int LMRDivision = 225;

const int NMPBase = 3;
const int NMPDivision = 3;
const int NMPMargin = 180;

extern TranspositionTable *table;
struct SearchInfo
{
   int32_t score = 0;
   uint8_t depth = 0;
   uint64_t nodes = 0;
   bool timeset = 0;
   bool stopped = 0;
   bool nodeset = 0;
   bool uci = 0;
};
struct SearchStack
{
   int16_t staticEval{};
   uint8_t ply{};

   Move move{NO_MOVE};
   Move killers[2] = {NO_MOVE, NO_MOVE};

   Piece movedPice{None};

   int staticScore;
   HistoryTable *continuationHistory;
};

// A struct to hold the search data
struct SearchThread
{
   SearchInfo &info;
   Board board;
   HistoryTable searchHistory;
   HistoryTable continuationHistory[13][64];
   uint64_t nodes = 0;
   Move bestMove = NO_MOVE;
   TimeMan tm;

   SearchThread(SearchInfo &i) : info(i), board(DEFAULT_POS)
   {
      clear();
   }

   inline void clear()
   {
      nodes = 0;

      memset(searchHistory.data(), 0, sizeof(searchHistory));
      memset(continuationHistory, 0, sizeof(continuationHistory));

      tm.reset();
   }

   inline void initialize()
   {
      tm.start_time = misc::tick();

      if (info.timeset)
      {
         tm.set_time(board.sideToMove);
      }
   }

   inline Time start_time()
   {
      return tm.start_time;
   }
   // Applying fen on board
   inline void applyFen(std::string fen)
   {
      board.applyFen(fen);
   }
 
   inline bool stop_early()
   {
      if (info.timeset && (tm.stop_search() || info.stopped))
      {
         return true;
      }
      else
      {
         return false;
      }
   }

   void check_time()
   {
      if ((info.timeset && tm.check_time()) || (info.nodeset && nodes >= info.nodes))
      {
         info.stopped = true;
      }
   }
};

// Global search stats object
void initLateMoveTable();
int negamax(int alpha, int beta, int depth, SearchThread &st, SearchStack *ss, bool cutnode);
int quiescence(int alpha, int beta, SearchThread &st, SearchStack *ss);

// Original non-templated function (kept for backward compatibility)
void iterativeDeepening(SearchThread &st, const int &maxDepth);

// New templated version with printInfo parameter
template <bool printInfo>
void iterativeDeepening(SearchThread &st, const int &maxDepth);

int aspirationWindow(int prevEval, int depth, SearchThread &st, Move &bestmove);