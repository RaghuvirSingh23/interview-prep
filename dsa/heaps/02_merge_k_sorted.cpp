/*
 * ============================================================================
 * Problem: Merge K Sorted Lists
 * ============================================================================
 * Difficulty: Hard
 * Source: LeetCode 23
 * Time: 30-40 minutes
 * 
 * DESCRIPTION:
 * ------------
 * You are given an array of k linked-lists lists, each linked-list is sorted 
 * in ascending order.
 * 
 * Merge all the linked-lists into one sorted linked-list and return it.
 * 
 * EXAMPLE 1:
 * ----------
 *   Input:  lists = [[1,4,5],[1,3,4],[2,6]]
 *   Output: [1,1,2,3,4,4,5,6]
 *   
 *   Explanation: The linked-lists are:
 *   [
 *     1->4->5,
 *     1->3->4,
 *     2->6
 *   ]
 *   merging them into one sorted list:
 *   1->1->2->3->4->4->5->6
 * 
 * EXAMPLE 2:
 * ----------
 *   Input:  lists = []
 *   Output: []
 * 
 * EXAMPLE 3:
 * ----------
 *   Input:  lists = [[]]
 *   Output: []
 * 
 * CONSTRAINTS:
 * ------------
 * - k == lists.length
 * - 0 <= k <= 10^4
 * - 0 <= lists[i].length <= 500
 * - -10^4 <= lists[i][j] <= 10^4
 * - lists[i] is sorted in ascending order
 * - The sum of lists[i].length will not exceed 10^4
 * 
 * HINTS:
 * ------
 * 1. Brute force: Merge two lists at a time - O(kN) where N is total nodes
 * 2. Use a min-heap to always get the smallest element - O(N log k)
 * 3. Divide and conquer: Merge pairs, then merge results - O(N log k)
 * 
 * APPROACHES:
 * -----------
 * 1. Merge one by one: O(kN) time
 * 2. Min-Heap: O(N log k) time, O(k) space
 * 3. Divide and Conquer: O(N log k) time, O(log k) space
 */

#include <iostream>
#include <vector>
#include <queue>
#include <cassert>

struct ListNode {
    int val;
    ListNode* next;
    ListNode(int x) : val(x), next(nullptr) {}
};

// Helper functions
ListNode* createList(const std::vector<int>& values) {
    if (values.empty()) return nullptr;
    ListNode* head = new ListNode(values[0]);
    ListNode* curr = head;
    for (size_t i = 1; i < values.size(); i++) {
        curr->next = new ListNode(values[i]);
        curr = curr->next;
    }
    return head;
}

std::vector<int> listToVector(ListNode* head) {
    std::vector<int> result;
    while (head) {
        result.push_back(head->val);
        head = head->next;
    }
    return result;
}

void freeList(ListNode* head) {
    while (head) {
        ListNode* temp = head;
        head = head->next;
        delete temp;
    }
}

/*
 * Approach 1: Using Min-Heap
 * Always extract the smallest element from k lists
 * 
 * @param lists: Vector of k sorted linked lists
 * @return: Merged sorted linked list
 */
ListNode* mergeKListsHeap(std::vector<ListNode*>& lists) {
    // TODO: Implement using min-heap
    // Hint: Store {value, list_index} or {value, node_pointer} in heap
    
    return nullptr;
}

/*
 * Approach 2: Divide and Conquer
 * Merge pairs of lists, then merge results
 * 
 * @param lists: Vector of k sorted linked lists
 * @return: Merged sorted linked list
 */
ListNode* mergeKListsDivideConquer(std::vector<ListNode*>& lists) {
    // TODO: Implement using divide and conquer
    
    return nullptr;
}

/*
 * Helper: Merge two sorted lists
 */
ListNode* mergeTwoLists(ListNode* l1, ListNode* l2) {
    // TODO: Implement (you did this in linked_lists folder!)
    
    return nullptr;
}


// ============================================================================
// TEST CASES
// ============================================================================

void runTests() {
    std::cout << "Running Merge K Sorted Lists Tests...\n" << std::endl;
    
    // Test 1: Normal case
    {
        std::vector<ListNode*> lists = {
            createList({1, 4, 5}),
            createList({1, 3, 4}),
            createList({2, 6})
        };
        ListNode* merged = mergeKListsHeap(lists);
        assert(listToVector(merged) == std::vector<int>({1, 1, 2, 3, 4, 4, 5, 6}));
        freeList(merged);
        std::cout << "✓ Test 1 passed: Merge 3 lists" << std::endl;
    }
    
    // Test 2: Empty input
    {
        std::vector<ListNode*> lists;
        ListNode* merged = mergeKListsHeap(lists);
        assert(merged == nullptr);
        std::cout << "✓ Test 2 passed: Empty input" << std::endl;
    }
    
    // Test 3: Single empty list
    {
        std::vector<ListNode*> lists = {nullptr};
        ListNode* merged = mergeKListsHeap(lists);
        assert(merged == nullptr);
        std::cout << "✓ Test 3 passed: Single empty list" << std::endl;
    }
    
    // Test 4: Single non-empty list
    {
        std::vector<ListNode*> lists = {createList({1, 2, 3})};
        ListNode* merged = mergeKListsHeap(lists);
        assert(listToVector(merged) == std::vector<int>({1, 2, 3}));
        freeList(merged);
        std::cout << "✓ Test 4 passed: Single list" << std::endl;
    }
    
    // Test 5: Lists of different lengths
    {
        std::vector<ListNode*> lists = {
            createList({1}),
            createList({2, 3, 4, 5, 6}),
            createList({7, 8})
        };
        ListNode* merged = mergeKListsHeap(lists);
        assert(listToVector(merged) == std::vector<int>({1, 2, 3, 4, 5, 6, 7, 8}));
        freeList(merged);
        std::cout << "✓ Test 5 passed: Different lengths" << std::endl;
    }
    
    // Test Divide and Conquer
    std::cout << "\n--- Testing Divide and Conquer ---" << std::endl;
    
    // Test 6: D&C normal case
    {
        std::vector<ListNode*> lists = {
            createList({1, 4, 5}),
            createList({1, 3, 4}),
            createList({2, 6})
        };
        ListNode* merged = mergeKListsDivideConquer(lists);
        assert(listToVector(merged) == std::vector<int>({1, 1, 2, 3, 4, 4, 5, 6}));
        freeList(merged);
        std::cout << "✓ Test 6 passed: D&C merge 3 lists" << std::endl;
    }
    
    std::cout << "\n=== All tests passed! ===" << std::endl;
}

int main() {
    runTests();
    return 0;
}



// ============================================================================
// SOLUTION: See solutions/02_merge_k_sorted_solution.cpp
// ============================================================================
