#include "uci.hpp"
#include <iostream>

int main()
{
   initLateMoveTable();
   uci_loop();
   return 0;
}