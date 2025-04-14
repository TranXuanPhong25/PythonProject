#include "evaluate_features.hpp"

using namespace Chess;


// Pawn Structure: isolated, doubled, passed pawns (basic skeleton)
int evaluatePawnStructure(const Board &board) {
    int score = 0;
    score -= evaluateDoubledPawns(board, White);
    score += evaluateDoubledPawns(board, Black);

    score -= evaluateIsolatedPawns(board, White);
    score += evaluateIsolatedPawns(board, Black);

    score += evaluatePassedPawns(board, White);
    score -= evaluatePassedPawns(board, Black);

    score += evaluatePassedPawnSupport(board, White);
    score -= evaluatePassedPawnSupport(board, Black);

    score += evaluateConnectedPawns(board, White);
    score -= evaluateConnectedPawns(board, Black);

    score += evaluatePhalanxPawns(board, White);
    score -= evaluatePhalanxPawns(board, Black);

    score += evaluateBlockedPawns(board, White);
    score -= evaluateBlockedPawns(board, Black);

    score += evaluatePawnChains(board, White);
    score -= evaluatePawnChains(board, Black);

    score += evaluatePawnShield(board, White);
    score -= evaluatePawnShield(board, Black);

    score += evaluateBackwardPawns(board, White);
    score -= evaluateBackwardPawns(board, Black);

    score += evaluateHolesAndOutposts(board, White);
    score -= evaluateHolesAndOutposts(board, Black);

    score += evaluatePawnLeverThreats(board, White);
    score -= evaluatePawnLeverThreats(board, Black);

    score += evaluateFileOpenness(board, White);
    score -= evaluateFileOpenness(board, Black);

    score += evaluateSpaceAdvantage(board, White);
    score -= evaluateSpaceAdvantage(board, Black);

    score += evaluatePawnMajority(board, White);
    score -= evaluatePawnMajority(board, Black);

    score += evaluatePawnStorm(board, White);
    score -= evaluatePawnStorm(board, Black);

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


int evaluateDoubledPawns(const Board &board, Color color) {
    int penalty = 0;
    // Get all pawns of the specified color
    Bitboard pawns = board.pieces(PAWN, color);

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


int evaluateIsolatedPawns(const Board &board, Color color) {
    Bitboard pawns = board.pieces(PAWN, color);
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


int evaluatePassedPawns(const Board &board, Color color) {
    Bitboard pawns = board.pieces(PAWN, color);
    Bitboard enemyPawns = board.pieces(PAWN, ~color);
    int bonus = 0;

    while (pawns) {
        Square sq = static_cast<Square>(pop_lsb(pawns));
        int file = square_file(sq);
        int rank = square_rank(sq);
        
        Bitboard blockerZone = 0;

        for (int df = -1; df <= 1; ++df) {
            int f = file + df;
            if (f < 0 || f > 7) continue;

            if (color == White) {
                for (int r = rank + 1; r <= 7; ++r)
                    blockerZone |= (1ULL << (r * 8 + f));
            } else {
                for (int r = rank - 1; r >= 0; --r)
                    blockerZone |= (1ULL << (r * 8 + f));
            }
        }

        if ((blockerZone & enemyPawns) == 0) {
            int advancement = (color == White) ? rank : (7 - rank);
            bonus += advancement * 5 + 10;

            // Bonus point for queens-pawn
            if (advancement >= 5)
                bonus += 10;
                
            // Check if the pawn is part of a killer move
            for (int ply = 0; ply < MAX_PLY; ++ply) {
                if (killerMoves[ply][0] == sq || killerMoves[ply][1] == sq) {
                    bonus += 20; // Bonus if part of a killer move
                    break;
                }
            }
        }
    }

    return bonus;
}

int evaluatePassedPawnSupport(const Board &board, Color color) {
    Bitboard pawns = board.pieces(PAWN, color);
    int bonus = 0;

    // Create a mutable copy of the board to avoid const issues
    Board &mutableBoard = const_cast<Board &>(board);

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

int evaluateConnectedPawns(const Board &board, Color color) {
    Bitboard pawns = board.pieces(PAWN, color);
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

        if (connected & board.pieces(PAWN, color)) {
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

int evaluatePhalanxPawns(const Board &board, Color color) {
    Bitboard pawns = board.pieces(PAWN, color);
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

int evaluateBlockedPawns(const Board &board, Color color) {
    Bitboard pawns = board.pieces(PAWN, color);
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

int evaluatePawnChains(const Board &board, Color color) {
    Bitboard pawns = board.pieces(PAWN, color);
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
        if (protectors & board.pieces(PAWN, color)) {
            bonus += 15;
        }
    }

    return bonus;
}

int evaluatePawnShield(const Board &board, Color color) {
    int bonus = 0;

    Square kingSquare = board.KingSQ(color);
    int kingRank = square_rank(kingSquare);
    int kingFile = square_file(kingSquare);

    int shieldRank = (color == White) ? kingRank + 1 : kingRank - 1;

    if (shieldRank >= 0 && shieldRank < 8) {
        for (int df = -1; df <= 1; ++df) {
            int shieldFile = kingFile + df;
            if (shieldFile >= 0 && shieldFile < 8) {
                Square shieldSquare = file_rank_square(File(shieldFile), Rank(shieldRank));
                if (board.pieceAtB(shieldSquare) == makePiece(PAWN, color)) {
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


int evaluateBackwardPawns(const Board &board, Color color) {
    Bitboard pawns = board.pieces(PAWN, color);
    Bitboard enemyPawns = board.pieces(PAWN, ~color);
    int penalty = 0;

    while (pawns) {
        Square sq = static_cast<Square>(pop_lsb(pawns));
        int file = square_file(sq);
        int rank = square_rank(sq);

        // The square in front of the pawn
        int frontRank = (color == White) ? rank + 1 : rank - 1;
        if (frontRank < 0 || frontRank >= 8) continue;

        Square frontSquare = file_rank_square(File(file), Rank(frontRank));
        bool blocked = board.pieceAtB(frontSquare) != None; // Check if blocked

        // Check if the pawn is blocked by an enemy pawn
        Bitboard enemyAttacks = PawnAttacks(sq, ~color);
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

        // If the pawn is blocked or controlled by an enemy pawn and not supported, apply penalty
        if (!supported && (blocked || controlled)) {
            penalty += 15;
        }
    }

    return penalty;
}

int evaluateHolesAndOutposts(const Board &board, Color color) {
    int bonus = 0;

    // Tính toán các ô trống bằng cách lấy tất cả các ô và loại bỏ các ô bị chiếm
    Bitboard allPieces = board.All();
    Bitboard emptySquares = ~allPieces;

    // Duyệt qua từng ô trên bàn cờ
    for (int sq = 0; sq < 64; ++sq) {
        if (!(emptySquares & (1ULL << sq))) continue;

        Square square = static_cast<Square>(sq);
        int rank = square_rank(square);
        int file = square_file(square);

        // Nếu ô không bị kiểm soát bởi tốt đối phương => đây có thể là một tiền đồn
        if (!(PawnAttacks(square, ~color) & board.pieces(PAWN, ~color))) {
            bonus += 5;

            // Nếu ô nằm ở trung tâm (16 ô trung tâm)
            if (rank >= 2 && rank <= 5 && file >= 2 && file <= 5) {
                bonus += 5;
            }

            // Kiểm tra nếu ô này liên quan đến một killer move
            for (int ply = 0; ply < MAX_PLY; ++ply) {
                if (killerMoves[ply][0] == sq || killerMoves[ply][1] == sq) {
                    bonus += 10;
                    break;
                }
            }

            // Kiểm tra nếu ô này liên quan đến một nước đi tốt trong lịch sử
            int side = (color == White) ? 0 : 1;
            if (historyTable[side][board.KingSQ(color)][sq] > 0) {
                bonus += 10;
            }
        }
    }

    return bonus;
}

int evaluatePawnLeverThreats(const Board &board, Color color) {
    int bonus = 0;

    Bitboard pawns = board.pieces(PAWN, color);
    Bitboard enemyPawns = board.pieces(PAWN, ~color);

    // Create a mutable copy of the board to avoid const issues
    Board &mutableBoard = const_cast<Board &>(board);

    while (pawns) {
        Square from = static_cast<Square>(pop_lsb(pawns));

        // Check if the pawn is supported by another pawn
        if (PawnAttacks(from, color) & board.pieces(PAWN, color)) {
            bonus += 5; 
        }

        // Check if the pawn is attacked and not supported
        if (!(PawnAttacks(from, color) & board.pieces(PAWN, color)) &&
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
            if (historyTable[side][board.KingSQ(color)][target] > 0) {
                bonus += 5;
            }

            // Calculate point follow MVV-LVA
            Piece victim = board.pieceAtB(target);
            if (victim != None && victim != PAWN) {
                bonus += mvv_lva[PAWN][victim];
            }
        }
    }

    return bonus;
}

int evaluateFileOpenness(const Board &board, Color color) {
    int bonus = 0;

    for (int file = 0; file < 8; ++file) {
        Bitboard fileMask = MASK_FILE[file];
        Bitboard pawnsInFile = board.pieces(PAWN, color) & fileMask;
        Bitboard enemyPawnsInFile = board.pieces(PAWN, ~color) & fileMask;

        // Get all rooks and queens in the file
        Bitboard rooks = board.pieces(ROOK, color) & fileMask;
        Bitboard queens = board.pieces(QUEEN, color) & fileMask;

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

int evaluateSpaceAdvantage(const Board &board, Color color) {
    int bonus = 0;

    // Identify the opponent's half of the board
    Bitboard opponentHalf = (color == White) ? MASK_RANK[4] | MASK_RANK[5] | MASK_RANK[6] | MASK_RANK[7]
                                             : MASK_RANK[0] | MASK_RANK[1] | MASK_RANK[2] | MASK_RANK[3];

    // Identify the center squares (D4, E4, D5, E5)
    Bitboard centerSquares = (1ULL << SQ_D4) | (1ULL << SQ_D5) | (1ULL << SQ_E4) | (1ULL << SQ_E5);

    // Calculate all pieces on the board
    Bitboard allPieces = 0ULL;
    for (int sq = 0; sq < 64; ++sq) {
        if (board.pieceAtB(static_cast<Square>(sq)) != None) {
            allPieces |= (1ULL << sq);
        }
    }
    Bitboard empty = ~allPieces;

    // Calculate controlled squares by pawns
    Bitboard controlledSquares = 0ULL;
    for (int sq = 0; sq < 64; ++sq) {
        Square square = static_cast<Square>(sq);
        if (board.pieceAtB(square) == makePiece(PAWN, color)) {
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
        if (controlledSquares & (1ULL << sq) && historyTable[side][board.KingSQ(color)][sq] > 0) {
            bonus += 3; 
        }
    }

    // Integrated MVV-LVA
    for (int sq = 0; sq < 64; ++sq) {
        if (controlledSquares & (1ULL << sq)) {
            Piece victim = board.pieceAtB(static_cast<Square>(sq));
            if (victim != None && victim != PAWN) {
                bonus += mvv_lva[PAWN][victim];
            }
        }
    }

    return bonus;
}

int evaluatePawnMajority(const Board &board, Color color) {
    int bonus = 0;

    // Identify the pawns in king side and queen side
    Bitboard pawns = board.pieces(PAWN, color);
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
        if (board.pieceAtB(frontSquare) != None) {
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
        if (pawns & (1ULL << sq) && historyTable[side][board.KingSQ(color)][sq] > 0) {
            bonus += 3; 
        }
    }

    return bonus;
}

int evaluatePawnStorm(const Board &board, Color color) {
    int bonus = 0;

    // Identify the opponent's king square
    Square enemyKingSquare = board.KingSQ(~color);
    int enemyKingFile = square_file(enemyKingSquare);

    // Identify the pawns of the current color
    Bitboard pawns = board.pieces(PAWN, color);

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
        if (board.pieceAtB(frontSquare) != None) {
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
        if (pawns & (1ULL << sq) && historyTable[side][board.KingSQ(color)][sq] > 0) {
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

    // Check if the king is in check and has no legal moves (checkmate)
    if (board.isSquareAttacked(~color, kingSquare)) {
        Movelist moves;
        if (color == White) {
            Movegen::legalmoves<White, Movetype::ALL>(mutableBoard, moves);
        } else {
            Movegen::legalmoves<Black, Movetype::ALL>(mutableBoard, moves);
        }
        if (moves.size == 0) {
            penalty += 10000; // Severe penalty for checkmate
        } else {
            penalty += 50; // Penalty for being in check
        }
    }

    // Additional penalty for double checks
    if (board.doubleCheck == 2) {
        penalty += 100; // Severe penalty for double check
    }

    // Check if the opponent's king is in check and has no legal moves (delivering checkmate)
    if (board.isSquareAttacked(color, kingSquare)) {
        Movelist moves;
        if (color == White) {
            Movegen::legalmoves<Black, Movetype::ALL>(mutableBoard, moves);
        } else {
            Movegen::legalmoves<White, Movetype::ALL>(mutableBoard, moves);
        }
        if (moves.size == 0) {
            penalty -= 10000; // High reward for delivering checkmate
        } else {
            penalty -= 50; // Penalty for checking the opponent
        }
    }

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