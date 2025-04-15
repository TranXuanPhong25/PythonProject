#include "evaluate_attacks.hpp"
#include "sliders.hpp"

// Compute pinned pieces (pieces that cannot move without exposing the king to attack)
Bitboard evaluatePinnedPieces(const Board& board, Color side) {
    Bitboard pinned = 0;
    Square kingSquare = board.KingSQ(side);
    Color opponent = ~side;
    
    // Check for pins by bishops/queens along diagonals
    Bitboard potentialPinners = board.pieces(BISHOP, opponent) | board.pieces(QUEEN, opponent);
    while (potentialPinners) {
        Square pinnerSquare = static_cast<Square>(pop_lsb(potentialPinners));
        Bitboard pinRay = BishopAttacks(pinnerSquare, 0) & BishopAttacks(kingSquare, 0);
        
        if (pinRay) {
            Bitboard possiblePinned = pinRay & board.Us(side);
            if (popcount(possiblePinned) == 1 && 
                (BishopAttacks(kingSquare, board.All() & ~possiblePinned) & (1ULL << pinnerSquare))) {
                pinned |= possiblePinned;
            }
        }
    }
    
    // Check for pins by rooks/queens along files and ranks
    potentialPinners = board.pieces(ROOK, opponent) | board.pieces(QUEEN, opponent);
    while (potentialPinners) {
        Square pinnerSquare = static_cast<Square>(pop_lsb(potentialPinners));
        Bitboard pinRay = RookAttacks(pinnerSquare, 0) & RookAttacks(kingSquare, 0);
        
        if (pinRay) {
            Bitboard possiblePinned = pinRay & board.Us(side);
            if (popcount(possiblePinned) == 1 && 
                (RookAttacks(kingSquare, board.All() & ~possiblePinned) & (1ULL << pinnerSquare))) {
                pinned |= possiblePinned;
            }
        }
    }
    
    return pinned;
}

Bitboard getKnightAttacks(const Board& board, Color side) {
    Bitboard knights = board.pieces(KNIGHT, side);
    Bitboard attacks = 0;
    
    while (knights) {
        Square sq = static_cast<Square>(pop_lsb(knights));
        attacks |= KnightAttacks(sq);
    }
    
    return attacks;
}

// X-ray attacks look through some pieces to find potential tactical opportunities
Bitboard getBishopXrayAttacks(const Board& board, Color side) {
    Bitboard bishops = board.pieces(BISHOP, side);
    Bitboard attacks = 0;
    Bitboard blockers = board.All() & ~board.pieces(QUEEN, side); // See through queens
    
    while (bishops) {
        Square sq = static_cast<Square>(pop_lsb(bishops));
        attacks |= BishopAttacks(sq, blockers);
    }
    
    return attacks;
}

Bitboard getRookXrayAttacks(const Board& board, Color side) {
    Bitboard rooks = board.pieces(ROOK, side);
    Bitboard attacks = 0;
    Bitboard blockers = board.All() & ~board.pieces(QUEEN, side); // See through queens
    
    while (rooks) {
        Square sq = static_cast<Square>(pop_lsb(rooks));
        attacks |= RookAttacks(sq, blockers);
    }
    
    return attacks;
}

Bitboard getQueenAttacks(const Board& board, Color side) {
    Bitboard queens = board.pieces(QUEEN, side);
    Bitboard attacks = 0;
    Bitboard blockers = board.All();
    
    while (queens) {
        Square sq = static_cast<Square>(pop_lsb(queens));
        attacks |= (BishopAttacks(sq, blockers) | RookAttacks(sq, blockers));
    }
    
    return attacks;
}

Bitboard getQueenDiagonalAttacks(const Board& board, Color side) {
    Bitboard queens = board.pieces(QUEEN, side);
    Bitboard attacks = 0;
    Bitboard blockers = board.All();
    
    while (queens) {
        Square sq = static_cast<Square>(pop_lsb(queens));
        attacks |= BishopAttacks(sq, blockers);
    }
    
    return attacks;
}

Bitboard getPawnAttacks(const Board& board, Color side) {
    Bitboard pawns = board.pieces(PAWN, side);
    return side == White ? (((pawns & ~FILE_A) << 7) | ((pawns & ~FILE_H) << 9)) :
                          (((pawns & ~FILE_H) >> 7) | ((pawns & ~FILE_A) >> 9));
}

Bitboard getKingAttacks(const Board& board, Color side) {
    Square kingSquare = board.KingSQ(side);
    return KingAttacks(kingSquare);
}

