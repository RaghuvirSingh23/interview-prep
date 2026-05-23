/*
 * SOLUTION: LRU Cache - Thread-Safe Implementation
 * 
 * Approach: Simple Mutex protecting entire cache
 * - std::lock_guard for RAII-style locking
 * - All operations are serialized
 * 
 * Time Complexity: O(1) for both get and put (excluding lock contention)
 * Space Complexity: O(capacity)
 */

#include <iostream>
#include <unordered_map>
#include <mutex>
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
    mutable std::mutex mtx;  // mutable allows locking in const methods
    
public:
    ThreadSafeLRUCache(int capacity) : capacity(capacity) {
        head = new Node(0, 0);
        tail = new Node(0, 0);
        head->next = tail;
        tail->prev = head;
    }
    
    ~ThreadSafeLRUCache() {
        std::lock_guard<std::mutex> lock(mtx);
        Node* curr = head;
        while (curr) {
            Node* next = curr->next;
            delete curr;
            curr = next;
        }
    }
    
    int get(int key) {
        std::lock_guard<std::mutex> lock(mtx);
        
        if (cache.find(key) == cache.end()) {
            return -1;
        }
        Node* node = cache[key];
        moveToFront(node);
        return node->value;
    }
    
    void put(int key, int value) {
        std::lock_guard<std::mutex> lock(mtx);
        
        if (cache.find(key) != cache.end()) {
            Node* node = cache[key];
            node->value = value;
            moveToFront(node);
        } else {
            if (cache.size() >= static_cast<size_t>(capacity)) {
                removeLRU();
            }
            Node* newNode = new Node(key, value);
            cache[key] = newNode;
            addToFront(newNode);
        }
    }
    
    size_t size() {
        std::lock_guard<std::mutex> lock(mtx);
        return cache.size();
    }
    
private:
    // These are called while holding the lock, so no additional locking needed
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

