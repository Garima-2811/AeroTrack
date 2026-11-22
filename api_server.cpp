#include "airport_graph.h"
#include "baggage_history.h"
#include "baggage_search.h"
#include "data_manager.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{

const unsigned short kPort = 8080;
const std::size_t kMaximumRequestSize = 16384;

struct HttpRequest
{
    std::string method;
    std::string path;
    std::vector<std::pair<std::string, std::string> > parameters;
};

struct HttpResponse
{
    int status;
    std::string reason;
    std::string body;
};

std::vector<aerotrack::Baggage> makeSampleRecords()
{
    std::vector<aerotrack::Baggage> records;

    aerotrack::Baggage baggage = {};
    baggage.id = 1001;
    baggage.ownerName = "Maya Patel";
    baggage.flightNumber = "AX101";
    baggage.colour = "Dark Red";
    baggage.type = "Hard-shell";
    baggage.departureAirport = "New York";
    baggage.arrivalAirport = "London";
    baggage.travelDate = "20-10-2026";
    baggage.approximateDepartureTime = "10:30 AM";
    baggage.approximateArrivalTime = "10:20 PM";
    baggage.lastCheckpoint = "Security screening";
    records.push_back(baggage);

    baggage.id = 1002;
    baggage.ownerName = "Liam Chen";
    baggage.flightNumber = "AX202";
    baggage.colour = "Red";
    baggage.type = "Soft-shell";
    baggage.departureAirport = "Newark";
    baggage.arrivalAirport = "London";
    baggage.travelDate = "20-10-2026";
    baggage.approximateDepartureTime = "11:00 AM";
    baggage.approximateArrivalTime = "11:00 PM";
    baggage.lastCheckpoint = "Check-in counter";
    records.push_back(baggage);

    baggage.id = 1003;
    baggage.ownerName = "Nia Shah";
    baggage.flightNumber = "BY303";
    baggage.colour = "Blue";
    baggage.type = "Hard-shell";
    baggage.departureAirport = "New York";
    baggage.arrivalAirport = "Paris";
    baggage.travelDate = "21-10-2026";
    baggage.approximateDepartureTime = "08:15 AM";
    baggage.approximateArrivalTime = "09:30 PM";
    baggage.lastCheckpoint = "Loading area";
    records.push_back(baggage);

    baggage.id = 1004;
    baggage.ownerName = "Omar Ali";
    baggage.flightNumber = "AX104";
    baggage.colour = "Red stripe";
    baggage.type = "Hard-shell";
    baggage.departureAirport = "New York";
    baggage.arrivalAirport = "London";
    baggage.travelDate = "20-10-2026";
    baggage.approximateDepartureTime = "10:45 AM";
    baggage.approximateArrivalTime = "10:35 PM";
    baggage.lastCheckpoint = "Baggage claim";
    records.push_back(baggage);

    return records;
}

std::string jsonString(const std::string& value)
{
    std::ostringstream output;
    output << '"';
    for (std::size_t index = 0; index < value.size(); ++index)
    {
        const unsigned char character =
            static_cast<unsigned char>(value[index]);
        switch (character)
        {
        case '"': output << "\\\""; break;
        case '\\': output << "\\\\"; break;
        case '\b': output << "\\b"; break;
        case '\f': output << "\\f"; break;
        case '\n': output << "\\n"; break;
        case '\r': output << "\\r"; break;
        case '\t': output << "\\t"; break;
        default:
            if (character < 0x20)
                output << "\\u" << std::hex << std::setw(4)
                       << std::setfill('0') << static_cast<int>(character)
                       << std::dec << std::setfill(' ');
            else
                output << static_cast<char>(character);
        }
    }
    output << '"';
    return output.str();
}

