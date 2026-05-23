/*
 * SOLUTION: Split Linked List by Odd and Even Indices
 * 
 * Approach: Two pointer technique
 * - Maintain separate odd and even pointers
 * - Alternate linking nodes to each list
 * 
 * Time Complexity: O(n)
 * Space Complexity: O(1)
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

// Solution 1: In-place with two pointers
std::pair<ListNode*, ListNode*> splitOddEven(ListNode* head) {
    if (!head) return {nullptr, nullptr};
    
    ListNode* odd = head;
    ListNode* even = head->next;
    ListNode* evenHead = even;
    
    while (even && even->next) {
        odd->next = even->next;
        odd = odd->next;
        even->next = odd->next;
        even = even->next;
    }
    
    odd->next = nullptr;  // Terminate odd list
    
    return {head, evenHead};
}

// Solution 2: More explicit version with dummy nodes
std::pair<ListNode*, ListNode*> splitOddEvenExplicit(ListNode* head) {
    if (!head) return {nullptr, nullptr};
    
    ListNode oddDummy(0), evenDummy(0);
    ListNode* oddTail = &oddDummy;
    ListNode* evenTail = &evenDummy;
    
    int index = 1;
    ListNode* current = head;
    
    while (current) {
        ListNode* next = current->next;
        current->next = nullptr;
        
        if (index % 2 == 1) {  // Odd index
            oddTail->next = current;
            oddTail = current;
        } else {  // Even index
            evenTail->next = current;
            evenTail = current;
        }
        
        current = next;
        index++;
    }
    
    return {oddDummy.next, evenDummy.next};
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

