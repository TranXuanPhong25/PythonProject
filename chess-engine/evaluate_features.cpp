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
    
    // Use constants to ensure symmetry
    const int PIECE_OCCUPATION_BONUS = 5;
    const int ATTACK_BONUS = 3;
    
    // Create a mutable copy of the board since attackersForSide is not const
    Board mutableBoard = board;

    for (Square sq : centerSquares) {
        auto piece = board.pieceAtB(sq);
        if (piece != None) {
            Color pieceColor = board.colorOf(sq);
            // Use relative scoring approach for better symmetry
            score += pieceColor == White ? PIECE_OCCUPATION_BONUS : -PIECE_OCCUPATION_BONUS;
        }

        // Use mutableBoard for attackersForSide
        U64 attackersWhite = mutableBoard.attackersForSide(White, sq, mutableBoard.occAll);
        U64 attackersBlack = mutableBoard.attackersForSide(Black, sq, mutableBoard.occAll);
        
        // Calculate attack bonuses symmetrically
        score += ATTACK_BONUS * popcount(attackersWhite);
        score -= ATTACK_BONUS * popcount(attackersBlack);
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
    const int BASE_PASSED_BONUS = 10;
    const int RANK_BONUS_MULTIPLIER = 5;
    const int ADVANCED_PASSED_BONUS = 10;

    while (pawns) {
        Square sq = static_cast<Square>(pop_lsb(pawns));
        int file = square_file(sq);
        int rank = square_rank(sq);
        
        // Calculate the blocker zone based on color
        Bitboard blockerZone = 0;
        int advancementMetric = 0;
        
        if (color == White) {
            // For white, we look at higher ranks
            advancementMetric = rank;
            for (int f = file-1; f <= file+1; f++) {
                if (f < 0 || f > 7) continue;
                for (int r = rank+1; r <= 7; r++) {
                    blockerZone |= (1ULL << (r * 8 + f));
                }
            }
        } else {
            // For black, we look at lower ranks
            advancementMetric = 7 - rank; // Normalize to 0-7 scale like white
            for (int f = file-1; f <= file+1; f++) {
                if (f < 0 || f > 7) continue;
                for (int r = rank-1; r >= 0; r--) {
                    blockerZone |= (1ULL << (r * 8 + f));
                }
            }
        }

        // If no enemy pawns in the blocker zone, this is a passed pawn
        if ((blockerZone & enemyPawns) == 0) {
            bonus += BASE_PASSED_BONUS + (advancementMetric * RANK_BONUS_MULTIPLIER);
            
            // Extra bonus for advanced passed pawns
            if (advancementMetric >= 5) {
                bonus += ADVANCED_PASSED_BONUS;
            }
        }
    }

    return bonus;
}

int evaluatePassedPawnSupport(const Board &board, Color color) {
    Bitboard pawns = board.pieces(PAWN, color);
    int bonus = 0;
    const int PAWN_SUPPORT_BONUS = 15;
    const int PIECE_SUPPORT_BONUS = 5;
    const int PROMOTION_PROXIMITY_BONUS = 2;
    
    // Create a mutable copy of the board since attackersForSide is not const
    Board mutableBoard = board;

    while (pawns) {
        Square sq = static_cast<Square>(pop_lsb(pawns));
        int file = square_file(sq);
        int rank = square_rank(sq);

        // Check if the pawn is passed (no enemy pawns in front of it)
        bool isPassed = true;
        Bitboard enemyPawns = board.pieces(PAWN, ~color);
        
        // Create a mask of squares ahead of the pawn where enemy pawns would block it
        Bitboard blockZone = 0ULL;
        
        for (int f = file-1; f <= file+1; f++) {
            if (f < 0 || f > 7) continue;
            
            if (color == White) {
                for (int r = rank+1; r <= 7; r++) {
                    blockZone |= (1ULL << (r * 8 + f));
                }
            } else { // Black
                for (int r = rank-1; r >= 0; r--) {
                    blockZone |= (1ULL << (r * 8 + f));
                }
            }
        }
        
        // If there are enemy pawns in the block zone, this pawn isn't passed
        if (blockZone & enemyPawns) {
            isPassed = false;
        }
        
        if (isPassed) {
            // Calculate support square locations
            int advanceDir = (color == White) ? 1 : -1;
            int forwardRank = rank + advanceDir;
            
            if (forwardRank >= 0 && forwardRank < 8) {
                Bitboard supportSquares = 0ULL;
                
                // Diagonal support squares
                if (file > 0) supportSquares |= (1ULL << (forwardRank * 8 + file - 1));
                if (file < 7) supportSquares |= (1ULL << (forwardRank * 8 + file + 1));
                
                // Calculate supporting pawns
                int pawnSupporters = popcount(supportSquares & board.pieces(PAWN, color));
                bonus += PAWN_SUPPORT_BONUS * pawnSupporters;
                
                // Calculate support from other pieces consistently for both colors
                // Only include the attackers that actually help support the pawn
                // Exclude attacks from enemy pieces
                Bitboard defenders = 0;
                
                // For White pawns moving up
                if (color == White) {
                    defenders = mutableBoard.attackersForSide(White, sq, mutableBoard.occAll);
                } 
                // For Black pawns moving down
                else {
                    defenders = mutableBoard.attackersForSide(Black, sq, mutableBoard.occAll);
                }
                
                // Remove own pawns to avoid double counting (already counted above)
                Bitboard pieceDefenders = defenders & ~board.pieces(PAWN, color);
                bonus += PIECE_SUPPORT_BONUS * popcount(pieceDefenders);
            }
            
            // Bonus for proximity to promotion - normalized for both colors
            int distanceToPromotion = (color == White) ? (7 - rank) : rank;
            bonus += (6 - distanceToPromotion) * PROMOTION_PROXIMITY_BONUS;
        }
    }

    return bonus;
}

