#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <ctime>
using namespace std;

/*

Design a Parking Lot system that supports:

- Multiple floors, each floor has a fixed number of spots
- Three types of spots: Compact, Regular, Large
- Three types of vehicles: Motorcycle, Car, Truck
  - Motorcycle can park in any spot
  - Car can park in Regular or Large
  - Truck can only park in Large
- A vehicle enters, gets assigned the smallest suitable spot on the lowest floor
- A vehicle exits, the spot is freed
- Hourly pricing:
  - Compact: $2/hr
  - Regular: $3/hr
  - Large: $5/hr
- Generate a ticket on entry (vehicle info, spot, entry time)
- Calculate fee on exit based on hours parked

Example flow:
  Parking lot: 2 floors, each has 2 compact + 3 regular + 1 large
  
  Car enters    → assigned Floor 0, Regular spot #0, ticket #1
  Truck enters  → assigned Floor 0, Large spot #0, ticket #2
  Car enters    → assigned Floor 0, Regular spot #1, ticket #3
  Car enters    → assigned Floor 0, Regular spot #2, ticket #4
  Car enters    → assigned Floor 1, Regular spot #0, ticket #5 (floor 0 regular full, car can't use compact)
  Truck enters  → assigned Floor 1, Large spot #0, ticket #6
  Truck enters  → FULL, no large spots left

  Ticket #1 exits after 3 hours → fee = 3 * $3 = $9

Think about:
- Who decides which spot a vehicle gets?
- How to efficiently find the next available spot
- Where does pricing logic live?

Entities
========
  - enum spotType {Compact, Regular, Large};
  - enum vehicleType {Motorcycle, Car, Truck};

  - Spot
    % spot(spotType, true)
    - isAvailable
    - spotType
    + canOccupy(vehicleType)

  - Floor
    % Floor (small, medium, large)
    - spots[]
    - floorLevel
    + findSpot --> return spot based on can Occupy
  
  - Vehicle
    - string numberPlate
    - vehicleType

  - Ticket
    - startTime
    - string numberPlate
    - spotType
    + fee()
  
  - ParkingLot
    % ParkingLot ()
    - Floor[]
    - map <numberPlate, ticket&>
    - map <numberPlate, spot&>
    + addFloor(small, medium, large)
    + entry(numberPlate, vehicleType) 
      --> check for closest spot
    + exit
      --> calculate fee
      --> mark spot as free
      --> remove ticket from map
*/

int main() {
    return 0;
}
