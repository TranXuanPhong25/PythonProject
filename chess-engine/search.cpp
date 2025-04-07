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

// Make a move on the board and update necessary fields
void makeMove(Board &board, Move move)
{
   // Save current state
   board.addStateHistory(State(board.enPassantSquare, board.castlingRights, board.halfMoveClock, board.pieceAtB(to(move))));

   Square from_sq = from(move);
   Square to_sq = to(move);
   Piece moving_piece = board.pieceAtB(from_sq);
   Piece captured_piece = board.pieceAtB(to_sq);

   // Handle captures
   if (captured_piece != None)
   {
      board.removePiece(captured_piece, to_sq);
      board.halfMoveClock = 0;
   }
   else if (type_of_piece(moving_piece) == PAWN)
   {
      board.halfMoveClock = 0;
   }
   else
   {
      board.halfMoveClock++;
   }

   // Move the piece
   board.movePiece(moving_piece, from_sq, to_sq);

   // Update enPassantSquare
   board.enPassantSquare = NO_SQ;

   // Update castling rights if necessary
   if (type_of_piece(moving_piece) == KING)
   {
      board.removeCastlingRightsAll(board.sideToMove);
   }
   else if (type_of_piece(moving_piece) == ROOK)
   {
      board.removeCastlingRightsRook(from_sq);
   }

   // Switch sides
   board.sideToMove = ~board.sideToMove;

   // Increment full move number if black just moved
   if (board.sideToMove == White)
   {
      board.fullMoveNumber++;
   }

   // Recalculate board state
   board.occEnemy = board.Enemy(board.sideToMove);
   board.occUs = board.Us(board.sideToMove);
   board.occAll = board.occUs | board.occEnemy;
   board.enemyEmptyBB = ~board.occUs;
}

// Unmake the last move
void unmakeMove(Board &board)
{
   // Restore the previous state
   State prev_state = board.getStateHistory().back();
   board.popStateHistory();

   // Switch sides back
   board.sideToMove = ~board.sideToMove;

   // Restore state variables
   board.enPassantSquare = prev_state.enPassant;
   board.castlingRights = prev_state.castling;
   board.halfMoveClock = prev_state.halfMove;

   // Decrement full move counter if white is now to move
   if (board.sideToMove == White)
   {
      board.fullMoveNumber--;
   }

   // Recalculate board state
   board.occEnemy = board.Enemy(board.sideToMove);
   board.occUs = board.Us(board.sideToMove);
   board.occAll = board.occUs | board.occEnemy;
   board.enemyEmptyBB = ~board.occUs;
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
         makeMove(board, move);
         int eval = alphaBeta(board, depth - 1, alpha, beta);
         unmakeMove(board);

         maxEval = std::max(maxEval, eval);
         alpha = std::max(alpha, eval);
         if (beta <= alpha)
            break; // Beta cutoff
      }
      return maxEval;
   }
   else
   {
      int minEval = mate_in(0);
      for (int i = 0; i < moves.size; i++)
      {
         Move move = moves[i].move;
         makeMove(board, move);
         int eval = alphaBeta(board, depth - 1, alpha, beta);
         unmakeMove(board);

         minEval = std::min(minEval, eval);
         beta = std::min(beta, eval);
         if (beta <= alpha)
            break; // Alpha cutoff
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
      makeMove(board, move);
      int score = alphaBeta(board, depth - 1, mated_in(0), mate_in(0));
      unmakeMove(board);

      if ((board.sideToMove == White && score > bestScore) ||
          (board.sideToMove == Black && score < bestScore))
      {
         bestScore = score;
         bestMove = move;
      }
   }

   return bestMove;
}