# YugabyteDB - DB Internals Interview Prep

Focused preparation for database internals role covering OS concepts, thread safety, concurrency, and database internals.

---

## Table of Contents

1. [About YugabyteDB](#about-yugabytedb)
2. [OS Fundamentals](#os-fundamentals)
3. [Thread Safety & Synchronization](#thread-safety--synchronization)
4. [Concurrency Patterns](#concurrency-patterns)
5. [Database Internals](#database-internals)
6. [Distributed Systems Concepts](#distributed-systems-concepts)
7. [Common Interview Questions](#common-interview-questions)

---

## About YugabyteDB

YugabyteDB is a **distributed SQL database** built on:
- PostgreSQL-compatible query layer
- Google Spanner-inspired architecture
- Raft consensus for replication
- LSM-tree based storage (RocksDB)

**Key areas they care about:**
- Distributed consensus (Raft/Paxos)
- Storage engines (LSM trees, B-trees)
- Concurrency control (MVCC, locks)
- Query execution and optimization
- Fault tolerance and replication

---

## OS Fundamentals

### Process vs Thread

| Aspect | Process | Thread |
|--------|---------|--------|
| Memory | Separate address space | Shared address space |
| Creation | Expensive (fork) | Lightweight |
| Communication | IPC (pipes, sockets, shared mem) | Direct (shared memory) |
| Crash impact | Isolated | Crashes whole process |
| Context switch | Expensive | Cheaper |

### Memory Layout

```
┌─────────────────┐ High Address
│      Stack      │ ← Local variables, function calls (grows down)
├─────────────────┤
│        ↓        │
│    (free)       │
│        ↑        │
├─────────────────┤
│      Heap       │ ← Dynamic allocation (grows up)
├─────────────────┤
│      BSS        │ ← Uninitialized global/static variables
├─────────────────┤
│      Data       │ ← Initialized global/static variables
├─────────────────┤
│      Text       │ ← Code (read-only)
└─────────────────┘ Low Address
```

### Virtual Memory

- **Page**: Fixed-size block (typically 4KB)
- **Page Table**: Maps virtual addresses to physical addresses
- **TLB**: Cache for page table entries
- **Page Fault**: Access to page not in memory → load from disk

**Why it matters for DBs:**
- Buffer pool management relies on page-based I/O
- mmap() can be used for memory-mapped files
- Understanding page faults helps optimize disk access

### File I/O

```cpp
// Buffered I/O (libc)
FILE* f = fopen("file.txt", "r");
fread(buf, size, count, f);

// Direct I/O (system calls)
int fd = open("file.txt", O_RDONLY | O_DIRECT);
read(fd, buf, size);

// Memory-mapped I/O
void* ptr = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
```

| Type | Buffered (fread) | Direct (read) | mmap |
|------|------------------|---------------|------|
| Caching | User-space buffer | Kernel buffer | Page cache |
| Control | Less | More | Most |
| Use case | General | DB wants control | Read-heavy |

---

## Thread Safety & Synchronization

### The Problem

```cpp
int counter = 0;

void increment() {
    counter++;  // NOT atomic!
    // Actually: load counter → increment → store counter
    // Another thread can interleave between these steps
}
```

### Mutex (Mutual Exclusion)

```cpp
#include <mutex>

mutex mtx;
int counter = 0;

void increment() {
    mtx.lock();
    counter++;
    mtx.unlock();
}

// Better: RAII style
void increment_safe() {
    lock_guard<mutex> lock(mtx);
    counter++;
}  // automatically unlocks
```

### Read-Write Lock

When reads >> writes, allow concurrent reads:

```cpp
#include <shared_mutex>

shared_mutex rw_lock;
int data = 0;

void reader() {
    shared_lock<shared_mutex> lock(rw_lock);  // shared access
    cout << data;  // multiple readers OK
}

void writer() {
    unique_lock<shared_mutex> lock(rw_lock);  // exclusive access
    data++;  // only one writer
}
```

### Condition Variables

Wait for a condition:

```cpp
mutex mtx;
condition_variable cv;
queue<int> work_queue;

// Producer
void producer() {
    lock_guard<mutex> lock(mtx);
    work_queue.push(42);
    cv.notify_one();  // wake up one waiting thread
}

// Consumer
void consumer() {
    unique_lock<mutex> lock(mtx);
    cv.wait(lock, []{ return !work_queue.empty(); });  // wait until queue not empty
    int item = work_queue.front();
    work_queue.pop();
}
```

### Atomic Operations

Lock-free for simple types:

```cpp
#include <atomic>

atomic<int> counter(0);

void increment() {
    counter++;  // atomic, no lock needed
}

// Compare-and-swap
bool cas_increment() {
    int expected = counter.load();
    return counter.compare_exchange_strong(expected, expected + 1);
}
```

### Spinlock vs Mutex

```cpp
// Spinlock - busy wait (good for short critical sections)
class Spinlock {
    atomic_flag flag = ATOMIC_FLAG_INIT;
public:
    void lock() {
        while (flag.test_and_set(memory_order_acquire));  // spin
    }
    void unlock() {
        flag.clear(memory_order_release);
    }
};
```

| Type | Spinlock | Mutex |
|------|----------|-------|
| Wait | Busy-wait (burns CPU) | Sleep (context switch) |
| Best for | Very short critical sections | Longer critical sections |
| Overhead | Low (no syscall) | Higher (syscall) |

### Deadlock

**Four conditions (all must be true):**
1. **Mutual exclusion** - resource held exclusively
2. **Hold and wait** - hold one, wait for another
3. **No preemption** - can't force release
4. **Circular wait** - A waits for B, B waits for A

**Prevention:**
- Lock ordering (always acquire in same order)
- Lock timeout (try_lock with timeout)
- Deadlock detection (cycle detection in wait graph)

```cpp
// BAD - potential deadlock
void thread1() { lock(A); lock(B); }
void thread2() { lock(B); lock(A); }

// GOOD - consistent ordering
void thread1() { lock(A); lock(B); }
void thread2() { lock(A); lock(B); }

// BETTER - use std::lock
void safe() {
    std::lock(A, B);  // locks both atomically, avoids deadlock
    lock_guard<mutex> lockA(A, adopt_lock);
    lock_guard<mutex> lockB(B, adopt_lock);
}
```

---

## Concurrency Patterns

### Thread Pool

Reuse threads instead of creating/destroying:

```cpp
class ThreadPool {
    vector<thread> workers;
    queue<function<void()>> tasks;
    mutex mtx;
    condition_variable cv;
    bool stop = false;
    
public:
    ThreadPool(int num_threads) {
        for (int i = 0; i < num_threads; i++) {
            workers.emplace_back([this] {
                while (true) {
                    function<void()> task;
                    {
                        unique_lock<mutex> lock(mtx);
                        cv.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) return;
                        task = move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }
    
    void submit(function<void()> task) {
        {
            lock_guard<mutex> lock(mtx);
            tasks.push(move(task));
        }
        cv.notify_one();
    }
};
```

### Producer-Consumer (Bounded Buffer)

```cpp
template<typename T>
class BoundedQueue {
    queue<T> buffer;
    int capacity;
    mutex mtx;
    condition_variable not_full, not_empty;
    
public:
    BoundedQueue(int cap) : capacity(cap) {}
    
    void produce(T item) {
        unique_lock<mutex> lock(mtx);
        not_full.wait(lock, [this] { return buffer.size() < capacity; });
        buffer.push(item);
        not_empty.notify_one();
    }
    
    T consume() {
        unique_lock<mutex> lock(mtx);
        not_empty.wait(lock, [this] { return !buffer.empty(); });
        T item = buffer.front();
        buffer.pop();
        not_full.notify_one();
        return item;
    }
};
```

### Reader-Writer Problem

```cpp
class ReadWriteLock {
    shared_mutex rw_mutex;
    
public:
    void read_lock() { rw_mutex.lock_shared(); }
    void read_unlock() { rw_mutex.unlock_shared(); }
    void write_lock() { rw_mutex.lock(); }
    void write_unlock() { rw_mutex.unlock(); }
};
```

### Lock-Free Data Structures

Using CAS (Compare-And-Swap):

```cpp
template<typename T>
class LockFreeStack {
    struct Node {
        T data;
        Node* next;
    };
    atomic<Node*> head;
    
public:
    void push(T data) {
        Node* new_node = new Node{data, nullptr};
        new_node->next = head.load();
        while (!head.compare_exchange_weak(new_node->next, new_node));
    }
    
    bool pop(T& result) {
        Node* old_head = head.load();
        while (old_head && !head.compare_exchange_weak(old_head, old_head->next));
        if (old_head) {
            result = old_head->data;
            delete old_head;
            return true;
        }
        return false;
    }
};
```

---

## Database Internals

### Storage Engines

#### B-Tree (Traditional: PostgreSQL, MySQL InnoDB)

```
                    [30 | 60]                    <- Root
                   /    |    \
           [10|20]   [40|50]   [70|80]           <- Internal
          /  |  \    /  |  \    /  |  \
        [1-9][11-19][21-29]...                   <- Leaf (data)
```

**Properties:**
- Balanced tree, O(log n) lookup
- Good for reads and range queries
- In-place updates
- Write amplification on updates

#### LSM-Tree (YugabyteDB uses RocksDB)

```
┌─────────────────────────────────────────────────┐
│                  MemTable                        │ ← In-memory (writes go here)
│              (sorted, ~64MB)                     │
└─────────────────────────────────────────────────┘
                      ↓ flush
┌─────────────────────────────────────────────────┐
│   L0: SSTable SSTable SSTable                    │ ← Recent flushes
├─────────────────────────────────────────────────┤
│   L1: [    Sorted SSTables    ]                  │ ← Compacted
├─────────────────────────────────────────────────┤
│   L2: [      Larger SSTables        ]            │ ← More compacted
└─────────────────────────────────────────────────┘
```

**Properties:**
- Write-optimized (sequential writes)
- Reads may check multiple levels
- Background compaction merges levels
- Bloom filters speed up lookups

**Read path:** MemTable → L0 → L1 → L2 (uses bloom filters)
**Write path:** Write to MemTable → flush to L0 when full

### Write-Ahead Log (WAL)

```
┌──────────────────────────────────────────────────────┐
│  1. Write to WAL (sequential, durable)               │
│  2. Write to MemTable (in-memory)                    │
│  3. Acknowledge to client                            │
│  4. Later: flush MemTable to disk                    │
└──────────────────────────────────────────────────────┘

Recovery: Replay WAL to rebuild MemTable
```

**Why WAL:**
- Sequential writes are fast
- Durability before acknowledging
- Crash recovery by replaying log

### MVCC (Multi-Version Concurrency Control)

Instead of locking, keep multiple versions:

```
Row: id=1
┌─────────────────────────────────────────────────────┐
│ Version 3: value=30, txn_id=103, created=T3         │
│ Version 2: value=20, txn_id=102, created=T2         │
│ Version 1: value=10, txn_id=101, created=T1         │
└─────────────────────────────────────────────────────┘

Transaction at T2.5 sees Version 2 (value=20)
Transaction at T3.5 sees Version 3 (value=30)
```

**Benefits:**
- Readers don't block writers
- Writers don't block readers
- Snapshot isolation

**Implementation:**
- Each row has: `xmin` (created by txn), `xmax` (deleted by txn)
- Transaction sees rows where `xmin <= my_txn_id` and (`xmax` is null or `xmax > my_txn_id`)

### Transaction Isolation Levels

| Level | Dirty Read | Non-Repeatable Read | Phantom Read |
|-------|------------|---------------------|--------------|
| Read Uncommitted | Yes | Yes | Yes |
| Read Committed | No | Yes | Yes |
| Repeatable Read | No | No | Yes |
| Serializable | No | No | No |

**Dirty Read:** See uncommitted data
**Non-Repeatable Read:** Same query returns different values
**Phantom Read:** Same query returns different rows

### Buffer Pool

```cpp
class BufferPool {
    unordered_map<PageId, Page*> page_table;
    list<PageId> lru_list;  // for eviction
    mutex pool_mutex;
    
public:
    Page* get_page(PageId pid) {
        lock_guard<mutex> lock(pool_mutex);
        if (page_table.count(pid)) {
            // Move to front of LRU
            lru_list.remove(pid);
            lru_list.push_front(pid);
            return page_table[pid];
        }
        // Page fault - load from disk
        if (page_table.size() >= MAX_PAGES) {
            evict_page();
        }
        Page* page = load_from_disk(pid);
        page_table[pid] = page;
        lru_list.push_front(pid);
        return page;
    }
    
    void evict_page() {
        PageId victim = lru_list.back();
        lru_list.pop_back();
        if (page_table[victim]->is_dirty()) {
            flush_to_disk(page_table[victim]);
        }
        delete page_table[victim];
        page_table.erase(victim);
    }
};
```

### Indexing

**B+ Tree Index:**
```sql
CREATE INDEX idx_name ON users(name);
-- Leaf nodes contain: (key, row_pointer)
```

**Hash Index:**
```sql
CREATE INDEX idx_id ON users USING HASH(id);
-- O(1) lookup, no range queries
```

**Covering Index:**
```sql
CREATE INDEX idx_cover ON orders(customer_id) INCLUDE (total, status);
-- Query can be satisfied from index alone
```

---

## Distributed Systems Concepts

### CAP Theorem

Pick 2 of 3:
- **Consistency**: All nodes see same data
- **Availability**: Every request gets a response
- **Partition Tolerance**: System works despite network failures

YugabyteDB: CP system (chooses consistency over availability during partitions)

### Raft Consensus

```
Leader Election:
┌─────────────────────────────────────────────────────────────┐
│  1. Nodes start as Followers                                │
│  2. Timeout → become Candidate → request votes              │
│  3. Majority votes → become Leader                          │
│  4. Leader sends heartbeats to maintain authority           │
└─────────────────────────────────────────────────────────────┘

Log Replication:
┌─────────────────────────────────────────────────────────────┐
│  1. Client sends write to Leader                            │
│  2. Leader appends to log                                   │
│  3. Leader replicates to Followers                          │
│  4. Majority acknowledge → commit                           │
│  5. Leader responds to client                               │
└─────────────────────────────────────────────────────────────┘
```

**Key properties:**
- Leader elected by majority vote
- Log entries committed when majority acknowledges
- Single leader at a time (within a term)

### Sharding

```
┌─────────────────────────────────────────────────────────────┐
│  Hash Sharding:     key → hash(key) % num_shards            │
│  Range Sharding:    key → find shard by key range           │
└─────────────────────────────────────────────────────────────┘

YugabyteDB: Hash sharding by default, automatic splitting
```

### Replication

```
Synchronous Replication (YugabyteDB):
  Client → Leader → Followers (wait for majority) → Ack client

Asynchronous Replication:
  Client → Leader → Ack client → Replicate later
```

---

## Common Interview Questions

### OS & Threading

**Q: What happens when you call malloc()?**
> 1. Check free list for available block
> 2. If not found, request memory from OS (sbrk/mmap)
> 3. OS maps virtual pages
> 4. Physical memory allocated on first access (page fault)

**Q: Explain context switching.**
> 1. Save current process state (registers, PC, stack pointer)
> 2. Save to PCB (Process Control Block)
> 3. Load new process state from its PCB
> 4. Switch page tables
> 5. Resume new process

**Q: How would you implement a thread-safe queue?**
> Use mutex + condition variables. Lock on push/pop. Use condition variable to signal when queue becomes non-empty (for consumers) or non-full (for producers).

**Q: Difference between mutex and semaphore?**
> Mutex: Binary, owned (only owner can unlock), for mutual exclusion
> Semaphore: Counting, no ownership, for signaling and resource counting

### Database Internals

**Q: Why use LSM-tree over B-tree?**
> LSM-tree converts random writes to sequential writes, better for write-heavy workloads. Trade-off is read amplification (may need to check multiple levels).

**Q: Explain write amplification.**
> Data written multiple times due to compaction. Write N bytes → eventually writes M*N bytes to disk. LSM-trees have higher write amplification than B-trees.

**Q: How does MVCC handle concurrent transactions?**
> Each transaction sees a snapshot. Writers create new versions instead of overwriting. Readers see appropriate version based on their start timestamp. No read-write blocking.

**Q: What is write-ahead logging?**
> Write changes to log before applying to data pages. Ensures durability (changes logged before ack) and atomicity (replay or rollback using log).

**Q: How would you implement a simple buffer pool?**
> Hash map (page_id → page) + LRU list for eviction. On page request: check pool, if miss load from disk and possibly evict LRU page. Track dirty pages for write-back.

### Distributed Systems

**Q: Explain Raft leader election.**
> Nodes start as followers with random timeouts. On timeout, become candidate and request votes. First to get majority becomes leader. Leader sends heartbeats to prevent new elections.

**Q: How does YugabyteDB ensure consistency?**
> Uses Raft consensus for each tablet. Writes committed only after majority of replicas acknowledge. Single leader per tablet ensures linearizability.

**Q: What happens during a network partition?**
> Partition with majority continues serving reads/writes. Minority partition cannot commit writes (no majority). When healed, minority syncs from majority.

---

## Quick Reference

### Threading Primitives
| Primitive | Use Case |
|-----------|----------|
| `mutex` | Mutual exclusion |
| `shared_mutex` | Read-write lock |
| `condition_variable` | Wait for condition |
| `atomic<T>` | Lock-free simple types |
| `lock_guard` | RAII mutex wrapper |
| `unique_lock` | Flexible mutex wrapper |

### Storage Comparison
| Aspect | B-Tree | LSM-Tree |
|--------|--------|----------|
| Write | Random I/O | Sequential I/O |
| Read | O(log n) | May check multiple levels |
| Space | More overhead | Compaction reclaims |
| Best for | Read-heavy, OLTP | Write-heavy |

### Isolation Levels Quick Check
| Level | Locks | MVCC |
|-------|-------|------|
| Read Committed | Short read locks | See committed versions |
| Repeatable Read | Long read locks | Snapshot at txn start |
| Serializable | Range locks | + conflict detection |

---

## Study Resources

- **YugabyteDB Docs**: https://docs.yugabyte.com/
- **Raft Paper**: "In Search of an Understandable Consensus Algorithm"
- **LSM-Tree Paper**: "The Log-Structured Merge-Tree"
- **RocksDB Wiki**: https://github.com/facebook/rocksdb/wiki

---

*Good luck with your YugabyteDB interview!*
