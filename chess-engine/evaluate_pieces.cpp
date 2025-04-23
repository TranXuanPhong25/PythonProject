#include "evaluate_pieces.hpp"
#include <algorithm>

using namespace Chess;

// Helper functions
Bitboard getKingRing(const Board& board, Color color) {
    Square kingSq = board.KingSQ(color);
    Bitboard kingRing = 0;
    
    // Add all squares adjacent to the king
   for (int dr = -1; dr <= 1; dr++) {
        for (int df = -1; df <= 1; df++) {
            if (dr == 0 && df == 0) continue; // Skip the king square itself
            
            int r = square_rank(kingSq) + dr;
            int f = square_file(kingSq) + df;
            
            if (r >= 0 && r < 8 && f >= 0 && f < 8) {
                kingRing |= 1ULL << (r * 8 + f);
            }
        }
    }
    
    return kingRing;
}

bool canPawnAttackSquare(const Board& board, Square sq, Color attackingColor) {
    int rank = square_rank(sq);
    int file = square_file(sq);
    Bitboard enemyPawns = board.pieces(PAWN, attackingColor);
    
    if (attackingColor == White) {
        // Check if white pawns can attack this square (from black's perspective)
        if (file > 0 && rank < 7 && (enemyPawns & (1ULL << ((rank + 1) * 8 + file - 1)))) return true;
        if (file < 7 && rank < 7 && (enemyPawns & (1ULL << ((rank + 1) * 8 + file + 1)))) return true;
    } else {
        // Check if black pawns can attack this square (from white's perspective)
        if (file > 0 && rank > 0 && (enemyPawns & (1ULL << ((rank - 1) * 8 + file - 1)))) return true;
        if (file < 7 && rank > 0 && (enemyPawns & (1ULL << ((rank - 1) * 8 + file + 1)))) return true;
    }
    
    return false;
}

bool isPawnProtected(const Board& board, Square sq, Color color) {
    int rank = square_rank(sq);
    int file = square_file(sq);
    Bitboard pawns = board.pieces(PAWN, color);
    
    if (color == White) {
        if (file > 0 && rank > 0 && (pawns & (1ULL << ((rank - 1) * 8 + file - 1)))) return true;
        if (file < 7 && rank > 0 && (pawns & (1ULL << ((rank - 1) * 8 + file + 1)))) return true;
    } else {
        if (file > 0 && rank < 7 && (pawns & (1ULL << ((rank + 1) * 8 + file - 1)))) return true;
        if (file < 7 && rank < 7 && (pawns & (1ULL << ((rank + 1) * 8 + file + 1)))) return true;
    }
    
    return false;
}

// Unified evaluation for pieces attacking king ring
void evaluatePiecesAttackingKingRing(EvalInfo& ei, Color color, int& attackCount) {
    Bitboard enemyKingRing = ei.kingRings[~color];
    attackCount = 0;
    
    // Evaluate knights attacking king ring
    Bitboard knights = ei.board.pieces(KNIGHT, color);
    while (knights) {
        Square sq = static_cast<Square>(pop_lsb(knights));
        // Use the correct method for attacks
        Bitboard attacks = KnightAttacks(sq);
        int attackedSquares = popcount(attacks & enemyKingRing);
        
        if (attackedSquares > 0) {
            ei.mgScore += 10 * attackedSquares * (color == White ? 1 : -1);
            ei.egScore += 5 * attackedSquares * (color == White ? 1 : -1);
            attackCount++;
        }
    }
    
    // Evaluate bishops attacking king ring
    Bitboard bishops = ei.board.pieces(BISHOP, color);
    while (bishops) {
        Square sq = static_cast<Square>(pop_lsb(bishops));
        // Use the correct method for attacks
        Bitboard attacks = BishopAttacks(sq, ei.board.All());
        int attackedSquares = popcount(attacks & enemyKingRing);
        
        if (attackedSquares > 0) {
            ei.mgScore += 10 * attackedSquares * (color == White ? 1 : -1);
            ei.egScore += 5 * attackedSquares * (color == White ? 1 : -1);
            attackCount++;
        }
    }
    
    // Evaluate rooks attacking king ring
    Bitboard rooks = ei.board.pieces(ROOK, color);
    while (rooks) {
        Square sq = static_cast<Square>(pop_lsb(rooks));
        // Use the correct method for attacks
        Bitboard attacks = RookAttacks(sq, ei.board.All());
        int attackedSquares = popcount(attacks & enemyKingRing);
        
        if (attackedSquares > 0) {
            ei.mgScore += 15 * attackedSquares * (color == White ? 1 : -1);
            ei.egScore += 8 * attackedSquares * (color == White ? 1 : -1);
            attackCount++;
        }
    }
    
    // Evaluate queens attacking king ring
    Bitboard queens = ei.board.pieces(QUEEN, color);
    while (queens) {
        Square sq = static_cast<Square>(pop_lsb(queens));
        // Use the correct method for attacks
        Bitboard attacks = QueenAttacks(sq, ei.board.All());
        int attackedSquares = popcount(attacks & enemyKingRing);
        
        if (attackedSquares > 0) {
            ei.mgScore += 20 * attackedSquares * (color == White ? 1 : -1);
            ei.egScore += 10 * attackedSquares * (color == White ? 1 : -1);
            attackCount++;
        }
    }
    
    // Bonus for multiple attackers
    if (attackCount >= 2) {
        ei.mgScore += 10 * attackCount * (color == White ? 1 : -1);
    }
}

