#include "search.hpp"
#include "evaluate.hpp"
#include "evaluate_features.hpp"

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

      if (!see(board, move, -65))
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
   // Early return checks remain the same
   if (depth <= 0)
      return quiescence(board, alpha, beta, table, ply);

   if (board.isRepetition())
   {
      return 0;
   }
   if (ply >= MAX_PLY - 1)
   {
      return evaluate(board);
   }
   // TT lookup remains the same
   U64 posKey = board.hashKey;
   table->prefetch_tt(posKey);
   bool ttHit;
   TTEntry &entry = table->probe_entry(posKey, ttHit);
   bool is_pvnode = (beta - alpha) > 1;
   int tt_score = ttHit ? score_from_tt(entry.get_score(), ply) : 0;

   // TT cutoff logic remains the same
   if (!is_pvnode && ttHit && entry.depth >= depth)
   {
      if (entry.flag == HFEXACT)
         return entry.score;
      if (entry.flag == HFALPHA && entry.score <= alpha)
         return alpha;
      if (entry.flag == HFBETA && entry.score >= beta)
         return beta;
   }

   // Checkmate detection and pruning logic remain the same
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

   // Reverse Futility Pruning remains the same
   if (!is_pvnode && !inCheck && depth <= 8 && eval >= beta)
   {
      int margin = 120 * depth;
      if (staticEval - margin >= beta)
         return beta;
   }

   // Null move pruning with safety limits
   if (!inCheck && !is_pvnode && !isRoot)
   {
      if (board.nonPawnMat(board.sideToMove) && depth >= 3 && (!ttHit || entry.flag != HFALPHA || eval >= beta))
      {
         // Add stack depth check to prevent excessive recursion
         if (ply < MAX_PLY - 10) // Reserve some stack space
         {
            // Cap the dynamic reduction to prevent excessive depth skipping
            int R = 3;

            // Strengthen zugzwang detection criteria
            bool skipNullMove = getPieceCounts(board, board.sideToMove) <= 3 ||
                                (board.pieces(PAWN, board.sideToMove) == 0 &&
                                 getPieceCounts(board, board.sideToMove) <= 5);

            if (!skipNullMove)
            {
               board.makeNullMove();

               // Use a reduced-depth search with tighter bounds
               int nullScore = -negamax(board, depth - 1 - R, -beta, -beta + 1, table, ply + 1);
               board.unmakeNullMove();

               if (nullScore >= beta)
               {
                  // Only do verification search in specific endgame positions
                  // where zugzwang is more likely
                  if (getPieceCounts(board, board.sideToMove) <= 4 && R >= 4 && depth >= 5)
                  {
                     // Use a more modest depth for verification
                     int verificationDepth = std::min(depth - 3, 5);
                     int verificationScore = negamax(board, verificationDepth, beta - 1, beta, table, ply);
                     if (verificationScore >= beta)
                        return beta;
                  }
                  else
                  {
                     return nullScore;
                  }
               }
            }
         }
      }
   }

   // Add futility pruning
   bool futilityPruning = false;
   if (!is_pvnode && !inCheck && depth <= 3)
   {
      int futilityMargin = 90 * depth;
      futilityPruning = (staticEval + futilityMargin <= alpha);
   }

   // Static Null Move Pruning
   if (!is_pvnode && !inCheck && depth <= 5)
   {
      int margin = depth <= 3 ? 90 * depth : 300 + 50 * (depth - 3);
      if (staticEval - margin >= beta)
         return beta;
   }

   // Razoring - if we're far below alpha, try qsearch
   if (!is_pvnode && !inCheck && depth <= 3 && staticEval + 350 * depth < alpha)
   {
      // Try directly going to quiescence search
      int razorScore = quiescence(board, alpha - 1, alpha, table, ply);
      if (razorScore < alpha)
         return razorScore;
   }

   // Move generation remains the same
   Movelist moves;
   if (board.sideToMove == White)
      Movegen::legalmoves<White, ALL>(board, moves);
   else
      Movegen::legalmoves<Black, ALL>(board, moves);

   // Terminal node check remains the same
   if (static_cast<int>(moves.size) == 0)
   {
      int score = 0;
      if (inCheck)
         score = board.sideToMove == White ? mated_in(ply) : mate_in(-ply);
      else
         score = 0;
      table->store(posKey, HFEXACT, NO_MOVE, depth, score, score);
      return score;
   }

   // Move ordering remains the same
   Move ttMove = ttHit ? entry.move : NO_MOVE;
   scoreMoves(moves, board, ttMove, ply);
   std::sort(moves.begin(), moves.end(), std::greater<ExtMove>());

   // Search variables
   int bestScore = -INF_BOUND;
   Move bestMove = NO_MOVE;
   uint8_t nodeFlag = HFALPHA;
   int movesSearched = 0;

   // Initialize mobility score
   int mobilityScore = evaluateMobility(board, board.sideToMove);

   // Main search loop with simplified LMR
   for (int i = 0; i < moves.size; i++)
   {
      Move move = moves[i].move;

      // Early pruning with SEE
      if (i > 0 && !see(board, move, -65 * depth))
         continue;

      // Then in your move loop:
      bool isCapture = is_capture(board, move);
      bool isPromotion = promoted(move);

      if (futilityPruning && i > 0 && !isCapture && !isPromotion)
         continue; // Skip this quiet move

      // Update mobility incrementally
      updateMobility(board, move, mobilityScore, board.sideToMove);

      board.makeMove(move);
      movesSearched++;

      int score;
      // Full Window Search for first move
      if (i == 0)
      {
         score = -negamax(board, depth - 1, -beta, -alpha, table, ply + 1);
      }
      // LMR for quiet moves after first few moves
      else if (!isRoot && depth >= 3 && !inCheck && !isCapture && !isPromotion)
      {
         // More aggressive reduction formula - vary based on depth and move index
         // Higher numbers = more pruning = faster search
         int R = 1;

         if (i > 4) // More reduction for later moves
            R++;

         if (depth > 5) // More reduction for deeper searches
            R++;

         // Don't reduce too much for killer moves (likely good tactical moves)
         if (move == killerMoves[ply][0] || move == killerMoves[ply][1])
            R = std::max(0, R - 1);

         // Do reduced search with null window
         score = -negamax(board, depth - 1 - R, -alpha - 1, -alpha, table, ply + 1);

         // Only do full search if the reduced search looks promising
         if (score > alpha)
            score = -negamax(board, depth - 1, -beta, -alpha, table, ply + 1);
      }
      // PVS for non-reduced moves
      else if (i > 0)
      {
         // Try null window search first (assume move is not good)
         score = -negamax(board, depth - 1, -alpha - 1, -alpha, table, ply + 1);

         // Only search with full window if it might improve alpha
         if (score > alpha && score < beta)
            score = -negamax(board, depth - 1, -beta, -alpha, table, ply + 1);
      }
      else
      {
         // Full window search for first move
         score = -negamax(board, depth - 1, -beta, -alpha, table, ply + 1);
      }

      board.unmakeMove(move);

      // Restore mobility score
      updateMobility(board, move, mobilityScore, board.sideToMove);

      // Update best score logic remains the same
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
               if (!isCapture && !isPromotion)
               {
                  addKillerMove(move, ply);
                  updateHistory(board, move, depth);
               }

               nodeFlag = HFBETA;
               break;
            }
         }
      }
   }

   // Store position in TT
   table->store(posKey, nodeFlag, bestMove, depth, bestScore, evaluate(board));


   return bestScore;
}

