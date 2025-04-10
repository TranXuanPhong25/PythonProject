#include "score_move.hpp"
Move killerMoves[MAX_PLY][2] = {{NO_MOVE}};
int historyTable[2][64][64] = {{{0}}};

// Function to add a new killer move
void addKillerMove(Move move, int ply) {
    // Don't add the same killer twice
    if (move != killerMoves[ply][0]) {
        // Shift existing killer
        killerMoves[ply][1] = killerMoves[ply][0];
        // Add new first killer
        killerMoves[ply][0] = move;
    }
}

// Update history score for a good move
void updateHistory(Board &board, Move move, int depth) {
    int side = board.sideToMove == White ? 0 : 1;
    int from_sq = from(move);
    int to_sq = to(move);
    
    // Bonus value proportional to depth
    int bonus = depth * depth;
    
    // Add bonus to history score
    historyTable[side][from_sq][to_sq] += bonus;
    
    // Prevent overflow by scaling down when scores get too high
    if (historyTable[side][from_sq][to_sq] > 16000) {
        // Scale down all history scores
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 64; j++) {
                for (int k = 0; k < 64; k++) {
                    historyTable[i][j][k] /= 2;
                }
            }
        }
    }
}

// Clear history scores between games
void clearHistory() {
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 64; j++) {
            for (int k = 0; k < 64; k++) {
                historyTable[i][j][k] = 0;
            }
        }
    }
    
    // Also clear killer moves
    for (int i = 0; i < MAX_PLY; i++) {
        killerMoves[i][0] = NO_MOVE;
        killerMoves[i][1] = NO_MOVE;
    }
}

void scoreMoves(Movelist &moves, Board &board, Move ttMove,int ply)
{
    for (int i = 0; i < moves.size; ++i)
    {
        Move move = moves[i].move;
        int score = 0;

        Piece attacker = board.pieceAtB(from(move));
        Piece victim = board.pieceAtB(to(move));
        // Prioritize TT move
        if (move == ttMove)
        {
            score = 20000;
        }
        // capture
        else if (victim != None)
        {
            score += mvv_lva[attacker][victim];
            // TODO: implement SEE
            // SEE = static exchange evaluation
            //  Winning captures (SEE > 0) get bonus
            score +=  (GoodCaptureScore * see(board, move, -107));;
        }

        // promotions
        if (promoted(move))
        {
            PieceType promoType = piece(move);
            switch (promoType) {
                case QUEEN:  score = 9900; break;
                case ROOK:   score = 9800; break;
                case BISHOP: score = 9700; break;
                case KNIGHT: score = 9600; break;
                default: break;
            }
        }

        else if (ply < MAX_PLY) {
            if (move == killerMoves[ply][0]) {
                score = 9000;
            } else if (move == killerMoves[ply][1]) {
                score = 8900;
            }
        }
        // 5. History heuristic (quiets that have been good in the past)
        else {
            int side = board.sideToMove == White ? 0 : 1;
            score = historyTable[side][from(move)][to(move)];
        }

        moves[i].value = score;
    }
}
