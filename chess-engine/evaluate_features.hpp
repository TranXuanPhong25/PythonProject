#pragma once

#include "chess.hpp" 
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