std::string urlDecode(const std::string& value, bool& valid)
{
    std::string decoded;
    for (std::size_t index = 0; index < value.size(); ++index)
    {
        if (value[index] == '+')
        {
            decoded += ' ';
        }
        else if (value[index] == '%')
        {
            if (index + 2 >= value.size())
            {
                valid = false;
                return std::string();
            }
            char* end = NULL;
            const std::string hex = value.substr(index + 1, 2);
            const long character = std::strtol(hex.c_str(), &end, 16);
            if (end == hex.c_str() || *end != '\0')
            {
                valid = false;
                return std::string();
            }
            decoded += static_cast<char>(character);
            index += 2;
        }
        else
        {
            decoded += value[index];
        }
    }
    return decoded;
}

bool parseRequest(const std::string& raw, HttpRequest& request)
{
    const std::size_t lineEnd = raw.find("\r\n");
    if (lineEnd == std::string::npos)
        return false;

    std::istringstream firstLine(raw.substr(0, lineEnd));
    std::string target;
    std::string version;
    if (!(firstLine >> request.method >> target >> version) ||
        version.compare(0, 5, "HTTP/") != 0)
        return false;

    const std::size_t queryStart = target.find('?');
    request.path = target.substr(0, queryStart);
    if (queryStart == std::string::npos)
        return true;

    const std::string query = target.substr(queryStart + 1);
    std::size_t fieldStart = 0;
    while (fieldStart <= query.size())
    {
        const std::size_t fieldEnd = query.find('&', fieldStart);
        const std::string field = query.substr(
            fieldStart, fieldEnd == std::string::npos
                            ? std::string::npos
                            : fieldEnd - fieldStart);
        if (!field.empty())
        {
            const std::size_t equals = field.find('=');
            bool valid = true;
            const std::string name = urlDecode(
                field.substr(0, equals), valid);
            const std::string value =
                equals == std::string::npos
                    ? std::string()
                    : urlDecode(field.substr(equals + 1), valid);
            if (!valid)
                return false;
            request.parameters.push_back(std::make_pair(name, value));
        }
        if (fieldEnd == std::string::npos)
            break;
        fieldStart = fieldEnd + 1;
    }
    return true;
}

std::string parameter(const HttpRequest& request, const std::string& name)
{
    for (std::size_t index = 0; index < request.parameters.size(); ++index)
    {
        if (request.parameters[index].first == name)
            return request.parameters[index].second;
    }
    return std::string();
}

HttpResponse response(int status,
                      const std::string& reason,
                      const std::string& body)
{
    HttpResponse result = {status, reason, body};
    return result;
}

HttpResponse errorResponse(int status,
                           const std::string& reason,
                           const std::string& message)
{
    return response(status, reason,
                    "{\"error\":" + jsonString(message) + "}");
}

std::string baggageJson(const aerotrack::Baggage& baggage)
{
    std::ostringstream output;
    output << "{\"id\":" << baggage.id
           << ",\"ownerName\":" << jsonString(baggage.ownerName)
           << ",\"flightNumber\":" << jsonString(baggage.flightNumber)
           << ",\"colour\":" << jsonString(baggage.colour)
           << ",\"type\":" << jsonString(baggage.type)
           << ",\"departureAirport\":"
           << jsonString(baggage.departureAirport)
           << ",\"arrivalAirport\":" << jsonString(baggage.arrivalAirport)
           << ",\"travelDate\":" << jsonString(baggage.travelDate)
           << ",\"approximateDepartureTime\":"
           << jsonString(baggage.approximateDepartureTime)
           << ",\"approximateArrivalTime\":"
           << jsonString(baggage.approximateArrivalTime)
           << ",\"lastCheckpoint\":" << jsonString(baggage.lastCheckpoint)
           << '}';
    return output.str();
}

