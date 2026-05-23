/*
 * SOLUTION: Lowest Common Ancestor of a Binary Tree
 * 
 * Approach: Recursive
 * - If current node is p or q, return it
 * - Recursively search left and right subtrees
 * - If both return non-null, current node is LCA
 * - If only one returns non-null, propagate that result
 * 
 * Time Complexity: O(n)
 * Space Complexity: O(h) for recursion stack
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

// Recursive Solution - O(n) time, O(h) space
TreeNode* lowestCommonAncestor(TreeNode* root, TreeNode* p, TreeNode* q) {
    // Base case: null or found p or q
    if (!root || root == p || root == q) {
        return root;
    }
    
    // Search in left and right subtrees
    TreeNode* left = lowestCommonAncestor(root->left, p, q);
    TreeNode* right = lowestCommonAncestor(root->right, p, q);
    
    // If both left and right are non-null, current node is LCA
    if (left && right) {
        return root;
    }
    
    // Otherwise, return the non-null one (or null if both are null)
    return left ? left : right;
}

// BST-specific Solution - O(h) time, O(1) space iterative
TreeNode* lowestCommonAncestorBST(TreeNode* root, TreeNode* p, TreeNode* q) {
    while (root) {
        if (p->val < root->val && q->val < root->val) {
            // Both in left subtree
            root = root->left;
        } else if (p->val > root->val && q->val > root->val) {
            // Both in right subtree
            root = root->right;
        } else {
            // Split point - this is the LCA
            return root;
        }
    }
    return nullptr;
}

// Iterative Solution using Parent Pointers - O(n) time, O(n) space
TreeNode* lowestCommonAncestorIterative(TreeNode* root, TreeNode* p, TreeNode* q) {
    // Build parent map using BFS
    std::unordered_map<TreeNode*, TreeNode*> parent;
    parent[root] = nullptr;
    
    std::queue<TreeNode*> bfsQueue;
    bfsQueue.push(root);
    
    // BFS until we find both p and q
    while (parent.find(p) == parent.end() || parent.find(q) == parent.end()) {
        TreeNode* node = bfsQueue.front();
        bfsQueue.pop();
        
        if (node->left) {
            parent[node->left] = node;
            bfsQueue.push(node->left);
        }
        if (node->right) {
            parent[node->right] = node;
            bfsQueue.push(node->right);
        }
    }
    
    // Build ancestor set for p
    std::unordered_set<TreeNode*> ancestors;
    TreeNode* curr = p;
    while (curr) {
        ancestors.insert(curr);
        curr = parent[curr];
    }
    
    // Find first ancestor of q that's also ancestor of p
    curr = q;
    while (ancestors.find(curr) == ancestors.end()) {
        curr = parent[curr];
    }
    
    return curr;
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

