#include "score_move.hpp"
void scoreMoves(Movelist &moves, Board &board, int depth, Move ttMove)
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
            score += 10000;
        }

        // promotions
        if (promoted(move))
        {
            PieceType promoType = piece(move);
            switch (promoType)
            {
            case QUEEN:
                score += 900;
                break;
            case ROOK:
                score += 500;
                break;
            case BISHOP:
                score += 330;
                break;
            case KNIGHT:
                score += 320;
                break;
            default:
                break;
            }
        }

        // 3. Killer moves (moves that caused beta cutoffs at same depth in another branch)
        // Can be implemented by tracking killers in search and checking here

        // 4. History heuristic (moves that have been good in the past)
        // Can be implemented by maintaining a history table

        // Store the score
        moves[i].value = score;
    }
}
