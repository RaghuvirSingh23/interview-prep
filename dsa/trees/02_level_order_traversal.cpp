/*
 * ============================================================================
 * Problem: Binary Tree Level Order Traversal
 * ============================================================================
 * Difficulty: Easy-Medium
 * Source: LeetCode 102
 * Time: 20-25 minutes
 * 
 * DESCRIPTION:
 * ------------
 * Given the root of a binary tree, return the level order traversal of its 
 * nodes' values. (i.e., from left to right, level by level).
 * 
 * EXAMPLE 1:
 * ----------
 *       3
 *      / \
 *     9  20
 *       /  \
 *      15   7
 *   
 *   Input:  root = [3,9,20,null,null,15,7]
 *   Output: [[3],[9,20],[15,7]]
 * 
 * EXAMPLE 2:
 * ----------
 *   Input:  root = [1]
 *   Output: [[1]]
 * 
 * EXAMPLE 3:
 * ----------
 *   Input:  root = []
 *   Output: []
 * 
 * CONSTRAINTS:
 * ------------
 * - The number of nodes in the tree is in the range [0, 2000]
 * - -1000 <= Node.val <= 1000
 * 
 * HINTS:
 * ------
 * 1. Use BFS (Breadth-First Search) with a queue
 * 2. Process all nodes at current level before moving to next level
 * 3. Track the number of nodes at each level
 * 
 * VARIATIONS:
 * -----------
 * - Zigzag Level Order (LeetCode 103)
 * - Level Order Bottom to Top (LeetCode 107)
 * - Average of Levels (LeetCode 637)
 */

#include <iostream>
#include <vector>
#include <queue>
#include <cassert>

struct TreeNode {
    int val;
    TreeNode* left;
    TreeNode* right;
    TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
};

TreeNode* buildTree(const std::vector<int>& values) {
    if (values.empty() || values[0] == -1001) return nullptr;
    
    TreeNode* root = new TreeNode(values[0]);
    std::queue<TreeNode*> q;
    q.push(root);
    
    size_t i = 1;
    while (i < values.size() && !q.empty()) {
        TreeNode* node = q.front();
        q.pop();
        
        if (i < values.size() && values[i] != -1001) {
            node->left = new TreeNode(values[i]);
            q.push(node->left);
        }
        i++;
        
        if (i < values.size() && values[i] != -1001) {
            node->right = new TreeNode(values[i]);
            q.push(node->right);
        }
        i++;
    }
    
    return root;
}

void freeTree(TreeNode* root) {
    if (!root) return;
    freeTree(root->left);
    freeTree(root->right);
    delete root;
}

/*
 * Level order traversal using BFS
 * 
 * @param root: Root of the binary tree
 * @return: 2D vector where each inner vector contains values at that level
 */
std::vector<std::vector<int>> levelOrder(TreeNode* root) {
    // TODO: Implement BFS level order traversal
    
    return {};
}

/*
 * BONUS: Zigzag level order traversal
 * Alternate between left-to-right and right-to-left
 */
std::vector<std::vector<int>> zigzagLevelOrder(TreeNode* root) {
    // TODO: Implement zigzag traversal
    
    return {};
}

/*
 * BONUS: Level order from bottom to top
 */
std::vector<std::vector<int>> levelOrderBottom(TreeNode* root) {
    // TODO: Implement bottom-up level order
    
    return {};
}


// ============================================================================
// TEST CASES
// ============================================================================

void runTests() {
    std::cout << "Running Level Order Traversal Tests...\n" << std::endl;
    
    // Test 1: Normal tree
    {
        // Using -1001 as null marker
        TreeNode* root = buildTree({3, 9, 20, -1001, -1001, 15, 7});
        auto result = levelOrder(root);
        std::vector<std::vector<int>> expected = {{3}, {9, 20}, {15, 7}};
        assert(result == expected);
        freeTree(root);
        std::cout << "✓ Test 1 passed: Normal tree" << std::endl;
    }
    
    // Test 2: Single node
    {
        TreeNode* root = buildTree({1});
        auto result = levelOrder(root);
        std::vector<std::vector<int>> expected = {{1}};
        assert(result == expected);
        freeTree(root);
        std::cout << "✓ Test 2 passed: Single node" << std::endl;
    }
    
    // Test 3: Empty tree
    {
        auto result = levelOrder(nullptr);
        assert(result.empty());
        std::cout << "✓ Test 3 passed: Empty tree" << std::endl;
    }
    
    // Test 4: Left-skewed tree
    {
        TreeNode* root = buildTree({1, 2, -1001, 3, -1001});
        auto result = levelOrder(root);
        std::vector<std::vector<int>> expected = {{1}, {2}, {3}};
        assert(result == expected);
        freeTree(root);
        std::cout << "✓ Test 4 passed: Left-skewed tree" << std::endl;
    }
    
    // Test 5: Complete binary tree
    {
        TreeNode* root = buildTree({1, 2, 3, 4, 5, 6, 7});
        auto result = levelOrder(root);
        std::vector<std::vector<int>> expected = {{1}, {2, 3}, {4, 5, 6, 7}};
        assert(result == expected);
        freeTree(root);
        std::cout << "✓ Test 5 passed: Complete binary tree" << std::endl;
    }
    
    // Test 6: Zigzag traversal
    {
        TreeNode* root = buildTree({3, 9, 20, -1001, -1001, 15, 7});
        auto result = zigzagLevelOrder(root);
        std::vector<std::vector<int>> expected = {{3}, {20, 9}, {15, 7}};
        assert(result == expected);
        freeTree(root);
        std::cout << "✓ Test 6 passed: Zigzag traversal" << std::endl;
    }
    
    std::cout << "\n=== All tests passed! ===" << std::endl;
}

int main() {
    runTests();
    return 0;
}



// ============================================================================
// SOLUTION: See solutions/02_level_order_traversal_solution.cpp
// ============================================================================
