"use strict";

const API_BASE_URL = "http://127.0.0.1:8080/api";
const baggageForm = document.querySelector("#baggage-form");
const baggageResults = document.querySelector("#baggage-results");
const resultCount = document.querySelector("#result-count");
const directionsForm = document.querySelector("#directions-form");
const startSelect = document.querySelector("#route-start");
const destinationSelect = document.querySelector("#route-destination");
const routeOutput = document.querySelector("#route-output");
const apiStatus = document.querySelector("#api-status");
const apiStatusText = document.querySelector("#api-status-text");
let baggageRequestNumber = 0;
let routeRequestNumber = 0;

function escapeHtml(value) {
  return String(value).replace(/[&<>"']/g, character => ({
    "&": "&amp;",
    "<": "&lt;",
    ">": "&gt;",
    '"': "&quot;",
    "'": "&#39;"
  })[character]);
}

function setApiStatus(status) {
  apiStatus.classList.remove("is-pending", "is-connected", "is-unavailable");
  if (status === "connected") {
    apiStatus.classList.add("is-connected");
    apiStatusText.textContent = "C++ API CONNECTED · SAMPLE DATA";
  } else if (status === "unavailable") {
    apiStatus.classList.add("is-unavailable");
    apiStatusText.textContent = "C++ API SERVER UNAVAILABLE";
  } else {
    apiStatus.classList.add("is-pending");
    apiStatusText.textContent = "CONNECTING TO C++ API";
  }
}

async function requestApi(path) {
  let response;
  try {
    response = await fetch(`${API_BASE_URL}${path}`);
  } catch (error) {
    setApiStatus("unavailable");
    error.serverUnavailable = true;
    throw error;
  }

  let result;
  try {
    result = await response.json();
  } catch (error) {
    setApiStatus("unavailable");
    const invalidResponse = new Error("The C++ API returned an invalid response.");
    invalidResponse.serverUnavailable = true;
    throw invalidResponse;
  }

  if (!response.ok) {
    const requestError = new Error(result.error || `API request failed (${response.status}).`);
    requestError.serverUnavailable = response.status >= 500;
    setApiStatus(requestError.serverUnavailable ? "unavailable" : "connected");
    throw requestError;
  }
  setApiStatus("connected");
  return result;
}

function renderApiError(container, error) {
  const message = error.serverUnavailable
    ? "C++ API server unavailable. Start the API server and try again."
    : `The request could not be completed: ${error.message}`;
  container.innerHTML = `
    <div class="empty-state error-state">
      <span class="empty-icon" aria-hidden="true">!</span>
      <p>${escapeHtml(message)}</p>
      <small>No local or demo results were substituted.</small>
    </div>`;
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
        <small>Search uses the fictional sample records served by the C++ API.</small>
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
        <span class="match-score">${Number(matchPercentage).toFixed(1)}% match</span>
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

async function loadLocations() {
  try {
    const result = await requestApi("/locations");
    if (!Array.isArray(result.locations) || !result.locations.length) {
      throw new Error("The C++ API returned no airport locations.");
    }

    const options = result.locations.map(location =>
      `<option value="${escapeHtml(location)}">${escapeHtml(location)}</option>`
    ).join("");
    startSelect.innerHTML = options;
    destinationSelect.innerHTML = options;
    startSelect.value = "Entrance";
    destinationSelect.value = "Gate A1";
  } catch (error) {
    if (error.serverUnavailable) setApiStatus("unavailable");
    renderApiError(routeOutput, error);
    startSelect.disabled = true;
    destinationSelect.disabled = true;
    directionsForm.querySelector('button[type="submit"]').disabled = true;
  }
}

function renderRoute(result) {
  if (result.status === "unreachable") {
    routeOutput.innerHTML = `
      <div class="empty-state">
        <span class="empty-icon" aria-hidden="true">⌖</span>
        <p>No route could be calculated for those locations.</p>
      </div>`;
    return;
  }

  if (result.locations.length === 1) {
    routeOutput.innerHTML = `
      <div class="empty-state">
        <span class="empty-icon" aria-hidden="true">⌖</span>
        <p>You are already at your destination.</p>
        <small>Choose a different destination to see directions.</small>
      </div>`;
    return;
  }

  routeOutput.innerHTML = `
    <div class="route-summary">
      <div class="route-stat"><strong>${Number(result.totalDistanceMetres).toFixed(1)} m</strong><span>Total distance</span></div>
      <div class="route-stat"><strong>${Number(result.estimatedWalkingMinutes).toFixed(1)} min</strong><span>Estimated walking time</span></div>
    </div>
    <div class="route-path" aria-label="Route">
      ${result.locations.map((place, index) => `
        ${index ? '<span class="path-chevron" aria-hidden="true">›</span>' : ""}
        <span class="path-place">${escapeHtml(place)}</span>`
      ).join("")}
    </div>
    <h3 class="instruction-title">Turn-by-turn instructions</h3>
    <ol class="instruction-list">
      ${result.instructions.map(instruction => `<li>${escapeHtml(instruction)}</li>`).join("")}
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

baggageForm.addEventListener("submit", async event => {
  event.preventDefault();
  const requestNumber = ++baggageRequestNumber;
  const criteria = Object.fromEntries(
    [...new FormData(baggageForm).entries()].map(([field, value]) => [field, value.trim()])
  );
  const hasCriteria = Object.values(criteria).some(Boolean);
  if (!hasCriteria) {
    renderBaggageResults([], false);
    return;
  }

  const query = new URLSearchParams(criteria).toString();
  try {
    const result = await requestApi(`/baggage?${query}`);
    if (requestNumber === baggageRequestNumber) {
      renderBaggageResults(result.results, true);
    }
  } catch (error) {
    if (requestNumber === baggageRequestNumber) {
      resultCount.textContent = "";
      renderApiError(baggageResults, error);
    }
  }
});

baggageForm.addEventListener("reset", () => {
  baggageRequestNumber += 1;
  requestAnimationFrame(() => renderBaggageResults([], false));
});

directionsForm.addEventListener("submit", async event => {
  event.preventDefault();
  const requestNumber = ++routeRequestNumber;
  const query = new URLSearchParams({
    start: startSelect.value,
    destination: destinationSelect.value
  }).toString();
  try {
    const result = await requestApi(`/route?${query}`);
    if (requestNumber === routeRequestNumber) renderRoute(result);
  } catch (error) {
    if (requestNumber === routeRequestNumber) renderApiError(routeOutput, error);
  }
});

requestApi("/health")
  .then(result => {
    if (result.service !== "AeroTrack C++ API" || result.status !== "ok") {
      const error = new Error("The AeroTrack C++ API could not be verified.");
      error.serverUnavailable = true;
      throw error;
    }
    return loadLocations();
  })
  .catch(error => {
    setApiStatus("unavailable");
    renderApiError(routeOutput, error);
    startSelect.disabled = true;
    destinationSelect.disabled = true;
    directionsForm.querySelector('button[type="submit"]').disabled = true;
  });
