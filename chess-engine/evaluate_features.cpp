#include "evaluate_features.hpp"

// External declarations for search-related variables
extern Move killerMoves[MAX_PLY][2];
extern int historyTable[2][64][64];
extern int mvv_lva[12][12];

using namespace Chess;

// Constructor implementation for PawnStructureContext
PawnStructureContext::PawnStructureContext(const Board& b) : board(b) {
    // Initialize bitboards
    whitePawns = board.pieces(PAWN, White);
    blackPawns = board.pieces(PAWN, Black);
    
    // Calculate pawn attacks
    whitePawnAttacks = 0ULL;
    blackPawnAttacks = 0ULL;
    
    // King positions
    whiteKingSq = board.KingSQ(White);
    blackKingSq = board.KingSQ(Black);
    
    // Calculate game phase
    gamePhase = getGamePhase(board);
    
    // Initialize file-based and rank-based bitboards
    for (int file = 0; file < 8; ++file) {
        whitePawnsOnFile[file] = whitePawns & MASK_FILE[file];
        blackPawnsOnFile[file] = blackPawns & MASK_FILE[file];
    }
    
    for (int rank = 0; rank < 8; ++rank) {
        whitePawnsOnRank[rank] = whitePawns & MASK_RANK[rank];
        blackPawnsOnRank[rank] = blackPawns & MASK_RANK[rank];
    }
    
    // Calculate pawn attacks
    Bitboard tempWhitePawns = whitePawns;
    while (tempWhitePawns) {
        Square sq = static_cast<Square>(pop_lsb(tempWhitePawns));
        whitePawnAttacks |= PawnAttacks(sq, White);
    }
    
    Bitboard tempBlackPawns = blackPawns;
    while (tempBlackPawns) {
        Square sq = static_cast<Square>(pop_lsb(tempBlackPawns));
        blackPawnAttacks |= PawnAttacks(sq, Black);
    }
}

// Helper method implementations
bool PawnStructureContext::isPassed(Square sq, Color color) const {
    // Use the precalculated file-based arrays from the context
    int file = square_file(sq);
    int rank = square_rank(sq);
    
    // Get enemy pawns directly from context
    Bitboard enemyPawns = (color == White) ? blackPawns : whitePawns;
    
    // Create a mask for files (current, left, and right) using precalculated MASK_FILE arrays
    Bitboard fileMask = MASK_FILE[file];
    if (file > 0) fileMask |= MASK_FILE[file - 1];
    if (file < 7) fileMask |= MASK_FILE[file + 1];
    
    // More efficient rank mask creation
    Bitboard rankMask;
    if (color == White) {
        // For white pawns, check ranks ahead (higher ranks)
        rankMask = ~((1ULL << ((rank + 1) * 8)) - 1);
    } else {
        // For black pawns, check ranks ahead (lower ranks)
        rankMask = (1ULL << (rank * 8)) - 1;
    }
    
    // Single operation to check for blockers in the stopping mask
    Bitboard blockerZone = fileMask & rankMask;
    return (blockerZone & enemyPawns) == 0;
}

bool PawnStructureContext::isIsolated(Square sq, Color color) const {
    int file = square_file(sq);
    Bitboard friendlyPawns = (color == White) ? whitePawns : blackPawns;
    
    // Check pawns on adjacent files
    Bitboard leftSupport = (file > 0) ? (friendlyPawns & MASK_FILE[file - 1]) : 0;
    Bitboard rightSupport = (file < 7) ? (friendlyPawns & MASK_FILE[file + 1]) : 0;
    
    return !leftSupport && !rightSupport;
}

bool PawnStructureContext::isDoubled(Square sq, Color color) const {
    int file = square_file(sq);
    Bitboard pawnsInFile = (color == White) ? whitePawnsOnFile[file] : blackPawnsOnFile[file];
    
    return popcount(pawnsInFile) > 1;
}

bool PawnStructureContext::isConnected(Square sq, Color color) const {
    int file = square_file(sq);
    int rank = square_rank(sq);
    Bitboard friendlyPawns = (color == White) ? whitePawns : blackPawns;
    
    // Check for connected pawns (side by side)
    Bitboard connected = 0ULL;
    if (file > 0)
        connected |= 1ULL << (rank * 8 + file - 1);
    if (file < 7)
        connected |= 1ULL << (rank * 8 + file + 1);
    
    return (connected & friendlyPawns) != 0;
}

bool PawnStructureContext::isPhalanx(Square sq, Color color) const {
    int file = square_file(sq);
    int rank = square_rank(sq);
    Bitboard friendlyPawns = (color == White) ? whitePawns : blackPawns;
    
    // Check for phalanx formation (adjacent pawns in the same rank)
    Bitboard phalanx = 0ULL;
    if (file > 0)
        phalanx |= 1ULL << (rank * 8 + file - 1);
    if (file < 7)
        phalanx |= 1ULL << (rank * 8 + file + 1);
    
    return (phalanx & friendlyPawns) != 0;
}

bool PawnStructureContext::isBackward(Square sq, Color color) const {
    int file = square_file(sq);
    int rank = square_rank(sq);
    
    // The square in front of the pawn
    int frontRank = (color == White) ? rank + 1 : rank - 1;
    if (frontRank < 0 || frontRank >= 8) return false;
    
    Square frontSquare = file_rank_square(File(file), Rank(frontRank));
    bool blocked = board.pieceAtB(frontSquare) != None; // Check if blocked
    
    // Check if the pawn is blocked by an enemy pawn
    Bitboard enemyAttacks = (color == White) ? blackPawnAttacks : whitePawnAttacks;
    bool controlled = enemyAttacks & (1ULL << frontSquare);
    
    // Check if the pawn is supported by another pawn
    bool supported = false;
    for (int df = -1; df <= 1; df += 2) {
        int adjFile = file + df;
        if (adjFile >= 0 && adjFile < 8) {
            Square supportSq = file_rank_square(File(adjFile), Rank(rank));
            if (board.pieceAtB(supportSq) == makePiece(PAWN, color)) {
                supported = true;
                break;
            }
        }
    }
    
    // If the pawn is blocked or controlled by an enemy pawn and not supported, it's backward
    return !supported && (blocked || controlled);
}

bool PawnStructureContext::isShieldingKing(Square sq, Color color) const {
    int kingFile = square_file(color == White ? whiteKingSq : blackKingSq);
    int kingRank = square_rank(color == White ? whiteKingSq : blackKingSq);
    int pawnFile = square_file(sq);
    int pawnRank = square_rank(sq);
    
    // Check if pawn is in front of the king
    if (abs(kingFile - pawnFile) <= 1) {
        if (color == White && pawnRank == kingRank + 1) {
            return true;
        } else if (color == Black && pawnRank == kingRank - 1) {
            return true;
        }
    }
    
    return false;
}

