#include "../backend/baggage_history.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace aerotrack;

TrackingEvent makeEvent(const std::string& baggageId,
                        const std::string& checkpointId,
                        const std::string& checkpointName,
                        const std::string& timestamp,
                        BaggageStatus status,
                        const std::string& notes = std::string())
{
    TrackingEvent event;
    event.baggageId = baggageId;
    event.checkpointId = checkpointId;
    event.checkpointName = checkpointName;
    event.timestamp = timestamp;
    event.status = status;
    event.notes = notes;
    return event;
}

std::vector<ExpectedCheckpoint> makeRoute()
{
    std::vector<ExpectedCheckpoint> route;
    ExpectedCheckpoint checkIn = {"check-in", "Check-in Counter", ""};
    ExpectedCheckpoint security = {"security", "Security Area", ""};
    ExpectedCheckpoint sorting = {"sorting", "Sorting Area", ""};
    ExpectedCheckpoint loading = {
        "loading", "Aircraft Loading Zone", "2026-10-21 11:00:00"};
    route.push_back(checkIn);
    route.push_back(security);
    route.push_back(sorting);
    route.push_back(loading);
    return route;
}

void addSampleHistory(BaggageHistory& history)
{
    assert(history.addScan(makeEvent("BG1024", "check-in", "Check-in Counter",
                                     "2026-10-21 10:05:00",
                                     BaggageStatus::CheckedIn)));
    assert(history.addScan(makeEvent("BG1024", "security", "Security Area",
                                     "2026-10-21 10:18:00",
                                     BaggageStatus::SecurityCleared)));
    assert(history.addScan(makeEvent("BG1024", "sorting", "Sorting Area",
                                     "2026-10-21 10:32:00",
                                     BaggageStatus::InTransit)));
}

void testHistorySummaryAndTimeline()
{
    BaggageHistory history;
    addSampleHistory(history);
    assert(history.setExpectedRoute("BG1024", makeRoute()));

    const std::vector<TrackingEvent> events = history.getHistory("BG1024");
    assert(events.size() == 3);
    assert(events[0].checkpointId == "check-in");
    assert(events[2].checkpointId == "sorting");

    const TrackingSummary summary = history.getSummary(
        "BG1024", "2026-10-21 10:40:00");
    assert(summary.overallStatus == BaggageStatus::InTransit);
    assert(!summary.statusIsInferred);
    assert(summary.hasLastKnownCheckpoint);
    assert(summary.lastCheckpointName == "Sorting Area");
    assert(summary.lastCheckpointTimestamp == "2026-10-21 10:32:00");
    assert(summary.hasLatestEvent);
    assert(summary.latestEvent.checkpointId == "sorting");
    assert(summary.hasNextExpectedCheckpoint);
    assert(summary.nextExpectedCheckpoint.checkpointId == "loading");
    assert(summary.timeline.size() == 4);
    assert(summary.timeline[2].recorded && summary.timeline[2].latest);
    assert(!summary.timeline[3].recorded && !summary.timeline[3].latest);
}

void testOutOfOrderAndDuplicateScans()
{
    BaggageHistory history;
    assert(history.addScan(makeEvent("BG2048", "security", "Security Area",
                                     "2026-10-21 10:18:00",
                                     BaggageStatus::SecurityCleared)));
    assert(history.addScan(makeEvent("BG2048", "check-in", "Check-in Counter",
                                     "2026-10-21 10:05:00",
                                     BaggageStatus::CheckedIn)));
    assert(!history.addScan(makeEvent("BG2048", "check-in", "Check-in Counter",
                                      "2026-10-21 10:05:00",
                                      BaggageStatus::CheckedIn)));
    assert(!history.addScan(makeEvent("BG2048", "late-checkin", "Check-in",
                                      "2026-10-21 10:20:00",
                                      BaggageStatus::CheckedIn)));

    const std::vector<TrackingEvent> events = history.getHistory("BG2048");
    assert(events.size() == 2);
    assert(events[0].status == BaggageStatus::CheckedIn);
    assert(events[1].status == BaggageStatus::SecurityCleared);
}

void testStatusVariantsAndUnknownBaggage()
{
    BaggageHistory history;
    assert(history.registerBaggage("BG3000"));
    TrackingSummary summary = history.getSummary(
        "BG3000", "2026-10-21 13:00:00");
    assert(summary.overallStatus == BaggageStatus::Unknown);
    assert(!summary.hasLatestEvent);
    assert(history.addScan(makeEvent("BG3000", "collection", "Collection Point",
                                     "2026-10-21 12:59:00",
                                     BaggageStatus::Collected)));
    summary = history.getSummary("BG3000", "2026-10-21 13:00:00");
    assert(summary.overallStatus == BaggageStatus::Collected);

    assert(history.addScan(makeEvent("BG3001", "claim", "Baggage Claim Belt",
                                     "2026-10-21 13:00:00",
                                     BaggageStatus::AtBaggageClaim)));
    summary = history.getSummary(
        "BG3001", "2026-10-21 13:00:00");
    assert(summary.overallStatus == BaggageStatus::AtBaggageClaim);

    assert(history.addScan(makeEvent("BG3002", "exception", "Sorting Area",
                                     "2026-10-21 13:01:00",
                                     BaggageStatus::ExceptionAttentionRequired)));
    summary = history.getSummary("BG3002", "2026-10-21 13:01:00");
    assert(summary.overallStatus == BaggageStatus::ExceptionAttentionRequired);

    summary = history.getSummary("UNKNOWN", "2026-10-21 13:01:00");
    assert(summary.overallStatus == BaggageStatus::Unknown);
    assert(!summary.hasLastKnownCheckpoint);
    assert(history.getHistory("UNKNOWN").empty());
}

