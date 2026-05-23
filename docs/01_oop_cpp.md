# Object-Oriented Programming in C++

A comprehensive guide to OOP concepts for interview preparation.

---

## Table of Contents

1. [Classes and Objects](#1-classes-and-objects)
2. [Access Specifiers](#2-access-specifiers)
3. [Constructors and Destructors](#3-constructors-and-destructors)
4. [The Rule of 3/5/0](#4-the-rule-of-350)
5. [Inheritance](#5-inheritance)
6. [Polymorphism](#6-polymorphism)
7. [Virtual Functions and VTables](#7-virtual-functions-and-vtables)
8. [Abstract Classes and Interfaces](#8-abstract-classes-and-interfaces)
9. [Encapsulation and Abstraction](#9-encapsulation-and-abstraction)
10. [Interview Questions](#10-interview-questions)

---

## 1. Classes and Objects

### What is a Class?

A **class** is a blueprint or template that defines the properties (data members) and behaviors (member functions) that objects of that type will have.

### What is an Object?

An **object** is an instance of a class - a concrete entity created from the class blueprint.

```cpp
// Class definition (blueprint)
class Car {
public:
    std::string brand;
    int year;
    
    void start() {
        std::cout << brand << " is starting..." << std::endl;
    }
};

// Object creation (instance)
int main() {
    Car myCar;           // Object created on stack
    myCar.brand = "Tesla";
    myCar.year = 2024;
    myCar.start();       // Output: Tesla is starting...
    
    Car* carPtr = new Car();  // Object created on heap
    delete carPtr;            // Must manually free heap memory
    
    return 0;
}
```

### Key Differences: Class vs Struct in C++

| Feature | Class | Struct |
|---------|-------|--------|
| Default access | `private` | `public` |
| Default inheritance | `private` | `public` |
| Use case | Complex objects with behavior | Simple data containers (POD) |

```cpp
struct Point {
    int x, y;  // public by default
};

class Point2 {
    int x, y;  // private by default
public:
    Point2(int x, int y) : x(x), y(y) {}
};
```

---

## 2. Access Specifiers

C++ provides three access specifiers to control visibility:

| Specifier | Same Class | Derived Class | Outside |
|-----------|------------|---------------|---------|
| `public` | Yes | Yes | Yes |
| `protected` | Yes | Yes | No |
| `private` | Yes | No | No |

```cpp
class BankAccount {
private:
    double balance;        // Only accessible within this class
    
protected:
    std::string accountId; // Accessible in this class and derived classes
    
public:
    std::string ownerName; // Accessible everywhere
    
    // Public interface to access private data
    double getBalance() const { return balance; }
    void deposit(double amount) { 
        if (amount > 0) balance += amount; 
    }
};

class SavingsAccount : public BankAccount {
public:
    void showAccountId() {
        // std::cout << balance;    // ERROR: private in base
        std::cout << accountId;     // OK: protected is accessible
        std::cout << ownerName;     // OK: public is accessible
    }
};
```

### Friend Functions and Classes

`friend` breaks encapsulation by allowing external access to private members:

```cpp
class Box {
private:
    double width;
    
public:
    Box(double w) : width(w) {}
    
    // Friend function declaration
    friend void printWidth(const Box& b);
    
    // Friend class declaration
    friend class BoxFactory;
};

// Friend function can access private members
void printWidth(const Box& b) {
    std::cout << "Width: " << b.width << std::endl;
}

class BoxFactory {
public:
    static Box createBox(double w) {
        Box b(w);
        b.width *= 2;  // Can access private member
        return b;
    }
};
```

> **Interview Tip**: Use `friend` sparingly. It violates encapsulation and should only be used when there's a strong reason (e.g., operator overloading, factory patterns).

---

## 3. Constructors and Destructors

### Types of Constructors

```cpp
class Student {
private:
    std::string name;
    int age;
    int* scores;
    
public:
    // 1. Default Constructor
    Student() : name("Unknown"), age(0), scores(nullptr) {
        std::cout << "Default constructor called" << std::endl;
    }
    
    // 2. Parameterized Constructor
    Student(const std::string& n, int a) : name(n), age(a), scores(new int[5]) {
        std::cout << "Parameterized constructor called" << std::endl;
    }
    
    // 3. Copy Constructor (deep copy)
    Student(const Student& other) : name(other.name), age(other.age) {
        std::cout << "Copy constructor called" << std::endl;
        if (other.scores) {
            scores = new int[5];
            std::copy(other.scores, other.scores + 5, scores);
        } else {
            scores = nullptr;
        }
    }
    
    // 4. Move Constructor (C++11)
    Student(Student&& other) noexcept 
        : name(std::move(other.name)), age(other.age), scores(other.scores) {
        std::cout << "Move constructor called" << std::endl;
        other.scores = nullptr;  // Leave source in valid state
        other.age = 0;
    }
    
    // Destructor
    ~Student() {
        std::cout << "Destructor called for " << name << std::endl;
        delete[] scores;
    }
};

int main() {
    Student s1;                          // Default constructor
    Student s2("Alice", 20);             // Parameterized constructor
    Student s3 = s2;                     // Copy constructor
    Student s4 = std::move(s2);          // Move constructor
    Student s5(Student("Bob", 22));      // Move constructor (temporary)
    
    return 0;
}
```

### Initializer Lists vs Assignment

**Always prefer initializer lists** - they're more efficient and required for:
- `const` members
- Reference members
- Members without default constructors
- Base class initialization

```cpp
class Example {
private:
    const int id;           // Must use initializer list
    std::string& ref;       // Must use initializer list
    
public:
    // CORRECT: Using initializer list
    Example(int i, std::string& r) : id(i), ref(r) {}
    
    // WRONG: This won't compile
    // Example(int i, std::string& r) {
    //     id = i;   // Error: can't assign to const
    //     ref = r;  // Error: can't assign to reference
    // }
};
```

### Delegating Constructors (C++11)

```cpp
class Rectangle {
private:
    int width, height;
    
public:
    Rectangle() : Rectangle(0, 0) {}  // Delegates to parameterized
    Rectangle(int side) : Rectangle(side, side) {}  // Square
    Rectangle(int w, int h) : width(w), height(h) {}  // Main constructor
};
```

---

## 4. The Rule of 3/5/0

### Rule of Three (Pre-C++11)

If a class needs any of these, it probably needs all three:
1. **Destructor**
2. **Copy Constructor**
3. **Copy Assignment Operator**

### Rule of Five (C++11+)

Add move semantics:
1. Destructor
2. Copy Constructor
3. Copy Assignment Operator
4. **Move Constructor**
5. **Move Assignment Operator**

### Rule of Zero (Modern C++)

**Prefer the Rule of Zero**: Let the compiler generate everything by using smart pointers and RAII containers.

```cpp
// Rule of Five Example (when managing raw resources)
class Buffer {
private:
    size_t size;
    int* data;
    
public:
    // Constructor
    explicit Buffer(size_t s) : size(s), data(new int[s]) {}
    
    // 1. Destructor
    ~Buffer() { delete[] data; }
    
    // 2. Copy Constructor
    Buffer(const Buffer& other) : size(other.size), data(new int[other.size]) {
        std::copy(other.data, other.data + size, data);
    }
    
    // 3. Copy Assignment Operator
    Buffer& operator=(const Buffer& other) {
        if (this != &other) {  // Self-assignment check
            delete[] data;
            size = other.size;
            data = new int[size];
            std::copy(other.data, other.data + size, data);
        }
        return *this;
    }
    
    // 4. Move Constructor
    Buffer(Buffer&& other) noexcept : size(other.size), data(other.data) {
        other.size = 0;
        other.data = nullptr;
    }
    
    // 5. Move Assignment Operator
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            delete[] data;
            size = other.size;
            data = other.data;
            other.size = 0;
            other.data = nullptr;
        }
        return *this;
    }
};

// Rule of Zero Example (PREFERRED in modern C++)
class ModernBuffer {
private:
    std::vector<int> data;  // RAII container handles everything
    
public:
    explicit ModernBuffer(size_t s) : data(s) {}
    // Compiler generates correct copy/move/destructor automatically!
};
```

---

## 5. Inheritance

### Types of Inheritance

```cpp
class Base {
public:
    int publicMember;
protected:
    int protectedMember;
private:
    int privateMember;
};

// Public Inheritance: "is-a" relationship
class PublicDerived : public Base {
    // publicMember -> public
    // protectedMember -> protected
    // privateMember -> not accessible
};

// Protected Inheritance: rare, implementation inheritance
class ProtectedDerived : protected Base {
    // publicMember -> protected
    // protectedMember -> protected
    // privateMember -> not accessible
};

// Private Inheritance: "implemented-in-terms-of" relationship
class PrivateDerived : private Base {
    // publicMember -> private
    // protectedMember -> private
    // privateMember -> not accessible
};
```

### Multiple Inheritance and the Diamond Problem

```cpp
class Animal {
public:
    virtual void speak() { std::cout << "Animal speaks" << std::endl; }
};

class Mammal : virtual public Animal {  // Virtual inheritance
public:
    void walk() { std::cout << "Mammal walks" << std::endl; }
};

class Bird : virtual public Animal {    // Virtual inheritance
public:
    void fly() { std::cout << "Bird flies" << std::endl; }
};

// Without 'virtual', Bat would have TWO copies of Animal!
class Bat : public Mammal, public Bird {
public:
    void speak() override { std::cout << "Bat squeaks" << std::endl; }
};

int main() {
    Bat b;
    b.speak();  // Unambiguous due to virtual inheritance
    b.walk();
    b.fly();
    return 0;
}
```

### Order of Construction/Destruction

```cpp
class Base {
public:
    Base() { std::cout << "Base constructed" << std::endl; }
    ~Base() { std::cout << "Base destroyed" << std::endl; }
};

class Derived : public Base {
public:
    Derived() { std::cout << "Derived constructed" << std::endl; }
    ~Derived() { std::cout << "Derived destroyed" << std::endl; }
};

// Output:
// Base constructed
// Derived constructed
// Derived destroyed
// Base destroyed
```

> **Key Rule**: Constructors are called base-to-derived, destructors are called derived-to-base.

---

## 6. Polymorphism

### Compile-Time Polymorphism (Static)

Resolved at compile time through:

**1. Function Overloading**
```cpp
class Calculator {
public:
    int add(int a, int b) { return a + b; }
    double add(double a, double b) { return a + b; }
    int add(int a, int b, int c) { return a + b + c; }
};
```

**2. Operator Overloading**
```cpp
class Complex {
private:
    double real, imag;
    
public:
    Complex(double r = 0, double i = 0) : real(r), imag(i) {}
    
    // Operator overloading
    Complex operator+(const Complex& other) const {
        return Complex(real + other.real, imag + other.imag);
    }
    
    // Friend for symmetric operations
    friend std::ostream& operator<<(std::ostream& os, const Complex& c) {
        os << c.real << " + " << c.imag << "i";
        return os;
    }
};
```

**3. Templates (Parametric Polymorphism)**
```cpp
template<typename T>
T maximum(T a, T b) {
    return (a > b) ? a : b;
}

int main() {
    std::cout << maximum(10, 20) << std::endl;      // int version
    std::cout << maximum(10.5, 20.5) << std::endl;  // double version
    return 0;
}
```

### Runtime Polymorphism (Dynamic)

Achieved through **virtual functions** and **inheritance**:

```cpp
class Shape {
public:
    virtual double area() const = 0;  // Pure virtual
    virtual void draw() const {       // Virtual with default
        std::cout << "Drawing shape" << std::endl;
    }
    virtual ~Shape() = default;       // Virtual destructor
};

class Circle : public Shape {
private:
    double radius;
public:
    Circle(double r) : radius(r) {}
    
    double area() const override {
        return 3.14159 * radius * radius;
    }
    
    void draw() const override {
        std::cout << "Drawing circle with radius " << radius << std::endl;
    }
};

class Rectangle : public Shape {
private:
    double width, height;
public:
    Rectangle(double w, double h) : width(w), height(h) {}
    
    double area() const override {
        return width * height;
    }
};

int main() {
    std::vector<std::unique_ptr<Shape>> shapes;
    shapes.push_back(std::make_unique<Circle>(5.0));
    shapes.push_back(std::make_unique<Rectangle>(4.0, 6.0));
    
    for (const auto& shape : shapes) {
        shape->draw();  // Polymorphic call
        std::cout << "Area: " << shape->area() << std::endl;
    }
    return 0;
}
```

---

## 7. Virtual Functions and VTables

### How Virtual Functions Work

When a class has virtual functions, the compiler creates a **Virtual Table (VTable)** - an array of function pointers. Each object of that class has a hidden **vptr** pointing to its class's VTable.

```
┌─────────────────────────────────────────────────────────────────┐
│                        Memory Layout                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   Shape Object          Shape VTable                            │
│  ┌──────────┐          ┌──────────────────┐                     │
│  │  vptr    │─────────>│ &Shape::area     │ (pure virtual)      │
│  ├──────────┤          ├──────────────────┤                     │
│  │  data    │          │ &Shape::draw     │                     │
│  └──────────┘          ├──────────────────┤                     │
│                        │ &Shape::~Shape   │                     │
│                        └──────────────────┘                     │
│                                                                  │
│   Circle Object         Circle VTable                           │
│  ┌──────────┐          ┌──────────────────┐                     │
│  │  vptr    │─────────>│ &Circle::area    │ (overridden)        │
│  ├──────────┤          ├──────────────────┤                     │
│  │  radius  │          │ &Circle::draw    │ (overridden)        │
│  └──────────┘          ├──────────────────┤                     │
│                        │ &Circle::~Circle │                     │
│                        └──────────────────┘                     │
└─────────────────────────────────────────────────────────────────┘
```

### Virtual Destructor - CRITICAL!

**Always make destructors virtual in base classes** when you expect polymorphic deletion:

```cpp
class Base {
public:
    ~Base() { std::cout << "Base destructor" << std::endl; }  // NON-VIRTUAL - BUG!
};

class Derived : public Base {
private:
    int* data;
public:
    Derived() : data(new int[100]) {}
    ~Derived() { 
        delete[] data;  // NEVER CALLED if base destructor is non-virtual!
        std::cout << "Derived destructor" << std::endl; 
    }
};

int main() {
    Base* ptr = new Derived();
    delete ptr;  // Only calls ~Base(), MEMORY LEAK!
    return 0;
}

// FIX: Make base destructor virtual
class Base {
public:
    virtual ~Base() { std::cout << "Base destructor" << std::endl; }
};
```

### override and final (C++11)

```cpp
class Base {
public:
    virtual void foo() const;
    virtual void bar();
    void baz();
};

class Derived : public Base {
public:
    void foo() const override;   // OK: correctly overrides
    // void foo() override;      // ERROR: signature doesn't match (missing const)
    void bar() override final;   // OK: overrides and prevents further overriding
    // void baz() override;      // ERROR: baz is not virtual
};

class MoreDerived : public Derived {
public:
    // void bar() override;      // ERROR: bar is final in Derived
};
```

---

## 8. Abstract Classes and Interfaces

### Abstract Class

A class with at least one **pure virtual function** (`= 0`). Cannot be instantiated.

```cpp
class AbstractShape {
public:
    virtual double area() const = 0;      // Pure virtual
    virtual double perimeter() const = 0; // Pure virtual
    
    // Can still have implemented methods
    void printInfo() const {
        std::cout << "Area: " << area() << ", Perimeter: " << perimeter() << std::endl;
    }
    
    virtual ~AbstractShape() = default;
};

// Must implement ALL pure virtual functions to be concrete
class Square : public AbstractShape {
private:
    double side;
public:
    Square(double s) : side(s) {}
    double area() const override { return side * side; }
    double perimeter() const override { return 4 * side; }
};
```

### Interface (Pure Abstract Class)

C++ doesn't have a formal `interface` keyword like Java. An interface is a class with:
- Only pure virtual functions
- No data members
- Virtual destructor

```cpp
// Interface pattern in C++
class ISerializable {
public:
    virtual std::string serialize() const = 0;
    virtual void deserialize(const std::string& data) = 0;
    virtual ~ISerializable() = default;
};

class IComparable {
public:
    virtual int compareTo(const IComparable& other) const = 0;
    virtual ~IComparable() = default;
};

// A class can implement multiple interfaces
class Person : public ISerializable, public IComparable {
private:
    std::string name;
    int age;
    
public:
    Person(const std::string& n, int a) : name(n), age(a) {}
    
    std::string serialize() const override {
        return name + "," + std::to_string(age);
    }
    
    void deserialize(const std::string& data) override {
        // Parse data...
    }
    
    int compareTo(const IComparable& other) const override {
        const Person& p = dynamic_cast<const Person&>(other);
        return age - p.age;
    }
};
```

---

## 9. Encapsulation and Abstraction

### Encapsulation

**Bundling data and methods** that operate on that data, restricting direct access.

```cpp
class BankAccount {
private:
    double balance;  // Hidden implementation detail
    
    // Private helper method
    bool isValidAmount(double amount) const {
        return amount > 0;
    }
    
public:
    BankAccount(double initial) : balance(initial) {}
    
    // Public interface
    bool deposit(double amount) {
        if (!isValidAmount(amount)) return false;
        balance += amount;
        return true;
    }
    
    bool withdraw(double amount) {
        if (!isValidAmount(amount) || amount > balance) return false;
        balance -= amount;
        return true;
    }
    
    double getBalance() const { return balance; }
};
```

### Abstraction

**Hiding complex implementation** details and showing only necessary features.

```cpp
// User doesn't need to know HOW the database works
class Database {
public:
    virtual void connect() = 0;
    virtual void query(const std::string& sql) = 0;
    virtual void disconnect() = 0;
    virtual ~Database() = default;
};

class MySQLDatabase : public Database {
private:
    // Complex MySQL-specific implementation hidden
    void* connection;
    void initializeDriver() { /* ... */ }
    void handleError(int code) { /* ... */ }
    
public:
    void connect() override {
        initializeDriver();
        // Complex connection logic...
    }
    
    void query(const std::string& sql) override {
        // Complex query execution...
    }
    
    void disconnect() override {
        // Cleanup...
    }
};

// User code only sees the simple interface
void useDatabase(Database& db) {
    db.connect();
    db.query("SELECT * FROM users");
    db.disconnect();
}
```

---

## 10. Interview Questions

### Q1: What is the difference between `new` and `malloc`?

| Feature | `new` | `malloc` |
|---------|-------|----------|
| Type | C++ operator | C function |
| Returns | Typed pointer | `void*` (needs cast) |
| Size | Calculated automatically | Must specify bytes |
| Constructor | Calls constructor | Does not |
| Failure | Throws `std::bad_alloc` | Returns `NULL` |
| Deallocation | `delete` | `free()` |

### Q2: What is object slicing?

When a derived class object is assigned to a base class object, the derived part is "sliced off":

```cpp
class Base { public: int x = 1; };
class Derived : public Base { public: int y = 2; };

Derived d;
Base b = d;  // Slicing! b.y doesn't exist, only b.x
```

**Solution**: Use pointers or references for polymorphism.

### Q3: Can constructors be virtual?

**No.** When a constructor is called, the object doesn't exist yet, so there's no vptr to look up the virtual function. However, you can use the **Virtual Constructor Idiom** (Factory Method):

```cpp
class Base {
public:
    virtual Base* clone() const = 0;  // "Virtual constructor"
};

class Derived : public Base {
public:
    Derived* clone() const override { return new Derived(*this); }
};
```

### Q4: What is RAII?

**Resource Acquisition Is Initialization**: A technique where resource lifetime is tied to object lifetime.

```cpp
class FileHandler {
private:
    FILE* file;
public:
    FileHandler(const char* filename) {
        file = fopen(filename, "r");  // Acquire resource
        if (!file) throw std::runtime_error("Cannot open file");
    }
    ~FileHandler() {
        if (file) fclose(file);  // Release resource
    }
    // ... file operations ...
};

void process() {
    FileHandler fh("data.txt");  // Resource acquired
    // Use file...
}  // fh destroyed here, file automatically closed
```

### Q5: Explain `this` pointer

- Implicit pointer to the current object
- Available in all non-static member functions
- Type: `ClassName* const` (constant pointer)

```cpp
class Counter {
    int count;
public:
    Counter& increment() {
        count++;
        return *this;  // Return reference to self for chaining
    }
};

Counter c;
c.increment().increment().increment();  // Method chaining
```

### Q6: What is the difference between shallow copy and deep copy?

```cpp
class Shallow {
    int* data;
public:
    Shallow(int val) : data(new int(val)) {}
    // Default copy constructor does shallow copy
    // Both objects point to SAME memory!
};

class Deep {
    int* data;
public:
    Deep(int val) : data(new int(val)) {}
    Deep(const Deep& other) : data(new int(*other.data)) {}  // Deep copy
    // Each object has its OWN memory
};
```

### Q7: When would you use `static` members?

1. **Shared data** across all instances (e.g., object count)
2. **Utility functions** that don't need object state
3. **Constants** shared by all instances

```cpp
class Employee {
    static int employeeCount;  // Shared across all objects
    static const int MAX_EMPLOYEES = 100;  // Class constant
    
public:
    Employee() { employeeCount++; }
    ~Employee() { employeeCount--; }
    
    static int getCount() { return employeeCount; }  // No 'this' pointer
};

int Employee::employeeCount = 0;  // Definition required outside class
```

---

## 11. SOLID Principles

### S - Single Responsibility Principle

**"A class should have only one reason to change"**

Each class does ONE thing.

```cpp
// BAD: User class does too much
class User {
    void saveToDatabase();   // persistence
    void sendEmail();        // notification
    void generateReport();   // reporting
};

// GOOD: Separate responsibilities
class User { /* just user data */ };
class UserRepository { void save(User& u); };
class EmailService { void send(User& u, string msg); };
```

---

### O - Open/Closed Principle

**"Open for extension, closed for modification"**

Add new functionality without changing existing code.

```cpp
// BAD: Must modify class for each new type
class PriceCalculator {
    double calculate(string type, double distance) {
        if (type == "normal") return 5 + distance * 2;
        else if (type == "surge") return (5 + distance * 2) * 1.5;
        // Adding new type = modify this class
    }
};

// GOOD: Extend by adding new classes
class PricingStrategy {
public:
    virtual double calculate(double distance) = 0;
};

class NormalPricing : public PricingStrategy {
    double calculate(double distance) override {
        return 5 + distance * 2;
    }
};

class SurgePricing : public PricingStrategy {
    double calculate(double distance) override {
        return (5 + distance * 2) * 1.5;
    }
};

// Adding HolidayPricing = new class, don't touch existing code
```

---

### L - Liskov Substitution Principle

**"Subtypes must be substitutable for their base types"**

If `B` extends `A`, you should be able to use `B` anywhere `A` is expected without breaking things.

```cpp
// BAD: Square breaks Rectangle behavior
class Rectangle {
public:
    virtual void setWidth(int w) { width = w; }
    virtual void setHeight(int h) { height = h; }
    int area() { return width * height; }
protected:
    int width, height;
};

class Square : public Rectangle {
    void setWidth(int w) override { width = height = w; }  // breaks expectation!
    void setHeight(int h) override { width = height = h; }
};

// This breaks:
Rectangle* r = new Square();
r->setWidth(5);
r->setHeight(10);
// Expected area: 50, Actual: 100 (both became 10)

// GOOD: Don't inherit if behavior differs
class Shape {
public:
    virtual int area() = 0;
};
class Rectangle : public Shape { /* width, height */ };
class Square : public Shape { /* side */ };
```

---

### I - Interface Segregation Principle

**"Clients shouldn't depend on interfaces they don't use"**

Keep interfaces small and focused.

```cpp
// BAD: Fat interface
class Worker {
    virtual void work() = 0;
    virtual void eat() = 0;
    virtual void sleep() = 0;
};

class Robot : public Worker {
    void work() override { /* ok */ }
    void eat() override { /* robots don't eat! */ }
    void sleep() override { /* robots don't sleep! */ }
};

// GOOD: Segregated interfaces
class Workable { virtual void work() = 0; };
class Eatable { virtual void eat() = 0; };
class Sleepable { virtual void sleep() = 0; };

class Human : public Workable, public Eatable, public Sleepable { };
class Robot : public Workable { };  // only implements what it needs
```

---

### D - Dependency Inversion Principle

**"Depend on abstractions, not concretions"**

High-level modules shouldn't depend on low-level modules. Both should depend on abstractions.

```cpp
// BAD: Direct dependency on concrete class
class RideService {
    MySQLDatabase db;  // tightly coupled to MySQL
    void saveRide(Ride& r) {
        db.insert(r);  // what if we switch to PostgreSQL?
    }
};

// GOOD: Depend on abstraction
class Database {
public:
    virtual void insert(Ride& r) = 0;
};

class MySQLDatabase : public Database { /* ... */ };
class PostgreSQLDatabase : public Database { /* ... */ };

class RideService {
    Database* db;  // depends on interface
public:
    RideService(Database* database) : db(database) {}
    void saveRide(Ride& r) {
        db->insert(r);  // works with any database
    }
};
```

---

### SOLID Quick Reference

| Principle | One-liner | Violation Sign |
|-----------|-----------|----------------|
| **S**ingle Responsibility | One class, one job | Class has multiple unrelated methods |
| **O**pen/Closed | Extend, don't modify | Adding feature requires changing existing class |
| **L**iskov Substitution | Subclass = drop-in replacement | Subclass breaks parent's behavior |
| **I**nterface Segregation | Small, focused interfaces | Class implements methods it doesn't need |
| **D**ependency Inversion | Depend on interfaces | Class directly instantiates its dependencies |

---

## Quick Reference Card

| Concept | Key Points |
|---------|------------|
| **Encapsulation** | Hide data, expose interface |
| **Abstraction** | Hide complexity, show essentials |
| **Inheritance** | Code reuse, "is-a" relationship |
| **Polymorphism** | Same interface, different behaviors |
| **Virtual** | Enable runtime polymorphism |
| **Pure Virtual** | `= 0`, makes class abstract |
| **Virtual Destructor** | ALWAYS for polymorphic base classes |
| **Rule of 5** | Destructor, copy/move ctor, copy/move assign |
| **Rule of 0** | Use smart pointers, let compiler generate |
| **RAII** | Tie resource lifetime to object lifetime |

---

*Good luck with your interview preparation!*

