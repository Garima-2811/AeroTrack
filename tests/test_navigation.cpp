#include "../backend/airport_graph.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>

using namespace aerotrack;

void testShortestRoute()
{
    const AirportGraph graph = makeSampleAirportGraph();
    const RouteResult route = graph.shortestRoute("Entrance", "Gate A1");

    assert(route.status == RouteFound);
    assert(route.locations.size() == 4);
    assert(route.locations[0] == "Entrance");
    assert(route.locations[1] == "Check-in");
    assert(route.locations[2] == "Security");
    assert(route.locations[3] == "Gate A1");
    assert(std::fabs(route.totalDistanceMetres - 300.0) < 1e-9);
    assert(std::fabs(route.estimatedWalkingMinutes -
                     (300.0 / 1.4 / 60.0)) < 1e-9);
}

void testUnreachableDestination()
{
    AirportGraph graph;
    assert(graph.addLocation("Entrance", 0.0, 0.0));
    assert(graph.addLocation("Remote Gate", 10.0, 10.0));

    const RouteResult route = graph.shortestRoute("Entrance", "Remote Gate");
    assert(route.status == DestinationUnreachable);
    assert(route.locations.empty());
}

void testInvalidLocationsAndGraphInputs()
{
    const AirportGraph graph = makeSampleAirportGraph();
    assert(graph.shortestRoute("Unknown", "Gate A1").status ==
           InvalidStartLocation);
    assert(graph.shortestRoute("Entrance", "Unknown").status ==
           InvalidDestinationLocation);

    AirportGraph editableGraph;
    assert(editableGraph.addLocation("Entrance", 0.0, 0.0));
    assert(!editableGraph.addLocation("Entrance", 1.0, 1.0));
    assert(!editableGraph.addLocation("Invalid", 0.0,
                                      std::numeric_limits<double>::infinity()));
    assert(!editableGraph.addCorridor("Entrance", "Missing", 10.0));
    assert(!editableGraph.addCorridor("Entrance", "Entrance", 10.0));
    assert(!editableGraph.addCorridor("Entrance", "Missing", -1.0));
}

void testTurnInstructionsUseGeometry()
{
    const AirportGraph graph = makeSampleAirportGraph();
    const RouteResult route = graph.shortestRoute("Entrance", "Washroom");

    assert(route.status == RouteFound);
    assert(route.locations.size() == 4);
    assert(route.locations[0] == "Entrance");
    assert(route.locations[1] == "Check-in");
    assert(route.locations[2] == "Security");
    assert(route.locations[3] == "Washroom");
    assert(route.instructions.size() == 2);
    assert(route.instructions[0] ==
           "Continue straight for 180.0 metres, then turn left at Security "
           "towards Washroom");
    assert(route.instructions[1] ==
           "Continue straight for 45.0 metres to Washroom");
}

void testManualLocationSelection()
{
    std::istringstream input("0\n1\n5\n");
    std::ostringstream output;

    assert(runNavigation(input, output));
    assert(output.str().find("fictional sample airport layout") !=
           std::string::npos);
    assert(output.str().find("does not provide live indoor positioning") !=
           std::string::npos);
    assert(output.str().find("Invalid location selection") !=
           std::string::npos);
    assert(output.str().find("then turn left at Security towards Washroom") !=
           std::string::npos);
    std::istringstream endedInput;
    assert(!runNavigation(endedInput, output));
}

int main()
{
    testShortestRoute();
    testUnreachableDestination();
    testInvalidLocationsAndGraphInputs();
    testTurnInstructionsUseGeometry();
    testManualLocationSelection();
    std::cout << "All navigation tests passed.\n";
    return 0;
}
