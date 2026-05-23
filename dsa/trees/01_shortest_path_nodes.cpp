/*
 * ============================================================================
 * Problem: Shortest Path Between Two Nodes in Binary Tree
 * ============================================================================
 * Difficulty: Medium
 * Source: NextHop.AI Interview Guidelines
 * Time: 30-40 minutes
 * 
 * DESCRIPTION:
 * ------------
 * Given a binary tree and two nodes, find the shortest path between them.
 * The path should be returned as a list of node values from the first node
 * to the second node.
 * 
 * EXAMPLE 1:
 * ----------
 *           1
 *          / \
 *         2   3
 *        / \   \
 *       4   5   6
 *   
 *   Input:  root, node1 = 4, node2 = 6
 *   Output: [4, 2, 1, 3, 6]
 *   Path:   4 -> 2 -> 1 -> 3 -> 6
 * 
 * EXAMPLE 2:
 * ----------
 *   Input:  root, node1 = 4, node2 = 5
 *   Output: [4, 2, 5]
 *   Path:   4 -> 2 -> 5
 * 
 * CONSTRAINTS:
 * ------------
 * - The number of nodes in the tree is in the range [2, 10^4]
 * - All node values are unique
 * - Both nodes exist in the tree
 * 
 * HINTS:
 * ------
 * 1. First find the Lowest Common Ancestor (LCA) of the two nodes
 * 2. Find path from LCA to node1 and path from LCA to node2
 * 3. Combine: reverse(path to node1) + path to node2
 * 4. Alternatively, find path from root to each node, then find divergence point
 * 
 * APPROACH:
 * ---------
 * 1. Find path from root to node1 and root to node2
 * 2. Find the LCA (last common node in both paths)
 * 3. Path = reverse(path from LCA to node1) + path from LCA to node2
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>

struct TreeNode {
    int val;
    TreeNode* left;
    TreeNode* right;
    TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
};

// Helper to build tree from level order (using -1 for null)
TreeNode* buildTree(const std::vector<int>& values) {
    if (values.empty() || values[0] == -1) return nullptr;
    
    TreeNode* root = new TreeNode(values[0]);
    std::vector<TreeNode*> nodes = {root};
    
    size_t i = 1;
    size_t parentIdx = 0;
    
    while (i < values.size()) {
        TreeNode* parent = nodes[parentIdx];
        
        // Left child
        if (i < values.size()) {
            if (values[i] != -1) {
                parent->left = new TreeNode(values[i]);
                nodes.push_back(parent->left);
            }
            i++;
        }
        
        // Right child
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

// Helper to free tree
void freeTree(TreeNode* root) {
    if (!root) return;
    freeTree(root->left);
    freeTree(root->right);
    delete root;
}

/*
 * Find the shortest path between two nodes in a binary tree
 * 
 * @param root: Root of the binary tree
 * @param node1: Value of first node
 * @param node2: Value of second node
 * @return: Vector of node values representing the path
 */
std::vector<int> shortestPath(TreeNode* root, int node1, int node2) {
    // TODO: Implement your solution
    
    return {};
}

/*
 * Helper: Find path from root to target node
 * 
 * @param root: Current node
 * @param target: Target value to find
 * @param path: Current path (output parameter)
 * @return: true if target found in subtree
 */
bool findPath(TreeNode* root, int target, std::vector<int>& path) {
    // TODO: Implement helper function
    
    return false;
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



// ============================================================================
// SOLUTION: See solutions/01_shortest_path_nodes_solution.cpp
// ============================================================================
