/*
 * ============================================================================
 * Problem: LRU Cache using Doubly Linked List
 * ============================================================================
 * Difficulty: Medium
 * Source: LeetCode 146 / interview practice design round
 * Time: 30-40 minutes
 * 
 * DESCRIPTION:
 * ------------
 * This is the DSA version of the LRU Cache problem, focusing on the
 * doubly linked list implementation. This bridges to the LLD folder
 * where you'll find more advanced versions.
 * 
 * Design a data structure that follows the constraints of a Least Recently 
 * Used (LRU) cache.
 * 
 * Implement the LRUCache class:
 * - LRUCache(int capacity): Initialize with positive capacity
 * - int get(int key): Return value if exists, else -1
 * - void put(int key, int value): Update or insert. Evict LRU if at capacity.
 * 
 * Both operations must be O(1) average time complexity.
 * 
 * EXAMPLE:
 * --------
 *   LRUCache cache(2);
 *   cache.put(1, 1);
 *   cache.put(2, 2);
 *   cache.get(1);       // returns 1
 *   cache.put(3, 3);    // evicts key 2
 *   cache.get(2);       // returns -1
 *   cache.put(4, 4);    // evicts key 1
 *   cache.get(1);       // returns -1
 *   cache.get(3);       // returns 3
 *   cache.get(4);       // returns 4
 * 
 * KEY INSIGHT:
 * ------------
 * HashMap alone gives O(1) access but can't track usage order.
 * Linked list alone tracks order but gives O(n) access.
 * 
 * Solution: Combine both!
 * - HashMap: key -> pointer to DLL node (O(1) lookup)
 * - Doubly Linked List: maintains usage order (O(1) move/remove)
 * 
 * DATA STRUCTURE:
 * ---------------
 *   
 *   HashMap: { key1 -> Node*, key2 -> Node*, ... }
 *   
 *   DLL: [HEAD] <-> [MRU] <-> [Node] <-> ... <-> [LRU] <-> [TAIL]
 *         dummy                                            dummy
 * 
 * HINTS:
 * ------
 * 1. Use dummy head and tail nodes to simplify edge cases
 * 2. When accessing a key, move its node to the front (after dummy head)
 * 3. When evicting, remove the node before dummy tail
 * 4. Store the key in the node so you can remove from HashMap when evicting
 */

#include <iostream>
#include <unordered_map>
#include <cassert>

class LRUCache {
private:
    // TODO: Define Node struct
    // struct Node {
    //     int key;
    //     int value;
    //     Node* prev;
    //     Node* next;
    //     Node(int k, int v) : key(k), value(v), prev(nullptr), next(nullptr) {}
    // };
    
    int capacity;
    // TODO: Add HashMap and DLL pointers
    
public:
    LRUCache(int capacity) : capacity(capacity) {
        // TODO: Initialize dummy head and tail
        // head->next = tail;
        // tail->prev = head;
    }
    
    ~LRUCache() {
        // TODO: Clean up all nodes
    }
    
    int get(int key) {
        // TODO: Implement
        // 1. Check if key exists in map
        // 2. If not, return -1
        // 3. If yes, move node to front and return value
        return -1;
    }
    
    void put(int key, int value) {
        // TODO: Implement
        // 1. If key exists, update value and move to front
        // 2. If key doesn't exist:
        //    a. If at capacity, evict LRU (node before tail)
        //    b. Create new node, add to front, add to map
    }
    
private:
    // Helper: Remove node from its current position
    void removeNode(/* Node* node */) {
        // TODO: Implement
        // node->prev->next = node->next;
        // node->next->prev = node->prev;
    }
    
    // Helper: Add node right after head (most recently used)
    void addToFront(/* Node* node */) {
        // TODO: Implement
        // node->next = head->next;
        // node->prev = head;
        // head->next->prev = node;
        // head->next = node;
    }
    
    // Helper: Move existing node to front
    void moveToFront(/* Node* node */) {
        // TODO: Implement
        // removeNode(node);
        // addToFront(node);
    }
    
    // Helper: Evict least recently used (node before tail)
    void evictLRU() {
        // TODO: Implement
        // Node* lru = tail->prev;
        // removeNode(lru);
        // cache.erase(lru->key);
        // delete lru;
    }
};


// ============================================================================
// TEST CASES
// ============================================================================

void runTests() {
    std::cout << "Running LRU Cache (DLL) Tests...\n" << std::endl;
    
    // Test 1: Basic operations (from LeetCode)
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
        cache.put(1, 10);  // update, key 1 becomes MRU
        assert(cache.get(1) == 10);
        cache.put(3, 3);  // should evict key 2 (LRU)
        assert(cache.get(2) == -1);
        assert(cache.get(1) == 10);
        std::cout << "✓ Test 2 passed: Update existing key" << std::endl;
    }
    
    // Test 3: Capacity 1
    {
        LRUCache cache(1);
        cache.put(1, 1);
        assert(cache.get(1) == 1);
        cache.put(2, 2);
        assert(cache.get(1) == -1);
        assert(cache.get(2) == 2);
        std::cout << "✓ Test 3 passed: Capacity 1" << std::endl;
    }
    
    // Test 4: Get updates recency
    {
        LRUCache cache(2);
        cache.put(1, 1);
        cache.put(2, 2);
        cache.get(1);     // key 1 becomes MRU
        cache.put(3, 3);  // should evict key 2
        assert(cache.get(1) == 1);
        assert(cache.get(2) == -1);
        assert(cache.get(3) == 3);
        std::cout << "✓ Test 4 passed: Get updates recency" << std::endl;
    }
    
    // Test 5: Larger capacity
    {
        LRUCache cache(3);
        cache.put(1, 1);
        cache.put(2, 2);
        cache.put(3, 3);
        cache.get(1);     // Order: 1, 3, 2
        cache.get(2);     // Order: 2, 1, 3
        cache.put(4, 4);  // Evicts 3
        assert(cache.get(3) == -1);
        assert(cache.get(4) == 4);
        assert(cache.get(1) == 1);
        assert(cache.get(2) == 2);
        std::cout << "✓ Test 5 passed: Larger capacity" << std::endl;
    }
    
    std::cout << "\n=== All tests passed! ===" << std::endl;
}

int main() {
    runTests();
    return 0;
}



// ============================================================================
// SOLUTION: See solutions/05_lru_cache_dll_solution.cpp
// ============================================================================