int evaluateConnectedPawns(const Board &board, Color color) {
    Bitboard pawns = board.pieces(PAWN, color);
    int bonus = 0;
    const int CONNECTION_BONUS = 12;

    while (pawns) {
        Square sq = static_cast<Square>(pop_lsb(pawns));
        int file = square_file(sq);
        int rank = square_rank(sq);

        // Check adjacent squares for connected pawns (horizontally and diagonally)
        Bitboard connected = 0ULL;
        
        // Check horizontally adjacent squares (independent of color)
        if (file > 0) connected |= 1ULL << (rank * 8 + file - 1);
        if (file < 7) connected |= 1ULL << (rank * 8 + file + 1);
        
        // Check diagonally forward squares (color dependent)
        int forwardRank = (color == White) ? rank + 1 : rank - 1;
        if (forwardRank >= 0 && forwardRank < 8) {
            if (file > 0) connected |= 1ULL << (forwardRank * 8 + file - 1);
            if (file < 7) connected |= 1ULL << (forwardRank * 8 + file + 1);
        }

        // Award bonus if any of the connected squares has a pawn of our color
        if (connected & board.pieces(PAWN, color)) {
            bonus += CONNECTION_BONUS;
        }
    }

    return bonus;
}

int evaluatePhalanxPawns(const Board &board, Color color) {
    Bitboard pawns = board.pieces(PAWN, color);
    int bonus = 0;
    const int PHALANX_BONUS = 10;
    const int EXTENDED_PHALANX_BONUS = 5;

    for (int rank = 0; rank < 8; ++rank) {
        Bitboard pawnsInRank = pawns & MASK_RANK[rank];
        
        // Handle both sides symmetrically by using a directional-neutral approach
        for (int file = 0; file < 7; ++file) {
            // Check if there's a pawn at the current position
            Bitboard currentPos = 1ULL << (rank * 8 + file);
            
            // If there's a pawn here and in the next file, it's a phalanx
            if ((currentPos & pawnsInRank) && ((1ULL << (rank * 8 + file + 1)) & pawnsInRank)) {
                bonus += PHALANX_BONUS;
                
                // Check for extended phalanx (three adjacent pawns)
                if (file < 6 && ((1ULL << (rank * 8 + file + 2)) & pawnsInRank)) {
                    bonus += EXTENDED_PHALANX_BONUS;
                }
            }
        }
    }

    return bonus;
}

int evaluateBlockedPawns(const Board &board, Color color) {
    Bitboard pawns = board.pieces(PAWN, color);
    int penalty = 0;
    const int BLOCKED_PAWN_PENALTY = 10;

    while (pawns) {
        Square sq = static_cast<Square>(pop_lsb(pawns));
        int file = square_file(sq);
        int rank = square_rank(sq);
        
        // Calculate the square in front of the pawn
        int frontRank = (color == White) ? rank + 1 : rank - 1;
        
        // Skip if the pawn is on the edge of the board
        if (frontRank < 0 || frontRank > 7)
            continue;
            
        Square frontSq = static_cast<Square>(frontRank * 8 + file);
        
        // Check if the square in front is occupied (pawn is blocked)
        if (board.pieceAtB(frontSq) != None) {
            penalty += BLOCKED_PAWN_PENALTY;
        }
    }

    return penalty;
}

int evaluatePawnChains(const Board &board, Color color) {
    Bitboard pawns = board.pieces(PAWN, color);
    int bonus = 0;
    const int CHAIN_BONUS = 15;

    while (pawns) {
        Square sq = static_cast<Square>(pop_lsb(pawns));
        int file = square_file(sq);
        int rank = square_rank(sq);

        // Skip edge ranks where chains aren't possible
        if ((color == White && rank == 7) || (color == Black && rank == 0))
            continue;

        // Calculate protecting squares based on color
        int protectingRank = (color == White) ? rank + 1 : rank - 1;
        
        // Skip if protecting rank would be off the board
        if (protectingRank < 0 || protectingRank > 7)
            continue;
            
        // Check if there are protecting pawns diagonally behind this one
        Bitboard protectors = 0ULL;
        if (file > 0)
            protectors |= 1ULL << (protectingRank * 8 + (file - 1));
        if (file < 7)
            protectors |= 1ULL << (protectingRank * 8 + (file + 1));

        // Apply bonus for each pawn in the chain
        if (protectors & board.pieces(PAWN, color)) {
            bonus += CHAIN_BONUS;
        }
    }

    return bonus;
}

