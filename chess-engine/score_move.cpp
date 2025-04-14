#include "score_move.hpp"

Move killerMoves[MAX_PLY][2] = {{NO_MOVE}};
int historyTable[2][64][64] = {{{0}}};
Move counterMoveTable[12][64] = {{NO_MOVE}};
// Continuation history table for tracking move sequences
// [fromPiece][fromSq][toPiece][toSq]
int continuationHistory[12][64][12][64] = {{{{0}}}};

// Function to add a new killer move
void addKillerMove(Move move, int ply)
{
    // Don't add the same killer twice
    if (move != killerMoves[ply][0])
    {
        // Shift existing killer
        killerMoves[ply][1] = killerMoves[ply][0];
        // Add new first killer
        killerMoves[ply][0] = move;
    }
}

// Update history score for a good move
void updateHistory(Board &board, Move move, int depth, bool isCutoff)
{
    int side = board.sideToMove == White ? 0 : 1;
    int from_sq = from(move);
    int to_sq = to(move);
    
    // Stockfish-style history update with depth scaling
    int bonus = std::min(32 * depth * depth, 1024);
    
    // Reduce bonus if not a cutoff move
    if (!isCutoff)
        bonus = -bonus;
    
    // Decay existing history and add new bonus 
    historyTable[side][from_sq][to_sq] = 
        historyTable[side][from_sq][to_sq] * 32 / 33 + bonus;
    
    // Clamp to prevent overflow
    if (historyTable[side][from_sq][to_sq] > 20000)
        historyTable[side][from_sq][to_sq] = 20000;
    if (historyTable[side][from_sq][to_sq] < -20000)
        historyTable[side][from_sq][to_sq] = -20000;
}

// Update countermove history - store a move that was good against opponent's last move
void updateCounterMove(Board &board, Move lastMove, Move counterMove)
{
    if (lastMove == NO_MOVE || counterMove == NO_MOVE)
        return;
    
    // Get the piece and square info from the last move
    int lastPiece = board.pieceAtB(to(lastMove)); // The piece at the destination square
    int lastToSquare = to(lastMove); // Where the opponent's piece landed
    
    // Store this counter move
    counterMoveTable[lastPiece][lastToSquare] = counterMove;
}

// Clear history scores between games
void clearHistory()
{
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 64; j++)
        {
            for (int k = 0; k < 64; k++)
            {
                historyTable[i][j][k] = 0;
            }
        }
    }
    
    // Also clear killer moves
    for (int i = 0; i < MAX_PLY; i++)
    {
        killerMoves[i][0] = NO_MOVE;
        killerMoves[i][1] = NO_MOVE;
    }
    
    // Clear countermove table
    for (int i = 0; i < 12; i++)
    {
        for (int j = 0; j < 64; j++)
        {
            counterMoveTable[i][j] = NO_MOVE;
        }
    }
    
    // Clear our move history
    clearMoveHistory();
    
    // Clear continuation history
    for (int p1 = 0; p1 < 12; p1++) {
        for (int sq1 = 0; sq1 < 64; sq1++) {
            for (int p2 = 0; p2 < 12; p2++) {
                for (int sq2 = 0; sq2 < 64; sq2++) {
                    continuationHistory[p1][sq1][p2][sq2] = 0;
                }
            }
        }
    }
}

// Score a single move - used for individual move scoring
int scoreSingleMove(Board &board, Move move, Move ttMove, int ply)
{
    // Get the last move for countermove lookup
    Move lastMove = getLastMove();
    
    // Get piece information
    Piece attacker = board.pieceAtB(from(move));
    Piece victim = board.pieceAtB(to(move));
    bool isCapture = (victim != None);
    bool isPromotion = promoted(move);
    int side = board.sideToMove == White ? 0 : 1;
    
    // Base score
    int score = 0;
    
    // 1. Prioritize TT move with highest score
    if (move == ttMove)
        return PvMoveScore;
    
    // 2. Captures and promotions
    if (isCapture || isPromotion)
    {
        if (isCapture)
        {
            // MVV-LVA scoring for captures
            score += mvv_lva[attacker][victim];
            
            // Check if this is a good capture with SEE
            bool isGoodCapture = see(board, move, -100);
            if (isGoodCapture)
                score += GoodCaptureScore;
            else
                score += BadCaptureScore;
        }
        
        // Promotions - prioritize queen promotions
        if (isPromotion)
        {
            score += PromotionScore;
            // Add bonus for queen promotions
            score+= 200; // Adjust this value as needed
        }
    }
    // 3. Killer moves
    else if (ply < MAX_PLY)
    {
        if (move == killerMoves[ply][0])
            score += Killer1Score;
        else if (move == killerMoves[ply][1])
            score += Killer2Score;
        // 4. Countermoves
        else if (lastMove != NO_MOVE)
        {
            Piece lastPiece = board.pieceAtB(to(lastMove));
            int lastToSq = to(lastMove);
            
            if (lastPiece != None && move == counterMoveTable[lastPiece][lastToSq])
                score += CounterMoveScore;
        }
    }
    
    // 5. History scoring for quiet moves
    if (!isCapture && !isPromotion)
    {
        // Get history score
        int history_score = historyTable[side][from(move)][to(move)];
        
        // Get continuation history score
        int continuation_score = 0;
        if (lastMove != NO_MOVE)
        {
            Piece lastPiece = board.pieceAtB(to(lastMove));
            int lastToSq = to(lastMove);
            
            if (lastPiece != None && attacker != None)
            {
                continuation_score = continuationHistory[lastPiece][lastToSq][attacker][to(move)];
            }
        }
        
        // Combine history scores with appropriate weights
        if (lastMove != NO_MOVE)
        {
            // Weight regular history highest, then continuation
            score += (history_score * 5 + continuation_score * 3) / 8;
        }
        else
        {
            // Without a last move, weight regular history
            score += history_score;
        }
    }
    
    return score;
}

