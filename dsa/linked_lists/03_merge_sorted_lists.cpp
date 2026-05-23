/*
 * ============================================================================
 * Problem: Merge Two Sorted Lists
 * ============================================================================
 * Difficulty: Easy
 * Source: LeetCode 21
 * Time: 15-20 minutes
 * 
 * DESCRIPTION:
 * ------------
 * You are given the heads of two sorted linked lists list1 and list2.
 * Merge the two lists into one sorted list. The list should be made by 
 * splicing together the nodes of the first two lists.
 * Return the head of the merged linked list.
 * 
 * EXAMPLE 1:
 * ----------
 *   Input:  list1 = 1 -> 2 -> 4
 *           list2 = 1 -> 3 -> 4
 *   Output: 1 -> 1 -> 2 -> 3 -> 4 -> 4
 * 
 * EXAMPLE 2:
 * ----------
 *   Input:  list1 = []
 *           list2 = []
 *   Output: []
 * 
 * EXAMPLE 3:
 * ----------
 *   Input:  list1 = []
 *           list2 = 0
 *   Output: 0
 * 
 * CONSTRAINTS:
 * ------------
 * - The number of nodes in both lists is in the range [0, 50]
 * - -100 <= Node.val <= 100
 * - Both list1 and list2 are sorted in non-decreasing order
 * 
 * HINTS:
 * ------
 * 1. Use a dummy head node to simplify edge cases
 * 2. Compare values at each step and choose the smaller one
 * 3. Don't forget to append the remaining nodes when one list is exhausted
 * 4. Both iterative and recursive solutions work well
 * 
 * FOLLOW-UP:
 * ----------
 * - Can you merge K sorted lists? (LeetCode 23 - see heaps folder)
 */

#include <iostream>
#include <vector>
#include <cassert>

struct ListNode {
    int val;
    ListNode* next;
    ListNode(int x) : val(x), next(nullptr) {}
};

ListNode* createList(const std::vector<int>& values) {
    if (values.empty()) return nullptr;
    ListNode* head = new ListNode(values[0]);
    ListNode* current = head;
    for (size_t i = 1; i < values.size(); i++) {
        current->next = new ListNode(values[i]);
        current = current->next;
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
 * Merge two sorted linked lists (iterative)
 * 
 * @param list1: Head of first sorted list
 * @param list2: Head of second sorted list
 * @return: Head of merged sorted list
 */
ListNode* mergeTwoListsIterative(ListNode* list1, ListNode* list2) {
    // TODO: Implement iterative solution
    
    return nullptr;
}

/*
 * Merge two sorted linked lists (recursive)
 * 
 * @param list1: Head of first sorted list
 * @param list2: Head of second sorted list
 * @return: Head of merged sorted list
 */
ListNode* mergeTwoListsRecursive(ListNode* list1, ListNode* list2) {
    // TODO: Implement recursive solution
    
    return nullptr;
}


// ============================================================================
// TEST CASES
// ============================================================================

void runTests() {
    std::cout << "Running Merge Two Sorted Lists Tests...\n" << std::endl;
    
    // Test iterative solution
    std::cout << "--- Testing Iterative Solution ---" << std::endl;
    
    // Test 1: Normal case
    {
        ListNode* list1 = createList({1, 2, 4});
        ListNode* list2 = createList({1, 3, 4});
        ListNode* merged = mergeTwoListsIterative(list1, list2);
        assert(listToVector(merged) == std::vector<int>({1, 1, 2, 3, 4, 4}));
        freeList(merged);
        std::cout << "✓ Test 1 passed: Normal case" << std::endl;
    }
    
    // Test 2: Both empty
    {
        ListNode* merged = mergeTwoListsIterative(nullptr, nullptr);
        assert(merged == nullptr);
        std::cout << "✓ Test 2 passed: Both empty" << std::endl;
    }
    
    // Test 3: One empty
    {
        ListNode* list1 = createList({1, 2, 3});
        ListNode* merged = mergeTwoListsIterative(list1, nullptr);
        assert(listToVector(merged) == std::vector<int>({1, 2, 3}));
        freeList(merged);
        std::cout << "✓ Test 3 passed: One empty" << std::endl;
    }
    
    // Test 4: Different lengths
    {
        ListNode* list1 = createList({1, 5, 10});
        ListNode* list2 = createList({2, 3, 4, 6, 7, 8, 9});
        ListNode* merged = mergeTwoListsIterative(list1, list2);
        assert(listToVector(merged) == std::vector<int>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));
        freeList(merged);
        std::cout << "✓ Test 4 passed: Different lengths" << std::endl;
    }
    
    // Test recursive solution
    std::cout << "\n--- Testing Recursive Solution ---" << std::endl;
    
    // Test 5: Normal case (recursive)
    {
        ListNode* list1 = createList({1, 2, 4});
        ListNode* list2 = createList({1, 3, 4});
        ListNode* merged = mergeTwoListsRecursive(list1, list2);
        assert(listToVector(merged) == std::vector<int>({1, 1, 2, 3, 4, 4}));
        freeList(merged);
        std::cout << "✓ Test 5 passed: Normal case (recursive)" << std::endl;
    }
    
    // Test 6: One empty (recursive)
    {
        ListNode* list2 = createList({0});
        ListNode* merged = mergeTwoListsRecursive(nullptr, list2);
        assert(listToVector(merged) == std::vector<int>({0}));
        freeList(merged);
        std::cout << "✓ Test 6 passed: One empty (recursive)" << std::endl;
    }
    
    std::cout << "\n=== All tests passed! ===" << std::endl;
}

int main() {
    runTests();
    return 0;
}



// ============================================================================
// SOLUTION: See solutions/03_merge_sorted_lists_solution.cpp
// ============================================================================
