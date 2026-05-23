/*
 * SOLUTION: Kth Largest Element in an Array
 * 
 * Two Approaches:
 * 1. Min-Heap of size k - O(n log k) time
 * 2. QuickSelect - O(n) average time
 * 
 * Space Complexity: O(k) for heap, O(1) for QuickSelect
 */

#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <cassert>

// Approach 1: Min-Heap - O(n log k) time, O(k) space
int findKthLargestHeap(std::vector<int>& nums, int k) {
    // Min-heap: smallest element at top
    std::priority_queue<int, std::vector<int>, std::greater<int>> minHeap;
    
    for (int num : nums) {
        minHeap.push(num);
        
        // Keep only k largest elements
        if (minHeap.size() > static_cast<size_t>(k)) {
            minHeap.pop();  // Remove smallest
        }
    }
    
    // Top of min-heap is kth largest
    return minHeap.top();
}

// Approach 2: QuickSelect - O(n) average, O(n²) worst
int partition(std::vector<int>& nums, int left, int right) {
    // Use random pivot to avoid worst case
    int pivotIdx = left + rand() % (right - left + 1);
    std::swap(nums[pivotIdx], nums[right]);
    
    int pivot = nums[right];
    int i = left;
    
    for (int j = left; j < right; j++) {
        if (nums[j] >= pivot) {  // Note: >= for descending order
            std::swap(nums[i], nums[j]);
            i++;
        }
    }
    
    std::swap(nums[i], nums[right]);
    return i;
}

int quickSelect(std::vector<int>& nums, int left, int right, int k) {
    if (left == right) {
        return nums[left];
    }
    
    int pivotIdx = partition(nums, left, right);
    
    if (pivotIdx == k - 1) {
        return nums[pivotIdx];
    } else if (pivotIdx > k - 1) {
        return quickSelect(nums, left, pivotIdx - 1, k);
    } else {
        return quickSelect(nums, pivotIdx + 1, right, k);
    }
}

int findKthLargestQuickSelect(std::vector<int>& nums, int k) {
    return quickSelect(nums, 0, nums.size() - 1, k);
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

