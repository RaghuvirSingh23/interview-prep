/*
 * SOLUTION: Binary Tree Level Order Traversal
 * 
 * Approach: BFS using queue
 * - Process all nodes at current level before moving to next
 * - Track level size to separate levels
 * 
 * Time Complexity: O(n)
 * Space Complexity: O(n) for queue
 */

#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
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

// Standard Level Order - O(n) time, O(n) space
std::vector<std::vector<int>> levelOrder(TreeNode* root) {
    std::vector<std::vector<int>> result;
    if (!root) return result;
    
    std::queue<TreeNode*> q;
    q.push(root);
    
    while (!q.empty()) {
        int levelSize = q.size();  // Number of nodes at current level
        std::vector<int> currentLevel;
        
        for (int i = 0; i < levelSize; i++) {
            TreeNode* node = q.front();
            q.pop();
            
            currentLevel.push_back(node->val);
            
            if (node->left) q.push(node->left);
            if (node->right) q.push(node->right);
        }
        
        result.push_back(currentLevel);
    }
    
    return result;
}

// Zigzag Level Order
std::vector<std::vector<int>> zigzagLevelOrder(TreeNode* root) {
    std::vector<std::vector<int>> result;
    if (!root) return result;
    
    std::queue<TreeNode*> q;
    q.push(root);
    bool leftToRight = true;
    
    while (!q.empty()) {
        int levelSize = q.size();
        std::vector<int> currentLevel(levelSize);
        
        for (int i = 0; i < levelSize; i++) {
            TreeNode* node = q.front();
            q.pop();
            
            // Determine index based on direction
            int idx = leftToRight ? i : (levelSize - 1 - i);
            currentLevel[idx] = node->val;
            
            if (node->left) q.push(node->left);
            if (node->right) q.push(node->right);
        }
        
        result.push_back(currentLevel);
        leftToRight = !leftToRight;  // Alternate direction
    }
    
    return result;
}

// Bottom-up Level Order
std::vector<std::vector<int>> levelOrderBottom(TreeNode* root) {
    std::vector<std::vector<int>> result = levelOrder(root);
    std::reverse(result.begin(), result.end());
    return result;
}


// ============================================================================
// TEST CASES
// ============================================================================

void runTests() {
    std::cout << "Running Level Order Traversal Tests...\n" << std::endl;
    
    // Test 1: Normal tree
    {
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

