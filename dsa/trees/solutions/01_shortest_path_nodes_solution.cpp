/*
 * SOLUTION: Shortest Path Between Two Nodes in Binary Tree
 * 
 * Approach:
 * 1. Find path from root to node1
 * 2. Find path from root to node2
 * 3. Find LCA (last common node in both paths)
 * 4. Path = reverse(path from LCA to node1) + path from LCA to node2
 * 
 * Time Complexity: O(n)
 * Space Complexity: O(h) where h is height of tree
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
    if (values.empty() || values[0] == -1) return nullptr;
    
    TreeNode* root = new TreeNode(values[0]);
    std::vector<TreeNode*> nodes = {root};
    
    size_t i = 1;
    size_t parentIdx = 0;
    
    while (i < values.size()) {
        TreeNode* parent = nodes[parentIdx];
        
        if (i < values.size()) {
            if (values[i] != -1) {
                parent->left = new TreeNode(values[i]);
                nodes.push_back(parent->left);
            }
            i++;
        }
        
        if (i < values.size()) {
            if (values[i] != -1) {
                parent->right = new TreeNode(values[i]);
                nodes.push_back(parent->right);
            }
            i++;
        }
        
        parentIdx++;
    }
    
    return root;
}

void freeTree(TreeNode* root) {
    if (!root) return;
    freeTree(root->left);
    freeTree(root->right);
    delete root;
}

// Helper: Find path from root to target
bool findPath(TreeNode* root, int target, std::vector<int>& path) {
    if (!root) return false;
    
    path.push_back(root->val);
    
    if (root->val == target) {
        return true;
    }
    
    // Search in left and right subtrees
    if (findPath(root->left, target, path) || 
        findPath(root->right, target, path)) {
        return true;
    }
    
    // Target not found in this subtree, backtrack
    path.pop_back();
    return false;
}

std::vector<int> shortestPath(TreeNode* root, int node1, int node2) {
    if (!root) return {};
    
    // Special case: same node
    if (node1 == node2) return {node1};
    
    // Find paths from root to both nodes
    std::vector<int> path1, path2;
    findPath(root, node1, path1);
    findPath(root, node2, path2);
    
    // Find LCA (last common node)
    size_t lcaIdx = 0;
    while (lcaIdx < path1.size() && lcaIdx < path2.size() && 
           path1[lcaIdx] == path2[lcaIdx]) {
        lcaIdx++;
    }
    lcaIdx--;  // Index of LCA
    
    // Build result path:
    // Reverse path from node1 to LCA + path from LCA to node2
    std::vector<int> result;
    
    // Add path from node1 to LCA (reverse order)
    for (int i = path1.size() - 1; i >= static_cast<int>(lcaIdx); i--) {
        result.push_back(path1[i]);
    }
    
    // Add path from LCA to node2 (skip LCA as it's already added)
    for (size_t i = lcaIdx + 1; i < path2.size(); i++) {
        result.push_back(path2[i]);
    }
    
    return result;
}


// ============================================================================
// TEST CASES
// ============================================================================

void runTests() {
    std::cout << "Running Shortest Path Tests...\n" << std::endl;
    
    // Build test tree:
    //        1
    //       / \
    //      2   3
    //     / \   \
    //    4   5   6
    TreeNode* root = buildTree({1, 2, 3, 4, 5, -1, 6});
    
    // Test 1: Nodes in different subtrees
    {
        auto path = shortestPath(root, 4, 6);
        assert(path == std::vector<int>({4, 2, 1, 3, 6}));
        std::cout << "✓ Test 1 passed: Path 4 -> 6" << std::endl;
    }
    
    // Test 2: Nodes in same subtree
    {
        auto path = shortestPath(root, 4, 5);
        assert(path == std::vector<int>({4, 2, 5}));
        std::cout << "✓ Test 2 passed: Path 4 -> 5" << std::endl;
    }
    
    // Test 3: One node is ancestor of other
    {
        auto path = shortestPath(root, 2, 4);
        assert(path == std::vector<int>({2, 4}));
        std::cout << "✓ Test 3 passed: Path 2 -> 4" << std::endl;
    }
    
    // Test 4: Path to root
    {
        auto path = shortestPath(root, 5, 1);
        assert(path == std::vector<int>({5, 2, 1}));
        std::cout << "✓ Test 4 passed: Path 5 -> 1" << std::endl;
    }
    
    // Test 5: Same node
    {
        auto path = shortestPath(root, 3, 3);
        assert(path == std::vector<int>({3}));
        std::cout << "✓ Test 5 passed: Same node" << std::endl;
    }
    
    // Test 6: Root to leaf
    {
        auto path = shortestPath(root, 1, 6);
        assert(path == std::vector<int>({1, 3, 6}));
        std::cout << "✓ Test 6 passed: Root to leaf" << std::endl;
    }
    
    freeTree(root);
    
    std::cout << "\n=== All tests passed! ===" << std::endl;
}

int main() {
    runTests();
    return 0;
}

