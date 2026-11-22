"use strict";

const baggageRecords = [
  {
    id: 1001, ownerName: "Maya Patel", flightNumber: "AX101",
    colour: "Dark Red", type: "Hard-shell", departureAirport: "New York",
    arrivalAirport: "London", travelDate: "20-10-2026",
    approximateDepartureTime: "10:30 AM", approximateArrivalTime: "10:20 PM",
    lastCheckpoint: "Security screening"
  },
  {
    id: 1002, ownerName: "Liam Chen", flightNumber: "AX202",
    colour: "Red", type: "Soft-shell", departureAirport: "Newark",
    arrivalAirport: "London", travelDate: "20-10-2026",
    approximateDepartureTime: "11:00 AM", approximateArrivalTime: "11:00 PM",
    lastCheckpoint: "Check-in counter"
  },
  {
    id: 1003, ownerName: "Nia Shah", flightNumber: "BY303",
    colour: "Blue", type: "Hard-shell", departureAirport: "New York",
    arrivalAirport: "Paris", travelDate: "21-10-2026",
    approximateDepartureTime: "08:15 AM", approximateArrivalTime: "09:30 PM",
    lastCheckpoint: "Loading area"
  },
  {
    id: 1004, ownerName: "Omar Ali", flightNumber: "AX104",
    colour: "Red stripe", type: "Hard-shell", departureAirport: "New York",
    arrivalAirport: "London", travelDate: "20-10-2026",
    approximateDepartureTime: "10:45 AM", approximateArrivalTime: "10:35 PM",
    lastCheckpoint: "Baggage claim"
  }
];

const locations = [
  { name: "Entrance", x: 0, y: 0 },
  { name: "Check-in", x: 100, y: 0 },
  { name: "Security", x: 200, y: 0 },
  { name: "Gate A1", x: 300, y: 0 },
  { name: "Washroom", x: 200, y: 80 },
  { name: "Baggage Claim", x: 100, y: -80 }
];

const corridors = [
  ["Entrance", "Check-in", 100],
  ["Check-in", "Security", 80],
  ["Entrance", "Security", 240],
  ["Security", "Gate A1", 120],
  ["Security", "Washroom", 45],
  ["Check-in", "Baggage Claim", 95],
  ["Entrance", "Baggage Claim", 150]
];

const baggageForm = document.querySelector("#baggage-form");
const baggageResults = document.querySelector("#baggage-results");
const resultCount = document.querySelector("#result-count");
const directionsForm = document.querySelector("#directions-form");
const startSelect = document.querySelector("#route-start");
const destinationSelect = document.querySelector("#route-destination");
const routeOutput = document.querySelector("#route-output");

function escapeHtml(value) {
  return String(value).replace(/[&<>"']/g, character => ({
    "&": "&amp;",
    "<": "&lt;",
    ">": "&gt;",
    '"': "&quot;",
    "'": "&#39;"
  })[character]);
}

function containsIgnoreCase(value, query) {
  return !query || value.toLocaleLowerCase().includes(query.toLocaleLowerCase());
}

function hasSearchDetails(criteria) {
  return Object.values(criteria).some(Boolean);
}

function matchesAnySuppliedDetail(baggage, criteria) {
  return Object.entries(criteria).some(([field, query]) =>
    Boolean(query) && containsIgnoreCase(baggage[field], query)
  );
}

function calculateMatchPercentage(baggage, criteria) {
  let availablePoints = 0;
  let matchedPoints = 0;
  const weightedFields = ["colour", "type", "flightNumber"];

  weightedFields.forEach(field => {
    if (!criteria[field]) return;
    availablePoints += 25;
    if (containsIgnoreCase(baggage[field], criteria[field])) matchedPoints += 25;
  });

  if (criteria.departureAirport && criteria.arrivalAirport) {
    availablePoints += 25;
    if (containsIgnoreCase(baggage.departureAirport, criteria.departureAirport) &&
        containsIgnoreCase(baggage.arrivalAirport, criteria.arrivalAirport)) {
      matchedPoints += 25;
    }
  }

  return availablePoints ? matchedPoints * 100 / availablePoints : 0;
}

function searchBaggage(criteria) {
  if (!hasSearchDetails(criteria)) return [];

  return baggageRecords
    .filter(baggage =>
      containsIgnoreCase(baggage.travelDate, criteria.travelDate) &&
      containsIgnoreCase(baggage.approximateDepartureTime, criteria.approximateDepartureTime) &&
      containsIgnoreCase(baggage.approximateArrivalTime, criteria.approximateArrivalTime) &&
      matchesAnySuppliedDetail(baggage, criteria)
    )
    .map(baggage => ({
      baggage,
      matchPercentage: calculateMatchPercentage(baggage, criteria)
    }))
    .sort((left, right) => right.matchPercentage - left.matchPercentage);
}