HttpResponse baggageSearch(const HttpRequest& request,
                           const std::vector<aerotrack::Baggage>& records)
{
    aerotrack::SearchCriteria criteria;
    criteria.colour = parameter(request, "colour");
    criteria.type = parameter(request, "type");
    criteria.flightNumber = parameter(request, "flightNumber");
    criteria.departureAirport = parameter(request, "departureAirport");
    criteria.arrivalAirport = parameter(request, "arrivalAirport");
    criteria.travelDate = parameter(request, "travelDate");
    criteria.approximateDepartureTime =
        parameter(request, "approximateDepartureTime");
    criteria.approximateArrivalTime =
        parameter(request, "approximateArrivalTime");

    const std::vector<aerotrack::SearchResult> matches =
        aerotrack::searchBaggage(records, criteria);
    std::ostringstream output;
    output << "{\"results\":[";
    for (std::size_t index = 0; index < matches.size(); ++index)
    {
        if (index != 0)
            output << ',';
        output << "{\"baggage\":" << baggageJson(matches[index].baggage)
               << ",\"matchPercentage\":"
               << std::setprecision(12) << matches[index].matchPercentage
               << '}';
    }
    output << "]}";
    return response(200, "OK", output.str());
}

HttpResponse locationsResponse(const aerotrack::AirportGraph& graph)
{
    const std::vector<std::string> locations = graph.locationNames();
    std::ostringstream output;
    output << "{\"locations\":[";
    for (std::size_t index = 0; index < locations.size(); ++index)
    {
        if (index != 0)
            output << ',';
        output << jsonString(locations[index]);
    }
    output << "]}";
    return response(200, "OK", output.str());
}

HttpResponse routeResponse(const HttpRequest& request,
                           const aerotrack::AirportGraph& graph)
{
    const aerotrack::RouteResult route = graph.shortestRoute(
        parameter(request, "start"), parameter(request, "destination"));
    if (route.status != aerotrack::RouteFound)
        return response(200, "OK", "{\"status\":\"unreachable\","
                                     "\"locations\":[],\"instructions\":[],"
                                     "\"totalDistanceMetres\":0,"
                                     "\"estimatedWalkingMinutes\":0}");

    std::ostringstream output;
    output << "{\"status\":\"found\",\"locations\":[";
    for (std::size_t index = 0; index < route.locations.size(); ++index)
    {
        if (index != 0)
            output << ',';
        output << jsonString(route.locations[index]);
    }
    output << "],\"instructions\":[";
    for (std::size_t index = 0; index < route.instructions.size(); ++index)
    {
        if (index != 0)
            output << ',';
        output << jsonString(route.instructions[index]);
    }
    output << "],\"totalDistanceMetres\":"
           << std::setprecision(12) << route.totalDistanceMetres
           << ",\"estimatedWalkingMinutes\":"
           << std::setprecision(12) << route.estimatedWalkingMinutes << '}';
    return response(200, "OK", output.str());
}

bool currentTimestamp(std::string& timestamp)
{
    const std::time_t now = std::time(NULL);
    const std::tm* localTime = std::localtime(&now);
    if (localTime == NULL)
        return false;
    std::ostringstream output;
    output << std::put_time(localTime, "%Y-%m-%d %H:%M:%S");
    timestamp = output.str();
    return true;
}

std::string optionalString(bool present, const std::string& value)
{
    return present ? jsonString(value) : "null";
}

