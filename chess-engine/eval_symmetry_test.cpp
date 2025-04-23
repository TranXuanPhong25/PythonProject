#include "chess.hpp"
#include "evaluate.hpp"
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

using namespace Chess;

// Utility functions
void printTestHeader(const std::string& title) {
    std::cout << "\n==================================================\n";
    std::cout << "  " << title;
    std::cout << "\n==================================================\n";
}

void printResult(const std::string& testName, bool passed, int score1 = 0, int score2 = 0) {
    std::cout << std::left << std::setw(40) << testName;
    if (passed) {
        std::cout << "\033[32m[PASSED]\033[0m";
    } else {
        std::cout << "\033[31m[FAILED]\033[0m";
    }
    if (score1 != 0 || score2 != 0) {
        std::cout << " (" << score1 << " vs " << score2 << ")";
    }
    std::cout << std::endl;
}

// Mirror a position (swap white and black pieces)
std::string mirrorFEN(const std::string& fen) {
    std::string result = "";
    
    // Split FEN into components
    std::vector<std::string> parts;
    size_t start = 0, end = 0;
    while ((end = fen.find(' ', start)) != std::string::npos) {
        parts.push_back(fen.substr(start, end - start));
        start = end + 1;
    }
    parts.push_back(fen.substr(start));
    
    // Mirror piece positions
    std::string position = parts[0];
    for (auto& c : position) {
        if (c >= 'a' && c <= 'z') {
            c = toupper(c);
        } else if (c >= 'A' && c <= 'Z') {
            c = tolower(c);
        }
    }
    
    // Change side to move
    std::string turn = parts[1] == "w" ? "b" : "w";
    
    // Mirror castling rights
    std::string castling = "";
    for (char c : parts[2]) {
        if (c == 'K') castling += 'k';
        else if (c == 'Q') castling += 'q';
        else if (c == 'k') castling += 'K';
        else if (c == 'q') castling += 'Q';
        else castling += c;
    }
    if (castling.empty()) castling = "-";
    
    // Mirror en passant square if present
    std::string enPassant = parts[3];
    if (enPassant != "-") {
        char file = enPassant[0];
        char rank = (enPassant[1] == '3') ? '6' : '3';
        enPassant = std::string(1, file) + rank;
    }
    
    // Combine the parts back into a FEN
    result = position + " " + turn + " " + castling + " " + enPassant;
    if (parts.size() > 4)
        result += " " + parts[4];
    if (parts.size() > 5)
        result += " " + parts[5];
    
    return result;
}

// Test symmetrical positions (should evaluate to 0)
void testSymmetricalPositions() {
    printTestHeader("TESTING SYMMETRICAL POSITIONS");
    
    std::vector<std::string> positions = {
        // Starting position
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        
        // Empty board
        "8/8/8/8/8/8/8/8 w - - 0 1",
        
        // Kings only in symmetrical position
        "4k3/8/8/8/8/8/8/4K3 w - - 0 1",
        
        // Rooks in symmetrical position
        "4k3/8/8/8/3RR3/8/8/4K3 w - - 0 1",
        
        // Knights in symmetrical position
        "4k3/8/8/3NN3/8/8/8/4K3 w - - 0 1",
        
        // Complex symmetrical position
        "r3k2r/p3p2p/b2p1p1b/1pPP1Pp1/1P3P2/2PBB2P/4K3/R6R w kq - 0 1"
    };
    
    for (const auto& fen : positions) {
        Board board(fen);
        int score = evaluate(board);
        
        std::string testName = "Position: " + fen.substr(0, 20) + "...";
        bool passed = (std::abs(score) < 5); // Allow small margin of error
        printResult(testName, passed, score, 0);
    }
}

// Test mirrored positions (should evaluate to exact opposites)
void testMirroredPositions() {
    printTestHeader("TESTING MIRRORED POSITIONS");
    
    std::vector<std::string> positions = {
        // Standard starting position
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        
        // Position with material advantage for white
        "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1",
        
        // Position with development advantage
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
        
        // Middle game position
        "r1bqk2r/ppp2ppp/2np1n2/2b1p3/2B1P3/2NP1N2/PPP2PPP/R1BQK2R w KQkq - 0 1",
        
        // Rook endgame
        "4k3/4r3/8/8/8/8/4R3/4K3 w - - 0 1",
        
        // Position with passed pawn
        "8/1P6/8/8/8/8/1p6/8 w - - 0 1",
        
        // Position with connected pawns
        "8/8/8/2PP4/8/8/8/8 w - - 0 1"
    };
    
    for (const auto& originalFEN : positions) {
        std::string mirroredFEN = mirrorFEN(originalFEN);
        
        Board originalBoard(originalFEN);
        Board mirroredBoard(mirroredFEN);
        
        int originalScore = evaluate(originalBoard);
        int mirroredScore = evaluate(mirroredBoard);
        
        std::string testName = "Position: " + originalFEN.substr(0, 20) + "...";
        bool passed = (originalScore == -mirroredScore);
        
        printResult(testName, passed, originalScore, mirroredScore);
    }
}

// Test for specific square mapping issues
void testSquareMapping() {
    printTestHeader("TESTING SQUARE MAPPING");
    
    // Create a minimal board with just kings and a single piece to test
    std::vector<std::pair<std::string, std::string>> positionPairs = {
        // White knight vs Black knight in mirrored positions
        {"4k3/8/8/8/4N3/8/8/4K3 w - - 0 1", "4K3/8/8/8/4n3/8/8/4k3 w - - 0 1"},
        
        // White bishop vs Black bishop in mirrored positions
        {"4k3/8/8/8/4B3/8/8/4K3 w - - 0 1", "4K3/8/8/8/4b3/8/8/4k3 w - - 0 1"},
        
        // White rook vs Black rook in mirrored positions
        {"4k3/8/8/8/4R3/8/8/4K3 w - - 0 1", "4K3/8/8/8/4r3/8/8/4k3 w - - 0 1"},
        
        // White queen vs Black queen in mirrored positions
        {"4k3/8/8/8/4Q3/8/8/4K3 w - - 0 1", "4K3/8/8/8/4q3/8/8/4k3 w - - 0 1"},
        
        // White pawn vs Black pawn in mirrored positions
        {"4k3/8/8/8/4P3/8/8/4K3 w - - 0 1", "4K3/8/8/8/4p3/8/8/4k3 w - - 0 1"}
    };
    
    for (const auto& [whitePos, blackPos] : positionPairs) {
        Board whiteBoard(whitePos);
        Board blackBoard(blackPos);
        
        int whiteScore = evaluate(whiteBoard);
        int blackScore = evaluate(blackBoard);
        
        std::string testName = "Testing: " + whitePos.substr(0, 20) + "...";
        bool passed = (whiteScore == -blackScore);
        
        printResult(testName, passed, whiteScore, blackScore);
    }
}

// Main function
int main() {
    std::cout << "\nEVALUATION SYMMETRY TESTS\n";
    std::cout << "=========================\n";
    
    testSquareMapping();
    testSymmetricalPositions();
    testMirroredPositions();
    
    return 0;
}