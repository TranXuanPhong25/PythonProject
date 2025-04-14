#pragma once

#include <chrono>
#include <iostream>
#include "chess.hpp"

namespace Chess {

// Profiling structure to track time spent in different parts of search
struct ProfilerData
{
   // Timers for different search components
   std::chrono::nanoseconds ttLookupTime{0};
   std::chrono::nanoseconds moveGenTime{0};
   std::chrono::nanoseconds evaluationTime{0};
   std::chrono::nanoseconds seeTime{0};
   std::chrono::nanoseconds moveMakingTime{0};
   std::chrono::nanoseconds moveOrderingTime{0};
   std::chrono::nanoseconds pruningTime{0};
   std::chrono::nanoseconds quiescenceTime{0};

   // Counters
   size_t ttLookups{0};
   size_t ttHits{0};
   size_t moveGens{0};
   size_t evaluations{0};
   size_t seeChecks{0};
   size_t movesGenerated{0};
   size_t movesMade{0};
   size_t pruningAttempts{0};
   size_t pruningSuccesses{0};

   void reset();
   void printProfileReport() const;
};

// Timer utility class for profiling sections
class ScopedTimer
{
private:
   std::chrono::time_point<std::chrono::high_resolution_clock> start;
   std::chrono::nanoseconds &target;

public:
   explicit ScopedTimer(std::chrono::nanoseconds &targetTime)
       : start(std::chrono::high_resolution_clock::now()), target(targetTime) {}

   ~ScopedTimer();
};

// Global profiling data
extern ProfilerData profiler;

// Add a macro to make timing code sections easier
#define PROFILE_SCOPE(name) ScopedTimer timer##__LINE__(profiler.name##Time)

} // namespace Chess