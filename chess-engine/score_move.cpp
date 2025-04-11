#include "score_move.hpp"
Move killerMoves[MAX_PLY][2] = {{NO_MOVE}};
int historyTable[2][64][64] = {{{0}}};

// Function to add a new killer move
void addKillerMove(Move move, int ply)
{
    // Don't add the same killer twice
    if (move != killerMoves[ply][0])
    {
        // Shift existing killer
        killerMoves[ply][1] = killerMoves[ply][0];
        // Add new first killer
        killerMoves[ply][0] = move;
    }
}

// Update history score for a good move
void updateHistory(Board &board, Move move, int depth)
{
    int side = board.sideToMove == White ? 0 : 1;
    int from_sq = from(move);
    int to_sq = to(move);

    // Bonus value proportional to depth
    int bonus = depth * depth*1.35;

    // Add bonus to history score
    historyTable[side][from_sq][to_sq] += bonus;

    // Prevent overflow by scaling down when scores get too high
    if (historyTable[side][from_sq][to_sq] > 16000)
    {
        // Scale down all history scores
        for (int i = 0; i < 2; i++)
        {
            for (int j = 0; j < 64; j++)
            {
                for (int k = 0; k < 64; k++)
                {
                    historyTable[i][j][k] /= 2;
                }
            }
        }
    }
}

// Clear history scores between games
void clearHistory()
{
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 64; j++)
        {
            for (int k = 0; k < 64; k++)
            {
                historyTable[i][j][k] = 0;
            }
        }
    }

    // Also clear killer moves
    for (int i = 0; i < MAX_PLY; i++)
    {
        killerMoves[i][0] = NO_MOVE;
        killerMoves[i][1] = NO_MOVE;
    }
}

void scoreMoves(Movelist &moves, Board &board, Move ttMove, int ply)
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
            score = 25000;
        }
        // capture
        else if (victim != None)
        {
            int captureScore = mvv_lva[victim][attacker];
            score = 6000 + captureScore;

            // Use SEE to refine capture evaluation
            int seeValue = see(board, move, -100);

            if (seeValue > 0)
            {
                // Winning captures get big bonus proportional to gain
                score += 1000 + seeValue;
            }
            else if (seeValue == 0)
            {
                // Equal captures still good but lower priority
                score += 500;
            }
            else
            {
                // Losing captures get lower priority but still higher than quiet moves
                // Don't completely discourage tactical sacrifices
                score += 200 + seeValue; // seeValue is negative, reducing the score
            }

            // Special case: Captures that give check might be tactical opportunities
            board.makeMove(move);
            bool givesCheck = (board.sideToMove == White) ? board.isSquareAttacked(Black, board.KingSQ(White)) : board.isSquareAttacked(White, board.KingSQ(Black));
            board.unmakeMove(move);

            if (givesCheck)
            {
                score += 600; // Bonus for check-giving captures
            }

            // Capturing pieces that have just moved can be tactically important
            // if (board.lastMove != NO_MOVE && to(board.lastMove) == from(move))
            // {
            //     score += 200; // Recapture bonus
            // }

            // Special case: Pawn captures to 7th/2nd rank
            if (attacker == WhitePawn || attacker == BlackPawn)
            {
                // Get rank of destination square
                int toRank = (to(move) >> 3); // Get rank (0-7)
                if ((board.sideToMove == White && toRank == 6) ||
                    (board.sideToMove == Black && toRank == 1))
                {
                    score += 500; // Pawn capture advancing to 7th/2nd rank
                }
            }
        }

        // promotions
        if (promoted(move))
        {
            PieceType promoType = piece(move);
            switch (promoType)
            {
            case QUEEN:
                score = 9900;
                break;
            case ROOK:
                score = 9800;
                break;
            case BISHOP:
                score = 9700;
                break;
            case KNIGHT:
                score = 9600;
                break;
            default:
                break;
            }
        }

        else if (ply < MAX_PLY)
        {
            if (move == killerMoves[ply][0])
            {
                score = 7600;
            }
            else if (move == killerMoves[ply][1])
            {
                score = 8300;
            }  else  // Add this else clause to use history for non-killer quiet moves
            {
                int side = board.sideToMove == White ? 0 : 1;
                score = historyTable[side][from(move)][to(move)];
            }
        }
        // 5. History heuristic (quiets that have been good in the past)
        else
        {
            int side = board.sideToMove == White ? 0 : 1;
            score = historyTable[side][from(move)][to(move)];
        }

        moves[i].value = score;
    }
}
