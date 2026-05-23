/*
 * SOLUTION: Reverse Linked List
 * 
 * Two Approaches:
 * 1. Iterative: Three pointers (prev, current, next)
 * 2. Recursive: Reverse rest, then fix pointers
 * 
 * Time Complexity: O(n)
 * Space Complexity: O(1) iterative, O(n) recursive
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

// Iterative Solution - O(n) time, O(1) space
ListNode* reverseListIterative(ListNode* head) {
    ListNode* prev = nullptr;
    ListNode* current = head;
    
    while (current) {
        ListNode* next = current->next;  // Save next
        current->next = prev;            // Reverse pointer
        prev = current;                  // Move prev forward
        current = next;                  // Move current forward
    }
    
    return prev;
}

// Recursive Solution - O(n) time, O(n) space (call stack)
ListNode* reverseListRecursive(ListNode* head) {
    // Base case: empty list or single node
    if (!head || !head->next) {
        return head;
    }
    
    // Recursively reverse the rest
    ListNode* newHead = reverseListRecursive(head->next);
    
    // head->next is now the last node of reversed sublist
    // Make it point back to head
    head->next->next = head;
    head->next = nullptr;
    
    return newHead;
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

