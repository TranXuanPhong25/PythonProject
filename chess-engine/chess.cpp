#include "types.h"


Board::Board(std::string fen) {
    initializeLookupTables();
    stateHistory.reserve(MAX_PLY);
    hashHistory.reserve(512);
    pawnKeyHistory.reserve(512);

    sideToMove = White;
    enPassantSquare = NO_SQ;
    castlingRights = wk | wq | bk | bq;
    halfMoveClock = 0;
    fullMoveNumber = 1;

    pinHV = 0;
    pinD = 0;
    doubleCheck = 0;
    checkMask = DEFAULT_CHECKMASK;
    seen = 0;

    std::fill(std::begin(board), std::end(board), None);

    applyFen(fen);

    occEnemy = Enemy(sideToMove);
    occUs = Us(sideToMove);
    occAll = All();
    enemyEmptyBB = EnemyEmpty(sideToMove);
}

void Board::applyFen(const std::string &fen) {
    for (Piece p = WhitePawn; p < None; p++) {
        piecesBB[p] = 0ULL;
    }

    pawnKey = 0ULL;

    const std::vector<std::string> params = splitInput(fen);

    const std::string position = params[0];
    const std::string move_right = params[1];
    const std::string castling = params[2];
    const std::string en_passant = params[3];

    const std::string half_move_clock = params.size() > 4 ? params[4] : "0";
    const std::string full_move_counter = params.size() > 4 ? params[5] : "1";

    sideToMove = (move_right == "w") ? White : Black;

    std::fill(std::begin(board), std::end(board), None);

    Square square = Square(56);
    for (int index = 0; index < static_cast<int>(position.size()); index++) {
        char curr = position[index];
        if (charToPiece.find(curr) != charToPiece.end()) {
            const Piece piece = charToPiece[curr];
            placePiece(piece, square);

            if (type_of_piece(piece) == PAWN) {
                pawnKey ^= updateKeyPiece(piece, square);
            }

            square = Square(square + 1);
        } else if (curr == '/')
            square = Square(square - 16);
        else if (isdigit(curr))
            square = Square(square + (curr - '0'));
    }

    removeCastlingRightsAll(White);
    removeCastlingRightsAll(Black);

    for (size_t i = 0; i < castling.size(); i++) {
        if (readCastleString.find(castling[i]) != readCastleString.end())
            castlingRights |= readCastleString[castling[i]];
    }

    if (en_passant == "-") {
        enPassantSquare = NO_SQ;
    } else {
        char letter = en_passant[0];
        int file = letter - 96;
        int rank = en_passant[1] - 48;
        enPassantSquare = Square((rank - 1) * 8 + file - 1);
    }

    halfMoveClock = std::stoi(half_move_clock);

    // full_move_counter actually half moves
    fullMoveNumber = std::stoi(full_move_counter) * 2;

    hashKey = zobristHash();

    hashHistory.clear();
    pawnKeyHistory.clear();
    stateHistory.clear();

    hashHistory.push_back(hashKey);
    pawnKeyHistory.push_back(pawnKey);

}


void Board::makeNullMove() {
    stateHistory.emplace_back(State(enPassantSquare, castlingRights, halfMoveClock, None));
    sideToMove = ~sideToMove;

    hashKey ^= updateKeySideToMove();
    if (enPassantSquare != NO_SQ)
        hashKey ^= updateKeyEnPassant(enPassantSquare);

    enPassantSquare = NO_SQ;
    fullMoveNumber++;
}

void Board::unmakeNullMove() {
    const State restore = stateHistory.back();
    stateHistory.pop_back();

    enPassantSquare = restore.enPassant;
    castlingRights = restore.castling;
    halfMoveClock = restore.halfMove;

    hashKey ^= updateKeySideToMove();
    if (enPassantSquare != NO_SQ)
        hashKey ^= updateKeyEnPassant(enPassantSquare);

    fullMoveNumber--;
    sideToMove = ~sideToMove;
}

void Board::removePiece(Piece piece, Square sq) {
    piecesBB[piece] &= ~(1ULL << sq);
    board[sq] = None;
}

void Board::placePiece(Piece piece, Square sq) {
    piecesBB[piece] |= (1ULL << sq);
    board[sq] = piece;
}

void Board::movePiece(Piece piece, Square fromSq, Square toSq) {
    piecesBB[piece] &= ~(1ULL << fromSq);
    piecesBB[piece] |= (1ULL << toSq);
    board[fromSq] = None;
    board[toSq] = piece;
}

