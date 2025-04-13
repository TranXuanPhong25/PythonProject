#pragma once
#include "chess.hpp"
#include "types.hpp"
#include "see.hpp"
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
extern Move killerMoves[MAX_PLY][2];

// History heuristic table - tracks "quiet" moves that have been good
// Index: [color][from][to]
extern int historyTable[2][64][64];

// Countermove history table - tracks moves that are good responses to opponent moves
// Index: [piece][to_square] - what move was good after opponent's last move
extern Move counterMoveTable[12][64];

// Continuation history - tracks moves played after other moves
// This is indexed by [piece][to_square][piece][to_square]
extern int continuationHistory[12][64][12][64];

void addKillerMove(Move move, int ply);

// Update history score for a move that caused a beta cutoff
void updateHistory(Board &board, Move move, int depth,bool isCutoff);

// Update countermove history for a move that was effective against last opponent move
void updateCounterMove(Board &board, Move lastMove, Move counterMove);

// Reset history scores (e.g., between games)
void clearHistory();


void scoreMoves(Movelist &moves, Board &board,Move ttMove = NO_MOVE,int ply =0);
void ScoreMovesForQS(Board &board, Movelist &list, Move tt_move) ;

void pickNextMove(const int& moveNum, Movelist &list);


