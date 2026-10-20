#include "baggage_search.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <ostream>

namespace aerotrack
{
namespace
{

std::string toLowerCase(const std::string& text)
{
    std::string lowerText = text;
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(),
                   [](unsigned char character)
                   {
                       return static_cast<char>(std::tolower(character));
                   });
    return lowerText;
}

bool containsIgnoreCase(const std::string& text,
                        const std::string& searchText)
{
    if (searchText.empty())
        return true;

    return toLowerCase(text).find(toLowerCase(searchText)) != std::string::npos;
}

bool hasSearchDetails(const SearchCriteria& criteria)
{
    return !criteria.colour.empty() ||
           !criteria.type.empty() ||
           !criteria.flightNumber.empty() ||
           !criteria.departureAirport.empty() ||
           !criteria.arrivalAirport.empty() ||
           !criteria.travelDate.empty() ||
           !criteria.approximateDepartureTime.empty() ||
           !criteria.approximateArrivalTime.empty();
}

bool matchesOptionalDetails(const Baggage& baggage,
                            const SearchCriteria& criteria)
{
    return containsIgnoreCase(baggage.travelDate, criteria.travelDate) &&
           containsIgnoreCase(baggage.approximateDepartureTime,
                              criteria.approximateDepartureTime) &&
           containsIgnoreCase(baggage.approximateArrivalTime,
                              criteria.approximateArrivalTime);
}

bool matchesAnySuppliedDetail(const Baggage& baggage,
                              const SearchCriteria& criteria)
{
    if (!criteria.colour.empty() &&
        containsIgnoreCase(baggage.colour, criteria.colour))
        return true;

    if (!criteria.type.empty() &&
        containsIgnoreCase(baggage.type, criteria.type))
        return true;

    if (!criteria.flightNumber.empty() &&
        containsIgnoreCase(baggage.flightNumber, criteria.flightNumber))
        return true;

    if (!criteria.departureAirport.empty() &&
        containsIgnoreCase(baggage.departureAirport,
                           criteria.departureAirport))
        return true;

    if (!criteria.arrivalAirport.empty() &&
        containsIgnoreCase(baggage.arrivalAirport,
                           criteria.arrivalAirport))
        return true;

    if (!criteria.travelDate.empty() &&
        containsIgnoreCase(baggage.travelDate, criteria.travelDate))
        return true;

    if (!criteria.approximateDepartureTime.empty() &&
        containsIgnoreCase(baggage.approximateDepartureTime,
                           criteria.approximateDepartureTime))
        return true;

    if (!criteria.approximateArrivalTime.empty() &&
        containsIgnoreCase(baggage.approximateArrivalTime,
                           criteria.approximateArrivalTime))
        return true;

    return false;
}

double calculateMatchPercentage(const Baggage& baggage,
                                const SearchCriteria& criteria)
{
    int availablePoints = 0;
    int matchedPoints = 0;

    if (!criteria.colour.empty())
    {
        availablePoints += 25;
        if (containsIgnoreCase(baggage.colour, criteria.colour))
            matchedPoints += 25;
    }

    if (!criteria.type.empty())
    {
        availablePoints += 25;
        if (containsIgnoreCase(baggage.type, criteria.type))
            matchedPoints += 25;
    }

    if (!criteria.flightNumber.empty())
    {
        availablePoints += 25;
        if (containsIgnoreCase(baggage.flightNumber, criteria.flightNumber))
            matchedPoints += 25;
    }

    // Route is one 25-point category and is available only when both airports
    // were supplied. A single supplied airport can still identify a candidate.
    if (!criteria.departureAirport.empty() &&
        !criteria.arrivalAirport.empty())
    {
        availablePoints += 25;
        if (containsIgnoreCase(baggage.departureAirport,
                               criteria.departureAirport) &&
            containsIgnoreCase(baggage.arrivalAirport,
                               criteria.arrivalAirport))
            matchedPoints += 25;
    }

    if (availablePoints == 0)
        return 0.0;

    return matchedPoints * 100.0 / availablePoints;
}

} // namespace

std::vector<Baggage> searchById(const std::vector<Baggage>& baggageRecords,
                                int baggageId)
{
    std::vector<Baggage> matches;

    for (std::size_t index = 0; index < baggageRecords.size(); ++index)
    {
        if (baggageRecords[index].id == baggageId)
            matches.push_back(baggageRecords[index]);
    }

    return matches;
}

std::vector<SearchResult> searchBaggage(
    const std::vector<Baggage>& baggageRecords,
    const SearchCriteria& criteria)
{
    std::vector<SearchResult> results;

    if (!hasSearchDetails(criteria))
        return results;

    for (std::size_t index = 0; index < baggageRecords.size(); ++index)
    {
        const Baggage& baggage = baggageRecords[index];

        // Date and time are optional filters, not weighted score categories.
        if (!matchesOptionalDetails(baggage, criteria))
            continue;

        // Avoid returning unrelated records when all weighted details miss.
        if (!matchesAnySuppliedDetail(baggage, criteria))
            continue;

        SearchResult result;
        result.baggage = baggage;
        result.matchPercentage = calculateMatchPercentage(baggage, criteria);
        results.push_back(result);
    }

    std::sort(results.begin(), results.end(),
              [](const SearchResult& left, const SearchResult& right)
              {
                  return left.matchPercentage > right.matchPercentage;
              });

    return results;
}

void displaySearchResults(const std::vector<SearchResult>& results,
                          std::ostream& output)
{
    if (results.empty())
    {
        output << "No matching baggage records found.\n";
        return;
    }

    output << std::fixed << std::setprecision(1);

    for (std::size_t index = 0; index < results.size(); ++index)
    {
        const Baggage& baggage = results[index].baggage;
        output << "\nBaggage ID: " << baggage.id << '\n';
        output << "Owner: " << baggage.ownerName << '\n';
        output << "Flight: " << baggage.flightNumber << '\n';
        output << "Colour: " << baggage.colour << '\n';
        output << "Type: " << baggage.type << '\n';
        output << "Route: " << baggage.departureAirport << " to "
             << baggage.arrivalAirport << '\n';
         output << "Last checkpoint: " << baggage.lastCheckpoint << '\n';
        output << "Match percentage: " << results[index].matchPercentage
               << "%\n";
    }

    output << "\nScore = matched points / available points * 100.\n";
    output << "This score measures similarity to the supplied details; it is "
            << "not the probability of ownership.\n";
}

} // namespace aerotrack
