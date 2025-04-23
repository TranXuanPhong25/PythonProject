#include "search.hpp"
#include "tunable_params.hpp" // Make sure to include this
#include "score_move.hpp"

int lmrTable[MAXDEPTH][NSQUARES] = {{0}};
int lmpTable[2][8] = {{0}};

void initLateMoveTable()
{
   // std::cout << "TunableParams::LMR_BASE: " << TunableParams::LMR_BASE << std::endl;
   // std::cout << "TunableParams::LMR_DIVISION: " << TunableParams::LMR_DIVISION << std::endl;
   // std::cout << "TunableParams::NMP_BASE: " << TunableParams::NMP_BASE << std::endl;
   // std::cout << "TunableParams::NMP_DIVISION: " << TunableParams::NMP_DIVISION << std::endl;
   // std::cout << "TunableParams::NMP_MARGIN: " << TunableParams::NMP_MARGIN << std::endl;
   // std::cout << "TunableParams::LMP_DEPTH_THRESHOLD: " << TunableParams::LMP_DEPTH_THRESHOLD << std::endl;
   // std::cout << "TunableParams::FUTILITY_MARGIN: " << TunableParams::FUTILITY_MARGIN << std::endl;
   // std::cout << "TunableParams::FUTILITY_DEPTH: " << TunableParams::FUTILITY_DEPTH << std::endl;
   // std::cout << "TunableParams::FUTILITY_IMPROVING: " << TunableParams::FUTILITY_IMPROVING << std::endl;
   // std::cout << "TunableParams::QS_FUTILITY_MARGIN: " << TunableParams::QS_FUTILITY_MARGIN << std::endl;
   // std::cout << "TunableParams::SEE_QUIET_MARGIN_BASE: " << TunableParams::SEE_QUIET_MARGIN_BASE << std::endl;
   // std::cout << "TunableParams::SEE_NOISY_MARGIN_BASE: " << TunableParams::SEE_NOISY_MARGIN_BASE << std::endl;
   // std::cout << "TunableParams::ASPIRATION_DELTA: " << TunableParams::ASPIRATION_DELTA << std::endl;
   // std::cout << "TunableParams::HISTORY_PRUNING_THRESHOLD: " << TunableParams::HISTORY_PRUNING_THRESHOLD << std::endl;

   float base = TunableParams::LMR_BASE / 100.0f;
   float division = TunableParams::LMR_DIVISION / 100.0f;
   for (int depth = 1; depth < MAXDEPTH; depth++)
   {
      for (int played = 1; played < 64; played++)
      {
         lmrTable[depth][played] = base + log(depth) * log2(played) / division;
      }
   }

   for (int depth = 1; depth < 8; depth++)
   {
      lmpTable[0][depth] = 2.5 + 2 * depth * depth / 4.5;
      lmpTable[1][depth] = 4.0 + 4 * depth * depth / 4.5;
   }
}
bool gives_check(Board &board, Move move)
{
   board.makeMove(move);
   bool check = board.isSquareAttacked(~board.sideToMove, board.KingSQ(board.sideToMove));
   board.unmakeMove(move);
   return check;
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

int quiescence(int alpha, int beta, SearchThread &st, SearchStack *ss)
{
   st.nodes++;
   /* Checking for time every 2048 nodes */
   if (!(st.nodes & 2047))
   {
      st.check_time();
   }
   /* We return static evaluation if we exceed max depth */
   Board &board = st.board;
   if (ss->ply > MAXPLY - 1)
   {
      return evaluate(board);
   }
   if (board.isRepetition())
   {
      return 0;
   }
   int standPat = evaluate(board);
   if (standPat >= beta)
      return beta;

   // Futility pruning in quiescence search
   // If our standing pat plus a maximum gain doesn't reach alpha, we can skip the search
   const int futilityMargin = TunableParams::QS_FUTILITY_MARGIN; // Was hardcoded as 177
   if (standPat + futilityMargin < alpha)
      return alpha;

   if (standPat > alpha)
      alpha = standPat;
   /* Probe Tranpsosition Table */
   bool ttHit = false;
   bool isPVNode = (beta - alpha) > 1;
   TTEntry &ttEntry = table->probe_entry(board.hashKey, ttHit);

   const int ttScore = ttHit ? score_from_tt(ttEntry.get_score(), ss->ply) : 0;

   /* Return TT score if we found a TT entry*/
   if (!isPVNode && ttHit)
   {
      if ((ttEntry.flag == HFALPHA && ttScore <= alpha) || (ttEntry.flag == HFBETA && ttScore >= beta) ||
          (ttEntry.flag == HFEXACT))
         return ttScore;
   }

   int bestScore = standPat;
   int moveCount = 0;
   int score = -INF_BOUND;
   Move bestMove = NO_MOVE;

   Movelist captures;
   if (board.sideToMove == White)
      Movegen::legalmoves<White, CAPTURE>(board, captures);
   else
      Movegen::legalmoves<Black, CAPTURE>(board, captures);

   scoreMovesForQS(st.board, captures, ttEntry.move);

   for (int i = 0; i < captures.size; i++)
   {
      pickNextMove(i, captures);
      Move move = captures[i].move;
      ss->movedPice = st.board.pieceAtB(to(move));

      /* SEE pruning in quiescence search */
      /* If we do not SEE a good capture move, we can skip the move.*/
      if (captures[i].value < GoodCaptureScore && moveCount >= 1)
      {
         continue;
      }

      // Futility pruning for each move
      // If the piece we're capturing plus our current standing pat won't exceed alpha, skip
      if (moveCount > 0 && !board.isSquareAttacked(~board.sideToMove, board.KingSQ(board.sideToMove)))
      {
         // Get the captured piece value
         int capturedValue = PIECE_VALUES[board.pieceAtB(to(move))];

         // If capturing the best piece possible doesn't bring us above alpha, skip
         if (standPat + capturedValue + TunableParams::QS_FUTILITY_MARGIN < alpha) // Was hardcoded as 140
            continue;
      }

      ss->continuationHistory = &st.continuationHistory[ss->movedPice][to(move)];

      board.makeMove(move);
      table->prefetch_tt(board.hashKey);

      (ss + 1)->ply = ss->ply + 1;
      moveCount++;
      ss->move = move;

      score = -quiescence(-beta, -alpha, st, ss + 1);
      board.unmakeMove(move);
      /* Return 0 if time is up */
      if (st.info.stopped)
      {
         return 0;
      }
      if (score > bestScore)
      {
         bestMove = move;
         bestScore = score;

         if (score > alpha)
         {
            alpha = score;

            /* Fail soft */
            if (score >= beta)
               break;
         }
      }
   }
   /* We don't store exact flag in quiescence search, only Beta flag (Lower
    * Bound) and Alpha flag (Upper bound)*/
   int flag = bestScore >= beta ? HFBETA : HFALPHA;

   /* Store transposition table entry */
   table->store(board.hashKey, flag, bestMove, 0, score_to_tt(bestScore, ss->ply), standPat);

   /* Return bestscore achieved */
   return bestScore;
}

int negamax(int alpha, int beta, int depth, SearchThread &st, SearchStack *ss, bool cutnode)
{
   st.nodes++;
   // Step 1: run quiescence search if depth <=0
   if (depth <= 0)
      return quiescence(alpha, beta, st, ss);
   /* Checking for time every 2048 nodes */
   if ((st.nodes & 2047) == 0)
   {
      st.check_time();
   }

   // Step 2: Helper variables
   Board &board = st.board;

   bool isRoot = (ss->ply == 0);
   bool inCheck = board.isSquareAttacked(~board.sideToMove, board.KingSQ(board.sideToMove));
   bool isPVNode = (beta - alpha) > 1;

   bool improving = false;
   int eval = 0;

   // Step 3:
   if (!isRoot)
   {
      /* We return static evaluation if we exceed max depth.*/
      if (ss->ply > MAXPLY - 1)
      {
         return evaluate(board);
      }

      /* Repetition check*/
      if ((board.isRepetition()) && ss->ply)
      {
         return 0;
      }
      //  Mate distance pruning. Even if we mate at the next move our score
      // would be at best mate_in(ss->ply+1), but if alpha is already bigger because
      // a shorter mate was found upward in the tree then there is no need to search
      // because we will never beat the current alpha. Same logic but with reversed
      // signs applies also in the opposite condition of being mated instead of giving
      // mate. In this case return a fail-high score.
      alpha = std::max(alpha, mated_in(ss->ply));
      beta = std::min(beta, mate_in(ss->ply + 1));
      if (alpha >= beta)
      {
         return alpha;
      }
   }

   // Step 4: TT lookup
   bool ttHit = false;
   TTEntry &ttEntry = table->probe_entry(board.hashKey, ttHit);

   const int ttScore = ttHit ? score_from_tt(ttEntry.get_score(), ss->ply) : 0;

   if (!isPVNode && ttHit && ttEntry.depth >= depth)
   {
      if ((ttEntry.flag == HFALPHA && ttScore <= alpha) || (ttEntry.flag == HFBETA && ttScore >= beta) ||
          (ttEntry.flag == HFEXACT))
         return ttScore;
   }
   // Use eval frrom TT if we have a hit
   ss->staticEval = eval = ttHit ? ttEntry.get_eval() : evaluate(board);

   // If staticEval is better than 2 ply ago -> improve
   improving = !inCheck && ss->staticEval > (ss - 2)->staticEval;

   // Reset staticEval and improving if we are in check
   if (inCheck)
   {
      ss->staticEval = eval = 0;
      improving = false;
   }

   // Step 5: Reverse futility pruning || Null move pruning ||  ProbCut || Razoring
   if (!isPVNode && !inCheck && !isRoot)
   {
      /* We can use tt entry's score as score if we hit one.*/
      /* This score is from search so this is more accurate */
      if (ttHit)
      {
         eval = ttScore;
      }

      /* Reverse Futility Pruning (RFP)
       * If the eval is well above beta by a margin, then we assume the eval
       * will hold above beta.
       * This is a more aggressive pruning technique that
       * allows us to prune more nodes.
       * https://www.chessprogramming.org/Reverse_Futility_Pruning
       */
      if (depth <= TunableParams::RFP_DEPTH && eval >= beta &&
          eval - ((depth - improving) * TunableParams::RFP_MARGIN) -
                  (ss - 1)->staticScore / 400 >=
              beta)
      {
         return eval; // fail soft
      }

      /* Null move pruning
       * If we give our opponent a free move and still maintain beta, we prune
       * some nodes.
       */
      if (ss->staticEval >= (beta - TunableParams::NMP_MARGIN * improving + TunableParams::RFP_IMPROVING_BONUS) &&
          board.nonPawnMat(board.sideToMove) && (depth >= TunableParams::NMP_BASE) &&
          ((ss - 1)->move != NULL_MOVE) && (!ttHit || ttEntry.flag != HFALPHA || eval >= beta))
      {
         int R = TunableParams::NMP_BASE;
         // https://www.chessprogramming.org/Null_Move_Pruning_Test_Results
         if (popcount(board.occUs) > 2)
            R = TunableParams::NMP_BASE + 1;
         R = depth / TunableParams::NMP_DIVISION + std::min(3, (eval - beta) / 180);

         ss->continuationHistory = &st.continuationHistory[None][0];
         board.makeNullMove();
         ss->move = NULL_MOVE;

         (ss + 1)->ply = ss->ply + 1;

         int score = -negamax(-beta, -beta + 1, depth - R, st, ss + 1, !cutnode);

         board.unmakeNullMove();
         if (st.info.stopped)
         {
            return 0;
         }
         if (score >= beta)
         {

            /* We don't return mate scores because it can be a false mate.
             */
            if (score > ISMATE)
            {
               score = beta;
            }

            return score;
         }
      }

      /* ProbCut
       * Permits to exclude probably irrelevant subtrees from beeing searched deeply
       */
      int rbeta = std::min(beta + 100, ISMATE - MAXPLY - 1);
      if (depth >= 5 && abs(beta) < ISMATE && (!ttHit || eval >= rbeta || ttEntry.depth < depth - 3))
      {
         Movelist captures;
         if (board.sideToMove == White)
            Movegen::legalmoves<White, CAPTURE>(board, captures);
         else
            Movegen::legalmoves<Black, CAPTURE>(board, captures);
         scoreMoves(st, captures, ss, ttEntry.move);
         int score = 0;
         for (int i = 0; i < captures.size; i++)
         {
            pickNextMove(i, captures);
            Move move = captures[i].move;
            ss->movedPice = board.pieceAtB(from(move));

            if (captures[i].value < GoodCaptureScore)
            {
               continue;
            }

            if (move == ttEntry.move)
            {
               continue;
            }

            ss->continuationHistory = &st.continuationHistory[ss->movedPice][to(move)];
            board.makeMove(move);

            score = -quiescence(-rbeta, -rbeta + 1, st, ss);

            if (score >= rbeta)
            {
               score = -negamax(-rbeta, -rbeta + 1, depth - 4, st, ss, !cutnode);
            }

            board.unmakeMove(move);

            if (score >= rbeta)
            {
               table->store(board.hashKey, HFBETA, move, depth - 3, score, ss->staticEval);
               return score;
            }
         }
      }
      // Razoring
      if (eval - 63 + 182 * depth <= alpha)
      {
         return quiescence(alpha, beta, st, ss);
      }
   }
   // Decrease depth if we are in a cut node
   if (cutnode && depth >= 7 && ttEntry.move == NO_MOVE)
      depth--;

   int bestScore = -INF_BOUND;
   int moveCount = 0;
   int oldAlpha = alpha;
   int score = -INF_BOUND;
   Move bestMove = NO_MOVE;
   bool skipQuietMove = false;

   // Step 6: Generate moves
   Movelist moves;
   if (board.sideToMove == White)
      Movegen::legalmoves<White, ALL>(board, moves);
   else
      Movegen::legalmoves<Black, ALL>(board, moves);

   Movelist quietList;

   // Step 7: Scoring moves for ordering moves
   scoreMoves(st, moves, ss, ttEntry.move);

   // Step 9: Iterate through moves
   for (int i = 0; i < moves.size; i++)
   {
      pickNextMove(i, moves);

      Move move = moves[i].move;
      ss->movedPice = board.pieceAtB(from(move));

      bool isCapture = is_capture(board, move);
      bool isPromotion = promoted(move);
      bool isQuiet = !isCapture && !isPromotion;
      bool givesCheck = gives_check(board, move);
      // It is not necessarily the best move, but good enough to refute opponents previous move
      bool refutationMove = (ss->killers[0] == move || ss->killers[1] == move);

      // Get history score for this move to use in pruning decisions
      int hist = 0, counterHist = 0, followUpHist = 0;
      int history = getHistoryScores(hist, counterHist, followUpHist, st, ss, move);
      ss->staticScore = 2 * hist + counterHist + followUpHist - 4000;

      if (isQuiet && skipQuietMove)
      {
         continue;
      }

      /* Step 10: Quiet move pruning
       * We can prune quiet moves that are not captures or promotions
       * LMP + Continuation History pruning + Futility Pruning + SEE pruning
       */
      if (!isRoot && bestScore > -ISMATE && board.nonPawnMat(board.sideToMove))
      {

         // Get precalculated lmr depth from lmrTable
         int lmrDepth = lmrTable[std::min(depth, MAXDEPTH)][std::min(moveCount, MAXDEPTH)];

         // Pruning for quiets
         if (isQuiet && !givesCheck)
         {
            /* Late Move Pruning/Movecount pruning
             * A further variation of Extended Futility Pruning
             * combining the ideas of Fruit's History Leaf Pruning and LMR implemented in Stockfish.
             * If we have searched many moves, we can skip the rest.
             * https://www.chessprogramming.org/Futility_Pruning#MoveCountBasedPruning
             */
            if (!inCheck && !isPVNode && depth <= TunableParams::LMP_DEPTH_THRESHOLD && quietList.size >= lmpTable[improving][depth])
            {
               skipQuietMove = true;
               continue;
            }

            // Continuation History pruning
            if (lmrDepth < 4 && history < -TunableParams::HISTORY_PRUNING_THRESHOLD * depth && (ss - 1)->staticScore > 0 && counterHist < 0)
            {
               skipQuietMove = true;
               continue;
            }

            /* Futility Pruning
             * If the eval is well above beta by a margin, then we assume the eval
             * will hold above beta.
             * https://www.chessprogramming.org/Futility_Pruning
             */
            if (lmrDepth <= TunableParams::FUTILITY_DEPTH && !inCheck &&
                ss->staticEval + TunableParams::FUTILITY_MARGIN + TunableParams::FUTILITY_IMPROVING * depth <= alpha)
            {
               skipQuietMove = true;
            }

            // SEE pruning for quiets
            if (depth <= TunableParams::FUTILITY_DEPTH && !see(board, move, TunableParams::SEE_QUIET_MARGIN_BASE * depth))
            {
               continue;
            }
         }
         else
            // SEE pruning for noisy
            if (!see(board, move, TunableParams::SEE_NOISY_MARGIN_BASE * depth * depth))
            {
               continue;
            }
      }

      // TODO: try to group ss update
      ss->continuationHistory = &st.continuationHistory[ss->movedPice][to(move)];

      // Step 11: Make the move
      board.makeMove(move);
      table->prefetch_tt(board.hashKey);

      ss->move = move;
      (ss + 1)->ply = ss->ply + 1;

      moveCount++;

      if (isRoot && depth == 1 && moveCount == 1)
      {
         // Assume the first move is the best move
         st.bestMove = move;
      }

      if (isQuiet)
      {
         quietList.Add(move);
      }
      bool doFullSearch = !isPVNode || moveCount > 1;

      /* Step 12: LMR and PVS
       * Late move reduction
       * Later moves will be searched in a reduced depth.
       * If they beat alpha, It will be researched in a reduced window but
       * full depth.
       * https://www.chessprogramming.org/Late_Move_Reductions
       */
      if (!inCheck && depth >= 3 && moveCount > (2 + 2 * isPVNode))
      {
         int reduction = lmrTable[std::min(depth, 63)][std::min(63, moveCount)];

         reduction += !improving;                                /* Increase reduction if we're not improving. */
         reduction += !isPVNode;                                 /* Increase for non pv nodes */
         // reduction += isQuiet && !see(board, move, -50 * depth); /* Increase for quiet moves that lose material */
         reduction += isQuiet && !givesCheck;                    /* Increase for quiet moves that don't give check */
         // reduction += (isQuiet&&cutnode)*2;
         // Reduce two plies if it's a counter or killer
         reduction -= refutationMove * 2;

         // Reduce or Increase according to history score
         reduction -= history / 4000;

         // Decrease/increase reduction for moves with a good/bad history (~30 Elo)
         reduction -= ss->staticScore / 16000;
         /* Adjust the reduction so we don't drop into Qsearch or cause an
          * extension*/
         reduction = std::min(depth - 1, std::max(1, reduction));

         score = -negamax(-alpha - 1, -alpha, depth - reduction, st, ss + 1, true);

         /* We do a full depth research if our score beats alpha that maybe promising. */
         doFullSearch = score > alpha && reduction != 1;
      }

      /* Full depth search on a zero window. */
      if (doFullSearch)
      {
         score = -negamax(-alpha - 1, -alpha, depth - 1, st, ss + 1, !cutnode);
      }

      /* Principal Variation Search (PVS)
       * an enhancement to Alpha-Beta, based on null- or zero window searches of none PV-nodes,
       * to prove a move is worse or not than an already safe score from the principal variation.
       * If we are in a PV node, we do a full window search on the first move or might improve alpha.
       * https://www.chessprogramming.org/Principal_Variation_Search
       */
      if (isPVNode && (moveCount == 1 || (score > alpha && (isRoot || score < beta))))
      {
         score = -negamax(-beta, -alpha, depth - 1, st, ss + 1, false);
      }

      // Step 13: Unmake the move
      board.unmakeMove(move);
      if (st.info.stopped && !isRoot)
      {
         return 0;
      }
      // Step 14: Alpha-beta pruning
      if (score > bestScore)
      {
         bestScore = score;

         if (score > alpha)
         {
            // Record PV
            alpha = score;
            bestMove = move;

            if (score >= beta)
            {
               if (isQuiet)
               {
                  // Update killers
                  ss->killers[1] = ss->killers[0];
                  ss->killers[0] = move;

                  // Update histories
                  updateHistories(st, ss, bestMove, quietList, depth);
               }
               break;
            }
            // clang-format on
         }
      }
      if (st.info.stopped && isRoot && st.bestMove != NO_MOVE)
      {
         break;
      }
   }
   // Terminal node if no moves exist
   if (moveCount == 0)
      bestScore = inCheck ? mated_in(ss->ply) : 0;

   int flag = bestScore >= beta ? HFBETA : (alpha != oldAlpha) ? HFEXACT
                                                               : HFALPHA;

   table->store(board.hashKey, flag, bestMove, depth, score_to_tt(bestScore, ss->ply), ss->staticEval);

   if (alpha != oldAlpha)
   {
      st.bestMove = bestMove;
   }

   return bestScore;
}

// Template implementation of iterativeDeepening with printInfo parameter
template <bool printInfo>
void iterativeDeepening(SearchThread &st, const int &maxDepth)
{
   SearchInfo &info = st.info;
   st.clear();
   st.initialize();
   table->nextAge();

   int score = 0;

   auto startime = st.start_time();
   Move bestMove = NO_MOVE;

   for (int depth = 1; depth <= maxDepth; depth++)
   {
      score = aspirationWindow(score, depth, st, bestMove);
      if (st.info.stopped || st.stop_early())
      {
         break;
      }
      bestMove = st.bestMove;
      if (info.timeset)
      {
         st.tm.update_tm(bestMove);
      }
      if constexpr (printInfo)
      {
        
            auto time_elapsed = misc::tick() - startime;


            std::cout<<"score ";
            if (score >= ISMATED && score <= IS_MATED_IN_MAX_PLY)
            {
               std::cout << "mate " << ((ISMATED - score) / 2);
            }
            else if (score >= IS_MATE_IN_MAX_PLY && score <= ISMATE)
            {
               std::cout << "mate " << ((ISMATE - score) / 2);
            }else{
               std::cout << score;
            }
            std::cout << " depth " << depth;
            std::cout << " nodes " << st.nodes;
            std::cout << " nps " << static_cast<uint64_t>(st.nodes  / (time_elapsed/1000));
            std::cout << " time " << static_cast<uint64_t>(time_elapsed);
            std::cout << std::endl;
         
        
      }
   }

   if (printInfo)
   {
       std::cout << "bestmove " << convertMoveToUci(bestMove) << std::endl;
   }
}

// Explicit instantiations for both versions
template void iterativeDeepening<true>(SearchThread &st, const int &maxDepth);
template void iterativeDeepening<false>(SearchThread &st, const int &maxDepth);

// Original function - now just forwards to the templated version with printInfo=true
void iterativeDeepening(SearchThread &st, const int &maxDepth)
{
   iterativeDeepening<true>(st, maxDepth);
}

int aspirationWindow(int prevEval, int depth, SearchThread &st, Move &bestmove)
{
   int score = 0;

   SearchStack stack[MAXPLY + 10], *ss = stack + 7;

   int delta = TunableParams::ASPIRATION_DELTA; // Was hardcoded as 12

   int alpha = -INF_BOUND;
   int beta = INF_BOUND;

   int initial_depth = depth;

   if (depth > 3)
   {
      alpha = std::max<int>(-INF_BOUND, prevEval - delta);
      beta = std::min<int>(INF_BOUND, prevEval + delta);
   }

   while (true)
   {

      score = negamax(alpha, beta, depth, st, ss, false);
      if (st.stop_early())
      {
         break;
      }
      if (score <= alpha)
      {
         beta = (alpha + beta) / 2;
         alpha = std::max<int>(-INF_BOUND, score - delta);

         depth = initial_depth;
      }
      else if (score >= beta)
      {
         beta = std::min<int>(score + delta, INF_BOUND);
         if (abs(score) <= ISMATE / 2 && depth > 1)
         {
            depth--;
         }

         // Idea from StockDory - Rice
         bestmove = st.bestMove;
      }
      else
      {
         break;
      }
      delta += delta / 2;
   }

   return score;
}