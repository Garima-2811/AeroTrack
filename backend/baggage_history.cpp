#include "baggage_history.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace aerotrack
{
namespace
{

bool isDigits(const std::string& value, std::size_t start, std::size_t count)
{
    for (std::size_t index = start; index < start + count; ++index)
    {
        if (index >= value.size() ||
            !std::isdigit(static_cast<unsigned char>(value[index])))
            return false;
    }
    return true;
}

bool isValidTimestamp(const std::string& timestamp)
{
    if (timestamp.size() != 19 || timestamp[4] != '-' ||
        timestamp[7] != '-' || timestamp[10] != ' ' ||
        timestamp[13] != ':' || timestamp[16] != ':' ||
        !isDigits(timestamp, 0, 4) || !isDigits(timestamp, 5, 2) ||
        !isDigits(timestamp, 8, 2) || !isDigits(timestamp, 11, 2) ||
        !isDigits(timestamp, 14, 2) || !isDigits(timestamp, 17, 2))
        return false;

    const int year = std::atoi(timestamp.substr(0, 4).c_str());
    const int month = std::atoi(timestamp.substr(5, 2).c_str());
    const int day = std::atoi(timestamp.substr(8, 2).c_str());
    const int hour = std::atoi(timestamp.substr(11, 2).c_str());
    const int minute = std::atoi(timestamp.substr(14, 2).c_str());
    const int second = std::atoi(timestamp.substr(17, 2).c_str());
    const int daysInMonth[] = {31, 28, 31, 30, 31, 30,
                               31, 31, 30, 31, 30, 31};

    if (month < 1 || month > 12 || hour > 23 || minute > 59 || second > 59)
        return false;

    int maximumDay = daysInMonth[month - 1];
    if (month == 2 && (year % 400 == 0 ||
                       (year % 4 == 0 && year % 100 != 0)))
        maximumDay = 29;
    return day >= 1 && day <= maximumDay;
}

bool isValidBaggageId(const std::string& baggageId)
{
    if (baggageId.empty())
        return false;

    for (std::size_t index = 0; index < baggageId.size(); ++index)
    {
        const unsigned char character =
            static_cast<unsigned char>(baggageId[index]);
        if (!std::isalnum(character) && character != '-' && character != '_')
            return false;
    }
    return true;
}

int statusRank(BaggageStatus status)
{
    switch (status)
    {
    case BaggageStatus::CheckedIn: return 1;
    case BaggageStatus::SecurityCleared: return 2;
    case BaggageStatus::InTransit: return 3;
    case BaggageStatus::Loaded: return 4;
    case BaggageStatus::Arrived: return 5;
    case BaggageStatus::AtBaggageClaim: return 6;
    case BaggageStatus::Collected: return 7;
    default: return 0;
    }
}

bool isValidStatus(BaggageStatus status)
{
    return status != BaggageStatus::Unknown;
}

bool hasValidProgression(const std::vector<TrackingEvent>& history)
{
    int latestProgressRank = 0;
    for (std::size_t index = 0; index < history.size(); ++index)
    {
        const int rank = statusRank(history[index].status);
        if (rank != 0)
        {
            if (rank < latestProgressRank)
                return false;
            latestProgressRank = rank;
        }
    }
    return true;
}

bool parseStatus(const std::string& value, BaggageStatus& status)
{
    const BaggageStatus statuses[] = {
        BaggageStatus::CheckedIn, BaggageStatus::SecurityCleared,
        BaggageStatus::InTransit, BaggageStatus::Loaded,
        BaggageStatus::Arrived, BaggageStatus::AtBaggageClaim,
        BaggageStatus::Collected, BaggageStatus::Delayed,
        BaggageStatus::ExceptionAttentionRequired};
    for (std::size_t index = 0;
         index < sizeof(statuses) / sizeof(statuses[0]); ++index)
    {
        if (toString(statuses[index]) == value)
        {
            status = statuses[index];
            return true;
        }
    }
    return false;
}

bool parseCsvRow(const std::string& line, std::vector<std::string>& fields)
{
    fields.clear();
    std::string field;
    bool quoted = false;

    for (std::size_t index = 0; index < line.size(); ++index)
    {
        const char character = line[index];
        if (quoted)
        {
            if (character == '"')
            {
                if (index + 1 < line.size() && line[index + 1] == '"')
                {
                    field += '"';
                    ++index;
                }
                else
                    quoted = false;
            }
            else
                field += character;
        }
        else if (character == ',')
        {
            fields.push_back(field);
            field.clear();
        }
        else if (character == '"')
        {
            if (!field.empty())
                return false;
            quoted = true;
        }
        else
            field += character;
    }

    if (quoted)
        return false;
    fields.push_back(field);
    return true;
}

std::string escapeCsvField(const std::string& value)
{
    if (value.find_first_of(",\"\r\n") == std::string::npos)
        return value;

    std::string escaped = "\"";
    for (std::size_t index = 0; index < value.size(); ++index)
    {
        if (value[index] == '"')
            escaped += '"';
        escaped += value[index];
    }
    escaped += '"';
    return escaped;
}

bool hasEventAtCheckpoint(const std::vector<TrackingEvent>& history,
                          const std::string& checkpointId)
{
    for (std::size_t index = 0; index < history.size(); ++index)
    {
        if (history[index].checkpointId == checkpointId)
            return true;
    }
    return false;
}

} // namespace

std::string toString(BaggageStatus status)
{
    switch (status)
    {
    case BaggageStatus::CheckedIn: return "Checked In";
    case BaggageStatus::SecurityCleared: return "Security Cleared";
    case BaggageStatus::InTransit: return "In Transit";
    case BaggageStatus::Loaded: return "Loaded";
    case BaggageStatus::Arrived: return "Arrived";
    case BaggageStatus::AtBaggageClaim: return "At Baggage Claim";
    case BaggageStatus::Collected: return "Collected";
    case BaggageStatus::Delayed: return "Delayed";
    case BaggageStatus::ExceptionAttentionRequired:
        return "Exception / Attention Required";
    default: return "Unknown";
    }
}

std::string toString(ComplaintStatus status)
{
    switch (status)
    {
    case ComplaintStatus::Open: return "Open";
    case ComplaintStatus::UnderReview: return "Under Review";
    case ComplaintStatus::Resolved: return "Resolved";
    default: return "Open";
    }
}

BaggageHistory::BaggageHistory() : nextComplaintNumber(1)
{
}

bool BaggageHistory::registerBaggage(const std::string& baggageId)
{
    if (!isValidBaggageId(baggageId) || hasBaggage(baggageId))
        return false;
    baggageIds.push_back(baggageId);
    return true;
}

bool BaggageHistory::hasBaggage(const std::string& baggageId) const
{
    return std::find(baggageIds.begin(), baggageIds.end(), baggageId) !=
           baggageIds.end();
}

bool BaggageHistory::addScan(const TrackingEvent& event)
{
    if (!isValidBaggageId(event.baggageId) || event.checkpointId.empty() ||
        event.checkpointName.empty() || !isValidTimestamp(event.timestamp) ||
        !isValidStatus(event.status))
        return false;

    std::vector<TrackingEvent> history = getHistory(event.baggageId);
    for (std::size_t index = 0; index < history.size(); ++index)
    {
        if (history[index].checkpointId == event.checkpointId &&
            history[index].timestamp == event.timestamp)
            return false;
    }
    history.push_back(event);
    std::sort(history.begin(), history.end(),
              [](const TrackingEvent& left, const TrackingEvent& right)
              {
                  if (left.timestamp != right.timestamp)
                      return left.timestamp < right.timestamp;
                  return left.checkpointId < right.checkpointId;
              });

    if (!hasValidProgression(history))
        return false;

    if (!hasBaggage(event.baggageId))
        baggageIds.push_back(event.baggageId);
    events.push_back(event);
    return true;
}

std::vector<TrackingEvent> BaggageHistory::getHistory(
    const std::string& baggageId) const
{
    std::vector<TrackingEvent> history;
    for (std::size_t index = 0; index < events.size(); ++index)
    {
        if (events[index].baggageId == baggageId)
            history.push_back(events[index]);
    }
    std::sort(history.begin(), history.end(),
              [](const TrackingEvent& left, const TrackingEvent& right)
              {
                  if (left.timestamp != right.timestamp)
                      return left.timestamp < right.timestamp;
                  return left.checkpointId < right.checkpointId;
              });
    return history;
}

bool BaggageHistory::setExpectedRoute(
    const std::string& baggageId,
    const std::vector<ExpectedCheckpoint>& route)
{
    if (!isValidBaggageId(baggageId))
        return false;

    for (std::size_t index = 0; index < route.size(); ++index)
    {
        if (route[index].checkpointId.empty() ||
            route[index].checkpointName.empty() ||
            (!route[index].deadline.empty() &&
             !isValidTimestamp(route[index].deadline)))
            return false;
        for (std::size_t other = 0; other < index; ++other)
        {
            if (route[other].checkpointId == route[index].checkpointId)
                return false;
        }
    }

    for (std::size_t index = 0; index < routeBaggageIds.size(); ++index)
    {
        if (routeBaggageIds[index] == baggageId)
        {
            routes[index] = route;
            return true;
        }
    }
    routeBaggageIds.push_back(baggageId);
    routes.push_back(route);
    return true;
}

std::vector<MissedScanWarning> BaggageHistory::getMissedScanWarnings(
    const std::string& baggageId,
    const std::string& currentTimestamp) const
{
    std::vector<MissedScanWarning> warnings;
    if (!isValidTimestamp(currentTimestamp))
        return warnings;

    const std::vector<TrackingEvent> history = getHistory(baggageId);
    for (std::size_t routeIndex = 0;
         routeIndex < routeBaggageIds.size(); ++routeIndex)
    {
        if (routeBaggageIds[routeIndex] != baggageId)
            continue;

        for (std::size_t index = 0; index < routes[routeIndex].size(); ++index)
        {
            const ExpectedCheckpoint& checkpoint = routes[routeIndex][index];
            if (checkpoint.deadline.empty() ||
                currentTimestamp < checkpoint.deadline ||
                hasEventAtCheckpoint(history, checkpoint.checkpointId))
                continue;

            MissedScanWarning warning;
            warning.checkpointId = checkpoint.checkpointId;
            warning.checkpointName = checkpoint.checkpointName;
            warning.deadline = checkpoint.deadline;
            warning.reason = "No scan was recorded by the configured deadline.";
            warnings.push_back(warning);
        }
        break;
    }
    return warnings;
}

TrackingSummary BaggageHistory::getSummary(
    const std::string& baggageId,
    const std::string& currentTimestamp) const
{
    TrackingSummary summary;
    summary.baggageId = baggageId;
    summary.overallStatus = BaggageStatus::Unknown;
    summary.statusIsInferred = false;
    summary.hasLatestEvent = false;
    summary.hasLastKnownCheckpoint = false;
    summary.hasNextExpectedCheckpoint = false;

    if (!hasBaggage(baggageId))
        return summary;

    const std::vector<TrackingEvent> history = getHistory(baggageId);
    if (!history.empty())
    {
        const TrackingEvent& latest = history.back();
        summary.hasLatestEvent = true;
        summary.latestEvent = latest;
        summary.overallStatus = latest.status;
        summary.hasLastKnownCheckpoint = true;
        summary.lastCheckpointId = latest.checkpointId;
        summary.lastCheckpointName = latest.checkpointName;
        summary.lastCheckpointTimestamp = latest.timestamp;
    }

    summary.missedScanWarnings = getMissedScanWarnings(
        baggageId, currentTimestamp);
    if (!summary.missedScanWarnings.empty() &&
        summary.overallStatus != BaggageStatus::ExceptionAttentionRequired &&
        summary.overallStatus != BaggageStatus::Collected)
    {
        summary.overallStatus = BaggageStatus::Delayed;
        summary.statusIsInferred = true;
    }

    const std::vector<ExpectedCheckpoint>* route = NULL;
    for (std::size_t index = 0; index < routeBaggageIds.size(); ++index)
    {
        if (routeBaggageIds[index] == baggageId)
        {
            route = &routes[index];
            break;
        }
    }

    if (route != NULL)
    {
        for (std::size_t index = 0; index < route->size(); ++index)
        {
            if (!hasEventAtCheckpoint(history, (*route)[index].checkpointId))
            {
                summary.hasNextExpectedCheckpoint = true;
                summary.nextExpectedCheckpoint = (*route)[index];
                break;
            }
        }
    }

    for (std::size_t index = 0; index < history.size(); ++index)
    {
        TimelineEntry entry;
        entry.checkpointId = history[index].checkpointId;
        entry.checkpointName = history[index].checkpointName;
        entry.timestamp = history[index].timestamp;
        entry.status = history[index].status;
        entry.notes = history[index].notes;
        entry.recorded = true;
        entry.latest = index + 1 == history.size();
        summary.timeline.push_back(entry);
    }

    if (route != NULL)
    {
        for (std::size_t index = 0; index < route->size(); ++index)
        {
            if (hasEventAtCheckpoint(history, (*route)[index].checkpointId))
                continue;
            TimelineEntry entry;
            entry.checkpointId = (*route)[index].checkpointId;
            entry.checkpointName = (*route)[index].checkpointName;
            entry.timestamp.clear();
            entry.status = BaggageStatus::Unknown;
            entry.notes.clear();
            entry.recorded = false;
            entry.latest = false;
            summary.timeline.push_back(entry);
        }
    }
    return summary;
}

bool BaggageHistory::addComplaint(const std::string& baggageId,
                                  const std::string& description,
                                  const std::string& timestamp,
                                  Complaint& createdComplaint)
{
    if (!hasBaggage(baggageId) || description.empty() ||
        !isValidTimestamp(timestamp))
        return false;

    std::ostringstream complaintId;
    complaintId << "C" << std::setw(6) << std::setfill('0')
                << nextComplaintNumber++;
    createdComplaint.complaintId = complaintId.str();
    createdComplaint.baggageId = baggageId;
    createdComplaint.description = description;
    createdComplaint.timestamp = timestamp;
    createdComplaint.status = ComplaintStatus::Open;
    createdComplaint.resolutionNotes.clear();
    complaints.push_back(createdComplaint);
    return true;
}

bool BaggageHistory::updateComplaintStatus(
    const std::string& complaintId,
    ComplaintStatus status,
    const std::string& resolutionNotes)
{
    if (status != ComplaintStatus::Open &&
        status != ComplaintStatus::UnderReview &&
        status != ComplaintStatus::Resolved)
        return false;

    for (std::size_t index = 0; index < complaints.size(); ++index)
    {
        if (complaints[index].complaintId == complaintId)
        {
            complaints[index].status = status;
            complaints[index].resolutionNotes = resolutionNotes;
            return true;
        }
    }
    return false;
}

std::vector<Complaint> BaggageHistory::getComplaints(
    const std::string& baggageId) const
{
    std::vector<Complaint> matchingComplaints;
    for (std::size_t index = 0; index < complaints.size(); ++index)
    {
        if (complaints[index].baggageId == baggageId)
            matchingComplaints.push_back(complaints[index]);
    }
    return matchingComplaints;
}

bool BaggageHistory::loadFromCsv(const std::string& path)
{
    std::ifstream file(path.c_str());
    if (!file.is_open())
        return false;

    std::vector<TrackingEvent> loadedEvents;
    std::vector<std::string> loadedBaggageIds;
    std::string line;
    bool headerRead = false;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        if (!headerRead)
        {
            headerRead = true;
            if (line == "baggage_id,checkpoint_id,checkpoint_name,timestamp,status,notes")
                continue;
        }

        std::vector<std::string> fields;
        if (!parseCsvRow(line, fields) || fields.size() != 6)
            continue;

        TrackingEvent event;
        event.baggageId = fields[0];
        event.checkpointId = fields[1];
        event.checkpointName = fields[2];
        event.timestamp = fields[3];
        if (!parseStatus(fields[4], event.status))
            continue;
        event.notes = fields[5];

        if (!isValidBaggageId(event.baggageId) || event.checkpointId.empty() ||
            event.checkpointName.empty() || !isValidTimestamp(event.timestamp))
            continue;

        bool duplicate = false;
        for (std::size_t index = 0; index < loadedEvents.size(); ++index)
        {
            if (loadedEvents[index].baggageId == event.baggageId &&
                loadedEvents[index].checkpointId == event.checkpointId &&
                loadedEvents[index].timestamp == event.timestamp)
            {
                duplicate = true;
                break;
            }
        }
        if (duplicate)
            continue;

        std::vector<TrackingEvent> baggageHistory;
        for (std::size_t index = 0; index < loadedEvents.size(); ++index)
        {
            if (loadedEvents[index].baggageId == event.baggageId)
                baggageHistory.push_back(loadedEvents[index]);
        }
        baggageHistory.push_back(event);
        std::sort(baggageHistory.begin(), baggageHistory.end(),
                  [](const TrackingEvent& left, const TrackingEvent& right)
                  {
                      if (left.timestamp != right.timestamp)
                          return left.timestamp < right.timestamp;
                      return left.checkpointId < right.checkpointId;
                  });
        if (!hasValidProgression(baggageHistory))
            continue;

        loadedEvents.push_back(event);
        if (std::find(loadedBaggageIds.begin(), loadedBaggageIds.end(),
                      event.baggageId) == loadedBaggageIds.end())
            loadedBaggageIds.push_back(event.baggageId);
    }

    if (file.bad())
        return false;

    events.swap(loadedEvents);
    baggageIds.swap(loadedBaggageIds);
    routes.clear();
    routeBaggageIds.clear();
    complaints.clear();
    nextComplaintNumber = 1;
    return true;
}

bool BaggageHistory::saveToCsv(const std::string& path) const
{
    std::ofstream file(path.c_str());
    if (!file.is_open())
        return false;

    file << "baggage_id,checkpoint_id,checkpoint_name,timestamp,status,notes\n";
    std::vector<TrackingEvent> sortedEvents = events;
    std::sort(sortedEvents.begin(), sortedEvents.end(),
              [](const TrackingEvent& left, const TrackingEvent& right)
              {
                  if (left.baggageId != right.baggageId)
                      return left.baggageId < right.baggageId;
                  if (left.timestamp != right.timestamp)
                      return left.timestamp < right.timestamp;
                  return left.checkpointId < right.checkpointId;
              });

    for (std::size_t index = 0; index < sortedEvents.size(); ++index)
    {
        const TrackingEvent& event = sortedEvents[index];
        file << escapeCsvField(event.baggageId) << ','
             << escapeCsvField(event.checkpointId) << ','
             << escapeCsvField(event.checkpointName) << ','
             << escapeCsvField(event.timestamp) << ','
             << escapeCsvField(toString(event.status)) << ','
             << escapeCsvField(event.notes) << '\n';
    }
    return file.good();
}

} // namespace aerotrack