#include "evaluate_mobility.hpp"
#include "evaluate_pieces.hpp"

using namespace Chess;

// Mobility bonus tables for each piece type based on Stockfish approach
// Values represent [middlegame, endgame] bonuses

// Knight mobility bonuses [0-8 squares]
const int KnightMobilityBonus[9][2] = {
    {-50, -50}, // 0 squares
    {-25, -30}, // 1 square
    {-10, -15}, // 2 squares
    {  0,   0}, // 3 squares
    { 10,  10}, // 4 squares
    { 20,  20}, // 5 squares
    { 25,  25}, // 6 squares
    { 30,  30}, // 7 squares
    { 35,  35}  // 8 squares
};

// Bishop mobility bonuses [0-13 squares]
const int BishopMobilityBonus[14][2] = {
    {-50, -50}, // 0 squares
    {-25, -30}, // 1 square
    {-10, -15}, // 2 squares
    {  0,   0}, // 3 squares
    { 10,  10}, // 4 squares
    { 20,  15}, // 5 squares
    { 25,  20}, // 6 squares
    { 30,  25}, // 7 squares
    { 35,  30}, // 8 squares
    { 40,  35}, // 9 squares
    { 45,  35}, // 10 squares
    { 50,  40}, // 11 squares
    { 55,  45}, // 12 squares
    { 60,  50}  // 13 squares
};

// Rook mobility bonuses [0-14 squares]
const int RookMobilityBonus[15][2] = {
    {-50, -50}, // 0 squares
    {-25, -30}, // 1 square
    {-10, -15}, // 2 squares
    {  0,   0}, // 3 squares
    {  5,   5}, // 4 squares
    { 10,  10}, // 5 squares
    { 15,  15}, // 6 squares
    { 20,  20}, // 7 squares
    { 25,  25}, // 8 squares
    { 30,  30}, // 9 squares
    { 35,  35}, // 10 squares
    { 40,  40}, // 11 squares
    { 45,  45}, // 12 squares
    { 50,  50}, // 13 squares
    { 55,  55}  // 14 squares
};

// Queen mobility bonuses [0-27 squares]
const int QueenMobilityBonus[28][2] = {
    {-50, -50}, // 0 squares
    {-30, -30}, // 1 square
    {-20, -20}, // 2 squares
    {-10, -10}, // 3 squares
    {  0,   0}, // 4 squares
    { 10,   5}, // 5 squares
    { 15,  10}, // 6 squares
    { 20,  15}, // 7 squares
    { 25,  20}, // 8 squares
    { 30,  25}, // 9 squares
    { 35,  30}, // 10 squares
    { 40,  35}, // 11 squares
    { 45,  40}, // 12 squares
    { 50,  45}, // 13 squares
    { 50,  45}, // 14 squares
    { 55,  50}, // 15 squares
    { 55,  50}, // 16 squares
    { 60,  55}, // 17 squares
    { 60,  55}, // 18 squares
    { 60,  55}, // 19 squares
    { 60,  55}, // 20 squares
    { 65,  60}, // 21 squares
    { 65,  60}, // 22 squares
    { 65,  60}, // 23 squares
    { 70,  65}, // 24 squares
    { 70,  65}, // 25 squares
    { 70,  65}, // 26 squares
    { 70,  65}  // 27 squares
};

// Calculate the mobility area for a given side
Bitboard getMobilityArea(const Board& board, Color color) {
    Bitboard mobility_area = ~0ULL; // Start with the full board
    Color them = ~color;
    
    // Remove squares with own pieces except king and queen
    mobility_area &= ~(board.pieces(PAWN, color) | 
                      board.pieces(KNIGHT, color) | 
                      board.pieces(BISHOP, color) | 
                      board.pieces(ROOK, color));
    
    // Remove squares attacked by enemy pawns
    Bitboard enemyPawns = board.pieces(PAWN, them);
    Bitboard pawnAttacks = 0ULL;
    
    if (them == White) {  // Changed from WHITE to White
        // Calculate black pawn attacks
        Bitboard temp = enemyPawns;
        while (temp) {
            Square sq = static_cast<Square>(pop_lsb(temp));
            int rank = square_rank(sq);
            int file = square_file(sq);
            if (file > 0) pawnAttacks |= 1ULL << ((rank - 1) * 8 + file - 1);
            if (file < 7) pawnAttacks |= 1ULL << ((rank - 1) * 8 + file + 1);
        }
    } else {
        // Calculate white pawn attacks
        Bitboard temp = enemyPawns;
        while (temp) {
            Square sq = static_cast<Square>(pop_lsb(temp));
            int rank = square_rank(sq);
            int file = square_file(sq);
            if (file > 0) pawnAttacks |= 1ULL << ((rank + 1) * 8 + file - 1);
            if (file < 7) pawnAttacks |= 1ULL << ((rank + 1) * 8 + file + 1);
        }
    }
    
    mobility_area &= ~pawnAttacks;
    
    // Remove squares where opponent king can attack
    mobility_area &= ~getKingRing(board, them);
    
    return mobility_area;
}

