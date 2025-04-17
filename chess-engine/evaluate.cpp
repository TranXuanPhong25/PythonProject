#include "evaluate.hpp"
#include "evaluate_features.hpp"
#include "evaluate_attacks.hpp"
#include "chess.hpp"

// Convert from Chess::Square to 0-63 index for piece-square tables
// Square mapping is critical for evaluation symmetry
inline int mapToTableIndex(Square sq) {
    // Our piece-square tables are defined with A8=0, H8=7, A1=56, H1=63
    // This matches directly with the PST definitions
    return (7 - square_rank(sq)) * 8 + square_file(sq);
}

// Flips a square from white's perspective to black's perspective
inline int flipSquareVertically(Square sq) {
    // For symmetry, black pieces need to see the board flipped vertically
    // A1 (rank 0) becomes A8 (rank 7), etc.
    return square_file(sq) + (7 - square_rank(sq)) * 8;
}

// Calculate game phase based on remaining material (0.0 = opening, 1.0 = endgame)
inline float getGamePhase(const Board &board)
{
   int remainingMaterial =
       popcount(board.pieces(PAWN, White) | board.pieces(PAWN, Black)) * PAWN_VALUE +
       popcount(board.pieces(KNIGHT, White) | board.pieces(KNIGHT, Black)) * KNIGHT_VALUE +
       popcount(board.pieces(BISHOP, White) | board.pieces(BISHOP, Black)) * BISHOP_VALUE +
       popcount(board.pieces(ROOK, White) | board.pieces(ROOK, Black)) * ROOK_VALUE +
       popcount(board.pieces(QUEEN, White) | board.pieces(QUEEN, Black)) * QUEEN_VALUE;

   float phase = 1.0f - std::min(1.0f, remainingMaterial / float(totalMaterial));
   return phase;
}

int evaluate(const Board &board)
{
   int score = 0;
   float endgameWeight = getGamePhase(board);
      
   // Material counting using bitboards for efficiency
   for (PieceType pt = PAWN; pt <= KING; ++pt)
   {
       Bitboard whitePieces = board.pieces(pt, White);
       Bitboard blackPieces = board.pieces(pt, Black);

       // WHITE
       while (whitePieces)
       {
           Square sq = static_cast<Square>(pop_lsb(whitePieces));
           score += PIECE_VALUES[pt];

           // Direct table mapping for white pieces
           int tableIdx = mapToTableIndex(sq);

           switch (pt)
           {
           case PAWN:
               score += PAWN_PST[tableIdx];
               break;
           case KNIGHT:
               score += KNIGHT_PST[tableIdx];
               break;
           case BISHOP:
               score += BISHOP_PST[tableIdx];
               break;
           case ROOK:
               score += ROOK_PST[tableIdx];
               break;
           case QUEEN:
               score += QUEEN_PST[tableIdx];
               break;
           case KING:
               score += KING_MG_PST[tableIdx] * (1.0f - endgameWeight) +
                        KING_EG_PST[tableIdx] * endgameWeight;
               break;
           }
       }

       // BLACK
       while (blackPieces)
       {
           Square sq = static_cast<Square>(pop_lsb(blackPieces));
           score -= PIECE_VALUES[pt];

           // For black pieces, we need to flip the square vertically
           // This ensures proper symmetry when using the same tables
           Square flipped_sq = static_cast<Square>(flipSquareVertically(sq));
           int tableIdx = mapToTableIndex(flipped_sq);

           switch (pt)
           {
           case PAWN:
               score -= PAWN_PST[tableIdx];
               break;
           case KNIGHT:
               score -= KNIGHT_PST[tableIdx];
               break;
           case BISHOP:
               score -= BISHOP_PST[tableIdx];
               break;
           case ROOK:
               score -= ROOK_PST[tableIdx];
               break;
           case QUEEN:
               score -= QUEEN_PST[tableIdx]; 
               break;
           case KING:
               score -= KING_MG_PST[tableIdx] * (1.0f - endgameWeight) +
                        KING_EG_PST[tableIdx] * endgameWeight;
               break;
           }
       }
   }

   // Bishop pair bonus - exactly balanced for both sides
   if (popcount(board.pieces(BISHOP, White)) >= 2)
      score += 30;
   if (popcount(board.pieces(BISHOP, Black)) >= 2)
      score -= 30;

   // Rook pair bonus - exactly balanced for both sides
   if (popcount(board.pieces(ROOK, White)) >= 2)
      score += 20;
   if (popcount(board.pieces(ROOK, Black)) >= 2)
      score -= 20;
   
   // Ensure pawn structure evaluation is balanced
   int pawnScore = evaluatePawnStructure(board);
   int centerScore = evaluateCenterControl(board);
   int attackScore = evaluateAttacks(board);

   score += pawnScore + centerScore + attackScore;

   // Return score from perspective of side to move
   return board.sideToMove == White ? score : -score;
}