// Pawn Structure: isolated, doubled, passed pawns (basic skeleton)
int evaluatePawnStructure(const Board &board) {
    // Create a context object for pawn structure evaluation
    PawnStructureContext ctx(board);
    
    int score = 0;
    score += evaluateDoubledPawns(ctx, White);
    score -= evaluateDoubledPawns(ctx, Black);

    score += evaluateIsolatedPawns(ctx, White);
    score -= evaluateIsolatedPawns(ctx, Black);

    score += evaluatePassedPawns(ctx, White);
    score -= evaluatePassedPawns(ctx, Black);

    score += evaluatePassedPawnSupport(ctx, White);
    score -= evaluatePassedPawnSupport(ctx, Black);

    score += evaluateConnectedPawns(ctx, White);
    score -= evaluateConnectedPawns(ctx, Black);

    score += evaluatePhalanxPawns(ctx, White);
    score -= evaluatePhalanxPawns(ctx, Black);

    score += evaluateBlockedPawns(ctx, White);
    score -= evaluateBlockedPawns(ctx, Black);

    score += evaluatePawnChains(ctx, White);
    score -= evaluatePawnChains(ctx, Black);

    score += evaluatePawnShield(ctx, White);
    score -= evaluatePawnShield(ctx, Black);

    score += evaluateBackwardPawns(ctx, White);
    score -= evaluateBackwardPawns(ctx, Black);

    score += evaluateHolesAndOutposts(ctx, White);
    score -= evaluateHolesAndOutposts(ctx, Black);

    score += evaluatePawnLeverThreats(ctx, White);
    score -= evaluatePawnLeverThreats(ctx, Black);

    score += evaluateFileOpenness(ctx, White);
    score -= evaluateFileOpenness(ctx, Black);

    score += evaluateSpaceAdvantage(ctx, White);
    score -= evaluateSpaceAdvantage(ctx, Black);

    score += evaluatePawnMajority(ctx, White);
    score -= evaluatePawnMajority(ctx, Black);

    score += evaluatePawnStorm(ctx, White);
    score -= evaluatePawnStorm(ctx, Black);

    return score;
}

// Center Control: reward for occupying/attacking center (d4/e4/d5/e5)
int evaluateCenterControl(const Board& board) {
    int score = 0;
    const Square centerSquares[] = { SQ_D4, SQ_E4, SQ_D5, SQ_E5 };

    Board tempBoard = board;

    for (Square sq : centerSquares) {
        auto piece = tempBoard.pieceAtB(sq);
        if (piece != None) {
            if (tempBoard.colorOf(sq) == White)
                score += 5;
            else if (tempBoard.colorOf(sq) == Black)
                score -= 5;
        }

            // Check for attackers on the center squares
            U64 attackersWhite = tempBoard.attackersForSide(White, sq, tempBoard.occAll);
            U64 attackersBlack = tempBoard.attackersForSide(Black, sq, tempBoard.occAll);
        
            score += 3 * popcount(attackersWhite); // Bonus if white attacks center
            score -= 3 * popcount(attackersBlack); // Bonus if black attacks center
    }
    return score;
}


int evaluateDoubledPawns(const PawnStructureContext &ctx, Color color) {
    int penalty = 0;
    // Get all pawns of the specified color
    Bitboard pawns = (color == White) ? ctx.whitePawns : ctx.blackPawns;

    // Iterate through each file (0 to 7)
    for (int file = 0; file < 8; ++file) {
        // Create a mask for the current file
        Bitboard pawnsInFile = pawns & MASK_FILE[file];
        // Count the number of pawns in that file
        int count = popcount(pawnsInFile);
        // If there are more than one pawn in the file, apply penalty
        if (count > 1) {
            penalty += (count - 1) * 10; // 10 points for each extra pawn in the file
        }
    }

    return penalty;
}


int evaluateIsolatedPawns(const PawnStructureContext &ctx, Color color) {
    Bitboard pawns = (color == White) ? ctx.whitePawns : ctx.blackPawns;
    int penalty = 0;

    // Iterate through each file (0 to 7)
    for (int file = 0; file < 8; ++file) {
        Bitboard pawnsInFile = pawns & MASK_FILE[file]; 
        // Check the left and right
        Bitboard leftSupport = (file > 0) ? (pawns & MASK_FILE[file - 1]) : 0;
        Bitboard rightSupport = (file < 7) ? (pawns & MASK_FILE[file + 1]) : 0;

        // If no support from both sides 
        if (pawnsInFile && !leftSupport && !rightSupport) {
            penalty += popcount(pawnsInFile) * 15; // minus 15 points for each isolated pawn
        }
    }

    return penalty;
}

// Efficiently evaluate passed pawns using the context
int evaluatePassedPawns(const PawnStructureContext &ctx, Color color) {
    int bonus = 0;
    Bitboard pawns = (color == White) ? ctx.whitePawns : ctx.blackPawns;
    
    // Reuse the precomputed values from context
    while (pawns) {
        Square sq = static_cast<Square>(pop_lsb(pawns));
        
        // Use the helper method in context
        if (ctx.isPassed(sq, color)) {
            int rank = square_rank(sq);
            int advancement = (color == White) ? rank : (7 - rank);
            
            // Apply bonus based on advancement with game phase consideration
            int baseBonus = advancement * 5 + 10;
            
            // Apply more bonus in endgame for passed pawns
            int phaseAdjustedBonus = static_cast<int>(baseBonus * (1.0 + ctx.gamePhase));
            
            // Extra bonus for advanced passed pawns
            if (advancement >= 5) {
                phaseAdjustedBonus += 10;
            }
            
            bonus += phaseAdjustedBonus;
        }
    }
    
    return bonus;
}

int evaluatePassedPawnSupport(const PawnStructureContext &ctx, Color color) {
    Bitboard pawns = (color == White) ? ctx.whitePawns : ctx.blackPawns;
    int bonus = 0;

    // Create a mutable copy of the board to avoid const issues
    Board &mutableBoard = const_cast<Board &>(ctx.board);

    while (pawns) {
        Square sq = static_cast<Square>(pop_lsb(pawns));
        int file = square_file(sq);
        int rank = square_rank(sq);

        // Check if the pawn is passed
        if ((mutableBoard.pieces(PAWN, ~color) & mutableBoard.SQUARES_BETWEEN_BB[sq][mutableBoard.KingSQ(~color)]) == 0) {
            int dir = (color == White) ? 1 : -1;
            int forwardRank = rank + dir;

            Bitboard support = 0ULL;

            // Create a mask for the squares in front of the pawn
            if (forwardRank >= 0 && forwardRank < 8) {
                if (file > 0)
                    support |= 1ULL << (forwardRank * 8 + file - 1);
                if (file < 7)
                    support |= 1ULL << (forwardRank * 8 + file + 1);
            }

            // Calculate the support from pawns
            int pawnSupporters = popcount(support & mutableBoard.pieces(PAWN, color));
            bonus += 15 * pawnSupporters;

            // Calculate the support from other pieces
            Bitboard otherSupport = mutableBoard.attackersForSide(color, sq, mutableBoard.occAll) & ~mutableBoard.pieces(PAWN, color);
            bonus += 5 * popcount(otherSupport);

            // Bonus for being close to promotion
            int promotionDistance = (color == White) ? (7 - rank) : rank;
            bonus += (6 - promotionDistance) * 2;
        }
    }

    return bonus;
}

