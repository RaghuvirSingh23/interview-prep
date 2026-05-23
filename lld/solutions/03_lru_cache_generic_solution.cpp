/*
 * SOLUTION: LRU Cache - Generic/Templated Implementation
 * 
 * Approach: std::list + std::unordered_map
 * - std::list provides O(1) splice operation
 * - Store iterators in map for O(1) access to list position
 * 
 * Modern C++ Features:
 * - Templates for any key/value types
 * - std::optional for nullable returns
 * - Move semantics for efficiency
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
    using ListType = std::list<std::pair<K, V>>;
    using ListIterator = typename ListType::iterator;
    using MapType = std::unordered_map<K, ListIterator, Hash>;
    
private:
    size_t capacity_;
    ListType lruList_;  // Front = MRU, Back = LRU
    MapType cache_;
    
public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {
        if (capacity == 0) {
            throw std::invalid_argument("Capacity must be positive");
        }
    }
    
    std::optional<V> get(const K& key) {
        auto it = cache_.find(key);
        if (it == cache_.end()) {
            return std::nullopt;
        }
        
        // Move to front (most recently used)
        lruList_.splice(lruList_.begin(), lruList_, it->second);
        return it->second->second;
    }
    
    void put(const K& key, const V& value) {
        putImpl(key, value);
    }
    
    void put(const K& key, V&& value) {
        putImpl(key, std::move(value));
    }
    
    bool contains(const K& key) const {
        return cache_.find(key) != cache_.end();
    }
    
    bool erase(const K& key) {
        auto it = cache_.find(key);
        if (it == cache_.end()) {
            return false;
        }
        lruList_.erase(it->second);
        cache_.erase(it);
        return true;
    }
    
    size_t size() const {
        return cache_.size();
    }
    
    size_t capacity() const {
        return capacity_;
    }
    
    bool empty() const {
        return cache_.empty();
    }
    
    void clear() {
        lruList_.clear();
        cache_.clear();
    }
    
    void resize(size_t newCapacity) {
        if (newCapacity == 0) {
            throw std::invalid_argument("Capacity must be positive");
        }
        
        while (cache_.size() > newCapacity) {
            evictLRU();
        }
        capacity_ = newCapacity;
    }
    
    void print() const {
        std::cout << "Cache contents (MRU -> LRU):" << std::endl;
        for (const auto& [key, value] : lruList_) {
            std::cout << "  " << key << " -> " << value << std::endl;
        }
    }
    
private:
    template<typename ValueArg>
    void putImpl(const K& key, ValueArg&& value) {
        auto it = cache_.find(key);
        
        if (it != cache_.end()) {
            // Update existing
            it->second->second = std::forward<ValueArg>(value);
            lruList_.splice(lruList_.begin(), lruList_, it->second);
        } else {
            // Insert new
            if (cache_.size() >= capacity_) {
                evictLRU();
            }
            lruList_.emplace_front(key, std::forward<ValueArg>(value));
            cache_[key] = lruList_.begin();
        }
    }
    
    void evictLRU() {
        if (lruList_.empty()) return;
        
        auto lru = lruList_.back();
        cache_.erase(lru.first);
        lruList_.pop_back();
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

