#pragma once
#include "chess.hpp"
#include "types.hpp"
#include "see.hpp"
#include "search.hpp"
#define MAXHISTORY 16384
#define MAXCOUNTERHISTORY 16384
constexpr int mvv_lva[12][12] = {
   105, 205, 305, 405, 505, 605, 105, 205, 305, 405, 505, 605, 104, 204, 304,
   404, 504, 604, 104, 204, 304, 404, 504, 604, 103, 203, 303, 403, 503, 603,
   103, 203, 303, 403, 503, 603, 102, 202, 302, 402, 502, 602, 102, 202, 302,
   402, 502, 602, 101, 201, 301, 401, 501, 601, 101, 201, 301, 401, 501, 601,
   100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600,
   105, 205, 305, 405, 505, 605, 105, 205, 305, 405, 505, 605, 104, 204, 304,
   404, 504, 604, 104, 204, 304, 404, 504, 604, 103, 203, 303, 403, 503, 603,
   103, 203, 303, 403, 503, 603, 102, 202, 302, 402, 502, 602, 102, 202, 302,
   402, 502, 602, 101, 201, 301, 401, 501, 601, 101, 201, 301, 401, 501, 601,
   100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600
};

void scoreMoves(SearchThread& st, Movelist &moves, SearchStack *ss, Move tt_move);
void scoreMovesForQS(Board &board, Movelist &moves, Move tt_move);
void pickNextMove(const int& index, Movelist &moves);
void updateContinuationHistories(SearchStack* ss, Piece piece, Move move, int bonus);
void updateHistories(SearchThread& st, SearchStack *ss, Move bestmove, Movelist &quietList, int depth);

inline int historyBonus(const int& depth){
   return std::min(2100, 300 * depth - 300);
}

int getHistoryScores(int& his, int& ch, int& fmh, SearchThread& st, SearchStack *ss, const Move move);