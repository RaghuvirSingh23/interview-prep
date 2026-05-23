/*
 * ============================================================================
 * Problem: LRU Cache - Thread-Safe Implementation
 * ============================================================================
 * Difficulty: Medium-Hard
 * Time: 60 minutes
 * 
 * DESCRIPTION:
 * ------------
 * Extend the basic LRU Cache to be thread-safe. Multiple threads should be
 * able to call get() and put() concurrently without data races or corruption.
 * 
 * REQUIREMENTS:
 * -------------
 * - All operations must be thread-safe
 * - Operations should still be O(1) on average
 * - Minimize lock contention where possible
 * - Handle concurrent reads efficiently
 * 
 * EXAMPLE:
 * --------
 *   ThreadSafeLRUCache cache(100);
 *   
 *   // Thread 1
 *   cache.put(1, 100);
 *   
 *   // Thread 2 (concurrent)
 *   cache.put(2, 200);
 *   
 *   // Thread 3 (concurrent)
 *   int val = cache.get(1);  // Should return 100
 * 
 * HINTS:
 * ------
 * 1. Start with a simple mutex that locks the entire cache
 * 2. Consider using std::mutex with std::lock_guard or std::unique_lock
 * 3. For better performance, consider std::shared_mutex (C++17) for read-write locks
 * 4. Think about what operations are "read" vs "write"
 *    - get() seems like a read, but it modifies the LRU order!
 * 5. Advanced: Consider lock striping or lock-free data structures
 * 
 * SYNCHRONIZATION OPTIONS:
 * ------------------------
 * 
 * Option 1: Simple Mutex (easiest)
 *   - One mutex protects entire cache
 *   - Simple but may have contention
 * 
 * Option 2: Read-Write Lock (better for read-heavy)
 *   - Multiple readers OR one writer
 *   - But get() modifies order, so it needs write lock too!
 * 
 * Option 3: Fine-grained locking (advanced)
 *   - Separate locks for different parts
 *   - Complex but better scalability
 * 
 * API INTERFACE:
 * --------------
 */

#include <iostream>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>  // C++17
#include <thread>
#include <vector>
#include <cassert>
#include <atomic>

class ThreadSafeLRUCache {
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
    Node* head;
    Node* tail;
    
    // TODO: Add synchronization primitives
    // Option 1: Simple mutex
    // std::mutex mtx;
    
    // Option 2: Read-write lock (C++17)
    // std::shared_mutex rwMutex;
    
public:
    ThreadSafeLRUCache(int capacity) : capacity(capacity) {
        head = new Node(0, 0);
        tail = new Node(0, 0);
        head->next = tail;
        tail->prev = head;
    }
    
    ~ThreadSafeLRUCache() {
        // TODO: Clean up (consider thread safety during destruction)
        Node* curr = head;
        while (curr) {
            Node* next = curr->next;
            delete curr;
            curr = next;
        }
    }
    
    int get(int key) {
        // TODO: Add thread-safe implementation
        // Remember: get() modifies the LRU order, so it's not a pure read!
        
        return -1;
    }
    
    void put(int key, int value) {
        // TODO: Add thread-safe implementation
        
    }
    
    // Optional: Get current size (useful for testing)
    size_t size() {
        // TODO: Make thread-safe
        return cache.size();
    }
    
private:
    void moveToFront(Node* node) {
        removeNode(node);
        addToFront(node);
    }
    
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
    
    void removeLRU() {
        Node* lru = tail->prev;
        removeNode(lru);
        cache.erase(lru->key);
        delete lru;
    }
};


// ============================================================================
// TEST CASES
// ============================================================================

void runBasicTests() {
    std::cout << "Running Basic Thread-Safe Tests...\n" << std::endl;
    
    ThreadSafeLRUCache cache(2);
    cache.put(1, 1);
    cache.put(2, 2);
    assert(cache.get(1) == 1);
    cache.put(3, 3);
    assert(cache.get(2) == -1);
    
    std::cout << "✓ Basic functionality works" << std::endl;
}

void runConcurrencyTests() {
    std::cout << "\nRunning Concurrency Tests...\n" << std::endl;
    
    ThreadSafeLRUCache cache(1000);
    std::atomic<int> successCount{0};
    std::atomic<int> errorCount{0};
    
    // Launch multiple writer threads
    std::vector<std::thread> writers;
    for (int t = 0; t < 4; t++) {
        writers.emplace_back([&cache, t, &successCount]() {
            for (int i = 0; i < 250; i++) {
                int key = t * 250 + i;
                cache.put(key, key * 10);
                successCount++;
            }
        });
    }
    
    // Launch multiple reader threads
    std::vector<std::thread> readers;
    for (int t = 0; t < 4; t++) {
        readers.emplace_back([&cache, &errorCount]() {
            for (int i = 0; i < 500; i++) {
                int key = rand() % 1000;
                int val = cache.get(key);
                // Value should be key*10 or -1 (not found)
                if (val != -1 && val != key * 10) {
                    errorCount++;
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : writers) t.join();
    for (auto& t : readers) t.join();
    
    std::cout << "✓ Completed " << successCount << " writes" << std::endl;
    std::cout << "✓ No data corruption detected (errors: " << errorCount << ")" << std::endl;
    
    assert(errorCount == 0);
    std::cout << "\n=== Concurrency tests passed! ===" << std::endl;
}

int main() {
    runBasicTests();
    runConcurrencyTests();
    return 0;
}


// ============================================================================
// SOLUTION: See solutions/02_lru_cache_threadsafe_solution.cpp
// ============================================================================
