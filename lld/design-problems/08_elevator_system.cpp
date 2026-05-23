#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <memory>
#include <functional>
using namespace std;

/*

Design an Elevator System for a building that supports:

- A building with N floors (e.g. 0 to 9)
- M elevators, each can move up/down one floor at a time
- External requests: a person on floor X presses UP or DOWN button
- Internal requests: a person inside elevator E presses floor Y
- Each elevator has a state: IDLE, MOVING_UP, MOVING_DOWN
- Dispatch: when an external request comes in, assign it to the best elevator:
  - Prefer an elevator already moving in the same direction and hasn't passed the floor yet
  - Otherwise, prefer the closest idle elevator
  - Otherwise, queue the request until an elevator becomes available
- Elevator processes requests in order of direction (doesn't reverse mid-trip):
  - If moving UP, handle all upward stops first, then switch direction
  - If moving DOWN, handle all downward stops first, then switch direction
  (This is the SCAN/elevator algorithm — like a disk head)

Example flow:
  Building: 10 floors, 2 elevators (E0 and E1), both start at floor 0, IDLE

  External request: floor 5 UP  → dispatched to E0 (closest idle)
  External request: floor 3 UP  → dispatched to E0 (already going up, will pass floor 3)
  Internal request: E0 floor 7  → E0 adds floor 7 to its stop list
  External request: floor 8 DOWN → dispatched to E1 (E0 is going up)

  E0 path: 0 → 3 (stop, pick up) → 5 (stop, pick up) → 7 (stop, drop off)
  E1 path: 0 → 8 (stop, pick up) → then waits for internal request

Think about:
- How does an elevator decide its next stop?
- Who decides which elevator gets a request — the elevator or a dispatcher?
- How to handle the direction-switching logic (SCAN algorithm)

Entities:
  - Request
    - floor
    - direction

  - Elevator
    - state (UP / DOWN / IDLE)
    - currFloor
    - set<int> upStops
    - set<int> downStops
    + addStop(floor)
    + moveOneStep()
    + hasStops()

  - ElevatorSystem
    - Elevator[]
    + findBestElevator(floor, direction) → Elevator&
    + externalRequest(floor, direction)
    + internalRequest(elevatorId, floor)
    + step()  ← advance all elevators by one tick

  */

int main() {
    return 0;
}
