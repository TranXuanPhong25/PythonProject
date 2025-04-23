#pragma once

#include "chess.hpp"

using namespace Chess;

// Attack weights calibrated to more balanced values
constexpr int QueenAttackValue = 20;  // Reduced from 45
constexpr int RookAttackValue = 18;   // Reduced from 45
constexpr int BishopAttackValue = 15; // Reduced from 25
constexpr int KnightAttackValue = 15; // Reduced from 35
constexpr int PawnAttackValue = 8;    // Reduced from 15

// Special bonuses for attacks - more nuanced
constexpr int AttackWeightKing = 40;  // Reduced from 100
constexpr int XrayRookBonus = 10;     // Reduced from 20
constexpr int XrayBishopBonus = 8;    // Reduced from 15
constexpr int DiagonalQueenAttackBonus = 5; // Reduced from 10

// Pinned piece penalties - less severe
constexpr int PinnedPieceRook = -25;  // Less severe than -60
constexpr int PinnedPieceBishop = -20; // Less severe than -45

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
Bitboard getKingDangerZone(const Board& board, Color side);