std::string summaryJson(const aerotrack::TrackingSummary& summary)
{
    std::ostringstream output;
    output << "{\"baggageId\":" << jsonString(summary.baggageId)
           << ",\"overallStatus\":"
           << jsonString(aerotrack::toString(summary.overallStatus))
           << ",\"statusIsInferred\":"
           << (summary.statusIsInferred ? "true" : "false")
           << ",\"hasLatestEvent\":"
           << (summary.hasLatestEvent ? "true" : "false")
           << ",\"latestEvent\":";
    if (summary.hasLatestEvent)
    {
        const aerotrack::TrackingEvent& event = summary.latestEvent;
        output << "{\"baggageId\":" << jsonString(event.baggageId)
               << ",\"checkpointId\":" << jsonString(event.checkpointId)
               << ",\"checkpointName\":" << jsonString(event.checkpointName)
               << ",\"timestamp\":" << jsonString(event.timestamp)
               << ",\"status\":"
               << jsonString(aerotrack::toString(event.status))
               << ",\"notes\":" << jsonString(event.notes) << '}';
    }
    else
        output << "null";

    output << ",\"hasLastKnownCheckpoint\":"
           << (summary.hasLastKnownCheckpoint ? "true" : "false")
           << ",\"lastCheckpointId\":"
           << optionalString(summary.hasLastKnownCheckpoint,
                             summary.lastCheckpointId)
           << ",\"lastCheckpointName\":"
           << optionalString(summary.hasLastKnownCheckpoint,
                             summary.lastCheckpointName)
           << ",\"lastCheckpointTimestamp\":"
           << optionalString(summary.hasLastKnownCheckpoint,
                             summary.lastCheckpointTimestamp)
           << ",\"hasNextExpectedCheckpoint\":"
           << (summary.hasNextExpectedCheckpoint ? "true" : "false")
           << ",\"nextExpectedCheckpoint\":";
    if (summary.hasNextExpectedCheckpoint)
    {
        output << "{\"checkpointId\":"
               << jsonString(summary.nextExpectedCheckpoint.checkpointId)
               << ",\"checkpointName\":"
               << jsonString(summary.nextExpectedCheckpoint.checkpointName)
               << ",\"deadline\":"
               << jsonString(summary.nextExpectedCheckpoint.deadline) << '}';
    }
    else
        output << "null";

    output << ",\"timeline\":[";
    for (std::size_t index = 0; index < summary.timeline.size(); ++index)
    {
        if (index != 0)
            output << ',';
        const aerotrack::TimelineEntry& entry = summary.timeline[index];
        output << "{\"checkpointId\":" << jsonString(entry.checkpointId)
               << ",\"checkpointName\":" << jsonString(entry.checkpointName)
               << ",\"timestamp\":" << jsonString(entry.timestamp)
               << ",\"status\":"
               << jsonString(aerotrack::toString(entry.status))
               << ",\"notes\":" << jsonString(entry.notes)
               << ",\"recorded\":" << (entry.recorded ? "true" : "false")
               << ",\"latest\":" << (entry.latest ? "true" : "false")
               << '}';
    }
    output << "],\"missedScanWarnings\":[";
    for (std::size_t index = 0;
         index < summary.missedScanWarnings.size(); ++index)
    {
        if (index != 0)
            output << ',';
        const aerotrack::MissedScanWarning& warning =
            summary.missedScanWarnings[index];
        output << "{\"checkpointId\":" << jsonString(warning.checkpointId)
               << ",\"checkpointName\":" << jsonString(warning.checkpointName)
               << ",\"deadline\":" << jsonString(warning.deadline)
               << ",\"reason\":" << jsonString(warning.reason) << '}';
    }
    output << "]}";
    return output.str();
}

