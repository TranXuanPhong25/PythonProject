#include "evaluate.hpp"
#include "evaluate_features.hpp"
#include "chess.hpp"

// Convert from Chess::Square to 0-63 index for piece-square tables
inline int squareToIndex(Square sq)
{
    return (7 - square_rank(sq)) * 8 + square_file(sq);
}

// Get flipped square for black pieces (to use same tables for both colors)
inline int getFlippedSquare(Square sq)
{
    return squareToIndex(sq) ^ 56; // Flips vertically
}

int evaluate(const Board &board)
{
    int score = 0;
    float endgameWeight = getGamePhase(board);

    // Material counting using bitboards for efficiency
    for (PieceType pt = PAWN; pt <= KING; ++pt)
    {
        Bitboard whitePieces = board.pieces(pt, White);
        Bitboard blackPieces = board.pieces(pt, Black);

        // WHITE
        while (whitePieces)
        {
            Square sq = static_cast<Square>(pop_lsb(whitePieces));
            score += PIECE_VALUES[pt];

            int idx = squareToIndex(sq);

            switch (pt)
            {
            case PAWN:
                score += PAWN_PST[idx];
                break;
            case KNIGHT:
                score += KNIGHT_PST[idx];
                break;
            case BISHOP:
                score += BISHOP_PST[idx];
                break;
            case ROOK:
                score += ROOK_PST[idx];
                break;
            case QUEEN:
                score += QUEEN_PST[idx];
                break;
            case KING:
                score += KING_MG_PST[idx] * (1.0f - endgameWeight) +
                         KING_EG_PST[idx] * endgameWeight;
                break;
            default:
                break;
            }
        }

        // BLACK
        while (blackPieces)
        {
            Square sq = static_cast<Square>(pop_lsb(blackPieces));
            score -= PIECE_VALUES[pt];

            int flippedIdx = getFlippedSquare(sq);

            switch (pt)
            {
            case PAWN:
                score -= PAWN_PST[flippedIdx];
                break;
            case KNIGHT:
                score -= KNIGHT_PST[flippedIdx];
                break;
            case BISHOP:
                score -= BISHOP_PST[flippedIdx];
                break;
            case ROOK:
                score -= ROOK_PST[flippedIdx];
                break;
            case QUEEN:
                score -= QUEEN_PST[flippedIdx];
                break;
            case KING:
                score -= KING_MG_PST[flippedIdx] * (1.0f - endgameWeight) +
                         KING_EG_PST[flippedIdx] * endgameWeight;
                break;
            default:
                break;
            }
        }
    }

    // Bishop pair bonus
    if (popcount(board.pieces(BISHOP, White)) >= 2)
        score += 30;
    if (popcount(board.pieces(BISHOP, Black)) >= 2)
        score -= 30;

    // Rook pair bonus
    if (popcount(board.pieces(ROOK, White)) >= 2)
        score += 20;
    if (popcount(board.pieces(ROOK, Black)) >= 2)
        score -= 20;

    //  //Evaluate PawnStructure
    score += (board.sideToMove == White ? evaluatePawnStructure(board) : -evaluatePawnStructure(board));

    // Evaluate center control
    score += (board.sideToMove == White ? evaluateCenterControl(board) : -evaluateCenterControl(board));

    // Evaluate mobility
    score += (board.sideToMove == White ? evaluateMobility(board, White) : -evaluateMobility(board, Black));

    // King safety (middle game)
    if (getGamePhase(board) < 0.5)
    { // Middle game
        score += (board.sideToMove == White ? evaluateKingSafety(board, White) : -evaluateKingSafety(board, Black));
        score += (board.sideToMove == White ? evaluateKingOpenFiles(board, White) : -evaluateKingOpenFiles(board, Black));
    }

    // King mobility (both phases)
    score += (board.sideToMove == White ? evaluateKingMobility(board, White) : -evaluateKingMobility(board, Black));

    // King checkmate potential (both phases)
    score += (board.sideToMove == White ? evaluateQueenControlAndCheckmatePotential(board, White) : -evaluateQueenControlAndCheckmatePotential(board, Black));

    // King castling ability (both phases)
    score += (board.sideToMove == White ? evaluateCastlingAbility(board, White) : -evaluateCastlingAbility(board, Black));

    // Return score from perspective of side to move
    return board.sideToMove == White ? score : -score;
}