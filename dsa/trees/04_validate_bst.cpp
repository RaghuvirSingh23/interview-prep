/*
 * ============================================================================
 * Problem: Validate Binary Search Tree
 * ============================================================================
 * Difficulty: Medium
 * Source: LeetCode 98
 * Time: 20-25 minutes
 * 
 * DESCRIPTION:
 * ------------
 * Given the root of a binary tree, determine if it is a valid binary search 
 * tree (BST).
 * 
 * A valid BST is defined as follows:
 * - The left subtree of a node contains only nodes with keys less than the 
 *   node's key
 * - The right subtree of a node contains only nodes with keys greater than 
 *   the node's key
 * - Both the left and right subtrees must also be binary search trees
 * 
 * EXAMPLE 1:
 * ----------
 *       2
 *      / \
 *     1   3
 *   
 *   Input:  root = [2,1,3]
 *   Output: true
 * 
 * EXAMPLE 2:
 * ----------
 *       5
 *      / \
 *     1   4
 *        / \
 *       3   6
 *   
 *   Input:  root = [5,1,4,null,null,3,6]
 *   Output: false
 *   Explanation: The root node's value is 5 but its right child's value is 4.
 * 
 * EXAMPLE 3 (TRICKY):
 * -------------------
 *       5
 *      / \
 *     4   6
 *        / \
 *       3   7
 *   
 *   Output: false
 *   Explanation: 3 is in right subtree of 5, but 3 < 5. Invalid!
 * 
 * CONSTRAINTS:
 * ------------
 * - The number of nodes in the tree is in the range [1, 10^4]
 * - -2^31 <= Node.val <= 2^31 - 1
 * 
 * HINTS:
 * ------
 * 1. Common mistake: only checking node vs immediate children
 * 2. Each node must be within a valid range
 * 3. As you go left, update upper bound; as you go right, update lower bound
 * 4. Alternative: In-order traversal should give sorted sequence
 */

#include <iostream>
#include <vector>
#include <queue>
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

/*
 * Validate if tree is a valid BST using range checking
 * 
 * @param root: Root of the binary tree
 * @return: true if valid BST, false otherwise
 */
bool isValidBST(TreeNode* root) {
    // TODO: Implement using range checking approach
    
    return false;
}

/*
 * Helper function with min/max bounds
 */
bool isValidBSTHelper(TreeNode* node, long minVal, long maxVal) {
    // TODO: Implement helper
    
    return false;
}

/*
 * Alternative: Validate using in-order traversal
 * In-order traversal of BST should be strictly increasing
 */
bool isValidBSTInorder(TreeNode* root) {
    // TODO: Implement using in-order traversal
    
    return false;
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



// ============================================================================
// SOLUTION: See solutions/04_validate_bst_solution.cpp
// ============================================================================
