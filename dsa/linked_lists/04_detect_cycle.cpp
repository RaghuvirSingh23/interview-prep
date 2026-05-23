/*
 * ============================================================================
 * Problem: Linked List Cycle Detection
 * ============================================================================
 * Difficulty: Medium
 * Source: LeetCode 141, 142
 * Time: 25-30 minutes
 * 
 * DESCRIPTION:
 * ------------
 * Given head, the head of a linked list, determine if the linked list has a 
 * cycle in it.
 * 
 * There is a cycle in a linked list if there is some node in the list that 
 * can be reached again by continuously following the next pointer.
 * 
 * PART 1: Detect if cycle exists (return true/false)
 * PART 2: Find the node where the cycle begins (return that node or nullptr)
 * 
 * EXAMPLE 1:
 * ----------
 *   Input:  3 -> 2 -> 0 -> -4 -+
 *                ^            |
 *                +------------+
 *   Output: true (cycle at node with value 2)
 * 
 * EXAMPLE 2:
 * ----------
 *   Input:  1 -> 2 -+
 *           ^      |
 *           +------+
 *   Output: true (cycle at node with value 1)
 * 
 * EXAMPLE 3:
 * ----------
 *   Input:  1 -> null
 *   Output: false
 * 
 * CONSTRAINTS:
 * ------------
 * - The number of nodes in the list is in the range [0, 10^4]
 * - -10^5 <= Node.val <= 10^5
 * 
 * HINTS:
 * ------
 * 1. Floyd's Cycle Detection (Tortoise and Hare) algorithm
 * 2. Use two pointers: slow moves 1 step, fast moves 2 steps
 * 3. If they meet, there's a cycle
 * 4. For finding cycle start: when they meet, reset one to head and 
 *    move both at same speed - they'll meet at cycle start
 * 
 * FOLLOW-UP:
 * ----------
 * - Can you solve it using O(1) memory?
 * - Can you find the length of the cycle?
 */

#include <iostream>
#include <unordered_set>
#include <cassert>

struct ListNode {
    int val;
    ListNode* next;
    ListNode(int x) : val(x), next(nullptr) {}
};

// Helper to create a cycle for testing
// Returns head of list, cycle starts at index cycleStart (-1 for no cycle)
ListNode* createListWithCycle(const std::vector<int>& values, int cycleStart) {
    if (values.empty()) return nullptr;
    
    std::vector<ListNode*> nodes;
    for (int val : values) {
        nodes.push_back(new ListNode(val));
    }
    
    for (size_t i = 0; i < nodes.size() - 1; i++) {
        nodes[i]->next = nodes[i + 1];
    }
    
    // Create cycle if specified
    if (cycleStart >= 0 && cycleStart < static_cast<int>(nodes.size())) {
        nodes.back()->next = nodes[cycleStart];
    }
    
    return nodes[0];
}

// Free list (only works for non-cyclic lists!)
void freeList(ListNode* head) {
    std::unordered_set<ListNode*> visited;
    while (head && visited.find(head) == visited.end()) {
        visited.insert(head);
        ListNode* temp = head;
        head = head->next;
        delete temp;
    }
}

/*
 * PART 1: Detect if cycle exists
 * 
 * @param head: Head of the list
 * @return: true if cycle exists, false otherwise
 */
bool hasCycle(ListNode* head) {
    // TODO: Implement Floyd's Cycle Detection
    
    return false;
}

/*
 * PART 2: Find the start of the cycle
 * 
 * @param head: Head of the list
 * @return: Node where cycle begins, or nullptr if no cycle
 */
ListNode* detectCycleStart(ListNode* head) {
    // TODO: Implement cycle start detection
    
    return nullptr;
}

/*
 * BONUS: Find the length of the cycle
 * 
 * @param head: Head of the list
 * @return: Length of cycle, or 0 if no cycle
 */
int cycleLength(ListNode* head) {
    // TODO: Implement cycle length calculation
    
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



// ============================================================================
// SOLUTION: See solutions/04_detect_cycle_solution.cpp
// ============================================================================
