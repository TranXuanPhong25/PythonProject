#include "evaluate_features.hpp"
#include "evaluate.hpp" 
#include "chess.hpp"
#include <vector>
using namespace Chess;


// Pawn Structure: isolated, doubled, passed pawns (basic skeleton)
int evaluatePawnStructure(const Board &board) {
    int score = 0;
    score -= evaluateDoubledPawns(board, White);
    score += evaluateDoubledPawns(board, Black);

    score -= evaluateIsolatedPawns(board, White);
    score += evaluateIsolatedPawns(board, Black);

    score += evaluatePassedPawns(board, White);
    score -= evaluatePassedPawns(board, Black);

    return score;
}

// Center Control: reward for occupying/attacking center (d4/e4/d5/e5)
int evaluateCenterControl(const Board& board) {
    int score = 0;
    const Square centerSquares[] = { SQ_D4, SQ_E4, SQ_D5, SQ_E5 };

    Board tempBoard = board;

    for (Square sq : centerSquares) {
        auto piece = tempBoard.pieceAtB(sq);
        if (piece != None) {
            if (tempBoard.colorOf(sq) == White)
                score += 5;
            else if (tempBoard.colorOf(sq) == Black)
                score -= 5;
        }

            // Check for attackers on the center squares
            U64 attackersWhite = tempBoard.attackersForSide(White, sq, tempBoard.occAll);
            U64 attackersBlack = tempBoard.attackersForSide(Black, sq, tempBoard.occAll);
        
            score += 3 * popcount(attackersWhite); // Bonus if white attacks center
            score -= 3 * popcount(attackersBlack); // Bonus if black attacks center
    }
    return score;
}


int evaluateDoubledPawns(const Board &board, Color color) {
    int penalty = 0;
    // Get all pawns of the specified color
    Bitboard pawns = board.pieces(PAWN, color);

    // Iterate through each file (0 to 7)
    for (int file = 0; file < 8; ++file) {
        // Create a mask for the current file
        Bitboard pawnsInFile = pawns & MASK_FILE[file];
        // Count the number of pawns in that file
        int count = popcount(pawnsInFile);
        // If there are more than one pawn in the file, apply penalty
        if (count > 1) {
            penalty += (count - 1) * 10; // 10 points for each extra pawn in the file
        }
    }

    return penalty;
}


int evaluateIsolatedPawns(const Board &board, Color color) {
    Bitboard pawns = board.pieces(PAWN, color);
    int penalty = 0;

    // Iterate through each file (0 to 7)
    for (int file = 0; file < 8; ++file) {
        Bitboard pawnsInFile = pawns & MASK_FILE[file]; 
        // Check the left and right
        Bitboard leftSupport = (file > 0) ? (pawns & MASK_FILE[file - 1]) : 0;
        Bitboard rightSupport = (file < 7) ? (pawns & MASK_FILE[file + 1]) : 0;

        // If no support from both sides 
        if (pawnsInFile && !leftSupport && !rightSupport) {
            penalty += popcount(pawnsInFile) * 15; // minus 15 points for each isolated pawn
        }
    }

    return penalty;
}


int evaluatePassedPawns(const Board &board, Color color) {
    Bitboard pawns = board.pieces(PAWN, color);
    Bitboard enemyPawns = board.pieces(PAWN, ~color);
    int bonus = 0;

    while (pawns) {
        Square sq = static_cast<Square>(pop_lsb(pawns));
        int file = square_file(sq);
        int rank = square_rank(sq);
        
        Bitboard blockerZone = 0;

        for (int df = -1; df <= 1; ++df) {
            int f = file + df;
            if (f < 0 || f > 7) continue;

            if (color == White) {
                for (int r = rank + 1; r <= 7; ++r)
                    blockerZone |= (1ULL << (r * 8 + f));
            } else {
                for (int r = rank - 1; r >= 0; --r)
                    blockerZone |= (1ULL << (r * 8 + f));
            }
        }

        if ((blockerZone & enemyPawns) == 0) {
            int advancement = (color == White) ? rank : (7 - rank);
            bonus += advancement * 5 + 10;

            // Bonus point for queens-pawn
            if (advancement >= 5)
                bonus += 10;
        }
    }

    return bonus;
}

int evaluateMobility(const Board &board, Color color) {
    int mobility = 0;

    // Knights
    Bitboard knights = board.pieces(KNIGHT, color);
    while (knights) {
        Square sq = poplsb(knights);
        mobility += popcount(KnightAttacks(sq) & ~board.Us(color));
    }

    // Bishops
    Bitboard bishops = board.pieces(BISHOP, color);
    while (bishops) {
        Square sq = poplsb(bishops);
        mobility += popcount(BishopAttacks(sq, board.occAll) & ~board.Us(color));
    }

    // Rooks
    Bitboard rooks = board.pieces(ROOK, color);
    while (rooks) {
        Square sq = poplsb(rooks);
        mobility += popcount(RookAttacks(sq, board.occAll) & ~board.Us(color));
    }

    // Queens
    Bitboard queens = board.pieces(QUEEN, color);
    while (queens) {
        Square sq = poplsb(queens);
        mobility += popcount(QueenAttacks(sq, board.occAll) & ~board.Us(color));
    }

    // King
    Bitboard king = board.pieces(KING, color);
    if (king) {
        Square sq = poplsb(king);
        mobility += popcount(KingAttacks(sq) & ~board.Us(color));
    }

    return mobility;
}