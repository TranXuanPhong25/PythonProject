#include "evaluate_attacks.hpp"
#include "sliders.hpp"
#include "evaluate.hpp"  // Added to access PIECE_VALUES

using namespace Chess;

// Improved pinned piece detection
Bitboard evaluatePinnedPieces(const Board& board, Color side) {
    Bitboard pinned = 0;
    Square kingSquare = board.KingSQ(side);
    Color opponent = ~side;
    
    // Check for pins by bishops/queens along diagonals
    Bitboard potentialPinners = board.pieces(BISHOP, opponent) | board.pieces(QUEEN, opponent);
    while (potentialPinners) {
        Square pinnerSquare = static_cast<Square>(pop_lsb(potentialPinners));
        
        // Get the ray between king and potential pinner
        Bitboard rayBetween = BishopAttacks(kingSquare, 0) & BishopAttacks(pinnerSquare, 0);
        
        // If they're on the same diagonal
        if (rayBetween) {
            Bitboard between = rayBetween & board.All();
            // There must be exactly one piece between them for a pin
            if (popcount(between) == 1 && (between & board.Us(side))) {
                pinned |= between;
            }
        }
    }
    
    // Check for pins by rooks/queens along files and ranks
    potentialPinners = board.pieces(ROOK, opponent) | board.pieces(QUEEN, opponent);
    while (potentialPinners) {
        Square pinnerSquare = static_cast<Square>(pop_lsb(potentialPinners));
        
        // Get the ray between king and potential pinner
        Bitboard rayBetween = RookAttacks(kingSquare, 0) & RookAttacks(pinnerSquare, 0);
        
        // If they're on the same rank/file
        if (rayBetween) {
            Bitboard between = rayBetween & board.All();
            // There must be exactly one piece between them for a pin
            if (popcount(between) == 1 && (between & board.Us(side))) {
                pinned |= between;
            }
        }
    }
    
    return pinned;
}

// Get knight attacks for a side
Bitboard getKnightAttacks(const Board& board, Color side) {
    Bitboard knights = board.pieces(KNIGHT, side);
    Bitboard attacks = 0;
    
    while (knights) {
        Square sq = static_cast<Square>(pop_lsb(knights));
        attacks |= KnightAttacks(sq);
    }
    
    return attacks;
}

// Get bishop attacks with X-ray vision through friendly pieces
Bitboard getBishopXrayAttacks(const Board& board, Color side) {
    Bitboard bishops = board.pieces(BISHOP, side);
    Bitboard attacks = 0;
    
    while (bishops) {
        Square sq = static_cast<Square>(pop_lsb(bishops));
        
        // Normal bishop attacks
        Bitboard normalAttacks = BishopAttacks(sq, board.All());
        attacks |= normalAttacks;
        
        // For X-rays, we look for potential targets behind friendly pieces
        Bitboard friendlyBlockers = normalAttacks & board.Us(side);
        if (friendlyBlockers) {
            // Get attacks assuming those friendly pieces aren't there
            Bitboard xrayAttacks = BishopAttacks(sq, board.All() & ~friendlyBlockers);
            attacks |= xrayAttacks;
        }
    }
    
    return attacks;
}

// Get rook attacks with X-ray vision through friendly pieces
Bitboard getRookXrayAttacks(const Board& board, Color side) {
    Bitboard rooks = board.pieces(ROOK, side);
    Bitboard attacks = 0;
    
    while (rooks) {
        Square sq = static_cast<Square>(pop_lsb(rooks));
        
        // Normal rook attacks
        Bitboard normalAttacks = RookAttacks(sq, board.All());
        attacks |= normalAttacks;
        
        // For X-rays, we look for potential targets behind friendly pieces
        Bitboard friendlyBlockers = normalAttacks & board.Us(side);
        if (friendlyBlockers) {
            // Get attacks assuming those friendly pieces aren't there
            Bitboard xrayAttacks = RookAttacks(sq, board.All() & ~friendlyBlockers);
            attacks |= xrayAttacks;
        }
    }
    
    return attacks;
}

// Get all queen attacks 
Bitboard getQueenAttacks(const Board& board, Color side) {
    Bitboard queens = board.pieces(QUEEN, side);
    Bitboard attacks = 0;
    
    while (queens) {
        Square sq = static_cast<Square>(pop_lsb(queens));
        attacks |= QueenAttacks(sq, board.All());
    }
    
    return attacks;
}

// Get diagonal queen attacks only
Bitboard getQueenDiagonalAttacks(const Board& board, Color side) {
    Bitboard queens = board.pieces(QUEEN, side);
    Bitboard attacks = 0;
    
    while (queens) {
        Square sq = static_cast<Square>(pop_lsb(queens));
        attacks |= BishopAttacks(sq, board.All());
    }
    
    return attacks;
}

