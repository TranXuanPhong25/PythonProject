#include "search.hpp"

#include <algorithm>

int quiescence(Board &board, int alpha, int beta, TranspositionTable *table, int ply)
{
   int standPat = evaluate(board);
   int bestValue = standPat;
   if (standPat >= beta)
      return standPat;

   if (alpha < standPat)
      alpha = standPat;
   auto entry = table->probe(board.getHash());
    if (entry && entry->depth >= ply) return entry->value;
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
      int score = -quiescence(board, -beta, -alpha, table, ply + 1);
      board.unmakeMove(move);

      if (score >= beta)
         return score;
      if (score > bestValue)
         bestValue = score;
      if (score > alpha)
         alpha = score;
   }

   return bestValue;
}
// Minimax search with alpha-beta pruning
int negamax(Board &board, int depth, int alpha, int beta, TranspositionTable *table, int ply)
{

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

   if (depth == 0)
   {
      int eval = quiescence(board, alpha, beta, table, ply);
      // Store leaf node in TT
      table->store(posKey, HFEXACT, NO_MOVE, 0, eval, eval, ply, false);
      return eval;
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