// Unified evaluation for outposts
void evaluateOutposts(EvalInfo& ei, Color color) {
    Bitboard knights = ei.board.pieces(KNIGHT, color);
    Bitboard bishops = ei.board.pieces(BISHOP, color);
    Bitboard potentialOutposts = ei.outpostSquares[color];
    int bonus = 0;
    
    // Filter out squares that can be attacked by enemy pawns
    Bitboard safeOutposts = 0;
    Bitboard outpostTemp = potentialOutposts;
    
    while (outpostTemp) {
        Square sq = static_cast<Square>(pop_lsb(outpostTemp));
        if (!canPawnAttackSquare(ei.board, sq, ~color)) {
            safeOutposts |= (1ULL << sq);
        }
    }
    
    // Evaluate knights in outposts
    while (knights) {
        Square sq = static_cast<Square>(pop_lsb(knights));
        if ((1ULL << sq) & safeOutposts) {
            // Knight is in a safe outpost
            bonus += 20;
            
            // Extra bonus if protected by pawn
            if (isPawnProtected(ei.board, sq, color)) {
                bonus += 10;
            }
            
            // Extra bonus for central outposts
            int file = square_file(sq);
            if (file >= 2 && file <= 5) {
                bonus += 5;
            }
        } else {
            // Knight can reach outpost - using correct attack method
            Bitboard attacks = KnightAttacks(sq);
            if (attacks & safeOutposts) {
                bonus += 8;
            }
        }
    }
    
    // Evaluate bishops in outposts (less valuable than knights in outposts)
    while (bishops) {
        Square sq = static_cast<Square>(pop_lsb(bishops));
        if ((1ULL << sq) & safeOutposts) {
            // Bishop is in a safe outpost
            bonus += 15;
            
            // Extra bonus if protected by pawn
            if (isPawnProtected(ei.board, sq, color)) {
                bonus += 8;
            }
        } else {
            // Bishop can reach outpost - using correct attack method
            Bitboard attacks = BishopAttacks(sq, ei.board.All());
            if (attacks & safeOutposts) {
                bonus += 6;
            }
        }
    }
    
    // Apply outpost bonus
    ei.mgScore += bonus * (color == White ? 1 : -1);
    ei.egScore += (bonus / 2) * (color == White ? 1 : -1); // Less important in endgame
}

// Unified rook evaluation
void evaluateRooks(EvalInfo& ei, Color color) {
    Bitboard rooks = ei.board.pieces(ROOK, color);
    Bitboard pawnsAll = ei.board.pieces(PAWN, White) | ei.board.pieces(PAWN, Black);
    int bonus = 0;
    
    while (rooks) {
        Square sq = static_cast<Square>(pop_lsb(rooks));
        int file = square_file(sq);
        int rank = square_rank(sq);
        
        // Evaluate rook on open/semi-open file
        Bitboard fileSquares = MASK_FILE[file];
        if ((fileSquares & pawnsAll) == 0) {
            bonus += 20; // Open file
        } else if ((fileSquares & ei.board.pieces(PAWN, color)) == 0) {
            bonus += 10; // Semi-open file
        }
        
        // Evaluate rook on queen file (d-file)
        if (file == 3) {
            bonus += 10;
        }
        
        // Evaluate trapped rook
        bool isTrapped = false;
        if (color == White) {
            if (rank == 0 && (file == 0 || file == 7)) {
                Square adjacentSquare = (file == 0) ? SQ_B1 : SQ_G1;
                if (ei.board.pieceAtB(adjacentSquare) == WhiteKing) {
                    isTrapped = true;
                }
            }
        } else {
            if (rank == 7 && (file == 0 || file == 7)) {
                Square adjacentSquare = (file == 0) ? SQ_B8 : SQ_G8;
                if (ei.board.pieceAtB(adjacentSquare) == BlackKing) {
                    isTrapped = true;
                }
            }
        }
        
        if (isTrapped) {
            bonus -= 25;
        }
        
        // Evaluate 7th rank rooks
        if ((color == White && rank == 6) || (color == Black && rank == 1)) {
            // Check if enemy king is on 8th/1st rank
            int enemyKingRank = square_rank(ei.board.KingSQ(~color));
            if ((color == White && enemyKingRank == 7) || (color == Black && enemyKingRank == 0)) {
                bonus += 20;
            } else {
                bonus += 10;
            }
        }
        
        // Evaluate rook on king ring
        Bitboard attacks = RookAttacks(sq, ei.board.All());
        if (attacks & ei.kingRings[~color]) {
            bonus += 15;
        }
    }
    
    // Apply rook bonus
    ei.mgScore += bonus * (color == White ? 1 : -1);
    ei.egScore += bonus * (color == White ? 1 : -1); // Rooks equally important in endgame
}

