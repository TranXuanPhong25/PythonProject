#include "bench.hpp"
#include <bits/unique_ptr.h>
// Performance test for move generation
uint64_t perft(Board &board, int depth)
{
   if (depth == 0)
      return 1;

   Movelist moves;
   if (board.sideToMove == White)
   {
      Movegen::legalmoves<White, ALL>(board, moves);
   }
   else
   {
      Movegen::legalmoves<Black, ALL>(board, moves);
   }

   if (depth == 1)
      return moves.size;

   uint64_t nodes = 0;
   for (int i = 0; i < moves.size; i++)
   {
      board.makeMove(moves[i].move);
      nodes += perft(board, depth - 1);
      board.unmakeMove(moves[i].move);
   }

   return nodes;
}

// Benchmark function for perft
void benchPerft(int maxDepth)
{
   std::string positions[] = {
       "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",             // Initial position
       "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", // Kiwipete
       "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",                            // Position 3
       "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"      // Position 4
   };

   std::cout << "Starting Perft Benchmark..." << std::endl;

   for (const auto &fen : positions)
   {
      Board board(fen);
      std::cout << "\nTesting position: " << fen << std::endl;

      for (int depth = 1; depth <= maxDepth; depth++)
      {
         auto start = std::chrono::high_resolution_clock::now();
         uint64_t nodes = perft(board, depth);
         auto end = std::chrono::high_resolution_clock::now();

         auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
         double nps = (duration > 0) ? (nodes * 1000.0 / duration) : 0;

         std::cout << "Depth " << depth << ": "
                   << nodes << " nodes in "
                   << duration << " ms ("
                   << static_cast<uint64_t>(nps) << " nodes/sec)" << std::endl;

         if (duration > 10000)
            break; // Skip deeper depths if taking too long
      }
   }
}
// Benchmark the speed of your search algorithm
void benchSearch(int maxDepth, TranspositionTable *table)
{
   std::string positions[] = {
       "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
       "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 3",
       "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
       "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10"};

   std::cout << "\nStarting Search Benchmark..." << std::endl;

   for (const auto &fen : positions)
   {
      Board board(fen);
      std::cout << "\nTesting position: " << fen << std::endl;

      for (int depth = 1; depth <= maxDepth; depth++)
      {
         table->clear(); // Clear TT between iterations

         auto start = std::chrono::high_resolution_clock::now();
         Move bestMove = getBestMove(board, depth, table);
         auto end = std::chrono::high_resolution_clock::now();

         auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

         std::cout << "Depth " << depth << ": Best move "
                   << (int)from(bestMove) << "->" << (int)to(bestMove);

         if (promoted(bestMove))
         {
            std::cout << " promotion: " << (int)piece(bestMove);
         }

         std::cout << " (" << duration << " ms)" << std::endl;
      }
   }
}
int main(int argc, char **argv)
{
   TranspositionTable *table;
   auto ttable = std::make_unique<TranspositionTable>();
   table = ttable.get();
   table->Initialize(64);
   int maxDepth = 6; // Default max depth for benchmarking
   if (argc > 1)
   {
      maxDepth = std::stoi(argv[1]);
   }

   benchPerft(maxDepth);
   benchSearch(maxDepth, table);

   return 0;
}