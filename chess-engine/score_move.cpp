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

bool moveResolvesCheck(Board &board, Move move)
{
    board.makeMove(move);
    bool resolvesCheck = !board.isSquareAttacked(~board.sideToMove, board.KingSQ(board.sideToMove));
    board.unmakeMove(move);
    return resolvesCheck;
}

void scoreMoves(Movelist &moves, Board &board, Move ttMove, int ply)
{
    int mobilityScore = evaluateMobility(board, board.sideToMove);

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
    
    // Single pass: score all moves and check for checkmates
    for (int i = 0; i < moves.size; ++i)
    {
        Move move = moves[i].move;
        int score = 0;
        Piece attacker = board.pieceAtB(from(move));
        Piece victim = board.pieceAtB(to(move));

        // Prioritize moves that resolve checks
        if (board.isSquareAttacked(~board.sideToMove, board.KingSQ(board.sideToMove))) {
            if (moveResolvesCheck(board, move)) {
                score = 10000; // High priority for check-evasion moves
            }
        }
        
        // Prioritize TT move
        if (move == ttMove)
        {
            score = PvMoveScore;
        }
        // capture
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

        // Temporarily update mobility for this move
        updateMobility(board, move, mobilityScore, board.sideToMove);
 
        // Add mobility score to the move's score
        score += mobilityScore;
        
        // Check if this move delivers checkmate
        // Only do this check for potentially good moves to save time
        if (move == ttMove || victim != None || score >= 8800 || 
            board.isSquareAttacked(~board.sideToMove, board.KingSQ(board.sideToMove))) {
            
            board.makeMove(move);
            
            // Check if opponent is in check and has no legal moves
            Square enemyKingSq = board.KingSQ(board.sideToMove);
            if (board.isSquareAttacked(~board.sideToMove, enemyKingSq)) {
                Movelist legalMoves;
                if (board.sideToMove == White) {
                    Movegen::legalmoves<White, Movetype::ALL>(board, legalMoves);
                } else {
                    Movegen::legalmoves<Black, Movetype::ALL>(board, legalMoves);
                }
                if (legalMoves.size == 0) {
                    // This move delivers checkmate, use ISMATE - ply for proper mate distance scoring
                    // Adding to PvMoveScore ensures it's always prioritized over TT moves
                    score = PvMoveScore + (ISMATE - ply);
                }
            }
            board.unmakeMove(move);
        }

        moves[i].value = score;
    }
}

void ScoreMovesForQS(Board &board, Movelist &list, Move tt_move)
{
    // Single pass: score all moves including checkmate detection
    for (int i = 0; i < list.size; i++)
    {
        Move move = list[i].move;
        Piece victim = board.pieceAtB(to(move));
        Piece attacker = board.pieceAtB(from(move));
        
        // First check if it's a TT move
        if (move == tt_move)
        {
            list[i].value = PvMoveScore;
        }
        // Then score based on MVV-LVA for captures
        else if (victim != None)
        {
            // move is list[i].move
            list[i].value = mvv_lva[attacker][victim] +
                           (GoodCaptureScore * see(board, move, -107));
        }
        
        // For promising moves, check if they deliver checkmate
        if (victim != None || move == tt_move) {
            board.makeMove(move);
            
            // Check if opponent is in check and has no legal moves
            Square enemyKingSq = board.KingSQ(board.sideToMove);
            if (board.isSquareAttacked(~board.sideToMove, enemyKingSq)) {
                Movelist legalMoves;
                if (board.sideToMove == White) {
                    Movegen::legalmoves<White, Movetype::ALL>(board, legalMoves);
                } else {
                    Movegen::legalmoves<Black, Movetype::ALL>(board, legalMoves);
                }
                if (legalMoves.size == 0) {
                    // This move delivers checkmate, use ISMATE - ply for proper mate distance scoring
                    // The quiescence search is typically called with ply values continuing from the main search
                    list[i].value = PvMoveScore + (ISMATE - board.halfMoveClock);
                }
            }
            board.unmakeMove(move);
        }
    }
}

void pickNextMove(const int &moveNum, Movelist &list)
{
    ExtMove temp;
    int index = 0;
    int bestscore = -INF_BOUND;
    int bestnum = moveNum;
    for (index = moveNum; index < list.size; ++index)
    {
        if (list[index].value > bestscore)
        {
            bestscore = list[index].value;
            bestnum = index;
        }
    }
    temp = list[moveNum];
    list[moveNum] = list[bestnum]; // Sort the highest score move to highest.
    list[bestnum] = temp;
}
