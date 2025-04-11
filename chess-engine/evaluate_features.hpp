#pragma once

#include "chess.hpp"
using namespace Chess;

int evaluatePawnStructure(const Board& board);
// int evaluateMobility(const Board& board);
// int evaluateKingSafety(const Board& board);
// int evaluateCenterControl(const Board& board);

int evaluateDoubledPawns(const Board &board, Color color);
int evaluateIsolatedPawns(const Board &board, Color color);
int evaluatePassedPawns(const Board &board, Color color);

// inline std::vector<Move> Board::generate_legal_moves() const {
//     std::vector<Move> moves;
//     //Using bitboards counting for creating list of legal moves
//     for (PieceType pt = PAWN; pt <= KING; ++pt) {
//         Bitboard pieces = this->pieces(pt, sideToMove); 

//         //Using pop_lsb to iterate through the pieces of the current type
//         while (pieces) {
//             Square sq = static_cast<Square>(pop_lsb(pieces)); // Take and remove LSB (Least Significant Bit)

//             // Generate moves for the piece at the square
//             std::vector<Move> pieceMoves = generate_moves_for_piece(pt, sq);
//             moves.insert(moves.end(), pieceMoves.begin(), pieceMoves.end());
//         }
//     }

//     // Lọc các nước đi để loại bỏ các nước khiến vua bị chiếu
//     std::vector<Move> legalMoves;
//     for (const Move& move : moves) {
//         Board tempBoard = *this; // Tạo một bản sao của bàn cờ
//         tempBoard.makeMove(move);
//         if (!isKingInCheck(sideToMove)) {
//             legalMoves.push_back(move);
//         }
//         tempBoard.unmakeMove(move);
//     }

//     return legalMoves;
//  }

//  inline bool Board::isKingInCheck(Color color) const {
//     Square kingSquare = KingSQ(color); // Get the square of the king
//     return isSquareAttacked(~color, kingSquare); // Check if the king is attacked by the opposite color
//  }

//  inline bool Board::is_on_board(Square sq) const {
//     return sq >= SQ_A1 && sq <= SQ_H8;
//  }

//  inline bool Board::is_empty(Square sq) const {
//     return pieceAtB(sq) == None;
//  }

//  inline bool Board::is_enemy(Square from, Square to) const {
//     return colorOf(from) != colorOf(to);
//  }

//  inline bool Board::is_knight_jump_valid(Square from, Square to) const {
//     if (!is_on_board(to)) return false;
//     int df = std::abs(square_file(to) - square_file(from));
//     int dr = std::abs(square_rank(to) - square_rank(from));
//     return (df == 2 && dr == 1) || (df == 1 && dr == 2);
//  }

//  inline bool Board::add_and_stop_on_block(std::vector<Move>& moves, Square from, Square to) const {
//     if (!is_on_board(to)) return false;

//     if (is_empty(to)) {
//         moves.emplace_back(static_cast<Move>((from << 6) | to));
//         return true;
//     } else if (is_enemy(from, to)) {
//         moves.emplace_back(static_cast<Move>((from << 6) | to));
//         return false;
//     }

//     return false;
// }

//  inline std::vector<Move> Board::generate_moves_for_piece(PieceType pt, Square sq) const {
//     std::vector<Move> moves;
//     Color myColor = colorOf(sq);

//     auto add = [&](Square to) {
//         if (!is_on_board(to)) return;
//         if (is_empty(to) || is_enemy(sq, to))
//             moves.emplace_back(static_cast<Move>((sq << 6) | to));
//     };

//     switch (pt) {
//         case PAWN: {
//             int dir = (myColor == White) ? 8 : -8;
//             Square oneStep = static_cast<Square>(sq + dir);
//             if (is_on_board(oneStep) && is_empty(oneStep))
//                 moves.emplace_back(static_cast<Move>((sq << 6) | oneStep));

//             // Double move from initial rank
//             int rank = square_rank(sq);
//             if ((myColor == White && rank == 1) || (myColor == Black && rank == 6)) {
//                 Square twoStep = static_cast<Square>(sq + 2 * dir);
//                 if (is_empty(oneStep) && is_empty(twoStep))
//                     moves.emplace_back(static_cast<Move>((sq << 6) | twoStep));
//             }