function renderBaggageResults(results, hasCriteria) {
  resultCount.textContent = results.length
    ? `${results.length} ${results.length === 1 ? "match" : "matches"}`
    : "";

  if (!hasCriteria) {
    baggageResults.innerHTML = `
      <div class="empty-state">
        <span class="empty-icon" aria-hidden="true">⌕</span>
        <p>Enter a baggage detail above to get started.</p>
        <small>Search uses the fictional sample records shown in demo mode.</small>
      </div>`;
    return;
  }

  if (!results.length) {
    baggageResults.innerHTML = `
      <div class="empty-state">
        <span class="empty-icon" aria-hidden="true">⌕</span>
        <p>No matching sample baggage records found.</p>
        <small>Try fewer or different details. Search checks for text contained in a field.</small>
      </div>`;
    return;
  }

  baggageResults.innerHTML = results.map(({ baggage, matchPercentage }) => `
    <article class="baggage-card">
      <div class="card-top">
        <div>
          <p class="bag-description">${escapeHtml(baggage.colour)} ${escapeHtml(baggage.type)}</p>
          <small class="bag-id">Baggage ID ${escapeHtml(baggage.id)} · ${escapeHtml(baggage.ownerName)}</small>
        </div>
        <span class="match-score">${matchPercentage.toFixed(1)}% match</span>
      </div>
      <p class="bag-route">${escapeHtml(baggage.departureAirport)}<span aria-hidden="true">→</span>${escapeHtml(baggage.arrivalAirport)}</p>
      <dl class="card-meta">
        <div><dt>Flight</dt><dd>${escapeHtml(baggage.flightNumber)}</dd></div>
        <div><dt>Last checkpoint</dt><dd>${escapeHtml(baggage.lastCheckpoint)}</dd></div>
        <div><dt>Travel date</dt><dd>${escapeHtml(baggage.travelDate)}</dd></div>
        <div><dt>Departure time</dt><dd>${escapeHtml(baggage.approximateDepartureTime)}</dd></div>
      </dl>
    </article>`
  ).join("");
}

function makeAdjacencyList() {
  const adjacency = new Map(locations.map(location => [location.name, []]));
  corridors.forEach(([from, to, distance]) => {
    adjacency.get(from).push({ name: to, distance });
    adjacency.get(to).push({ name: from, distance });
  });
  return adjacency;
}

function findShortestRoute(start, destination) {
  const adjacency = makeAdjacencyList();
  const distances = new Map(locations.map(location => [location.name, Infinity]));
  const previous = new Map();
  const pending = [{ name: start, distance: 0 }];
  distances.set(start, 0);

  while (pending.length) {
    pending.sort((left, right) => right.distance - left.distance);
    const current = pending.pop();
    if (current.distance !== distances.get(current.name)) continue;
    if (current.name === destination) break;

    adjacency.get(current.name).forEach(corridor => {
      const candidate = current.distance + corridor.distance;
      if (candidate < distances.get(corridor.name)) {
        distances.set(corridor.name, candidate);
        previous.set(corridor.name, current.name);
        pending.push({ name: corridor.name, distance: candidate });
      }
    });
  }

  if (!Number.isFinite(distances.get(destination))) return null;

  const path = [destination];
  while (path[0] !== start) path.unshift(previous.get(path[0]));
  const routeCorridors = path.slice(1).map((name, index) =>
    corridors.find(corridor =>
      (corridor[0] === path[index] && corridor[1] === name) ||
      (corridor[1] === path[index] && corridor[0] === name)
    )
  );

  return {
    path,
    routeCorridors,
    distance: distances.get(destination),
    walkingMinutes: distances.get(destination) / 1.4 / 60
  };
}

function turnInstruction(previous, current, next) {
  const before = locations.find(location => location.name === previous);
  const at = locations.find(location => location.name === current);
  const after = locations.find(location => location.name === next);
  const incomingX = at.x - before.x;
  const incomingY = at.y - before.y;
  const outgoingX = after.x - at.x;
  const outgoingY = after.y - at.y;
  const crossProduct = incomingX * outgoingY - incomingY * outgoingX;
  const dotProduct = incomingX * outgoingX + incomingY * outgoingY;

  if (Math.abs(crossProduct) <= 1e-9) {
    return dotProduct < 0 ? "turn around" : "continue straight";
  }
  return crossProduct > 0 ? "turn left" : "turn right";
}

