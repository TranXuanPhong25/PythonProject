#ifndef EVALUATE_MOBILITY_HPP
#define EVALUATE_MOBILITY_HPP

#include "types.hpp"
#include "chess.hpp"

// Forward declaration
struct EvalInfo;

// Mobility area represents squares where pieces can safely move
Chess::Bitboard getMobilityArea(const Chess::Board& board, Chess::Color color);

// Mobility evaluation functions for each piece type
void evaluateKnightMobility(EvalInfo& ei, Chess::Color color);
void evaluateBishopMobility(EvalInfo& ei, Chess::Color color);
void evaluateRookMobility(EvalInfo& ei, Chess::Color color);
void evaluateQueenMobility(EvalInfo& ei, Chess::Color color);

// Main mobility evaluation function
void evaluateMobility(EvalInfo& ei, Chess::Color color);

// Mobility evaluation constants
extern const int KnightMobilityBonus[9][2]; // [squares][mg/eg]
extern const int BishopMobilityBonus[14][2]; // [squares][mg/eg]
extern const int RookMobilityBonus[15][2]; // [squares][mg/eg]
extern const int QueenMobilityBonus[28][2]; // [squares][mg/eg]

#endif // EVALUATE_MOBILITY_HPP