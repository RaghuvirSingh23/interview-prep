/*
 * LLD PROBLEM: Smart Home Automation System
 * 
 * Design a smart home system with the following features:
 * 
 * 1. DEVICES
 *    - Different device types: Light, Thermostat, SecurityCamera, DoorLock
 *    - Each device has: id, name, status (on/off), and type-specific properties
 *      - Light: brightness (0-100)
 *      - Thermostat: temperature, mode (heat/cool/auto)
 *      - SecurityCamera: recording (true/false), resolution
 *      - DoorLock: locked (true/false)
 *    - Devices can be turned on/off and configured
 * 
 * 2. COMMANDS
 *    - Commands can be executed on devices: TurnOn, TurnOff, SetBrightness, etc.
 *    - Commands can be undone (undo last action)
 *    - Commands can be queued and executed in batch
 * 
 * 3. AUTOMATION RULES
 *    - Rules trigger actions based on conditions
 *    - Example: "If motion detected, turn on lights"
 *    - Example: "If temperature > 75, set AC to cool"
 *    - Example: "At 10pm, lock all doors"
 *    - Multiple conditions can be combined (AND/OR)
 * 
 * 4. SCENES
 *    - Predefined configurations: "Movie Night", "Away", "Good Morning"
 *    - Scene activates multiple device settings at once
 *    - Scenes are built with multiple device commands
 * 
 * 5. NOTIFICATIONS
 *    - User notified when: device state changes, security alerts, automation triggers
 *    - Multiple notification channels: App, Email, SMS
 * 
 * 6. CENTRAL HUB
 *    - Single hub manages all devices
 *    - Hub maintains device registry
 *    - Hub executes commands and rules
 * 
 * Patterns to identify:
 * ---------------------
 *   - How to create different device types?
 *   - How to build complex scenes with many device settings?
 *   - How to execute and undo commands?
 *   - How to notify users through different channels?
 *   - How to ensure single hub instance?
 *   - How to handle different automation conditions?
 * 
 * Example Output:
 * ---------------
 *   [HUB] Smart Home Hub initialized
 *   [DEVICE] Light "Living Room Light" created
 *   [DEVICE] Thermostat "Main Thermostat" created
 *   [COMMAND] TurnOn executed on Living Room Light
 *   [COMMAND] SetBrightness(80) executed on Living Room Light
 *   [SCENE] "Movie Night" activated
 *     -> Living Room Light: brightness 20
 *     -> Main Thermostat: 72°F
 *   [COMMAND] Undo: SetBrightness reverted to 100
 *   [RULE] Motion detected -> Turning on Porch Light
 *   [NOTIFY] App: Motion detected at front door
 *   [NOTIFY] SMS: Security alert sent
 */

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <stack>
#include <functional>

using namespace std;

// TODO: Implement your solution




// ==================== TEST CASES ====================
void runTests() {
    cout << "========================================" << endl;
    cout << "   SMART HOME SYSTEM TESTS" << endl;
    cout << "========================================" << endl << endl;

    // Test 1: Singleton - Hub is single instance
    cout << "TEST 1: Hub Singleton" << endl;
    cout << "---------------------" << endl;
    cout << endl;

    // Test 2: Factory - Create different device types
    cout << "TEST 2: Device Factory" << endl;
    cout << "----------------------" << endl;
    // Create Light, Thermostat, Camera, DoorLock
    cout << endl;

    // Test 3: Command - Execute and Undo
    cout << "TEST 3: Command Pattern (Execute & Undo)" << endl;
    cout << "----------------------------------------" << endl;
    // TurnOn light, SetBrightness, Undo brightness, Undo turn on
    cout << endl;

    // Test 4: Builder - Build a Scene
    cout << "TEST 4: Scene Builder" << endl;
    cout << "---------------------" << endl;
    // Build "Movie Night" scene with multiple device settings
    cout << endl;

    // Test 5: Observer - Notifications
    cout << "TEST 5: Notification Observer" << endl;
    cout << "-----------------------------" << endl;
    // Subscribe App, Email, SMS channels
    // Trigger notification, all channels receive
    cout << endl;

    // Test 6: Strategy - Different notification channels
    cout << "TEST 6: Notification Strategies" << endl;
    cout << "-------------------------------" << endl;
    // App notification, SMS notification, Email notification
    cout << endl;

    // Test 7: Full integration
    cout << "TEST 7: Complete Smart Home Flow" << endl;
    cout << "--------------------------------" << endl;
    // Create devices, build scene, execute commands, trigger automation, notify
    cout << endl;

    cout << "========================================" << endl;
    cout << "   ALL TESTS COMPLETED" << endl;
    cout << "========================================" << endl;
}

int main() {
    runTests();
    return 0;
}
