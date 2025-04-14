#pragma once

#include "chess.hpp"

namespace Chess {

// Endgame type classification
enum EndgameType {
    NO_ENDGAME,    // Not a recognized endgame
    KPK,           // King and pawn vs King
    KBNK,          // King, Bishop, Knight vs King
    KRK,           // King and Rook vs King
    KQKP,          // King and Queen vs King and Pawn
    KRKP,          // King and Rook vs King and Pawn
    KRPKR,         // King, Rook, Pawn vs King and Rook
    KPPK,          // King and two Pawns vs King
    KBK,           // King and Bishop vs King
    KNK,           // King and Knight vs King
    OTHER          // Other endgame types
};

// Zugzwang risk level
enum ZugzwangRisk {
    NONE,       // No zugzwang risk
    LOW,        // Low zugzwang risk
    MEDIUM,     // Medium zugzwang risk
    HIGH,       // High zugzwang risk
    EXTREME     // Extreme zugzwang risk (almost certainly zugzwang)
};

class ZugzwangDetector {
public:
    // Main zugzwang detection function
    static bool isPotentialZugzwang(const Board& board);
    
    // Get the risk level of zugzwang for the current position
    static ZugzwangRisk getZugzwangRisk(const Board& board);
    
    // Determine if null move pruning should be avoided
    static bool shouldAvoidNullMove(const Board& board);
    
    // Classify the endgame type
    static EndgameType classifyEndgame(const Board& board);
    
    // Check if position is nearing 50-move rule
    static bool isNear50MoveRule(const Board& board);
    
    // Get adjusted search depth based on position characteristics
    static int getAdjustedDepth(const Board& board, int originalDepth);
    
    // Get recommended null move reduction based on position type
    static int getNullMoveR(const Board& board, int depth);

    // Material-based zugzwang detection
    static bool hasMaterialZugzwangRisk(const Board& board);
    
    // Check for low piece count
    static bool hasLowPieceCount(const Board& board, Color c);
    
    // Check for blocked pawn structures
    static bool hasPawnStructureZugzwangRisk(const Board& board);
    
    // Check for king+pawns endgame
    static bool isKingAndPawnsEndgame(const Board& board);
    
    // Check for single minor piece
    static bool hasSingleMinorPiece(const Board& board, Color c);
    
    // Check if pieces are sufficient for checkmate
    static bool hasSufficientMaterial(const Board& board, Color c);
};

} // namespace Chess