# Design Patterns in C++

A practical guide to common design patterns for interview preparation.

---

## Table of Contents

1. [Introduction to Design Patterns](#introduction)
2. [Creational Patterns](#creational-patterns)
  - [Singleton](#singleton)
  - [Factory Method](#factory-method)
  - [Abstract Factory](#abstract-factory)
  - [Builder](#builder)
3. [Structural Patterns](#structural-patterns)
  - [Adapter](#adapter)
  - [Decorator](#decorator)
  - [Facade](#facade)
4. [Behavioral Patterns](#behavioral-patterns)
  - [Observer](#observer)
  - [Strategy](#strategy)
  - [Iterator](#iterator)
5. [Pattern Selection Guide](#pattern-selection-guide)
6. [Interview Questions](#interview-questions)

---

## Introduction

### What are Design Patterns?

Design patterns are **reusable solutions to common software design problems**. They are not code you can copy-paste, but templates for solving problems that can be adapted to your specific situation.

### Categories


| Category       | Purpose                    | Examples                     |
| -------------- | -------------------------- | ---------------------------- |
| **Creational** | Object creation mechanisms | Singleton, Factory, Builder  |
| **Structural** | Object composition         | Adapter, Decorator, Facade   |
| **Behavioral** | Object communication       | Observer, Strategy, Iterator |


---

## Creational Patterns

### Singleton

**Intent**: Ensure a class has only one instance and provide a global point of access.

**When to Use**:

- Database connections
- Configuration managers
- Logging services
- Thread pools

```cpp
#include <iostream>
#include <mutex>
#include <memory>
using namespace std;

// ============================================
// SINGLETON - Meyer's Singleton (Recommended)
// ============================================
class Logger {
private:
    // Private constructor - no one can create instances
    Logger() { cout << "Logger initialized" << endl; }
    
    // Delete copy/move - prevent copying
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

public:
    // The magic: static local variable = created once, lives forever
    static Logger& getInstance() {
        static Logger instance;  // Thread-safe in C++11+
        return instance;
    }
    
    void log(const string& message) {
        cout << "[LOG] " << message << endl;
    }
};

// Usage
int main() {
    // Both calls return the SAME instance
    Logger::getInstance().log("Application started");
    Logger::getInstance().log("Processing data...");
    
    Logger& log1 = Logger::getInstance();
    Logger& log2 = Logger::getInstance();
    // &log1 == &log2  ✓ Same address!
    
    return 0;
}
```

**Pros**: Controlled access, lazy initialization, thread-safe
**Cons**: Global state, difficult to test, hidden dependencies

---

### Factory Method

**Intent**: Define an interface for creating objects, but let subclasses decide which class to instantiate.

**When to Use**:

- When you don't know the exact type of objects to create
- When you want to delegate creation to subclasses
- When creating objects involves complex logic

```cpp
#include <iostream>
#include <memory>
#include <string>
using namespace std;

// ============================================
// PRODUCT - What we're creating
// ============================================
class Document {
public:
    virtual void open() = 0;
    virtual void save() = 0;
    virtual ~Document() = default;
};

class PDFDocument : public Document {
public:
    void open() override { cout << "Opening PDF document" << endl; }
    void save() override { cout << "Saving PDF document" << endl; }
};

class WordDocument : public Document {
public:
    void open() override { cout << "Opening Word document" << endl; }
    void save() override { cout << "Saving Word document" << endl; }
};

// ============================================
// SIMPLE FACTORY - Most common in practice
// ============================================
class DocumentFactory {
public:
    static unique_ptr<Document> create(const string& type) {
        if (type == "pdf")  return make_unique<PDFDocument>();
        if (type == "word") return make_unique<WordDocument>();
        throw invalid_argument("Unknown document type");
    }
};

// Usage
int main() {
    // Create documents without knowing the exact class
    auto doc1 = DocumentFactory::create("pdf");
    auto doc2 = DocumentFactory::create("word");
    
    doc1->open();  // "Opening PDF document"
    doc2->open();  // "Opening Word document"
    
    return 0;
}
```

---

### Abstract Factory

**Intent**: Provide an interface for creating families of related objects without specifying their concrete classes.

**When to Use**:

- When you need to create families of related products
- When you want to ensure products from the same family are used together
- Cross-platform UI components, themed applications

```cpp
#include <iostream>
#include <memory>
using namespace std;

// ============================================
// ABSTRACT PRODUCTS - Button and Checkbox
// ============================================
class Button {
public:
    virtual void render() = 0;
    virtual ~Button() = default;
};

class Checkbox {
public:
    virtual void render() = 0;
    virtual ~Checkbox() = default;
};

// ============================================
// WINDOWS FAMILY
// ============================================
class WindowsButton : public Button {
public:
    void render() override { cout << "[Windows Button]" << endl; }
};

class WindowsCheckbox : public Checkbox {
public:
    void render() override { cout << "[Windows Checkbox]" << endl; }
};

// ============================================
// MAC FAMILY
// ============================================
class MacButton : public Button {
public:
    void render() override { cout << "[Mac Button]" << endl; }
};

class MacCheckbox : public Checkbox {
public:
    void render() override { cout << "[Mac Checkbox]" << endl; }
};

// ============================================
// ABSTRACT FACTORY - Creates a family of products
// ============================================
class GUIFactory {
public:
    virtual unique_ptr<Button> createButton() = 0;
    virtual unique_ptr<Checkbox> createCheckbox() = 0;
    virtual ~GUIFactory() = default;
};

class WindowsFactory : public GUIFactory {
public:
    unique_ptr<Button> createButton() override { return make_unique<WindowsButton>(); }
    unique_ptr<Checkbox> createCheckbox() override { return make_unique<WindowsCheckbox>(); }
};

class MacFactory : public GUIFactory {
public:
    unique_ptr<Button> createButton() override { return make_unique<MacButton>(); }
    unique_ptr<Checkbox> createCheckbox() override { return make_unique<MacCheckbox>(); }
};

// ============================================
// CLIENT - Uses factory without knowing concrete types
// ============================================
void renderUI(GUIFactory& factory) {
    auto button = factory.createButton();
    auto checkbox = factory.createCheckbox();
    button->render();
    checkbox->render();
}

int main() {
    cout << "=== Windows UI ===" << endl;
    WindowsFactory winFactory;
    renderUI(winFactory);
    
    cout << "\n=== Mac UI ===" << endl;
    MacFactory macFactory;
    renderUI(macFactory);
    
    return 0;
}
```

---

### Builder

**Intent**: Separate the construction of a complex object from its representation, allowing the same construction process to create different representations.

**When to Use**:

- Creating objects with many optional parameters
- Constructing complex objects step by step
- When you want to avoid "telescoping constructors"

```cpp
#include <iostream>
#include <string>
#include <map>
using namespace std;

// ============================================
// PRODUCT - Complex object with many fields
// ============================================
class HttpRequest {
public:
    string method = "GET";
    string url;
    string body;
    map<string, string> headers;
    int timeout = 30;
    
    void send() {
        cout << method << " " << url << endl;
        for (auto& [key, value] : headers) {
            cout << "  " << key << ": " << value << endl;
        }
        if (!body.empty()) cout << "  Body: " << body << endl;
    }
};

// ============================================
// BUILDER - Fluent interface for construction
// ============================================
class HttpRequestBuilder {
private:
    HttpRequest request;
    
public:
    // Each method returns *this for chaining
    HttpRequestBuilder& method(const string& m) { request.method = m; return *this; }
    HttpRequestBuilder& url(const string& u)    { request.url = u; return *this; }
    HttpRequestBuilder& body(const string& b)   { request.body = b; return *this; }
    HttpRequestBuilder& timeout(int t)          { request.timeout = t; return *this; }
    
    HttpRequestBuilder& header(const string& key, const string& value) {
        request.headers[key] = value;
        return *this;
    }
    
    HttpRequest build() {
        if (request.url.empty()) throw runtime_error("URL required");
        return request;
    }
};

// Usage - Clean, readable construction
int main() {
    auto request = HttpRequestBuilder()
        .method("POST")
        .url("https://api.example.com/users")
        .header("Content-Type", "application/json")
        .header("Authorization", "Bearer token123")
        .body(R"({"name": "John"})")
        .timeout(60)
        .build();
    
    request.send();
    
    return 0;
}
```

---

## Structural Patterns

### Adapter

**Intent**: Convert the interface of a class into another interface clients expect. Allows incompatible interfaces to work together.

**When to Use**:

- Integrating legacy code with new systems
- Using third-party libraries with different interfaces
- Making incompatible classes work together

```cpp
#include <iostream>
#include <string>
using namespace std;

// ============================================
// OLD INTERFACE - What we have (legacy code)
// ============================================
class OldPrinter {
public:
    void printOld(const string& text) {
        cout << "OLD PRINTER: " << text << endl;
    }
};

// ============================================
// NEW INTERFACE - What client expects
// ============================================
class Printer {
public:
    virtual void print(const string& text) = 0;
    virtual ~Printer() = default;
};

// ============================================
// ADAPTER - Makes old work with new
// ============================================
class PrinterAdapter : public Printer {
private:
    OldPrinter oldPrinter;  // Wraps the old interface
    
public:
    void print(const string& text) override {
        // Translate new interface to old
        oldPrinter.printOld(text);
    }
};

// Client code expects Printer interface
void printDocument(Printer& printer, const string& doc) {
    printer.print(doc);
}

int main() {
    // Can't use OldPrinter directly with printDocument()
    // OldPrinter old;
    // printDocument(old, "Hello");  // ERROR!
    
    // Use adapter to make it compatible
    PrinterAdapter adapter;
    printDocument(adapter, "Hello World!");  // Works!
    
    return 0;
}
```

---

### Decorator

**Intent**: Attach additional responsibilities to an object dynamically. Provides a flexible alternative to subclassing for extending functionality.

**When to Use**:

- Adding features to objects without modifying their class
- When extension by subclassing is impractical
- For responsibilities that can be added/removed at runtime

```cpp
#include <iostream>
#include <string>
#include <memory>
using namespace std;

// ============================================
// COMPONENT - Base interface
// ============================================
class Coffee {
public:
    virtual string getDescription() const = 0;
    virtual double getCost() const = 0;
    virtual ~Coffee() = default;
};

// ============================================
// CONCRETE COMPONENT - Basic coffee
// ============================================
class SimpleCoffee : public Coffee {
public:
    string getDescription() const override { return "Coffee"; }
    double getCost() const override { return 2.0; }
};

// ============================================
// DECORATOR BASE - Wraps a Coffee
// ============================================
class CoffeeDecorator : public Coffee {
protected:
    unique_ptr<Coffee> coffee;
public:
    CoffeeDecorator(unique_ptr<Coffee> c) : coffee(move(c)) {}
};

// ============================================
// CONCRETE DECORATORS - Add features
// ============================================
class Milk : public CoffeeDecorator {
public:
    Milk(unique_ptr<Coffee> c) : CoffeeDecorator(move(c)) {}
    string getDescription() const override { return coffee->getDescription() + " + Milk"; }
    double getCost() const override { return coffee->getCost() + 0.5; }
};

class Sugar : public CoffeeDecorator {
public:
    Sugar(unique_ptr<Coffee> c) : CoffeeDecorator(move(c)) {}
    string getDescription() const override { return coffee->getDescription() + " + Sugar"; }
    double getCost() const override { return coffee->getCost() + 0.2; }
};

class Whip : public CoffeeDecorator {
public:
    Whip(unique_ptr<Coffee> c) : CoffeeDecorator(move(c)) {}
    string getDescription() const override { return coffee->getDescription() + " + Whip"; }
    double getCost() const override { return coffee->getCost() + 0.7; }
};

int main() {
    // Plain coffee
    unique_ptr<Coffee> order1 = make_unique<SimpleCoffee>();
    cout << order1->getDescription() << " = $" << order1->getCost() << endl;
    
    // Coffee with milk and sugar (wrap decorators around each other)
    unique_ptr<Coffee> order2 = make_unique<Sugar>(
                                    make_unique<Milk>(
                                        make_unique<SimpleCoffee>()));
    cout << order2->getDescription() << " = $" << order2->getCost() << endl;
    
    // Fancy coffee with everything
    unique_ptr<Coffee> order3 = make_unique<Whip>(
                                    make_unique<Sugar>(
                                        make_unique<Milk>(
                                            make_unique<SimpleCoffee>())));
    cout << order3->getDescription() << " = $" << order3->getCost() << endl;
    
    return 0;
}
```

**Output**:

```
Coffee = $2
Coffee + Milk + Sugar = $2.7
Coffee + Milk + Sugar + Whip = $3.4
```

---

### Facade

**Intent**: Provide a unified interface to a set of interfaces in a subsystem. Defines a higher-level interface that makes the subsystem easier to use.

**When to Use**:

- Simplifying complex subsystems
- Reducing dependencies between clients and subsystems
- Creating layers in your architecture

```cpp
#include <iostream>
#include <string>
using namespace std;

// ============================================
// COMPLEX SUBSYSTEM - Many classes to coordinate
// ============================================
class CPU {
public:
    void freeze()  { cout << "  CPU: Freeze" << endl; }
    void execute() { cout << "  CPU: Execute" << endl; }
};

class Memory {
public:
    void load(const string& data) { cout << "  Memory: Load " << data << endl; }
};

class HardDrive {
public:
    string read() { cout << "  HardDrive: Read boot sector" << endl; return "OS_DATA"; }
};

// ============================================
// FACADE - Simple interface to complex system
// ============================================
class ComputerFacade {
private:
    CPU cpu;
    Memory memory;
    HardDrive hd;
    
public:
    // One simple method hides all the complexity
    void start() {
        cout << "Starting computer..." << endl;
        cpu.freeze();
        string data = hd.read();
        memory.load(data);
        cpu.execute();
        cout << "Computer ready!" << endl;
    }
    
    void shutdown() {
        cout << "Shutting down..." << endl;
        cpu.freeze();
        cout << "Goodbye!" << endl;
    }
};

int main() {
    // Client only needs to know about the Facade
    // No need to understand CPU, Memory, HardDrive
    ComputerFacade computer;
    computer.start();
    computer.shutdown();
    
    return 0;
}
```

---

## Behavioral Patterns

### Observer

**Intent**: Define a one-to-many dependency between objects so that when one object changes state, all its dependents are notified and updated automatically.

**When to Use**:

- Event handling systems
- MVC architecture (Model notifies Views)
- Distributed event handling
- Subscription/notification systems

```cpp
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
using namespace std;

// ============================================
// OBSERVER - Gets notified of changes
// ============================================
class Observer {
public:
    virtual void onNotify(const string& event) = 0;
    virtual ~Observer() = default;
};

// ============================================
// SUBJECT - Maintains list of observers
// ============================================
class Subject {
    vector<Observer*> observers;
    
public:
    void subscribe(Observer* obs)   { observers.push_back(obs); }
    void unsubscribe(Observer* obs) { 
        observers.erase(remove(observers.begin(), observers.end(), obs), observers.end());
    }
    
    void notify(const string& event) {
        for (auto* obs : observers) {
            obs->onNotify(event);
        }
    }
};

// ============================================
// CONCRETE SUBJECT - News Agency
// ============================================
class NewsAgency : public Subject {
public:
    void publishNews(const string& news) {
        cout << "NEWS AGENCY: " << news << endl;
        notify(news);  // Tell all subscribers
    }
};

// ============================================
// CONCRETE OBSERVERS
// ============================================
class TVChannel : public Observer {
    string name;
public:
    TVChannel(const string& n) : name(n) {}
    void onNotify(const string& event) override {
        cout << "  " << name << " broadcasting: " << event << endl;
    }
};

class Website : public Observer {
    string url;
public:
    Website(const string& u) : url(u) {}
    void onNotify(const string& event) override {
        cout << "  " << url << " updated: " << event << endl;
    }
};

int main() {
    NewsAgency agency;
    
    TVChannel cnn("CNN");
    TVChannel bbc("BBC");
    Website site("news.com");
    
    // Subscribe to news
    agency.subscribe(&cnn);
    agency.subscribe(&bbc);
    agency.subscribe(&site);
    
    agency.publishNews("Mars discovery!");
    
    cout << "\n--- BBC unsubscribes ---\n" << endl;
    agency.unsubscribe(&bbc);
    
    agency.publishNews("New Mars images!");
    
    return 0;
}
```

**Output**:

```
NEWS AGENCY: Mars discovery!
  CNN broadcasting: Mars discovery!
  BBC broadcasting: Mars discovery!
  news.com updated: Mars discovery!

--- BBC unsubscribes ---

NEWS AGENCY: New Mars images!
  CNN broadcasting: New Mars images!
  news.com updated: New Mars images!
```

---

### Strategy

**Intent**: Define a family of algorithms, encapsulate each one, and make them interchangeable. Strategy lets the algorithm vary independently from clients that use it.

**When to Use**:

- When you have multiple algorithms for a specific task
- When you want to avoid conditional statements for selecting behavior
- When you need to switch algorithms at runtime

```cpp
#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
using namespace std;

// ============================================
// STRATEGY INTERFACE
// ============================================
class SortStrategy {
public:
    virtual void sort(vector<int>& data) = 0;
    virtual ~SortStrategy() = default;
};

// ============================================
// CONCRETE STRATEGIES
// ============================================
class BubbleSort : public SortStrategy {
public:
    void sort(vector<int>& data) override {
        cout << "Using Bubble Sort" << endl;
        for (size_t i = 0; i < data.size(); i++) {
            for (size_t j = 0; j < data.size() - i - 1; j++) {
                if (data[j] > data[j + 1]) swap(data[j], data[j + 1]);
            }
        }
    }
};

class QuickSort : public SortStrategy {
public:
    void sort(vector<int>& data) override {
        cout << "Using Quick Sort" << endl;
        std::sort(data.begin(), data.end());  // Simplified for clarity
    }
};

class MergeSort : public SortStrategy {
public:
    void sort(vector<int>& data) override {
        cout << "Using Merge Sort" << endl;
        std::stable_sort(data.begin(), data.end());  // Simplified
    }
};

// ============================================
// CONTEXT - Uses a strategy
// ============================================
class Sorter {
    unique_ptr<SortStrategy> strategy;
    
public:
    void setStrategy(unique_ptr<SortStrategy> s) {
        strategy = move(s);
    }
    
    void sort(vector<int>& data) {
        if (strategy) strategy->sort(data);
    }
};

void print(const vector<int>& v) {
    for (int n : v) cout << n << " ";
    cout << endl;
}

int main() {
    vector<int> data = {64, 34, 25, 12, 22, 11, 90};
    Sorter sorter;
    
    // Use bubble sort
    sorter.setStrategy(make_unique<BubbleSort>());
    sorter.sort(data);
    print(data);
    
    // Switch to quick sort at runtime!
    data = {5, 2, 8, 1, 9};
    sorter.setStrategy(make_unique<QuickSort>());
    sorter.sort(data);
    print(data);
    
    return 0;
}
```

---

### Iterator

**Intent**: Provide a way to access the elements of an aggregate object sequentially without exposing its underlying representation.

**When to Use**:

- Traversing collections without exposing internal structure
- Supporting multiple traversal algorithms
- Providing a uniform interface for traversing different collections

```cpp
#include <iostream>
#include <vector>
#include <string>
using namespace std;

// ============================================
// SIMPLE ITERATOR EXAMPLE
// ============================================
class Playlist {
    vector<string> songs;
    
public:
    void add(const string& song) { songs.push_back(song); }
    
    // Iterator - nested class that knows how to traverse
    class Iterator {
        const Playlist& playlist;
        size_t index = 0;
    public:
        Iterator(const Playlist& p) : playlist(p) {}
        
        bool hasNext() const { return index < playlist.songs.size(); }
        string next() { return playlist.songs[index++]; }
        void reset() { index = 0; }
    };
    
    Iterator createIterator() const { return Iterator(*this); }
};

int main() {
    Playlist playlist;
    playlist.add("Song 1");
    playlist.add("Song 2");
    playlist.add("Song 3");
    
    // Use our custom iterator
    auto it = playlist.createIterator();
    cout << "Playing:" << endl;
    while (it.hasNext()) {
        cout << "  " << it.next() << endl;
    }
    
    return 0;
}
```

**Note**: In modern C++, the STL provides iterators for all containers. The Iterator pattern is built into the language through `begin()`, `end()`, and range-based for loops.

```cpp
// Modern C++ - Iterators are built-in!
vector<int> nums = {1, 2, 3, 4, 5};

// Old style with iterators
for (auto it = nums.begin(); it != nums.end(); ++it) {
    cout << *it << " ";
}

// Modern range-based for (preferred)
for (int n : nums) {
    cout << n << " ";
}
```

---

## Pattern Selection Guide


| Problem                                 | Pattern              | Why                                  |
| --------------------------------------- | -------------------- | ------------------------------------ |
| Need only one instance                  | **Singleton**        | Controlled access to single instance |
| Create objects without specifying class | **Factory**          | Decouple creation from usage         |
| Create families of related objects      | **Abstract Factory** | Ensure compatible products           |
| Complex object construction             | **Builder**          | Step-by-step construction            |
| Incompatible interfaces                 | **Adapter**          | Convert interface                    |
| Add responsibilities dynamically        | **Decorator**        | Flexible extension                   |
| Simplify complex subsystem              | **Facade**           | Unified interface                    |
| Notify multiple objects of changes      | **Observer**         | Loose coupling                       |
| Switch algorithms at runtime            | **Strategy**         | Interchangeable algorithms           |
| Traverse collection uniformly           | **Iterator**         | Hide internal structure              |


---

## Interview Questions

### Q1: What's the difference between Factory Method and Abstract Factory?


| Factory Method         | Abstract Factory             |
| ---------------------- | ---------------------------- |
| Creates ONE product    | Creates FAMILIES of products |
| Uses inheritance       | Uses composition             |
| Single method          | Multiple factory methods     |
| Subclasses decide type | Factory object decides types |


### Q2: When would you use Decorator over Inheritance?

Use **Decorator** when:

- You need to add responsibilities at runtime
- You want to combine features flexibly
- Inheritance would lead to class explosion

Use **Inheritance** when:

- The relationship is truly "is-a"
- Behavior is fixed at compile time
- You need to override protected methods

### Q3: How is Strategy different from State?


| Strategy                   | State                           |
| -------------------------- | ------------------------------- |
| Client chooses algorithm   | State changes automatically     |
| Algorithms are independent | States know about each other    |
| Replaces behavior          | Changes behavior based on state |


### Q4: What are the drawbacks of Singleton?

1. **Global state** - Hard to reason about
2. **Testing difficulty** - Hard to mock
3. **Hidden dependencies** - Not explicit in interfaces
4. **Thread safety concerns** - Need careful implementation
5. **Violates SRP** - Controls instantiation AND provides functionality

### Q5: How would you implement Observer in a thread-safe way?

```cpp
#include <shared_mutex>
using namespace std;

class ThreadSafeSubject {
    vector<Observer*> observers;
    mutable shared_mutex mtx;
    
public:
    void attach(Observer* obs) {
        unique_lock lock(mtx);       // Exclusive lock for writing
        observers.push_back(obs);
    }
    
    void notify(const string& msg) {
        shared_lock lock(mtx);       // Shared lock for reading
        for (auto* obs : observers) {
            obs->onNotify(msg);
        }
    }
};
```

---

## Quick Reference

```
┌─────────────────────────────────────────────────────────────────┐
│                    DESIGN PATTERNS CHEAT SHEET                  │
├─────────────────────────────────────────────────────────────────┤
│ CREATIONAL                                                      │
│   Singleton    → One instance only                              │
│   Factory      → Create without specifying exact class          │
│   Builder      → Complex objects step by step                   │
├─────────────────────────────────────────────────────────────────┤
│ STRUCTURAL                                                      │
│   Adapter      → Convert incompatible interface                 │
│   Decorator    → Add behavior dynamically                       │
│   Facade       → Simplify complex subsystem                     │
├─────────────────────────────────────────────────────────────────┤
│ BEHAVIORAL                                                      │
│   Observer     → Notify dependents of changes                   │
│   Strategy     → Interchangeable algorithms                     │
│   Iterator     → Sequential access to elements                  │
└─────────────────────────────────────────────────────────────────┘
```

---

*Good luck with your NextHop.AI interview!*