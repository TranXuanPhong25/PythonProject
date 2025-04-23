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

    score += evaluatePassedPawnSupport(board, White);
    score -= evaluatePassedPawnSupport(board, Black);

    score += evaluateConnectedPawns(board, White);
    score -= evaluateConnectedPawns(board, Black);

    score += evaluatePhalanxPawns(board, White);
    score -= evaluatePhalanxPawns(board, Black);

    score += evaluateBlockedPawns(board, White);
    score -= evaluateBlockedPawns(board, Black);

    score += evaluatePawnChains(board, White);
    score -= evaluatePawnChains(board, Black);

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

int evaluatePassedPawnSupport(const Board &board, Color color) {
    Bitboard pawns = board.pieces(PAWN, color);
    int bonus = 0;

    // Create a mutable copy of the board to avoid const issues
    Board &mutableBoard = const_cast<Board &>(board);

    while (pawns) {
        Square sq = static_cast<Square>(pop_lsb(pawns));
        int file = square_file(sq);
        int rank = square_rank(sq);

        // Check if the pawn is passed
        if ((mutableBoard.pieces(PAWN, ~color) & mutableBoard.SQUARES_BETWEEN_BB[sq][mutableBoard.KingSQ(~color)]) == 0) {
            int dir = (color == White) ? 1 : -1;
            int forwardRank = rank + dir;

            Bitboard support = 0ULL;

            // Create a mask for the squares in front of the pawn
            if (forwardRank >= 0 && forwardRank < 8) {
                if (file > 0)
                    support |= 1ULL << (forwardRank * 8 + file - 1);
                if (file < 7)
                    support |= 1ULL << (forwardRank * 8 + file + 1);
            }

            // Calculate the support from pawns
            int pawnSupporters = popcount(support & mutableBoard.pieces(PAWN, color));
            bonus += 15 * pawnSupporters;

            // Calculate the support from other pieces
            Bitboard otherSupport = mutableBoard.attackersForSide(color, sq, mutableBoard.occAll) & ~mutableBoard.pieces(PAWN, color);
            bonus += 5 * popcount(otherSupport);

            // Bonus for being close to promotion
            int promotionDistance = (color == White) ? (7 - rank) : rank;
            bonus += (6 - promotionDistance) * 2;
        }
    }

    return bonus;
}

int evaluateConnectedPawns(const Board &board, Color color) {
    Bitboard pawns = board.pieces(PAWN, color);
    int bonus = 0;

    while (pawns) {
        Square sq = static_cast<Square>(pop_lsb(pawns));
        int file = square_file(sq);
        int rank = square_rank(sq);

        Bitboard connected = 0ULL;
        int dir = (color == White) ? -1 : 1;
        int forwardRank = rank + dir;

        // Check for connected pawns in the same file and diagonally
        if (file > 0 && forwardRank >= 0 && forwardRank < 8)
            connected |= 1ULL << (forwardRank * 8 + file - 1);
        if (file < 7 && forwardRank >= 0 && forwardRank < 8)
            connected |= 1ULL << (forwardRank * 8 + file + 1);
        if (file > 0)
            connected |= 1ULL << (rank * 8 + file - 1);
        if (file < 7)
            connected |= 1ULL << (rank * 8 + file + 1);

        if (connected & board.pieces(PAWN, color)) {
            bonus += 12; // Bonus for connected pawns
        }
    }

    return bonus;
}

int evaluatePhalanxPawns(const Board &board, Color color) {
    Bitboard pawns = board.pieces(PAWN, color);
    int bonus = 0;

    for (int rank = 0; rank < 8; ++rank) {
        Bitboard pawnsInRank = pawns & MASK_RANK[rank];

        // Tạo mặt nạ để tránh lỗi khi shift ngang
        Bitboard safeLeftShift = pawnsInRank & ~MASK_FILE[7]; // loại bỏ tốt ở file H
        Bitboard safeRightShift = pawnsInRank & ~MASK_FILE[0]; // loại bỏ tốt ở file A

        // Cặp tốt liền nhau (phalanx)
        Bitboard phalanx = (safeLeftShift << 1) & pawnsInRank;
        bonus += 10 * popcount(phalanx);

        // Chuỗi tốt dài hơn 2 (ví dụ A-B-C)
        Bitboard extendedPhalanx = (phalanx << 1) & pawnsInRank;
        bonus += 5 * popcount(extendedPhalanx);
    }

    return bonus;
}

int evaluateBlockedPawns(const Board &board, Color color) {
    Bitboard pawns = board.pieces(PAWN, color);
    int penalty = 0;

    while (pawns) {
        Square sq = static_cast<Square>(pop_lsb(pawns));
        int file = square_file(sq);
        int rank = square_rank(sq);

        // Check if the pawn is blocked
        if ((color == White && rank == 6) || (color == Black && rank == 1)) {
            penalty += 10; // Penalty for blocked pawn
        }
    }

    return penalty;
}

int evaluatePawnChains(const Board &board, Color color) {
    Bitboard pawns = board.pieces(PAWN, color);
    int bonus = 0;

    while (pawns) {
        Square sq = static_cast<Square>(pop_lsb(pawns));
        int file = square_file(sq);
        int rank = square_rank(sq);

        // Check if the pawn is part of a chain
        if ((color == White && rank == 0) || (color == Black && rank == 7))
            continue;

        // Check for pawns in the same file and diagonally
        Bitboard protectors = 0ULL;
        if (color == White && rank > 0) {
            if (file > 0) protectors |= 1ULL << ((rank - 1) * 8 + (file - 1));
            if (file < 7) protectors |= 1ULL << ((rank - 1) * 8 + (file + 1));
        } else if (color == Black && rank < 7) {
            if (file > 0) protectors |= 1ULL << ((rank + 1) * 8 + (file - 1));
            if (file < 7) protectors |= 1ULL << ((rank + 1) * 8 + (file + 1));
        }

        // Bonus for each pawn in the chain
        if (protectors & board.pieces(PAWN, color)) {
            bonus += 15;
        }
    }

    return bonus;
}
