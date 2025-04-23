#include "search.hpp"
#include "tunable_params.hpp" // Make sure to include this
#include "score_move.hpp"

int lmrTable[MAXDEPTH][NSQUARES] = {{0}};
int lmpTable[2][8] = {{0}};

void initLateMoveTable()
{
   std::cout<<"TunableParams::LMR_BASE: " << TunableParams::LMR_BASE << std::endl;
   std::cout<<"TunableParams::LMR_DIVISION: " << TunableParams::LMR_DIVISION << std::endl;
   std::cout<<"TunableParams::NMP_BASE: " << TunableParams::NMP_BASE << std::endl;
   std::cout<<"TunableParams::NMP_DIVISION: " << TunableParams::NMP_DIVISION << std::endl;
   std::cout<<"TunableParams::NMP_MARGIN: " << TunableParams::NMP_MARGIN << std::endl;
   std::cout<<"TunableParams::LMP_DEPTH_THRESHOLD: " << TunableParams::LMP_DEPTH_THRESHOLD << std::endl;
   std::cout<<"TunableParams::FUTILITY_MARGIN: " << TunableParams::FUTILITY_MARGIN << std::endl;
   std::cout<<"TunableParams::FUTILITY_DEPTH: " << TunableParams::FUTILITY_DEPTH << std::endl;
   std::cout<<"TunableParams::FUTILITY_IMPROVING: " << TunableParams::FUTILITY_IMPROVING << std::endl;
   std::cout<<"TunableParams::QS_FUTILITY_MARGIN: " << TunableParams::QS_FUTILITY_MARGIN << std::endl;
   std::cout<<"TunableParams::SEE_QUIET_MARGIN_BASE: " << TunableParams::SEE_QUIET_MARGIN_BASE << std::endl;
   std::cout<<"TunableParams::SEE_NOISY_MARGIN_BASE: " << TunableParams::SEE_NOISY_MARGIN_BASE << std::endl;
   std::cout<<"TunableParams::ASPIRATION_DELTA: " << TunableParams::ASPIRATION_DELTA << std::endl;
   std::cout<<"TunableParams::HISTORY_PRUNING_THRESHOLD: " << TunableParams::HISTORY_PRUNING_THRESHOLD << std::endl;

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
#include <chrono> // Add for timing measurements

SearchStats searchStats;

// Profiling structure to track time spent in different parts of search
struct ProfilerData
{
   // Timers for different search components
   std::chrono::nanoseconds ttLookupTime{0};
   std::chrono::nanoseconds moveGenTime{0};
   std::chrono::nanoseconds evaluationTime{0};
   std::chrono::nanoseconds seeTime{0};
   std::chrono::nanoseconds moveMakingTime{0};
   std::chrono::nanoseconds moveOrderingTime{0};
   std::chrono::nanoseconds pruningTime{0};
   std::chrono::nanoseconds quiescenceTime{0};

   // Counters
   size_t ttLookups{0};
   size_t ttHits{0};
   size_t moveGens{0};
   size_t evaluations{0};
   size_t seeChecks{0};
   size_t movesGenerated{0};
   size_t movesMade{0};
   size_t pruningAttempts{0};
   size_t pruningSuccesses{0};

   void reset()
   {
      ttLookupTime = std::chrono::nanoseconds{0};
      moveGenTime = std::chrono::nanoseconds{0};
      evaluationTime = std::chrono::nanoseconds{0};
      seeTime = std::chrono::nanoseconds{0};
      moveMakingTime = std::chrono::nanoseconds{0};
      moveOrderingTime = std::chrono::nanoseconds{0};
      pruningTime = std::chrono::nanoseconds{0};
      quiescenceTime = std::chrono::nanoseconds{0};

      ttLookups = 0;
      ttHits = 0;
      moveGens = 0;
      evaluations = 0;
      seeChecks = 0;
      movesGenerated = 0;
      movesMade = 0;
      pruningAttempts = 0;
      pruningSuccesses = 0;
   }

   void printProfileReport() const
   {
      auto totalSearchTime = ttLookupTime + moveGenTime + evaluationTime +
                             seeTime + moveMakingTime + moveOrderingTime +
                             pruningTime + quiescenceTime;

      std::cout << "\n===== SEARCH PROFILING REPORT =====\n";
      std::cout << "Total profiled time: " << std::chrono::duration_cast<std::chrono::milliseconds>(totalSearchTime).count() << " ms\n\n";

      // Print timing information
      std::cout << "TIME BREAKDOWN:\n";
      std::cout << "TT Lookup:      " << std::chrono::duration_cast<std::chrono::milliseconds>(ttLookupTime).count()
                << " ms (" << (ttLookupTime.count() * 100.0 / totalSearchTime.count()) << "%)\n";
      std::cout << "Move Generation: " << std::chrono::duration_cast<std::chrono::milliseconds>(moveGenTime).count()
                << " ms (" << (moveGenTime.count() * 100.0 / totalSearchTime.count()) << "%)\n";
      std::cout << "Evaluation:      " << std::chrono::duration_cast<std::chrono::milliseconds>(evaluationTime).count()
                << " ms (" << (evaluationTime.count() * 100.0 / totalSearchTime.count()) << "%)\n";
      std::cout << "SEE Checks:      " << std::chrono::duration_cast<std::chrono::milliseconds>(seeTime).count()
                << " ms (" << (seeTime.count() * 100.0 / totalSearchTime.count()) << "%)\n";
      std::cout << "Move Making:     " << std::chrono::duration_cast<std::chrono::milliseconds>(moveMakingTime).count()
                << " ms (" << (moveMakingTime.count() * 100.0 / totalSearchTime.count()) << "%)\n";
      std::cout << "Move Ordering:   " << std::chrono::duration_cast<std::chrono::milliseconds>(moveOrderingTime).count()
                << " ms (" << (moveOrderingTime.count() * 100.0 / totalSearchTime.count()) << "%)\n";
      std::cout << "Pruning:         " << std::chrono::duration_cast<std::chrono::milliseconds>(pruningTime).count()
                << " ms (" << (pruningTime.count() * 100.0 / totalSearchTime.count()) << "%)\n";
      std::cout << "Quiescence:      " << std::chrono::duration_cast<std::chrono::milliseconds>(quiescenceTime).count()
                << " ms (" << (quiescenceTime.count() * 100.0 / totalSearchTime.count()) << "%)\n";

      // Print counter information
      std::cout << "\nCOUNTERS:\n";
      std::cout << "TT Lookups:      " << ttLookups << " (Hit rate: " << (ttHits * 100.0 / ttLookups) << "%)\n";
      std::cout << "Move Generations: " << moveGens << " (Avg moves: " << (movesGenerated * 1.0 / moveGens) << ")\n";
      std::cout << "Evaluations:      " << evaluations << "\n";
      std::cout << "SEE Checks:       " << seeChecks << "\n";
      std::cout << "Moves Made:       " << movesMade << "\n";
      std::cout << "Pruning Attempts: " << pruningAttempts << " (Success rate: "
                << (pruningSuccesses * 100.0 / pruningAttempts) << "%)\n";

      // Print efficiency metrics
      std::cout << "\nEFFICIENCY METRICS:\n";
      std::cout << "Time per evaluation: " << (evaluations ? evaluationTime.count() / evaluations : 0) << " ns\n";
      std::cout << "Time per move made:  " << (movesMade ? moveMakingTime.count() / movesMade : 0) << " ns\n";
      std::cout << "Time per TT lookup:  " << (ttLookups ? ttLookupTime.count() / ttLookups : 0) << " ns\n";
      std::cout << "Time per SEE check:  " << (seeChecks ? seeTime.count() / seeChecks : 0) << " ns\n";
      std::cout << "==============================\n";
   }
};

// Global profiling data
ProfilerData profiler;

// Global move history stack
MoveHistoryEntry moveHistory[MAX_MOVE_HISTORY];
int moveHistoryCount = 0;

// Add a move to the move history stack
void addMoveToHistory(Move move, U64 hashKey)
{
   if (moveHistoryCount < MAX_MOVE_HISTORY)
   {
      moveHistory[moveHistoryCount++] = MoveHistoryEntry(move, hashKey);
   }
   else
   {
      // If history is full, shift everything down and add at the end
      for (int i = 0; i < MAX_MOVE_HISTORY - 1; i++)
      {
         moveHistory[i] = moveHistory[i + 1];
      }
      moveHistory[MAX_MOVE_HISTORY - 1] = MoveHistoryEntry(move, hashKey);
   }
}

// Get the last move from history
Move getLastMove()
{
   if (moveHistoryCount > 0)
   {
      return moveHistory[moveHistoryCount - 1].move;
   }
   return NO_MOVE;
}

// Clear the move history
void clearMoveHistory()
{
   moveHistoryCount = 0;
}

// Update continuation history for a move that was effective after another move
void updateContinuationHistory(Board &board, Move prevMove, Move currMove, int depth, bool isCutoff)
{
   if (prevMove == NO_MOVE || currMove == NO_MOVE)
      return;

   // Get pieces and squares for both moves
   Piece prevPiece = board.pieceAtB(to(prevMove));
   int prevTo = to(prevMove);

   Piece currPiece = board.pieceAtB(from(currMove));
   int currTo = to(currMove);

   // Skip if any information is invalid
   if (prevPiece == None || currPiece == None)
      return;

   // Update continuation history with bonus or penalty
   int bonus = std::min(32 * depth * depth, 1024);
   if (!isCutoff)
      bonus = -bonus;

   // Apply the same decay/bonus formula used in regular history
   continuationHistory[prevPiece][prevTo][currPiece][currTo] =
       continuationHistory[prevPiece][prevTo][currPiece][currTo] * 32 / 33 + bonus;

   // Clamp values to prevent overflow
   if (continuationHistory[prevPiece][prevTo][currPiece][currTo] > 20000)
      continuationHistory[prevPiece][prevTo][currPiece][currTo] = 20000;
   if (continuationHistory[prevPiece][prevTo][currPiece][currTo] < -20000)
      continuationHistory[prevPiece][prevTo][currPiece][currTo] = -20000;
}

int getPieceCounts(const Board &board, Color color)
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
int deltaPruningMargin(Board &board)
{
   // Base margin
   int margin = 200;

   // Adjust for game phase
   float phase = getGamePhase(board);
   margin += int(50 * (1.0f - phase)); // Larger margin in endgames

   // Adjust for king safety - larger margin when king is less safe
   int kingDanger = evaluateKingSafety(board, board.sideToMove);
   if (kingDanger > 0)
      margin += std::min(kingDanger / 5, 100);

   return margin;
}

// Timer utility class for profiling sections
class ScopedTimer
{
private:
   std::chrono::time_point<std::chrono::high_resolution_clock> start;
   std::chrono::nanoseconds &target;

public:
   explicit ScopedTimer(std::chrono::nanoseconds &targetTime)
       : start(std::chrono::high_resolution_clock::now()), target(targetTime) {}

   ~ScopedTimer()
   {
      auto end = std::chrono::high_resolution_clock::now();
      target += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
   }
};

// Add a macro to make timing code sections easier
#define PROFILE_SCOPE(name) ScopedTimer timer##__LINE__(profiler.name##Time)

// Global search stack
Stack ss[MAX_SEARCH_DEPTH + 10]; // +10 for safety with quiescence search

// Initialize the search stack before starting a new search
void initializeSearchStack()
{
   for (int i = 0; i < MAX_SEARCH_DEPTH + 10; i++)
   {
      ss[i].pv = nullptr;
      ss[i].ply = i;
      ss[i].currentMove = NO_MOVE;
      ss[i].excludedMove = NO_MOVE;
      ss[i].staticEval = 0;
      ss[i].statScore = 0;
      ss[i].moveCount = 0;
      ss[i].inCheck = false;
      ss[i].ttPv = false;
      ss[i].ttHit = false;
      ss[i].cutoffCnt = 0;
      ss[i].reduction = 0;
      ss[i].isTTMove = false;
   }
}

int quiescence(int alpha, int beta, SearchThread &st, SearchStack *ss)
{
   searchStats.qnodes++; // Count each quiescence node

   if (ply >= MAX_PLY - 1)
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

   bool ttHit = false;
   bool is_pvnode = (beta - alpha) > 1;

   TTEntry &tte = table->probe_entry(board.hashKey, ttHit);
   const int tt_score = ttHit ? score_from_tt(tte.get_score(), ply) : 0;
   if (!is_pvnode && ttHit)
   {
      if ((ttEntry.flag == HFALPHA && ttScore <= alpha) || (ttEntry.flag == HFBETA && ttScore >= beta) ||
          (ttEntry.flag == HFEXACT))
         return ttScore;
   }

   int bestScore = standPat;
   int moveCount = 0;
   int score = -INF_BOUND;
   Move bestMove = NO_MOVE;

   // Time the move generation
   Movelist captures;
   {
      PROFILE_SCOPE(moveGen);
      profiler.moveGens++;

      if (board.sideToMove == White)
         Movegen::legalmoves<White, CAPTURE>(board, captures);
      else
         Movegen::legalmoves<Black, CAPTURE>(board, captures);

   ScoreMovesForQS(board, captures, tte.move);
   std::sort(captures.begin(), captures.end(), std::greater<ExtMove>());

   for (int i = 0; i < captures.size; i++)
   {
      pickNextMove(i, captures);
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
   int flag = bestValue >= beta ? HFBETA : HFALPHA;

   table->store(board.hashKey, flag, bestmove, 0, score_to_tt(bestValue, ply), standPat);

   /* Return bestscore achieved */
   return bestScore;
}
// Minimax search with alpha-beta pruning
int negamax(Board &board, int depth, int alpha, int beta, TranspositionTable *table, int ply)
{
   st.nodes++;
   // Step 1: run quiescence search if depth <=0
   if (depth <= 0)
      return quiescence(alpha, beta, st, ss);
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
   Move ttMove = NO_MOVE;

   // TT cutoff logic remains the same
   if (!is_pvnode && ttHit && entry.depth >= depth)
   {
      searchStats.ttCutoffs++;

      if (entry.flag == HFEXACT)
         return entry.score;
      if (entry.flag == HFALPHA && entry.score <= alpha)
         return alpha;
      if (entry.flag == HFBETA && entry.score >= beta)
         return beta;

      searchStats.ttCutoffs--;
   }

   if (ttHit)
   {
      ttMove = entry.move;
   }

   // Get the lastMove from our custom move history
   Move lastMove = getLastMove();

   // Checkmate detection and pruning logic remain the same
   bool inCheck = board.isSquareAttacked(~board.sideToMove, board.KingSQ(board.sideToMove));
   bool isRoot = (ply == 0);
   int staticEval;
   int eval = 0;

   if (ttHit)
   {
      ttMove = entry.move;
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
   //Null move prunning
   if (!inCheck && !is_pvnode && !isRoot)
   {
      // More aggressive material condition - require at least one "major" piece
      bool hasMajorPiece = board.pieces(KNIGHT, board.sideToMove) |
                           board.pieces(BISHOP, board.sideToMove) |
                           board.pieces(ROOK, board.sideToMove) |
                           board.pieces(QUEEN, board.sideToMove);

      if (hasMajorPiece && depth >= 3 && (!ttHit || entry.flag != HFALPHA || eval >= beta))
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
          (ss - 1)->staticScore / 400 >= beta)
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
         if(popcount(board.occUs) > 2) R = TunableParams::NMP_BASE + 1;
         R = depth / TunableParams::NMP_DIVISION + std::min(3, (eval - beta) / 180);
         
         ss->continuationHistory = &st.continuationHistory[None][0];
         board.makeNullMove();
         ss->move = NULL_MOVE;

         (ss + 1)->ply = ss->ply + 1;

         int score = -negamax(-beta, -beta + 1, depth - R, st, ss + 1, !cutnode);

         board.unmakeNullMove();

               if (nullScore >= beta)
               {
                  // Verification search with adaptive strategy
                  searchStats.nullCutoffs++;

                  // 1. Skip verification for very deep positions with high margins
                  if (depth >= 12 && nullScore >= beta + 170)
                     return beta;

                  // 2. Multi-tiered verification approach
                  bool needsVerification = false;

                  // Always verify critical endgame positions
                  if (materialCount <= 14)
                     needsVerification = true;

                  // Verify deeper searches
                  if (depth >= 8)
                     needsVerification = true;

                  // Verify positions where score is just barely above beta
                  if (nullScore < beta + 50)
                     needsVerification = true;

                  if (needsVerification)
                  {
                     // Adaptive verification depth
                     int verificationDepth;

                     // Use deeper verification for endgames (more zugzwang risk)
                     if (materialCount <= 14)
                        verificationDepth = std::max(depth / 2, std::min(depth - 2, 6));
                     else
                        verificationDepth = std::max(depth / 3, std::min(depth - 3, 5));

                     // Use "safest" window for verification
                     int verificationScore = negamax(board, verificationDepth, beta - 1, beta, table, ply);

                     if (verificationScore >= beta)
                        return beta;
                  }
                  else
                  {
                     // No verification needed
                     return nullScore;
                  }
               }
            }
         }
      }
   }

   
   // Stockfish's probabilistic cutoff technique (ProbCut)
   if (!is_pvnode && depth >= 5 && !inCheck && std::abs(beta) < IS_MATE_IN_MAX_PLY)
   {
      int margin;
      if (std::abs(staticEval) < 150)
      {
         margin = 80; // Smaller margin in balanced positions
      }
      else
      {
         margin = 100 + std::min(30, std::abs(staticEval) / 10);
      }

      int rbeta = std::min(beta + margin, static_cast<int>(INF_BOUND));
      int rdepth = depth - 4;

      // Try captures that might exceed beta with high probability
      Movelist captureMoves;
      if (board.sideToMove == White)
         Movegen::legalmoves<White, CAPTURE>(board, captureMoves);
      else
         Movegen::legalmoves<Black, CAPTURE>(board, captureMoves);

      for (int i = 0; i < captureMoves.size; i++)
      {
         Move move = captureMoves[i].move;

         // Skip unlikely captures with SEE
         if (!see(board, move, 200))
            continue;

         board.makeMove(move);
         int score = -negamax(board, rdepth, -rbeta, -rbeta + 1, table, ply + 1);
         board.unmakeMove(move);

         if (score >= rbeta)
            return score;
      }
   }

   // Add futility pruning
   bool futilityPruning = false;
   if (!is_pvnode && !inCheck && depth <= 3)
   {
      int futilityMargin = 70 * depth;
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

   scoreMoves(moves, board, ttMove, ply);
   std::sort(moves.begin(), moves.end(), std::greater<ExtMove>());

   // Search variables
   int bestScore = -INF_BOUND;
   Move bestMove = NO_MOVE;
   uint8_t nodeFlag = HFALPHA;
   int movesSearched = 0;

   // History-based Late Move Pruning thresholds
   // Number of moves to consider before starting to prune based on move count
   // Higher depth = more moves searched before pruning
   const int LateMovePruningCounts[9] = {0, 8, 12, 22, 36, 56, 96, 160, 256};

   // History score thresholds for pruning - negative values indicate poor moves
   // More negative = more aggressive pruning at higher depths
   const int HistoryPruningThreshold[9] = {0, 0, 0, -2000, -4000, -6000, -8000, -10000, -12000};

   // Step 9: Iterate through moves
   for (int i = 0; i < moves.size; i++)
   {
      pickNextMove(i, moves);

      Move move = moves[i].move;

      // Early pruning with SEE
      if (i > 0 && !see(board, move, -65 * depth))
         continue;

      bool isCapture = is_capture(board, move);
      bool isPromotion = promoted(move);
      bool isQuiet = !isCapture && !isPromotion;
      bool givesCheck = gives_check(board, move);
      // It is not necessarily the best move, but good enough to refute opponents previous move
      bool refutationMove = (ss->killers[0] == move || ss->killers[1] == move);

      // Get history score for this move to use in pruning decisions
      int side = board.sideToMove == White ? 0 : 1;
      int history_score = historyTable[side][from(move)][to(move)];

      // Get continuation history score if we have a previous move
      int continuation_score = 0;
      if (lastMove != NO_MOVE) {
          Piece lastPiece = board.pieceAtB(to(lastMove));
          int lastTo = to(lastMove);
          Piece currPiece = board.pieceAtB(from(move));
          int currTo = to(move);
          
          if (lastPiece != None && currPiece != None) {
              continuation_score = continuationHistory[lastPiece][lastTo][currPiece][currTo];
          }
      }

      // Combine scores for pruning decision
      int combined_history = history_score;
      if (lastMove != NO_MOVE) {
          // Weight the scores (can be adjusted based on testing)
          combined_history = (history_score * 2 + continuation_score) / 3;
      }

      if (futilityPruning && i > 0 && !isCapture && !isPromotion)
         continue; // Skip this quiet move

      // History-based Late Move Pruning: skip late quiet moves with bad history
      if (!isRoot && !inCheck && !isCapture && !isPromotion && depth <= 8) 
      {
          // Standard count-based LMP
          if (i >= moveCountThreshold)
              continue;
              
          // History-based pruning: skip quiet moves with very bad history scores
          if (i > 3 && combined_history <= HistoryPruningThreshold[depth])
              continue;
              
          // Continuation-history based pruning (more aggressive)
          if (lastMove != NO_MOVE && i > 2 && continuation_score <= ContinuationPruningThreshold[depth])
              continue;
      }

      board.makeMove(move);

      // Add the move to our custom move history
      addMoveToHistory(move, board.hashKey);

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
         int R = 1;

         if (i > 4) // More reduction for later moves
            R++;
            
         if (depth > 5) // More reduction for deeper searches
            R++;

         // History-based LMR adjustments
         // Reduce less for moves with good history
         if (history_score > 6000)
            R = std::max(0, R - 1);  // Good history, reduce less
         else if (history_score < -4000)
            R++;  // Bad history, reduce more
            
         // Continuation-history based LMR adjustments
         if (continuation_score > 6000)
            R = std::max(0, R - 1);  // Good continuation history, reduce less
         else if (continuation_score < -4000)
            R++;  // Bad continuation history, reduce more

         // Don't reduce too much for killer moves (likely good tactical moves)
         if (move == killerMoves[ply][0] || move == killerMoves[ply][1])
            R = std::max(0, R - 1);

         // Do reduced search with null window
         score = -negamax(board, depth - 1 - R, -alpha - 1, -alpha, table, ply + 1);

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
      if (isPVNode && (moveCount == 1 || (score > alpha && (isRoot||score < beta))))
      {
         score = -negamax(-beta, -alpha, depth - 1, st, ss + 1, false);
      }

      // Remove the last move from our history to backtrack
      if (moveHistoryCount > 0)
          moveHistoryCount--;
          
      board.unmakeMove(move);

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
                  addKillerMove(move, ply);
                  updateHistory(board, move, depth, true);

                  // Now also update countermove and continuation history
                  if (lastMove != NO_MOVE) {
                      updateCounterMove(board, lastMove, move);
                      updateContinuationHistory(board, lastMove, move, depth, true);
                  }
               }

               nodeFlag = HFBETA;
               break;
            }
            // clang-format on
         }
      }

      // For non-cutoff quiet moves, update history negatively
      if (!isCapture && !isPromotion)
      {
         updateHistory(board, move, depth, false);

         // Also update continuation history negatively
         if (lastMove != NO_MOVE) {
             updateContinuationHistory(board, lastMove, move, depth, false);
         }
      }
   }

   table->store(posKey, nodeFlag, bestMove, depth, bestScore, evaluate(board));

   return bestScore;
}

