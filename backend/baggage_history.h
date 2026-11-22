#ifndef AEROTRACK_BAGGAGE_HISTORY_H
#define AEROTRACK_BAGGAGE_HISTORY_H

#include <string>
#include <vector>

namespace aerotrack
{

enum class BaggageStatus
{
    Unknown,
    CheckedIn,
    SecurityCleared,
    InTransit,
    Loaded,
    Arrived,
    AtBaggageClaim,
    Collected,
    Delayed,
    ExceptionAttentionRequired
};

enum class ComplaintStatus
{
    Open,
    UnderReview,
    Resolved
};

struct TrackingEvent
{
    std::string baggageId;
    std::string checkpointId;
    std::string checkpointName;
    std::string timestamp;
    BaggageStatus status;
    std::string notes;
};

struct ExpectedCheckpoint
{
    std::string checkpointId;
    std::string checkpointName;
    std::string deadline;
};

struct MissedScanWarning
{
    std::string checkpointId;
    std::string checkpointName;
    std::string deadline;
    std::string reason;
};

struct TimelineEntry
{
    std::string checkpointId;
    std::string checkpointName;
    std::string timestamp;
    BaggageStatus status;
    std::string notes;
    bool recorded;
    bool latest;
};

struct Complaint
{
    std::string complaintId;
    std::string baggageId;
    std::string description;
    std::string timestamp;
    ComplaintStatus status;
    std::string resolutionNotes;
};

struct TrackingSummary
{
    std::string baggageId;
    BaggageStatus overallStatus;
    bool statusIsInferred;
    bool hasLatestEvent;
    TrackingEvent latestEvent;
    bool hasLastKnownCheckpoint;
    std::string lastCheckpointId;
    std::string lastCheckpointName;
    std::string lastCheckpointTimestamp;
    bool hasNextExpectedCheckpoint;
    ExpectedCheckpoint nextExpectedCheckpoint;
    std::vector<TimelineEntry> timeline;
    std::vector<MissedScanWarning> missedScanWarnings;
};

std::string toString(BaggageStatus status);
std::string toString(ComplaintStatus status);

class BaggageHistory
{
public:
    BaggageHistory();
    bool registerBaggage(const std::string& baggageId);
    bool hasBaggage(const std::string& baggageId) const;
    bool addScan(const TrackingEvent& event);
    std::vector<TrackingEvent> getHistory(
        const std::string& baggageId) const;
    bool setExpectedRoute(const std::string& baggageId,
                         const std::vector<ExpectedCheckpoint>& route);
    std::vector<MissedScanWarning> getMissedScanWarnings(
        const std::string& baggageId,
        const std::string& currentTimestamp) const;
    TrackingSummary getSummary(const std::string& baggageId,
                               const std::string& currentTimestamp) const;
    bool addComplaint(const std::string& baggageId,
                      const std::string& description,
                      const std::string& timestamp,
                      Complaint& createdComplaint);
    bool updateComplaintStatus(const std::string& complaintId,
                               ComplaintStatus status,
                               const std::string& resolutionNotes);
    std::vector<Complaint> getComplaints(
        const std::string& baggageId) const;
    bool loadFromCsv(const std::string& path);
    bool saveToCsv(const std::string& path) const;

private:
    std::vector<std::string> baggageIds;
    std::vector<TrackingEvent> events;
    std::vector<std::vector<ExpectedCheckpoint> > routes;
    std::vector<std::string> routeBaggageIds;
    std::vector<Complaint> complaints;
    unsigned long nextComplaintNumber;
};

} // namespace aerotrack

#endif