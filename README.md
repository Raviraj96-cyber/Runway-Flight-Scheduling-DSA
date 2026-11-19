# Runway Flight Scheduling and Management (DSA Mini Project)

A console-based C++17 mini project that schedules landing and takeoff requests across two runways using priority-based scheduling and a custom binary min-heap.

## Features
- Add landing and takeoff requests
- Priority order: Emergency, Low Fuel, Normal
- Landing requests are preferred over takeoff requests at the same priority
- Fuel-based priority escalation for landing requests
- Declare an emergency for a queued flight
- Cancel a queued flight
- View the waiting queue in service order
- Assign the next flight or process all waiting flights
- Track runway status and operation history
- Load demo flight data

## Data Structures
| Data structure | Purpose |
|---|---|
| Custom binary min-heap | Select the next flight according to priority |
| `unordered_map<string, int>` | Map flight IDs to heap indexes for lookup and updates |
| `vector<Runway>` | Store runway availability and usage details |
| `vector<string>` | Store the operation log |

## Priority Rules
1. Emergency
2. Low fuel
3. Normal

For equal priority:
1. Landing before takeoff
2. Less remaining fuel first
3. First-come-first-served

## Requirements
- C++17-compatible compiler (such as GCC)

## Compile and Run

### Linux / macOS
```bash
g++ -std=c++17 -Wall -Wextra -pedantic src/flight_scheduler.cpp -o flight_scheduler
./flight_scheduler
```

### Windows (MinGW)
```bash
g++ -std=c++17 -Wall -Wextra -pedantic src/flight_scheduler.cpp -o flight_scheduler.exe
flight_scheduler.exe
```

## Menu
1. Add flight request
2. Declare emergency for a queued flight
3. Cancel flight
4. Show waiting queue
5. Assign next flight to a runway
6. Process all waiting flights
7. Advance time (fuel burns and priority may escalate)
8. Show runway status
9. Show operation log
10. Load demo data
0. Exit

## Notes and Limitations
- This is an educational DSA simulation, not real airport-control software.
- Runway service durations are simplified constants: 5 minutes for landing and 3 minutes for takeoff.
- Flight and runway state is kept in memory and is not saved after the program exits.
- The project uses simulated scheduling rules and should not be used for real aviation decisions.