// Unified bishop evaluation
void evaluateBishops(EvalInfo& ei, Color color) {
    Bitboard bishops = ei.board.pieces(BISHOP, color);
    Bitboard pawns = ei.board.pieces(PAWN, color);
    int bonus = 0;
    
    // Bishop pair bonus
    if (popcount(bishops) >= 2) {
        bonus += 30;
    }
    
    // Main diagonals
    Bitboard mainDiagonal1 = 0x8040201008040201ULL;  // a1-h8
    Bitboard mainDiagonal2 = 0x0102040810204080ULL;  // a8-h1
    
    while (bishops) {
        Square sq = static_cast<Square>(pop_lsb(bishops));
        Bitboard bishopBB = 1ULL << sq;
        
        // Bishop on long diagonal
        if (bishopBB & (mainDiagonal1 | mainDiagonal2)) {
            bonus += 15;
            
            // Additional bonus if the bishop controls many squares on the diagonal
            Bitboard attacks = BishopAttacks(sq, ei.board.All());
            int controlledSquares = popcount(attacks & (mainDiagonal1 | mainDiagonal2));
            bonus += controlledSquares * 2;
        }
        
        // Bishop behind pawn
        int file = square_file(sq);
        int rank = square_rank(sq);
        
        // Look for pawns in front of the bishop
        if (color == White) {
            for (int r = rank + 1; r < 8; r++) {
                if (pawns & (1ULL << (r * 8 + file))) {
                    bonus += 5;
                    break;
                }
            }
        } else {
            for (int r = rank - 1; r >= 0; r--) {
                if (pawns & (1ULL << (r * 8 + file))) {
                    bonus += 5;
                    break;
                }
            }
        }
        
        // Evaluate bishop pawns (pawns on same colored squares)
        bool isDarkSquare = ((square_rank(sq) + square_file(sq)) % 2) == 1;
        int sameColorPawns = 0;
        Bitboard pawnsCopy = pawns;
        
        while (pawnsCopy) {
            Square pawnSq = static_cast<Square>(pop_lsb(pawnsCopy));
            bool isPawnOnDark = ((square_rank(pawnSq) + square_file(pawnSq)) % 2) == 1;
            
            if (isPawnOnDark == isDarkSquare) {
                sameColorPawns++;
            }
        }
        
        // Penalty for having many pawns on same colored squares as bishop
        if (sameColorPawns >= 3) {
            bonus -= sameColorPawns * 3;
        }
        
        // Evaluate bishop x-ray attacks
        Bitboard enemyPieces = ei.board.occAll & ~ei.board.Us(color);

        // Using empty board for x-ray attacks
        Bitboard xrayAttacks = BishopAttacks(sq, 0);
        
        // Get normal attacks with pieces
        Bitboard normalAttacks = BishopAttacks(sq, ei.board.All());
        
        // X-ray squares are those the bishop could attack if pieces weren't in the way
        Bitboard xraySquares = xrayAttacks & ~normalAttacks;
        
        // Check if any enemy pieces are on those x-ray squares
        bonus += 10 * popcount(xraySquares & enemyPieces);
        
        // Bishop on king ring
        if (normalAttacks & ei.kingRings[~color]) {
            bonus += 10;
        }
    }
    
    // Apply bishop bonus
    ei.mgScore += bonus * (color == White ? 1 : -1);
    ei.egScore += bonus * (color == White ? 1 : -1);
}

// Unified knight evaluation
void evaluateKnights(EvalInfo& ei, Color color) {
    Bitboard knights = ei.board.pieces(KNIGHT, color);
    int bonus = 0;
    
    while (knights) {
        Square sq = static_cast<Square>(pop_lsb(knights));
        
        // Knight mobility
        Bitboard attacks = KnightAttacks(sq);
        int mobility = popcount(attacks);
        bonus += mobility * 2;
        
        // Knights protecting the king
        if (attacks & ei.kingRings[color]) {
            bonus += 10;
        }
        
        // Knight on king ring
        if (attacks & ei.kingRings[~color]) {
            bonus += 10;
        }
        
        // Minor behind pawn
        int file = square_file(sq);
        int rank = square_rank(sq);
        Bitboard pawns = ei.board.pieces(PAWN, color);
        
        // Look for pawns in front of the knight
        if (color == White) {
            for (int r = rank + 1; r < 8; r++) {
                if (pawns & (1ULL << (r * 8 + file))) {
                    bonus += 5;
                    break;
                }
            }
        } else {
            for (int r = rank - 1; r >= 0; r--) {
                if (pawns & (1ULL << (r * 8 + file))) {
                    bonus += 5;
                    break;
                }
            }
        }
    }
    
    // Apply knight bonus
    ei.mgScore += bonus * (color == White ? 1 : -1);
    ei.egScore += bonus * (color == White ? 1 : -1);
}

