/*
 * SOLUTION: Merge Two Sorted Lists
 * 
 * Two Approaches:
 * 1. Iterative: Dummy node + compare and link
 * 2. Recursive: Choose smaller, recurse on rest
 * 
 * Time Complexity: O(n + m)
 * Space Complexity: O(1) iterative, O(n + m) recursive
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

// Iterative Solution - O(n + m) time, O(1) space
ListNode* mergeTwoListsIterative(ListNode* list1, ListNode* list2) {
    // Dummy node simplifies edge cases
    ListNode dummy(0);
    ListNode* tail = &dummy;
    
    while (list1 && list2) {
        if (list1->val <= list2->val) {
            tail->next = list1;
            list1 = list1->next;
        } else {
            tail->next = list2;
            list2 = list2->next;
        }
        tail = tail->next;
    }
    
    // Append remaining nodes
    tail->next = list1 ? list1 : list2;
    
    return dummy.next;
}

// Recursive Solution - O(n + m) time, O(n + m) space (call stack)
ListNode* mergeTwoListsRecursive(ListNode* list1, ListNode* list2) {
    // Base cases
    if (!list1) return list2;
    if (!list2) return list1;
    
    // Choose smaller value as head, recursively merge rest
    if (list1->val <= list2->val) {
        list1->next = mergeTwoListsRecursive(list1->next, list2);
        return list1;
    } else {
        list2->next = mergeTwoListsRecursive(list1, list2->next);
        return list2;
    }
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