HttpResponse handleRequest(
    const HttpRequest& request,
    const std::vector<aerotrack::Baggage>& records,
    DataManager& dataManager,
    aerotrack::BaggageHistory& history,
    const aerotrack::AirportGraph& graph)
{
    if (request.method == "OPTIONS")
        return response(204, "No Content", "");
    if (request.method != "GET" && request.method != "POST")
        return errorResponse(405, "Method Not Allowed",
                             "Only GET and POST requests are supported.");

    if (request.method == "GET" && request.path == "/api/health")
        return response(200, "OK",
                        "{\"service\":\"AeroTrack C++ API\","
                        "\"status\":\"ok\"}");
    if (request.method == "GET" && request.path == "/api/baggage")
        return baggageSearch(request, records);
    if (request.method == "GET" && request.path == "/api/locations")
        return locationsResponse(graph);
    if (request.method == "GET" && request.path == "/api/route")
        return routeResponse(request, graph);

    if (request.method == "GET" && request.path == "/api/data/baggage")
    {
        const std::string id = parameter(request, "id");
        char* end = NULL;
        const long numericId = std::strtol(id.c_str(), &end, 10);
        if (id.empty() || end == id.c_str() || *end != '\0' ||
            numericId < 0 || numericId > 2147483647L)
            return errorResponse(400, "Bad Request",
                                 "A valid baggage id is required.");

        Baggage baggage;
        if (!dataManager.searchBaggage(static_cast<int>(numericId), baggage))
            return errorResponse(404, "Not Found",
                                 "No baggage record was found for that id.");
        std::ostringstream output;
        output << "{\"bagId\":" << baggage.bagId
               << ",\"passengerName\":" << jsonString(baggage.passengerName)
               << ",\"flightNo\":" << jsonString(baggage.flightNo)
               << ",\"destination\":" << jsonString(baggage.destination)
               << ",\"status\":" << jsonString(baggage.status) << '}';
        return response(200, "OK", output.str());
    }

    if (request.method == "GET" && request.path == "/api/history")
    {
        const std::string id = parameter(request, "baggageId");
        if (id.empty())
            return errorResponse(400, "Bad Request",
                                 "A baggageId is required.");
        std::string at = parameter(request, "at");
        if (at.empty() && !currentTimestamp(at))
            return errorResponse(500, "Internal Server Error",
                                 "The current time could not be determined.");
        const aerotrack::TrackingSummary summary = history.getSummary(
            id, at);
        return response(200, "OK", summaryJson(summary));
    }

    if (request.method == "POST" && request.path == "/api/history/scan")
    {
        aerotrack::TrackingEvent event;
        event.baggageId = parameter(request, "baggageId");
        event.checkpointId = parameter(request, "checkpointId");
        event.checkpointName = parameter(request, "checkpointName");
        event.timestamp = parameter(request, "timestamp");
        event.notes = parameter(request, "notes");
        const std::string status = parameter(request, "status");
        const aerotrack::BaggageStatus statuses[] = {
            aerotrack::BaggageStatus::CheckedIn,
            aerotrack::BaggageStatus::SecurityCleared,
            aerotrack::BaggageStatus::InTransit,
            aerotrack::BaggageStatus::Loaded,
            aerotrack::BaggageStatus::Arrived,
            aerotrack::BaggageStatus::AtBaggageClaim,
            aerotrack::BaggageStatus::Collected,
            aerotrack::BaggageStatus::Delayed,
            aerotrack::BaggageStatus::ExceptionAttentionRequired};
        bool validStatus = false;
        for (std::size_t index = 0;
             index < sizeof(statuses) / sizeof(statuses[0]); ++index)
        {
            if (aerotrack::toString(statuses[index]) == status)
            {
                event.status = statuses[index];
                validStatus = true;
                break;
            }
        }
        if (!validStatus)
            return errorResponse(400, "Bad Request",
                                 "A supported baggage status is required.");
        if (!history.addScan(event))
            return errorResponse(400, "Bad Request",
                                 "The tracking scan is invalid or duplicates an existing scan.");
        std::string at;
        if (!currentTimestamp(at))
            return errorResponse(500, "Internal Server Error",
                                 "The current time could not be determined.");
        return response(201, "Created",
                        summaryJson(history.getSummary(event.baggageId, at)));
    }

    if (request.method == "POST" && request.path == "/api/data/status")
    {
        const std::string id = parameter(request, "id");
        char* end = NULL;
        const long numericId = std::strtol(id.c_str(), &end, 10);
        const std::string status = parameter(request, "status");
        if (id.empty() || end == id.c_str() || *end != '\0' ||
            numericId < 0 || numericId > 2147483647L || status.empty())
            return errorResponse(400, "Bad Request",
                                 "A valid baggage id and status are required.");
        if (!dataManager.updateStatus(static_cast<int>(numericId), status))
            return errorResponse(404, "Not Found",
                                 "No baggage record was found for that id.");
        return response(200, "OK", "{\"updated\":true}");
    }

    return errorResponse(404, "Not Found", "The requested API endpoint was not found.");
}

bool sendAll(SOCKET client, const std::string& data)
{
    std::size_t sent = 0;
    while (sent < data.size())
    {
        const int result = send(
            client, data.data() + sent,
            static_cast<int>(data.size() - sent), 0);
        if (result == SOCKET_ERROR || result == 0)
            return false;
        sent += static_cast<std::size_t>(result);
    }
    return true;
}

