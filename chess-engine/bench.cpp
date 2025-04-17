#include "bench.hpp"
#include <bits/unique_ptr.h>
#include <fstream>
#include <iomanip>

TranspositionTable *table;
const std::string positions[] = {
    //  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    //  "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 3",
    //  "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    //  "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
    //  "4rrk1/2p1b1p1/p1p3q1/4p3/2P2n1p/1P1NR2P/PB3PP1/3R1QK1 b - - 2 24",
    //  "r3qbrk/6p1/2b2pPp/p3pP1Q/PpPpP2P/3P1B2/2PB3K/R5R1 w - - 16 42",
    //  "6k1/1R3p2/6p1/2Bp3p/3P2q1/P7/1P2rQ1K/5R2 b - - 4 44",
    //  "8/8/1p2k1p1/3p3p/1p1P1P1P/1P2PK2/8/8 w - - 3 54",
    //  "7r/2p3k1/1p1p1qp1/1P1Bp3/p1P2r1P/P7/4R3/Q4RK1 w - - 0 36",
    //  "r1bq1rk1/pp2b1pp/n1pp1n2/3P1p2/2P1p3/2N1P2N/PP2BPPP/R1BQ1RK1 b - - 2 10",
    //  "3r3k/2r4p/1p1b3q/p4P2/P2Pp3/1B2P3/3BQ1RP/6K1 w - - 3 87",
    //  "2r4r/1p4k1/1Pnp4/3Qb1pq/8/4BpPp/5P2/2RR1BK1 w - - 0 42",
    //  "4q1bk/6b1/7p/p1p4p/PNPpP2P/KN4P1/3Q4/4R3 b - - 0 37",
    //  "2q3r1/1r2pk2/pp3pp1/2pP3p/P1Pb1BbP/1P4Q1/R3NPP1/4R1K1 w - - 2 34",
    //  "1r2r2k/1b4q1/pp5p/2pPp1p1/P3Pn2/1P1B1Q1P/2R3P1/4BR1K b - - 1 37",
    //  "r3kbbr/pp1n1p1P/3ppnp1/q5N1/1P1pP3/P1N1B3/2P1QP2/R3KB1R b KQkq b3 0 17",
    //  "8/6pk/2b1Rp2/3r4/1R1B2PP/P5K1/8/2r5 b - - 16 42",
    //  "1r4k1/4ppb1/2n1b1qp/pB4p1/1n1BP1P1/7P/2PNQPK1/3RN3 w - - 8 29",
    //  "8/p2B4/PkP5/4p1pK/4Pb1p/5P2/8/8 w - - 29 68",
    //  "3r4/ppq1ppkp/4bnp1/2pN4/2P1P3/1P4P1/PQ3PBP/R4K2 b - - 2 20",
    //  "5rr1/4n2k/4q2P/P1P2n2/3B1p2/4pP2/2N1P3/1RR1K2Q w - - 1 49",
    //  "1r5k/2pq2p1/3p3p/p1pP4/4QP2/PP1R3P/6PK/8 w - - 1 51",
    //  "q5k1/5ppp/1r3bn1/1B6/P1N2P2/BQ2P1P1/5K1P/8 b - - 2 34",
    //  "r1b2k1r/5n2/p4q2/1ppn1Pp1/3pp1p1/NP2P3/P1PPBK2/1RQN2R1 w - - 0 22",
    //  "r1bqk2r/pppp1ppp/5n2/4b3/4P3/P1N5/1PP2PPP/R1BQKB1R w KQkq - 0 5",
    //  "r1bqr1k1/pp1p1ppp/2p5/8/3N1Q2/P2BB3/1PP2PPP/R3K2n b Q - 1 12",
    //  "r1bq2k1/p4r1p/1pp2pp1/3p4/1P1B3Q/P2B1N2/2P3PP/4R1K1 b - - 2 19",
    //  "r4qk1/6r1/1p4p1/2ppBbN1/1p5Q/P7/2P3PP/5RK1 w - - 2 25",
    //  "r7/6k1/1p6/2pp1p2/7Q/8/p1P2K1P/8 w - - 0 32",
    //  "r3k2r/ppp1pp1p/2nqb1pn/3p4/4P3/2PP4/PP1NBPPP/R2QK1NR w KQkq - 1 5",
    //  "3r1rk1/1pp1pn1p/p1n1q1p1/3p4/Q3P3/2P5/PP1NBPPP/4RRK1 w - - 0 12",
    //  "5rk1/1pp1pn1p/p3Brp1/8/1n6/5N2/PP3PPP/2R2RK1 w - - 2 20",
    //  "8/1p2pk1p/p1p1r1p1/3n4/8/5R2/PP3PPP/4R1K1 b - - 3 27",
    //  "8/8/8/8/5kp1/P7/8/1K1N4 w - - 0 1",   // Kc2 - mate
    //  "8/8/8/5N2/8/p7/8/2NK3k w - - 0 1",    // Na2 - mate
    //  "8/3k4/8/8/8/4B3/4KB2/2B5 w - - 0 1",  // draw,
    //  "Q7/8/8/1R6/3R4/4K2P/2k5/8 w - - 0 1", // need to check
    //  "7k/8/8/8/5q2/8/7r/3K4 b - - 0 1",     // f4d2,
    //  "8/8/4k3/8/6q1/7r/1K6/8 b - - 0 1",
    "r2r2k1/pp2bppp/4b3/8/4N3/8/PPP1BPPP/R4RK1 w - - 0 1",                     // b2b3
    "3k4/8/q7/5r2/8/3B4/PPP5/2KN4 b - - 0 1",                                  // a6h6
    "3k4/8/7q/5r2/8/3B4/PPP5/1K1N4 b - - 0 1",                                 // h6d2
    "3k4/8/8/5r2/8/3B4/PPPq4/1K1N4 w - - 0 1",                                 // a2a3
    "r1bqk2r/ppp1bppp/2n1p3/2Pp4/3P4/2PBPN2/P4PPP/R1BQK2R b KQkq - 2 8",       // e8g8
    "4b1k1/p5p1/7p/2qp1Q2/6P1/7P/5RK1/4rB2 b - - 2 39",                        // c5d6,
    "r2q1rk1/ppp1nppp/3p1n2/3Pp1B1/1b2P1b1/2NB1N2/PPP2PPP/R2Q1RK1 w - - 4 10", // g5f6,
    "r3r3/5pk1/3p3p/3Pp1p1/1Pp1n1q1/2Q1B3/1PB1NK2/3R2R1 w - - 0 41",           // c2e4
    "r3r3/5pk1/3p3p/3Pp1p1/1Pp1q3/2Q1B3/1P2NK2/3R2R1 w - - 0 42",              // e3d2
    "8/r5k1/3p4/3P2pp/1P3p2/1K6/1P1r4/4RR2 w - - 4 52"                         // e1e6

};
const std::string targetMoves[] = {
    "b2b3",
    "a6h6",
    "h6d2",
    "a2a3",
    "e8g8",
    "c5d6",
    "g5f6",
    "c2e4",
    "e3d2",
    "e1e6"};

