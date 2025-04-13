#pragma once
#include "evaluate.hpp" 
#include "score_move.hpp"
#include "chess.hpp"
#include <vector>
using namespace Chess;

int evaluatePawnStructure(const Board& board);
int evaluateCenterControl(const Board& board);

int evaluateDoubledPawns(const Board &board, Color color);
int evaluateIsolatedPawns(const Board &board, Color color);
int evaluatePassedPawns(const Board &board, Color color);
int evaluatePhalanxPawns(const Board &board, Color color);
int evaluatePassedPawnSupport(const Board &board, Color color);
int evaluateBlockedPawns(const Board &board, Color color);
int evaluatePawnChains(const Board &board, Color color);
int evaluateConnectedPawns(const Board &board, Color color);
int evaluatePawnShield(const Board &board, Color color);
int evaluateBackwardPawns(const Board &board, Color color);
int evaluateHolesAndOutposts(const Board &board, Color color);
int evaluatePawnLeverThreats(const Board &board, Color color);
int evaluateFileOpenness(const Board &board, Color color);
int evaluateSpaceAdvantage(const Board &board, Color color);
int evaluatePawnMajority(const Board &board, Color color);
int evaluatePawnStorm(const Board &board, Color color);
int evaluateMobility(const Board &board, Color color);
void updateMobility(Board &board, Move move, int &mobilityScore, Color color);
int evaluateKingSafety(const Board &board, Color color);
int evaluateKingOpenFiles(const Board &board, Color color);
int evaluateKingMobility(const Board &board, Color color);