// Get all pawn attacks
Bitboard getPawnAttacks(const Board& board, Color side) {
    Bitboard pawns = board.pieces(PAWN, side);
    return side == White ? (((pawns & ~FILE_A) << 7) | ((pawns & ~FILE_H) << 9)) :
                          (((pawns & ~FILE_H) >> 7) | ((pawns & ~FILE_A) >> 9));
}

// Get king attacks
Bitboard getKingAttacks(const Board& board, Color side) {
    Square kingSquare = board.KingSQ(side);
    return KingAttacks(kingSquare);
}

// Get an extended king danger zone (2 squares away)
Bitboard getKingDangerZone(const Board& board, Color side) {
    Square kingSq = board.KingSQ(side);
    int kFile = square_file(kingSq);
    int kRank = square_rank(kingSq);
    Bitboard zone = 0;
    
    // Create a zone extending 2 squares from the king
    for (int f = std::max(0, kFile - 2); f <= std::min(7, kFile + 2); f++) {
        for (int r = std::max(0, kRank - 2); r <= std::min(7, kRank + 2); r++) {
            zone |= 1ULL << (r * 8 + f);
        }
    }
    
    return zone;
}

// Main attack evaluation function
int evaluateAttacks(const Board& board) {
    int score = 0;
    
    // Get king zones (immediate squares + slightly extended zone)
    Bitboard whiteKingZone = KingAttacks(board.KingSQ(White));
    Bitboard blackKingZone = KingAttacks(board.KingSQ(Black));
    Bitboard whiteKingDangerZone = getKingDangerZone(board, White);
    Bitboard blackKingDangerZone = getKingDangerZone(board, Black);
    
    // Get attack bitboards
    Bitboard whiteKnightAttacks = getKnightAttacks(board, White);
    Bitboard blackKnightAttacks = getKnightAttacks(board, Black);
    
    Bitboard whiteBishopAttacks = getBishopXrayAttacks(board, White);
    Bitboard blackBishopAttacks = getBishopXrayAttacks(board, Black);
    
    Bitboard whiteRookAttacks = getRookXrayAttacks(board, White);
    Bitboard blackRookAttacks = getRookXrayAttacks(board, Black);
    
    Bitboard whiteQueenAttacks = getQueenAttacks(board, White);
    Bitboard blackQueenAttacks = getQueenAttacks(board, Black);
    
    Bitboard whitePawnAttacks = getPawnAttacks(board, White);
    Bitboard blackPawnAttacks = getPawnAttacks(board, Black);
    
    // Count attacking pieces near enemy king - weighted by piece importance
    int whiteAttackCount = 0;
    int blackAttackCount = 0;
    
    // Knight attacks near king
    whiteAttackCount += 2 * popcount(whiteKnightAttacks & blackKingDangerZone);
    blackAttackCount += 2 * popcount(blackKnightAttacks & whiteKingDangerZone);
    
    // Direct attacks on king zone are worth more
    whiteAttackCount += 3 * popcount(whiteKnightAttacks & blackKingZone);
    blackAttackCount += 3 * popcount(blackKnightAttacks & whiteKingZone);
    
    // Bishop attacks
    whiteAttackCount += 2 * popcount(whiteBishopAttacks & blackKingDangerZone);
    blackAttackCount += 2 * popcount(blackBishopAttacks & whiteKingDangerZone);
    
    whiteAttackCount += 3 * popcount(whiteBishopAttacks & blackKingZone);
    blackAttackCount += 3 * popcount(blackBishopAttacks & whiteKingZone);
    
    // Rook attacks
    whiteAttackCount += 3 * popcount(whiteRookAttacks & blackKingDangerZone);
    blackAttackCount += 3 * popcount(blackRookAttacks & whiteKingDangerZone);
    
    whiteAttackCount += 4 * popcount(whiteRookAttacks & blackKingZone);
    blackAttackCount += 4 * popcount(blackRookAttacks & whiteKingZone);
    
    // Queen attacks - highest weight
    whiteAttackCount += 5 * popcount(whiteQueenAttacks & blackKingDangerZone);
    blackAttackCount += 5 * popcount(blackQueenAttacks & whiteKingDangerZone);
    
    whiteAttackCount += 6 * popcount(whiteQueenAttacks & blackKingZone);
    blackAttackCount += 6 * popcount(blackQueenAttacks & whiteKingZone);
    
    // Pawn attacks
    whiteAttackCount += 1 * popcount(whitePawnAttacks & blackKingDangerZone);
    blackAttackCount += 1 * popcount(blackPawnAttacks & whiteKingDangerZone);
    
    whiteAttackCount += 2 * popcount(whitePawnAttacks & blackKingZone);
    blackAttackCount += 2 * popcount(blackPawnAttacks & whiteKingZone);
    
    // Non-linear king attack evaluation - small # of attackers means little,
    // but many attackers should be heavily rewarded
    int whiteKingAttackScore = 0;
    int blackKingAttackScore = 0;
    
    if (whiteAttackCount > 0) {
        whiteKingAttackScore = AttackWeightKing * whiteAttackCount * whiteAttackCount / 8;
    }
    
    if (blackAttackCount > 0) {
        blackKingAttackScore = AttackWeightKing * blackAttackCount * blackAttackCount / 8;
    }
    
    score += whiteKingAttackScore - blackKingAttackScore;
    
    // Score general piece-on-piece attacks with lower weights
    // Knight attacks
    score += KnightAttackValue * popcount(whiteKnightAttacks & board.Us(Black));
    score -= KnightAttackValue * popcount(blackKnightAttacks & board.Us(White));
    
    // Bishop attacks
    score += BishopAttackValue * popcount(whiteBishopAttacks & board.Us(Black));
    score -= BishopAttackValue * popcount(blackBishopAttacks & board.Us(White));
    
    // Rook attacks
    score += RookAttackValue * popcount(whiteRookAttacks & board.Us(Black));
    score -= RookAttackValue * popcount(blackRookAttacks & board.Us(White));
    
    // Queen attacks
    score += QueenAttackValue * popcount(whiteQueenAttacks & board.Us(Black));
    score -= QueenAttackValue * popcount(blackQueenAttacks & board.Us(White));
    
    // Pawn attacks
    score += PawnAttackValue * popcount(whitePawnAttacks & board.Us(Black));
    score -= PawnAttackValue * popcount(blackPawnAttacks & board.Us(White));
    
    // X-ray attack specific bonuses - only count x-ray squares that are not directly attacked
    Bitboard whiteRookDirectAttacks = 0, blackRookDirectAttacks = 0;
    Bitboard whiteBishopDirectAttacks = 0, blackBishopDirectAttacks = 0;
    
    {
        Bitboard rooks = board.pieces(ROOK, White);
        while (rooks) {
            Square sq = static_cast<Square>(pop_lsb(rooks));
            whiteRookDirectAttacks |= RookAttacks(sq, board.All());
        }
        
        Bitboard bishops = board.pieces(BISHOP, White);
        while (bishops) {
            Square sq = static_cast<Square>(pop_lsb(bishops));
            whiteBishopDirectAttacks |= BishopAttacks(sq, board.All());
        }
    }
    
    {
        Bitboard rooks = board.pieces(ROOK, Black);
        while (rooks) {
            Square sq = static_cast<Square>(pop_lsb(rooks));
            blackRookDirectAttacks |= RookAttacks(sq, board.All());
        }
        
        Bitboard bishops = board.pieces(BISHOP, Black);
        while (bishops) {
            Square sq = static_cast<Square>(pop_lsb(bishops));
            blackBishopDirectAttacks |= BishopAttacks(sq, board.All());
        }
    }
    
    // X-ray specific squares are those in the x-ray attack set but not in direct attacks
    Bitboard whiteRookXraySpecific = whiteRookAttacks & ~whiteRookDirectAttacks;
    Bitboard blackRookXraySpecific = blackRookAttacks & ~blackRookDirectAttacks;
    Bitboard whiteBishopXraySpecific = whiteBishopAttacks & ~whiteBishopDirectAttacks;
    Bitboard blackBishopXraySpecific = blackBishopAttacks & ~blackBishopDirectAttacks;
    
    // Bonus for x-ray attacks that hit enemy pieces
    score += XrayRookBonus * popcount(whiteRookXraySpecific & board.Us(Black));
    score -= XrayRookBonus * popcount(blackRookXraySpecific & board.Us(White));
    score += XrayBishopBonus * popcount(whiteBishopXraySpecific & board.Us(Black));
    score -= XrayBishopBonus * popcount(blackBishopXraySpecific & board.Us(White));
    
    // Evaluate pinned pieces with more moderate penalties
    Bitboard whitePinned = evaluatePinnedPieces(board, White);
    Bitboard blackPinned = evaluatePinnedPieces(board, Black);
    
    // Pinned by bishop (diagonal pin)
    Bitboard whitePinnedByBishop = whitePinned & (getBishopXrayAttacks(board, Black) & ~getRookXrayAttacks(board, Black));
    Bitboard blackPinnedByBishop = blackPinned & (getBishopXrayAttacks(board, White) & ~getRookXrayAttacks(board, White));
    
    // Pinned by rook (horizontal/vertical pin)
    Bitboard whitePinnedByRook = whitePinned & getRookXrayAttacks(board, Black);
    Bitboard blackPinnedByRook = blackPinned & getRookXrayAttacks(board, White);
    
    // Scale pin penalty by piece value
    int whitePinPenalty = 0, blackPinPenalty = 0;
    
    // Check what pieces are pinned and apply appropriate penalties
    {
        Bitboard pinnedPieces = whitePinnedByBishop;
        while (pinnedPieces) {
            Square sq = static_cast<Square>(pop_lsb(pinnedPieces));
            PieceType pt = type_of_piece(board.pieceAtB(sq));
            // Apply penalty based on pinned piece value
            whitePinPenalty += (pt == QUEEN ? 3 : 1) * PinnedPieceBishop;
        }
    }
    
    {
        Bitboard pinnedPieces = blackPinnedByBishop;
        while (pinnedPieces) {
            Square sq = static_cast<Square>(pop_lsb(pinnedPieces));
            PieceType pt = type_of_piece(board.pieceAtB(sq));
            // Apply penalty based on pinned piece value
            blackPinPenalty += (pt == QUEEN ? 3 : 1) * PinnedPieceBishop;
        }
    }
    
    {
        Bitboard pinnedPieces = whitePinnedByRook;
        while (pinnedPieces) {
            Square sq = static_cast<Square>(pop_lsb(pinnedPieces));
            PieceType pt = type_of_piece(board.pieceAtB(sq));
            // Apply penalty based on pinned piece value
            whitePinPenalty += (pt == QUEEN ? 3 : 1) * PinnedPieceRook;
        }
    }
    
    {
        Bitboard pinnedPieces = blackPinnedByRook;
        while (pinnedPieces) {
            Square sq = static_cast<Square>(pop_lsb(pinnedPieces));
            PieceType pt = type_of_piece(board.pieceAtB(sq));
            // Apply penalty based on pinned piece value
            blackPinPenalty += (pt == QUEEN ? 3 : 1) * PinnedPieceRook;
        }
    }
    
    score += whitePinPenalty - blackPinPenalty;
    
    // Additional evaluation for discovered check potential
    Bitboard whiteDiscoveryPieces = whitePinned;
    Bitboard blackDiscoveryPieces = blackPinned;
    
    // Bonus for having discovery check possibilities
    score += 5 * popcount(whiteDiscoveryPieces);
    score -= 5 * popcount(blackDiscoveryPieces);
    
    // Hanging pieces (pieces that are attacked but not defended)
    Bitboard whiteAttacked = 0, blackAttacked = 0;
    
    // Get all attacked squares by each side
    Bitboard whiteAttacksAll = whitePawnAttacks | whiteKnightAttacks | whiteBishopAttacks | 
                               whiteRookAttacks | whiteQueenAttacks | getKingAttacks(board, White);
                               
    Bitboard blackAttacksAll = blackPawnAttacks | blackKnightAttacks | blackBishopAttacks | 
                               blackRookAttacks | blackQueenAttacks | getKingAttacks(board, Black);
    
    // Find hanging pieces (attacked enemy pieces that aren't defended)
    whiteAttacked = blackAttacksAll & board.Us(White);
    blackAttacked = whiteAttacksAll & board.Us(Black);
    
    // Find hanging pieces
    Bitboard whiteHanging = whiteAttacked & ~whitePawnAttacks & ~whiteKnightAttacks & 
                            ~whiteBishopAttacks & ~whiteRookAttacks & ~whiteQueenAttacks & 
                            ~getKingAttacks(board, White);
                            
    Bitboard blackHanging = blackAttacked & ~blackPawnAttacks & ~blackKnightAttacks & 
                            ~blackBishopAttacks & ~blackRookAttacks & ~blackQueenAttacks & 
                            ~getKingAttacks(board, Black);
    
    // Penalty for hanging pieces
    while (whiteHanging) {
        Square sq = static_cast<Square>(pop_lsb(whiteHanging));
        PieceType pt = type_of_piece(board.pieceAtB(sq));
        score -= 10 + PIECE_VALUES[pt] / 10;  // Penalty based on piece value
    }
    
    while (blackHanging) {
        Square sq = static_cast<Square>(pop_lsb(blackHanging));
        PieceType pt = type_of_piece(board.pieceAtB(sq));
        score += 10 + PIECE_VALUES[pt] / 10;  // Penalty based on piece value
    }
    
    return score;
}