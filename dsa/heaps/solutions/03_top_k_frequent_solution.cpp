/*
 * SOLUTION: Top K Frequent Elements
 * 
 * Two Approaches:
 * 1. Min-Heap - O(n log k) time, O(n) space
 * 2. Bucket Sort - O(n) time, O(n) space
 * 
 * Bucket Sort is optimal when frequency range is bounded by n
 */

#include <iostream>
#include <vector>
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <cassert>

// Approach 1: Min-Heap - O(n log k) time, O(n) space
std::vector<int> topKFrequentHeap(std::vector<int>& nums, int k) {
    // Step 1: Count frequencies
    std::unordered_map<int, int> freq;
    for (int num : nums) {
        freq[num]++;
    }
    
    // Step 2: Use min-heap of size k
    // Pair: {frequency, element}
    auto compare = [](std::pair<int,int>& a, std::pair<int,int>& b) {
        return a.first > b.first;  // Min-heap by frequency
    };
    
    std::priority_queue<std::pair<int,int>, 
                        std::vector<std::pair<int,int>>, 
                        decltype(compare)> minHeap(compare);
    
    for (auto& [num, count] : freq) {
        minHeap.push({count, num});
        if (minHeap.size() > static_cast<size_t>(k)) {
            minHeap.pop();  // Remove least frequent
        }
    }
    
    // Step 3: Extract results
    std::vector<int> result;
    while (!minHeap.empty()) {
        result.push_back(minHeap.top().second);
        minHeap.pop();
    }
    
    return result;
}

// Approach 2: Bucket Sort - O(n) time, O(n) space
std::vector<int> topKFrequentBucket(std::vector<int>& nums, int k) {
    // Step 1: Count frequencies
    std::unordered_map<int, int> freq;
    for (int num : nums) {
        freq[num]++;
    }
    
    // Step 2: Create buckets where index = frequency
    // Max frequency can be nums.size()
    std::vector<std::vector<int>> buckets(nums.size() + 1);
    
    for (auto& [num, count] : freq) {
        buckets[count].push_back(num);
    }
    
    // Step 3: Collect top k from highest frequency buckets
    std::vector<int> result;
    
    for (int i = buckets.size() - 1; i >= 0 && result.size() < static_cast<size_t>(k); i--) {
        for (int num : buckets[i]) {
            result.push_back(num);
            if (result.size() == static_cast<size_t>(k)) break;
        }
    }
    
    return result;
}


// ============================================================================
// TEST CASES
// ============================================================================

// Helper to check if result contains expected elements (order doesn't matter)
bool containsSameElements(std::vector<int> a, std::vector<int> b) {
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());
    return a == b;
}

void runTests() {
    std::cout << "Running Top K Frequent Tests...\n" << std::endl;
    
    // Test 1: Normal case
    {
        std::vector<int> nums = {1, 1, 1, 2, 2, 3};
        auto result = topKFrequentHeap(nums, 2);
        assert(containsSameElements(result, {1, 2}));
        std::cout << "✓ Test 1 passed: k=2 in [1,1,1,2,2,3]" << std::endl;
    }
    
    // Test 2: Single element
    {
        std::vector<int> nums = {1};
        auto result = topKFrequentHeap(nums, 1);
        assert(containsSameElements(result, {1}));
        std::cout << "✓ Test 2 passed: Single element" << std::endl;
    }
    
    // Test 3: All same frequency
    {
        std::vector<int> nums = {1, 2, 3, 4};
        auto result = topKFrequentHeap(nums, 2);
        assert(result.size() == 2);
        std::cout << "✓ Test 3 passed: All same frequency" << std::endl;
    }
    
    // Test 4: k equals unique elements
    {
        std::vector<int> nums = {1, 1, 2, 2, 3, 3};
        auto result = topKFrequentHeap(nums, 3);
        assert(containsSameElements(result, {1, 2, 3}));
        std::cout << "✓ Test 4 passed: k = unique count" << std::endl;
    }
    
    // Test 5: Negative numbers
    {
        std::vector<int> nums = {-1, -1, -2, -2, -2, -3};
        auto result = topKFrequentHeap(nums, 2);
        assert(containsSameElements(result, {-2, -1}));
        std::cout << "✓ Test 5 passed: Negative numbers" << std::endl;
    }
    
    // Test Bucket Sort
    std::cout << "\n--- Testing Bucket Sort ---" << std::endl;
    
    // Test 6: Bucket sort normal case
    {
        std::vector<int> nums = {1, 1, 1, 2, 2, 3};
        auto result = topKFrequentBucket(nums, 2);
        assert(containsSameElements(result, {1, 2}));
        std::cout << "✓ Test 6 passed: Bucket sort k=2" << std::endl;
    }
    
    // Test 7: Bucket sort with varied frequencies
    {
        std::vector<int> nums = {4, 4, 4, 4, 3, 3, 3, 2, 2, 1};
        auto result = topKFrequentBucket(nums, 3);
        assert(containsSameElements(result, {4, 3, 2}));
        std::cout << "✓ Test 7 passed: Bucket sort varied frequencies" << std::endl;
    }
    
    std::cout << "\n=== All tests passed! ===" << std::endl;
}

int main() {
    runTests();
    return 0;
}

