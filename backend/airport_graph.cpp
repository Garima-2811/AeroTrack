#include "airport_graph.h"

#include <cmath>
#include <iomanip>
#include <limits>
#include <queue>
#include <sstream>

namespace aerotrack
{
namespace
{

const double kWalkingSpeedMetresPerSecond = 1.4;
const double kGeometryEpsilon = 1e-9;

struct QueueEntry
{
    double distance;
    std::size_t location;
};

struct QueueEntryGreater
{
    bool operator()(const QueueEntry& left, const QueueEntry& right) const
    {
        return left.distance > right.distance;
    }
};

std::string turnInstruction(const Location& previous,
                            const Location& current,
                            const Location& next)
{
    const double incomingX = current.x - previous.x;
    const double incomingY = current.y - previous.y;
    const double outgoingX = next.x - current.x;
    const double outgoingY = next.y - current.y;
    const double crossProduct =
        incomingX * outgoingY - incomingY * outgoingX;
    const double dotProduct =
        incomingX * outgoingX + incomingY * outgoingY;

    if (std::fabs(crossProduct) <= kGeometryEpsilon)
    {
        if (dotProduct < 0.0)
            return "turn around";
        return "continue straight";
    }

    return crossProduct > 0.0 ? "turn left" : "turn right";
}

bool readSelection(std::istream& input,
                   std::ostream& output,
                   const std::vector<std::string>& names,
                   const std::string& prompt,
                   std::string& selectedName)
{
    while (true)
    {
        output << prompt;
        std::string line;
        if (!std::getline(input, line))
            return false;

        std::istringstream parser(line);
        std::size_t choice;
        char extraCharacter;
        if ((parser >> choice) && !(parser >> extraCharacter) &&
            choice >= 1 && choice <= names.size())
        {
            selectedName = names[choice - 1];
            return true;
        }

        output << "Invalid location selection. Enter a number from 1 to "
               << names.size() << ".\n";
    }
}

void displayLocations(const std::vector<std::string>& names,
                      std::ostream& output)
{
    for (std::size_t index = 0; index < names.size(); ++index)
        output << index + 1 << ". " << names[index] << '\n';
}

} // namespace

RouteResult::RouteResult()
    : status(DestinationUnreachable),
      totalDistanceMetres(0.0),
      estimatedWalkingMinutes(0.0)
{
}

bool AirportGraph::addLocation(const std::string& name, double x, double y)
{
    if (name.empty() || !std::isfinite(x) || !std::isfinite(y) ||
        findLocation(name) != locations_.size())
        return false;

    Location location;
    location.name = name;
    location.x = x;
    location.y = y;
    locations_.push_back(location);
    corridors_.push_back(std::vector<Corridor>());
    return true;
}

bool AirportGraph::addCorridor(const std::string& from,
                               const std::string& to,
                               double distanceMetres)
{
    const std::size_t fromIndex = findLocation(from);
    const std::size_t toIndex = findLocation(to);
    if (fromIndex == locations_.size() || toIndex == locations_.size() ||
        fromIndex == toIndex || !std::isfinite(distanceMetres) ||
        distanceMetres <= 0.0)
        return false;

    Corridor forward;
    forward.destination = toIndex;
    forward.distanceMetres = distanceMetres;
    corridors_[fromIndex].push_back(forward);

    Corridor reverse;
    reverse.destination = fromIndex;
    reverse.distanceMetres = distanceMetres;
    corridors_[toIndex].push_back(reverse);
    return true;
}

std::vector<std::string> AirportGraph::locationNames() const
{
    std::vector<std::string> names;
    for (std::size_t index = 0; index < locations_.size(); ++index)
        names.push_back(locations_[index].name);
    return names;
}

std::size_t AirportGraph::findLocation(const std::string& name) const
{
    for (std::size_t index = 0; index < locations_.size(); ++index)
    {
        if (locations_[index].name == name)
            return index;
    }
    return locations_.size();
}

double AirportGraph::corridorDistance(std::size_t from,
                                      std::size_t to) const
{
    const std::vector<Corridor>& adjacent = corridors_[from];
    for (std::size_t index = 0; index < adjacent.size(); ++index)
    {
        if (adjacent[index].destination == to)
            return adjacent[index].distanceMetres;
    }
    return 0.0;
}

std::vector<std::string> AirportGraph::createInstructions(
    const std::vector<std::size_t>& route) const
{
    std::vector<std::string> instructions;
    if (route.size() < 2)
        return instructions;

    std::size_t start = 0;
    while (start + 1 < route.size())
    {
        std::size_t next = start + 1;
        double segmentDistance = corridorDistance(route[start], route[next]);
        while (next + 1 < route.size() &&
               turnInstruction(locations_[route[next - 1]],
                               locations_[route[next]],
                               locations_[route[next + 1]]) ==
                   "continue straight")
        {
            ++next;
            segmentDistance += corridorDistance(route[next - 1], route[next]);
        }

        std::ostringstream instruction;
        instruction << "Continue straight for " << std::fixed
                    << std::setprecision(1) << segmentDistance
                    << " metres";
        if (next + 1 < route.size())
        {
            instruction << ", then "
                        << turnInstruction(locations_[route[next - 1]],
                                           locations_[route[next]],
                                           locations_[route[next + 1]])
                        << " at " << locations_[route[next]].name
                        << " towards " << locations_[route[next + 1]].name;
        }
        else
        {
            instruction << " to " << locations_[route[next]].name;
        }
        instructions.push_back(instruction.str());
        start = next;
    }

    return instructions;
}

RouteResult AirportGraph::shortestRoute(const std::string& start,
                                        const std::string& destination) const
{
    RouteResult result;
    const std::size_t startIndex = findLocation(start);
    const std::size_t destinationIndex = findLocation(destination);

    if (startIndex == locations_.size())
    {
        result.status = InvalidStartLocation;
        return result;
    }
    if (destinationIndex == locations_.size())
    {
        result.status = InvalidDestinationLocation;
        return result;
    }

    const double infinity = std::numeric_limits<double>::infinity();
    std::vector<double> distances(locations_.size(), infinity);
    std::vector<std::size_t> previous(
        locations_.size(), locations_.size());
    std::priority_queue<QueueEntry, std::vector<QueueEntry>,
                        QueueEntryGreater> pending;

    distances[startIndex] = 0.0;
    QueueEntry initial;
    initial.distance = 0.0;
    initial.location = startIndex;
    pending.push(initial);

    while (!pending.empty())
    {
        const QueueEntry current = pending.top();
        pending.pop();

        if (current.distance != distances[current.location])
            continue;
        if (current.location == destinationIndex)
            break;

        const std::vector<Corridor>& adjacent =
            corridors_[current.location];
        for (std::size_t index = 0; index < adjacent.size(); ++index)
        {
            const Corridor& corridor = adjacent[index];
            const double candidate =
                current.distance + corridor.distanceMetres;
            if (candidate < distances[corridor.destination])
            {
                distances[corridor.destination] = candidate;
                previous[corridor.destination] = current.location;

                QueueEntry next;
                next.distance = candidate;
                next.location = corridor.destination;
                pending.push(next);
            }
        }
    }

    if (!std::isfinite(distances[destinationIndex]))
    {
        result.status = DestinationUnreachable;
        return result;
    }

    std::vector<std::size_t> route;
    for (std::size_t location = destinationIndex;
         location != locations_.size();
         location = previous[location])
    {
        route.push_back(location);
        if (location == startIndex)
            break;
    }
    if (route.empty() || route.back() != startIndex)
    {
        result.status = DestinationUnreachable;
        return result;
    }

    for (std::vector<std::size_t>::reverse_iterator location = route.rbegin();
         location != route.rend(); ++location)
        result.locations.push_back(locations_[*location].name);

    result.status = RouteFound;
    result.totalDistanceMetres = distances[destinationIndex];
    result.estimatedWalkingMinutes =
        result.totalDistanceMetres / kWalkingSpeedMetresPerSecond / 60.0;
    result.instructions = createInstructions(
        std::vector<std::size_t>(route.rbegin(), route.rend()));
    return result;
}

AirportGraph makeSampleAirportGraph()
{
    AirportGraph graph;
    graph.addLocation("Entrance", 0.0, 0.0);
    graph.addLocation("Check-in", 100.0, 0.0);
    graph.addLocation("Security", 200.0, 0.0);
    graph.addLocation("Gate A1", 300.0, 0.0);
    graph.addLocation("Washroom", 200.0, 80.0);
    graph.addLocation("Baggage Claim", 100.0, -80.0);

    graph.addCorridor("Entrance", "Check-in", 100.0);
    graph.addCorridor("Check-in", "Security", 80.0);
    graph.addCorridor("Entrance", "Security", 240.0);
    graph.addCorridor("Security", "Gate A1", 120.0);
    graph.addCorridor("Security", "Washroom", 45.0);
    graph.addCorridor("Check-in", "Baggage Claim", 95.0);
    graph.addCorridor("Entrance", "Baggage Claim", 150.0);
    return graph;
}

bool runNavigation(std::istream& input, std::ostream& output)
{
    const AirportGraph graph = makeSampleAirportGraph();
    const std::vector<std::string> names = graph.locationNames();
    std::string start;
    std::string destination;

    output << "AeroTrack indoor navigation - fictional sample airport layout "
              "and illustrative distances.\n";
    output << "This demo does not provide live indoor positioning or live GPS.\n";
    output << "\nAvailable locations:\n";
    displayLocations(names, output);
    if (!readSelection(input, output, names, "Choose starting location: ",
                       start) ||
        !readSelection(input, output, names, "Choose destination: ",
                       destination))
        return false;

    const RouteResult route = graph.shortestRoute(start, destination);
    if (route.status != RouteFound)
    {
        output << "No route could be calculated for those locations.\n";
        return true;
    }

    output << "\nRoute: ";
    for (std::size_t index = 0; index < route.locations.size(); ++index)
    {
        if (index != 0)
            output << " -> ";
        output << route.locations[index];
    }
    output << "\nTotal distance: " << std::fixed << std::setprecision(1)
           << route.totalDistanceMetres << " metres\n";
    output << "Estimated walking time: " << std::setprecision(1)
           << route.estimatedWalkingMinutes << " minutes (at "
           << kWalkingSpeedMetresPerSecond << " m/s)\n";
    output << "Turn-by-turn instructions:\n";
    if (route.instructions.empty())
        output << "You are already at your destination.\n";
    for (std::size_t index = 0; index < route.instructions.size(); ++index)
        output << index + 1 << ". " << route.instructions[index] << '\n';
    output << "Distances and walking time are illustrative estimates.\n";
    return true;
}

} // namespace aerotrack
