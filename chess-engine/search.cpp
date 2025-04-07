#include "search.hpp"
#include "types.h"
#include <algorithm>

// Piece values for evaluation
const int PAWN_VALUE = 100;
const int KNIGHT_VALUE = 320;
const int BISHOP_VALUE = 330;
const int ROOK_VALUE = 500;
const int QUEEN_VALUE = 900;

// Simple evaluation function - just counts material
int evaluate(const Board &board)
{
   int score = 0;

   // Material count for White
   score += popcount(board.pieces(PAWN, White)) * PAWN_VALUE;
   score += popcount(board.pieces(KNIGHT, White)) * KNIGHT_VALUE;
   score += popcount(board.pieces(BISHOP, White)) * BISHOP_VALUE;
   score += popcount(board.pieces(ROOK, White)) * ROOK_VALUE;
   score += popcount(board.pieces(QUEEN, White)) * QUEEN_VALUE;

   // Material count for Black
   score -= popcount(board.pieces(PAWN, Black)) * PAWN_VALUE;
   score -= popcount(board.pieces(KNIGHT, Black)) * KNIGHT_VALUE;
   score -= popcount(board.pieces(BISHOP, Black)) * BISHOP_VALUE;
   score -= popcount(board.pieces(ROOK, Black)) * ROOK_VALUE;
   score -= popcount(board.pieces(QUEEN, Black)) * QUEEN_VALUE;

   // Return score from the perspective of the side to move
   return board.sideToMove == White ? score : -score;
}

// Minimax search with alpha-beta pruning
int alphaBeta(Board &board, int depth, int alpha, int beta)
{
   if (depth == 0)
   {
      return evaluate(board);
   }
   Movelist moves;

   if (board.sideToMove == White)
   {
      Movegen::legalmoves<White, ALL>(board, moves);
   }
   else
   {
      Movegen::legalmoves<Black, ALL>(board, moves);
   }

   if (moves.size == 0)
   {
      // Check for checkmate or stalemate
      std::cout << "Check for checkmate or stalemate\n";
      bool inCheck = (board.sideToMove == White) ? board.isSquareAttacked(Black, board.KingSQ(White)) : board.isSquareAttacked(White, board.KingSQ(Black));

      if (inCheck)
      {
         // Checkmate - return mated score
         return board.sideToMove == White ? mated_in(0) : mate_in(0);
      }
      else
      {
         // Stalemate - return draw score
         return 0;
      }
   }
   if (board.sideToMove == White)
   {
      int maxEval = mated_in(0);
      for (int i = 0; i < moves.size; i++)
      {
         Move move = moves[i].move;
         board.makeMove(move);
         int eval = alphaBeta(board, depth - 1, alpha, beta);
         board.unmakeMove(move);

         maxEval = std::max(maxEval, eval);
         if (beta <= maxEval)
            break; // Beta cutoff
         alpha = std::max(alpha, maxEval);
      }
      return maxEval;
   }
   else
   {
      int minEval = mate_in(0);
      for (int i = 0; i < moves.size; i++)
      {
         Move move = moves[i].move;
         board.makeMove(move);
         int eval = alphaBeta(board, depth - 1, alpha, beta);
         board.unmakeMove(move);

         minEval = std::min(minEval, eval);
         if (minEval <= alpha)
            break; // Alpha cutoff
         beta = std::min(beta, minEval);
      }
      return minEval;
   }
}

// Find the best move at the given depth
Move getBestMove(Board &board, int depth)
{
   Movelist moves;
   if (board.sideToMove == White)
   {
      Movegen::legalmoves<White, ALL>(board, moves);
   }
   else
   {
      Movegen::legalmoves<Black, ALL>(board, moves);
   }

   if (moves.size == 0)
   {
      return NO_MOVE;
   }

   Move bestMove = moves[0].move;
   int bestScore = board.sideToMove == White ? mated_in(0) : mate_in(0);

   for (int i = 0; i < moves.size; i++)
   {
      Move move = moves[i].move;
      board.makeMove(move);
      int score = alphaBeta(board, depth - 1, mated_in(0), mate_in(0));
      // random score
      //  int score = rand() % 1000 - 500;
      board.unmakeMove(move);

      if ((board.sideToMove == White && score > bestScore) ||
          (board.sideToMove == Black && score < bestScore))
      {
         bestScore = score;
         bestMove = move;
      }
   }

   return bestMove;
}