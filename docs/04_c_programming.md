# C Programming - Interview Preparation Guide

## Table of Contents
1. [Memory Management](#memory-management)
2. [Pointers Deep Dive](#pointers-deep-dive)
3. [Arrays and Strings](#arrays-and-strings)
4. [Structures and Unions](#structures-and-unions)
5. [Preprocessor and Macros](#preprocessor-and-macros)
6. [Storage Classes](#storage-classes)
7. [Bit Manipulation](#bit-manipulation)
8. [File I/O](#file-io)
9. [Common Interview Questions](#common-interview-questions)

---

## Memory Management

### Memory Layout of a C Program

```
+------------------+  High Address
|      Stack       |  ← Local variables, function calls (grows downward)
|        ↓         |
|                  |
|        ↑         |
|       Heap       |  ← Dynamic allocation (grows upward)
+------------------+
|       BSS        |  ← Uninitialized global/static variables
+------------------+
|       Data       |  ← Initialized global/static variables
+------------------+
|       Text       |  ← Program code (read-only)
+------------------+  Low Address
```

### Stack vs Heap

| Feature | Stack | Heap |
|---------|-------|------|
| Allocation | Automatic | Manual (`malloc`, `calloc`) |
| Deallocation | Automatic (scope exit) | Manual (`free`) |
| Speed | Very fast | Slower |
| Size | Limited (typically 1-8 MB) | Large (limited by RAM) |
| Fragmentation | No | Yes |
| Access | LIFO | Random |

### Dynamic Memory Functions

```c
#include <stdlib.h>

// malloc - allocates uninitialized memory
int *arr = (int *)malloc(10 * sizeof(int));  // 10 integers

// calloc - allocates zero-initialized memory
int *arr = (int *)calloc(10, sizeof(int));   // 10 integers, all 0

// realloc - resize previously allocated memory
arr = (int *)realloc(arr, 20 * sizeof(int)); // Now 20 integers

// free - deallocate memory
free(arr);
arr = NULL;  // ALWAYS set to NULL after free!
```

### Common Memory Errors

```c
// 1. Memory Leak - forgetting to free
void leak() {
    int *p = malloc(100);
    return;  // Memory leaked!
}

// 2. Double Free
int *p = malloc(100);
free(p);
free(p);  // UNDEFINED BEHAVIOR!

// 3. Use After Free
int *p = malloc(sizeof(int));
*p = 42;
free(p);
printf("%d", *p);  // UNDEFINED BEHAVIOR!

// 4. Buffer Overflow
int arr[5];
arr[10] = 42;  // Writing beyond bounds!

// 5. Dangling Pointer
int *danglingPointer() {
    int x = 10;
    return &x;  // x is destroyed after function returns!
}
```

---

## Pointers Deep Dive

### Pointer Basics

```c
int x = 10;
int *p = &x;      // p stores address of x

printf("%d\n", x);    // 10 (value)
printf("%p\n", &x);   // 0x7fff... (address)
printf("%p\n", p);    // 0x7fff... (same address)
printf("%d\n", *p);   // 10 (dereferencing)
```

### Pointer Arithmetic

```c
int arr[] = {10, 20, 30, 40, 50};
int *p = arr;

printf("%d\n", *p);       // 10
printf("%d\n", *(p + 1)); // 20
printf("%d\n", *(p + 2)); // 30

p++;  // Moves by sizeof(int) bytes
printf("%d\n", *p);       // 20

// Difference between pointers
int *p1 = &arr[1];
int *p2 = &arr[4];
printf("%ld\n", p2 - p1); // 3 (number of elements, not bytes!)
```

### Pointer to Pointer

```c
int x = 10;
int *p = &x;
int **pp = &p;

printf("%d\n", x);    // 10
printf("%d\n", *p);   // 10
printf("%d\n", **pp); // 10

// Modifying through double pointer
**pp = 20;
printf("%d\n", x);    // 20
```

### Array of Pointers vs Pointer to Array

```c
// Array of pointers - array containing pointers
int *arr[5];  // 5 pointers to int
for (int i = 0; i < 5; i++) {
    arr[i] = malloc(sizeof(int));
    *arr[i] = i * 10;
}

// Pointer to array - pointer pointing to entire array
int nums[5] = {1, 2, 3, 4, 5};
int (*ptr)[5] = &nums;  // ptr points to entire array
printf("%d\n", (*ptr)[0]);  // 1
```

### Function Pointers

```c
// Declaration
int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }

int main() {
    // Function pointer declaration
    int (*operation)(int, int);
    
    operation = add;
    printf("%d\n", operation(5, 3));  // 8
    
    operation = sub;
    printf("%d\n", operation(5, 3));  // 2
    
    // Array of function pointers
    int (*ops[2])(int, int) = {add, sub};
    printf("%d\n", ops[0](10, 5));  // 15
    printf("%d\n", ops[1](10, 5));  // 5
    
    return 0;
}

// Callback pattern
void processArray(int *arr, int n, int (*callback)(int)) {
    for (int i = 0; i < n; i++) {
        arr[i] = callback(arr[i]);
    }
}

int square(int x) { return x * x; }

int main() {
    int arr[] = {1, 2, 3, 4, 5};
    processArray(arr, 5, square);
    // arr is now {1, 4, 9, 16, 25}
}
```

### const with Pointers

```c
int x = 10, y = 20;

// Pointer to const - can't modify value through pointer
const int *p1 = &x;
// *p1 = 20;  // ERROR!
p1 = &y;      // OK - can change what it points to

// Const pointer - can't change what it points to
int *const p2 = &x;
*p2 = 20;     // OK - can modify value
// p2 = &y;   // ERROR!

// Const pointer to const - can't do either
const int *const p3 = &x;
// *p3 = 20;  // ERROR!
// p3 = &y;   // ERROR!

// Read right to left:
// const int *p      → pointer to const int
// int *const p      → const pointer to int
// const int *const  → const pointer to const int
```

### void Pointers

```c
// Generic pointer - can point to any type
void *ptr;

int x = 10;
float f = 3.14;

ptr = &x;
printf("%d\n", *(int *)ptr);    // Must cast before dereferencing

ptr = &f;
printf("%f\n", *(float *)ptr);

// Common use: generic functions
void swap(void *a, void *b, size_t size) {
    char temp[size];
    memcpy(temp, a, size);
    memcpy(a, b, size);
    memcpy(b, temp, size);
}

int main() {
    int x = 10, y = 20;
    swap(&x, &y, sizeof(int));
    
    double a = 1.5, b = 2.5;
    swap(&a, &b, sizeof(double));
}
```

---

## Arrays and Strings

### Arrays vs Pointers

```c
int arr[5] = {1, 2, 3, 4, 5};
int *p = arr;

// Similarities
printf("%d\n", arr[2]);    // 3
printf("%d\n", p[2]);      // 3
printf("%d\n", *(arr + 2)); // 3
printf("%d\n", *(p + 2));   // 3

// Differences
printf("%zu\n", sizeof(arr)); // 20 (5 * 4 bytes)
printf("%zu\n", sizeof(p));   // 8 (pointer size on 64-bit)

// arr = p;  // ERROR! Array name is not modifiable lvalue
p = arr;     // OK
```

### 2D Arrays

```c
// Static 2D array
int matrix[3][4] = {
    {1, 2, 3, 4},
    {5, 6, 7, 8},
    {9, 10, 11, 12}
};

// Accessing elements
printf("%d\n", matrix[1][2]);        // 7
printf("%d\n", *(*(matrix + 1) + 2)); // 7

// Dynamic 2D array - Method 1: Array of pointers
int **arr = malloc(3 * sizeof(int *));
for (int i = 0; i < 3; i++) {
    arr[i] = malloc(4 * sizeof(int));
}

// Dynamic 2D array - Method 2: Contiguous memory
int *arr = malloc(3 * 4 * sizeof(int));
// Access: arr[i * cols + j]

// Dynamic 2D array - Method 3: Pointer to array (VLA style)
int (*arr)[4] = malloc(3 * sizeof(*arr));
// Access: arr[i][j]
```

### Strings in C

```c
// String literal (read-only, stored in text/rodata segment)
char *str1 = "Hello";
// str1[0] = 'h';  // UNDEFINED BEHAVIOR!

// Character array (modifiable)
char str2[] = "Hello";
str2[0] = 'h';  // OK

// String functions
#include <string.h>

strlen(str);           // Length (excluding '\0')
strcpy(dest, src);     // Copy (unsafe!)
strncpy(dest, src, n); // Copy with limit
strcat(dest, src);     // Concatenate
strncat(dest, src, n); // Concatenate with limit
strcmp(s1, s2);        // Compare (0 if equal)
strncmp(s1, s2, n);    // Compare n characters
strchr(str, c);        // Find character
strstr(str, substr);   // Find substring
strtok(str, delim);    // Tokenize
```

### Implement Common String Functions

```c
// strlen
size_t my_strlen(const char *s) {
    size_t len = 0;
    while (*s++) len++;
    return len;
}

// strcpy
char *my_strcpy(char *dest, const char *src) {
    char *ret = dest;
    while ((*dest++ = *src++));
    return ret;
}

// strcmp
int my_strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

// strrev (reverse string in place)
void my_strrev(char *str) {
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - 1 - i];
        str[len - 1 - i] = temp;
    }
}
```

---

## Structures and Unions

### Structure Basics

```c
// Definition
struct Point {
    int x;
    int y;
};

// Usage
struct Point p1 = {10, 20};
struct Point p2 = {.y = 30, .x = 40};  // Designated initializers

printf("%d, %d\n", p1.x, p1.y);

// Pointer to structure
struct Point *ptr = &p1;
printf("%d, %d\n", ptr->x, ptr->y);  // Arrow operator
printf("%d, %d\n", (*ptr).x, (*ptr).y);  // Equivalent
```

### Structure Padding and Alignment

```c
// Structures are padded for alignment
struct Example1 {
    char a;    // 1 byte + 3 padding
    int b;     // 4 bytes
    char c;    // 1 byte + 3 padding
};
// sizeof = 12 bytes (not 6!)

// Reorder to minimize padding
struct Example2 {
    int b;     // 4 bytes
    char a;    // 1 byte
    char c;    // 1 byte + 2 padding
};
// sizeof = 8 bytes

// Pack structure (no padding)
struct __attribute__((packed)) Example3 {
    char a;
    int b;
    char c;
};
// sizeof = 6 bytes (but may cause unaligned access!)
```

### Unions

```c
// Union - all members share same memory
union Data {
    int i;
    float f;
    char str[20];
};

union Data d;
printf("%zu\n", sizeof(d));  // 20 (size of largest member)

d.i = 10;
printf("%d\n", d.i);  // 10

d.f = 3.14;
printf("%f\n", d.f);  // 3.14
printf("%d\n", d.i);  // Garbage! (memory reinterpreted)

// Use case: Type punning / variant types
union Value {
    int as_int;
    float as_float;
    unsigned char bytes[4];
};

union Value v;
v.as_float = 3.14f;
// Can now inspect individual bytes
for (int i = 0; i < 4; i++) {
    printf("%02x ", v.bytes[i]);
}
```

### Bit Fields

```c
struct Flags {
    unsigned int is_active : 1;   // 1 bit
    unsigned int priority : 3;    // 3 bits (0-7)
    unsigned int type : 4;        // 4 bits (0-15)
};

struct Flags f = {1, 5, 10};
printf("%d\n", f.is_active);  // 1
printf("%d\n", f.priority);   // 5
printf("%d\n", f.type);       // 10
printf("%zu\n", sizeof(f));   // 4 bytes (compiler dependent)
```

### Self-Referential Structures

```c
// Linked List Node
struct Node {
    int data;
    struct Node *next;  // Pointer to same type
};

// Binary Tree Node
struct TreeNode {
    int data;
    struct TreeNode *left;
    struct TreeNode *right;
};

// Using typedef
typedef struct Node {
    int data;
    struct Node *next;
} Node;

Node *head = malloc(sizeof(Node));
head->data = 10;
head->next = NULL;
```

---

## Preprocessor and Macros

### Preprocessor Directives

```c
// Include
#include <stdio.h>   // System headers
#include "myheader.h" // Local headers

// Define constants
#define PI 3.14159
#define MAX_SIZE 100

// Function-like macros
#define SQUARE(x) ((x) * (x))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Why parentheses matter:
#define BAD_SQUARE(x) x * x
int result = BAD_SQUARE(2 + 3);  // 2 + 3 * 2 + 3 = 11, not 25!

// Stringification
#define STRINGIFY(x) #x
printf("%s\n", STRINGIFY(Hello World));  // "Hello World"

// Token pasting
#define CONCAT(a, b) a##b
int xy = 10;
printf("%d\n", CONCAT(x, y));  // Prints xy (10)

// Variadic macros
#define DEBUG_LOG(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
DEBUG_LOG("Value: %d", 42);
```

### Conditional Compilation

```c
// #ifdef / #ifndef
#define DEBUG

#ifdef DEBUG
    printf("Debug mode\n");
#endif

#ifndef RELEASE
    printf("Not release build\n");
#endif

// #if / #elif / #else
#define VERSION 2

#if VERSION == 1
    printf("Version 1\n");
#elif VERSION == 2
    printf("Version 2\n");
#else
    printf("Unknown version\n");
#endif

// Include guards
#ifndef MYHEADER_H
#define MYHEADER_H

// Header contents here

#endif

// Or use pragma once (non-standard but widely supported)
#pragma once
```

### Predefined Macros

```c
printf("File: %s\n", __FILE__);      // Current filename
printf("Line: %d\n", __LINE__);      // Current line number
printf("Function: %s\n", __func__);  // Current function name
printf("Date: %s\n", __DATE__);      // Compilation date
printf("Time: %s\n", __TIME__);      // Compilation time

// Useful for debugging
#define ASSERT(expr) \
    if (!(expr)) { \
        fprintf(stderr, "Assertion failed: %s at %s:%d\n", \
                #expr, __FILE__, __LINE__); \
        abort(); \
    }
```

---

## Storage Classes

### auto, register, static, extern

```c
// auto - default for local variables (rarely used explicitly)
auto int x = 10;  // Same as: int x = 10;

// register - hint to store in CPU register (compiler may ignore)
register int counter = 0;
// &counter;  // ERROR! Can't take address of register variable

// static - preserves value between function calls
void counter() {
    static int count = 0;  // Initialized only once
    count++;
    printf("%d\n", count);
}
counter();  // 1
counter();  // 2
counter();  // 3

// static at file scope - limits visibility to current file
static int privateVar = 10;  // Not visible in other files
static void privateFunc() {} // Not visible in other files

// extern - declaration (definition is elsewhere)
extern int globalVar;  // Declared here, defined in another file

// In file1.c
int globalVar = 100;  // Definition

// In file2.c
extern int globalVar;  // Declaration
printf("%d\n", globalVar);  // Uses globalVar from file1.c
```

### Scope and Lifetime

```c
int global = 1;           // Global scope, program lifetime

void func() {
    static int s = 2;     // Function scope, program lifetime
    int local = 3;        // Function scope, function lifetime
    
    {
        int block = 4;    // Block scope, block lifetime
    }
    // block not accessible here
}

// Variable shadowing
int x = 10;
void func() {
    int x = 20;  // Shadows global x
    {
        int x = 30;  // Shadows local x
        printf("%d\n", x);  // 30
    }
    printf("%d\n", x);  // 20
}
```

---

## Bit Manipulation

### Bitwise Operators

```c
// AND (&) - both bits must be 1
5 & 3   // 0101 & 0011 = 0001 = 1

// OR (|) - at least one bit must be 1
5 | 3   // 0101 | 0011 = 0111 = 7

// XOR (^) - bits must be different
5 ^ 3   // 0101 ^ 0011 = 0110 = 6

// NOT (~) - flip all bits
~5      // ~0101 = 1010 (in 32-bit: 0xFFFFFFFA = -6)

// Left shift (<<) - multiply by 2^n
5 << 1  // 0101 << 1 = 1010 = 10
5 << 2  // 0101 << 2 = 10100 = 20

// Right shift (>>) - divide by 2^n
20 >> 1 // 10100 >> 1 = 1010 = 10
20 >> 2 // 10100 >> 2 = 101 = 5
```

### Common Bit Operations

```c
// Set bit at position n
#define SET_BIT(x, n)    ((x) | (1 << (n)))

// Clear bit at position n
#define CLEAR_BIT(x, n)  ((x) & ~(1 << (n)))

// Toggle bit at position n
#define TOGGLE_BIT(x, n) ((x) ^ (1 << (n)))

// Check if bit at position n is set
#define CHECK_BIT(x, n)  (((x) >> (n)) & 1)

// Examples
int x = 5;  // 0101
SET_BIT(x, 1);    // 0111 = 7
CLEAR_BIT(x, 2);  // 0001 = 1
TOGGLE_BIT(x, 0); // 0100 = 4
CHECK_BIT(x, 2);  // 1 (bit is set)
```

### Bit Manipulation Tricks

```c
// Check if number is power of 2
bool isPowerOf2(int n) {
    return n > 0 && (n & (n - 1)) == 0;
}

// Count set bits (Brian Kernighan's algorithm)
int countSetBits(int n) {
    int count = 0;
    while (n) {
        n &= (n - 1);  // Clears lowest set bit
        count++;
    }
    return count;
}

// Swap without temp variable
void swap(int *a, int *b) {
    *a ^= *b;
    *b ^= *a;
    *a ^= *b;
}

// Check if opposite signs
bool oppositeSign(int a, int b) {
    return (a ^ b) < 0;
}

// Get lowest set bit
int lowestSetBit(int n) {
    return n & (-n);
}

// Turn off rightmost set bit
int turnOffRightmost(int n) {
    return n & (n - 1);
}
```

---

## File I/O

### File Operations

```c
#include <stdio.h>

// Opening files
FILE *fp = fopen("file.txt", "r");   // Read
FILE *fp = fopen("file.txt", "w");   // Write (truncate)
FILE *fp = fopen("file.txt", "a");   // Append
FILE *fp = fopen("file.txt", "r+");  // Read + Write
FILE *fp = fopen("file.txt", "rb");  // Binary read

if (fp == NULL) {
    perror("Error opening file");
    return 1;
}

// Always close files
fclose(fp);
```

### Reading Files

```c
// Character by character
int ch;
while ((ch = fgetc(fp)) != EOF) {
    putchar(ch);
}

// Line by line
char line[256];
while (fgets(line, sizeof(line), fp) != NULL) {
    printf("%s", line);
}

// Formatted input
int num;
char str[100];
fscanf(fp, "%d %s", &num, str);

// Binary read
int arr[10];
fread(arr, sizeof(int), 10, fp);  // Read 10 integers
```

### Writing Files

```c
// Character by character
fputc('A', fp);

// String
fputs("Hello, World!\n", fp);

// Formatted output
fprintf(fp, "Name: %s, Age: %d\n", name, age);

// Binary write
int arr[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
fwrite(arr, sizeof(int), 10, fp);  // Write 10 integers
```

### File Positioning

```c
// Get current position
long pos = ftell(fp);

// Seek to position
fseek(fp, 0, SEEK_SET);   // Beginning
fseek(fp, 0, SEEK_END);   // End
fseek(fp, -10, SEEK_CUR); // 10 bytes back from current

// Rewind to beginning
rewind(fp);  // Equivalent to fseek(fp, 0, SEEK_SET)

// Get file size
fseek(fp, 0, SEEK_END);
long size = ftell(fp);
rewind(fp);
```

---

## Common Interview Questions

### Q1: What is the difference between `malloc` and `calloc`?

| malloc | calloc |
|--------|--------|
| `malloc(size)` | `calloc(n, size)` |
| Allocates uninitialized memory | Allocates zero-initialized memory |
| Single argument | Two arguments |
| Faster (no initialization) | Slower (zeroes memory) |

### Q2: What is a memory leak? How to detect it?

**Memory leak**: Allocated memory that is never freed and no longer accessible.

```c
void leak() {
    int *p = malloc(100);
    // No free(p) - memory leaked!
}
```

**Detection tools**: Valgrind, AddressSanitizer, Dr. Memory

### Q3: Explain `const char *`, `char const *`, `char * const`

```c
const char *p;      // Pointer to const char (can't modify string)
char const *p;      // Same as above
char * const p;     // Const pointer to char (can't change pointer)
const char * const; // Const pointer to const char (can't change either)
```

### Q4: What is the output?

```c
int arr[] = {1, 2, 3, 4, 5};
int *p = arr;
printf("%d\n", *p++);   // 1 (prints *p, then increments p)
printf("%d\n", *++p);   // 3 (increments p, then prints *p)
printf("%d\n", ++*p);   // 4 (increments *p, then prints it)
```

### Q5: Implement `memcpy` and `memmove`

```c
// memcpy - doesn't handle overlapping regions
void *my_memcpy(void *dest, const void *src, size_t n) {
    char *d = dest;
    const char *s = src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

// memmove - handles overlapping regions
void *my_memmove(void *dest, const void *src, size_t n) {
    char *d = dest;
    const char *s = src;
    
    if (d < s) {
        // Copy forward
        while (n--) *d++ = *s++;
    } else {
        // Copy backward
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }
    return dest;
}
```

### Q6: What is undefined behavior?

Actions that the C standard doesn't define, leading to unpredictable results:
- Dereferencing NULL pointer
- Array out of bounds access
- Use after free
- Signed integer overflow
- Modifying string literals
- Uninitialized variable access

### Q7: Difference between `struct` and `union`?

| struct | union |
|--------|-------|
| Each member has own memory | All members share memory |
| Size = sum of all members + padding | Size = largest member |
| All members accessible simultaneously | Only one member valid at a time |

### Q8: What does `volatile` keyword do?

Tells compiler not to optimize accesses to this variable - always read from memory.

```c
volatile int *hardware_register = (int *)0x1234;

// Without volatile, compiler might optimize away repeated reads
while (*hardware_register == 0) {
    // Wait for hardware
}
```

Use cases: Hardware registers, signal handlers, multi-threaded variables

### Q9: Explain `typedef` vs `#define`

```c
#define PTR1 int *
typedef int * PTR2;

PTR1 a, b;  // int *a, b;  (only a is pointer!)
PTR2 c, d;  // int *c, *d; (both are pointers)
```

`typedef` is processed by compiler, `#define` is text substitution by preprocessor.

### Q10: What is a segmentation fault?

Memory access violation - accessing memory you don't have permission to access:
- Dereferencing NULL
- Accessing freed memory
- Stack overflow
- Writing to read-only memory
- Buffer overflow

---

## Quick Reference

### Format Specifiers

| Specifier | Type |
|-----------|------|
| `%d`, `%i` | int |
| `%u` | unsigned int |
| `%ld` | long |
| `%lu` | unsigned long |
| `%lld` | long long |
| `%f` | float/double |
| `%e` | Scientific notation |
| `%c` | char |
| `%s` | string |
| `%p` | pointer |
| `%x`, `%X` | hex |
| `%o` | octal |
| `%zu` | size_t |

### Size of Types (64-bit system)

| Type | Size (bytes) |
|------|--------------|
| char | 1 |
| short | 2 |
| int | 4 |
| long | 8 |
| long long | 8 |
| float | 4 |
| double | 8 |
| pointer | 8 |
