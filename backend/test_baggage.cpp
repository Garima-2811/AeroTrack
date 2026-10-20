#include "baggage_search.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace aerotrack;

Baggage makeBaggage(int id,
                    const std::string& owner,
                    const std::string& flight,
                    const std::string& colour,
                    const std::string& type,
                    const std::string& departure,
                    const std::string& arrival,
                    const std::string& date,
                    const std::string& departureTime,
                    const std::string& arrivalTime,
                    const std::string& checkpoint)
{
    Baggage baggage;
    baggage.id = id;
    baggage.ownerName = owner;
    baggage.flightNumber = flight;
    baggage.colour = colour;
    baggage.type = type;
    baggage.departureAirport = departure;
    baggage.arrivalAirport = arrival;
    baggage.travelDate = date;
    baggage.approximateDepartureTime = departureTime;
    baggage.approximateArrivalTime = arrivalTime;
    baggage.lastCheckpoint = checkpoint;
    return baggage;
}

std::vector<Baggage> makeSampleRecords()
{
    std::vector<Baggage> records;
    records.push_back(makeBaggage(1001, "Maya Patel", "AX101", "Dark Red",
                                  "Hard-shell", "New York", "London",
                                  "20-10-2026", "10:30 AM", "10:20 PM",
                                  "Security screening"));
    records.push_back(makeBaggage(1002, "Liam Chen", "AX202", "red",
                                  "Soft-shell", "Newark", "London",
                                  "20-10-2026", "11:00 AM", "11:00 PM",
                                  "Check-in counter"));
    records.push_back(makeBaggage(1003, "Nia Shah", "BY303", "Blue",
                                  "Hard-shell", "New York", "Paris",
                                  "21-10-2026", "08:15 AM", "09:30 PM",
                                  "Loading area"));
    records.push_back(makeBaggage(1004, "Omar Ali", "AX104", "Red stripe",
                                  "Hard-shell", "New York", "London",
                                  "20-10-2026", "10:45 AM", "10:35 PM",
                                  "Baggage claim"));
    return records;
}

void testSearchById(const std::vector<Baggage>& records)
{
    std::vector<Baggage> results = searchById(records, 1002);
    assert(results.size() == 1);
    assert(results[0].ownerName == "Liam Chen");
}

void testCaseInsensitivePartialColour(const std::vector<Baggage>& records)
{
    SearchCriteria criteria;
    criteria.colour = "rEd";

    std::vector<SearchResult> results = searchBaggage(records, criteria);
    assert(results.size() == 3);
    assert(results[0].matchPercentage == 100.0);
}

void testSearchByType(const std::vector<Baggage>& records)
{
    SearchCriteria criteria;
    criteria.type = "hard";

    std::vector<SearchResult> results = searchBaggage(records, criteria);
    assert(results.size() == 3);
}

void testUnknownFlightNumber(const std::vector<Baggage>& records)
{
    SearchCriteria criteria;
    criteria.colour = "red";
    criteria.departureAirport = "new york";
    criteria.arrivalAirport = "london";
    criteria.travelDate = "20-10";

    std::vector<SearchResult> results = searchBaggage(records, criteria);
    assert(results.size() == 3);
    assert(results[0].baggage.id == 1001);
    assert(results[1].baggage.id == 1004);
    assert(results[2].baggage.id == 1002);
    assert(results[2].matchPercentage == 50.0);
}

void testCandidatesAreSortedByScore(const std::vector<Baggage>& records)
{
    SearchCriteria criteria;
    criteria.colour = "red";
    criteria.type = "hard";
    criteria.flightNumber = "AX101";
    criteria.departureAirport = "New York";
    criteria.arrivalAirport = "London";

    std::vector<SearchResult> results = searchBaggage(records, criteria);
    assert(results.size() == 4);
    assert(results[0].baggage.id == 1001);
    assert(results[0].matchPercentage == 100.0);
    assert(results[1].matchPercentage == 75.0);
    assert(results[2].matchPercentage == 25.0);
    assert(results[3].matchPercentage == 25.0);
}

void testSkippedFieldsAffectDenominator(const std::vector<Baggage>& records)
{
    SearchCriteria criteria;
    criteria.colour = "red";
    criteria.flightNumber = "AX101";

    std::vector<SearchResult> results = searchBaggage(records, criteria);
    assert(results.size() == 3);
    assert(results[0].baggage.id == 1001);
    assert(results[0].matchPercentage == 100.0);
    assert(results[1].matchPercentage == 50.0);
    assert(results[2].matchPercentage == 50.0);
}

void testNoMatchingBaggage(const std::vector<Baggage>& records)
{
    SearchCriteria criteria;
    criteria.colour = "purple";
    criteria.type = "wooden";

    std::vector<SearchResult> results = searchBaggage(records, criteria);
    assert(results.empty());

    std::vector<SearchResult> emptySearch = searchBaggage(records,
                                                          SearchCriteria());
    assert(emptySearch.empty());
}

int main()
{
    const std::vector<Baggage> records = makeSampleRecords();

    testSearchById(records);
    testCaseInsensitivePartialColour(records);
    testSearchByType(records);
    testUnknownFlightNumber(records);
    testCandidatesAreSortedByScore(records);
    testSkippedFieldsAffectDenominator(records);
    testNoMatchingBaggage(records);

    std::cout << "All baggage search tests passed.\n";
    return 0;
}
