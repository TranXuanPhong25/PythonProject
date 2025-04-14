#include "zugzwang.hpp"
#include <algorithm>

namespace Chess {

bool ZugzwangDetector::isPotentialZugzwang(const Board& board) {
    // Quick checks for potential zugzwang positions
    
    // Material-based checks
    if (hasMaterialZugzwangRisk(board))
        return true;
    
    // Few pieces for side to move
    if (hasLowPieceCount(board, board.sideToMove))
        return true;
    
    // Specific endgame patterns
    EndgameType endgame = classifyEndgame(board);
    if (endgame == KPK || endgame == KPPK || endgame == KBK || endgame == KNK)
        return true;
        
    // Pawn structure analysis
    if (hasPawnStructureZugzwangRisk(board))
        return true;
    
    return false;
}

ZugzwangRisk ZugzwangDetector::getZugzwangRisk(const Board& board) {
    // Pure king+pawns endgames have extreme zugzwang risk
    if (isKingAndPawnsEndgame(board))
        return EXTREME;
    
    // Count total pieces
    int totalPieces = popcount(board.occAll);
    
    // Few pieces often means zugzwang is possible
    if (totalPieces <= 5) {
        // King + single minor piece
        if (hasSingleMinorPiece(board, board.sideToMove))
            return HIGH;
            
        // King + pawns only for side to move
        if (board.pieces(PAWN, board.sideToMove) != 0 && 
            !board.pieces(KNIGHT, board.sideToMove) && 
            !board.pieces(BISHOP, board.sideToMove) &&
            !board.pieces(ROOK, board.sideToMove) &&
            !board.pieces(QUEEN, board.sideToMove))
            return HIGH;
            
        return MEDIUM;
    } 
    else if (totalPieces <= 8) {
        // If only pawns for our side (no pieces)
        if (board.pieces(PAWN, board.sideToMove) != 0 &&
            !board.pieces(KNIGHT, board.sideToMove) && 
            !board.pieces(BISHOP, board.sideToMove) &&
            !board.pieces(ROOK, board.sideToMove) &&
            !board.pieces(QUEEN, board.sideToMove))
            return MEDIUM;
            
        return LOW;
    }
    
    // Check specific endgame patterns
    EndgameType endgame = classifyEndgame(board);
    if (endgame != NO_ENDGAME && endgame != OTHER)
        return LOW;
    
    // Check pawn structure for potential zugzwang
    if (hasPawnStructureZugzwangRisk(board))
        return LOW;
    
    return NONE;
}

bool ZugzwangDetector::shouldAvoidNullMove(const Board& board) {
    ZugzwangRisk risk = getZugzwangRisk(board);
    
    // High risk positions - avoid null move
    if (risk == HIGH || risk == EXTREME)
        return true;
        
    // King and pawns endgames
    if (isKingAndPawnsEndgame(board))
        return true;
        
    // Very few pieces total
    if (popcount(board.occAll) <= 6)
        return true;
    
    // Side to move has very few pieces
    if (hasLowPieceCount(board, board.sideToMove))
        return true;
    
    // Side to move lacks material for meaningful threats
    if (!hasSufficientMaterial(board, board.sideToMove))
        return true;
    
    return false;
}

EndgameType ZugzwangDetector::classifyEndgame(const Board& board) {
    // Count material for each side
    U64 whitePawns = board.pieces(PAWN, White);
    U64 whiteKnights = board.pieces(KNIGHT, White);
    U64 whiteBishops = board.pieces(BISHOP, White);
    U64 whiteRooks = board.pieces(ROOK, White);
    U64 whiteQueens = board.pieces(QUEEN, White);
    
    U64 blackPawns = board.pieces(PAWN, Black);
    U64 blackKnights = board.pieces(KNIGHT, Black);
    U64 blackBishops = board.pieces(BISHOP, Black);
    U64 blackRooks = board.pieces(ROOK, Black);
    U64 blackQueens = board.pieces(QUEEN, Black);
    
    int wpCount = popcount(whitePawns);
    int wnCount = popcount(whiteKnights);
    int wbCount = popcount(whiteBishops);
    int wrCount = popcount(whiteRooks);
    int wqCount = popcount(whiteQueens);
    
    int bpCount = popcount(blackPawns);
    int bnCount = popcount(blackKnights);
    int bbCount = popcount(blackBishops);
    int brCount = popcount(blackRooks);
    int bqCount = popcount(blackQueens);
    
    // Total material count
    int totalPieces = wpCount + wnCount + wbCount + wrCount + wqCount + 
                      bpCount + bnCount + bbCount + brCount + bqCount + 2; // +2 for kings
    
    // Too many pieces for specific endgame
    if (totalPieces > 6)
        return OTHER;
    
    // KPK - King and Pawn vs King
    if ((wpCount == 1 && wnCount == 0 && wbCount == 0 && wrCount == 0 && wqCount == 0 &&
         bpCount == 0 && bnCount == 0 && bbCount == 0 && brCount == 0 && bqCount == 0) ||
        (bpCount == 1 && bnCount == 0 && bbCount == 0 && brCount == 0 && bqCount == 0 &&
         wpCount == 0 && wnCount == 0 && wbCount == 0 && wrCount == 0 && wqCount == 0))
        return KPK;
    
    // KBNK - King, Bishop, Knight vs King
    if ((wnCount == 1 && wbCount == 1 && wpCount == 0 && wrCount == 0 && wqCount == 0 &&
         bpCount == 0 && bnCount == 0 && bbCount == 0 && brCount == 0 && bqCount == 0) ||
        (bnCount == 1 && bbCount == 1 && bpCount == 0 && brCount == 0 && bqCount == 0 &&
         wpCount == 0 && wnCount == 0 && wbCount == 0 && wrCount == 0 && wqCount == 0))
        return KBNK;
    
    // KBK - King and Bishop vs King
    if ((wbCount == 1 && wpCount == 0 && wnCount == 0 && wrCount == 0 && wqCount == 0 &&
         bpCount == 0 && bnCount == 0 && bbCount == 0 && brCount == 0 && bqCount == 0) ||
        (bbCount == 1 && bpCount == 0 && bnCount == 0 && brCount == 0 && bqCount == 0 &&
         wpCount == 0 && wnCount == 0 && wbCount == 0 && wrCount == 0 && wqCount == 0))
        return KBK;
    
    // KNK - King and Knight vs King
    if ((wnCount == 1 && wpCount == 0 && wbCount == 0 && wrCount == 0 && wqCount == 0 &&
         bpCount == 0 && bnCount == 0 && bbCount == 0 && brCount == 0 && bqCount == 0) ||
        (bnCount == 1 && bpCount == 0 && bbCount == 0 && brCount == 0 && bqCount == 0 &&
         wpCount == 0 && wnCount == 0 && wbCount == 0 && wrCount == 0 && wqCount == 0))
        return KNK;
    
    // KRK - King and Rook vs King
    if ((wrCount == 1 && wpCount == 0 && wnCount == 0 && wbCount == 0 && wqCount == 0 &&
         bpCount == 0 && bnCount == 0 && bbCount == 0 && brCount == 0 && bqCount == 0) ||
        (brCount == 1 && bpCount == 0 && bnCount == 0 && bbCount == 0 && bqCount == 0 &&
         wpCount == 0 && wnCount == 0 && wbCount == 0 && wrCount == 0 && wqCount == 0))
        return KRK;
    
    // KRKP - King and Rook vs King and Pawn
    if ((wrCount == 1 && wpCount == 0 && wnCount == 0 && wbCount == 0 && wqCount == 0 &&
         bpCount == 1 && bnCount == 0 && bbCount == 0 && brCount == 0 && bqCount == 0) ||
        (brCount == 1 && bpCount == 0 && bnCount == 0 && bbCount == 0 && bqCount == 0 &&
         wpCount == 1 && wnCount == 0 && wbCount == 0 && wrCount == 0 && wqCount == 0))
        return KRKP;
    
    // KQKP - King and Queen vs King and Pawn
    if ((wqCount == 1 && wpCount == 0 && wnCount == 0 && wbCount == 0 && wrCount == 0 &&
         bpCount == 1 && bnCount == 0 && bbCount == 0 && brCount == 0 && bqCount == 0) ||
        (bqCount == 1 && bpCount == 0 && bnCount == 0 && bbCount == 0 && brCount == 0 &&
         wpCount == 1 && wnCount == 0 && wbCount == 0 && wrCount == 0 && wqCount == 0))
        return KQKP;
    
    // KRPKR - King, Rook, and Pawn vs King and Rook
    if ((wrCount == 1 && wpCount == 1 && wnCount == 0 && wbCount == 0 && wqCount == 0 &&
         bpCount == 0 && bnCount == 0 && bbCount == 0 && brCount == 1 && bqCount == 0) ||
        (brCount == 1 && bpCount == 1 && bnCount == 0 && bbCount == 0 && bqCount == 0 &&
         wpCount == 0 && wnCount == 0 && wbCount == 0 && wrCount == 1 && wqCount == 0))
        return KRPKR;
    
    // KPPK - King and two pawns vs King
    if ((wpCount == 2 && wnCount == 0 && wbCount == 0 && wrCount == 0 && wqCount == 0 &&
         bpCount == 0 && bnCount == 0 && bbCount == 0 && brCount == 0 && bqCount == 0) ||
        (bpCount == 2 && bnCount == 0 && bbCount == 0 && brCount == 0 && bqCount == 0 &&
         wpCount == 0 && wnCount == 0 && wbCount == 0 && wrCount == 0 && wqCount == 0))
        return KPPK;
    
    // Other endgame
    return OTHER;
}

bool ZugzwangDetector::isNear50MoveRule(const Board& board) {
    // Check if we're nearing 50-move rule
    // Higher numbers mean we're closer to a draw
    return board.halfMoveClock >= 80;  // Within 20 half-moves of a draw
}

int ZugzwangDetector::getAdjustedDepth(const Board& board, int originalDepth) {
    // Adjust search depth based on position characteristics
    
    // Check zugzwang risk
    ZugzwangRisk risk = getZugzwangRisk(board);
    
    // Search deeper in zugzwang-prone positions
    if (risk == EXTREME)
        return originalDepth + 2;
    
    if (risk == HIGH)
        return originalDepth + 1;
    
    // Search deeper when approaching 50-move rule
    if (board.halfMoveClock >= 90)
        return originalDepth + 2;
    else if (board.halfMoveClock >= 80)
        return originalDepth + 1;
    
    // Specific endgames that benefit from deeper search
    EndgameType endgame = classifyEndgame(board);
    if (endgame == KPK || endgame == KBNK || endgame == KRKP)
        return originalDepth + 1;
    
    return originalDepth;
}

int ZugzwangDetector::getNullMoveR(const Board& board, int depth) {
    // Get optimal null move reduction value
    ZugzwangRisk risk = getZugzwangRisk(board);
    
    // Base R value that scales with depth
    int R = 3 + depth / 6;
    
    // Adjust R based on zugzwang risk
    switch (risk) {
        case EXTREME:
            // Should avoid null move completely
            return 0;
        case HIGH:
            // Very conservative reduction
            return std::max(2, R - 1);
        case MEDIUM:
            // Slightly reduced depth
            return R;
        case LOW:
            // Normal reduction
            return R + 1;
        case NONE:
            // More aggressive reduction
            return R + 2;
    }
    
    return R; // Default fallback
}

// Private helper methods

bool ZugzwangDetector::hasMaterialZugzwangRisk(const Board& board) {
    // King + pawns only (typical zugzwang pattern)
    if (!board.pieces(KNIGHT, board.sideToMove) && 
        !board.pieces(BISHOP, board.sideToMove) &&
        !board.pieces(ROOK, board.sideToMove) &&
        !board.pieces(QUEEN, board.sideToMove) &&
        board.pieces(PAWN, board.sideToMove) != 0)
        return true;
    
    // King + single minor piece (often zugzwang-prone)
    if (hasSingleMinorPiece(board, board.sideToMove))
        return true;
    
    return false;
}

bool ZugzwangDetector::hasLowPieceCount(const Board& board, Color c) {
    // Count non-king pieces for side
    int pieceCount = 0;
    pieceCount += popcount(board.pieces(PAWN, c));
    pieceCount += popcount(board.pieces(KNIGHT, c));
    pieceCount += popcount(board.pieces(BISHOP, c));
    pieceCount += popcount(board.pieces(ROOK, c));
    pieceCount += popcount(board.pieces(QUEEN, c));
    
    // Low piece count is zugzwang-prone
    return pieceCount <= 2;
}

bool ZugzwangDetector::hasPawnStructureZugzwangRisk(const Board& board) {
    Color us = board.sideToMove;
    Color them = ~us;
    
    // Get pawns for both sides
    U64 ourPawns = board.pieces(PAWN, us);
    U64 theirPawns = board.pieces(PAWN, them);
    
    // No pawns, no pawn-related zugzwang
    if (ourPawns == 0)
        return false;
    
    // Look for blocked pawn structures
    U64 blockedPawns = 0;
    
    if (us == White) {
        // White pawns blocked by black pawns (one square ahead)
        blockedPawns = (ourPawns << 8) & theirPawns;
    } else {
        // Black pawns blocked by white pawns (one square ahead)
        blockedPawns = (ourPawns >> 8) & theirPawns;
    }
    
    // If most/all of our pawns are blocked, high zugzwang risk
    int totalPawns = popcount(ourPawns);
    int blockedCount = popcount(blockedPawns);
    
    return (blockedCount >= totalPawns - 1) && (totalPawns >= 2);
}

bool ZugzwangDetector::isKingAndPawnsEndgame(const Board& board) {
    // Pure king+pawns endgame (no pieces)
    return (!board.pieces(KNIGHT, White) && !board.pieces(BISHOP, White) &&
            !board.pieces(ROOK, White) && !board.pieces(QUEEN, White) &&
            !board.pieces(KNIGHT, Black) && !board.pieces(BISHOP, Black) &&
            !board.pieces(ROOK, Black) && !board.pieces(QUEEN, Black) &&
            (board.pieces(PAWN, White) != 0 || board.pieces(PAWN, Black) != 0));
}

bool ZugzwangDetector::hasSingleMinorPiece(const Board& board, Color c) {
    // Check if side has exactly one knight or bishop
    int knightCount = popcount(board.pieces(KNIGHT, c));
    int bishopCount = popcount(board.pieces(BISHOP, c));
    
    // Must have exactly one minor piece
    if (knightCount + bishopCount != 1)
        return false;
    
    // And no other pieces except king and pawns
    return (!board.pieces(ROOK, c) && !board.pieces(QUEEN, c));
}

bool ZugzwangDetector::hasSufficientMaterial(const Board& board, Color c) {
    // Check if side has enough material to create threats
    
    // Queens and rooks can always create threats
    if (board.pieces(QUEEN, c) || board.pieces(ROOK, c))
        return true;
    
    // Pawns can usually create threats
    if (board.pieces(PAWN, c))
        return true;
    
    // Two minor pieces can usually create threats
    int minorCount = popcount(board.pieces(KNIGHT, c)) + 
                     popcount(board.pieces(BISHOP, c));
    if (minorCount >= 2)
        return true;
    
    // Single bishop or knight - depends on position, but generally weak
    return false;
}

} // namespace Chess