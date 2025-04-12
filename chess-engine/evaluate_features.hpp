#pragma once

#include "chess.hpp" 
using namespace Chess;

int evaluatePawnStructure(const Board& board);
int evaluateCenterControl(const Board& board);

int evaluateDoubledPawns(const Board &board, Color color);
int evaluateIsolatedPawns(const Board &board, Color color);
int evaluatePassedPawns(const Board &board, Color color);
int evaluateMobility(const Board &board, Color color);
void updateMobility(Board &board, Move move, int &mobilityScore, Color color);