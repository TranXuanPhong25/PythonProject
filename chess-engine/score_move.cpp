#include "score_move.hpp"

static inline int getContinuationHistoryScores(SearchThread &st, SearchStack *ss,
                                               const Move &move)
{

    int score = 0;

    if ((ss - 1)->move)
    {
        score += (*(ss - 1)->continuationHistory)[st.board.pieceAtB(from(move))][to(move)];
    }

    if ((ss - 2)->move)
    {
        score += (*(ss - 2)->continuationHistory)[st.board.pieceAtB(from(move))][to(move)];
    }

    return score;
}

// Clear history scores between games
void clearHistory()
{
    for (int i = 0; i < 2; i++)
    {
        Piece victim = st.board.pieceAtB(to(list[i].move));
        Piece attacker = st.board.pieceAtB(from(list[i].move));

        // Score tt move the highest
        if (list[i].move == tt_move)
        {
            list[i].value = PvMoveScore;
        }
        else if (victim != None)
        {
            score += mvv_lva[attacker][victim];
            score += (GoodCaptureScore * see(board, move, -107));
        }
        // Check for killer moves
        else if (ply < MAX_PLY)
        {
            if (move == killerMoves[ply][0])
            {
                score = 9000;
            }
            else if (move == killerMoves[ply][1])
            {
                score = 8900;
            }
            // Check for countermove (slightly less value than killer moves)
            else if (move == counterMove)
            {
                score = 8800;
            }
            // History and continuation history for quiet moves
            else
            {
                int side = board.sideToMove == White ? 0 : 1;
                int history_score = historyTable[side][from(move)][to(move)];
                
                // Add continuation history score if we have a previous move
                int continuation_score = 0;
                if (lastMove != NO_MOVE) {
                    Piece lastPiece = board.pieceAtB(to(lastMove));
                    int lastTo = to(lastMove);
                    
                    if (lastPiece != None && attacker != None) {
                        continuation_score = continuationHistory[lastPiece][lastTo][attacker][to(move)];
                    }
                }
                
                // Combine history and continuation history scores
                if (lastMove != NO_MOVE) {
                    // Weight more on regular history as it's more reliable
                    score = (history_score * 2 + continuation_score) / 3;
                } else {
                    score = history_score;
                }
            }
        }
        // History heuristic (quiets that have been good in the past)
        else
        {
            int side = board.sideToMove == White ? 0 : 1;
            score = historyTable[side][from(move)][to(move)];
        }
        
        moves[i].value = score;
    }
}

// Used for Qsearch move scoring
// Enhanced move ordering for quiescence search
void scoreMovesForQS(Board &board, Movelist &list, Move tt_move)
{

    // Loop through moves in movelist.
>>>>>>>>> Temporary merge branch 2
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
            list[i].value = PvMoveScore;
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
void pickNextMove(const int& moveNum, Movelist &list)
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
<<<<<<<<< Temporary merge branch 1
    
    // Swap the highest scoring move to the current position
=========

>>>>>>>>> Temporary merge branch 2
    temp = list[moveNum];
    list[moveNum] = list[bestnum];
    list[bestnum] = temp;
}

<<<<<<<<< Temporary merge branch 1
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
=========
void updateH(int16_t &historyScore, const int bonus)
{
    historyScore += bonus - historyScore * std::abs(bonus) / MAXHISTORY;
}

void updateCH(int16_t &historyScore, const int bonus)
{

    historyScore += bonus - historyScore * std::abs(bonus) / MAXCOUNTERHISTORY;
}

void updateContinuationHistories(SearchStack *ss, Piece piece, Move move, int bonus)
{

    if ((ss - 1)->move)
    {
        updateCH((*(ss - 1)->continuationHistory)[ss->movedPice][to(move)], bonus);
    }

    if ((ss - 2)->move)
    {
        updateCH((*(ss - 2)->continuationHistory)[ss->movedPice][to(move)], bonus);
    }
}

void updateHistories(SearchThread &st, SearchStack *ss, Move bestmove, Movelist &quietList, int depth)
{
    // Update best move score
    int bonus = historyBonus(depth);

    if (depth > 2)
    {
        updateH(st.searchHistory[st.board.pieceAtB(from(bestmove))][to(bestmove)], bonus);
        updateContinuationHistories(ss, st.board.pieceAtB(from(bestmove)), bestmove, bonus);
    }

    for (int i = 0; i < quietList.size; i++)
    {
        Move move = quietList[i].move;

        if (move == bestmove)
            continue; // Don't give penalty to our best move, so skip it.

        // Penalize moves that didn't cause a beta cutoff.
        updateContinuationHistories(ss, st.board.pieceAtB(from(move)), move, -bonus);
        updateH(st.searchHistory[st.board.pieceAtB(from(move))][to(move)], -bonus);
    }
}

int getHistoryScores(int &his, int &ch, int &fmh, SearchThread &st, SearchStack *ss, const Move move)
{
    Piece moved_piece = st.board.pieceAtB(from(move));

    his = st.searchHistory[moved_piece][to(move)];

    ch = (ss - 1)->move ? (*(ss - 1)->continuationHistory)[moved_piece][to(move)] : 0;
    fmh = (ss - 2)->move ? (*(ss - 2)->continuationHistory)[moved_piece][to(move)] : 0;

    return his + ch + fmh;
>>>>>>>>> Temporary merge branch 2
}