// Unified queen evaluation
void evaluateQueens(EvalInfo& ei, Color color) {
    Bitboard queens = ei.board.pieces(QUEEN, color);
    int bonus = 0;
    
    if (queens == 0) return;
    
    // Since there's usually only one queen, we can simplify
    Square queenSq = static_cast<Square>(lsb(queens));
    int rank = square_rank(queenSq);
    
    // Queen mobility
    Bitboard attacks = QueenAttacks(queenSq, ei.board.All());
    int mobility = popcount(attacks);
    bonus += mobility;
    
    // Queen infiltration in enemy territory
    if ((color == White && rank >= 5) || (color == Black && rank <= 2)) {
        bonus += 10 * (color == White ? rank - 4 : 3 - rank);
        
        // Additional bonus if the queen is near the enemy king
        int distance = std::max(
            std::abs(square_file(queenSq) - square_file(ei.board.KingSQ(~color))),
            std::abs(square_rank(queenSq) - square_rank(ei.board.KingSQ(~color)))
        );
        
        if (distance <= 2) {
            bonus += (3 - distance) * 15;
        }
    }
    
    // Queen safety

    Board mutableBoard = ei.board;
    Bitboard enemyAttackers = mutableBoard.attackersForSide(~color, queenSq, mutableBoard.All());
    int numAttackers = popcount(enemyAttackers);
    Bitboard defenders = mutableBoard.attackersForSide(color, queenSq, ei.board.All()) & ~queens;
    int numDefenders = popcount(defenders);
    
    if (numAttackers > numDefenders) {
        bonus -= 15 * (numAttackers - numDefenders);
    }
    
    // Early queen development penalty
    if (color == White) {
        if (queenSq != SQ_D1) {
            // Check for developed minor pieces
            int developedPieces = popcount(ei.board.pieces(KNIGHT, White) & ~0x42ULL) + // Knights not on b1,g1
                                 popcount(ei.board.pieces(BISHOP, White) & ~0x24ULL);  // Bishops not on c1,f1
            
            if (developedPieces < 2) {
                bonus -= 20;
            }
        }
    } else {
        if (queenSq != SQ_D8) {
            // Check for developed minor pieces
            int developedPieces = popcount(ei.board.pieces(KNIGHT, Black) & ~0x4200000000000000ULL) + // Knights not on b8,g8
                                 popcount(ei.board.pieces(BISHOP, Black) & ~0x2400000000000000ULL);  // Bishops not on c8,f8
            
            if (developedPieces < 2) {
                bonus -= 20;
            }
        }
    }
    
    // Queen on king ring
    if (attacks & ei.kingRings[~color]) {
        bonus += 20;
    }
    
    // Apply queen bonus
    ei.mgScore += bonus * (color == White ? 1 : -1);
    ei.egScore += bonus * (color == White ? 1 : -1);
}

// King evaluation
void evaluateKingSafety(EvalInfo& ei, Color color) {
    Square kingSq = ei.board.KingSQ(color);
    int bonus = 0;
    
    // Protectors around the king
    Board mutableBoard = ei.board;
    Bitboard protectors = mutableBoard.attackersForSide(color, kingSq, ei.board.All());
    int numProtectors = popcount(protectors);
    bonus += numProtectors * 5;
    
    // Pawn shield
    int pawnShield = 0;
    int file = square_file(kingSq);
    int rank = square_rank(kingSq);
    
    // Check for pawns in front of the king
    if (color == White && rank < 7) {
        Bitboard frontSquares = 0;
        
        // Define area in front of king
        for (int f = std::max(0, file - 1); f <= std::min(7, file + 1); f++) {
            for (int r = rank + 1; r <= std::min(7, rank + 2); r++) {
                frontSquares |= 1ULL << (r * 8 + f);
            }
        }
        
        pawnShield = popcount(frontSquares & ei.board.pieces(PAWN, White));
    } 
    else if (color == Black && rank > 0) {
        Bitboard frontSquares = 0;
        
        // Define area in front of king
        for (int f = std::max(0, file - 1); f <= std::min(7, file + 1); f++) {
            for (int r = std::max(0, rank - 2); r < rank; r++) {
                frontSquares |= 1ULL << (r * 8 + f);
            }
        }
        
        pawnShield = popcount(frontSquares & ei.board.pieces(PAWN, Black));
    }
    
    bonus += pawnShield * 10;
    
    // King protector
    Bitboard knights = ei.board.pieces(KNIGHT, color);
    Bitboard bishops = ei.board.pieces(BISHOP, color);
    
    // Knights protecting the king
    Bitboard knightAttacks = 0;
    while (knights) {
        Square sq = static_cast<Square>(pop_lsb(knights));
        if ((1ULL << sq) & ei.kingRings[color]) {
            bonus += 10;
        }
        knightAttacks |= KnightAttacks(sq);
    }
    
    if (knightAttacks & ei.kingRings[color]) {
        bonus += 5;
    }
    
    // Bishops protecting the king
    Bitboard bishopAttacks = 0;
    while (bishops) {
        Square sq = static_cast<Square>(pop_lsb(bishops));
        if ((1ULL << sq) & ei.kingRings[color]) {
            bonus += 10;
        }
        bishopAttacks |= BishopAttacks(sq, ei.board.All());
    }
    
    if (bishopAttacks & ei.kingRings[color]) {
        bonus += 5;
    }
    
    // Apply king safety bonus (more important in middlegame)
    ei.mgScore += bonus * (color == White ? 1 : -1);
    ei.egScore += (bonus / 3) * (color == White ? 1 : -1); // Less important in endgame
}

