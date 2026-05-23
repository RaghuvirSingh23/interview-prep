/*
 * ============================================================================
 * Problem: Lowest Common Ancestor of a Binary Tree
 * ============================================================================
 * Difficulty: Medium
 * Source: LeetCode 236
 * Time: 25-30 minutes
 * 
 * DESCRIPTION:
 * ------------
 * Given a binary tree, find the lowest common ancestor (LCA) of two given 
 * nodes in the tree.
 * 
 * The lowest common ancestor is defined as the lowest node in T that has 
 * both p and q as descendants (where we allow a node to be a descendant 
 * of itself).
 * 
 * EXAMPLE 1:
 * ----------
 *           3
 *          / \
 *         5   1
 *        / \ / \
 *       6  2 0  8
 *         / \
 *        7   4
 *   
 *   Input:  root, p = 5, q = 1
 *   Output: 3
 *   Explanation: LCA of 5 and 1 is 3
 * 
 * EXAMPLE 2:
 * ----------
 *   Input:  root, p = 5, q = 4
 *   Output: 5
 *   Explanation: LCA of 5 and 4 is 5 (node can be ancestor of itself)
 * 
 * CONSTRAINTS:
 * ------------
 * - The number of nodes in the tree is in the range [2, 10^5]
 * - All Node.val are unique
 * - p != q
 * - p and q exist in the tree
 * 
 * HINTS:
 * ------
 * 1. Use recursion: if current node is p or q, return it
 * 2. Recursively search left and right subtrees
 * 3. If both return non-null, current node is LCA
 * 4. If only one returns non-null, propagate that result up
 * 
 * VARIATIONS:
 * -----------
 * - LCA in BST (easier - use BST property)
 * - LCA with parent pointers (different approach)
 */

#include <iostream>
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <cassert>

struct TreeNode {
    int val;
    TreeNode* left;
    TreeNode* right;
    TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
};

TreeNode* buildTree(const std::vector<int>& values) {
    if (values.empty() || values[0] == -1) return nullptr;
    
    TreeNode* root = new TreeNode(values[0]);
    std::queue<TreeNode*> q;
    q.push(root);
    
    size_t i = 1;
    while (i < values.size() && !q.empty()) {
        TreeNode* node = q.front();
        q.pop();
        
        if (i < values.size() && values[i] != -1) {
            node->left = new TreeNode(values[i]);
            q.push(node->left);
        }
        i++;
        
        if (i < values.size() && values[i] != -1) {
            node->right = new TreeNode(values[i]);
            q.push(node->right);
        }
        i++;
    }
    
    return root;
}

TreeNode* findNode(TreeNode* root, int val) {
    if (!root) return nullptr;
    if (root->val == val) return root;
    TreeNode* left = findNode(root->left, val);
    if (left) return left;
    return findNode(root->right, val);
}

void freeTree(TreeNode* root) {
    if (!root) return;
    freeTree(root->left);
    freeTree(root->right);
    delete root;
}

/*
 * Find the Lowest Common Ancestor of two nodes
 * 
 * @param root: Root of the binary tree
 * @param p: First node
 * @param q: Second node
 * @return: LCA node
 */
TreeNode* lowestCommonAncestor(TreeNode* root, TreeNode* p, TreeNode* q) {
    // TODO: Implement recursive solution
    
    return nullptr;
}

/*
 * BONUS: LCA for Binary Search Tree (use BST property)
 */
TreeNode* lowestCommonAncestorBST(TreeNode* root, TreeNode* p, TreeNode* q) {
    // TODO: Implement BST-specific solution
    
    return nullptr;
}

/*
 * BONUS: LCA using parent pointers approach
 * (Useful when nodes have parent pointers or you build parent map)
 */
TreeNode* lowestCommonAncestorIterative(TreeNode* root, TreeNode* p, TreeNode* q) {
    // TODO: Implement iterative solution using parent pointers
    
    return nullptr;
}


// ============================================================================
// TEST CASES
// ============================================================================

void runTests() {
    std::cout << "Running LCA Tests...\n" << std::endl;
    
    // Build test tree:
    //        3
    //       / \
    //      5   1
    //     / \ / \
    //    6  2 0  8
    //      / \
    //     7   4
    TreeNode* root = buildTree({3, 5, 1, 6, 2, 0, 8, -1, -1, 7, 4});
    
    // Test 1: Nodes in different subtrees
    {
        TreeNode* p = findNode(root, 5);
        TreeNode* q = findNode(root, 1);
        TreeNode* lca = lowestCommonAncestor(root, p, q);
        assert(lca && lca->val == 3);
        std::cout << "✓ Test 1 passed: LCA(5, 1) = 3" << std::endl;
    }
    
    // Test 2: One node is ancestor of other
    {
        TreeNode* p = findNode(root, 5);
        TreeNode* q = findNode(root, 4);
        TreeNode* lca = lowestCommonAncestor(root, p, q);
        assert(lca && lca->val == 5);
        std::cout << "✓ Test 2 passed: LCA(5, 4) = 5" << std::endl;
    }
    
    // Test 3: Nodes in same subtree
    {
        TreeNode* p = findNode(root, 6);
        TreeNode* q = findNode(root, 4);
        TreeNode* lca = lowestCommonAncestor(root, p, q);
        assert(lca && lca->val == 5);
        std::cout << "✓ Test 3 passed: LCA(6, 4) = 5" << std::endl;
    }
    
    // Test 4: Leaf nodes
    {
        TreeNode* p = findNode(root, 7);
        TreeNode* q = findNode(root, 4);
        TreeNode* lca = lowestCommonAncestor(root, p, q);
        assert(lca && lca->val == 2);
        std::cout << "✓ Test 4 passed: LCA(7, 4) = 2" << std::endl;
    }
    
    // Test 5: Root is LCA
    {
        TreeNode* p = findNode(root, 6);
        TreeNode* q = findNode(root, 8);
        TreeNode* lca = lowestCommonAncestor(root, p, q);
        assert(lca && lca->val == 3);
        std::cout << "✓ Test 5 passed: LCA(6, 8) = 3" << std::endl;
    }
    
    freeTree(root);
    
    // Test 6: BST LCA
    {
        //     6
        //    / \
        //   2   8
        //  / \ / \
        // 0  4 7  9
        TreeNode* bstRoot = buildTree({6, 2, 8, 0, 4, 7, 9});
        TreeNode* p = findNode(bstRoot, 2);
        TreeNode* q = findNode(bstRoot, 8);
        TreeNode* lca = lowestCommonAncestorBST(bstRoot, p, q);
        assert(lca && lca->val == 6);
        freeTree(bstRoot);
        std::cout << "✓ Test 6 passed: BST LCA(2, 8) = 6" << std::endl;
    }
    
    std::cout << "\n=== All tests passed! ===" << std::endl;
}

int main() {
    runTests();
    return 0;
}



// ============================================================================
// SOLUTION: See solutions/03_lowest_common_ancestor_solution.cpp
// ============================================================================