// Evaluate knight mobility
void evaluateKnightMobility(EvalInfo& ei, Color color) {
    Bitboard knights = ei.board.pieces(KNIGHT, color);
    Bitboard mobilityArea = getMobilityArea(ei.board, color);
    
    while (knights) {
        Square sq = static_cast<Square>(pop_lsb(knights));
        Bitboard attacks = KnightAttacks(sq) & mobilityArea;
        int mobility = popcount(attacks);
        
        // Ensure index is in bounds
        if (mobility > 8) mobility = 8;
        
        // Apply mobility bonus
        ei.mgScore += KnightMobilityBonus[mobility][0] * (color == White ? 1 : -1);
        ei.egScore += KnightMobilityBonus[mobility][1] * (color == White ? 1 : -1);
    }
}

// Evaluate bishop mobility
void evaluateBishopMobility(EvalInfo& ei, Color color) {
    Bitboard bishops = ei.board.pieces(BISHOP, color);
    Bitboard mobilityArea = getMobilityArea(ei.board, color);
    
    while (bishops) {
        Square sq = static_cast<Square>(pop_lsb(bishops));
        Bitboard attacks = BishopAttacks(sq, ei.board.All()) & mobilityArea;
        int mobility = popcount(attacks);
        
        // Ensure index is in bounds
        if (mobility > 13) mobility = 13;
        
        // Apply mobility bonus
        ei.mgScore += BishopMobilityBonus[mobility][0] * (color == White ? 1 : -1);
        ei.egScore += BishopMobilityBonus[mobility][1] * (color == White ? 1 : -1);
    }
}

// Evaluate rook mobility
void evaluateRookMobility(EvalInfo& ei, Color color) {
    Bitboard rooks = ei.board.pieces(ROOK, color);
    Bitboard mobilityArea = getMobilityArea(ei.board, color);
    
    while (rooks) {
        Square sq = static_cast<Square>(pop_lsb(rooks));
        Bitboard attacks = RookAttacks(sq, ei.board.All()) & mobilityArea;
        int mobility = popcount(attacks);
        
        // Ensure index is in bounds
        if (mobility > 14) mobility = 14;
        
        // Apply mobility bonus
        ei.mgScore += RookMobilityBonus[mobility][0] * (color == White ? 1 : -1);
        ei.egScore += RookMobilityBonus[mobility][1] * (color == White ? 1 : -1);
    }
}

// Evaluate queen mobility
void evaluateQueenMobility(EvalInfo& ei, Color color) {
    Bitboard queens = ei.board.pieces(QUEEN, color);
    Bitboard mobilityArea = getMobilityArea(ei.board, color);
    
    while (queens) {
        Square sq = static_cast<Square>(pop_lsb(queens));
        Bitboard attacks = QueenAttacks(sq, ei.board.All()) & mobilityArea;
        int mobility = popcount(attacks);
        
        // Ensure index is in bounds
        if (mobility > 27) mobility = 27;
        
        // Apply mobility bonus
        ei.mgScore += QueenMobilityBonus[mobility][0] * (color == White ? 1 : -1);
        ei.egScore += QueenMobilityBonus[mobility][1] * (color == White ? 1 : -1);
    }
}

// Main mobility evaluation function
void evaluateMobility(EvalInfo& ei, Color color) {
    evaluateKnightMobility(ei, color);
    evaluateBishopMobility(ei, color);
    evaluateRookMobility(ei, color);
    evaluateQueenMobility(ei, color);
}