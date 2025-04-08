#pragma once
#include <chrono>
#include "chess.hpp"
#include <iostream>
#include <string>
#include <vector>
#include "search.hpp"
#include "types.hpp"
#include "tt.hpp"
// Performance test for move generation
uint64_t perft(Board &board, int depth);

// Benchmark function for perft
void benchPerft(int maxDepth = 6);

// Benchmark the search algorithm
void benchSearch(int maxDepth, TranspositionTable* table);