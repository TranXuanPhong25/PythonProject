// #ifndef EVALUATE_PIECES_HPP
// #define EVALUATE_PIECES_HPP

// #include "types.hpp"  // Include this first to get basic types
// #include "chess.hpp"  // Then include chess.hpp for the full implementation

// Chess::Bitboard getKingRing(const Chess::Board& board, Chess::Color color);

// // Helper structures
// struct EvalInfo {
//     const Chess::Board& board;
//     int mgScore;
//     int egScore;
    
//     // Cached bitboards
//     Chess::Bitboard kingRings[2];
//     Chess::Bitboard outpostSquares[2];
    
//     EvalInfo(const Chess::Board& b) : board(b), mgScore(0), egScore(0) {
//         // Initialize king rings
//         kingRings[Chess::White] = getKingRing(board, Chess::White);
//         kingRings[Chess::Black] = getKingRing(board, Chess::Black);
        
//         // Initialize outpost squares
//         outpostSquares[Chess::White] = 0x00007E7E00000000ULL; // White outpost squares (ranks 4-5)
//         outpostSquares[Chess::Black] = 0x00000000007E7E00ULL; // Black outpost squares (ranks 3-4)
//     }
// };

// // Helper functions

// bool canPawnAttackSquare(const Chess::Board& board, Chess::Square sq, Chess::Color attackingColor);
// bool isPawnProtected(const Chess::Board& board, Chess::Square sq, Chess::Color color);

// // Function declarations for evaluation
// void evaluatePiecesAttackingKingRing(EvalInfo& ei, Chess::Color color, int& attackCount);
// void evaluateOutposts(EvalInfo& ei, Chess::Color color);
// void evaluateRooks(EvalInfo& ei, Chess::Color color);
// void evaluateBishops(EvalInfo& ei, Chess::Color color);
// void evaluateKnights(EvalInfo& ei, Chess::Color color);
// void evaluateQueens(EvalInfo& ei, Chess::Color color);
// void evaluateKingSafety(EvalInfo& ei, Chess::Color color);

// // Main evaluation functions
// int evaluatePiecesMg(const Chess::Board& board);
// int evaluatePiecesEg(const Chess::Board& board);
// int evaluatePieces(const Chess::Board& board);
// bool isEndgame(const Chess::Board& board);

// #endif // EVALUATE_PIECES_HPP