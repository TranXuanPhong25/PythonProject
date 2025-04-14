#include "bench.hpp"
#include <bits/unique_ptr.h>

// Benchmark the speed of your search algorithm
void benchSearch(int depth, TranspositionTable *table)
{
   std::string positions[] = {

      "8/8/1p6/2r1k3/8/4np2/6r1/K7 b - - 0 1",  //c5c1
   };

   std::cout << "\nStarting Search Benchmark..." << std::endl;
   int index = 1;
   for (const auto &fen : positions)
   {
      Board board(fen);
      std::cout << "\n"
                << index++ << " Testing position: " << fen << std::endl;

      // table->clear(); // Clear TT between iterations

      auto start = std::chrono::high_resolution_clock::now();
      Move bestMove = getBestMove(board, depth, table);
      auto end = std::chrono::high_resolution_clock::now();

      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

      std::cout << "Depth " << depth << ": Best move "
                << convertMoveToUci(bestMove);

      if (promoted(bestMove))
      {
         std::cout << " promotion: " << (int)piece(bestMove);
      }

      std::cout << " (" << duration << " ms)" << std::endl;
   }
}
int main()
{
   TranspositionTable *table;
   auto ttable = std::make_unique<TranspositionTable>();
   table = ttable.get();
   table->Initialize(512);
   int maxDepth = 13; // Default max depth for benchmarking
   Board board(DEFAULT_POS);
   for (int d = 1; d <= 8; d++) {
      getBestMove(board, d, table);
   }
   // benchPerft(maxDepth);
   auto start = std::chrono::high_resolution_clock::now();

   benchSearch(maxDepth, table);
   auto end = std::chrono::high_resolution_clock::now();

   auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
   std::cout << "\nTotal time taken for all positions: " << duration << " s" << std::endl;
   return 0;
}
