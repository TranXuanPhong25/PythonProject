#include "evaluate.hpp"
#include "evaluate_features.hpp"
#include "evaluate_pieces.hpp"
#include "evaluate_threatEG.hpp"
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

       // WHITE
       while (whitePieces)
       {
           Square sq = static_cast<Square>(pop_lsb(whitePieces));
           score += PIECE_VALUES[pt];

           int idx = squareToIndex(sq);

           switch (pt)
           {
           case PAWN:
               score += PAWN_PST[idx];
               break;
           case KNIGHT:
               score += KNIGHT_PST[idx];
               break;
           case BISHOP:
               score += BISHOP_PST[idx];
               break;
           case ROOK:
               score += ROOK_PST[idx];
               break;
           case QUEEN:
               score += QUEEN_PST[idx];
               break;
           case KING:
               score += KING_MG_PST[idx] * (1.0f - endgameWeight) +
                        KING_EG_PST[idx] * endgameWeight;
               break;
           }
       }

       // BLACK
       while (blackPieces)
       {
           Square sq = static_cast<Square>(pop_lsb(blackPieces));
           score -= PIECE_VALUES[pt];

           int flippedIdx = getFlippedSquare(sq);

           switch (pt)
           {
           case PAWN:
               score -= PAWN_PST[flippedIdx];
               break;
           case KNIGHT:
               score -= KNIGHT_PST[flippedIdx];
               break;
           case BISHOP:
               score -= BISHOP_PST[flippedIdx];
               break;
           case ROOK:
               score -= ROOK_PST[flippedIdx];
               break;
           case QUEEN:
               score -= QUEEN_PST[flippedIdx]; 
               break;
           case KING:
               score -= KING_MG_PST[flippedIdx] * (1.0f - endgameWeight) +
                        KING_EG_PST[flippedIdx] * endgameWeight;
               break;
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
   
   score += evaluatePieces(board);

   // Evaluate PawnStructure
   score += evaluatePawnStructure(board);
   
   // Evaluate center control
   score += evaluateCenterControl(board);
   
   // Apply threat evaluation in endgame with proper scaling based on game phase
   if (endgameWeight > 0.0f) {
      int threatScore = threats_endgame(board);
      score += threatScore * endgameWeight;
   }

   // Return score from perspective of side to move
   return board.sideToMove == White ? score : -score;
}