void testValidationAndMissedScanRules()
{
    BaggageHistory history;
    assert(!history.registerBaggage(""));
    assert(!history.registerBaggage("BG 1"));
    assert(!history.addScan(makeEvent("", "check-in", "Counter",
                                      "2026-10-21 10:05:00",
                                      BaggageStatus::CheckedIn)));
    assert(!history.addScan(makeEvent("BG4001", "check-in", "Counter",
                                      "not-a-date", BaggageStatus::CheckedIn)));
    assert(!history.addScan(makeEvent("BG4001", "check-in", "Counter",
                                      "2026-10-21 10:05:00",
                                      BaggageStatus::Unknown)));
    assert(history.registerBaggage("BG4001"));
    assert(history.getMissedScanWarnings(
        "BG4001", "2026-10-21 12:00:00").empty());

    std::vector<ExpectedCheckpoint> route;
    ExpectedCheckpoint due = {
        "security", "Security Area", "2026-10-21 11:00:00"};
    route.push_back(due);
    assert(history.setExpectedRoute("BG4001", route));
    assert(history.getMissedScanWarnings(
        "BG4001", "2026-10-21 10:59:59").empty());

    std::vector<MissedScanWarning> warnings = history.getMissedScanWarnings(
        "BG4001", "2026-10-21 11:00:00");
    assert(warnings.size() == 1);
    assert(warnings[0].checkpointId == "security");
    assert(warnings[0].reason.find("No scan") != std::string::npos);
    const TrackingSummary summary = history.getSummary(
        "BG4001", "2026-10-21 11:30:00");
    assert(summary.overallStatus == BaggageStatus::Delayed);
    assert(summary.statusIsInferred);
    assert(!summary.hasLastKnownCheckpoint);
}

void testComplaints()
{
    BaggageHistory history;
    assert(history.addScan(makeEvent("BG5001", "check-in", "Counter",
                                     "2026-10-21 10:05:00",
                                     BaggageStatus::CheckedIn)));
    Complaint complaint;
    assert(!history.addComplaint("UNKNOWN", "Missing bag",
                                 "2026-10-21 12:00:00", complaint));
    assert(!history.addComplaint("BG5001", "", "2026-10-21 12:00:00",
                                 complaint));
    assert(!history.addComplaint("BG5001", "Missing bag", "bad-date",
                                 complaint));
    assert(history.addComplaint("BG5001", "Bag has not arrived",
                                "2026-10-21 12:00:00", complaint));
    assert(complaint.complaintId == "C000001");
    assert(complaint.status == ComplaintStatus::Open);
    assert(history.updateComplaintStatus(complaint.complaintId,
                                         ComplaintStatus::UnderReview,
                                         "Checking sample records"));
    assert(history.getComplaints("BG5001").size() == 1);
    assert(history.getComplaints("BG5001")[0].status ==
           ComplaintStatus::UnderReview);
    assert(history.updateComplaintStatus(complaint.complaintId,
                                         ComplaintStatus::Resolved,
                                         "Resolved for prototype testing"));
    assert(history.getComplaints("BG5001")[0].status ==
           ComplaintStatus::Resolved);
    assert(!history.updateComplaintStatus("missing", ComplaintStatus::Resolved,
                                          ""));
    assert(history.getComplaints("UNKNOWN").empty());
}

void testCsvStorageAndMalformedInput()
{
    const std::string csvPath = "aerotrack_history_test.csv";
    const std::string malformedPath = "aerotrack_history_malformed_test.csv";
    BaggageHistory history;
    addSampleHistory(history);
    assert(history.addScan(makeEvent("BG1024", "note", "Notes, Area",
                                     "2026-10-21 10:40:00",
                                     BaggageStatus::InTransit,
                                     "Checked, with \"care\"")));
    assert(history.saveToCsv(csvPath));

    BaggageHistory loaded;
    assert(loaded.loadFromCsv(csvPath));
    const std::vector<TrackingEvent> loadedEvents = loaded.getHistory("BG1024");
    assert(loadedEvents.size() == 4);
    assert(loadedEvents[3].checkpointName == "Notes, Area");
    assert(loadedEvents[3].notes == "Checked, with \"care\"");

    {
        std::ofstream malformed(malformedPath.c_str());
        malformed << "baggage_id,checkpoint_id,checkpoint_name,timestamp,status,notes\n";
        malformed << "broken,row\n";
        malformed << "BG6001,check-in,Counter,2026-10-21 10:05:00,Checked In,valid\n";
        malformed << "BG6001,bad,Counter,invalid,Checked In,invalid timestamp\n";
    }
    assert(loaded.loadFromCsv(malformedPath));
    assert(loaded.getHistory("BG6001").size() == 1);
    assert(!loaded.hasBaggage("BG1024"));
    assert(!loaded.loadFromCsv("aerotrack_missing_test.csv"));
    std::remove(csvPath.c_str());
    std::remove(malformedPath.c_str());
}

void testFictionalSampleData()
{
    BaggageHistory history;
    assert(history.loadFromCsv("data/sample_tracking.csv"));
    const TrackingSummary summary = history.getSummary(
        "BG1024", "2026-10-21 10:40:00");
    assert(summary.overallStatus == BaggageStatus::InTransit);
    assert(summary.lastCheckpointName == "Sorting Area");
}

int main()
{
    testHistorySummaryAndTimeline();
    testOutOfOrderAndDuplicateScans();
    testStatusVariantsAndUnknownBaggage();
    testValidationAndMissedScanRules();
    testComplaints();
    testCsvStorageAndMalformedInput();
    testFictionalSampleData();
    std::cout << "All baggage history tests passed.\n";
    return 0;
}