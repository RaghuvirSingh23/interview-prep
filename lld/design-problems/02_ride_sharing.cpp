/*
 * LLD PROBLEM: Ride-Sharing System (Uber/Lyft)
 * 
 * Design a ride-sharing system with the following features:
 * 
 * 1. Users can request rides with pickup, destination, distance
 * 2. Different vehicle types available: Economy, Premium, SUV
 *    - Economy: cheapest, multiplier 1.0x
 *    - Premium: mid-tier, multiplier 1.5x
 *    - SUV: expensive, multiplier 2.0x
 * 
 * 3. Different pricing modes:
 *    - Normal: baseFare + (distance * perKmRate * vehicleMultiplier)
 *    - Surge: normal price * 1.5 (during peak hours)
 *    - Discount: normal price * 0.8 (20% off promo)
 * 
 * 4. Drivers can go online/offline
 *    - Online drivers get notified when a ride is requested
 *    - Offline drivers don't receive notifications
 * 
 * 5. App configuration should be centralized:
 *    - baseFare = $5
 *    - perKmRate = $2
 *    - surgeFactor = 1.5
 *    - Only ONE config instance should exist
 * 
 * Example Flow:
 * -------------
 *   - User requests a ride: Downtown -> Airport, 25km, Premium vehicle, Surge pricing
 *   - System calculates price
 *   - All online drivers get notified
 *   - Driver accepts, ride starts
 * 
 * Expected Output Examples:
 * -------------------------
 *   Creating Economy vehicle...
 *   Creating Premium vehicle...
 *   
 *   Ride Request:
 *     From: Downtown
 *     To: Airport
 *     Distance: 25 km
 *     Vehicle: Premium
 *     Price: $82.50
 *   
 *   [DRIVER] Alice received: New ride available - Downtown to Airport
 *   [DRIVER] Bob received: New ride available - Downtown to Airport
 * 
 * Think about:
 * ------------
 *   - How to ensure only one config exists?
 *   - How to create different vehicle types cleanly?
 *   - How to build a ride request with many optional fields?
 *   - How to swap pricing algorithms easily?
 *   - How to notify multiple drivers without tight coupling?
 */

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <ctime>

using namespace std;

// TODO: Implement your solution here

// ==================== Logging ====================
class Logger{
private:
    vector <string> logHistory;
    Logger(){
        cout << "[Logger initialised]" << endl;
    }

public:
    static Logger& getInstance(){
        static Logger instance;
        return instance;
    }

    void log(string message){
        time_t timestamp = time(nullptr);
        string timeStr = ctime(&timestamp);
        timeStr.pop_back();  // remove trailing newline from ctime
        string logMessage = "[" + timeStr + "] " + message;
        logHistory.push_back(logMessage);
        cout << logMessage << endl;
    }

    void printLogs(){
        for(string logMessage: logHistory){
            cout << logMessage << endl;
        }
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};

// global logger
Logger& logger = Logger::getInstance();

// ==================== App Config ====================
class AppConfig{
    // Singleton obviously, and Single Responsibility
private:
    AppConfig(){
        logger.log("Config initialised");
    }
public:
    double baseFare = 5;
    double perKmRate = 2;
    double surgeFactor = 1.5;

    // new fields go here (Open/Close principle not very relevant here imo)

    static AppConfig& getInstance(){
        logger.log("App config instance fetched");
        static AppConfig instance;
        return instance;
    }

    AppConfig(const AppConfig&) = delete;
    AppConfig& operator=(const AppConfig&) = delete;
};

// ==================== Vehicle ====================

class Vehicle{
public:
    string type;
    double multiplier;

    Vehicle(string t, double m){
        type = t;
        multiplier = m;
        logger.log("Vehicle of type " + type + " created");
    }

