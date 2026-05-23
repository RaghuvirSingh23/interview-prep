#include <iostream>
#include <string>
#include <unordered_map>
#include <list>
using namespace std;

/*

Design an LRU (Least Recently Used) Cache that supports:

- Fixed capacity (set at construction)
- get(key) → returns value if key exists, -1 otherwise. Marks key as recently used.
- put(key, value) → inserts or updates the key-value pair. Marks as recently used.
  If cache is at capacity, evict the LEAST recently used item before inserting.
- Both get and put must be O(1) time.

Example flow:
  LRUCache cache(3);         // capacity 3

  cache.put(1, 10);          // cache: {1:10}
  cache.put(2, 20);          // cache: {1:10, 2:20}
  cache.put(3, 30);          // cache: {1:10, 2:20, 3:30}  — full
  cache.get(1);              // returns 10, now 1 is most recent
  cache.put(4, 40);          // evicts key 2 (least recent), cache: {1:10, 3:30, 4:40}
  cache.get(2);              // returns -1 (evicted)
  cache.get(3);              // returns 30
  cache.put(5, 50);          // evicts key 1? or 4? Think about it.

Constraints:
- O(1) for both get and put
- Think about which data structures give you:
  - O(1) lookup by key
  - O(1) insertion/removal at both ends
  - O(1) move-to-front

Hint: You need TWO data structures working together.

*/

struct Node {
    int key, value;
    Node* prev;
    Node* next;
    Node(int k, int v) : key(k), value(v), prev(nullptr), next(nullptr) {}
};

class LRUCache {
    int capacity;
    int size;
    Node* head;   // dummy head (most recent side)
    Node* tail;   // dummy tail (least recent side)
    unordered_map<int, Node*> map;

    void remove(Node* node) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    void insertAfterHead(Node* node) {
        node->next = head->next;
        node->prev = head;
        head->next->prev = node;
        head->next = node;
    }

    void moveToFront(Node* node) {
        remove(node);
        insertAfterHead(node);
    }

public:
    LRUCache(int cap) : capacity(cap), size(0) {
        head = new Node(0, 0);
        tail = new Node(0, 0);
        head->next = tail;
        tail->prev = head;
    }

    ~LRUCache() {
        Node* curr = head;
        while (curr) {
            Node* next = curr->next;
            delete curr;
            curr = next;
        }
    }

    int get(int key) {
        auto it = map.find(key);
        if (it == map.end()) return -1;

        moveToFront(it->second);
        return it->second->value;
    }

    void put(int key, int value) {
        auto it = map.find(key);

        if (it != map.end()) {
            it->second->value = value;
            moveToFront(it->second);
            return;
        }

        if (size == capacity) {
            Node* lru = tail->prev;
            remove(lru);
            map.erase(lru->key);
            delete lru;
            size--;
        }

        Node* node = new Node(key, value);
        insertAfterHead(node);
        map[key] = node;
        size++;
    }
};

// ──────────────── Tests ────────────────

int tests_passed = 0;
int tests_failed = 0;

void check(bool cond, const string& name) {
    if (cond) {
        cout << "  PASS: " << name << endl;
        tests_passed++;
    } else {
        cout << "  FAIL: " << name << endl;
        tests_failed++;
    }
}

void test_basic_get_put() {
    LRUCache c(2);
    c.put(1, 10);
    c.put(2, 20);
    check(c.get(1) == 10, "get existing key 1");
    check(c.get(2) == 20, "get existing key 2");
    check(c.get(3) == -1, "get missing key");
}

void test_eviction() {
    LRUCache c(3);
    c.put(1, 10);
    c.put(2, 20);
    c.put(3, 30);
    c.get(1);           // makes 1 most recent, order: 1, 3, 2
    c.put(4, 40);       // evicts 2 (least recent)
    check(c.get(2) == -1, "key 2 evicted");
    check(c.get(1) == 10, "key 1 survived");
    check(c.get(3) == 30, "key 3 survived");
    check(c.get(4) == 40, "key 4 inserted");
}

void test_update_existing() {
    LRUCache c(2);
    c.put(1, 10);
    c.put(2, 20);
    c.put(1, 100);      // update key 1, makes it most recent
    c.put(3, 30);        // evicts 2 (not 1, since 1 was just touched)
    check(c.get(1) == 100, "key 1 updated to 100");
    check(c.get(2) == -1, "key 2 evicted after key 1 update");
    check(c.get(3) == 30, "key 3 present");
}

void test_capacity_one() {
    LRUCache c(1);
    c.put(1, 10);
    check(c.get(1) == 10, "cap=1 get after put");
    c.put(2, 20);
    check(c.get(1) == -1, "cap=1 key 1 evicted");
    check(c.get(2) == 20, "cap=1 key 2 present");
}

void test_get_promotes() {
    LRUCache c(3);
    c.put(1, 10);
    c.put(2, 20);
    c.put(3, 30);       // order: 3, 2, 1
    c.get(1);            // order: 1, 3, 2
    c.get(2);            // order: 2, 1, 3
    c.put(4, 40);        // evicts 3
    check(c.get(3) == -1, "key 3 evicted (was least recent)");
    check(c.get(1) == 10, "key 1 survived");
    check(c.get(2) == 20, "key 2 survived");
}

int main() {
    cout << "[LRU Cache Tests]" << endl;
    test_basic_get_put();
    test_eviction();
    test_update_existing();
    test_capacity_one();
    test_get_promotes();

    cout << "\n=============================" << endl;
    cout << "Results: " << tests_passed << " passed, " << tests_failed << " failed" << endl;
    return tests_failed > 0 ? 1 : 0;
}
