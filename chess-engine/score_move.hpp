#pragma once
#include "chess.hpp"
#include "evaluate.hpp"
#include "search.hpp"
#include "types.hpp"
#include "see.hpp"

// Move scoring categories - higher values = higher priority

constexpr int CounterMoveScore = 8800000;     // Counter move
constexpr int PromotionScore = 8700000;       // Promotion moves
constexpr int EqualCaptureScore = 8500000;    // Equal captures
constexpr int BadCaptureScore = 8000000;      // Bad captures (negative SEE)

// Capture scoring array - victim piece type (columns) vs attacker piece type (rows)
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

// Bonus for captures based on piece values
constexpr int PieceValues[7] = {0, 100, 320, 330, 500, 900, 20000};

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

// Killer move manipulation functions
void addKillerMove(Move move, int ply);

// Update history score for a move that caused a beta cutoff
void updateHistory(Board &board, Move move, int depth, bool isCutoff);

// Update countermove history for a move that was effective against last opponent move
void updateCounterMove(Board &board, Move lastMove, Move counterMove);

// Reset history scores (e.g., between games)
void clearHistory();

// Enhanced move ordering system
// Score all moves in a list for regular search
void scoreMoves(Movelist &moves, Board &board, Move ttMove = NO_MOVE, int ply = 0);

// Score capture moves for quiescence search
void ScoreMovesForQS(Board &board, Movelist &list, Move tt_move);

// Helper to pick the next best move from an already scored list
void pickNextMove(const int& moveNum, Movelist &list);
bool moveResolvesCheck(Board &board, Move move);

// Update mobility score for a specific move
void updateMobility(Board &board, Move move, int &mobilityScore, Color side);

// Clear move history between searches
void clearMoveHistory();

// Get the last move played
Move getLastMove();

// New function to score a single move (useful for incremental sorting)
int scoreSingleMove(Board &board, Move move, Move ttMove, int ply);

// Move stage enum for staged move selection
enum MoveStage {
    TT_MOVE,        // Transposition table move
    GOOD_CAPTURES,  // Captures with positive SEE
    KILLER_MOVES,   // Killer moves
    COUNTER_MOVES,  // Counter moves
    QUIET_MOVES,    // Quiet moves with good history
    BAD_CAPTURES,   // Captures with negative SEE
    REMAINING_MOVES // Any other moves
};

// Class for staged move generation
class StagedMoveGenerator {
private:
    Board &board;
    Movelist &moves;
    int currentStage;
    int currentMoveIndex;
    Move ttMove;
    int ply;
    bool ttMoveSearched;
    
public:
    StagedMoveGenerator(Board &b, Movelist &m, Move tt = NO_MOVE, int p = 0);
    Move nextMove();
    bool hasNext() const;
};


