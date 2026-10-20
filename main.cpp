#include "backend/baggage_search.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using namespace aerotrack;

vector<Baggage> makeSampleRecords()
{
    vector<Baggage> records;

    Baggage first;
    first.id = 1001;
    first.ownerName = "Maya Patel";
    first.flightNumber = "AX101";
    first.colour = "Dark Red";
    first.type = "Hard-shell";
    first.departureAirport = "New York";
    first.arrivalAirport = "London";
    first.travelDate = "20-10-2026";
    first.approximateDepartureTime = "10:30 AM";
    first.approximateArrivalTime = "10:20 PM";
    first.lastCheckpoint = "Security screening";
    records.push_back(first);

    Baggage second;
    second.id = 1002;
    second.ownerName = "Liam Chen";
    second.flightNumber = "AX202";
    second.colour = "Red";
    second.type = "Soft-shell";
    second.departureAirport = "Newark";
    second.arrivalAirport = "London";
    second.travelDate = "20-10-2026";
    second.approximateDepartureTime = "11:00 AM";
    second.approximateArrivalTime = "11:00 PM";
    second.lastCheckpoint = "Check-in counter";
    records.push_back(second);

    Baggage third;
    third.id = 1003;
    third.ownerName = "Nia Shah";
    third.flightNumber = "BY303";
    third.colour = "Blue";
    third.type = "Hard-shell";
    third.departureAirport = "New York";
    third.arrivalAirport = "Paris";
    third.travelDate = "21-10-2026";
    third.approximateDepartureTime = "08:15 AM";
    third.approximateArrivalTime = "09:30 PM";
    third.lastCheckpoint = "Loading area";
    records.push_back(third);

    Baggage fourth;
    fourth.id = 1004;
    fourth.ownerName = "Omar Ali";
    fourth.flightNumber = "AX104";
    fourth.colour = "Red stripe";
    fourth.type = "Hard-shell";
    fourth.departureAirport = "New York";
    fourth.arrivalAirport = "London";
    fourth.travelDate = "20-10-2026";
    fourth.approximateDepartureTime = "10:45 AM";
    fourth.approximateArrivalTime = "10:35 PM";
    fourth.lastCheckpoint = "Baggage claim";
    records.push_back(fourth);

    return records;
}

bool readInteger(const string& prompt, int& value)
{
    while (true)
    {
        cout << prompt;
        string line;
        if (!getline(cin, line))
            return false;

        stringstream input(line);
        char extraCharacter;
        if ((input >> value) && !(input >> extraCharacter))
            return true;

        cout << "Please enter a whole number.\n";
    }
}

bool readTextField(const string& prompt, string& value)
{
    cout << prompt;
    return static_cast<bool>(getline(cin, value));
}

void displayIdMatches(const vector<Baggage>& matches)
{
    if (matches.empty())
    {
        cout << "No matching baggage record found.\n";
        return;
    }

    for (size_t index = 0; index < matches.size(); ++index)
    {
        const Baggage& baggage = matches[index];
        cout << "\nBaggage ID: " << baggage.id << '\n';
        cout << "Owner: " << baggage.ownerName << '\n';
        cout << "Flight: " << baggage.flightNumber << '\n';
        cout << "Colour: " << baggage.colour << '\n';
        cout << "Type: " << baggage.type << '\n';
        cout << "Route: " << baggage.departureAirport << " to "
             << baggage.arrivalAirport << '\n';
        cout << "Last checkpoint: " << baggage.lastCheckpoint << '\n';
    }
}

void displayAllBaggageRecords(const vector<Baggage>& baggageRecords)
{
    for (size_t index = 0; index < baggageRecords.size(); ++index)
    {
        const Baggage& baggage = baggageRecords[index];
        cout << "\nBaggage ID: " << baggage.id << '\n';
        cout << "Owner: " << baggage.ownerName << '\n';
        cout << "Flight: " << baggage.flightNumber << '\n';
        cout << "Colour: " << baggage.colour << '\n';
        cout << "Type: " << baggage.type << '\n';
        cout << "Departure: " << baggage.departureAirport << '\n';
        cout << "Arrival: " << baggage.arrivalAirport << '\n';
        cout << "Travel date: " << baggage.travelDate << '\n';
        cout << "Departure time: " << baggage.approximateDepartureTime << '\n';
        cout << "Arrival time: " << baggage.approximateArrivalTime << '\n';
        cout << "Last checkpoint: " << baggage.lastCheckpoint << '\n';
    }
}

int main()
{
    const vector<Baggage> baggageRecords = makeSampleRecords();

    cout << "===== AEROTRACK BAGGAGE SEARCH =====\n";
    cout << "Demo records are loaded. Leave a detail blank if unknown.\n";

    while (true)
    {
        cout << "\n1. Search baggage by ID\n";
        cout << "2. Search using remembered details\n";
        cout << "3. Display all sample baggage records\n";
        cout << "4. Exit\n";

        int choice;
        if (!readInteger("Enter choice: ", choice))
        {
            cout << "\nInput ended. Exiting AeroTrack.\n";
            break;
        }

        if (choice == 1)
        {
            int baggageId;
            if (!readInteger("Enter baggage ID: ", baggageId))
            {
                cout << "\nInput ended. Exiting AeroTrack.\n";
                break;
            }

            displayIdMatches(searchById(baggageRecords, baggageId));
        }
        else if (choice == 2)
        {
            SearchCriteria criteria;
            if (!readTextField("Colour (blank if unknown): ", criteria.colour) ||
                !readTextField("Baggage type (blank if unknown): ", criteria.type) ||
                !readTextField("Flight number (blank if unknown): ",
                               criteria.flightNumber) ||
                !readTextField("Departure airport (blank if unknown): ",
                               criteria.departureAirport) ||
                !readTextField("Arrival airport (blank if unknown): ",
                               criteria.arrivalAirport) ||
                !readTextField("Travel date (blank if unknown): ",
                               criteria.travelDate) ||
                !readTextField("Approximate departure time (blank if unknown): ",
                               criteria.approximateDepartureTime) ||
                !readTextField("Approximate arrival time (blank if unknown): ",
                               criteria.approximateArrivalTime))
            {
                cout << "\nInput ended. Exiting AeroTrack.\n";
                break;
            }

            displaySearchResults(searchBaggage(baggageRecords, criteria), cout);
        }
        else if (choice == 3)
        {
            displayAllBaggageRecords(baggageRecords);
        }
        else if (choice == 4)
        {
            cout << "Thank you for using AeroTrack!\n";
            break;
        }
        else
        {
            cout << "Invalid choice. Please choose 1, 2, 3, or 4.\n";
        }
    }

    return 0;
}
