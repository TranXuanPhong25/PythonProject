#include "search.hpp"

#include <algorithm>

int score_to_tt(int score, int ply)
{
   if (score >= IS_MATE_IN_MAX_PLY)
   {

      return score - ply;
   }
   else if (score <= IS_MATED_IN_MAX_PLY)
   {

      return score + ply;
   }

   return score;
}

int score_from_tt(int score, int ply)
{
   if (score >= IS_MATE_IN_MAX_PLY)
   {

      return score - ply;
   }
   else if (score <= IS_MATED_IN_MAX_PLY)
   {

      return score + ply;
   }

   return score;
}

int quiescence(Board &board, int alpha, int beta, TranspositionTable *table, int ply)
{
   int standPat = evaluate(board);
   int bestValue = standPat;
   if (standPat >= beta)
      return standPat;

   if (alpha < standPat)
      alpha = standPat;
   bool ttHit = false;
   bool is_pvnode = (beta - alpha) > 1;

   TTEntry &tte = table->probe_entry(board.hashKey, ttHit, ply);
   const int tt_score = ttHit ? score_from_tt(tte.get_score(), ply) : 0;
   if (!is_pvnode && ttHit)
   {
      if ((tte.flag == HFALPHA && tt_score <= alpha) || (tte.flag == HFBETA && tt_score >= beta) ||
          (tte.flag == HFEXACT))
         return tt_score;
   }
   Move bestmove = NO_MOVE;
   Movelist captures;
   if (board.sideToMove == White)
      Movegen::legalmoves<White, CAPTURE>(board, captures);
   else
      Movegen::legalmoves<Black, CAPTURE>(board, captures);

   scoreMoves(captures, board, 0);
   std::sort(captures.begin(), captures.end(), std::greater<ExtMove>());

   for (int i = 0; i < captures.size; i++)
   {
      Move move = captures[i].move;
      board.makeMove(move);
      table->prefetch_tt(board.hashKey);
      int score = -quiescence(board, -beta, -alpha, table, ply + 1);
      board.unmakeMove(move);

      if (score > bestValue)
      {
         bestmove = move;
         bestValue = score;

         if (score > alpha)
         {
            alpha = score;
            if (score >= beta)
            {
               break;
            }
         }
      }
   }
   int flag = bestValue >= beta ? HFBETA : HFALPHA;

   table->store(board.hashKey, flag, bestmove, 0, score_to_tt(bestValue, ply), standPat, ply, is_pvnode);

   return bestValue;
}
// Minimax search with alpha-beta pruning
int negamax(Board &board, int depth, int alpha, int beta, TranspositionTable *table, int ply)
{
   if (depth == 0)
   {
      return quiescence(board, alpha, beta, table, ply);
   }
   // Check if position is already in TT
   U64 posKey = board.hashKey;
   table->prefetch_tt(posKey);

   // TT lookup
   bool ttHit;
   TTEntry &entry = table->probe_entry(posKey, ttHit, ply);

   if (ttHit && entry.depth >= depth)
   {
      // Use the value from TT based on bound type
      if (entry.flag == HFEXACT)
         return entry.score;
      if (entry.flag == HFALPHA && entry.score <= alpha)
         return alpha;
      if (entry.flag == HFBETA && entry.score >= beta)
         return beta;
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

   if (static_cast<int>(moves.size) == 0)
   {
      // Check for checkmate or stalemate
      // std::cout << "Check for checkmate or stalemate\n";
      int score = 0;
      bool inCheck = (board.sideToMove == White) ? board.isSquareAttacked(Black, board.KingSQ(White)) : board.isSquareAttacked(White, board.KingSQ(Black));

      if (inCheck)
      {
         // Checkmate - return mated score
         score = board.sideToMove == White ? mated_in(ply) : mate_in(-ply);
      }
      else
      {
         // Stalemate - return draw score
         score = 0;
      }
      table->store(posKey, HFEXACT, NO_MOVE, depth, score, score, ply, true);
      return score;
   }

   // Try TT move first if available
   Move ttMove = ttHit ? entry.move : NO_MOVE;

   scoreMoves(moves, board, depth);

   std::sort(moves.begin(), moves.end(), std::greater<ExtMove>());

   int bestScore = -INF_BOUND;
   Move bestMove = NO_MOVE;
   uint8_t nodeFlag = HFALPHA;

   for (int i = 0; i < moves.size; i++)
   {
      Move move = moves[i].move;
      board.makeMove(move);
      // Negamax recursively calls with inverted bounds
      int score = -negamax(board, depth - 1, -beta, -alpha, table, ply + 1);
      board.unmakeMove(move);

      if (score > bestScore)
      {
         bestScore = score;
         bestMove = move;

         if (score > alpha)
         {
            nodeFlag = HFEXACT;
            alpha = score;

            if (alpha >= beta)
            {
               nodeFlag = HFBETA;
               break; // Beta cutoff
            }
         }
      }
   }

   // Store position in TT
   table->store(posKey, nodeFlag, bestMove, depth, bestScore, evaluate(board), ply, nodeFlag == HFEXACT);

   return bestScore;
}

// Find the best move at the given depth
Move getBestMoveIterative(Board &board, int depth, TranspositionTable *table)
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

   // Check if we have a TT move for this position
   Move ttMove = table->probeMove(board.hashKey);
   if (ttMove != NO_MOVE)
   {
      // Verify the move is legal
      for (int i = 0; i < moves.size; i++)
      {
         if (moves[i].move == ttMove)
         {
            // Start with the TT move as best
            // but still search to confirm it's best
            break;
         }
      }
   }

   Move bestMove = moves[0].move;
   int bestScore = -INF_BOUND;
   int alpha = -INF_BOUND;
   int beta = INF_BOUND;

   // Score and sort moves - put TT move first

   scoreMoves(moves, board, depth, ttMove);
   std::sort(moves.begin(), moves.end(), std::greater<ExtMove>());

   for (int i = 0; i < moves.size; i++)
   {
      Move move = moves[i].move;
      board.makeMove(move);
      int score = -negamax(board, depth - 1, -beta, -alpha, table, 1);
      board.unmakeMove(move);

      if (score > bestScore)
      {
         bestScore = score;
         bestMove = move;
         alpha = std::max(alpha, score);
      }
   }
   table->store(board.hashKey, HFEXACT, bestMove, depth, bestScore, evaluate(board), 0, true);

   return bestMove;
}
Move getBestMove(Board &board, int maxDepth, TranspositionTable *table)
{
   Move bestMove = NO_MOVE;

   for (int depth = 1; depth <= maxDepth; depth++)
   {

      Move currentBestMOve = getBestMoveIterative(board, depth, table);
      if (currentBestMOve != NO_MOVE)
      {
         bestMove = currentBestMOve;
      }
   }

   return bestMove;
}