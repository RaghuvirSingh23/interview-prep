/*
 * ============================================================================
 * Problem: Reverse Linked List
 * ============================================================================
 * Difficulty: Easy
 * Source: LeetCode 206
 * Time: 15-20 minutes
 * 
 * DESCRIPTION:
 * ------------
 * Given the head of a singly linked list, reverse the list, and return the 
 * reversed list.
 * 
 * EXAMPLE 1:
 * ----------
 *   Input:  1 -> 2 -> 3 -> 4 -> 5
 *   Output: 5 -> 4 -> 3 -> 2 -> 1
 * 
 * EXAMPLE 2:
 * ----------
 *   Input:  1 -> 2
 *   Output: 2 -> 1
 * 
 * EXAMPLE 3:
 * ----------
 *   Input:  []
 *   Output: []
 * 
 * CONSTRAINTS:
 * ------------
 * - The number of nodes in the list is in the range [0, 5000]
 * - -5000 <= Node.val <= 5000
 * 
 * HINTS:
 * ------
 * 1. Iterative: Use three pointers (prev, current, next)
 * 2. Recursive: The base case is when head is null or has no next
 * 3. Think about what happens to each pointer at each step
 * 
 * FOLLOW-UP:
 * ----------
 * - Can you implement both iterative and recursive solutions?
 * - Can you reverse a portion of the list (between positions m and n)?
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
 * Reverse the linked list iteratively
 * 
 * @param head: Head of the original list
 * @return: Head of the reversed list
 */
ListNode* reverseListIterative(ListNode* head) {
    // TODO: Implement iterative solution
    
    return nullptr;
}

/*
 * Reverse the linked list recursively
 * 
 * @param head: Head of the original list
 * @return: Head of the reversed list
 */
ListNode* reverseListRecursive(ListNode* head) {
    // TODO: Implement recursive solution
    
    return nullptr;
}


// ============================================================================
// TEST CASES
// ============================================================================

void runTests() {
    std::cout << "Running Reverse Linked List Tests...\n" << std::endl;
    
    // Test iterative solution
    std::cout << "--- Testing Iterative Solution ---" << std::endl;
    
    // Test 1: Normal case
    {
        ListNode* list = createList({1, 2, 3, 4, 5});
        ListNode* reversed = reverseListIterative(list);
        assert(listToVector(reversed) == std::vector<int>({5, 4, 3, 2, 1}));
        freeList(reversed);
        std::cout << "✓ Test 1 passed: Normal case" << std::endl;
    }
    
    // Test 2: Two elements
    {
        ListNode* list = createList({1, 2});
        ListNode* reversed = reverseListIterative(list);
        assert(listToVector(reversed) == std::vector<int>({2, 1}));
        freeList(reversed);
        std::cout << "✓ Test 2 passed: Two elements" << std::endl;
    }
    
    // Test 3: Single element
    {
        ListNode* list = createList({1});
        ListNode* reversed = reverseListIterative(list);
        assert(listToVector(reversed) == std::vector<int>({1}));
        freeList(reversed);
        std::cout << "✓ Test 3 passed: Single element" << std::endl;
    }
    
    // Test 4: Empty list
    {
        ListNode* reversed = reverseListIterative(nullptr);
        assert(reversed == nullptr);
        std::cout << "✓ Test 4 passed: Empty list" << std::endl;
    }
    
    // Test recursive solution
    std::cout << "\n--- Testing Recursive Solution ---" << std::endl;
    
    // Test 5: Normal case (recursive)
    {
        ListNode* list = createList({1, 2, 3, 4, 5});
        ListNode* reversed = reverseListRecursive(list);
        assert(listToVector(reversed) == std::vector<int>({5, 4, 3, 2, 1}));
        freeList(reversed);
        std::cout << "✓ Test 5 passed: Normal case (recursive)" << std::endl;
    }
    
    // Test 6: Empty list (recursive)
    {
        ListNode* reversed = reverseListRecursive(nullptr);
        assert(reversed == nullptr);
        std::cout << "✓ Test 6 passed: Empty list (recursive)" << std::endl;
    }
    
    std::cout << "\n=== All tests passed! ===" << std::endl;
}

int main() {
    runTests();
    return 0;
}



// ============================================================================
// SOLUTION: See solutions/02_reverse_linked_list_solution.cpp
// ============================================================================