// Find the best move at the given depth
Move getBestMoveIterative(Board &board, int depth, TranspositionTable *table,
                          int alpha, int beta)
{
   int score;
   return getBestMoveIterativeWithScore(board, depth, table, alpha, beta, &score);
}

// Modified to return the score
Move getBestMoveIterativeWithScore(Board &board, int depth, TranspositionTable *table,
                                   int alpha, int beta, int *score)
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
      *score = 0; // Stalemate score
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
            break;
         }
      }
   }

   Move bestMove = moves[0].move;
   int bestScore = -INF_BOUND;
   int originalAlpha = alpha;

   // Score and sort moves - put TT move first
   scoreMoves(moves, board, ttMove, 0);
   std::sort(moves.begin(), moves.end(), std::greater<ExtMove>());

   for (int i = 0; i < moves.size; i++)
   {
      Move move = moves[i].move;
      board.makeMove(move);
      int moveScore = -negamax(board, depth - 1, -beta, -alpha, table, 1);
      board.unmakeMove(move);

      if (moveScore > bestScore)
      {
         bestScore = moveScore;
         bestMove = move;

         if (moveScore > alpha)
         {
            alpha = moveScore;

            // Beta cutoff
            if (alpha >= beta)
               break;
         }
      }
   }

   // Store result in TT
   uint8_t flag = bestScore <= originalAlpha ? HFALPHA : (bestScore >= beta ? HFBETA : HFEXACT);
   table->store(board.hashKey, flag, bestMove, depth, bestScore, evaluate(board));

   // Return the score through the pointer
   *score = bestScore;
   return bestMove;
}

Move getBestMove(Board &board, int maxDepth, TranspositionTable *table)
{
   Move bestMove = NO_MOVE;
   int prevScore = 0; // Previous iteration score

   // Use full-width window for first three depths
   for (int depth = 1; depth <= maxDepth; depth++)
   {
      int score;
      Move currentBestMove;

      // For shallow depths or early game positions, use full window
      if (depth <= 3)
      {
         currentBestMove = getBestMoveIterativeWithScore(board, depth, table, -INF_BOUND, INF_BOUND, &score);
         if (currentBestMove != NO_MOVE)
         {
            bestMove = currentBestMove;
         }
         prevScore = score;
         continue;
      }

      // For deeper depths, use aspiration window
      // Fixed window size to start - smaller in opening positions
      int windowSize = 25; // Smaller initial window
      int alpha = prevScore - windowSize;
      int beta = prevScore + windowSize;

      // Try with increasingly wider windows until success
      int failCount = 0;
      while (true)
      {
         // Ensure our bounds are within limits
         alpha = std::max(-INF_BOUND, alpha);
         beta = std::min(static_cast<int>(INF_BOUND), beta);

         currentBestMove = getBestMoveIterativeWithScore(board, depth, table, alpha, beta, &score);

         // Store any valid move we find
         if (currentBestMove != NO_MOVE)
         {
            bestMove = currentBestMove;
         }

         // Search successful - move to next depth
         if (score > alpha && score < beta)
         {
            break;
         }

         failCount++;

         // After just 2 fails with opening positions, use full window
         if (failCount >= 2)
         {
            currentBestMove = getBestMoveIterativeWithScore(board, depth, table, -INF_BOUND, INF_BOUND, &score);
            if (currentBestMove != NO_MOVE)
            {
               bestMove = currentBestMove;
            }
            break;
         }

         // Adjust window based on failure type
         if (score <= alpha)
         {
            // Failed low - widen below
            alpha = alpha - windowSize * 3; // More aggressive widening
         }
         else
         { // score >= beta
            // Failed high - widen above
            beta = beta + windowSize * 3; // More aggressive widening
         }

         // Triple window size for next attempt - THIS IS THE PROBLEM
         windowSize += 25;  // Linear growth to prevent exponential growth
      }

      // Save this depth's score for next iteration
      prevScore = score;
   }

   return bestMove;
}