int evaluateConnectedPawns(const PawnStructureContext &ctx, Color color) {
    Bitboard pawns = (color == White) ? ctx.whitePawns : ctx.blackPawns;
    int bonus = 0;

    while (pawns) {
        Square sq = static_cast<Square>(pop_lsb(pawns));
        int file = square_file(sq);
        int rank = square_rank(sq);

        Bitboard connected = 0ULL;
        int dir = (color == White) ? -1 : 1;
        int forwardRank = rank + dir;

        // Check for connected pawns in the same file and diagonally
        if (file > 0 && forwardRank >= 0 && forwardRank < 8)
            connected |= 1ULL << (forwardRank * 8 + file - 1);
        if (file < 7 && forwardRank >= 0 && forwardRank < 8)
            connected |= 1ULL << (forwardRank * 8 + file + 1);
        if (file > 0)
            connected |= 1ULL << (rank * 8 + file - 1);
        if (file < 7)
            connected |= 1ULL << (rank * 8 + file + 1);

        if (connected & ctx.board.pieces(PAWN, color)) {
            bonus += 12; // Bonus for connected pawns

            // // Check if the pawn is part of a historical good move
            // int side = (color == White) ? 0 : 1;
            // if (historyTable[side][sq][connected]) {
            //     bonus += 10; // Thưởng thêm nếu quân tốt liên quan đến nước đi tốt trong lịch sử
            // }
        }
    }

    return bonus;
}

int evaluatePhalanxPawns(const PawnStructureContext &ctx, Color color) {
    Bitboard pawns = (color == White) ? ctx.whitePawns : ctx.blackPawns;
    int bonus = 0;

    for (int rank = 0; rank < 8; ++rank) {
        Bitboard pawnsInRank = pawns & MASK_RANK[rank];

        // Tạo mặt nạ để tránh lỗi khi shift ngang
        Bitboard safeLeftShift = pawnsInRank & ~MASK_FILE[7]; // loại bỏ tốt ở file H
        Bitboard safeRightShift = pawnsInRank & ~MASK_FILE[0]; // loại bỏ tốt ở file A

        // Cặp tốt liền nhau (phalanx)
        Bitboard phalanx = (safeLeftShift << 1) & pawnsInRank;
        bonus += 10 * popcount(phalanx);

        // Chuỗi tốt dài hơn 2 (ví dụ A-B-C)
        Bitboard extendedPhalanx = (phalanx << 1) & pawnsInRank;
        bonus += 5 * popcount(extendedPhalanx);
    }

    return bonus;
}

int evaluateBlockedPawns(const PawnStructureContext &ctx, Color color) {
    Bitboard pawns = (color == White) ? ctx.whitePawns : ctx.blackPawns;
    int penalty = 0;

    while (pawns) {
        Square sq = static_cast<Square>(pop_lsb(pawns));
        int file = square_file(sq);
        int rank = square_rank(sq);

        // Check if the pawn is blocked
        if ((color == White && rank == 6) || (color == Black && rank == 1)) {
            penalty += 10; // Penalty for blocked pawn
        }
    }

    return penalty;
}

int evaluatePawnChains(const PawnStructureContext &ctx, Color color) {
    Bitboard pawns = (color == White) ? ctx.whitePawns : ctx.blackPawns;
    int bonus = 0;

    while (pawns) {
        Square sq = static_cast<Square>(pop_lsb(pawns));
        int file = square_file(sq);
        int rank = square_rank(sq);

        // Check if the pawn is part of a chain
        if ((color == White && rank == 0) || (color == Black && rank == 7))
            continue;

        // Check for pawns in the same file and diagonally
        Bitboard protectors = 0ULL;
        if (color == White && rank > 0) {
            if (file > 0) protectors |= 1ULL << ((rank - 1) * 8 + (file - 1));
            if (file < 7) protectors |= 1ULL << ((rank - 1) * 8 + (file + 1));
        } else if (color == Black && rank < 7) {
            if (file > 0) protectors |= 1ULL << ((rank + 1) * 8 + (file - 1));
            if (file < 7) protectors |= 1ULL << ((rank + 1) * 8 + (file + 1));
        }

        // Bonus for each pawn in the chain
        if (protectors & ctx.board.pieces(PAWN, color)) {
            bonus += 15;
        }
    }

    return bonus;
}

int evaluatePawnShield(const PawnStructureContext &ctx, Color color) {
    int bonus = 0;

    Square kingSquare = ctx.board.KingSQ(color);
    int kingRank = square_rank(kingSquare);
    int kingFile = square_file(kingSquare);

    int shieldRank = (color == White) ? kingRank + 1 : kingRank - 1;

    if (shieldRank >= 0 && shieldRank < 8) {
        for (int df = -1; df <= 1; ++df) {
            int shieldFile = kingFile + df;
            if (shieldFile >= 0 && shieldFile < 8) {
                Square shieldSquare = file_rank_square(File(shieldFile), Rank(shieldRank));
                if (ctx.board.pieceAtB(shieldSquare) == makePiece(PAWN, color)) {
                    int center_bonus = (shieldFile == 3 || shieldFile == 4) ? 12 : 10;
                    bonus += center_bonus;

                    // Check killer moves
                    for (int ply = 0; ply < MAX_PLY; ++ply) {
                        if (killerMoves[ply][0] == shieldSquare || killerMoves[ply][1] == shieldSquare) {
                            bonus += 15;
                            break;
                        }
                    }

                    // Check history heuristic
                    int side = (color == White) ? 0 : 1;
                    if (historyTable[side][kingSquare][shieldSquare] > 0) {
                        bonus += 10;
                    }
                }
            }
        }
    }

    return bonus;
}


