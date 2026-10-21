
#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <cstddef>

struct Baggage {
    int bagId;
    std::string passengerName;
    std::string flightNo;
    std::string destination;
    std::string status;
};

class DataManager {
private:
    std::vector<Baggage> bags;
    std::unordered_map<int, std::size_t> bagTable;

public:
    bool addBaggage(Baggage bag);
    bool searchBaggage(int id, Baggage &result);
    bool updateStatus(int id, std::string status);
    void displayBaggage();
    bool saveToFile();
    bool loadFromFile();
};

#endif
