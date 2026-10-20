#include "airport_graph.h"

#include <iostream>

int main()
{
    return aerotrack::runNavigation(std::cin, std::cout) ? 0 : 1;
}