// Main evaluation functions
int evaluatePiecesMg(const Board& board) {
    EvalInfo ei(board);
    
    // Clear previous scores
    ei.mgScore = 0;
    
    // Evaluate each piece type for both colors
    for (Color color : {White, Black}) {
        // Get all minor and major pieces (knights, bishops, rooks, queens)
        Bitboard knights = board.pieces(KNIGHT, color);
        Bitboard bishops = board.pieces(BISHOP, color);
        Bitboard rooks = board.pieces(ROOK, color);
        Bitboard queens = board.pieces(QUEEN, color);

        // Knights evaluation
        while (knights) {
            Square sq = static_cast<Square>(pop_lsb(knights));
            int score = 0;
            
            // Outpost value: [0,31,-7,30,56][outpost_total]
            if ((1ULL << sq) & ei.outpostSquares[color]) {
                if (!canPawnAttackSquare(board, sq, ~color)) {
                    score += 31; // Basic outpost
                    if (isPawnProtected(board, sq, color)) {
                        score += 25; // Protected outpost (31 + 25 = 56 total)
                    }
                } else {
                    score -= 7; // Can be attacked by enemy pawn
                }
            }
            
            // Minor behind pawn: 18 points
            int file = square_file(sq);
            int rank = square_rank(sq);
            Bitboard pawns = board.pieces(PAWN, color);
            if (color == White) {
                for (int r = rank + 1; r < 8; r++) {
                    if (pawns & (1ULL << (r * 8 + file))) {
                        score += 18;
                        break;
                    }
                }
            } else {
                for (int r = rank - 1; r >= 0; r--) {
                    if (pawns & (1ULL << (r * 8 + file))) {
                        score += 18;
                        break;
                    }
                }
            }
            
            // King protector: -8 for knights
            if ((KnightAttacks(sq) & ei.kingRings[color]) || 
                ((1ULL << sq) & ei.kingRings[color])) {
                score -= 8;
            }
            
            // Apply score
            ei.mgScore += score * (color == White ? 1 : -1);
        }
        
        // Bishops evaluation
        while (bishops) {
            Square sq = static_cast<Square>(pop_lsb(bishops));
            int score = 0;
            
            // Outpost value: same as knights
            if ((1ULL << sq) & ei.outpostSquares[color]) {
                if (!canPawnAttackSquare(board, sq, ~color)) {
                    score += 31; // Basic outpost
                    if (isPawnProtected(board, sq, color)) {
                        score += 25; // Protected outpost (31 + 25 = 56 total)
                    }
                } else {
                    score -= 7; // Can be attacked by enemy pawn
                }
            }
            
            // Minor behind pawn: 18 points
            int file = square_file(sq);
            int rank = square_rank(sq);
            Bitboard pawns = board.pieces(PAWN, color);
            if (color == White) {
                for (int r = rank + 1; r < 8; r++) {
                    if (pawns & (1ULL << (r * 8 + file))) {
                        score += 18;
                        break;
                    }
                }
            } else {
                for (int r = rank - 1; r >= 0; r--) {
                    if (pawns & (1ULL << (r * 8 + file))) {
                        score += 18;
                        break;
                    }
                }
            }
            
            // Bishop pawns: -3 per same-colored pawn
            bool isDarkSquare = ((square_rank(sq) + square_file(sq)) % 2) == 1;
            int sameColorPawns = 0;
            Bitboard pawnsCopy = pawns;
            while (pawnsCopy) {
                Square pawnSq = static_cast<Square>(pop_lsb(pawnsCopy));
                bool isPawnOnDark = ((square_rank(pawnSq) + square_file(pawnSq)) % 2) == 1;
                if (isPawnOnDark == isDarkSquare) {
                    sameColorPawns++;
                }
            }
            score -= 3 * sameColorPawns;
            
            // Bishop xray pawns: -4 per vulnerable pawn
            // Using empty board for x-ray attacks
            Bitboard xrayAttacks = BishopAttacks(sq, 0);
            // Get normal attacks with pieces
            Bitboard normalAttacks = BishopAttacks(sq, board.All());
            // X-ray squares are those the bishop could attack if pieces weren't in the way
            Bitboard xraySquares = xrayAttacks & ~normalAttacks;
            // Count enemy pawns on x-ray squares
            score -= 4 * popcount(xraySquares & board.pieces(PAWN, ~color));
            
            // Bishop on king ring: +24
            if (BishopAttacks(sq, board.All()) & ei.kingRings[~color]) {
                score += 24;
            }
            
            // Long diagonal bishop: +45
            Bitboard mainDiagonal1 = 0x8040201008040201ULL;  // a1-h8
            Bitboard mainDiagonal2 = 0x0102040810204080ULL;  // a8-h1
            if ((1ULL << sq) & (mainDiagonal1 | mainDiagonal2)) {
                score += 45;
            }
            
            // King protector: -6 for bishops
            if ((BishopAttacks(sq, board.All()) & ei.kingRings[color]) || 
                ((1ULL << sq) & ei.kingRings[color])) {
                score -= 6;
            }
            
            // Apply score
            ei.mgScore += score * (color == White ? 1 : -1);
        }
        
        // Rooks evaluation
        while (rooks) {
            Square sq = static_cast<Square>(pop_lsb(rooks));
            int score = 0;
            
            // Rook on queen file (d-file): +6
            if (square_file(sq) == 3) {
                score += 6;
            }
            
            // Rook on king ring: +16
            if (RookAttacks(sq, board.All()) & ei.kingRings[~color]) {
                score += 16;
            }
            
            // Rook on file: [0,19,48]
            int file = square_file(sq);
            Bitboard fileSquares = MASK_FILE[file];
            Bitboard pawnsAll = board.pieces(PAWN, White) | board.pieces(PAWN, Black);
            
            if ((fileSquares & pawnsAll) == 0) {
                score += 48; // Open file
            } else if ((fileSquares & board.pieces(PAWN, color)) == 0) {
                score += 19; // Semi-open file
            }
            
            // Trapped rook: -55 or -110 (doubled if no castling)
            bool isTrapped = false;
            if (color == White) {
                if (square_rank(sq) == 0 && (file == 0 || file == 7)) {
                    Square adjacentSquare = (file == 0) ? SQ_B1 : SQ_G1;
                    if (board.pieceAtB(adjacentSquare) == WhiteKing) {
                        isTrapped = true;
                    }
                }
            } else {
                if (square_rank(sq) == 7 && (file == 0 || file == 7)) {
                    Square adjacentSquare = (file == 0) ? SQ_B8 : SQ_G8;
                    if (board.pieceAtB(adjacentSquare) == BlackKing) {
                        isTrapped = true;
                    }
                }
            }
            
            if (isTrapped) {
                // Check castling rights using the bitflags system
                uint8_t relevantRights = 0;
                if (color == White) {
                    // wk = 1, wq = 2
                    relevantRights = (file == 7) ? 1 : 2; // kingside or queenside
                } else {
                    // bk = 4, bq = 8
                    relevantRights = (file == 7) ? 4 : 8; // kingside or queenside
                }
                bool hasCastling = (board.castlingRights & relevantRights) != 0;
                score -= 55 * (hasCastling ? 1 : 2);
            }
            
            // Apply score
            ei.mgScore += score * (color == White ? 1 : -1);
        }
        
        // Queens evaluation
        while (queens) {
            Square sq = static_cast<Square>(pop_lsb(queens));
            int score = 0;
            
            // Weak queen: -56 (queen under attack by lesser pieces)
            Board mutableBoard = board;
            Bitboard attackers = mutableBoard.attackersForSide(~color, sq, mutableBoard.All());
            Bitboard minorAttackers = attackers & (mutableBoard.pieces(KNIGHT, ~color) | mutableBoard.pieces(BISHOP, ~color));
            
            if (minorAttackers) {
                score -= 56;
            }
            
            // Queen infiltration: -2 per square into enemy territory
            int rank = square_rank(sq);
            if ((color == White && rank >= 5) || (color == Black && rank <= 2)) {
                score -= 2 * (color == White ? rank - 4 : 3 - rank);
            }
            
            // Apply score
            ei.mgScore += score * (color == White ? 1 : -1);
        }
    }
    
    // Evaluate king safety - already done in the current implementation
    evaluateKingSafety(ei, White);
    evaluateKingSafety(ei, Black);
    
    return ei.mgScore;
}