int evaluateBackwardPawns(const PawnStructureContext &ctx, Color color) {
    Bitboard pawns = (color == White) ? ctx.whitePawns : ctx.blackPawns;
    Bitboard enemyPawns = (color == White) ? ctx.blackPawns : ctx.whitePawns;
    int penalty = 0;

    while (pawns) {
        Square sq = static_cast<Square>(pop_lsb(pawns));
        int file = square_file(sq);
        int rank = square_rank(sq);

        // The square in front of the pawn
        int frontRank = (color == White) ? rank + 1 : rank - 1;
        if (frontRank < 0 || frontRank >= 8) continue;

        Square frontSquare = file_rank_square(File(file), Rank(frontRank));
        bool blocked = ctx.board.pieceAtB(frontSquare) != None; // Check if blocked

        // Check if the pawn is blocked by an enemy pawn
        Bitboard enemyAttacks = PawnAttacks(sq, ~color);
        bool controlled = enemyAttacks & (1ULL << frontSquare);

        // Check if the pawn is supported by another pawn
        bool supported = false;
        for (int df = -1; df <= 1; df += 2) {
            int adjFile = file + df;
            if (adjFile >= 0 && adjFile < 8) {
                Square supportSq = file_rank_square(File(adjFile), Rank(rank));
                if (ctx.board.pieceAtB(supportSq) == makePiece(PAWN, color)) {
                    supported = true;
                    break;
                }
            }
        }

        // If the pawn is blocked or controlled by an enemy pawn and not supported, apply penalty
        if (!supported && (blocked || controlled)) {
            penalty += 15;
        }
    }

    return penalty;
}

int evaluateHolesAndOutposts(const PawnStructureContext &ctx, Color color) {
    int bonus = 0;
    Color opponent = ~color;
    
    // Get important bitboards from context instead of recomputing
    Bitboard ourPawns = (color == White) ? ctx.whitePawns : ctx.blackPawns;
    Bitboard enemyPawns = (color == White) ? ctx.blackPawns : ctx.whitePawns;
    Bitboard ourPawnAttacks = (color == White) ? ctx.whitePawnAttacks : ctx.blackPawnAttacks;
    Bitboard enemyPawnAttacks = (color == White) ? ctx.blackPawnAttacks : ctx.whitePawnAttacks;
    
    // Get piece bitboards (only once)
    Bitboard ourKnights = ctx.board.pieces(KNIGHT, color);
    Bitboard ourBishops = ctx.board.pieces(BISHOP, color);
    Bitboard ourRooks = ctx.board.pieces(ROOK, color);
    Bitboard allPieces = ctx.board.All();
    
    // Use the precalculated enemy king position
    Square enemyKing = (color == White) ? ctx.blackKingSq : ctx.whiteKingSq;
    
    // Potential outposts are squares not attacked by enemy pawns
    Bitboard potentialOutposts = ~enemyPawnAttacks;
    
    // Outposts need to be supported by our pawns
    Bitboard supportedSquares = ourPawnAttacks;
    
    // Strong outposts are supported squares not attacked by enemy pawns
    Bitboard strongOutposts = potentialOutposts & supportedSquares & ~allPieces;
    
    // Define value constants (moved outside of loops)
    const int centerValue = 10;
    const int extendedCenterValue = 7;
    const int thirdRankValue = 5;
    const int fourthRankValue = 8;
    const int fifthRankValue = 12;
    const int sixthRankValue = 15;
    
    // Precompute center and extended center masks
    Bitboard centerMask = (1ULL << SQ_D4) | (1ULL << SQ_E4) | (1ULL << SQ_D5) | (1ULL << SQ_E5);
    Bitboard extendedCenterMask = 0ULL;
    for (int r = 2; r <= 5; ++r) {
        for (int f = 2; f <= 5; ++f) {
            if ((f == 3 || f == 4) && (r == 3 || r == 4)) continue; // Skip center squares
            extendedCenterMask |= 1ULL << (r * 8 + f);
        }
    }
    
    // Process knight outposts (most valuable)
    Bitboard outpostKnights = ourKnights & strongOutposts;
    while (outpostKnights) {
        Square sq = static_cast<Square>(pop_lsb(outpostKnights));
        int rank = square_rank(sq);
        int file = square_file(sq);
        int distance = square_distance(sq, enemyKing);
        
        // Base value for knight outpost
        int outpostValue = 18;
        
        // Rank value adjustments - use lookup array instead of conditionals
        const int rankValues[8] = {0, 0, 0, thirdRankValue, fourthRankValue, fifthRankValue, sixthRankValue, 0};
        rank = (color == White) ? rank : 7 - rank;
        if (rank >= 0 && rank < 8) {
            outpostValue += rankValues[rank];
        }
        
        // Use bitwise operations for center detection - much faster than conditionals
        if (centerMask & (1ULL << sq)) {
            outpostValue += centerValue;
        } else if (extendedCenterMask & (1ULL << sq)) {
            outpostValue += extendedCenterValue;
        }
        
        // Proximity to enemy king bonus - simplified calculation
        outpostValue += std::max(0, 7 - distance) * 3;
        
        // Calculate knight attacks once
        Bitboard knightAttacks = KnightAttacks(sq);
        outpostValue += popcount(knightAttacks & ctx.board.Enemy(color)) * 6;
        
        // Add history heuristic bonus (simplified)
        int side = (color == White) ? 0 : 1;
        int historyScore = historyTable[side][ctx.board.KingSQ(color)][sq];
        outpostValue += std::min(10, historyScore / 100);
        
        bonus += outpostValue;
    }
    
    // Process bishop outposts (also valuable) - similar optimizations as above
    Bitboard outpostBishops = ourBishops & strongOutposts;
    while (outpostBishops) {
        Square sq = static_cast<Square>(pop_lsb(outpostBishops));
        int rank = square_rank(sq);
        int file = square_file(sq);
        
        int outpostValue = 15;
        
        // Convert to relative rank once, use array lookup for values
        const int bishopRankValues[8] = {0, 0, 0, thirdRankValue-2, fourthRankValue-2, fifthRankValue-2, sixthRankValue-2, 0};
        rank = (color == White) ? rank : 7 - rank;
        if (rank >= 0 && rank < 8) {
            outpostValue += bishopRankValues[rank];
        }
        
        // Use bitwise operations for center detection
        if (centerMask & (1ULL << sq)) {
            outpostValue += centerValue - 2;
        } else if (extendedCenterMask & (1ULL << sq)) {
            outpostValue += extendedCenterValue - 2;
        }
        
        // Calculate bishop attacks once and reuse
        Bitboard bishopAttacks = BishopAttacks(sq, allPieces);
        outpostValue += popcount(bishopAttacks & ctx.board.Enemy(color)) * 4;
        
        bonus += outpostValue;
    }
    
    // Process rook outposts (less common but valuable on 7th rank)
    Bitboard outpostRooks = ourRooks & strongOutposts;
    while (outpostRooks) {
        Square sq = static_cast<Square>(pop_lsb(outpostRooks));
        int rank = (color == White) ? square_rank(sq) : 7 - square_rank(sq);
        
        // Rooks are especially strong on 7th rank - one simple comparison instead of multiple cases
        if (rank == 6) {
            bonus += 25;
            
            // Compute rook attacks once
            Bitboard rookAttacks = RookAttacks(sq, allPieces);
            bonus += popcount(rookAttacks & ctx.board.pieces(PAWN, opponent)) * 8;
        } else {
            bonus += 12;
        }
    }
    
    // Remaining potential outposts - simplified loop
    Bitboard remainingOutposts = strongOutposts & ~(outpostKnights | outpostBishops | outpostRooks);
    
    // Process remaining outposts more efficiently with lookup tables
    while (remainingOutposts) {
        Square sq = static_cast<Square>(pop_lsb(remainingOutposts));
        int rank = square_rank(sq);
        int file = square_file(sq);
        
        int outpostValue = 4; // Base value
        
        // Use relative rank consistently
        rank = (color == White) ? rank : 7 - rank;
        outpostValue += (rank >= 4) ? ((rank - 3) * 2) : 0;
        
        // Use precomputed masks for center regions
        if (centerMask & (1ULL << sq)) {
            outpostValue += 3;
        } else if (extendedCenterMask & (1ULL << sq)) {
            outpostValue += 2;
        }
        
        // Enemy king proximity bonus
        outpostValue += (square_distance(sq, enemyKing) <= 3) ? 2 : 0;
        
        // Check killer moves for tactical importance (simplified loop)
        for (int ply = 0; ply < std::min(MAX_PLY, 10); ++ply) {
            if (killerMoves[ply][0] == sq || killerMoves[ply][1] == sq) {
                outpostValue += 3;
                break;
            }
        }
        
        bonus += outpostValue;
    }
    
    // Evaluate holes in our position (squares that cannot be defended by our pawns)
    // Use our half of the board based on color
    Bitboard ourHalf = (color == White) ? 
        (MASK_RANK[2] | MASK_RANK[3] | MASK_RANK[4]) :
        (MASK_RANK[3] | MASK_RANK[4] | MASK_RANK[5]);
    
    // Holes: squares on our half that can't be defended by pawns and are empty
    Bitboard holes = ~ourPawnAttacks & ourHalf & ~allPieces;
    
    // Penalize important holes more efficiently
    Bitboard centralHoles = holes & centerMask;
    Bitboard extendedCentralHoles = holes & extendedCenterMask;
    
    // Apply fixed penalty for central holes (avoid looping again)
    bonus -= popcount(centralHoles) * 5;
    bonus -= popcount(extendedCentralHoles) * 3;
    
    // Check if enemy knights can reach these holes (simplified)
    if (ctx.board.pieces(KNIGHT, opponent)) {
        // Knights are the most threatening piece for holes
        // This is a simplification - we're not checking individual knight moves
        bonus -= popcount(centralHoles) * 2;
    }
    
    return bonus;
}

