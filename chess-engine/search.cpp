#include "search.hpp"

SearchStats searchStats;

// Global move history stack
MoveHistoryEntry moveHistory[MAX_MOVE_HISTORY];
int moveHistoryCount = 0;

// Add a move to the move history stack
void addMoveToHistory(Move move, U64 hashKey) {
    if (moveHistoryCount < MAX_MOVE_HISTORY) {
        moveHistory[moveHistoryCount++] = MoveHistoryEntry(move, hashKey);
    }
    else {
        // If history is full, shift everything down and add at the end
        for (int i = 0; i < MAX_MOVE_HISTORY - 1; i++) {
            moveHistory[i] = moveHistory[i + 1];
        }
        moveHistory[MAX_MOVE_HISTORY - 1] = MoveHistoryEntry(move, hashKey);
    }
}

// Get the last move from history
Move getLastMove() {
    if (moveHistoryCount > 0) {
        return moveHistory[moveHistoryCount - 1].move;
    }
    return NO_MOVE;
}

// Clear the move history
void clearMoveHistory() {
    moveHistoryCount = 0;
}

// Update continuation history for a move that was effective after another move
void updateContinuationHistory(Board &board, Move prevMove, Move currMove, int depth, bool isCutoff) {
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
int deltaPruningMargin(Board &board) {
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
int quiescence(Board &board, int alpha, int beta, TranspositionTable *table, int ply)
{
   searchStats.qnodes++; // Count each quiescence node

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
   bool inCheck = board.isSquareAttacked(~board.sideToMove, board.KingSQ(board.sideToMove));
   TTEntry &tte = table->probe_entry(board.hashKey, ttHit);
   const int tt_score = ttHit ? score_from_tt(tte.get_score(), ply) : 0;
   if (!is_pvnode && ttHit)
   {
      if ((tte.flag == HFALPHA && tt_score <= alpha) || (tte.flag == HFBETA && tt_score >= beta) ||
          (tte.flag == HFEXACT))
      {
         searchStats.ttCutoffs++;
         return tt_score;
      }
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
   
   // Delta pruning margin - can be tuned
   const int DELTA_MARGIN = deltaPruningMargin(board);

   for (int i = 0; i < captures.size; i++)
   {
      Move move = captures[i].move;

      // Apply delta pruning - skip captures that can't improve alpha
      if (!is_pvnode && !inCheck)
      {
         // Get the captured piece
         Piece capturedPiece = board.pieceAtB(to(move));
         
         // Calculate maximum possible material gain
         int captureValue = 0;
         
         // Convert piece to its value
         switch (capturedPiece % 6) {
             case QUEEN:  captureValue = QUEEN_VALUE;  break;
             case ROOK:   captureValue = ROOK_VALUE;   break;
             case BISHOP: captureValue = BISHOP_VALUE; break;
             case KNIGHT: captureValue = KNIGHT_VALUE; break;
             case PAWN:   captureValue = PAWN_VALUE;   break;
             default:     captureValue = 0;            break;
         }
         
         // Add promotion value if move is a promotion
         if (promoted(move)) {
             // Assume promotion to queen (best case)
             captureValue += (QUEEN_VALUE - PAWN_VALUE);
         }
         
         // Skip this capture if even with the maximum possible gain it can't improve alpha
         if (standPat + captureValue + DELTA_MARGIN <= alpha) {
             continue;
         }
      }

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
   searchStats.nodes++; // Count each regular node

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
         if (ply < MAX_PLY - 10)
         {
            // Adaptive reduction based on multiple factors
            // 1. Base reduction increases with depth
            // 2. Higher reduction when eval is far above beta
            // 3. Lower reduction in endgames
            int materialCount = popcount(board.occAll);
            int evalMargin = std::max(0, eval - beta);

            // Start with base reduction that scales with depth
            int R = 3 + depth / 6;

            // Increase R when significantly above beta
            if (evalMargin > 100)
               R++;

            // Decrease R in endgames (less than 16 pieces on board)
            if (materialCount < 16)
               R = std::max(2, R - 1);

            // Cap R to reasonable values
            R = std::min(R, depth / 2 + 2);

            // Enhanced zugzwang detection using multiple criteria
            bool skipNullMove = false;

            // 1. Few pieces total
            if (getPieceCounts(board, board.sideToMove) <= 3)
               skipNullMove = true;

            // 2. King+pawns endgame situations
            if (board.pieces(PAWN, board.sideToMove) != 0 &&
                !(board.pieces(KNIGHT, board.sideToMove) |
                  board.pieces(BISHOP, board.sideToMove) |
                  board.pieces(ROOK, board.sideToMove) |
                  board.pieces(QUEEN, board.sideToMove)))
               skipNullMove = true;

            // 3. King vs King+pawns is often zugzwang-prone
            if (getPieceCounts(board, board.sideToMove) <= 5 &&
                board.pieces(PAWN, ~board.sideToMove) != 0)
               skipNullMove = true;

            // 4. Avoid null move when evaluation is too far below beta
            if (staticEval < beta - 120 * depth)
               skipNullMove = true;

            if (!skipNullMove)
            {
               board.makeNullMove();

               // Use narrower window for more efficiency
               int nullScore = -negamax(board, depth - 1 - R, -beta, -beta + 1, table, ply + 1);
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

   // Initialize mobility score
   int mobilityScore = evaluateMobility(board, board.sideToMove);

   // History-based Late Move Pruning thresholds
   // Number of moves to consider before starting to prune based on move count
   // Higher depth = more moves searched before pruning
   const int LateMovePruningCounts[9] = {0, 8, 12, 22, 36, 56, 96, 160, 256};

   // History score thresholds for pruning - negative values indicate poor moves
   // More negative = more aggressive pruning at higher depths
   const int HistoryPruningThreshold[9] = {0, 0, 0, -2000, -4000, -6000, -8000, -10000, -12000};

   // Continuation history thresholds for pruning
   const int ContinuationPruningThreshold[9] = {0, 0, 0, -3000, -5000, -7000, -9000, -11000, -13000};

   // Number of moves to search before applying history pruning
   int moveCountThreshold = depth <= 8 ? LateMovePruningCounts[depth] : 256;


   for (int i = 0; i < moves.size; i++)
   {
      Move move = moves[i].move;

      // Early pruning with SEE
      if (i > 0 && !see(board, move, -65 * depth))
         continue;

      // Then in your move loop:
      bool isCapture = is_capture(board, move);
      bool isPromotion = promoted(move);

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

      // Update mobility incrementally
      updateMobility(board, move, mobilityScore, board.sideToMove);

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
      else if (!isRoot && depth >= 5 && !inCheck && !isCapture && !isPromotion)
      {
         // More aggressive reduction formula - vary based on depth and move index
         int R = int(0.5 + log(depth) * log(i) / 2.0);

         if (i > 4) // More reduction for later moves
            R++;
            
         if (depth > 5) // More reduction for deeper searches
            R++;

         // History-based LMR adjustments
         // Reduce less for moves with good history
         if (history_score > 5600)
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
         int pieceCount = popcount(board.occAll);
         if (pieceCount < 10)
             R = std::max(0, R - 1);  // Reduce less in endgames
         
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

      // Remove the last move from our history to backtrack
      if (moveHistoryCount > 0)
          moveHistoryCount--;
          
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
   bool ttHit;
   TTEntry &entry = table->probe_entry(board.hashKey, ttHit);
   Move ttMove = ttHit ? entry.move : NO_MOVE;

   // Verify the move is legal if we have one
   if (ttMove != NO_MOVE)
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
   int bestScore = 0; // Best score found so far
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
         bestScore = std::max(bestScore, score);
         prevScore = score;
         std::cout << "Depth " << depth << ", Move: "<<convertMoveToUci(bestMove)<<", Score: " << prevScore
         << ", Nodes: " << searchStats.totalNodes()
         << ", NPS: " << searchStats.nodesPerSecond() << std::endl;
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
      bestScore= std::max(bestScore, score);
      // Save this depth's score for next iteration
      prevScore = score;
      std::cout << "Depth " << depth << ", Move: "<<convertMoveToUci(bestMove)<<", Score: " << prevScore
                << ", Nodes: " << searchStats.totalNodes()
                << ", NPS: " << searchStats.nodesPerSecond() << std::endl;
   }
   
   std::cout<<"\nBest move: " << convertMoveToUci(bestMove) <<" with score: " <<bestScore << std::endl;
   searchStats.printStats();
   return bestMove;
}