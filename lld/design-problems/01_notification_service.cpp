/*
 * LLD PROBLEM: Notification Service
 * 
 * Design a notification service that can:
 * 
 * 1. Send notifications through multiple channels (Email, SMS, Push)
 * 2. Users can subscribe/unsubscribe to notifications
 * 3. Support priority levels (LOW, MEDIUM, HIGH)
 * 4. HIGH priority sends to ALL channels, MEDIUM to user's preferred, LOW to email only
 * 
 * Expected Output Example:
 *   [EMAIL] To: john@email.com - "Your order shipped"
 *   [SMS] To: +1234567890 - "Your order shipped"
 *   [PUSH] To: John - "Your order shipped"
 * 
 * Think about:
 * - How to represent users and their contact info
 * - How to handle different notification channels
 * - How to manage subscriptions
 * - Which design patterns might help
 */

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

enum class Priority { LOW, MEDIUM, HIGH };

// TODO: Implement your solution

class User {
public:
    string name;
    string email;
    string number;
    string preferredChannel;

    User (string name, string email, string number, string preferredChannel)
    : name(name), email(email), number(number), preferredChannel(preferredChannel){}
};

class Notification {
public:
    string message;
    Priority priority;
    
    Notification(string message, Priority priority) 
    : message(message), priority(priority){}
    
    vector<string> getChannels(const User& user) const {
        if (priority == Priority::HIGH) 
            return {"email", "sms", "push"};
        if (priority == Priority::MEDIUM) 
            return {user.preferredChannel};
        return {"email"};
    }
};

class Channel {
public:
    virtual void send(User& user, const Notification& notification) = 0;
    virtual ~Channel() = default;
};

class EmailChannel : public Channel {
public:
    void send(User& user, const Notification& notification) override {
        cout << "[EMAIL] To: " << user.email << " - " << notification.message << endl;
    }
};

class SMSChannel : public Channel {
public:
    void send(User& user, const Notification& notification) override {
        cout << "[SMS] To: " << user.number << " - " << notification.message << endl;
    }
};

class PushChannel : public Channel {
public:
    void send(User& user, const Notification& notification) override {
        cout << "[PUSH] To: " << user.name << " - " << notification.message << endl;
    }
};

class Service {
private:
    vector<User> subscribers;
    unordered_map<string, Channel*> channels;

public:
    Service() {
        channels["email"] = new EmailChannel();
        channels["sms"] = new SMSChannel();
        channels["push"] = new PushChannel();
    }
    
    ~Service() {
        for (auto& [name, channel] : channels) {
            delete channel;
        }
    }

    void subscribe(User user) {
        subscribers.push_back(user);
    }

    void notify(Notification& notif) {
        for (auto& user : subscribers) {
            for (const auto& channelName : notif.getChannels(user)) {
                channels[channelName]->send(user, notif);
            }
        }
    }
};

int main(){
    Service service;
    
    User u1("John", "john@email.com", "+123456", "sms");
    User u2("Jane", "jane@email.com", "+789012", "email");
    
    service.subscribe(u1);
    service.subscribe(u2);
    
    cout << "=== HIGH Priority ===" << endl;
    Notification n1("Security Alert!", Priority::HIGH);
    service.notify(n1);
    
    cout << "\n=== MEDIUM Priority ===" << endl;
    Notification n2("Your order shipped", Priority::MEDIUM);
    service.notify(n2);
    
    cout << "\n=== LOW Priority ===" << endl;
    Notification n3("Weekly newsletter", Priority::LOW);
    service.notify(n3);
    
    return 0;
}
/*

Entities:
- user -- has preffered 
- notification
- channel
- service -- has many users

*/
