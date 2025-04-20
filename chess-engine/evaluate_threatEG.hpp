#pragma once

#include "chess.hpp"
#include "types.hpp"

using namespace Chess;

// Main threats evaluation function for endgame phase
int threats_endgame(const Board& board);

// Helper functions for threat evaluation
int hanging(const Board& board);
bool king_threat(const Board& board);
int pawn_push_threat(const Board& board);
int threat_safe_pawn(const Board& board);
int slider_on_queen(const Board& board);
int knight_on_queen(const Board& board);
int restricted(const Board& board);
int minor_threat(const Board& board, Square s);
int rook_threat(const Board& board, Square s);