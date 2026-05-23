# C/C++ Fundamentals

A comprehensive guide to pointers, memory management, and common interview topics.

---

## Table of Contents

1. [Pointers and References](#1-pointers-and-references)
2. [Memory Layout](#2-memory-layout)
3. [Dynamic Memory Management](#3-dynamic-memory-management)
4. [Smart Pointers](#4-smart-pointers)
5. [Const Correctness](#5-const-correctness)
6. [Static Keyword](#6-static-keyword)
7. [Common Interview Questions](#7-common-interview-questions)

---

## 1. Pointers and References

### What is a Pointer?

A **pointer** is a variable that stores the memory address of another variable.

```cpp
int x = 42;
int* ptr = &x;    // ptr holds the address of x

std::cout << "Value of x: " << x << std::endl;        // 42
std::cout << "Address of x: " << &x << std::endl;     // 0x7ffd...
std::cout << "Value of ptr: " << ptr << std::endl;    // 0x7ffd... (same)
std::cout << "Value at ptr: " << *ptr << std::endl;   // 42 (dereferencing)

*ptr = 100;  // Modify x through pointer
std::cout << "x is now: " << x << std::endl;          // 100
```

### What is a Reference?

A **reference** is an alias for an existing variable. Once initialized, it cannot be changed to refer to another variable.

```cpp
int x = 42;
int& ref = x;    // ref is an alias for x

std::cout << "x: " << x << std::endl;      // 42
std::cout << "ref: " << ref << std::endl;  // 42

ref = 100;
std::cout << "x: " << x << std::endl;      // 100 (x changed through ref)
```

### Key Differences

| Feature | Pointer | Reference |
|---------|---------|-----------|
| Declaration | `int* ptr;` | `int& ref = x;` |
| Initialization | Can be uninitialized | Must be initialized |
| Null | Can be `nullptr` | Cannot be null |
| Reassignment | Can point to different objects | Cannot be reseated |
| Indirection | Use `*` to dereference | Direct access |
| Memory | Has its own memory address | Shares address with target |
| Arithmetic | Supports pointer arithmetic | No arithmetic |

```cpp
// Pointer can be reassigned
int a = 10, b = 20;
int* ptr = &a;
ptr = &b;        // Now points to b

// Reference cannot be reseated
int& ref = a;
ref = b;         // This assigns b's VALUE to a, doesn't rebind ref!
// a is now 20, ref still refers to a
```

### Pointer to Pointer

```cpp
int x = 5;
int* ptr = &x;      // Pointer to int
int** pptr = &ptr;  // Pointer to pointer to int

std::cout << **pptr << std::endl;  // 5
```

### Function Parameters

```cpp
// Pass by value (copy)
void passByValue(int x) {
    x = 100;  // Modifies local copy only
}

// Pass by pointer
void passByPointer(int* ptr) {
    if (ptr) *ptr = 100;  // Modifies original
}

// Pass by reference (preferred in C++)
void passByReference(int& ref) {
    ref = 100;  // Modifies original
}

// Const reference (for read-only access, no copy)
void passByConstRef(const std::string& str) {
    std::cout << str << std::endl;
    // str[0] = 'X';  // ERROR: cannot modify
}

int main() {
    int a = 1, b = 2, c = 3;
    
    passByValue(a);      // a is still 1
    passByPointer(&b);   // b is now 100
    passByReference(c);  // c is now 100
    
    return 0;
}
```

### Array and Pointer Relationship

```cpp
int arr[] = {10, 20, 30, 40, 50};
int* ptr = arr;  // Array decays to pointer

std::cout << arr[2] << std::endl;     // 30
std::cout << *(arr + 2) << std::endl; // 30 (pointer arithmetic)
std::cout << ptr[2] << std::endl;     // 30
std::cout << *(ptr + 2) << std::endl; // 30

// BUT they're not the same!
std::cout << sizeof(arr) << std::endl;  // 20 (5 * sizeof(int))
std::cout << sizeof(ptr) << std::endl;  // 8 (size of pointer on 64-bit)
```

---

## 2. Memory Layout

A C/C++ program's memory is divided into segments:

```
┌─────────────────────────────────────────────────────────────────┐
│                     MEMORY LAYOUT                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  High Address                                                    │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                    STACK                                 │    │
│  │  - Local variables                                       │    │
│  │  - Function parameters                                   │    │
│  │  - Return addresses                                      │    │
│  │  - Grows DOWNWARD                                        │    │
│  │  - Automatic allocation/deallocation                     │    │
│  └─────────────────────────────────────────────────────────┘    │
│                         ↓                                        │
│                    (free space)                                  │
│                         ↑                                        │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                    HEAP                                  │    │
│  │  - Dynamic memory (new/malloc)                           │    │
│  │  - Grows UPWARD                                          │    │
│  │  - Manual management (or smart pointers)                 │    │
│  └─────────────────────────────────────────────────────────┘    │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                    BSS                                   │    │
│  │  - Uninitialized global/static variables                 │    │
│  │  - Zero-initialized by OS                                │    │
│  └─────────────────────────────────────────────────────────┘    │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                    DATA                                  │    │
│  │  - Initialized global/static variables                   │    │
│  │  - String literals                                       │    │
│  └─────────────────────────────────────────────────────────┘    │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                    TEXT (CODE)                           │    │
│  │  - Compiled program code                                 │    │
│  │  - Read-only                                             │    │
│  └─────────────────────────────────────────────────────────┘    │
│  Low Address                                                     │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### Example

```cpp
#include <iostream>

int globalVar = 100;           // DATA segment
int uninitGlobal;              // BSS segment (zero-initialized)
const char* strLiteral = "Hi"; // DATA segment (string in read-only)

void function() {
    static int staticVar = 50; // DATA segment
    int localVar = 10;         // STACK
    int* heapVar = new int(5); // Pointer on STACK, data on HEAP
    
    delete heapVar;
}

int main() {
    int stackVar = 42;         // STACK
    int* heapArray = new int[100]; // HEAP
    
    delete[] heapArray;
    return 0;
}
```

### Stack vs Heap

| Feature | Stack | Heap |
|---------|-------|------|
| Allocation | Automatic | Manual (`new`/`malloc`) |
| Deallocation | Automatic (scope exit) | Manual (`delete`/`free`) |
| Size | Limited (usually 1-8 MB) | Large (limited by RAM) |
| Speed | Very fast | Slower |
| Fragmentation | None | Can fragment |
| Access | LIFO order | Any order |
| Thread safety | Each thread has own stack | Shared, needs synchronization |

---

## 3. Dynamic Memory Management

### C-style (malloc/free)

```cpp
#include <cstdlib>

// Allocate
int* arr = (int*)malloc(5 * sizeof(int));
if (arr == NULL) {
    // Handle allocation failure
}

// Use
for (int i = 0; i < 5; i++) {
    arr[i] = i * 10;
}

// Deallocate
free(arr);
arr = NULL;  // Good practice: avoid dangling pointer
```

### C++ style (new/delete)

```cpp
// Single object
int* ptr = new int(42);       // Allocate and initialize
delete ptr;                    // Deallocate

// Array
int* arr = new int[5];        // Allocate array
delete[] arr;                  // MUST use delete[] for arrays!

// With initialization (C++11)
int* arr2 = new int[5]{1, 2, 3, 4, 5};
delete[] arr2;
```

### Key Differences: new vs malloc

| Feature | `new` | `malloc` |
|---------|-------|----------|
| Language | C++ operator | C function |
| Type safety | Returns typed pointer | Returns `void*` |
| Size | Calculated automatically | Must specify bytes |
| Constructor | Calls constructor | Does not |
| Failure | Throws `std::bad_alloc` | Returns `NULL` |
| Overloadable | Yes | No |
| Pair with | `delete` | `free()` |

### Common Memory Errors

```cpp
// 1. Memory Leak - forgetting to free
void leak() {
    int* ptr = new int[1000];
    // Forgot delete[] ptr;
}  // Memory lost forever!

// 2. Double Free
int* ptr = new int(5);
delete ptr;
delete ptr;  // UNDEFINED BEHAVIOR!

// 3. Dangling Pointer
int* ptr = new int(5);
delete ptr;
*ptr = 10;   // UNDEFINED BEHAVIOR! ptr is dangling

// 4. Array delete mismatch
int* arr = new int[10];
delete arr;  // WRONG! Should be delete[]

// 5. Using uninitialized pointer
int* ptr;
*ptr = 5;    // UNDEFINED BEHAVIOR!
```

---

## 4. Smart Pointers

Smart pointers (C++11+) provide automatic memory management through RAII.

### unique_ptr

**Exclusive ownership** - only one `unique_ptr` can own a resource.

```cpp
#include <memory>

// Creation
std::unique_ptr<int> ptr1 = std::make_unique<int>(42);  // Preferred
std::unique_ptr<int> ptr2(new int(42));                  // Also valid

// Usage
std::cout << *ptr1 << std::endl;  // 42

// Cannot copy
// std::unique_ptr<int> ptr3 = ptr1;  // ERROR!

// Can move
std::unique_ptr<int> ptr3 = std::move(ptr1);  // ptr1 is now nullptr

// Array support
std::unique_ptr<int[]> arr = std::make_unique<int[]>(5);
arr[0] = 10;

// Custom deleter
auto fileDeleter = [](FILE* f) { if (f) fclose(f); };
std::unique_ptr<FILE, decltype(fileDeleter)> file(fopen("test.txt", "r"), fileDeleter);

// Automatic cleanup when scope ends
```

### shared_ptr

**Shared ownership** - multiple `shared_ptr`s can own the same resource. Uses reference counting.

```cpp
#include <memory>

// Creation
std::shared_ptr<int> ptr1 = std::make_shared<int>(42);

// Can copy - reference count increases
std::shared_ptr<int> ptr2 = ptr1;  // ref_count = 2
std::shared_ptr<int> ptr3 = ptr1;  // ref_count = 3

std::cout << ptr1.use_count() << std::endl;  // 3

// When all shared_ptrs go out of scope, memory is freed
ptr1.reset();  // ref_count = 2
ptr2.reset();  // ref_count = 1
// ptr3 goes out of scope -> ref_count = 0 -> memory freed
```

### weak_ptr

**Non-owning reference** to a `shared_ptr`. Breaks circular references.

```cpp
#include <memory>

// Circular reference problem
struct Node {
    std::shared_ptr<Node> next;  // Creates cycle!
    ~Node() { std::cout << "Node destroyed" << std::endl; }
};

void circularLeak() {
    auto node1 = std::make_shared<Node>();
    auto node2 = std::make_shared<Node>();
    node1->next = node2;
    node2->next = node1;  // Cycle! Neither will be freed
}

// Solution: use weak_ptr
struct SafeNode {
    std::weak_ptr<SafeNode> next;  // Doesn't contribute to ref count
    ~SafeNode() { std::cout << "SafeNode destroyed" << std::endl; }
};

void noLeak() {
    auto node1 = std::make_shared<SafeNode>();
    auto node2 = std::make_shared<SafeNode>();
    node1->next = node2;
    node2->next = node1;  // No cycle - weak_ptr doesn't own
}  // Both nodes properly destroyed

// Using weak_ptr
std::shared_ptr<int> shared = std::make_shared<int>(42);
std::weak_ptr<int> weak = shared;

// Check if resource still exists
if (auto locked = weak.lock()) {  // Returns shared_ptr
    std::cout << *locked << std::endl;
} else {
    std::cout << "Resource expired" << std::endl;
}
```

### When to Use Which?

| Smart Pointer | Use When |
|---------------|----------|
| `unique_ptr` | Single owner, exclusive access |
| `shared_ptr` | Multiple owners needed |
| `weak_ptr` | Observing without ownership, breaking cycles |

---

## 5. Const Correctness

### Const with Variables

```cpp
const int x = 10;      // x cannot be modified
// x = 20;             // ERROR!

int y = 20;
const int* ptr1 = &y;  // Pointer to const int
// *ptr1 = 30;         // ERROR: can't modify through ptr1
ptr1 = &x;             // OK: can change what ptr1 points to

int* const ptr2 = &y;  // Const pointer to int
*ptr2 = 30;            // OK: can modify value
// ptr2 = &x;          // ERROR: can't change what ptr2 points to

const int* const ptr3 = &x;  // Const pointer to const int
// *ptr3 = 40;         // ERROR
// ptr3 = &y;          // ERROR
```

### Reading Pointer Declarations

Read **right to left**:
- `const int*` → pointer to const int
- `int* const` → const pointer to int
- `const int* const` → const pointer to const int

### Const Member Functions

```cpp
class Rectangle {
private:
    int width, height;
    
public:
    Rectangle(int w, int h) : width(w), height(h) {}
    
    // Const member function - promises not to modify object
    int getArea() const {
        // width = 10;  // ERROR: can't modify in const function
        return width * height;
    }
    
    // Non-const version
    void setWidth(int w) {
        width = w;  // OK
    }
};

void printArea(const Rectangle& r) {
    std::cout << r.getArea() << std::endl;  // OK: getArea is const
    // r.setWidth(10);  // ERROR: setWidth is not const
}
```

### Mutable Keyword

Allows modification in const member functions (for caching, mutexes, etc.):

```cpp
class DataFetcher {
private:
    mutable int cacheHits = 0;  // Can modify even in const functions
    mutable std::mutex mutex;
    
public:
    std::string getData() const {
        std::lock_guard<std::mutex> lock(mutex);  // OK: mutex is mutable
        cacheHits++;  // OK: cacheHits is mutable
        return "data";
    }
};
```

---

## 6. Static Keyword

### Static Local Variables

Persist across function calls:

```cpp
void counter() {
    static int count = 0;  // Initialized only once
    count++;
    std::cout << "Called " << count << " times" << std::endl;
}

int main() {
    counter();  // Called 1 times
    counter();  // Called 2 times
    counter();  // Called 3 times
    return 0;
}
```

### Static Class Members

Shared across all instances:

```cpp
class Employee {
private:
    static int totalEmployees;  // Shared by all objects
    std::string name;
    
public:
    Employee(const std::string& n) : name(n) {
        totalEmployees++;
    }
    
    ~Employee() {
        totalEmployees--;
    }
    
    static int getTotal() {  // Static function - no 'this' pointer
        // std::cout << name;  // ERROR: can't access non-static members
        return totalEmployees;
    }
};

// Must define static member outside class
int Employee::totalEmployees = 0;

int main() {
    std::cout << Employee::getTotal() << std::endl;  // 0
    
    Employee e1("Alice");
    Employee e2("Bob");
    std::cout << Employee::getTotal() << std::endl;  // 2
    
    {
        Employee e3("Charlie");
        std::cout << Employee::getTotal() << std::endl;  // 3
    }  // e3 destroyed
    
    std::cout << Employee::getTotal() << std::endl;  // 2
    return 0;
}
```

### Static in File Scope (Internal Linkage)

```cpp
// file1.cpp
static int secretVar = 42;  // Only visible in this file
static void secretFunc() {} // Only visible in this file

// file2.cpp
// Cannot access secretVar or secretFunc from here
```

---

## 7. Common Interview Questions

### Q1: What is a dangling pointer?

A pointer that points to memory that has been freed or is out of scope.

```cpp
int* createDangling() {
    int local = 42;
    return &local;  // DANGER: local is destroyed after return
}

int* ptr = createDangling();  // ptr is dangling!
```

### Q2: What is a memory leak?

Memory that is allocated but never freed, making it unavailable for reuse.

```cpp
void leak() {
    int* ptr = new int[1000];
    // Function returns without delete[] ptr
}  // 4000 bytes leaked
```

### Q3: Difference between `delete` and `delete[]`?

- `delete` - frees single object, calls one destructor
- `delete[]` - frees array, calls destructor for each element

```cpp
int* single = new int(5);
delete single;  // Correct

int* array = new int[5];
delete[] array;  // Correct

// Mismatched delete is UNDEFINED BEHAVIOR
```

### Q4: What is a void pointer?

A generic pointer that can point to any data type but cannot be dereferenced directly.

```cpp
void* vptr;
int x = 10;
float f = 3.14;

vptr = &x;  // OK
vptr = &f;  // OK

// *vptr = 20;  // ERROR: can't dereference void*
int* iptr = static_cast<int*>(vptr);
*iptr = 20;  // OK after casting
```

### Q5: What is a null pointer?

A pointer that points to nothing (address 0).

```cpp
// C++11 and later
int* ptr = nullptr;  // Preferred

// C-style (avoid in modern C++)
int* ptr2 = NULL;
int* ptr3 = 0;

// Always check before dereferencing
if (ptr != nullptr) {
    *ptr = 10;
}
```

### Q6: Explain pointer arithmetic

```cpp
int arr[] = {10, 20, 30, 40, 50};
int* ptr = arr;

std::cout << *ptr << std::endl;       // 10
std::cout << *(ptr + 1) << std::endl; // 20 (moves by sizeof(int))
std::cout << *(ptr + 2) << std::endl; // 30

ptr++;  // Now points to arr[1]
std::cout << *ptr << std::endl;       // 20

// Pointer difference
int* end = arr + 5;
std::cout << (end - arr) << std::endl; // 5 (number of elements)
```

### Q7: What is the size of a pointer?

Depends on the architecture:
- 32-bit system: 4 bytes
- 64-bit system: 8 bytes

```cpp
std::cout << sizeof(int*) << std::endl;    // 8 on 64-bit
std::cout << sizeof(char*) << std::endl;   // 8 on 64-bit
std::cout << sizeof(double*) << std::endl; // 8 on 64-bit
// All pointers are the same size!
```

### Q8: What is a function pointer?

A pointer that stores the address of a function.

```cpp
int add(int a, int b) { return a + b; }
int subtract(int a, int b) { return a - b; }

// Function pointer declaration
int (*operation)(int, int);

operation = add;
std::cout << operation(5, 3) << std::endl;  // 8

operation = subtract;
std::cout << operation(5, 3) << std::endl;  // 2

// Using typedef for readability
typedef int (*MathOp)(int, int);
MathOp op = add;

// Modern C++: using
using MathOperation = int(*)(int, int);
MathOperation op2 = subtract;
```

### Q9: What is `this` pointer?

An implicit pointer to the current object, available in non-static member functions.

```cpp
class Counter {
    int count;
public:
    Counter& increment() {
        count++;
        return *this;  // Return reference to self
    }
    
    bool equals(const Counter& other) const {
        return this == &other;  // Compare addresses
    }
};
```

### Q10: Explain `inline` functions

Suggests compiler to replace function call with function body (reduces call overhead).

```cpp
inline int square(int x) {
    return x * x;
}

// Modern C++: constexpr is often better
constexpr int cube(int x) {
    return x * x * x;
}
```

---

## Quick Reference

```
┌─────────────────────────────────────────────────────────────────┐
│                 C/C++ FUNDAMENTALS CHEAT SHEET                   │
├─────────────────────────────────────────────────────────────────┤
│ POINTERS                                                         │
│   int* ptr = &x;     // Pointer declaration                     │
│   *ptr              // Dereference                               │
│   ptr++             // Move to next element                      │
│   nullptr           // Null pointer (C++11)                      │
├─────────────────────────────────────────────────────────────────┤
│ REFERENCES                                                       │
│   int& ref = x;     // Must initialize                          │
│   Cannot be null    // Cannot be reseated                        │
├─────────────────────────────────────────────────────────────────┤
│ SMART POINTERS                                                   │
│   unique_ptr        // Exclusive ownership                       │
│   shared_ptr        // Shared ownership (ref counted)            │
│   weak_ptr          // Non-owning observer                       │
├─────────────────────────────────────────────────────────────────┤
│ CONST                                                            │
│   const int* p      // Pointer to const                          │
│   int* const p      // Const pointer                             │
│   void f() const    // Const member function                     │
├─────────────────────────────────────────────────────────────────┤
│ MEMORY                                                           │
│   Stack             // Auto, fast, limited                       │
│   Heap              // Manual, large, slower                     │
│   new/delete        // C++ allocation                            │
│   malloc/free       // C allocation                              │
└─────────────────────────────────────────────────────────────────┘
```

---

*Good luck with your interview preparation!*