int evaluatePawnLeverThreats(const PawnStructureContext &ctx, Color color) {
    int bonus = 0;

    Bitboard pawns = (color == White) ? ctx.whitePawns : ctx.blackPawns;
    Bitboard enemyPawns = (color == White) ? ctx.blackPawns : ctx.whitePawns;

    // Create a mutable copy of the board to avoid const issues
    Board &mutableBoard = const_cast<Board &>(ctx.board);

    while (pawns) {
        Square from = static_cast<Square>(pop_lsb(pawns));

        // Check if the pawn is supported by another pawn
        if (PawnAttacks(from, color) & ctx.board.pieces(PAWN, color)) {
            bonus += 5; 
        }

        // Check if the pawn is attacked and not supported
        if (!(PawnAttacks(from, color) & ctx.board.pieces(PAWN, color)) &&
            mutableBoard.attackersForSide(~color, from, mutableBoard.All())) {
            bonus -= 3; // Penalty for being attacked without support
        }

        Bitboard attacks = PawnAttacks(from, color) & enemyPawns;

        // Check if the pawn can attack enemy pawns
        while (attacks) {
            Square target = static_cast<Square>(pop_lsb(attacks));

            // Check if the pawn is on the edge files (A or H)
            int file = square_file(target);
            if (file == 0 || file == 7) {
                bonus -= 2; 
            }

            // Check if the pawn is on center squares (D4, E4, D5, E5)
            if (file >= 2 && file <= 5) {
                bonus += 2; 
            }

            bonus += 10; // Bonus for each Pawn lever threat

            // Check killer move
            for (int ply = 0; ply < std::min(MAX_PLY, 10); ++ply) {
                if (killerMoves[ply][0] == target || killerMoves[ply][1] == target) {
                    bonus += 5;
                    break;
                }
            }

            // Check history move
            int side = (color == White) ? 0 : 1;
            if (historyTable[side][ctx.board.KingSQ(color)][target] > 0) {
                bonus += 5;
            }

            // Calculate point follow MVV-LVA
            Piece victim = ctx.board.pieceAtB(target);
            if (victim != None && victim != PAWN) {
                bonus += mvv_lva[PAWN][victim];
            }
        }
    }

    return bonus;
}

int evaluateFileOpenness(const PawnStructureContext &ctx, Color color) {
    int bonus = 0;

    for (int file = 0; file < 8; ++file) {
        Bitboard fileMask = MASK_FILE[file];
        Bitboard pawnsInFile = ctx.board.pieces(PAWN, color) & fileMask;
        Bitboard enemyPawnsInFile = ctx.board.pieces(PAWN, ~color) & fileMask;

        // Get all rooks and queens in the file
        Bitboard rooks = ctx.board.pieces(ROOK, color) & fileMask;
        Bitboard queens = ctx.board.pieces(QUEEN, color) & fileMask;

        // Check OpenFile
        if (!pawnsInFile && !enemyPawnsInFile) {
            bonus += 10; 

            // Bonus if rooks or queens are on the open file
            if (rooks || queens) {
                bonus += 10;
            }

            // Bonus if the open file is a central file (d or e)
            if (file == 3 || file == 4) {
                bonus += 5;
            }

            // Check intrude if the file is open
            if ((rooks | queens) && color == White && (MASK_RANK[6] & fileMask || MASK_RANK[7] & fileMask)) {
                bonus += 15; // Bonus if the open file leads to rank 7 or 8
            } else if ((rooks | queens) && color == Black && (MASK_RANK[1] & fileMask || MASK_RANK[0] & fileMask)) {
                bonus += 15; // Bonus if the open file leads to rank 2 or 1
            }
        }
        // Check semi-OpenFile
        else if (!pawnsInFile || !enemyPawnsInFile) {
            bonus += 5; 

            // Bonus if rooks or queens are on the semi-open file
            if (rooks || queens) {
                bonus += 5;
            }

            // Bonus if the semi-open file is a central file (d or e)
            if (file == 3 || file == 4) {
                bonus += 3;
            }
        }

        // Penalty for blocked pawns in the file
        if (pawnsInFile && !(rooks || queens)) {
            bonus -= 5;
        }
    }

    return bonus;
}

