/*
 * SOLUTION: Linked List Cycle Detection
 * 
 * Approach: Floyd's Cycle Detection (Tortoise and Hare)
 * - Slow pointer moves 1 step, fast moves 2 steps
 * - If they meet, cycle exists
 * - To find cycle start: reset one to head, move both at same speed
 * 
 * Time Complexity: O(n)
 * Space Complexity: O(1)
 */

#include <iostream>
#include <unordered_set>
#include <vector>
#include <cassert>

struct ListNode {
    int val;
    ListNode* next;
    ListNode(int x) : val(x), next(nullptr) {}
};

ListNode* createListWithCycle(const std::vector<int>& values, int cycleStart) {
    if (values.empty()) return nullptr;
    
    std::vector<ListNode*> nodes;
    for (int val : values) {
        nodes.push_back(new ListNode(val));
    }
    
    for (size_t i = 0; i < nodes.size() - 1; i++) {
        nodes[i]->next = nodes[i + 1];
    }
    
    if (cycleStart >= 0 && cycleStart < static_cast<int>(nodes.size())) {
        nodes.back()->next = nodes[cycleStart];
    }
    
    return nodes[0];
}

void freeList(ListNode* head) {
    std::unordered_set<ListNode*> visited;
    while (head && visited.find(head) == visited.end()) {
        visited.insert(head);
        ListNode* temp = head;
        head = head->next;
        delete temp;
    }
}

// PART 1: Detect cycle - O(n) time, O(1) space
bool hasCycle(ListNode* head) {
    if (!head || !head->next) return false;
    
    ListNode* slow = head;
    ListNode* fast = head;
    
    while (fast && fast->next) {
        slow = slow->next;        // Move 1 step
        fast = fast->next->next;  // Move 2 steps
        
        if (slow == fast) {
            return true;  // They met, cycle exists
        }
    }
    
    return false;  // Fast reached end, no cycle
}

// PART 2: Find cycle start - O(n) time, O(1) space
ListNode* detectCycleStart(ListNode* head) {
    if (!head || !head->next) return nullptr;
    
    ListNode* slow = head;
    ListNode* fast = head;
    
    // Phase 1: Detect cycle
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
        
        if (slow == fast) {
            // Phase 2: Find cycle start
            // Reset slow to head, keep fast at meeting point
            // Move both at same speed - they'll meet at cycle start
            slow = head;
            while (slow != fast) {
                slow = slow->next;
                fast = fast->next;
            }
            return slow;
        }
    }
    
    return nullptr;
}

// BONUS: Cycle length - O(n) time, O(1) space
int cycleLength(ListNode* head) {
    if (!head || !head->next) return 0;
    
    ListNode* slow = head;
    ListNode* fast = head;
    
    // Find meeting point
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
        
        if (slow == fast) {
            // Count cycle length
            int length = 1;
            ListNode* current = slow->next;
            while (current != slow) {
                length++;
                current = current->next;
            }
            return length;
        }
    }
    
    return 0;
}


// ============================================================================
// TEST CASES
// ============================================================================

void runTests() {
    std::cout << "Running Cycle Detection Tests...\n" << std::endl;
    
    // Test 1: Cycle exists
    {
        ListNode* list = createListWithCycle({3, 2, 0, -4}, 1);
        assert(hasCycle(list) == true);
        ListNode* cycleNode = detectCycleStart(list);
        assert(cycleNode != nullptr && cycleNode->val == 2);
        freeList(list);
        std::cout << "✓ Test 1 passed: Cycle at index 1" << std::endl;
    }
    
    // Test 2: Cycle at head
    {
        ListNode* list = createListWithCycle({1, 2}, 0);
        assert(hasCycle(list) == true);
        ListNode* cycleNode = detectCycleStart(list);
        assert(cycleNode != nullptr && cycleNode->val == 1);
        freeList(list);
        std::cout << "✓ Test 2 passed: Cycle at head" << std::endl;
    }
    
    // Test 3: No cycle
    {
        ListNode* list = createListWithCycle({1, 2, 3, 4, 5}, -1);
        assert(hasCycle(list) == false);
        assert(detectCycleStart(list) == nullptr);
        freeList(list);
        std::cout << "✓ Test 3 passed: No cycle" << std::endl;
    }
    
    // Test 4: Single node, no cycle
    {
        ListNode* list = createListWithCycle({1}, -1);
        assert(hasCycle(list) == false);
        freeList(list);
        std::cout << "✓ Test 4 passed: Single node, no cycle" << std::endl;
    }
    
    // Test 5: Single node with self-loop
    {
        ListNode* list = createListWithCycle({1}, 0);
        assert(hasCycle(list) == true);
        ListNode* cycleNode = detectCycleStart(list);
        assert(cycleNode != nullptr && cycleNode->val == 1);
        freeList(list);
        std::cout << "✓ Test 5 passed: Single node with self-loop" << std::endl;
    }
    
    // Test 6: Empty list
    {
        assert(hasCycle(nullptr) == false);
        assert(detectCycleStart(nullptr) == nullptr);
        std::cout << "✓ Test 6 passed: Empty list" << std::endl;
    }
    
    // Test 7: Cycle length
    {
        ListNode* list = createListWithCycle({1, 2, 3, 4, 5}, 2);
        // Cycle: 3 -> 4 -> 5 -> 3 (length 3)
        assert(cycleLength(list) == 3);
        freeList(list);
        std::cout << "✓ Test 7 passed: Cycle length" << std::endl;
    }
    
    std::cout << "\n=== All tests passed! ===" << std::endl;
}

int main() {
    runTests();
    return 0;
}

