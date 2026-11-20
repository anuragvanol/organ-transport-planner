# Organ Transport Planner

A C++11 DSA project that allocates fictional donated organs and finds the earliest road route to each compatible recipient hospital before the organ's remaining viability time expires.

## Run

Open a terminal in this folder. No external libraries are required.

```sh
c++ -std=c++11 -Wall -Wextra -pedantic main.cpp routing.cpp allocation.cpp -o planner
./planner
```

The default compares normal roads and a temporary road closure. Run `./planner normal` or `./planner closure` to run either scenario on its own. Edit the fictional hospitals, roads, organs, recipients, and compatibility scores in `main.cpp` to try another scenario.

## How it meets the assignment

| Requirement | Implementation |
| --- | --- |
| Graph theory | The road network is a directed weighted graph. The donor–recipient compatibility graph is bipartite and stored as adjacency lists of scored matches. A match is supplied by the demo data and requires the organ types to agree. |
| Dynamic programming | The state is whether a hospital is reachable at an elapsed minute. Process time buckets in increasing order, discard arrivals past the deadline, and compress the state to the earliest arrival at each hospital. |
| Greedy allocation | Process organs with the least remaining viability first. For each organ, scan its reachable, compatible, unassigned recipients once. Prefer higher urgency, then higher compatibility score, then shorter ETA, then recipient input order. |

The transport budget is viability minus time already elapsed. A road with travel time `w` that opens at minute `r` takes a state at hospital `u` and minute `t` to its neighbor at minute `max(t, r) + w`. Arrival exactly at the budget is allowed. Positive travel times make every transition go forward in time. An earlier arrival at a hospital can wait for any road, so a later arrival there cannot lead to a better result under this road model. Predecessor roads reconstruct the route, and successive departure times show where the vehicle waits.

## Modularity and complexity

- `model.hpp` defines the shared data and interfaces.
- `routing.cpp` computes deadline-constrained routes and reconstructs paths.
- `allocation.cpp` builds the compatibility graph, ranks eligible recipients, and prints decisions.
- `main.cpp` defines the scenarios and runs the comparison.

Let V be the number of road nodes, E the number of directed roads, and T an organ's remaining time budget in minutes. A routing pass takes **O(V + E + T)** time and space before reconstructing paths. Each road is relaxed at most once, each successful improvement creates one bucket entry, and stale entries are skipped. Store only one current arrival label per node. Stop once all requested hospitals are settled. Reconstructing and storing the requested paths adds **O(P)** time and space for P total road segments across those paths.

With N organs, R recipients, and M supplied compatibility edges, the allocation work outside routing takes **O(N log N + R + M)** time. Each organ shares one routing pass across all of its candidate hospitals. Greedy allocation commits each organ immediately, so it can miss a better overall matching across multiple organs.

## Demo walkthrough

1. On normal roads, D1 can reach R1 in 10 minutes and selects R1 on urgency. D2 then selects R2.
2. The East Junction road reopens at minute 9. D1 would reach R1 in 15 minutes via that road, one minute past its budget. R2 and R3 both arrive in 12 minutes with equal urgency, so R3 wins on compatibility.
3. D2 has a larger budget. It can wait five minutes at East Junction, reach R1 in 15 minutes, and choose R1 on urgency. The program prints the wait, ETA, and minutes to spare.

All hospitals, viability budgets, compatibility scores, and allocation rules are fictional classroom data. Closure schedules are relative to the common planning start, and the demo treats the resulting transports as independent.