int evaluateSpaceAdvantage(const PawnStructureContext &ctx, Color color) {
    int bonus = 0;

    // Identify the opponent's half of the board
    Bitboard opponentHalf = (color == White) ? MASK_RANK[4] | MASK_RANK[5] | MASK_RANK[6] | MASK_RANK[7]
                                             : MASK_RANK[0] | MASK_RANK[1] | MASK_RANK[2] | MASK_RANK[3];

    // Identify the center squares (D4, E4, D5, E5)
    Bitboard centerSquares = (1ULL << SQ_D4) | (1ULL << SQ_D5) | (1ULL << SQ_E4) | (1ULL << SQ_E5);

    // Calculate all pieces on the board
    Bitboard allPieces = 0ULL;
    for (int sq = 0; sq < 64; ++sq) {
        if (ctx.board.pieceAtB(static_cast<Square>(sq)) != None) {
            allPieces |= (1ULL << sq);
        }
    }
    Bitboard empty = ~allPieces;

    // Calculate controlled squares by pawns
    Bitboard controlledSquares = 0ULL;
    for (int sq = 0; sq < 64; ++sq) {
        Square square = static_cast<Square>(sq);
        if (ctx.board.pieceAtB(square) == makePiece(PAWN, color)) {
            controlledSquares |= PawnAttacks(square, color);
        }
    }

    // Calculate the number of squares controlled by pawns in the opponent's half
    bonus += 2 * popcount(controlledSquares & opponentHalf & empty);

    // Calculate the number of squares controlled by pawns in the center
    bonus += 3 * popcount(controlledSquares & centerSquares);

    // Integrated killerMoves
    for (int ply = 0; ply < MAX_PLY; ++ply) {
        if (killerMoves[ply][0] & controlledSquares || killerMoves[ply][1] & controlledSquares) {
            bonus += 5; 
            break;
        }
    }

    // Integrated historyTable
    int side = (color == White) ? 0 : 1;
    for (int sq = 0; sq < 64; ++sq) {
        if (controlledSquares & (1ULL << sq) && historyTable[side][ctx.board.KingSQ(color)][sq] > 0) {
            bonus += 3; 
        }
    }

    // Integrated MVV-LVA
    for (int sq = 0; sq < 64; ++sq) {
        if (controlledSquares & (1ULL << sq)) {
            Piece victim = ctx.board.pieceAtB(static_cast<Square>(sq));
            if (victim != None && victim != PAWN) {
                bonus += mvv_lva[PAWN][victim];
            }
        }
    }

    return bonus;
}

int evaluatePawnMajority(const PawnStructureContext &ctx, Color color) {
    int bonus = 0;

    // Identify the pawns in king side and queen side
    Bitboard pawns = (color == White) ? ctx.whitePawns : ctx.blackPawns;
    Bitboard kingSide = MASK_FILE[5] | MASK_FILE[6] | MASK_FILE[7]; // Cột f, g, h
    Bitboard queenSide = MASK_FILE[0] | MASK_FILE[1] | MASK_FILE[2]; // Cột a, b, c

    int kingSidePawns = popcount(pawns & kingSide);
    int queenSidePawns = popcount(pawns & queenSide);

    // Calculate the majority of pawns
    if (kingSidePawns > queenSidePawns) {
        bonus += 10; // king side majority
    } else if (queenSidePawns > kingSidePawns) {
        bonus += 10; // queen side majority
    }

    // Bonus for having pawns on the 4th rank or higher
    while (pawns) {
        Square sq = static_cast<Square>(pop_lsb(pawns));
        int rank = square_rank(sq);

        int advancement = (color == White) ? rank : (7 - rank);
        bonus += advancement * 2;

        // Penalty for pawns on the 2nd rank or lower
        Square frontSquare = (color == White) ? static_cast<Square>(sq + 8) : static_cast<Square>(sq - 8);
        if (ctx.board.pieceAtB(frontSquare) != None) {
            bonus -= 5; // Phạt nếu quân tốt bị cản trở
        }
    }

    // Integrate killerMoves
    for (int ply = 0; ply < MAX_PLY; ++ply) {
        if (killerMoves[ply][0] & pawns || killerMoves[ply][1] & pawns) {
            bonus += 5; 
            break;
        }
    }

    // Integrate historyTable
    int side = (color == White) ? 0 : 1;
    for (int sq = 0; sq < 64; ++sq) {
        if (pawns & (1ULL << sq) && historyTable[side][ctx.board.KingSQ(color)][sq] > 0) {
            bonus += 3; 
        }
    }

    return bonus;
}

int evaluatePawnStorm(const PawnStructureContext &ctx, Color color) {
    int bonus = 0;

    // Identify the opponent's king square
    Square enemyKingSquare = ctx.board.KingSQ(~color);
    int enemyKingFile = square_file(enemyKingSquare);

    // Identify the pawns of the current color
    Bitboard pawns = (color == White) ? ctx.whitePawns : ctx.blackPawns;

    // Calculate the small distance of each pawn to the opponent's king
    while (pawns) {
        Square sq = static_cast<Square>(pop_lsb(pawns));
        int distance = square_distance(sq, enemyKingSquare);
        int pawnFile = square_file(sq);

        // Bonus for being close to the opponent's king
        if (distance <= 2) {
            bonus += 10; 
        } else if (distance <= 4) {
            bonus += 5; // smaller than the average distance
        }

        // Bonus for the pawn on the same file as the opponent's king
        if ((pawnFile >= 4 && enemyKingFile >= 4) || (pawnFile <= 3 && enemyKingFile <= 3)) {
            bonus += 3; 
        }

        // Penalty for the blocked pawns
        Square frontSquare = (color == White) ? static_cast<Square>(sq + 8) : static_cast<Square>(sq - 8);
        if (ctx.board.pieceAtB(frontSquare) != None) {
            bonus -= 5; 
        }
    }

    // Integrate killerMoves
    for (int ply = 0; ply < MAX_PLY; ++ply) {
        if (killerMoves[ply][0] & pawns || killerMoves[ply][1] & pawns) {
            bonus += 5; 
            break;
        }
    }

    // Integrate historyTable
    int side = (color == White) ? 0 : 1;
    for (int sq = 0; sq < 64; ++sq) {
        if (pawns & (1ULL << sq) && historyTable[side][ctx.board.KingSQ(color)][sq] > 0) {
            bonus += 3;
        }
    }

    return bonus;
}