    virtual ~Vehicle() = default;
};

class Economy : public Vehicle{
public:
    Economy() : Vehicle("Economy", 1){}
};

class Premium : public Vehicle{
public:
    Premium() : Vehicle("Premium", 1.5){}
};

class SUV : public Vehicle{
public:
    SUV() : Vehicle("SUV", 2){}
};

class VehicleFactory{
public:
    static unique_ptr<Vehicle> create(string type){
        if(type == "Economy") return make_unique<Economy>();
        if(type == "Premium") return make_unique<Premium>();
        if(type == "SUV") return make_unique<SUV>();

        logger.log("Invalid Vehicle Type requested");
        throw invalid_argument("Invalid Vehicle type");
    }
};

// ==================== Pricing ====================

class PricingStrategy {
public:
    virtual double calculate(double distance, double vehicleMultiplier) = 0;
    virtual ~PricingStrategy() = default;
};

class NormalPricing : public PricingStrategy {
public:
    double calculate(double distance, double vehicleMultiplier) override {
        return AppConfig::getInstance().baseFare + (distance*AppConfig::getInstance().perKmRate*vehicleMultiplier);
    }
};

class SurgePricing : public PricingStrategy {
public:
    double calculate(double distance, double vehicleMultiplier) override {
        return 1.5*(AppConfig::getInstance().baseFare + (distance*AppConfig::getInstance().perKmRate*vehicleMultiplier));
    }
};

class DiscountPricing : public PricingStrategy {
public:
    double calculate(double distance, double vehicleMultiplier) override {
        return 0.8*(AppConfig::getInstance().baseFare + (distance*AppConfig::getInstance().perKmRate*vehicleMultiplier));
    }
};

// ==================== RideRequest ====================

class RideRequest {
friend class RideRequestBuilder;
private:
    string pickup, destination, vehicleType, pricingType;
    double distance;
public:
    void printRideDetails(){
        cout << pickup << " to " << destination << " with " << vehicleType << " vehicle and " << pricingType << " pricing and Distance: " << distance << " km" << endl;
    }
};

class RideRequestBuilder{
private:
    RideRequest rideRequest;
public:
    RideRequestBuilder& setPickup(const string& pickup){
        rideRequest.pickup = pickup;
        return *this;
    }
    
    RideRequestBuilder& setDestination(const string& destination){
        rideRequest.destination = destination;
        return *this;
    }
    
    RideRequestBuilder& setDistance(double distance){
        rideRequest.distance = distance;
        return *this;
    }
    
    RideRequestBuilder& setVehicleType(const string& vehicleType){
        rideRequest.vehicleType = vehicleType;
        return *this;
    }
    
    RideRequestBuilder& setPricingType(const string& pricingType){
        rideRequest.pricingType = pricingType;
        return *this;
    }
    
    RideRequest build(){
        return rideRequest;
    }
};

// ==================== Driver/RideNotifications (Observer Pattern) ====================

// Observer - receives notifications
class Driver {
private:
    string name;
public:
    Driver(const string& name) : name(name) {}
    
    string getName() const { return name; }
    
    void notify(const string& message) {
        logger.log("[DRIVER] " + name + " received: " + message);
    }
};

// Subject - manages observers and sends notifications
class RideNotifier {
private:
    vector<Driver*> drivers;  // subscribed (online) drivers
    
public:
    void subscribe(Driver* driver) {
        drivers.push_back(driver);
        logger.log(driver->getName() + " is now online");
    }
    
    void unsubscribe(Driver* driver) {
        drivers.erase(
            remove(drivers.begin(), drivers.end(), driver),
            drivers.end()
        );
        logger.log(driver->getName() + " is now offline");
    }
    
