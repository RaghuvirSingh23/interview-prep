/*
 * SOLUTION: Serialize and Deserialize Binary Tree
 * 
 * Approach: Preorder DFS with null markers
 * - Serialize: preorder traversal, use "null" for null nodes
 * - Deserialize: process tokens in same order
 * 
 * Time Complexity: O(n)
 * Space Complexity: O(n)
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

bool isSameTree(TreeNode* p, TreeNode* q) {
    if (!p && !q) return true;
    if (!p || !q) return false;
    return p->val == q->val && 
           isSameTree(p->left, q->left) && 
           isSameTree(p->right, q->right);
}

// Approach 1: Preorder DFS
class Codec {
public:
    std::string serialize(TreeNode* root) {
        std::ostringstream out;
        serializeHelper(root, out);
        return out.str();
    }
    
    TreeNode* deserialize(std::string data) {
        std::istringstream in(data);
        std::queue<std::string> nodes;
        std::string token;
        
        while (std::getline(in, token, ',')) {
            nodes.push(token);
        }
        
        return deserializeHelper(nodes);
    }
    
private:
    void serializeHelper(TreeNode* node, std::ostringstream& out) {
        if (!node) {
            out << "null,";
            return;
        }
        
        out << node->val << ",";
        serializeHelper(node->left, out);
        serializeHelper(node->right, out);
    }
    
    TreeNode* deserializeHelper(std::queue<std::string>& nodes) {
        if (nodes.empty()) return nullptr;
        
        std::string val = nodes.front();
        nodes.pop();
        
        if (val == "null") {
            return nullptr;
        }
        
        TreeNode* node = new TreeNode(std::stoi(val));
        node->left = deserializeHelper(nodes);
        node->right = deserializeHelper(nodes);
        
        return node;
    }
};

// Approach 2: BFS Level Order
class CodecBFS {
public:
    std::string serialize(TreeNode* root) {
        if (!root) return "";
        
        std::ostringstream out;
        std::queue<TreeNode*> q;
        q.push(root);
        
        while (!q.empty()) {
            TreeNode* node = q.front();
            q.pop();
            
            if (node) {
                out << node->val << ",";
                q.push(node->left);
                q.push(node->right);
            } else {
                out << "null,";
            }
        }
        
        return out.str();
    }
    
    TreeNode* deserialize(std::string data) {
        if (data.empty()) return nullptr;
        
        std::istringstream in(data);
        std::string token;
        std::getline(in, token, ',');
        
        TreeNode* root = new TreeNode(std::stoi(token));
        std::queue<TreeNode*> q;
        q.push(root);
        
        while (!q.empty()) {
            TreeNode* node = q.front();
            q.pop();
            
            // Left child
            if (std::getline(in, token, ',')) {
                if (token != "null") {
                    node->left = new TreeNode(std::stoi(token));
                    q.push(node->left);
                }
            }
            
            // Right child
            if (std::getline(in, token, ',')) {
                if (token != "null") {
                    node->right = new TreeNode(std::stoi(token));
                    q.push(node->right);
                }
            }
        }
        
        return root;
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

