/*
 * SOLUTION: Validate Binary Search Tree
 * 
 * Approach: Range checking
 * - Each node must be within a valid range
 * - As you go left, update upper bound
 * - As you go right, update lower bound
 * 
 * Time Complexity: O(n)
 * Space Complexity: O(h) for recursion stack
 */

#include <iostream>
#include <vector>
#include <queue>
#include <stack>
#include <limits>
#include <cassert>

struct TreeNode {
    int val;
    TreeNode* left;
    TreeNode* right;
    TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
};

TreeNode* buildTree(const std::vector<long>& values) {
    if (values.empty() || values[0] == LONG_MIN) return nullptr;
    
    TreeNode* root = new TreeNode(values[0]);
    std::queue<TreeNode*> q;
    q.push(root);
    
    size_t i = 1;
    while (i < values.size() && !q.empty()) {
        TreeNode* node = q.front();
        q.pop();
        
        if (i < values.size() && values[i] != LONG_MIN) {
            node->left = new TreeNode(values[i]);
            q.push(node->left);
        }
        i++;
        
        if (i < values.size() && values[i] != LONG_MIN) {
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

// Approach 1: Range Checking - O(n) time, O(h) space
bool isValidBSTHelper(TreeNode* node, long minVal, long maxVal) {
    if (!node) return true;
    
    // Check if current node violates the range
    if (node->val <= minVal || node->val >= maxVal) {
        return false;
    }
    
    // Left subtree: values must be less than current node
    // Right subtree: values must be greater than current node
    return isValidBSTHelper(node->left, minVal, node->val) &&
           isValidBSTHelper(node->right, node->val, maxVal);
}

bool isValidBST(TreeNode* root) {
    return isValidBSTHelper(root, LONG_MIN, LONG_MAX);
}

// Approach 2: In-order Traversal - O(n) time, O(h) space
bool inorderCheck(TreeNode* node, TreeNode*& prev) {
    if (!node) return true;
    
    // Check left subtree
    if (!inorderCheck(node->left, prev)) {
        return false;
    }
    
    // Check current node against previous
    if (prev && node->val <= prev->val) {
        return false;
    }
    prev = node;
    
    // Check right subtree
    return inorderCheck(node->right, prev);
}

bool isValidBSTInorder(TreeNode* root) {
    TreeNode* prev = nullptr;
    return inorderCheck(root, prev);
}

// Approach 3: Iterative In-order - O(n) time, O(h) space
bool isValidBSTIterative(TreeNode* root) {
    std::stack<TreeNode*> stk;
    TreeNode* prev = nullptr;
    TreeNode* curr = root;
    
    while (curr || !stk.empty()) {
        // Go to leftmost node
        while (curr) {
            stk.push(curr);
            curr = curr->left;
        }
        
        curr = stk.top();
        stk.pop();
        
        // Check against previous
        if (prev && curr->val <= prev->val) {
            return false;
        }
        prev = curr;
        
        // Move to right subtree
        curr = curr->right;
    }
    
    return true;
}


// ============================================================================
// TEST CASES
// ============================================================================

void runTests() {
    std::cout << "Running Validate BST Tests...\n" << std::endl;
    
    // Test 1: Valid BST
    {
        TreeNode* root = buildTree({2, 1, 3});
        assert(isValidBST(root) == true);
        freeTree(root);
        std::cout << "✓ Test 1 passed: Valid BST [2,1,3]" << std::endl;
    }
    
    // Test 2: Invalid - right child less than root
    {
        TreeNode* root = buildTree({5, 1, 4, LONG_MIN, LONG_MIN, 3, 6});
        assert(isValidBST(root) == false);
        freeTree(root);
        std::cout << "✓ Test 2 passed: Invalid [5,1,4,null,null,3,6]" << std::endl;
    }
    
    // Test 3: Tricky case - node violates ancestor constraint
    {
        //     5
        //    / \
        //   4   6
        //      / \
        //     3   7
        TreeNode* root = buildTree({5, 4, 6, LONG_MIN, LONG_MIN, 3, 7});
        assert(isValidBST(root) == false);
        freeTree(root);
        std::cout << "✓ Test 3 passed: Invalid - 3 in right subtree of 5" << std::endl;
    }
    
    // Test 4: Single node
    {
        TreeNode* root = buildTree({1});
        assert(isValidBST(root) == true);
        freeTree(root);
        std::cout << "✓ Test 4 passed: Single node" << std::endl;
    }
    
    // Test 5: Left-skewed valid BST
    {
        TreeNode* root = buildTree({3, 2, LONG_MIN, 1});
        assert(isValidBST(root) == true);
        freeTree(root);
        std::cout << "✓ Test 5 passed: Left-skewed valid BST" << std::endl;
    }
    
    // Test 6: Equal values (should be invalid)
    {
        TreeNode* root = buildTree({2, 2, 2});
        assert(isValidBST(root) == false);
        freeTree(root);
        std::cout << "✓ Test 6 passed: Equal values are invalid" << std::endl;
    }
    
    // Test 7: Large values (edge case with INT_MIN/MAX)
    {
        TreeNode* root = buildTree({INT_MAX});
        assert(isValidBST(root) == true);
        freeTree(root);
        std::cout << "✓ Test 7 passed: INT_MAX value" << std::endl;
    }
    
    // Test 8: Using in-order approach
    {
        TreeNode* root = buildTree({2, 1, 3});
        assert(isValidBSTInorder(root) == true);
        freeTree(root);
        std::cout << "✓ Test 8 passed: In-order validation" << std::endl;
    }
    
    std::cout << "\n=== All tests passed! ===" << std::endl;
}

int main() {
    runTests();
    return 0;
}

