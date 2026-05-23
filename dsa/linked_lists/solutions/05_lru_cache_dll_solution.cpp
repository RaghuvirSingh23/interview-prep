/*
 * SOLUTION: LRU Cache using Doubly Linked List
 * 
 * Approach: HashMap + Doubly Linked List
 * - HashMap: O(1) lookup by key
 * - DLL: O(1) insertion/deletion, maintains usage order
 * - Dummy head/tail simplify edge cases
 * 
 * Time Complexity: O(1) for both get and put
 * Space Complexity: O(capacity)
 */

#include <iostream>
#include <unordered_map>
#include <cassert>

class LRUCache {
private:
    struct Node {
        int key;
        int value;
        Node* prev;
        Node* next;
        Node(int k, int v) : key(k), value(v), prev(nullptr), next(nullptr) {}
    };
    
    int capacity;
    std::unordered_map<int, Node*> cache;
    Node* head;  // Dummy head (MRU side)
    Node* tail;  // Dummy tail (LRU side)
    
public:
    LRUCache(int capacity) : capacity(capacity) {
        head = new Node(0, 0);
        tail = new Node(0, 0);
        head->next = tail;
        tail->prev = head;
    }
    
    ~LRUCache() {
        Node* curr = head;
        while (curr) {
            Node* next = curr->next;
            delete curr;
            curr = next;
        }
    }
    
    int get(int key) {
        auto it = cache.find(key);
        if (it == cache.end()) {
            return -1;
        }
        
        Node* node = it->second;
        moveToFront(node);
        return node->value;
    }
    
    void put(int key, int value) {
        auto it = cache.find(key);
        
        if (it != cache.end()) {
            // Update existing
            Node* node = it->second;
            node->value = value;
            moveToFront(node);
        } else {
            // Insert new
            if (cache.size() >= static_cast<size_t>(capacity)) {
                evictLRU();
            }
            
            Node* newNode = new Node(key, value);
            cache[key] = newNode;
            addToFront(newNode);
        }
    }
    
private:
    void removeNode(Node* node) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
    
    void addToFront(Node* node) {
        node->next = head->next;
        node->prev = head;
        head->next->prev = node;
        head->next = node;
    }
    
    void moveToFront(Node* node) {
        removeNode(node);
        addToFront(node);
    }
    
    void evictLRU() {
        Node* lru = tail->prev;
        removeNode(lru);
        cache.erase(lru->key);
        delete lru;
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