//             // Captures
//             for (int dx : {-1, 1}) {
//                 Square diag = static_cast<Square>(sq + dir + dx);
//                 if (is_on_board(diag) && is_enemy(sq, diag))
//                     moves.emplace_back(static_cast<Move>((sq << 6) | diag));
//             }

//             // En passant
//             if (enPassantSquare != NO_SQ) {
//                 for (int dx : {-1, 1}) {
//                     Square diag = static_cast<Square>(sq + dir + dx);
//                     if (diag == enPassantSquare)
//                         moves.emplace_back(static_cast<Move>((sq << 6) | diag));
//                 }
//             }
//             break;
//         }

//         case KNIGHT: {
//             const int knightMoves[] = { 17, 15, 10, 6, -17, -15, -10, -6 };
//             for (int offset : knightMoves) {
//                 Square to = static_cast<Square>(sq + offset);
//                 if (is_knight_jump_valid(sq, to))
//                     add(to);
//             }
//             break;
//         }

//         case BISHOP: {
//             static const int dirs[] = { 9, 7, -9, -7 };
//             for (int d : dirs) {
//                 for (int t = sq + d; is_on_board(static_cast<Square>(t)); t += d) {
//                     if (!add_and_stop_on_block(moves, sq, static_cast<Square>(t))) break;
//                 }
//             }
//             break;
//         }

//         case ROOK: {
//             static const int dirs[] = { 8, -8, 1, -1 };
//             for (int d : dirs) {
//                 for (int t = sq + d; is_on_board(static_cast<Square>(t)); t += d) {
//                     if (!add_and_stop_on_block(moves, sq, static_cast<Square>(t))) break;
//                 }
//             }
//             break;
//         }

//         case QUEEN: {
//             static const int dirs[] = { 8, -8, 1, -1, 9, 7, -9, -7 };
//             for (int d : dirs) {
//                 for (int t = sq + d; is_on_board(static_cast<Square>(t)); t += d) {
//                     if (!add_and_stop_on_block(moves, sq, static_cast<Square>(t))) break;
//                 }
//             }
//             break;
//         }

//         case KING: {
//             static const int kingMoves[] = { 8, -8, 1, -1, 9, 7, -9, -7 };
//             for (int d : kingMoves) {
//                 Square to = static_cast<Square>(sq + d);
//                 if (is_on_board(to))
//                     add(to);
//             }

//             // Castling
//             if (myColor == White) {
//                 if (castlingRights & wk && is_empty(SQ_F1) && is_empty(SQ_G1) &&
//                     !isSquareAttacked(Black, SQ_E1) && !isSquareAttacked(Black, SQ_F1) && !isSquareAttacked(Black, SQ_G1))
//                     moves.emplace_back(static_cast<Move>((SQ_E1 << 6) | SQ_G1)); // King-side castling

//                 if (castlingRights & wq && is_empty(SQ_D1) && is_empty(SQ_C1) && is_empty(SQ_B1) &&
//                     !isSquareAttacked(Black, SQ_E1) && !isSquareAttacked(Black, SQ_D1) && !isSquareAttacked(Black, SQ_C1))
//                     moves.emplace_back(static_cast<Move>((SQ_E1 << 6) | SQ_C1)); // Queen-side castling
//             } else {
//                 if (castlingRights & bk && is_empty(SQ_F8) && is_empty(SQ_G8) &&
//                     !isSquareAttacked(White, SQ_E8) && !isSquareAttacked(White, SQ_F8) && !isSquareAttacked(White, SQ_G8))
//                     moves.emplace_back(static_cast<Move>((SQ_E8 << 6) | SQ_G8)); // King-side castling

//                 if (castlingRights & bq && is_empty(SQ_D8) && is_empty(SQ_C8) && is_empty(SQ_B8) &&
//                     !isSquareAttacked(White, SQ_E8) && !isSquareAttacked(White, SQ_D8) && !isSquareAttacked(White, SQ_C8))
//                     moves.emplace_back(static_cast<Move>((SQ_E8 << 6) | SQ_C8)); // Queen-side castling
//             }
//             break;
//         }
//         default:
//         break;
//     }

//     return moves;
//  }