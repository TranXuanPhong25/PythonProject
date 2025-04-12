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
        U64 attacks = KnightAttacks(sq) & ~board.Us(color); // Exclude friendly pieces
        mobility += popcount(attacks) * 3;                 // Weight: 3
        mobility += popcount(attacks & board.Enemy(color)) * 2; // Bonus for attacking enemy pieces
    }

    // Bishops
    Bitboard bishops = board.pieces(BISHOP, color);
    while (bishops) {
        Square sq = poplsb(bishops);
        U64 attacks = BishopAttacks(sq, board.occAll) & ~board.Us(color); // Exclude friendly pieces
        mobility += popcount(attacks) * 3;                               // Weight: 3
        mobility += popcount(attacks & board.Enemy(color)) * 2;          // Bonus for attacking enemy pieces
    }

    // Rooks
    Bitboard rooks = board.pieces(ROOK, color);
    while (rooks) {
        Square sq = poplsb(rooks);
        U64 attacks = RookAttacks(sq, board.occAll) & ~board.Us(color); // Exclude friendly pieces
        mobility += popcount(attacks) * 5;                             // Weight: 5
        mobility += popcount(attacks & board.Enemy(color)) * 2;        // Bonus for attacking enemy pieces
    }

    // Queens
    Bitboard queens = board.pieces(QUEEN, color);
    while (queens) {
        Square sq = poplsb(queens);
        U64 attacks = QueenAttacks(sq, board.occAll) & ~board.Us(color); // Exclude friendly pieces
        mobility += popcount(attacks) * 9;                              // Weight: 9
        mobility += popcount(attacks & board.Enemy(color)) * 2;         // Bonus for attacking enemy pieces
    }

    // King
    Bitboard king = board.pieces(KING, color);
    if (king) {
        Square sq = poplsb(king);
        U64 attacks = KingAttacks(sq) & ~board.Us(color); // Exclude friendly pieces
        mobility += popcount(attacks) * 2;               // Weight: 2

        // Penalize unsafe king moves (attacks on squares controlled by the enemy)
        mobility -= popcount(attacks & board.Enemy(color)) * 3;
    }

    // Penalize blocked pieces (pieces with very low mobility)
    if (mobility < 5) {
        mobility -= 5; // Apply a penalty for very low mobility
    }

    // Scale mobility by game phase
    float endgameWeight = getGamePhase(board); // 0.0 in opening, 1.0 in endgame
    float middlegameWeight = 1.0f - endgameWeight;

    return static_cast<int>(mobility * middlegameWeight);
}

void updateMobility(Board &board, Move move, int &mobilityScore, Color color) {
    Square fromS = from(move);
    Square toS = to(move);
    Piece movedPiece = board.pieceAtB(fromS);
    Piece capturedPiece = board.pieceAtB(toS);

    // Subtract mobility of the piece from its old square
    mobilityScore -= popcount(board.attacksByPiece(type_of_piece(movedPiece), fromS, color) & ~board.Us(color));

    // If a piece is captured, subtract its mobility
    if (capturedPiece != None) {
        mobilityScore -= popcount(board.attacksByPiece(type_of_piece(capturedPiece), toS, ~color) & ~board.Us(~color));
    }

    // Update the board state for the move
    board.makeMove(move);

    // Add mobility of the piece in its new square
    mobilityScore += popcount(board.attacksByPiece(type_of_piece(movedPiece), toS, color) & ~board.Us(color));

    // Restore the board state (if needed for further calculations)
    board.unmakeMove(move);
}