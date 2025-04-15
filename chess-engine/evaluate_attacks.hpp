#pragma once

#include "chess.hpp"

using namespace Chess;

// Attack weights from Stockfish
constexpr int QueenAttackValue = 45;
constexpr int RookAttackValue = 45;
constexpr int BishopAttackValue = 25;
constexpr int KnightAttackValue = 35;
constexpr int PawnAttackValue = 15;

// Special bonuses for attacks
constexpr int AttackWeightKing = 100;
constexpr int XrayRookBonus = 20;
constexpr int XrayBishopBonus = 15;
constexpr int DiagonalQueenAttackBonus = 10;

// Pinned piece penalties
constexpr int PinnedPieceRook = -60;
constexpr int PinnedPieceBishop = -45;

// Function to evaluate all attack-related features
int evaluateAttacks(const Board& board);

// Helper functions for attack evaluation
Bitboard evaluatePinnedPieces(const Board& board, Color side);
Bitboard getKnightAttacks(const Board& board, Color side);
Bitboard getBishopXrayAttacks(const Board& board, Color side);
Bitboard getRookXrayAttacks(const Board& board, Color side);
Bitboard getQueenAttacks(const Board& board, Color side);
Bitboard getQueenDiagonalAttacks(const Board& board, Color side);
Bitboard getPawnAttacks(const Board& board, Color side);
Bitboard getKingAttacks(const Board& board, Color side);