int evaluatePiecesEg(const Board& board) {
    EvalInfo ei(board);
    
    // Clear previous scores
    ei.egScore = 0;
    
    // Evaluate each piece type for both colors
    for (Color color : {White, Black}) {
        // Get all minor and major pieces (knights, bishops, rooks, queens)
        Bitboard knights = board.pieces(KNIGHT, color);
        Bitboard bishops = board.pieces(BISHOP, color);
        Bitboard rooks = board.pieces(ROOK, color);
        Bitboard queens = board.pieces(QUEEN, color);

        // Knights evaluation
        while (knights) {
            Square sq = static_cast<Square>(pop_lsb(knights));
            int score = 0;
            
            // Outpost value: [0,22,36,23,36][outpost_total] - FIXED to match Stockfish guide
            if ((1ULL << sq) & ei.outpostSquares[color]) {
                if (!canPawnAttackSquare(board, sq, ~color)) {
                    score += 22; // Basic outpost
                    if (isPawnProtected(board, sq, color)) {
                        score += 14; // Protected outpost (22 + 14 = 36 total)
                    }
                } else if (isPawnProtected(board, sq, color)) {
                    score += 23; // Can be attacked but protected
                }
            }
            
            // Minor behind pawn: 3 points in endgame - FIXED to match Stockfish guide
            int file = square_file(sq);
            int rank = square_rank(sq);
            Bitboard pawns = board.pieces(PAWN, color);
            if (color == White) {
                for (int r = rank + 1; r < 8; r++) {
                    if (pawns & (1ULL << (r * 8 + file))) {
                        score += 3;
                        break;
                    }
                }
            } else {
                for (int r = rank - 1; r >= 0; r--) {
                    if (pawns & (1ULL << (r * 8 + file))) {
                        score += 3;
                        break;
                    }
                }
            }
            
            // King protector: -9 in endgame - FIXED to match Stockfish guide
            if ((KnightAttacks(sq) & ei.kingRings[color]) || 
                ((1ULL << sq) & ei.kingRings[color])) {
                score -= 9;
            }
            
            // Apply score
            ei.egScore += score * (color == White ? 1 : -1);
        }
        
        // Bishops evaluation
        while (bishops) {
            Square sq = static_cast<Square>(pop_lsb(bishops));
            int score = 0;
            
            // Outpost value: same structure as knights
            if ((1ULL << sq) & ei.outpostSquares[color]) {
                if (!canPawnAttackSquare(board, sq, ~color)) {
                    score += 22; // Basic outpost
                    if (isPawnProtected(board, sq, color)) {
                        score += 14; // Protected outpost
                    }
                } else if (isPawnProtected(board, sq, color)) {
                    score += 23; // Can be attacked but protected
                }
            }
            
            // Minor behind pawn: 3 points in endgame - FIXED to match Stockfish guide
            int file = square_file(sq);
            int rank = square_rank(sq);
            Bitboard pawns = board.pieces(PAWN, color);
            if (color == White) {
                for (int r = rank + 1; r < 8; r++) {
                    if (pawns & (1ULL << (r * 8 + file))) {
                        score += 3;
                        break;
                    }
                }
            } else {
                for (int r = rank - 1; r >= 0; r--) {
                    if (pawns & (1ULL << (r * 8 + file))) {
                        score += 3;
                        break;
                    }
                }
            }
            
            // Bishop pawns: -7 per same-colored pawn in endgame - FIXED to match Stockfish guide
            bool isDarkSquare = ((square_rank(sq) + square_file(sq)) % 2) == 1;
            int sameColorPawns = 0;
            Bitboard pawnsCopy = pawns;
            while (pawnsCopy) {
                Square pawnSq = static_cast<Square>(pop_lsb(pawnsCopy));
                bool isPawnOnDark = ((square_rank(pawnSq) + square_file(pawnSq)) % 2) == 1;
                if (isPawnOnDark == isDarkSquare) {
                    sameColorPawns++;
                }
            }
            score -= 7 * sameColorPawns; // Correct penalty in endgame
            
            // Bishop xray pawns: -5 per vulnerable pawn in endgame - FIXED to match Stockfish guide
            Bitboard xrayAttacks = BishopAttacks(sq, 0);
            Bitboard normalAttacks = BishopAttacks(sq, board.All());
            Bitboard xraySquares = xrayAttacks & ~normalAttacks;
            score -= 5 * popcount(xraySquares & board.pieces(PAWN, ~color));
            
            // King protector: -9 in endgame (same for all pieces) - FIXED to match Stockfish guide
            if ((BishopAttacks(sq, board.All()) & ei.kingRings[color]) || 
                ((1ULL << sq) & ei.kingRings[color])) {
                score -= 9;
            }
            
            // Apply score
            ei.egScore += score * (color == White ? 1 : -1);
        }
        
        // Rooks evaluation
        while (rooks) {
            Square sq = static_cast<Square>(pop_lsb(rooks));
            int score = 0;
            
            // Rook on queen file: +11 in endgame - FIXED to match Stockfish guide
            if (square_file(sq) == 3) {
                score += 11;
            }
            
            // Rook on file: [0,7,29] in endgame - FIXED to match Stockfish guide
            int file = square_file(sq);
            Bitboard fileSquares = MASK_FILE[file];
            Bitboard pawnsAll = board.pieces(PAWN, White) | board.pieces(PAWN, Black);
            
            if ((fileSquares & pawnsAll) == 0) {
                score += 29; // Open file
            } else if ((fileSquares & board.pieces(PAWN, color)) == 0) {
                score += 7; // Semi-open file
            }
            
            // Trapped rook: -13 or -26 (doubled if no castling) in endgame - FIXED to match Stockfish guide
            bool isTrapped = false;
            if (color == White) {
                if (square_rank(sq) == 0 && (file == 0 || file == 7)) {
                    Square adjacentSquare = (file == 0) ? SQ_B1 : SQ_G1;
                    if (board.pieceAtB(adjacentSquare) == WhiteKing) {
                        isTrapped = true;
                    }
                }
            } else {
                if (square_rank(sq) == 7 && (file == 0 || file == 7)) {
                    Square adjacentSquare = (file == 0) ? SQ_B8 : SQ_G8;
                    if (board.pieceAtB(adjacentSquare) == BlackKing) {
                        isTrapped = true;
                    }
                }
            }
            
            if (isTrapped) {
                // Check castling rights using the bitflags system
                uint8_t relevantRights = 0;
                if (color == White) {
                    // wk = 1, wq = 2
                    relevantRights = (file == 7) ? 1 : 2; // kingside or queenside
                } else {
                    // bk = 4, bq = 8
                    relevantRights = (file == 7) ? 4 : 8; // kingside or queenside
                }
                bool hasCastling = (board.castlingRights & relevantRights) != 0;
                score -= 13 * (hasCastling ? 1 : 2); // FIXED to match Stockfish guide
            }
            
            // Apply score
            ei.egScore += score * (color == White ? 1 : -1);
        }
        
        // Queens evaluation
        while (queens) {
            Square sq = static_cast<Square>(pop_lsb(queens));
            int score = 0;
            
            // Weak queen: -15 in endgame - FIXED to match Stockfish guide
            Board mutableBoard = board;
            Bitboard attackers = mutableBoard.attackersForSide(~color, sq, mutableBoard.All());
            Bitboard minorAttackers = attackers & (mutableBoard.pieces(KNIGHT, ~color) | mutableBoard.pieces(BISHOP, ~color));
            
            if (minorAttackers) {
                score -= 15;
            }
            
            // Queen infiltration: +14 in endgame (positive bonus) - FIXED to match Stockfish guide
            int rank = square_rank(sq);
            if ((color == White && rank >= 5) || (color == Black && rank <= 2)) {
                score += 14 * (color == White ? rank - 4 : 3 - rank);
            }
            
            // King protector: -9 in endgame (same for all pieces) - FIXED to match Stockfish guide
            if ((QueenAttacks(sq, board.All()) & ei.kingRings[color]) || 
                ((1ULL << sq) & ei.kingRings[color])) {
                score -= 9;
            }
            
            // Apply score
            ei.egScore += score * (color == White ? 1 : -1);
        }
    }
    
    // Evaluate king safety - less important in endgame
    evaluateKingSafety(ei, White);
    evaluateKingSafety(ei, Black);
    
    return ei.egScore;
}

