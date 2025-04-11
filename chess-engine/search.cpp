#include "search.hpp"

#include <algorithm>

int getPieceCounts(const Board &board, Color color)
{
   int count = 0;
   count += popcount(board.pieces(PAWN, color));
   count += popcount(board.pieces(KNIGHT, color));
   count += popcount(board.pieces(BISHOP, color));
   count += popcount(board.pieces(ROOK, color));
   count += popcount(board.pieces(QUEEN, color));
   count += popcount(board.pieces(KING, color));
   return count;
}
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
   if (ply >= MAX_PLY - 1)
      return evaluate(board);
   if (board.isRepetition())
   {
      return 0;
   }

   int standPat = evaluate(board);
   if (standPat >= beta)
      return beta;

   if (standPat > alpha)
      alpha = standPat;

   bool ttHit = false;
   bool is_pvnode = (beta - alpha) > 1;

   TTEntry &tte = table->probe_entry(board.hashKey, ttHit);
   const int tt_score = ttHit ? score_from_tt(tte.get_score(), ply) : 0;
   if (!is_pvnode && ttHit)
   {
      if ((tte.flag == HFALPHA && tt_score <= alpha) || (tte.flag == HFBETA && tt_score >= beta) ||
          (tte.flag == HFEXACT))
         return tt_score;
   }

   int bestValue = standPat;
   Move bestmove = NO_MOVE;
   int moveCount = 0;

   Movelist captures;
   if (board.sideToMove == White)
      Movegen::legalmoves<White, CAPTURE>(board, captures);
   else
      Movegen::legalmoves<Black, CAPTURE>(board, captures);

   ScoreMovesForQS(board, captures, tte.move);
   std::sort(captures.begin(), captures.end(), std::greater<ExtMove>());

   for (int i = 0; i < captures.size; i++)
   {
      Move move = captures[i].move;

      if (!see(board, move, -65) )
      {
         continue;
      }
      if (captures[i].value < GoodCaptureScore && moveCount >= 1)
      {
         continue;
      }
      board.makeMove(move);
      table->prefetch_tt(board.hashKey);
      moveCount++;
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

   table->store(board.hashKey, flag, bestmove, 0, score_to_tt(bestValue, ply), standPat);

   return bestValue;
}
// Minimax search with alpha-beta pruning
int negamax(Board &board, int depth, int alpha, int beta, TranspositionTable *table, int ply)
{
   if (depth <= 0)
   {
      return quiescence(board, alpha, beta, table, ply);
   }
   // Check if position is already in TT
   U64 posKey = board.hashKey;
   table->prefetch_tt(posKey);

   // TT lookup
   bool ttHit;
   TTEntry &entry = table->probe_entry(posKey, ttHit);
   bool is_pvnode = (beta - alpha) > 1;
   int tt_score = ttHit ? score_from_tt(entry.get_score(), ply) : 0;
   if (!is_pvnode && ttHit && entry.depth >= depth)
   {
      // Use the value from TT based on bound type
      if (entry.flag == HFEXACT)
         return entry.score;
      if (entry.flag == HFALPHA && entry.score <= alpha)
         return alpha;
      if (entry.flag == HFBETA && entry.score >= beta)
         return beta;
   }
   bool inCheck = board.isSquareAttacked(~board.sideToMove, board.KingSQ(board.sideToMove));
   bool isRoot = (ply == 0);
   int staticEval;
   int eval = 0;

   if (ttHit)
   {
      staticEval = entry.eval;
      eval = tt_score;
   }
   else
   {
      staticEval = evaluate(board);
   }
   // Reverse Futility Pruning
   // Only apply at non-PV nodes, not in check, and at shallow depths
   if (!is_pvnode && !inCheck && depth <= 8 && eval >= beta)
   {
      // The margin increases with depth
      int margin = 120 * depth;

      // If static eval is well above beta, assume position is too good
      // and will lead to a beta cutoff anyway
      if (staticEval - margin >= beta)
      {
         return beta; // Return beta as a lower bound
      }
   }

   if (!inCheck && !is_pvnode && !isRoot)
   {

      /* Null move pruning
       * If we give our opponent a free move and still maintain beta, we prune
       * some nodes.
       */
      if (board.nonPawnMat(board.sideToMove) && depth >= 3 && (!ttHit || entry.flag != HFALPHA || eval >= beta))
      {
         int R = 2;
         // Skip null move in endgame positions (simplistic approach)
         if (getPieceCounts(board, board.sideToMove) > 5) // Adjusted threshold for better tuning
         {
            board.makeNullMove();
            int nullScore = -negamax(board, depth - 1 - R, -beta, -beta + 1, table, ply + 1); // Use a reduction factor R
            board.unmakeNullMove();

            if (nullScore >= beta)
            {
               if (nullScore > ISMATE)
               {
                  nullScore = beta;
               }
               return nullScore; // Beta cutoff
            }
         }
      }
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
      table->store(posKey, HFEXACT, NO_MOVE, depth, score, score);
      return score;
   }

   // Try TT move first if available
   Move ttMove = ttHit ? entry.move : NO_MOVE;

   scoreMoves(moves, board, ttMove, ply);

   std::sort(moves.begin(), moves.end(), std::greater<ExtMove>());

   int bestScore = -INF_BOUND;
   Move bestMove = NO_MOVE;
   uint8_t nodeFlag = HFALPHA;
   for (int i = 0; i < moves.size; i++)
   {
      Move move = moves[i].move;
      if (!see(board, move, -65*depth))
      {
         continue;
      }

      board.makeMove(move);
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
               if (!is_capture(board, move) && !promoted(move))
               {
                  addKillerMove(move, ply);

                  // Also update history table for quiet moves
                  updateHistory(board, move, depth);
               }

               nodeFlag = HFBETA;
               break; // Beta cutoff
            }
         }
      }
   }

   // Store position in TT
   table->store(posKey, nodeFlag, bestMove, depth, bestScore, evaluate(board));

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

   scoreMoves(moves, board, ttMove);
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
   table->store(board.hashKey, HFEXACT, bestMove, depth, bestScore, evaluate(board));

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