int evaluateMobility(const Board &board, Color color) {
    int mobility = 0;

    // Knights
    Bitboard knights = board.pieces(KNIGHT, color);
    while (knights) {
        Square sq = poplsb(knights);
        U64 attacks = KnightAttacks(sq) & ~board.Us(color); // Exclude friendly pieces
        mobility += popcount(attacks) * 3;                 // Weight: 3
        mobility += popcount(attacks & board.Enemy(color)) * 2; // Bonus for attacking enemy pieces
    }

    // Bishops
    Bitboard bishops = board.pieces(BISHOP, color);
    while (bishops) {
        Square sq = poplsb(bishops);
        U64 attacks = BishopAttacks(sq, board.occAll) & ~board.Us(color); // Exclude friendly pieces
        mobility += popcount(attacks) * 3;                               // Weight: 3
        mobility += popcount(attacks & board.Enemy(color)) * 2;          // Bonus for attacking enemy pieces
    }

    // Rooks
    Bitboard rooks = board.pieces(ROOK, color);
    while (rooks) {
        Square sq = poplsb(rooks);
        U64 attacks = RookAttacks(sq, board.occAll) & ~board.Us(color); // Exclude friendly pieces
        mobility += popcount(attacks) * 5;                             // Weight: 5
        mobility += popcount(attacks & board.Enemy(color)) * 2;        // Bonus for attacking enemy pieces
    }

    // Queens
    Bitboard queens = board.pieces(QUEEN, color);
    while (queens) {
        Square sq = poplsb(queens);
        U64 attacks = QueenAttacks(sq, board.occAll) & ~board.Us(color); // Exclude friendly pieces
        mobility += popcount(attacks) * 9;                              // Weight: 9
        mobility += popcount(attacks & board.Enemy(color)) * 2;         // Bonus for attacking enemy pieces
    }

    // // King
    // Bitboard king = board.pieces(KING, color);
    // if (king) {
    //     Square sq = poplsb(king);
    //     U64 attacks = KingAttacks(sq) & ~board.Us(color); // Exclude friendly pieces
    //     mobility += popcount(attacks) * 2;               // Weight: 2
 
    //     // Penalize unsafe king moves (attacks on squares controlled by the enemy)
    //     mobility -= popcount(attacks & board.Enemy(color)) * 3;
    // }

    // Penalize blocked pieces (pieces with very low mobility)
    if (mobility < 5) {
        mobility -= 5; // Apply a penalty for very low mobility
    }

    // Scale mobility by game phase
    float endgameWeight = getGamePhase(board); // 0.0 in opening, 1.0 in endgame
    float middlegameWeight = 1.0f - endgameWeight;

    return static_cast<int>(mobility * middlegameWeight);
}

void updateMobility(Board &board, Move move, int &mobilityScore, Color color) {
    Square fromS = from(move);
    Square toS = to(move);
    Piece movedPiece = board.pieceAtB(fromS);
    Piece capturedPiece = board.pieceAtB(toS);

    // Subtract mobility of the piece from its old square
    mobilityScore -= popcount(board.attacksByPiece(type_of_piece(movedPiece), fromS, color) & ~board.Us(color));

    // If a piece is captured, subtract its mobility
    if (capturedPiece != None) {
        mobilityScore -= popcount(board.attacksByPiece(type_of_piece(capturedPiece), toS, ~color) & ~board.Us(~color));
    }

    // Update the board state for the move
    board.makeMove(move);

    // Add mobility of the piece in its new square
    mobilityScore += popcount(board.attacksByPiece(type_of_piece(movedPiece), toS, color) & ~board.Us(color));

    // Restore the board state (if needed for further calculations)
    board.unmakeMove(move);
}

int evaluateKingSafety(const Board &board, Color color) {
    // Create a mutable copy of the board
    Board mutableBoard = board;

    Square kingSquare = mutableBoard.KingSQ(color);
    U64 kingZone = KingAttacks(kingSquare) | (1ULL << kingSquare); // King zone includes the king's square

    // Use the mutable copy to call attackersForSide
    U64 enemyAttacks = mutableBoard.attackersForSide(~color, kingSquare, mutableBoard.occAll);

    int penalty = popcount(kingZone & enemyAttacks) * 10; // Penalty for each attacked square
    return -penalty;
}

int evaluateKingMobility(const Board &board, Color color) {
    float endgameWeight = getGamePhase(board); // 0.0 in opening, 1.0 in endgame
    float middlegameWeight = 1.0f - endgameWeight;
    
    // Get the king's square
    Square kingSquare = board.KingSQ(color);

    // Calculate the legal moves for the king
    U64 kingMoves = KingAttacks(kingSquare) & ~board.Us(color); // Exclude friendly pieces

    // Reward for each legal move
    int mobilityScore = popcount(kingMoves) * 5 * endgameWeight; // Reward mobility in the endgame

    // Penalize unsafe king moves (squares attacked by enemy pieces)
    U64 unsafeMoves = kingMoves & board.Enemy(color);
    mobilityScore -= popcount(unsafeMoves) * 3 * middlegameWeight; // Penalize unsafe moves in the middlegame

    return mobilityScore;
}

int evaluateKingOpenFiles(const Board &board, Color color) {
    Square kingSquare = board.KingSQ(color);
    int file = square_file(kingSquare);

    // Check if there are pawns in the king's file
    Bitboard pawnsInFile = board.pieces(PAWN, color) & MASK_FILE[file];
    if (!pawnsInFile) {
        return -20; // Penalty for king on an open file
    }

    // Check if there are enemy pawns in the king's file
    Bitboard enemyPawnsInFile = board.pieces(PAWN, ~color) & MASK_FILE[file];
    if (!enemyPawnsInFile) {
        return -10; // Smaller penalty for king on a semi-open file
    }

    return 0; // No penalty if the file is not open or semi-open
}

int evaluateQueenControlAndCheckmatePotential(const Board &board, Color color) {
    int score = 0;
    Color opponent = ~color;
    
    Bitboard ourQueens = board.pieces(QUEEN, color);
    if (!ourQueens) return 0; // Early exit if no queens
    
    Bitboard enemyKing = board.pieces(KING, opponent);
    Square enemyKingSq = board.KingSQ(opponent);
    
    Board &mutableBoard = const_cast<Board &>(board);
    
    Bitboard allPieces = board.All();
    Bitboard ourPieces = board.Us(color);
    Bitboard enemyPieces = board.Enemy(color);
    
    // Calculate the king's attacks and zone
    Bitboard kingAttacks = KingAttacks(enemyKingSq);
    Bitboard kingZone = kingAttacks | (1ULL << enemyKingSq);
    
    // Store the center squares (D4, E4, D5, E5)
    Bitboard centerSquares = (1ULL << SQ_D4) | (1ULL << SQ_E4) | (1ULL << SQ_D5) | (1ULL << SQ_E5);
    
    // Analyze each queen
    while (ourQueens) {
        Square queenSq = static_cast<Square>(pop_lsb(ourQueens));
        
        // 1. Queen's control over the board
        Bitboard queenAttacks = QueenAttacks(queenSq, allPieces) & ~ourPieces;
        
        // 2-3. Queen's attack on enemy pieces
        int attackBonus = 0;
        attackBonus += popcount(queenAttacks & enemyPieces) * 5;          // Attack enemy pieces
        attackBonus += popcount(queenAttacks & kingZone) * 10;            // Attack enemy king zone
        attackBonus += ((queenAttacks & centerSquares) ? 15 : 0);         // Center control bonus
        score += popcount(queenAttacks) * 2 + attackBonus;                // Basic attack bonus
        
        // 4. Analyze checkmate potential
        int distance = square_distance(queenSq, enemyKingSq);
        if (distance <= 2) {
            score += (3 - distance) * 15; // Closer to the enemy king = more potential
            
            // Check if the queen attacks the enemy king
            if (queenAttacks & enemyKing) {
                score += 30;
                
                // Check if the enemy king has any legal moves
                Bitboard kingMoves = kingAttacks & ~board.Us(opponent);
                
                // Check if the enemy king has any safe moves
                Bitboard safeKingMoves = kingMoves;
                Bitboard tempKingMoves = kingMoves;
                
                while (tempKingMoves) {
                    Square kingMoveSq = static_cast<Square>(pop_lsb(tempKingMoves));
                    
                    // Check if the move is attacked by our pieces
                    if (mutableBoard.attackersForSide(color, kingMoveSq, allPieces)) {
                        safeKingMoves &= ~(1ULL << kingMoveSq);
                    }
                }
                
                // Check if the enemy king has no any safe moves
                if (safeKingMoves == 0 && mutableBoard.attackersForSide(color, enemyKingSq, allPieces)) {
                    score += 200; // Checkmate potential bonus
                }
            }
        }
        
        // Priority for the queen's safety
        if (!mutableBoard.attackersForSide(opponent, queenSq, allPieces)) {
            score += 10;
        }
        
        // Check killer moves + History heuristic
        int side = (color == White) ? 0 : 1;
        bool killerBonus = false;
        
        for (int ply = 0; ply < std::min(MAX_PLY, 10) && !killerBonus; ++ply) {
            if (killerMoves[ply][0] == queenSq || killerMoves[ply][1] == queenSq) {
                score += 8;
                killerBonus = true;
            }
        }
        
        if (historyTable[side][board.KingSQ(color)][queenSq] > 0) {
            score += 5;
        }
    }
    
    return score;
}