function createInstructions(path, routeCorridors) {
  if (path.length < 2) return [];
  const instructions = [];
  let startIndex = 0;

  while (startIndex + 1 < path.length) {
    let nextIndex = startIndex + 1;
    let segmentDistance = routeCorridors[startIndex][2];

    while (nextIndex + 1 < path.length &&
           turnInstruction(path[nextIndex - 1], path[nextIndex], path[nextIndex + 1]) === "continue straight") {
      segmentDistance += routeCorridors[nextIndex][2];
      nextIndex += 1;
    }

    let instruction = `Continue straight for ${segmentDistance.toFixed(1)} metres`;
    if (nextIndex + 1 < path.length) {
      instruction += `, then ${turnInstruction(path[nextIndex - 1], path[nextIndex], path[nextIndex + 1])}` +
        ` at ${path[nextIndex]} towards ${path[nextIndex + 1]}`;
    } else {
      instruction += ` to ${path[nextIndex]}`;
    }
    instructions.push(instruction);
    startIndex = nextIndex;
  }

  return instructions;
}

function populateLocationSelects() {
  const options = locations.map(location =>
    `<option value="${escapeHtml(location.name)}">${escapeHtml(location.name)}</option>`
  ).join("");
  startSelect.innerHTML = options;
  destinationSelect.innerHTML = options;
  startSelect.value = "Entrance";
  destinationSelect.value = "Gate A1";
}

function renderRoute() {
  const start = startSelect.value;
  const destination = destinationSelect.value;
  if (start === destination) {
    routeOutput.innerHTML = `
      <div class="empty-state">
        <span class="empty-icon" aria-hidden="true">⌖</span>
        <p>You are already at your destination.</p>
        <small>Choose a different destination to see directions.</small>
      </div>`;
    return;
  }

  const route = findShortestRoute(start, destination);
  if (!route) {
    routeOutput.innerHTML = `
      <div class="empty-state">
        <span class="empty-icon" aria-hidden="true">⌖</span>
        <p>No route could be calculated for those locations.</p>
      </div>`;
    return;
  }

  const instructions = createInstructions(route.path, route.routeCorridors);
  routeOutput.innerHTML = `
    <div class="route-summary">
      <div class="route-stat"><strong>${route.distance.toFixed(1)} m</strong><span>Total distance</span></div>
      <div class="route-stat"><strong>${route.walkingMinutes.toFixed(1)} min</strong><span>Estimated walking time</span></div>
    </div>
    <div class="route-path" aria-label="Route">
      ${route.path.map((place, index) => `
        ${index ? '<span class="path-chevron" aria-hidden="true">›</span>' : ""}
        <span class="path-place">${escapeHtml(place)}</span>`
      ).join("")}
    </div>
    <h3 class="instruction-title">Turn-by-turn instructions</h3>
    <ol class="instruction-list">
      ${instructions.map(instruction => `<li>${escapeHtml(instruction)}</li>`).join("")}
    </ol>
    <p class="notice"><span aria-hidden="true">ⓘ</span> Distances and walking time are illustrative estimates, based on a sample walking speed of 1.4 m/s.</p>`;
}

function setActiveTab(tab) {
  document.querySelectorAll('[role="tab"]').forEach(button => {
    const isActive = button === tab;
    button.classList.toggle("is-active", isActive);
    button.setAttribute("aria-selected", String(isActive));
    button.tabIndex = isActive ? 0 : -1;
    document.getElementById(button.getAttribute("aria-controls")).hidden = !isActive;
  });
}

document.querySelectorAll('[role="tab"]').forEach((tab, index, tabs) => {
  tab.addEventListener("click", () => setActiveTab(tab));
  tab.addEventListener("keydown", event => {
    if (!["ArrowLeft", "ArrowRight", "Home", "End"].includes(event.key)) return;
    event.preventDefault();
    let nextIndex = index;
    if (event.key === "ArrowRight") nextIndex = (index + 1) % tabs.length;
    if (event.key === "ArrowLeft") nextIndex = (index - 1 + tabs.length) % tabs.length;
    if (event.key === "Home") nextIndex = 0;
    if (event.key === "End") nextIndex = tabs.length - 1;
    tabs[nextIndex].focus();
    setActiveTab(tabs[nextIndex]);
  });
});

baggageForm.addEventListener("submit", event => {
  event.preventDefault();
  const criteria = Object.fromEntries(
    [...new FormData(baggageForm).entries()].map(([field, value]) => [field, value.trim()])
  );
  renderBaggageResults(searchBaggage(criteria), hasSearchDetails(criteria));
});

baggageForm.addEventListener("reset", () => {
  requestAnimationFrame(() => renderBaggageResults([], false));
});

directionsForm.addEventListener("submit", event => {
  event.preventDefault();
  renderRoute();
});

populateLocationSelects();
