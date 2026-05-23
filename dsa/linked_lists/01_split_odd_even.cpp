/*
 * ============================================================================
 * Problem: Split Linked List by Odd and Even Indices
 * ============================================================================
 * Difficulty: Easy-Medium
 * Source: NextHop.AI Interview Guidelines
 * Time: 20-30 minutes
 * 
 * DESCRIPTION:
 * ------------
 * Write a function to split a linked list into two lists based on odd and 
 * even indices. The first list should contain all nodes at odd indices 
 * (1, 3, 5, ...) and the second list should contain all nodes at even 
 * indices (2, 4, 6, ...).
 * 
 * Note: We use 1-based indexing (first node is index 1).
 * 
 * EXAMPLE:
 * --------
 *   Input:  1 -> 2 -> 3 -> 4 -> 5 -> 6 -> 7
 *   
 *   Output: 
 *     Odd list:  1 -> 3 -> 5 -> 7
 *     Even list: 2 -> 4 -> 6
 * 
 * CONSTRAINTS:
 * ------------
 * - The number of nodes in the list is in the range [0, 10^4]
 * - -10^6 <= Node.val <= 10^6
 * 
 * HINTS:
 * ------
 * 1. Use two pointers to track the current odd and even nodes
 * 2. Iterate through the list, alternating between odd and even
 * 3. Keep track of the head of each list
 * 4. Don't forget to terminate both lists properly (set last->next = nullptr)
 * 
 * FOLLOW-UP:
 * ----------
 * - Can you do it in-place with O(1) extra space?
 * - What if you want to maintain relative order within each list?
 */

#include <iostream>
#include <vector>
#include <cassert>

// Definition for singly-linked list
struct ListNode {
    int val;
    ListNode* next;
    ListNode(int x) : val(x), next(nullptr) {}
};

// Helper function to create a linked list from vector
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

// Helper function to convert list to vector (for testing)
std::vector<int> listToVector(ListNode* head) {
    std::vector<int> result;
    while (head) {
        result.push_back(head->val);
        head = head->next;
    }
    return result;
}

// Helper function to print a list
void printList(ListNode* head, const std::string& label = "") {
    if (!label.empty()) std::cout << label << ": ";
    while (head) {
        std::cout << head->val;
        if (head->next) std::cout << " -> ";
        head = head->next;
    }
    std::cout << std::endl;
}

// Helper function to free a list
void freeList(ListNode* head) {
    while (head) {
        ListNode* temp = head;
        head = head->next;
        delete temp;
    }
}

/*
 * Split the linked list into odd-indexed and even-indexed lists
 * 
 * @param head: Head of the original list
 * @return: Pair of (oddHead, evenHead)
 */
std::pair<ListNode*, ListNode*> splitOddEven(ListNode* head) {
    // TODO: Implement your solution here
    
    return {nullptr, nullptr};
}


// ============================================================================
// TEST CASES
// ============================================================================

void runTests() {
    std::cout << "Running Split Odd Even Tests...\n" << std::endl;
    
    // Test 1: Normal case
    {
        ListNode* list = createList({1, 2, 3, 4, 5, 6, 7});
        auto [odd, even] = splitOddEven(list);
        
        assert(listToVector(odd) == std::vector<int>({1, 3, 5, 7}));
        assert(listToVector(even) == std::vector<int>({2, 4, 6}));
        
        freeList(odd);
        freeList(even);
        std::cout << "✓ Test 1 passed: Normal case (7 elements)" << std::endl;
    }
    
    // Test 2: Even number of elements
    {
        ListNode* list = createList({1, 2, 3, 4, 5, 6});
        auto [odd, even] = splitOddEven(list);
        
        assert(listToVector(odd) == std::vector<int>({1, 3, 5}));
        assert(listToVector(even) == std::vector<int>({2, 4, 6}));
        
        freeList(odd);
        freeList(even);
        std::cout << "✓ Test 2 passed: Even number of elements" << std::endl;
    }
    
    // Test 3: Single element
    {
        ListNode* list = createList({42});
        auto [odd, even] = splitOddEven(list);
        
        assert(listToVector(odd) == std::vector<int>({42}));
        assert(even == nullptr);
        
        freeList(odd);
        std::cout << "✓ Test 3 passed: Single element" << std::endl;
    }
    
    // Test 4: Two elements
    {
        ListNode* list = createList({1, 2});
        auto [odd, even] = splitOddEven(list);
        
        assert(listToVector(odd) == std::vector<int>({1}));
        assert(listToVector(even) == std::vector<int>({2}));
        
        freeList(odd);
        freeList(even);
        std::cout << "✓ Test 4 passed: Two elements" << std::endl;
    }
    
    // Test 5: Empty list
    {
        auto [odd, even] = splitOddEven(nullptr);
        
        assert(odd == nullptr);
        assert(even == nullptr);
        
        std::cout << "✓ Test 5 passed: Empty list" << std::endl;
    }
    
    std::cout << "\n=== All tests passed! ===" << std::endl;
}

int main() {
    runTests();
    return 0;
}



// ============================================================================
// SOLUTION: See solutions/01_split_odd_even_solution.cpp
// ============================================================================