// Enhanced move ordering function for regular search
void scoreMoves(Movelist &moves, Board &board, Move ttMove, int ply)
{
    // Get the current position's last move for countermove lookup
    Move lastMove = getLastMove();
    Move counterMove = NO_MOVE;
    
    // If there was a last move, check if we have a countermove for it
    if (lastMove != NO_MOVE)
    {
        Piece lastMovePiece = board.pieceAtB(to(lastMove));
        int lastMoveToSq = to(lastMove);
        
        // Get the countermove for this piece/square combination
        counterMove = counterMoveTable[lastMovePiece][lastMoveToSq];
    }
    
    // Pre-calculate mobility score once
    int mobilityScore = evaluateMobility(board, board.sideToMove);
    
    // Score each move
    for (int i = 0; i < moves.size; ++i)
    {
        Move move = moves[i].move;
        
        // Get base move score
        int score = scoreSingleMove(board, move, ttMove, ply);
        
        // Update mobility for this move (a more expensive operation,
        // only needed for certain types of moves)
        bool isCapture = (board.pieceAtB(to(move)) != None);
        bool isPromotion = promoted(move);
        
        // Only do mobility calculation for captures and important moves
        if (isCapture || isPromotion || move == ttMove || 
            move == killerMoves[ply][0] || move == killerMoves[ply][1] || 
            move == counterMove)
        {
            // Update mobility for this specific move
            updateMobility(board, move, mobilityScore, board.sideToMove);
            
            // Add mobility score to the move's score
            score += mobilityScore;
        }
        
        // Assign final score to the move
        moves[i].value = score;
    }
}

// Enhanced move ordering for quiescence search
void ScoreMovesForQS(Board &board, Movelist &list, Move tt_move)
{
    // Score each move in the quiescence search list
    for (int i = 0; i < list.size; i++)
    {
        Move move = list[i].move;
        Piece victim = board.pieceAtB(to(move));
        Piece attacker = board.pieceAtB(from(move));
        bool isPromotion = promoted(move);
        
        // Base score
        int score = 0;
        
        // 1. PV move from transposition table
        if (move == tt_move)
        {
            score = PvMoveScore;
        }
        // 2. Captures and promotions
        else if (victim != None || isPromotion)
        {
            // For captures, use MVV-LVA and SEE
            if (victim != None)
            {
                // MVV-LVA base scoring
                score += mvv_lva[attacker][victim];
                
                // Add SEE score for better ordering
                bool isGoodCapture = see(board, move, -65);
                if (isGoodCapture)
                {
                    // Good capture
                    score += GoodCaptureScore;
                }
                else
                {
                    // Bad capture, but sometimes necessary in QS
                    score += BadCaptureScore;
                }
            }
            
            // For promotions, prioritize queen promotions
            if (isPromotion)
            {
                score += PromotionScore;
                score += 200; 
            }
        }
        
        // Set the final score
        list[i].value = score;
    }
}

// Find the best move from the list at the given position
void pickNextMove(const int &moveNum, Movelist &list)
{
    ExtMove temp;
    int index = 0;
    int bestscore = -INF_BOUND;
    int bestnum = moveNum;
    
    // Find the highest scoring move
    for (index = moveNum; index < list.size; ++index)
    {
        if (list[index].value > bestscore)
        {
            bestscore = list[index].value;
            bestnum = index;
        }
    }
    
    // Swap the highest scoring move to the current position
    temp = list[moveNum];
    list[moveNum] = list[bestnum];
    list[bestnum] = temp;
}

bool StagedMoveGenerator::hasNext() const
{
    // Still have moves to generate in current stage or more stages to go
    if (currentMoveIndex < moves.size || currentStage < REMAINING_MOVES)
        return true;
    return false;
}