// Test if the engine finds the expected best move for a position
bool testPosition(const std::string &fen, const std::string &expectedMove, int depth)
{
   SearchThread st;
   st.board = Board(fen);

   std::cout << "Testing position: " << fen << std::endl;
   std::cout << "Expected best move: " << expectedMove << std::endl;

   auto startime = std::chrono::high_resolution_clock::now();

  
   iterativeDeepening(st, depth);
   auto endtime = std::chrono::high_resolution_clock::now();
   std::chrono::duration<double> elapsed = endtime - startime;
   std::cout << "Time taken: " << elapsed.count() * 1000 << " ms" << std::endl;
   std::cout << "Nodes searched: " << st.nodes << std::endl;
   std::cout << "Best move found: " << convertMoveToUci(st.bestMove) << std::endl;
   std::cout << "-----------------------------------" << std::endl;
   // Convert found move to UCI format and check if it matches expected
   std::string foundMove = convertMoveToUci(st.bestMove);
   bool success = (foundMove == expectedMove);

   std::cout << "Result: " << (success ? "PASS" : "FAIL") << std::endl;
   if (!success)
   {
      std::cout << "Found: " << foundMove << ", Expected: " << expectedMove << std::endl;
   }
   std::cout << "-----------------------------------" << std::endl;

   return success;
}


// Run all position tests and report results
void runTests(int depth)
{
   std::cout << "\n==== RUNNING CHESS ENGINE TESTS ====" << std::endl;
   std::cout << "Testing engine at depth " << depth << std::endl;

   int totalTests = sizeof(positions) / sizeof(positions[0]);
   int passedTests = 0;
   time_t t = std::time(nullptr);
   std::ofstream logFile("test/test_results.txt");
   logFile << "Chess Engine Test Results" << std::endl;
   logFile << "=========================" << std::endl;
   logFile << "Date: " << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S") << std::endl;
   logFile << "Depth: " << depth << std::endl
           << std::endl;

   auto totalStartTime = std::chrono::high_resolution_clock::now();

   for (int i = 0; i < totalTests; i++)
   {
      std::cout << "\nTest #" << (i + 1) << " of " << totalTests << std::endl;

      // Store console output to redirect to file
      std::stringstream buffer;
      std::streambuf *old = std::cout.rdbuf(buffer.rdbuf());

      bool result = testPosition(positions[i], targetMoves[i], depth);

      // Restore console output
      std::cout.rdbuf(old);

      // Write to both console and file
      std::cout << buffer.str();
      logFile << "Test #" << (i + 1) << " of " << totalTests << std::endl;
      logFile << buffer.str() << std::endl;

      if (result)
         passedTests++;
   }

   auto totalEndTime = std::chrono::high_resolution_clock::now();
   auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(totalEndTime - totalStartTime).count();

   // Print and log summary
   std::string summary = "\n==== TEST SUMMARY ====\n"
                         "Tests passed: " +
                         std::to_string(passedTests) + "/" + std::to_string(totalTests) +
                         " (" + std::to_string(passedTests * 100 / totalTests) + "%)\n"
                                                                                 "Total time: " +
                         std::to_string(totalDuration) + " ms\n";

   std::cout << summary;
   logFile << summary;
   logFile.close();

   std::cout << "Test results saved to test_results.txt" << std::endl;
}


int main(int argc, char *argv[])
{
   initLateMoveTable();

   int testDepth = 15; // Default test depth

   // Create and initialize TT
   auto ttable = std::make_unique<TranspositionTable>();
   table = ttable.get();
   table->Initialize(512);

   auto start = std::chrono::high_resolution_clock::now();
   runTests(testDepth);

   // benchSearch(testDepth);
   auto end = std::chrono::high_resolution_clock::now();
   auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
   std::cout << "\nTotal time taken for all positions: " << duration << " ms" << std::endl;

   return 0;
}
