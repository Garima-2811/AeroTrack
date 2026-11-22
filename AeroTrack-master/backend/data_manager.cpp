
#include "data_manager.h"

#include <cctype>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

namespace
{

bool isValidField(const string& value)
{
    if (value.find('|') != string::npos ||
        value.find('\n') != string::npos ||
        value.find('\r') != string::npos)
        return false;

    for (string::const_iterator character = value.begin();
         character != value.end(); ++character)
    {
        if (!isspace(static_cast<unsigned char>(*character)))
            return true;
    }

    return false;
}

bool isValidBaggage(const Baggage& bag, string& reason)
{
    if (bag.bagId <= 0)
        reason = "bag ID must be a positive integer";
    else if (!isValidField(bag.passengerName))
        reason = "passenger name is empty or contains an invalid delimiter";
    else if (!isValidField(bag.flightNo))
        reason = "flight number is empty or contains an invalid delimiter";
    else if (!isValidField(bag.destination))
        reason = "destination is empty or contains an invalid delimiter";
    else if (!isValidField(bag.status))
        reason = "status is empty or contains an invalid delimiter";
    else
        return true;

    return false;
}

bool parseBagId(const string& value, int& id)
{
    if (value.empty())
        return false;

    for (string::const_iterator character = value.begin();
         character != value.end(); ++character)
    {
        if (!isdigit(static_cast<unsigned char>(*character)))
            return false;
    }

    istringstream input(value);
    char extraCharacter;
    return static_cast<bool>(input >> id) && !(input >> extraCharacter);
}

} // namespace

bool DataManager::addBaggage(Baggage bag) {
    string reason;
    if (!isValidBaggage(bag, reason)) {
        lastErrorMessage = "Invalid baggage record: " + reason + ".";
        return false;
    }

    if (bagTable.find(bag.bagId) != bagTable.end()) {
        lastErrorMessage = "A baggage record with this ID already exists.";
        return false;
    }

    bags.push_back(bag);
    try {
        bagTable.emplace(bag.bagId, bags.size() - 1);
    }
    catch (...) {
        bags.pop_back();
        throw;
    }

    lastErrorMessage.clear();
    return true;
}

bool DataManager::searchBaggage(int id, Baggage &result) {
    auto it = bagTable.find(id);

    if (it == bagTable.end()) {
        lastErrorMessage = "No baggage record exists with this ID.";
        return false;
    }

    result = bags[it->second];
    lastErrorMessage.clear();
    return true;
}

bool DataManager::updateStatus(int id, string status) {
    auto it = bagTable.find(id);

    if (it == bagTable.end()) {
        lastErrorMessage = "No baggage record exists with this ID.";
        return false;
    }

    if (!isValidField(status)) {
        lastErrorMessage =
            "Invalid status: it is empty or contains an invalid delimiter.";
        return false;
    }

    bags[it->second].status = status;
    lastErrorMessage.clear();
    return true;
}

const vector<Baggage>& DataManager::getBaggageRecords() const {
    return bags;
}

void DataManager::displayBaggage() {
    if (bags.empty()) {
        cout << "No baggage records found.\n";
        return;
    }

    for (const auto &bag : bags) {
        cout << "\nBag ID: " << bag.bagId
             << "\nPassenger: " << bag.passengerName
             << "\nFlight: " << bag.flightNo
             << "\nDestination: " << bag.destination
             << "\nStatus: " << bag.status << "\n";
    }
}

bool DataManager::saveToFile() {
    ofstream file("data/baggage.txt");

    if (!file.is_open()) {
        lastErrorMessage = "Unable to open data/baggage.txt for writing.";
        return false;
    }

    for (const auto &bag : bags) {
        file << bag.bagId << '|'
             << bag.passengerName << '|'
             << bag.flightNo << '|'
             << bag.destination << '|'
             << bag.status << '\n';

        if (!file) {
            lastErrorMessage = "Error writing baggage data to data/baggage.txt.";
            return false;
        }
    }

    file.close();
    if (file.fail()) {
        lastErrorMessage = "Error closing data/baggage.txt after writing.";
        return false;
    }

    lastErrorMessage.clear();
    return true;
}

bool DataManager::loadFromFile() {
    ifstream file("data/baggage.txt");

    if (!file.is_open()) {
        lastErrorMessage = "Unable to open data/baggage.txt for reading.";
        return false;
    }

    vector<Baggage> loadedBags;
    unordered_map<int, size_t> loadedBagTable;
    string line;
    size_t lineNumber = 0;

    while (getline(file, line)) {
        ++lineNumber;
        stringstream ss(line);
        string id, name, flight, destination, status;

        if (getline(ss, id, '|') &&
            getline(ss, name, '|') &&
            getline(ss, flight, '|') &&
            getline(ss, destination, '|') &&
            getline(ss, status)) {
            Baggage bag;
            string reason;
            if (!parseBagId(id, bag.bagId)) {
                lastErrorMessage = "Invalid baggage data at line " +
                    to_string(lineNumber) + ": bag ID must be a positive integer.";
                return false;
            }

            bag.passengerName = name;
            bag.flightNo = flight;
            bag.destination = destination;
            bag.status = status;

            if (!isValidBaggage(bag, reason)) {
                lastErrorMessage = "Invalid baggage data at line " +
                    to_string(lineNumber) + ": " + reason + ".";
                return false;
            }

            if (loadedBagTable.find(bag.bagId) != loadedBagTable.end()) {
                lastErrorMessage = "Invalid baggage data at line " +
                    to_string(lineNumber) + ": duplicate bag ID.";
                return false;
            }

            loadedBags.push_back(bag);
            loadedBagTable.emplace(bag.bagId, loadedBags.size() - 1);
        }
        else {
            lastErrorMessage = "Invalid baggage data at line " +
                to_string(lineNumber) + ": expected five pipe-delimited fields.";
            return false;
        }
    }

    if (file.bad()) {
        lastErrorMessage = "Error reading baggage data from data/baggage.txt.";
        return false;
    }

    bags.swap(loadedBags);
    bagTable.swap(loadedBagTable);
    lastErrorMessage.clear();
    return true;
}

const string& DataManager::lastError() const {
    return lastErrorMessage;
}
