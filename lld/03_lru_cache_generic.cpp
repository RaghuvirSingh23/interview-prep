/*
 * ============================================================================
 * Problem: LRU Cache - Generic/Templated Implementation
 * ============================================================================
 * Difficulty: Medium-Hard
 * Time: 60 minutes
 * 
 * DESCRIPTION:
 * ------------
 * Create a production-quality, generic LRU Cache that can work with any
 * key-value types. This version should demonstrate modern C++ best practices.
 * 
 * REQUIREMENTS:
 * -------------
 * - Templated to work with any key (K) and value (V) types
 * - Use smart pointers for memory management (no raw new/delete)
 * - Support move semantics for efficient value insertion
 * - Provide iterator support for debugging/inspection
 * - Include proper RAII (destructor should clean up automatically)
 * - Consider exception safety
 * 
 * EXAMPLE:
 * --------
 *   LRUCache<std::string, User> userCache(100);
 *   userCache.put("user_123", User{"Alice", 25});
 *   
 *   LRUCache<int, std::vector<int>> dataCache(50);
 *   dataCache.put(1, {1, 2, 3, 4, 5});
 *   
 *   auto result = dataCache.get(1);
 *   if (result) {
 *       // Use *result
 *   }
 * 
 * HINTS:
 * ------
 * 1. Use std::list instead of manual doubly-linked list (provides iterators)
 * 2. Store iterator in the map for O(1) access to list position
 * 3. Return std::optional<V> from get() instead of special value
 * 4. Use std::unordered_map with custom hash for flexibility
 * 5. Consider using std::unique_ptr for node management
 * 
 * MODERN C++ FEATURES TO USE:
 * ---------------------------
 * - Templates
 * - std::optional (C++17)
 * - std::list with splice()
 * - Move semantics (std::move)
 * - Smart pointers
 * - Range-based for loops
 * - auto type deduction
 * 
 * API INTERFACE:
 * --------------
 */

#include <iostream>
#include <unordered_map>
#include <list>
#include <optional>
#include <memory>
#include <functional>
#include <cassert>
#include <string>

template<typename K, typename V, typename Hash = std::hash<K>>
class LRUCache {
public:
    using KeyType = K;
    using ValueType = V;
    
private:
    // TODO: Define your internal data structures
    // Hint: Use std::list<std::pair<K, V>> for the LRU list
    // Hint: Use std::unordered_map<K, iterator, Hash> for O(1) lookup
    
    size_t capacity_;
    
public:
    // Constructor
    explicit LRUCache(size_t capacity) : capacity_(capacity) {
        if (capacity == 0) {
            throw std::invalid_argument("Capacity must be positive");
        }
        // TODO: Initialize data structures
    }
    
    // Get value by key, return std::optional
    std::optional<V> get(const K& key) {
        // TODO: Implement
        // Return std::nullopt if not found
        // Return the value wrapped in optional if found
        return std::nullopt;
    }
    
    // Put with copy semantics
    void put(const K& key, const V& value) {
        // TODO: Implement
    }
    
    // Put with move semantics (for efficiency)
    void put(const K& key, V&& value) {
        // TODO: Implement
    }
    
    // Check if key exists (without affecting LRU order)
    bool contains(const K& key) const {
        // TODO: Implement
        return false;
    }
    
    // Remove a specific key
    bool erase(const K& key) {
        // TODO: Implement
        return false;
    }
    
    // Get current size
    size_t size() const {
        // TODO: Implement
        return 0;
    }
    
    // Get capacity
    size_t capacity() const {
        return capacity_;
    }
    
    // Check if empty
    bool empty() const {
        return size() == 0;
    }
    
    // Clear all entries
    void clear() {
        // TODO: Implement
    }
    
    // Resize capacity (may evict items if new capacity is smaller)
    void resize(size_t newCapacity) {
        // TODO: Implement
    }
    
    // Debug: Print cache contents (most recent first)
    void print() const {
        // TODO: Implement
        std::cout << "Cache contents (MRU -> LRU):" << std::endl;
    }
};


// ============================================================================
// TEST CASES
// ============================================================================

void runTests() {
    std::cout << "Running Generic LRU Cache Tests...\n" << std::endl;
    
    // Test 1: Basic int-int cache
    {
        LRUCache<int, int> cache(2);
        cache.put(1, 100);
        cache.put(2, 200);
        
        auto val1 = cache.get(1);
        assert(val1.has_value() && val1.value() == 100);
        
        cache.put(3, 300);  // Evicts key 2
        
        assert(!cache.get(2).has_value());
        assert(cache.get(3).value() == 300);
        
        std::cout << "✓ Test 1 passed: Basic int-int cache" << std::endl;
    }
    
    // Test 2: String keys
    {
        LRUCache<std::string, int> cache(3);
        cache.put("one", 1);
        cache.put("two", 2);
        cache.put("three", 3);
        
        assert(cache.get("one").value() == 1);
        assert(cache.get("two").value() == 2);
        assert(cache.contains("three"));
        
        std::cout << "✓ Test 2 passed: String keys" << std::endl;
    }
    
    // Test 3: Complex value types
    {
        LRUCache<int, std::vector<int>> cache(2);
        cache.put(1, {1, 2, 3});
        cache.put(2, {4, 5, 6});
        
        auto vec = cache.get(1);
        assert(vec.has_value());
        assert(vec.value().size() == 3);
        assert(vec.value()[0] == 1);
        
        std::cout << "✓ Test 3 passed: Complex value types" << std::endl;
    }
    
    // Test 4: Move semantics
    {
        LRUCache<int, std::string> cache(2);
        std::string longStr = "This is a very long string that should be moved";
        cache.put(1, std::move(longStr));
        
        assert(cache.get(1).value() == "This is a very long string that should be moved");
        
        std::cout << "✓ Test 4 passed: Move semantics" << std::endl;
    }
    
    // Test 5: Erase and clear
    {
        LRUCache<int, int> cache(3);
        cache.put(1, 100);
        cache.put(2, 200);
        cache.put(3, 300);
        
        assert(cache.size() == 3);
        
        cache.erase(2);
        assert(cache.size() == 2);
        assert(!cache.get(2).has_value());
        
        cache.clear();
        assert(cache.empty());
        
        std::cout << "✓ Test 5 passed: Erase and clear" << std::endl;
    }
    
    // Test 6: Resize
    {
        LRUCache<int, int> cache(5);
        for (int i = 1; i <= 5; i++) {
            cache.put(i, i * 10);
        }
        assert(cache.size() == 5);
        
        cache.resize(3);  // Should evict 2 LRU items
        assert(cache.size() == 3);
        assert(cache.capacity() == 3);
        
        std::cout << "✓ Test 6 passed: Resize" << std::endl;
    }
    
    std::cout << "\n=== All tests passed! ===" << std::endl;
}

int main() {
    runTests();
    return 0;
}


// ============================================================================
// SOLUTION: See solutions/03_lru_cache_generic_solution.cpp
// ============================================================================
