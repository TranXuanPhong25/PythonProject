#include "evaluate.hpp"

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
   const int totalMaterial =
       16 * PAWN_VALUE +
       4 * KNIGHT_VALUE +
       4 * BISHOP_VALUE +
       4 * ROOK_VALUE +
       2 * QUEEN_VALUE;

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

   // Material counting
   for (Square sq = SQ_A1; sq <= SQ_H8; ++sq)
   {
      Piece piece = board.pieceAtB(sq);
      if (piece == None)
         continue;

      PieceType pt = type_of_piece(piece);
      Color color = (piece >= BlackPawn) ? Black : (piece == None ? NO_COLOR : White);

      // Material value
      int pieceValue = PIECE_VALUES[pt];
      
      // Apply material value
      if (color == White)
         score += pieceValue;
      else
         score -= pieceValue;

      // Piece-square table bonuses
      int pstBonus = 0;
      int pstIndex = (color == White) ? squareToIndex(sq) : getFlippedSquare(sq);

      switch (pt)
      {
      case PAWN:
         pstBonus = PAWN_PST[pstIndex];
         break;
      case KNIGHT:
         pstBonus = KNIGHT_PST[pstIndex];
         break;
      case BISHOP:
         pstBonus = BISHOP_PST[pstIndex];
         break;
      case ROOK:
         pstBonus = ROOK_PST[pstIndex];
         break;
      case QUEEN:
         pstBonus = QUEEN_PST[pstIndex];
         break;
      case KING:
         // Interpolate between middlegame and endgame king tables
         pstBonus = KING_MG_PST[pstIndex] * (1.0f - endgameWeight) +
                    KING_EG_PST[pstIndex] * endgameWeight;
         break;
      default:
         break;
      }

      // Apply piece-square table bonus
      if (color == White)
         score += pstBonus;
      else
         score -= pstBonus;
   }

   // Bishop pair bonus
   if (popcount(board.pieces(BISHOP, White)) >= 2)
      score += 30;
   if (popcount(board.pieces(BISHOP, Black)) >= 2)
      score -= 30;

   // Return score from perspective of side to move
   return board.sideToMove == White ? score : -score;
}