int evaluateCastlingAbility(const Board &board, Color color) {
    int score = 0;
    Color opponent = ~color;
    
    // Judge castling
    unsigned castlingRights = board.castlingRights;
    
    // Identify the castling rights for the current color
    bool hasKingsideCastling = false;
    bool hasQueensideCastling = false;
    
    if (color == White) {
        hasKingsideCastling = (castlingRights & 1); // WHITE_OO (0001)
        hasQueensideCastling = (castlingRights & 2); // WHITE_OOO (0010)
    } else {
        hasKingsideCastling = (castlingRights & 4); // BLACK_OO (0100)
        hasQueensideCastling = (castlingRights & 8); // BLACK_OOO (1000)
    }
    
    // Bonus basic point for castling rights
    if (hasKingsideCastling) score += 15;
    if (hasQueensideCastling) score += 10;
    
    // Check if the king is already castled
    Square kingPos = board.KingSQ(color);
    int kingFile = square_file(kingPos);
    int kingRank = square_rank(kingPos);
    int expectedRank = (color == White) ? 0 : 7;
    int castledBonus = 0;
    
    // Check if the king is castled
    if (kingRank == expectedRank) {
        if (kingFile == 6) {      // G-file (castled kingside)
            castledBonus = 40;
        } 
        else if (kingFile == 2) { // C-file (castled queenside)
            castledBonus = 35;
        }
    }
    
    // Check path for castling if not castled yet
    if (castledBonus == 0 && (hasKingsideCastling || hasQueensideCastling)) {
        
        Board &mutableBoard = const_cast<Board &>(board);
        Bitboard allPieces = board.All();
        
        // Store the squares for castling path
        Square squareB = file_rank_square(File(FILE_B), Rank(expectedRank));
        Square squareC = file_rank_square(File(FILE_C), Rank(expectedRank));
        Square squareD = file_rank_square(File(FILE_D), Rank(expectedRank));
        Square squareF = file_rank_square(File(FILE_F), Rank(expectedRank));
        Square squareG = file_rank_square(File(FILE_G), Rank(expectedRank));
        
        // Check path for castling kingside
        if (hasKingsideCastling) {
            bool pathClear = (board.pieceAtB(squareF) == None) && (board.pieceAtB(squareG) == None);
            
            if (pathClear) {
                // Combined check for squares F and G
                // Check if the squares F and G are attacked by the opponent
                bool pathUnderAttack = mutableBoard.attackersForSide(opponent, squareF, allPieces) || 
                                      mutableBoard.attackersForSide(opponent, squareG, allPieces);
                
                if (!pathUnderAttack) {
                    score += 20; // Safe path
                } else {
                    score -= 5;  // Path under attack
                }
            } else {
                score -= 8; // Path blocked
            }
        }
        
        // Check path for castling queenside
        if (hasQueensideCastling) {
            bool pathClear = (board.pieceAtB(squareB) == None) && 
                            (board.pieceAtB(squareC) == None) &&
                            (board.pieceAtB(squareD) == None);
            
            if (pathClear) {
                // Check if the squares B, C, and D are attacked by the opponent
                bool pathUnderAttack = mutableBoard.attackersForSide(opponent, squareC, allPieces) || 
                                      mutableBoard.attackersForSide(opponent, squareD, allPieces);
                
                if (!pathUnderAttack) {
                    score += 15; // Safe path
                } else {
                    score -= 5;  // Path under attack
                }
            } else {
                score -= 8; // Path blocked
            }
        }
    }
    
    // Modify the score based on the game phase
    float gamePhase = getGamePhase(board);
    
    if (gamePhase > 0.5) { // Endgame phase
        castledBonus = static_cast<int>(castledBonus * (1.0 - gamePhase));
    }
    
    score += castledBonus;
    
    // Analyze the advantage after castling
    if (castledBonus > 0) {
        Bitboard rooks = board.pieces(ROOK, color);
        
        // Check squares for castling path
        if (kingFile == 6) {
            // Check rook h has connection with the king
            Square squareH = file_rank_square(File(FILE_H), Rank(expectedRank));
            if (rooks & (1ULL << squareH)) {
                score += 5;
            }
            
            // Check rook of file pawns
            Square squareE = file_rank_square(File(FILE_E), Rank(expectedRank));
            Square squareF = file_rank_square(File(FILE_F), Rank(expectedRank));
            if (rooks & ((1ULL << squareE) | (1ULL << squareF))) {
                score += 15; // Rook on file pawns after castling
            }
        }
        else if (kingFile == 2) { 
            // Check rook a has connection with the king
            Square squareA = file_rank_square(File(FILE_A), Rank(expectedRank));
            if (rooks & (1ULL << squareA)) {
                score += 5;
            }
            
            // Check rook of file pawns
            Square squareD = file_rank_square(File(FILE_D), Rank(expectedRank));
            Square squareE = file_rank_square(File(FILE_E), Rank(expectedRank));
            if (rooks & ((1ULL << squareD) | (1ULL << squareE))) {
                score += 15; // Rook on file pawns after castling
            }
        }
    }
    return score;
}