// Helper function to determine if position is in endgame
bool isEndgame(const Board& board) {
    constexpr int KnightEg = 854;
    constexpr int BishopEg = 915;
    constexpr int RookEg = 1380;
    constexpr int QueenEg = 2682;
    constexpr int EndgameLimit = 3915;
    
    int materialSum =
        popcount(board.pieces(KNIGHT, White) | board.pieces(KNIGHT, Black)) * KnightEg +
        popcount(board.pieces(BISHOP, White) | board.pieces(BISHOP, Black)) * BishopEg +
        popcount(board.pieces(ROOK, White) | board.pieces(ROOK, Black)) * RookEg +
        popcount(board.pieces(QUEEN, White) | board.pieces(QUEEN, Black)) * QueenEg;
    
    return materialSum <= EndgameLimit;
}

// Main evaluation function
int evaluatePieces(const Board& board) {
    int mgScore = evaluatePiecesMg(board);
    int egScore = evaluatePiecesEg(board);
    
    // Get game phase for smooth interpolation (0.0 = middlegame, 1.0 = endgame)
    float phase = getGamePhase(board);
    
    // Smooth interpolation between middlegame and endgame scores
    int finalScore = int((1.0f - phase) * mgScore + phase * egScore);
    
    // Return the score from the perspective of the side to move
    return board.sideToMove == White ? finalScore : -finalScore;
}