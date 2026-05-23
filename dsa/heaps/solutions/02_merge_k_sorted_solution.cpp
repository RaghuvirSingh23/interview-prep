/*
 * SOLUTION: Merge K Sorted Lists
 * 
 * Two Approaches:
 * 1. Min-Heap - O(N log k) time, O(k) space
 * 2. Divide and Conquer - O(N log k) time, O(log k) space
 * 
 * Where N = total nodes, k = number of lists
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

// Approach 1: Min-Heap - O(N log k) time, O(k) space
ListNode* mergeKListsHeap(std::vector<ListNode*>& lists) {
    // Custom comparator for min-heap
    auto compare = [](ListNode* a, ListNode* b) {
        return a->val > b->val;  // Min-heap: smaller values have higher priority
    };
    
    std::priority_queue<ListNode*, std::vector<ListNode*>, decltype(compare)> minHeap(compare);
    
    // Initialize heap with head of each non-empty list
    for (ListNode* list : lists) {
        if (list) {
            minHeap.push(list);
        }
    }
    
    ListNode dummy(0);
    ListNode* tail = &dummy;
    
    while (!minHeap.empty()) {
        // Get smallest element
        ListNode* smallest = minHeap.top();
        minHeap.pop();
        
        // Add to result
        tail->next = smallest;
        tail = tail->next;
        
        // If there's more in this list, add next node to heap
        if (smallest->next) {
            minHeap.push(smallest->next);
        }
    }
    
    return dummy.next;
}

// Helper: Merge two sorted lists
ListNode* mergeTwoLists(ListNode* l1, ListNode* l2) {
    ListNode dummy(0);
    ListNode* tail = &dummy;
    
    while (l1 && l2) {
        if (l1->val <= l2->val) {
            tail->next = l1;
            l1 = l1->next;
        } else {
            tail->next = l2;
            l2 = l2->next;
        }
        tail = tail->next;
    }
    
    tail->next = l1 ? l1 : l2;
    return dummy.next;
}

// Approach 2: Divide and Conquer - O(N log k) time, O(log k) space
ListNode* mergeKListsDivideConquer(std::vector<ListNode*>& lists) {
    if (lists.empty()) return nullptr;
    
    int n = lists.size();
    
    // Merge pairs until only one list remains
    while (n > 1) {
        int k = (n + 1) / 2;  // Ceiling division
        
        for (int i = 0; i < n / 2; i++) {
            lists[i] = mergeTwoLists(lists[i], lists[i + k]);
        }
        
        n = k;
    }
    
    return lists[0];
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

