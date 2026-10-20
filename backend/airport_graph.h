#ifndef AEROTRACK_AIRPORT_GRAPH_H
#define AEROTRACK_AIRPORT_GRAPH_H

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

namespace aerotrack
{

struct Location
{
    std::string name;
    double x;
    double y;
};

enum RouteStatus
{
    RouteFound,
    InvalidStartLocation,
    InvalidDestinationLocation,
    DestinationUnreachable
};

struct RouteResult
{
    RouteStatus status;
    std::vector<std::string> locations;
    std::vector<std::string> instructions;
    double totalDistanceMetres;
    double estimatedWalkingMinutes;

    RouteResult();
};

class AirportGraph
{
public:
    // Locations are corridor points. Their coordinates determine turn geometry.
    bool addLocation(const std::string& name, double x, double y);
    bool addCorridor(const std::string& from,
                     const std::string& to,
                     double distanceMetres);

    std::vector<std::string> locationNames() const;
    RouteResult shortestRoute(const std::string& start,
                              const std::string& destination) const;

private:
    struct Corridor
    {
        std::size_t destination;
        double distanceMetres;
    };

    std::vector<Location> locations_;
    std::vector<std::vector<Corridor> > corridors_;

    std::size_t findLocation(const std::string& name) const;
    double corridorDistance(std::size_t from, std::size_t to) const;
    std::vector<std::string> createInstructions(
        const std::vector<std::size_t>& route) const;
};

// Returns a fictional sample layout; it is not a live airport map.
AirportGraph makeSampleAirportGraph();

// Prompts for numbered start/destination locations and displays the route.
// Returns false if input ends before both selections are made.
bool runNavigation(std::istream& input, std::ostream& output);

} // namespace aerotrack

#endif
