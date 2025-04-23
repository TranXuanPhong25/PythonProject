#include "evaluate_threatEG.hpp"
#include "evaluate.hpp"

const int MINOR_THREAT_SCORES[7] = {0, 32, 41, 56, 119, 161, 0};
const int ROOK_THREAT_SCORES[7] = {0, 46, 68, 60, 38, 41, 0};

// Main threats evaluation function with O(n) complexity
int threats_endgame(const Board& board) {
  // Pre-calculate piece locations and attack squares in a single pass
  Color c = board.sideToMove;
  Color them = ~c;
  
  // Get key piece bitboards
  U64 ourPawns = board.pieces(PAWN, c);
  U64 theirPawns = board.pieces(PAWN, them);
  U64 ourKnights = board.pieces(KNIGHT, c);
  U64 theirKnights = board.pieces(KNIGHT, them);
  U64 ourBishops = board.pieces(BISHOP, c);
  U64 theirBishops = board.pieces(BISHOP, them);
  U64 ourRooks = board.pieces(ROOK, c);
  U64 theirRooks = board.pieces(ROOK, them);
  U64 ourQueens = board.pieces(QUEEN, c);
  U64 theirQueens = board.pieces(QUEEN, them);
  U64 ourKing = 1ULL << board.KingSQ(c);
  U64 theirKing = 1ULL << board.KingSQ(them);
  
  // Combined bitboards for faster lookups
  U64 ourPieces = board.Us(c);
  U64 theirPieces = board.Us(them);
  U64 theirMinorPieces = theirKnights | theirBishops;
  U64 theirMajorPieces = theirRooks | theirQueens;
  U64 theirNonPawns = theirMinorPieces | theirMajorPieces;
  
  // Setup square occupancy arrays (64 bits = one lookup per square)
  U64 squareOccupancy[64] = {0};
  U64 squareType[64] = {0};
  
  // Attack maps
  U64 pawnAttacks = 0;
  U64 knightAttacks = 0;
  U64 bishopAttacks = 0;
  U64 rookAttacks = 0;
  U64 queenAttacks = 0;
  U64 kingAttacks = KingAttacks(board.KingSQ(c));
  U64 ourAttacks = 0;
  
  U64 theirPawnAttacks = 0;
  U64 theirKnightAttacks = 0;
  U64 theirBishopAttacks = 0;
  U64 theirRookAttacks = 0;
  U64 theirQueenAttacks = 0;
  U64 theirKingAttacks = KingAttacks(board.KingSQ(them));
  U64 theirAttacks = 0;
  
  // Fill the square lookup tables and calculate attacks
  U64 tempBB;
  Square sq;
  
  // Our pawn attacks
  tempBB = ourPawns;
  while (tempBB) {
    sq = static_cast<Square>(pop_lsb(tempBB));
    U64 attacks = PawnAttacks(sq, c);
    pawnAttacks |= attacks;
    ourAttacks |= attacks;
  }
  
  // Their pawn attacks
  tempBB = theirPawns;
  while (tempBB) {
    sq = static_cast<Square>(pop_lsb(tempBB));
    U64 attacks = PawnAttacks(sq, them);
    theirPawnAttacks |= attacks;
    theirAttacks |= attacks;
  }
  
  // Our knight attacks
  tempBB = ourKnights;
  while (tempBB) {
    sq = static_cast<Square>(pop_lsb(tempBB));
    U64 attacks = KnightAttacks(sq);
    knightAttacks |= attacks;
    ourAttacks |= attacks;
  }
  
  // Their knight attacks
  tempBB = theirKnights;
  while (tempBB) {
    sq = static_cast<Square>(pop_lsb(tempBB));
    U64 attacks = KnightAttacks(sq);
    theirKnightAttacks |= attacks;
    theirAttacks |= attacks;
    
    // Record piece type
    squareOccupancy[sq] = 1ULL << sq;
    squareType[sq] = KNIGHT;
  }
  
  // Our bishop attacks
  tempBB = ourBishops;
  while (tempBB) {
    sq = static_cast<Square>(pop_lsb(tempBB));
    U64 attacks = BishopAttacks(sq, board.occAll);
    bishopAttacks |= attacks;
    ourAttacks |= attacks;
  }
  
  // Their bishop attacks
  tempBB = theirBishops;
  while (tempBB) {
    sq = static_cast<Square>(pop_lsb(tempBB));
    U64 attacks = BishopAttacks(sq, board.occAll);
    theirBishopAttacks |= attacks;
    theirAttacks |= attacks;
    
    // Record piece type
    squareOccupancy[sq] = 1ULL << sq;
    squareType[sq] = BISHOP;
  }
  
  // Our rook attacks
  tempBB = ourRooks;
  while (tempBB) {
    sq = static_cast<Square>(pop_lsb(tempBB));
    U64 attacks = RookAttacks(sq, board.occAll);
    rookAttacks |= attacks;
    ourAttacks |= attacks;
  }
  
  // Their rook attacks
  tempBB = theirRooks;
  while (tempBB) {
    sq = static_cast<Square>(pop_lsb(tempBB));
    U64 attacks = RookAttacks(sq, board.occAll);
    theirRookAttacks |= attacks;
    theirAttacks |= attacks;
    
    // Record piece type
    squareOccupancy[sq] = 1ULL << sq;
    squareType[sq] = ROOK;
  }
  
  // Our queen attacks
  tempBB = ourQueens;
  while (tempBB) {
    sq = static_cast<Square>(pop_lsb(tempBB));
    U64 attacks = QueenAttacks(sq, board.occAll);
    queenAttacks |= attacks;
    ourAttacks |= attacks;
  }
  
  // Their queen attacks
  tempBB = theirQueens;
  while (tempBB) {
    sq = static_cast<Square>(pop_lsb(tempBB));
    U64 attacks = QueenAttacks(sq, board.occAll);
    theirQueenAttacks |= attacks;
    theirAttacks |= attacks;
    
    // Record piece type
    squareOccupancy[sq] = 1ULL << sq;
    squareType[sq] = QUEEN;
  }
  
  int v = 0;
  
  // Hanging pieces 
  int hangingCount = 0;
  for (PieceType pt = PAWN; pt <= QUEEN; ++pt) {
    U64 pieces = board.pieces(pt, c);
    U64 attackedPieces = pieces & theirAttacks;
    
    while (attackedPieces) {
      sq = static_cast<Square>(pop_lsb(attackedPieces));
      
      // Check if undefended or only defended by pawn
      Board mutableBoard = board; // Need mutable for non-const method
      U64 defenders = mutableBoard.attackersForSide(c, sq, mutableBoard.All());
      if (defenders == 0 || (defenders & ourPawns) == defenders) {
        hangingCount++;
      }
    }
  }
  v += 36 * hangingCount;
  
  // 2. King threat 
  U64 kingRing = KingAttacks(board.KingSQ(c)) | ourKing;
  bool hasKingThreat = (theirAttacks & kingRing) != 0;
  v += hasKingThreat ? 89 : 0;
  
  // 3. Pawn push threats
  int pawnPushCount = 0;
  tempBB = ourPawns;
  
  while (tempBB) {
    sq = static_cast<Square>(pop_lsb(tempBB));
    int rank = square_rank(sq);
    int file = square_file(sq);
    
    // Skip pawns that can't advance
    if ((c == White && rank == 7) || (c == Black && rank == 0)) {
      continue;
    }
    
    // Get square in front of pawn
    Square pushSq = c == White ? Square(sq + 8) : Square(sq - 8);
    
    // Skip if square is occupied
    if (board.pieceAtB(pushSq) != None) {
      continue;
    }
    
    // Check if push threatens enemy pieces (captures on left or right)
    U64 attackMask = 0;
    if (file > 0) {
      Square leftCapture = c == White ? Square(pushSq + 7) : Square(pushSq - 9);
      attackMask |= 1ULL << leftCapture;
    }
    
    if (file < 7) {
      Square rightCapture = c == White ? Square(pushSq + 9) : Square(pushSq - 7);
      attackMask |= 1ULL << rightCapture;
    }
    
    // Check if any enemy non-pawn pieces are in the attack mask
    if (attackMask & theirNonPawns) {
      pawnPushCount++;
    }
  }
  v += 39 * pawnPushCount;
  
  // Safe pawn threats
  int safePawnCount = 0;
  tempBB = ourPawns;
  
  while (tempBB) {
    sq = static_cast<Square>(pop_lsb(tempBB));
    U64 sqBit = 1ULL << sq;
    
    // Check if pawn is safe (not attacked or defended)
    bool isSafe = !(theirAttacks & sqBit) || (ourAttacks & sqBit);
    
    if (isSafe) {
      // Get attacks by this pawn
      U64 attacks = PawnAttacks(sq, c);
      
      // Count threatened pieces
      safePawnCount += popcount(attacks & theirNonPawns);
    }
  }
  v += 94 * safePawnCount;
  
  // Slider on queen
  int sliderOnQueenCount = 0;
  if (theirQueens) {
    U64 ourSliderAttacks = bishopAttacks | rookAttacks | queenAttacks;
    sliderOnQueenCount = popcount(ourSliderAttacks & theirQueens);
  }
  v += 18 * sliderOnQueenCount;
  
  // Knight on queen
  int knightOnQueenCount = 0;
  if (theirQueens) {
    knightOnQueenCount = popcount(knightAttacks & theirQueens);
  }
  v += 11 * knightOnQueenCount;
  
  // Restricted pieces 
  int restrictedCount = 0;
  
  // Knights
  tempBB = theirKnights;
  while (tempBB) {
    sq = static_cast<Square>(pop_lsb(tempBB));
    U64 attacks = KnightAttacks(sq);
    int mobility = popcount(attacks & ~theirPieces);
    if (mobility <= 2) restrictedCount++;
  }
  
  // Bishops
  tempBB = theirBishops;
  while (tempBB) {
    sq = static_cast<Square>(pop_lsb(tempBB));
    U64 attacks = BishopAttacks(sq, board.occAll);
    int mobility = popcount(attacks & ~theirPieces);
    if (mobility <= 4) restrictedCount++;
  }
  
  // Rooks
  tempBB = theirRooks;
  while (tempBB) {
    sq = static_cast<Square>(pop_lsb(tempBB));
    U64 attacks = RookAttacks(sq, board.occAll);
    int mobility = popcount(attacks & ~theirPieces);
    if (mobility <= 6) restrictedCount++;
  }
  
  // Queens
  tempBB = theirQueens;
  while (tempBB) {
    sq = static_cast<Square>(pop_lsb(tempBB));
    U64 attacks = QueenAttacks(sq, board.occAll);
    int mobility = popcount(attacks & ~theirPieces);
    if (mobility <= 8) restrictedCount++;
  }
  
  v += 7 * restrictedCount;
  
  // Minor and rook threats
  int minorThreatScore = 0;
  int rookThreatScore = 0;
  
  // Process enemy pieces
  for (PieceType pt = PAWN; pt <= QUEEN; ++pt) {
    if (pt == KING) continue; // Skip king
    
    U64 pieces = board.pieces(pt, them);
    
    // Minor threats (knights and bishops)
    U64 minorThreats = (knightAttacks | bishopAttacks) & pieces;
    while (minorThreats) {
      pop_lsb(minorThreats);
      minorThreatScore += MINOR_THREAT_SCORES[pt];
    }
    
    // Rook threats
    U64 rookThreats = rookAttacks & pieces;
    while (rookThreats) {
      pop_lsb(rookThreats);
      rookThreatScore += ROOK_THREAT_SCORES[pt];
    }
  }
  
  v += minorThreatScore + rookThreatScore;
  
  return c == White ? v : -v;
}

// Helper function to get piece color
Color get_piece_color(Piece p) {
  if (p == None) return White; 
  return (p >= WhitePawn && p <= WhiteKing) ? White : Black;
}

// Helper function to get piece type
PieceType get_piece_type(Piece p) {
  if (p == None) return NONETYPE;
  return PieceType(p % 6);
}

int hanging(const Board& board) { return 0; }
bool king_threat(const Board& board) { return false; }
int pawn_push_threat(const Board& board) { return 0; }
int threat_safe_pawn(const Board& board) { return 0; }
int slider_on_queen(const Board& board) { return 0; }
int knight_on_queen(const Board& board) { return 0; }
int restricted(const Board& board) { return 0; }
int minor_threat(const Board& board, Square s) { return 0; }
int rook_threat(const Board& board, Square s) { return 0; }