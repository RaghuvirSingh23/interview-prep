/*
 * ============================================================================
 * Problem: Top K Frequent Elements
 * ============================================================================
 * Difficulty: Medium
 * Source: LeetCode 347
 * Time: 25-30 minutes
 * 
 * DESCRIPTION:
 * ------------
 * Given an integer array nums and an integer k, return the k most frequent 
 * elements. You may return the answer in any order.
 * 
 * EXAMPLE 1:
 * ----------
 *   Input:  nums = [1,1,1,2,2,3], k = 2
 *   Output: [1,2]
 * 
 * EXAMPLE 2:
 * ----------
 *   Input:  nums = [1], k = 1
 *   Output: [1]
 * 
 * CONSTRAINTS:
 * ------------
 * - 1 <= nums.length <= 10^5
 * - -10^4 <= nums[i] <= 10^4
 * - k is in the range [1, the number of unique elements in the array]
 * - It is guaranteed that the answer is unique
 * 
 * HINTS:
 * ------
 * 1. First, count frequency of each element using a hash map
 * 2. Use a min-heap of size k to find top k frequent
 * 3. Alternative: Bucket sort by frequency - O(n) time!
 * 
 * FOLLOW-UP:
 * ----------
 * Your algorithm's time complexity must be better than O(n log n), where n 
 * is the array's size.
 */

#include <iostream>
#include <vector>
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <cassert>

/*
 * Approach 1: Using Min-Heap
 * Count frequencies, then use min-heap of size k
 * 
 * @param nums: Input array
 * @param k: Number of top frequent elements to return
 * @return: Vector of k most frequent elements
 */
std::vector<int> topKFrequentHeap(std::vector<int>& nums, int k) {
    // TODO: Implement using hash map + min-heap
    
    return {};
}

/*
 * Approach 2: Bucket Sort
 * Use frequency as index - O(n) time!
 * 
 * @param nums: Input array
 * @param k: Number of top frequent elements to return
 * @return: Vector of k most frequent elements
 */
std::vector<int> topKFrequentBucket(std::vector<int>& nums, int k) {
    // TODO: Implement using bucket sort by frequency
    
    return {};
}

/*
 * Approach 3: Using QuickSelect on frequencies
 * O(n) average time
 */
std::vector<int> topKFrequentQuickSelect(std::vector<int>& nums, int k) {
    // TODO: Implement using QuickSelect
    
    return {};
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



// ============================================================================
// SOLUTION: See solutions/03_top_k_frequent_solution.cpp
// ============================================================================
