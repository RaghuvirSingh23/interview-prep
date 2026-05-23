/*
 * ============================================================================
 * Problem: Serialize and Deserialize Binary Tree
 * ============================================================================
 * Difficulty: Hard
 * Source: LeetCode 297
 * Time: 35-45 minutes
 * 
 * DESCRIPTION:
 * ------------
 * Design an algorithm to serialize and deserialize a binary tree. There is 
 * no restriction on how your serialization/deserialization algorithm should 
 * work. You just need to ensure that a binary tree can be serialized to a 
 * string and this string can be deserialized to the original tree structure.
 * 
 * Clarification: The input/output format is the same as how LeetCode 
 * serializes a binary tree. You do not necessarily need to follow this 
 * format, so please be creative and come up with different approaches.
 * 
 * EXAMPLE 1:
 * ----------
 *       1
 *      / \
 *     2   3
 *        / \
 *       4   5
 *   
 *   serialize:   "1,2,3,null,null,4,5"
 *   deserialize: reconstructs the original tree
 * 
 * EXAMPLE 2:
 * ----------
 *   Input:  root = []
 *   Output: ""
 * 
 * CONSTRAINTS:
 * ------------
 * - The number of nodes in the tree is in the range [0, 10^4]
 * - -1000 <= Node.val <= 1000
 * 
 * HINTS:
 * ------
 * 1. Use preorder traversal for serialization (root, left, right)
 * 2. Use "null" or special marker for null nodes
 * 3. For deserialization, use a queue or index to track position
 * 4. BFS (level-order) also works well
 * 
 * APPROACHES:
 * -----------
 * 1. Preorder DFS with null markers
 * 2. Level-order BFS
 * 3. Preorder + Inorder (without null markers, but more complex)
 */

#include <iostream>
#include <string>
#include <sstream>
#include <queue>
#include <cassert>

struct TreeNode {
    int val;
    TreeNode* left;
    TreeNode* right;
    TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
};

void freeTree(TreeNode* root) {
    if (!root) return;
    freeTree(root->left);
    freeTree(root->right);
    delete root;
}

// Helper to check if two trees are identical
bool isSameTree(TreeNode* p, TreeNode* q) {
    if (!p && !q) return true;
    if (!p || !q) return false;
    return p->val == q->val && 
           isSameTree(p->left, q->left) && 
           isSameTree(p->right, q->right);
}

class Codec {
public:
    /*
     * Serialize a binary tree to a string
     * 
     * @param root: Root of the binary tree
     * @return: String representation of the tree
     */
    std::string serialize(TreeNode* root) {
        // TODO: Implement serialization
        // Suggestion: Use preorder traversal with "null" for null nodes
        // Format: "1,2,null,null,3,4,null,null,5,null,null"
        
        return "";
    }
    
    /*
     * Deserialize a string to a binary tree
     * 
     * @param data: String representation of the tree
     * @return: Root of the reconstructed binary tree
     */
    TreeNode* deserialize(std::string data) {
        // TODO: Implement deserialization
        
        return nullptr;
    }
    
private:
    // Helper functions
    
    // Serialize helper (preorder)
    void serializeHelper(TreeNode* node, std::ostringstream& out) {
        // TODO: Implement
    }
    
    // Deserialize helper (preorder)
    TreeNode* deserializeHelper(std::queue<std::string>& nodes) {
        // TODO: Implement
        return nullptr;
    }
};

// Alternative: BFS-based Codec
class CodecBFS {
public:
    std::string serialize(TreeNode* root) {
        // TODO: Implement BFS serialization
        return "";
    }
    
    TreeNode* deserialize(std::string data) {
        // TODO: Implement BFS deserialization
        return nullptr;
    }
};


// ============================================================================
// TEST CASES
// ============================================================================

void runTests() {
    std::cout << "Running Serialize/Deserialize Tests...\n" << std::endl;
    
    Codec codec;
    
    // Test 1: Normal tree
    {
        //     1
        //    / \
        //   2   3
        //      / \
        //     4   5
        TreeNode* root = new TreeNode(1);
        root->left = new TreeNode(2);
        root->right = new TreeNode(3);
        root->right->left = new TreeNode(4);
        root->right->right = new TreeNode(5);
        
        std::string serialized = codec.serialize(root);
        std::cout << "Serialized: " << serialized << std::endl;
        
        TreeNode* deserialized = codec.deserialize(serialized);
        assert(isSameTree(root, deserialized));
        
        freeTree(root);
        freeTree(deserialized);
        std::cout << "✓ Test 1 passed: Normal tree" << std::endl;
    }
    
    // Test 2: Empty tree
    {
        std::string serialized = codec.serialize(nullptr);
        TreeNode* deserialized = codec.deserialize(serialized);
        assert(deserialized == nullptr);
        std::cout << "✓ Test 2 passed: Empty tree" << std::endl;
    }
    
    // Test 3: Single node
    {
        TreeNode* root = new TreeNode(42);
        std::string serialized = codec.serialize(root);
        TreeNode* deserialized = codec.deserialize(serialized);
        assert(isSameTree(root, deserialized));
        freeTree(root);
        freeTree(deserialized);
        std::cout << "✓ Test 3 passed: Single node" << std::endl;
    }
    
    // Test 4: Left-skewed tree
    {
        TreeNode* root = new TreeNode(1);
        root->left = new TreeNode(2);
        root->left->left = new TreeNode(3);
        
        std::string serialized = codec.serialize(root);
        TreeNode* deserialized = codec.deserialize(serialized);
        assert(isSameTree(root, deserialized));
        freeTree(root);
        freeTree(deserialized);
        std::cout << "✓ Test 4 passed: Left-skewed tree" << std::endl;
    }
    
    // Test 5: Right-skewed tree
    {
        TreeNode* root = new TreeNode(1);
        root->right = new TreeNode(2);
        root->right->right = new TreeNode(3);
        
        std::string serialized = codec.serialize(root);
        TreeNode* deserialized = codec.deserialize(serialized);
        assert(isSameTree(root, deserialized));
        freeTree(root);
        freeTree(deserialized);
        std::cout << "✓ Test 5 passed: Right-skewed tree" << std::endl;
    }
    
    // Test 6: Negative values
    {
        TreeNode* root = new TreeNode(-1);
        root->left = new TreeNode(-2);
        root->right = new TreeNode(-3);
        
        std::string serialized = codec.serialize(root);
        TreeNode* deserialized = codec.deserialize(serialized);
        assert(isSameTree(root, deserialized));
        freeTree(root);
        freeTree(deserialized);
        std::cout << "✓ Test 6 passed: Negative values" << std::endl;
    }
    
    std::cout << "\n=== All tests passed! ===" << std::endl;
}

int main() {
    runTests();
    return 0;
}



// ============================================================================
// SOLUTION: See solutions/05_serialize_deserialize_solution.cpp
// ============================================================================
