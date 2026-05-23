/*
 * ============================================================================
 * Problem: Kth Largest Element in an Array
 * ============================================================================
 * Difficulty: Medium
 * Source: LeetCode 215
 * Time: 20-25 minutes
 * 
 * DESCRIPTION:
 * ------------
 * Given an integer array nums and an integer k, return the kth largest 
 * element in the array.
 * 
 * Note that it is the kth largest element in the sorted order, not the 
 * kth distinct element.
 * 
 * Can you solve it without sorting?
 * 
 * EXAMPLE 1:
 * ----------
 *   Input:  nums = [3,2,1,5,6,4], k = 2
 *   Output: 5
 * 
 * EXAMPLE 2:
 * ----------
 *   Input:  nums = [3,2,3,1,2,4,5,5,6], k = 4
 *   Output: 4
 * 
 * CONSTRAINTS:
 * ------------
 * - 1 <= k <= nums.length <= 10^5
 * - -10^4 <= nums[i] <= 10^4
 * 
 * HINTS:
 * ------
 * 1. Sorting gives O(n log n) - can we do better?
 * 2. Use a min-heap of size k - keep only k largest elements
 * 3. QuickSelect algorithm gives O(n) average case
 * 4. Think: kth largest = (n-k+1)th smallest
 * 
 * APPROACHES:
 * -----------
 * 1. Sort: O(n log n) time, O(1) space
 * 2. Min-Heap of size k: O(n log k) time, O(k) space
 * 3. Max-Heap: O(n + k log n) time, O(n) space
 * 4. QuickSelect: O(n) average, O(n²) worst case
 */

#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <cassert>

/*
 * Approach 1: Using Min-Heap of size k
 * Keep only the k largest elements in the heap
 * The top of min-heap is the kth largest
 * 
 * @param nums: Input array
 * @param k: Find kth largest
 * @return: The kth largest element
 */
int findKthLargestHeap(std::vector<int>& nums, int k) {
    // TODO: Implement using min-heap
    // Hint: Use std::priority_queue with std::greater<int> for min-heap
    
    return 0;
}

/*
 * Approach 2: Using QuickSelect (Hoare's selection algorithm)
 * Similar to QuickSort but only recurse into one partition
 * 
 * @param nums: Input array
 * @param k: Find kth largest
 * @return: The kth largest element
 */
int findKthLargestQuickSelect(std::vector<int>& nums, int k) {
    // TODO: Implement using QuickSelect
    
    return 0;
}

/*
 * Helper: Partition function for QuickSelect
 */
int partition(std::vector<int>& nums, int left, int right) {
    // TODO: Implement partition
    
    return left;
}


// ============================================================================
// TEST CASES
// ============================================================================

void runTests() {
    std::cout << "Running Kth Largest Tests...\n" << std::endl;
    
    // Test 1: Normal case
    {
        std::vector<int> nums = {3, 2, 1, 5, 6, 4};
        assert(findKthLargestHeap(nums, 2) == 5);
        std::cout << "✓ Test 1 passed: k=2 in [3,2,1,5,6,4]" << std::endl;
    }
    
    // Test 2: With duplicates
    {
        std::vector<int> nums = {3, 2, 3, 1, 2, 4, 5, 5, 6};
        assert(findKthLargestHeap(nums, 4) == 4);
        std::cout << "✓ Test 2 passed: k=4 with duplicates" << std::endl;
    }
    
    // Test 3: k = 1 (largest)
    {
        std::vector<int> nums = {1, 2, 3, 4, 5};
        assert(findKthLargestHeap(nums, 1) == 5);
        std::cout << "✓ Test 3 passed: k=1 (largest)" << std::endl;
    }
    
    // Test 4: k = n (smallest)
    {
        std::vector<int> nums = {1, 2, 3, 4, 5};
        assert(findKthLargestHeap(nums, 5) == 1);
        std::cout << "✓ Test 4 passed: k=n (smallest)" << std::endl;
    }
    
    // Test 5: Single element
    {
        std::vector<int> nums = {42};
        assert(findKthLargestHeap(nums, 1) == 42);
        std::cout << "✓ Test 5 passed: Single element" << std::endl;
    }
    
    // Test 6: Negative numbers
    {
        std::vector<int> nums = {-1, -2, -3, -4, -5};
        assert(findKthLargestHeap(nums, 2) == -2);
        std::cout << "✓ Test 6 passed: Negative numbers" << std::endl;
    }
    
    // Test QuickSelect
    std::cout << "\n--- Testing QuickSelect ---" << std::endl;
    
    // Test 7: QuickSelect normal case
    {
        std::vector<int> nums = {3, 2, 1, 5, 6, 4};
        assert(findKthLargestQuickSelect(nums, 2) == 5);
        std::cout << "✓ Test 7 passed: QuickSelect k=2" << std::endl;
    }
    
    // Test 8: QuickSelect with duplicates
    {
        std::vector<int> nums = {3, 2, 3, 1, 2, 4, 5, 5, 6};
        assert(findKthLargestQuickSelect(nums, 4) == 4);
        std::cout << "✓ Test 8 passed: QuickSelect with duplicates" << std::endl;
    }
    
    std::cout << "\n=== All tests passed! ===" << std::endl;
}

int main() {
    runTests();
    return 0;
}



// ============================================================================
// SOLUTION: See solutions/01_kth_largest_solution.cpp
// ============================================================================
