#pragma once
#include "evaluate.hpp" 
#include "score_move.hpp"
#include "chess.hpp"
#include <vector>
using namespace Chess;

// Class to store pre-computed pawn structure data
class PawnStructureContext {
public:
    PawnStructureContext(const Board& board);
    
    // Precomputed bitboards
    Bitboard whitePawns;
    Bitboard blackPawns;
    Bitboard whitePawnAttacks;
    Bitboard blackPawnAttacks;
    
    // File-based bitboards
    Bitboard whitePawnsOnFile[8];
    Bitboard blackPawnsOnFile[8];
    
    // Rank-based bitboards
    Bitboard whitePawnsOnRank[8];
    Bitboard blackPawnsOnRank[8];
    
    // King positions
    Square whiteKingSq;
    Square blackKingSq;
    
    // Board state
    const Board& board;
    
    // Helper methods
    bool isPassed(Square sq, Color color) const;
    bool isIsolated(Square sq, Color color) const;
    bool isBackward(Square sq, Color color) const;
    bool isDoubled(Square sq, Color color) const;
    bool isConnected(Square sq, Color color) const;
    bool isPhalanx(Square sq, Color color) const;
    bool isShieldingKing(Square sq, Color color) const;
    
    // Game phase
    float gamePhase;
};

int evaluatePawnStructure(const Board& board);
int evaluateCenterControl(const Board& board);

int evaluateDoubledPawns(const PawnStructureContext& ctx, Color color);
int evaluateIsolatedPawns(const PawnStructureContext& ctx, Color color);
int evaluatePassedPawns(const PawnStructureContext& ctx, Color color);
int evaluatePhalanxPawns(const PawnStructureContext& ctx, Color color);
int evaluatePassedPawnSupport(const PawnStructureContext& ctx, Color color);
int evaluateBlockedPawns(const PawnStructureContext& ctx, Color color);
int evaluatePawnChains(const PawnStructureContext& ctx, Color color);
int evaluateConnectedPawns(const PawnStructureContext& ctx, Color color);
int evaluatePawnShield(const PawnStructureContext& ctx, Color color);
int evaluateBackwardPawns(const PawnStructureContext& ctx, Color color);
int evaluateHolesAndOutposts(const PawnStructureContext& ctx, Color color);
int evaluatePawnLeverThreats(const PawnStructureContext& ctx, Color color);
int evaluateFileOpenness(const PawnStructureContext& ctx, Color color);
int evaluateSpaceAdvantage(const PawnStructureContext& ctx, Color color);
int evaluatePawnMajority(const PawnStructureContext& ctx, Color color);
int evaluatePawnStorm(const PawnStructureContext& ctx, Color color);

int evaluateMobility(const Board &board, Color color);
void updateMobility(Board &board, Move move, int &mobilityScore, Color color);
int evaluateKingSafety(const Board &board, Color color);
int evaluateKingOpenFiles(const Board &board, Color color);
int evaluateKingMobility(const Board &board, Color color);
int evaluateQueenControlAndCheckmatePotential(const Board &board, Color color);
int evaluateCastlingAbility(const Board &board, Color color);