#include "profiler.hpp"

namespace Chess {

// Global profiling data instance
ProfilerData profiler;

void ProfilerData::reset()
{
   ttLookupTime = std::chrono::nanoseconds{0};
   moveGenTime = std::chrono::nanoseconds{0};
   evaluationTime = std::chrono::nanoseconds{0};
   seeTime = std::chrono::nanoseconds{0};
   moveMakingTime = std::chrono::nanoseconds{0};
   moveOrderingTime = std::chrono::nanoseconds{0};
   pruningTime = std::chrono::nanoseconds{0};
   quiescenceTime = std::chrono::nanoseconds{0};

   ttLookups = 0;
   ttHits = 0;
   moveGens = 0;
   evaluations = 0;
   seeChecks = 0;
   movesGenerated = 0;
   movesMade = 0;
   pruningAttempts = 0;
   pruningSuccesses = 0;
}

void ProfilerData::printProfileReport() const
{
   auto totalSearchTime = ttLookupTime + moveGenTime + evaluationTime +
                         seeTime + moveMakingTime + moveOrderingTime +
                         pruningTime + quiescenceTime;

   std::cout << "\n===== SEARCH PROFILING REPORT =====\n";
   std::cout << "Total profiled time: " << std::chrono::duration_cast<std::chrono::milliseconds>(totalSearchTime).count() << " ms\n\n";

   // Print timing information
   std::cout << "TIME BREAKDOWN:\n";
   std::cout << "TT Lookup:      " << std::chrono::duration_cast<std::chrono::milliseconds>(ttLookupTime).count()
            << " ms (" << (ttLookupTime.count() * 100.0 / totalSearchTime.count()) << "%)\n";
   std::cout << "Move Generation: " << std::chrono::duration_cast<std::chrono::milliseconds>(moveGenTime).count()
            << " ms (" << (moveGenTime.count() * 100.0 / totalSearchTime.count()) << "%)\n";
   std::cout << "Evaluation:      " << std::chrono::duration_cast<std::chrono::milliseconds>(evaluationTime).count()
            << " ms (" << (evaluationTime.count() * 100.0 / totalSearchTime.count()) << "%)\n";
   std::cout << "SEE Checks:      " << std::chrono::duration_cast<std::chrono::milliseconds>(seeTime).count()
            << " ms (" << (seeTime.count() * 100.0 / totalSearchTime.count()) << "%)\n";
   std::cout << "Move Making:     " << std::chrono::duration_cast<std::chrono::milliseconds>(moveMakingTime).count()
            << " ms (" << (moveMakingTime.count() * 100.0 / totalSearchTime.count()) << "%)\n";
   std::cout << "Move Ordering:   " << std::chrono::duration_cast<std::chrono::milliseconds>(moveOrderingTime).count()
            << " ms (" << (moveOrderingTime.count() * 100.0 / totalSearchTime.count()) << "%)\n";
   std::cout << "Pruning:         " << std::chrono::duration_cast<std::chrono::milliseconds>(pruningTime).count()
            << " ms (" << (pruningTime.count() * 100.0 / totalSearchTime.count()) << "%)\n";
   std::cout << "Quiescence:      " << std::chrono::duration_cast<std::chrono::milliseconds>(quiescenceTime).count()
            << " ms (" << (quiescenceTime.count() * 100.0 / totalSearchTime.count()) << "%)\n";

   // Print counter information
   std::cout << "\nCOUNTERS:\n";
   std::cout << "TT Lookups:      " << ttLookups << " (Hit rate: " << (ttHits * 100.0 / (ttLookups ? ttLookups : 1)) << "%)\n";
   std::cout << "Move Generations: " << moveGens << " (Avg moves: " << (movesGenerated * 1.0 / (moveGens ? moveGens : 1)) << ")\n";
   std::cout << "Evaluations:      " << evaluations << "\n";
   std::cout << "SEE Checks:       " << seeChecks << "\n";
   std::cout << "Moves Made:       " << movesMade << "\n";
   std::cout << "Pruning Attempts: " << pruningAttempts << " (Success rate: "
            << (pruningSuccesses * 100.0 / (pruningAttempts ? pruningAttempts : 1)) << "%)\n";

   // Print efficiency metrics
   std::cout << "\nEFFICIENCY METRICS:\n";
   std::cout << "Time per evaluation: " << (evaluations ? evaluationTime.count() / evaluations : 0) << " ns\n";
   std::cout << "Time per move made:  " << (movesMade ? moveMakingTime.count() / movesMade : 0) << " ns\n";
   std::cout << "Time per TT lookup:  " << (ttLookups ? ttLookupTime.count() / ttLookups : 0) << " ns\n";
   std::cout << "Time per SEE check:  " << (seeChecks ? seeTime.count() / seeChecks : 0) << " ns\n";
   std::cout << "==============================\n";
}

ScopedTimer::~ScopedTimer()
{
   auto end = std::chrono::high_resolution_clock::now();
   target += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
}

} // namespace Chess