#include "evaluate.hpp"
#include "evaluate_features.hpp"
#include "chess.hpp"

// Convert from Chess::Square to 0-63 index for piece-square tables
inline int squareToIndex(Square sq)
{
   return (7 - square_rank(sq)) * 8 + square_file(sq);
}

// Get flipped square for black pieces (to use same tables for both colors)
inline int getFlippedSquare(Square sq)
{
   return squareToIndex(sq) ^ 56; // Flips vertically
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

      // Process white pieces
      while (whitePieces)
      {
         Square sq = static_cast<Square>(pop_lsb(whitePieces)); // Get and remove the least significant bit
         score += PIECE_VALUES[pt];        // Add material value
         if (pt == KING)
         {
            // Interpolate between middlegame and endgame king tables
            score += KING_MG_PST[sq] * (1.0f - endgameWeight) + KING_EG_PST[sq] * endgameWeight;
         }
         else
         {
            score += PAWN_PST[sq]; // Add piece-square table bonus
         }
      }

      // Process black pieces
      while (blackPieces)
      {
         Square sq = static_cast<Square>(pop_lsb(blackPieces)); // Get and remove the least significant bit
         score -= PIECE_VALUES[pt];        // Subtract material value
         if (pt == KING)
         {
            // Interpolate between middlegame and endgame king tables
            score -= KING_MG_PST[getFlippedSquare(sq)] * (1.0f - endgameWeight) +
                     KING_EG_PST[getFlippedSquare(sq)] * endgameWeight;
         }
         else
         {
            score -= PAWN_PST[getFlippedSquare(sq)]; // Subtract piece-square table bonus
         }
      }
   }

   // Bishop pair bonus
   if (popcount(board.pieces(BISHOP, White)) >= 2)
      score += 30;
   if (popcount(board.pieces(BISHOP, Black)) >= 2)
      score -= 30;

       // Rook pair bonus
   if (popcount(board.pieces(ROOK, White)) >= 2)
      score += 20;
   if (popcount(board.pieces(ROOK, Black)) >= 2)
      score -= 20;
   
   //  //Evaluate PawnStructure
   score += (board.sideToMove == White ? evaluatePawnStructure(board) : -evaluatePawnStructure(board));
   
   //Evaluate center control
   score += (board.sideToMove == White ? evaluateCenterControl(board) : -evaluateCenterControl(board));

   // Return score from perspective of side to move
   return board.sideToMove == White ? score : -score;
}