void serveClient(SOCKET client,
                 const std::vector<aerotrack::Baggage>& records,
                 DataManager& dataManager,
                 aerotrack::BaggageHistory& history,
                 const aerotrack::AirportGraph& graph)
{
    std::string raw;
    char buffer[2048];
    while (raw.find("\r\n\r\n") == std::string::npos &&
           raw.size() < kMaximumRequestSize)
    {
        const int received = recv(client, buffer, sizeof(buffer), 0);
        if (received <= 0)
            return;
        raw.append(buffer, static_cast<std::size_t>(received));
    }
    if (raw.size() >= kMaximumRequestSize)
    {
        const HttpResponse tooLarge = errorResponse(
            413, "Payload Too Large", "The request headers are too large.");
        const std::string output =
            "HTTP/1.1 413 Payload Too Large\r\nContent-Type: application/json\r\n"
            "Content-Length: " + std::to_string(tooLarge.body.size()) +
            "\r\nConnection: close\r\n\r\n" + tooLarge.body;
        sendAll(client, output);
        return;
    }

    HttpRequest request;
    HttpResponse result;
    if (!parseRequest(raw, request))
        result = errorResponse(400, "Bad Request",
                               "The HTTP request target is invalid.");
    else
        result = handleRequest(request, records, dataManager, history, graph);

    std::ostringstream output;
    output << "HTTP/1.1 " << result.status << ' ' << result.reason << "\r\n"
           << "Content-Type: application/json; charset=utf-8\r\n"
           << "Content-Length: " << result.body.size() << "\r\n"
           << "Access-Control-Allow-Origin: http://127.0.0.1:5500\r\n"
           << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
           << "Access-Control-Allow-Headers: Content-Type\r\n"
           << "Vary: Origin\r\n"
           << "Connection: close\r\n\r\n" << result.body;
    sendAll(client, output.str());
}

} // namespace

int main()
{
    const std::vector<aerotrack::Baggage> records = makeSampleRecords();
    DataManager dataManager;
    aerotrack::BaggageHistory history;
    for (std::size_t index = 0; index < records.size(); ++index)
    {
        Baggage bag;
        bag.bagId = records[index].id;
        bag.passengerName = records[index].ownerName;
        bag.flightNo = records[index].flightNumber;
        bag.destination = records[index].arrivalAirport;
        bag.status = "Unknown";
        if (!dataManager.addBaggage(bag))
        {
            std::cerr << "Could not initialize the baggage data manager.\n";
            return 1;
        }
        history.registerBaggage(std::to_string(records[index].id));
    }
    const aerotrack::AirportGraph graph = aerotrack::makeSampleAirportGraph();

    WSADATA winsockData;
    if (WSAStartup(MAKEWORD(2, 2), &winsockData) != 0)
    {
        std::cerr << "Could not initialize Windows networking.\n";
        return 1;
    }

    SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server == INVALID_SOCKET)
    {
        std::cerr << "Could not create the API server socket.\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(kPort);
    if (bind(server, reinterpret_cast<sockaddr*>(&address),
             sizeof(address)) == SOCKET_ERROR ||
        listen(server, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cerr << "Could not bind to 127.0.0.1:" << kPort
                  << ". Check whether the port is already in use.\n";
        closesocket(server);
        WSACleanup();
        return 1;
    }

    std::cout << "AeroTrack C++ API listening at http://127.0.0.1:"
              << kPort << "/api\n";
    std::cout.flush();
    while (true)
    {
        SOCKET client = accept(server, NULL, NULL);
        if (client == INVALID_SOCKET)
        {
            std::cerr << "Accept failed while waiting for an API request.\n";
            closesocket(server);
            WSACleanup();
            return 1;
        }
        serveClient(client, records, dataManager, history, graph);
        closesocket(client);
    }
}
