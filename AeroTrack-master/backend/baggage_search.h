#ifndef AEROTRACK_BAGGAGE_SEARCH_H
#define AEROTRACK_BAGGAGE_SEARCH_H

#include <iosfwd>
#include <string>
#include <vector>

namespace aerotrack
{

struct Baggage
{
    int id;
    std::string ownerName;
    std::string flightNumber;
    std::string colour;
    std::string type;
    std::string departureAirport;
    std::string arrivalAirport;
    std::string travelDate;
    std::string approximateDepartureTime;
    std::string approximateArrivalTime;
    std::string lastCheckpoint;
};

struct SearchCriteria
{
    std::string colour;
    std::string type;
    std::string flightNumber;
    std::string departureAirport;
    std::string arrivalAirport;
    std::string travelDate;
    std::string approximateDepartureTime;
    std::string approximateArrivalTime;
};

struct SearchResult
{
    Baggage baggage;
    double matchPercentage;
};

// Returns every record whose ID exactly matches the requested ID.
std::vector<Baggage> searchById(const std::vector<Baggage>& baggageRecords,
                                int baggageId);

// Returns candidates ranked by their similarity to the supplied details.
std::vector<SearchResult> searchBaggage(
    const std::vector<Baggage>& baggageRecords,
    const SearchCriteria& criteria);

// Prints the requested candidate details, or a message when there are no results.
void displaySearchResults(const std::vector<SearchResult>& results,
                          std::ostream& output);

} // namespace aerotrack

#endif
