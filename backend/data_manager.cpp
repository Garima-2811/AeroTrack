
#include "data_manager.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

bool DataManager::addBaggage(Baggage bag) {
    if (bagTable.find(bag.bagId) != bagTable.end())
        return false;

    bags.push_back(bag);
    bagTable[bag.bagId] = bags.size() - 1;
    return true;
}

bool DataManager::searchBaggage(int id, Baggage &result) {
    auto it = bagTable.find(id);

    if (it == bagTable.end())
        return false;

    result = bags[it->second];
    return true;
}

bool DataManager::updateStatus(int id, string status) {
    auto it = bagTable.find(id);

    if (it == bagTable.end())
        return false;

    bags[it->second].status = status;
    return true;
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

    if (!file.is_open())
        return false;

    for (const auto &bag : bags) {
        file << bag.bagId << '|'
             << bag.passengerName << '|'
             << bag.flightNo << '|'
             << bag.destination << '|'
             << bag.status << '\n';
    }

    return file.good();
}

bool DataManager::loadFromFile() {
    ifstream file("data/baggage.txt");

    if (!file.is_open())
        return false;

    bags.clear();
    bagTable.clear();

    string line;

    while (getline(file, line)) {
        stringstream ss(line);
        string id, name, flight, destination, status;

        if (getline(ss, id, '|') &&
            getline(ss, name, '|') &&
            getline(ss, flight, '|') &&
            getline(ss, destination, '|') &&
            getline(ss, status)) {
            try {
                Baggage bag = {
                    stoi(id), name, flight, destination, status
                };

                addBaggage(bag);
            }
            catch (...) {
                continue;
            }
        }
    }

    return true;
}
