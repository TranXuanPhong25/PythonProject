#include "uci.hpp"
#include "tunable_params.hpp"
#include <iostream>

int main()
{
   if (TunableParams::load_params("benchmark_best_params.txt")) {
      std::cout << "Loaded optimized parameters from benchmark_best_params.txt" << std::endl;
   } else if (TunableParams::load_params("test/params/benchmark_best_params.txt")) {
      std::cout << "Loaded optimized parameters from test/params/benchmark_best_params.txt" << std::endl;
   } else if (TunableParams::load_params("tunable_params_current.txt")) {
      std::cout << "Loaded optimized parameters from tunable_params_current.txt" << std::endl;
   } else {
      std::cout << "Using default search parameters" << std::endl;
   }
   initLateMoveTable();
   uci_loop();
   return 0;
}