// Template implementation of iterativeDeepening with printInfo parameter
template<bool printInfo>
void iterativeDeepening(SearchThread &st, const int &maxDepth)
{
   st.clear();
   table->nextAge();

   int score = 0;

   auto startime = std::chrono::high_resolution_clock::now();
   Move bestMove = NO_MOVE;

   for (int depth = 1; depth <= maxDepth; depth++)
   {
      bool moveIsLegal = false;
      for (int i = 0; i < moves.size; i++)
      {
         if (moves[i].move == ttMove)
         {
            moveIsLegal = true;
            break;
         }
      }

      // If move is not legal, reset it
      if (!moveIsLegal)
      {
         ttMove = NO_MOVE;
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

      // Add move to our custom history
      addMoveToHistory(move, board.hashKey);

      int moveScore = -negamax(board, depth - 1, -beta, -alpha, table, 1);

      // Remove the move from history when backtracking
      if (moveHistoryCount > 0)
          moveHistoryCount--;
          
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
   searchStats.clear();
   clearMoveHistory(); // Clear the move history at the start of a new search

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
         windowSize += 25; // Linear growth to prevent exponential growth
      }

      // Save this depth's score for next iteration
      prevScore = score;
      std::cout << "Depth " << depth << ", Score: " << prevScore
                << ", Nodes: " << searchStats.totalNodes()
                << ", NPS: " << searchStats.nodesPerSecond() << std::endl;
   }
   std::cout << "\nSearch completed to depth " << maxDepth << std::endl;
   searchStats.printStats();
   return bestMove;
}