Move StagedMoveGenerator::nextMove()
{
    if (!hasNext())
        return NO_MOVE;
    
    // TT move stage
    if (currentStage == TT_MOVE)
    {
        currentStage = GOOD_CAPTURES;
        currentMoveIndex = 0;
        
        // If we have a TT move, return it first
        if (ttMove != NO_MOVE && !ttMoveSearched)
        {
            // Verify it's a legal move in our list
            for (int i = 0; i < moves.size; i++)
            {
                if (moves[i].move == ttMove)
                {
                    ttMoveSearched = true;
                    return ttMove;
                }
            }
        }
        // No TT move or not found, skip to next stage
        ttMoveSearched = true;
    }
    
    // Good captures stage
    if (currentStage == GOOD_CAPTURES)
    {
        while (currentMoveIndex < moves.size)
        {
            Move move = moves[currentMoveIndex++].move;
            
            // Skip the TT move as it's already been returned
            if (move == ttMove)
                continue;
            
            Piece victim = board.pieceAtB(to(move));
            
            // Process captures with positive SEE
            if (victim != None && see(board, move, -100))
            {
                return move;
            }
        }
        
        // Move to killer moves stage
        currentStage = KILLER_MOVES;
        currentMoveIndex = 0;
    }
    
    // Killer moves stage
    if (currentStage == KILLER_MOVES)
    {
        // Try both killer moves
        while (currentMoveIndex < 2)
        {
            Move killerMove = killerMoves[ply][currentMoveIndex++];
            
            // Skip invalid or already processed moves
            if (killerMove == NO_MOVE || killerMove == ttMove)
                continue;
                
            // Check if it's a legal move in our list
            for (int i = 0; i < moves.size; i++)
            {
                if (moves[i].move == killerMove && 
                    board.pieceAtB(to(killerMove)) == None) // Make sure it's not a capture
                {
                    return killerMove;
                }
            }
        }
        
        // Move to counter moves stage
        currentStage = COUNTER_MOVES;
        currentMoveIndex = 0;
    }
    
    // Counter moves stage
    if (currentStage == COUNTER_MOVES)
    {
        // Get the last move from history
        Move lastMove = getLastMove();
        
        if (lastMove != NO_MOVE)
        {
            // Get the counter move for this position
            Piece lastPiece = board.pieceAtB(to(lastMove));
            int lastToSq = to(lastMove);
            
            if (lastPiece != None)
            {
                Move counterMove = counterMoveTable[lastPiece][lastToSq];
                
                // Skip if already processed
                if (counterMove != NO_MOVE && 
                    counterMove != ttMove && 
                    counterMove != killerMoves[ply][0] && 
                    counterMove != killerMoves[ply][1])
                {
                    // Check if it's a legal move in our list
                    for (int i = 0; i < moves.size; i++)
                    {
                        if (moves[i].move == counterMove && 
                            board.pieceAtB(to(counterMove)) == None) // Make sure it's not a capture
                        {
                            currentStage = QUIET_MOVES;
                            currentMoveIndex = 0;
                            return counterMove;
                        }
                    }
                }
            }
        }
        
        // No counter move or not found, move to quiet moves stage
        currentStage = QUIET_MOVES;
        currentMoveIndex = 0;
    }
    
    // Quiet moves stage (with good history)
    if (currentStage == QUIET_MOVES)
    {
        while (currentMoveIndex < moves.size)
        {
            Move move = moves[currentMoveIndex++].move;
            
            // Skip moves already processed
            if (move == ttMove || 
                move == killerMoves[ply][0] || 
                move == killerMoves[ply][1])
                continue;
                
            // Process only quiet moves with good history
            if (board.pieceAtB(to(move)) == None && !promoted(move))
            {
                int side = board.sideToMove == White ? 0 : 1;
                int history = historyTable[side][from(move)][to(move)];
                
                // Only return moves with decent history scores
                if (history > -5000)
                    return move;
            }
        }
        
        // Move to bad captures stage
        currentStage = BAD_CAPTURES;
        currentMoveIndex = 0;
    }
    
    // Bad captures stage
    if (currentStage == BAD_CAPTURES)
    {
        while (currentMoveIndex < moves.size)
        {
            Move move = moves[currentMoveIndex++].move;
            
            // Skip moves already processed
            if (move == ttMove)
                continue;
                
            Piece victim = board.pieceAtB(to(move));
            
            // Process captures with negative SEE
            if (victim != None && !see(board, move, -100))
            {
                return move;
            }
        }
        
        // Move to remaining moves stage
        currentStage = REMAINING_MOVES;
        currentMoveIndex = 0;
    }
    
    // Remaining moves stage
    if (currentStage == REMAINING_MOVES)
    {
        while (currentMoveIndex < moves.size)
        {
            Move move = moves[currentMoveIndex++].move;
            
            // Skip all moves already processed at earlier stages
            if (move == ttMove || 
                move == killerMoves[ply][0] || 
                move == killerMoves[ply][1] || 
                board.pieceAtB(to(move)) != None)
                continue;
                
            // Return any remaining unprocessed moves
            return move;
        }
    }
    
    // No moves left
    return NO_MOVE;
}