// Main attack evaluation function
int evaluateAttacks(const Board& board) {
    int score = 0;
    
    // Get attacked squares for both sides
    Bitboard whiteAttacks = getKnightAttacks(board, White) |
                           getBishopXrayAttacks(board, White) |
                           getRookXrayAttacks(board, White) |
                           getQueenAttacks(board, White) |
                           getPawnAttacks(board, White) |
                           getKingAttacks(board, White);
                           
    Bitboard blackAttacks = getKnightAttacks(board, Black) |
                           getBishopXrayAttacks(board, Black) |
                           getRookXrayAttacks(board, Black) |
                           getQueenAttacks(board, Black) |
                           getPawnAttacks(board, Black) |
                           getKingAttacks(board, Black);
    
    // Evaluate piece-specific attacks
    
    // Knight attacks
    Bitboard whiteKnightAttacks = getKnightAttacks(board, White);
    Bitboard blackKnightAttacks = getKnightAttacks(board, Black);
    
    // Score for attacking opponent pieces
    score += KnightAttackValue * popcount(whiteKnightAttacks & board.Us(Black));
    score -= KnightAttackValue * popcount(blackKnightAttacks & board.Us(White));
    
    // Bishop X-ray attacks
    Bitboard whiteBishopXRays = getBishopXrayAttacks(board, White);
    Bitboard blackBishopXRays = getBishopXrayAttacks(board, Black);
    
    // Score for attacking opponent pieces with X-rays
    score += BishopAttackValue * popcount(whiteBishopXRays & board.Us(Black));
    score -= BishopAttackValue * popcount(blackBishopXRays & board.Us(White));
    
    // Additional bonus for X-ray attacks
    score += XrayBishopBonus * popcount(whiteBishopXRays & ~board.All());
    score -= XrayBishopBonus * popcount(blackBishopXRays & ~board.All());
    
    // Rook X-ray attacks
    Bitboard whiteRookXRays = getRookXrayAttacks(board, White);
    Bitboard blackRookXRays = getRookXrayAttacks(board, Black);
    
    score += RookAttackValue * popcount(whiteRookXRays & board.Us(Black));
    score -= RookAttackValue * popcount(blackRookXRays & board.Us(White));
    
    // Additional bonus for X-ray attacks
    score += XrayRookBonus * popcount(whiteRookXRays & ~board.All());
    score -= XrayRookBonus * popcount(blackRookXRays & ~board.All());
    
    // Queen attacks
    Bitboard whiteQueenAttacks = getQueenAttacks(board, White);
    Bitboard blackQueenAttacks = getQueenAttacks(board, Black);
    
    score += QueenAttackValue * popcount(whiteQueenAttacks & board.Us(Black));
    score -= QueenAttackValue * popcount(blackQueenAttacks & board.Us(White));
    
    // Queen diagonal attacks (usually more dangerous)
    Bitboard whiteQueenDiagAttacks = getQueenDiagonalAttacks(board, White);
    Bitboard blackQueenDiagAttacks = getQueenDiagonalAttacks(board, Black);
    
    score += DiagonalQueenAttackBonus * popcount(whiteQueenDiagAttacks & board.Us(Black));
    score -= DiagonalQueenAttackBonus * popcount(blackQueenDiagAttacks & board.Us(White));
    
    // Pawn attacks
    Bitboard whitePawnAttacks = getPawnAttacks(board, White);
    Bitboard blackPawnAttacks = getPawnAttacks(board, Black);
    
    score += PawnAttackValue * popcount(whitePawnAttacks & board.Us(Black));
    score -= PawnAttackValue * popcount(blackPawnAttacks & board.Us(White));
    
    // King attacks - attacking near the king is valuable
    Square whiteKingSq = board.KingSQ(White);
    Square blackKingSq = board.KingSQ(Black);
    
    // King vicinity squares
    Bitboard whiteKingZone = KingAttacks(whiteKingSq);
    Bitboard blackKingZone = KingAttacks(blackKingSq);
    
    // Attacks near the king
    score -= AttackWeightKing * popcount(blackAttacks & whiteKingZone);
    score += AttackWeightKing * popcount(whiteAttacks & blackKingZone);
    
    // Evaluate pinned pieces
    Bitboard whitePinned = evaluatePinnedPieces(board, White);
    Bitboard blackPinned = evaluatePinnedPieces(board, Black);
    
    // Count pins by bishop/queen (diagonal)
    Bitboard whitePinnedByBishop = whitePinned & (getBishopXrayAttacks(board, Black) | getQueenDiagonalAttacks(board, Black));
    Bitboard blackPinnedByBishop = blackPinned & (getBishopXrayAttacks(board, White) | getQueenDiagonalAttacks(board, White));
    
    // Count pins by rook/queen (horizontal/vertical)
    Bitboard whitePinnedByRook = whitePinned & (getRookXrayAttacks(board, Black) | (getQueenAttacks(board, Black) & ~getQueenDiagonalAttacks(board, Black)));
    Bitboard blackPinnedByRook = blackPinned & (getRookXrayAttacks(board, White) | (getQueenAttacks(board, White) & ~getQueenDiagonalAttacks(board, White)));
    
    // Apply penalties for pinned pieces
    score += PinnedPieceBishop * popcount(whitePinnedByBishop);
    score -= PinnedPieceBishop * popcount(blackPinnedByBishop);
    
    score += PinnedPieceRook * popcount(whitePinnedByRook);
    score -= PinnedPieceRook * popcount(blackPinnedByRook);
    
    return score;
}