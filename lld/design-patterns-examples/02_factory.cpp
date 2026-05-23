/*
 * FACTORY PATTERN - Notification System
 *
 * Problem:
 * --------
 * Design a notification system that can send different types of notifications:
 * - EmailNotification
 * - SMSNotification
 * - PushNotification
 *
 * Requirements:
 * 1. Create a base class/interface `Notification` with method: send(message)
 * 2. Create concrete classes for each notification type
 * 3. Create a `NotificationFactory` that creates the right notification based
 * on type
 *
 * The factory should have:
 *   static Notification* create(string type)
 *   - "email" -> returns EmailNotification
 *   - "sms"   -> returns SMSNotification
 *   - "push"  -> returns PushNotification
 *   - unknown -> returns nullptr
 *
 * Each send() should print something like:
 *   [EMAIL] Sending: <message>
 *   [SMS] Sending: <message>
 *   [PUSH] Sending: <message>
 *
 * Example Usage:
 *   Notification* notif = NotificationFactory::create("email");
 *   notif->send("Hello World");
 *   delete notif;
 *
 * Bonus: Why do we need virtual destructor in the base class?
 */

#include <iostream>
#include <string>
#include <memory>

using namespace std;

// TODO: Implement Notification base class (with virtual send and virtual
// destructor)

class Notification{
public:
    virtual void send(string message) = 0;
    virtual ~Notification() = default;
};

// TODO: Implement EmailNotification
class EmailNotification : public Notification{
public:
    void send(string message){
        cout << "[EMAIL] sending " << message << endl;
    }
};

// TODO: Implement SMSNotification
class SMSNotification : public Notification{
public:
    void send(string message){
        cout << "[SMS] sending " << message << endl;
    }
};

// TODO: Implement PushNotification
class PushNotification : public Notification{
public:
    void send(string message){
        cout << "[PUSH] sending " << message << endl;
    }
};

// TODO: Implement NotificationFactory
class NotificationFactory{
public:
    static unique_ptr<Notification> create(string type){
        if(type == "email") return make_unique<EmailNotification>();
        if(type == "sms") return make_unique<SMSNotification>();
        if(type == "push") return make_unique<PushNotification>();
        return nullptr;
    }
};

int main() {
    auto notif = NotificationFactory::create("email");
    notif->send("Hello World");
    notif = NotificationFactory::create("sms");
    notif->send("Hello World");
    notif = NotificationFactory::create("push");
    notif->send("Hello World");
    return 0;
}