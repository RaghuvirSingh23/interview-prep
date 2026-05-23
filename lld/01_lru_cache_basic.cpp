/*
 * ============================================================================
 * Problem: LRU Cache - Basic Implementation
 * ============================================================================
 * Difficulty: Medium
 * Time: 45 minutes
 * 
 * DESCRIPTION:
 * ------------
 * Design and implement a data structure for Least Recently Used (LRU) cache.
 * It should support the following operations:
 * 
 *   - get(key)    : Get the value of the key if it exists, otherwise return -1
 *   - put(key, value) : Set or insert the value. When the cache reaches its 
 *                       capacity, it should invalidate the least recently 
 *                       used item before inserting a new item.
 * 
 * REQUIREMENTS:
 * -------------
 * - Both operations must run in O(1) average time complexity
 * - The cache is initialized with a positive capacity
 * 
 * EXAMPLE:
 * --------
 *   LRUCache cache(2);  // capacity = 2
 *   
 *   cache.put(1, 1);
 *   cache.put(2, 2);
 *   cache.get(1);       // returns 1
 *   cache.put(3, 3);    // evicts key 2
 *   cache.get(2);       // returns -1 (not found)
 *   cache.put(4, 4);    // evicts key 1
 *   cache.get(1);       // returns -1 (not found)
 *   cache.get(3);       // returns 3
 *   cache.get(4);       // returns 4
 * 
 * HINTS:
 * ------
 * 1. Think about what data structures give O(1) access by key
 * 2. Think about what data structure allows O(1) insertion/deletion at both ends
 * 3. How can you combine these two structures?
 * 4. A HashMap + Doubly Linked List is the classic approach
 * 5. The most recently used item goes to the front, least recently used at back
 * 
 * DATA STRUCTURE DESIGN:
 * ----------------------
 * 
 *   HashMap: key -> pointer to DLL node
 *   
 *   Doubly Linked List: [HEAD] <-> [Node1] <-> [Node2] <-> ... <-> [TAIL]
 *                        ^                                          ^
 *                        |                                          |
 *                   Most Recent                              Least Recent
 * 
 * API INTERFACE:
 * --------------
 */

#include <iostream>
#include <unordered_map>
#include <cassert>

class LRUCache {
private:
    // TODO: Define your internal data structures here
    // Hint: You'll need a Node struct for the doubly linked list
    
    int capacity;
    
public:
    // Constructor
    LRUCache(int capacity) {
        // TODO: Initialize your cache
    }
    
    // Get value by key, return -1 if not found
    int get(int key) {
        // TODO: Implement get operation
        // Remember: accessing a key makes it "recently used"
        return -1;
    }
    
    // Put key-value pair into cache
    void put(int key, int value) {
        // TODO: Implement put operation
        // Remember: if capacity exceeded, evict LRU item
    }
    
private:
    // Helper functions (optional but recommended)
    
    // Move a node to the front (most recently used)
    void moveToFront(/* Node* node */) {
        // TODO: Implement
    }
    
    // Remove a node from its current position
    void removeNode(/* Node* node */) {
        // TODO: Implement
    }
    
    // Add a node right after head (most recently used position)
    void addToFront(/* Node* node */) {
        // TODO: Implement
    }
    
    // Remove the least recently used node (right before tail)
    void removeLRU() {
        // TODO: Implement
    }
};


// ============================================================================
// TEST CASES - Do not modify
// ============================================================================

void runTests() {
    std::cout << "Running LRU Cache Tests...\n" << std::endl;
    
    // Test 1: Basic operations
    {
        LRUCache cache(2);
        cache.put(1, 1);
        cache.put(2, 2);
        assert(cache.get(1) == 1);
        cache.put(3, 3);  // evicts key 2
        assert(cache.get(2) == -1);
        cache.put(4, 4);  // evicts key 1
        assert(cache.get(1) == -1);
        assert(cache.get(3) == 3);
        assert(cache.get(4) == 4);
        std::cout << "✓ Test 1 passed: Basic operations" << std::endl;
    }
    
    // Test 2: Update existing key
    {
        LRUCache cache(2);
        cache.put(1, 1);
        cache.put(2, 2);
        cache.put(1, 10);  // update key 1
        assert(cache.get(1) == 10);
        cache.put(3, 3);  // should evict key 2, not key 1
        assert(cache.get(2) == -1);
        assert(cache.get(1) == 10);
        std::cout << "✓ Test 2 passed: Update existing key" << std::endl;
    }
    
    // Test 3: Capacity of 1
    {
        LRUCache cache(1);
        cache.put(1, 1);
        assert(cache.get(1) == 1);
        cache.put(2, 2);
        assert(cache.get(1) == -1);
        assert(cache.get(2) == 2);
        std::cout << "✓ Test 3 passed: Capacity of 1" << std::endl;
    }
    
    // Test 4: Get updates recency
    {
        LRUCache cache(2);
        cache.put(1, 1);
        cache.put(2, 2);
        cache.get(1);     // key 1 is now most recent
        cache.put(3, 3);  // should evict key 2
        assert(cache.get(1) == 1);
        assert(cache.get(2) == -1);
        assert(cache.get(3) == 3);
        std::cout << "✓ Test 4 passed: Get updates recency" << std::endl;
    }
    
    std::cout << "\n=== All tests passed! ===" << std::endl;
}

int main() {
    runTests();
    return 0;
}



// ============================================================================
// SOLUTION: See solutions/01_lru_cache_basic_solution.cpp
// ============================================================================