    void notifyAll(const string& message) {
        for (Driver* driver : drivers) {
            driver->notify(message);
        }
    }
};

// ==================== TEST CASES ====================
void runTests() {
    cout << "========================================" << endl;
    cout << "   RIDE-SHARING SYSTEM TESTS" << endl;
    cout << "========================================" << endl << endl;

    // Test 1: Singleton - Config same instance
    cout << "TEST 1: Config - Same instance check (Singleton)" << endl;
    cout << "-------------------------------------------------" << endl;
    AppConfig& config1 = AppConfig::getInstance();
    AppConfig& config2 = AppConfig::getInstance();
    cout << "config1 address: " << &config1 << endl;
    cout << "config2 address: " << &config2 << endl;
    cout << "Result: " << ((&config1 == &config2) ? "PASS - Same instance" : "FAIL") << endl;
    cout << "BaseFare: $" << config1.baseFare << ", PerKmRate: $" << config1.perKmRate << endl;
    cout << endl;

    // Test 2: Factory - Create different vehicles
    cout << "TEST 2: Create different vehicle types (Factory)" << endl;
    cout << "------------------------------------------------" << endl;
    auto economy = VehicleFactory::create("Economy");
    auto premium = VehicleFactory::create("Premium");
    auto suv = VehicleFactory::create("SUV");
    cout << "Economy - Type: " << economy->type << ", Multiplier: " << economy->multiplier << endl;
    cout << "Premium - Type: " << premium->type << ", Multiplier: " << premium->multiplier << endl;
    cout << "SUV     - Type: " << suv->type << ", Multiplier: " << suv->multiplier << endl;
    cout << endl;

    // Test 3: Builder - Build a ride request
    cout << "TEST 3: Build ride request (Builder)" << endl;
    cout << "------------------------------------" << endl;
    RideRequest ride = RideRequestBuilder()
        .setPickup("Downtown")
        .setDestination("Airport")
        .setDistance(25.0)
        .setVehicleType("Premium")
        .setPricingType("Surge")
        .build();
    ride.printRideDetails();
    cout << endl;

    // Test 4: Strategy - Different pricing calculations
    cout << "TEST 4: Pricing calculations (Strategy)" << endl;
    cout << "---------------------------------------" << endl;
    double distance = 10.0;
    double vehicleMultiplier = 1.5; // Premium
    
    NormalPricing normalPricing;
    SurgePricing surgePricing;
    DiscountPricing discountPricing;
    
    cout << "Distance: " << distance << " km, Vehicle Multiplier: " << vehicleMultiplier << "x" << endl;
    cout << "Normal price:   $" << normalPricing.calculate(distance, vehicleMultiplier) << " (expected: $35)" << endl;
    cout << "Surge price:    $" << surgePricing.calculate(distance, vehicleMultiplier) << " (expected: $52.50)" << endl;
    cout << "Discount price: $" << discountPricing.calculate(distance, vehicleMultiplier) << " (expected: $28)" << endl;
    cout << endl;

    // Test 5: Observer - Notify online drivers
    cout << "TEST 5: Notify online drivers (Observer)" << endl;
    cout << "----------------------------------------" << endl;
    RideNotifier notifier;
    
    Driver alice("Alice");
    Driver bob("Bob");
    Driver charlie("Charlie");
    
    notifier.subscribe(&alice);
    notifier.subscribe(&bob);
    notifier.subscribe(&charlie);
    
    cout << "Notifying all 3 drivers:" << endl;
    notifier.notifyAll("New ride: Downtown -> Airport, Premium, $52.50");
    cout << endl;

    // Test 6: Observer - Offline driver not notified
    cout << "TEST 6: Offline driver not notified (Observer)" << endl;
    cout << "----------------------------------------------" << endl;
    notifier.unsubscribe(&bob);
    cout << "Bob went offline. Notifying remaining drivers:" << endl;
    notifier.notifyAll("New ride: Mall -> Station, Economy, $15");
    cout << endl;

    // Test 7: Full integration - Complete ride flow
    cout << "TEST 7: Complete ride request flow (All Patterns)" << endl;
    cout << "-------------------------------------------------" << endl;
    
    // 1. Get config (Singleton)
    AppConfig& config = AppConfig::getInstance();
    cout << "1. Config loaded - BaseFare: $" << config.baseFare << endl;
    
    // 2. Create vehicle (Factory)
    auto vehicle = VehicleFactory::create("SUV");
    cout << "2. Vehicle created - " << vehicle->type << " (" << vehicle->multiplier << "x)" << endl;
    
    // 3. Build ride request (Builder)
    RideRequest fullRide = RideRequestBuilder()
        .setPickup("Home")
        .setDestination("Office")
        .setDistance(15.0)
        .setVehicleType("SUV")
        .setPricingType("Normal")
        .build();
    cout << "3. Ride built - ";
    fullRide.printRideDetails();
    
    // 4. Calculate price (Strategy)
    NormalPricing pricing;
    double price = pricing.calculate(15.0, vehicle->multiplier);
    cout << "4. Price calculated - $" << price << endl;
    
    // 5. Notify drivers (Observer)
    cout << "5. Notifying drivers:" << endl;
    notifier.notifyAll("New ride: Home -> Office, SUV, $" + to_string(price));
    cout << endl;

    cout << "========================================" << endl;
    cout << "   ALL TESTS COMPLETED" << endl;
    cout << "========================================" << endl;
}

int main() {
    runTests();
    return 0;
}


/*
NOTES:
-----
- App config should be a singleton since it remains constant. 
- To maintain 
*/