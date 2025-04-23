#pragma once
#include <chrono>
#include "chess.hpp"
#include <iostream>
#include <string>
#include <vector>
#include "search.hpp"
#include "types.hpp"
#include "tt.hpp"

// Benchmark the search algorithm
void benchSearch(int maxDepth);