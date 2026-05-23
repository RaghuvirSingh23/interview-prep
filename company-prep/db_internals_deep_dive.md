# Database Internals - Deep Dive

Comprehensive guide for DB internals interviews. Covers everything from how data hits disk to distributed consensus.

---

## Table of Contents

1. [How Data is Stored on Disk](#1-how-data-is-stored-on-disk)
2. [Storage Engines](#2-storage-engines)
3. [Indexing Deep Dive](#3-indexing-deep-dive)
4. [Buffer Pool & Memory Management](#4-buffer-pool--memory-management)
5. [Write-Ahead Logging & Recovery](#5-write-ahead-logging--recovery)
6. [Concurrency Control](#6-concurrency-control)
7. [Transaction Management](#7-transaction-management)
8. [Query Processing & Optimization](#8-query-processing--optimization)
9. [Distributed Databases](#9-distributed-databases)
10. [Consensus Protocols](#10-consensus-protocols)
11. [Distributed Transactions](#11-distributed-transactions)
12. [YugabyteDB Architecture](#12-yugabytedb-architecture)
13. [Interview Questions](#13-interview-questions)

---

## 1. How Data is Stored on Disk

### Disk Basics

```
HDD:
  - Seek time: ~5-10ms (move head to track)
  - Rotational latency: ~2-5ms (wait for sector)
  - Transfer: ~100-200 MB/s
  - Random read: ~100 IOPS

SSD:
  - No seek/rotation
  - Random read: ~100,000 IOPS
  - Sequential: ~500 MB/s (SATA), ~3000 MB/s (NVMe)
  - Write amplification is a concern
```

**Key insight:** Random I/O is expensive. Databases are designed to minimize random I/O and favor sequential access.

### Pages

The fundamental unit of storage in a database is a **page** (typically 4KB-16KB).

```
┌──────────────────────────────────────────────────────────────┐
│                        PAGE (8KB typical)                      │
├──────────────────────────────────────────────────────────────┤
│  Page Header                                                  │
│  ┌──────────────────────────────────────────────────────────┐│
│  │ page_id | lsn | checksum | free_space_offset | num_tuples││
│  └──────────────────────────────────────────────────────────┘│
├──────────────────────────────────────────────────────────────┤
│  Line Pointers (slot array)                                   │
│  ┌─────┬─────┬─────┬─────┐                                  │
│  │ ptr1│ ptr2│ ptr3│ ptr4│ → points to tuple locations       │
│  └─────┴─────┴─────┴─────┘                                  │
├──────────────────────────────────────────────────────────────┤
│                                                               │
│  Free Space                                                   │
│                                                               │
├──────────────────────────────────────────────────────────────┤
│  Tuples (actual row data, grows from bottom)                  │
│  ┌──────────────────────────────────────────────────────────┐│
│  │ Tuple 4 | Tuple 3 | Tuple 2 | Tuple 1                   ││
│  └──────────────────────────────────────────────────────────┘│
└──────────────────────────────────────────────────────────────┘
```

**Why pages?**

- Disk I/O operates in blocks, not individual bytes
- Amortize I/O cost: read one page = read many rows
- Pages fit into buffer pool for caching
- Uniform size simplifies memory management

### Page Layout: Slotted Pages

- **Line pointers** grow from top → down
- **Tuples** grow from bottom → up
- When they meet, page is full
- Deletions leave "holes" → need compaction

### Tuple Layout

```
┌──────────────────────────────────────────────────────┐
│                    TUPLE HEADER                        │
│  xmin (creating txn) | xmax (deleting txn) |          │
│  ctid (physical location) | null bitmap | flags       │
├──────────────────────────────────────────────────────┤
│                    TUPLE DATA                          │
│  col1_value | col2_value | col3_value | ...           │
└──────────────────────────────────────────────────────┘
```

- `xmin`: Transaction ID that created this tuple
- `xmax`: Transaction ID that deleted/updated this tuple (0 if alive)
- `ctid`: Physical location (page_id, slot_number)
- Used for MVCC visibility checks

### Row Store vs Column Store

```
ROW STORE (OLTP - PostgreSQL, MySQL):
  Page 1: [row1: id,name,age,email] [row2: id,name,age,email] ...
  Good for: SELECT * FROM users WHERE id = 5
  Bad for:  SELECT AVG(age) FROM users

COLUMN STORE (OLAP - ClickHouse, Redshift):
  Page 1 (id col):    [1, 2, 3, 4, 5, ...]
  Page 2 (name col):  [alice, bob, charlie, ...]
  Page 3 (age col):   [25, 30, 35, ...]
  Good for: SELECT AVG(age) FROM users  (only reads age column)
  Bad for:  SELECT * FROM users WHERE id = 5 (needs to read all columns)
```


| Aspect        | Row Store                | Column Store                   |
| ------------- | ------------------------ | ------------------------------ |
| Best for      | OLTP, point queries      | OLAP, aggregations             |
| Compression   | Low                      | High (similar values together) |
| Insert        | Fast (one write)         | Slow (write to each column)    |
| Full row read | Fast                     | Slow                           |
| Aggregation   | Slow (reads entire rows) | Fast (reads one column)        |


---

## 2. Storage Engines

### B-Tree Storage Engine

Used by: PostgreSQL, MySQL InnoDB, SQL Server

**How it works:**

- Data stored in B+ tree structure
- Leaf nodes contain actual data (or pointers to data)
- Internal nodes contain keys and child pointers
- All leaves at same depth → balanced → O(log n) lookup

```
                         [50]                          ← Root
                        /    \
                  [20|30]    [70|80]                    ← Internal
                 /   |  \    /  |  \
              [10] [25] [35][60][75][90]                ← Leaf (data pages)
               ↕    ↕    ↕   ↕   ↕   ↕
              prev ←→ next (leaves are doubly linked)
```

**Key properties:**

- Leaf nodes linked for range scans
- Fan-out is high (100s of children) → tree is shallow
- 4-level tree with fan-out 100 = 100M keys
- In-place updates (overwrite existing pages)

**Write path:**

```
1. Find correct leaf page
2. If space available → insert into page
3. If page full → SPLIT:
   - Create new page
   - Move half the entries
   - Add pointer in parent
   - If parent full → split propagates up
```

**Read path:**

```
1. Start at root
2. Binary search within node → find correct child pointer
3. Follow pointer to next level
4. Repeat until leaf
5. Binary search in leaf for key
```

### LSM-Tree Storage Engine

Used by: RocksDB, LevelDB, Cassandra, YugabyteDB (via RocksDB)

**Core idea:** Convert random writes into sequential writes.

```
WRITE PATH:
                                                    
  Write → ┌──────────┐    flush     ┌──────────────────────┐
          │ MemTable │  ──────────→ │  L0: SSTable files   │
          │ (sorted) │              └──────────────────────┘
          └──────────┘                       │ compaction
                                             ▼
                                  ┌──────────────────────┐
                                  │  L1: Sorted SSTables │
                                  └──────────────────────┘
                                             │ compaction
                                             ▼
                                  ┌──────────────────────┐
                                  │  L2: Larger SSTables │
                                  └──────────────────────┘
```

**Components in detail:**

**MemTable (in-memory):**

- Usually a skip list or red-black tree
- All writes go here first (after WAL)
- Sorted by key
- When full (e.g., 64MB) → flush to disk as SSTable
- Immutable MemTable created during flush, new MemTable for writes

**SSTable (Sorted String Table):**

```
┌─────────────────────────────────────────────────┐
│  Data Block 1: [key1:val1] [key2:val2] ...      │
├─────────────────────────────────────────────────┤
│  Data Block 2: [key5:val5] [key6:val6] ...      │
├─────────────────────────────────────────────────┤
│  ...                                             │
├─────────────────────────────────────────────────┤
│  Index Block: [key1→block1] [key5→block2] ...   │
├─────────────────────────────────────────────────┤
│  Bloom Filter Block                              │
├─────────────────────────────────────────────────┤
│  Footer: offsets to index and bloom filter       │
└─────────────────────────────────────────────────┘
```

- Immutable once written
- Sorted by key
- Contains index block for binary search
- Bloom filter for quick "key exists?" check

**Compaction:**

```
Level Compaction (RocksDB default):
  L0: overlapping SSTables (direct flush from MemTable)
  L1: non-overlapping, size ~10x L0
  L2: non-overlapping, size ~10x L1
  
  Compaction: merge L(n) files with overlapping L(n+1) files
  - Read overlapping files from both levels
  - Merge sort
  - Write new files to L(n+1)
  - Delete old files

Size-Tiered Compaction (Cassandra):
  - Group SSTables of similar size
  - Merge them into larger SSTable
  - Simpler but more space amplification
```

**Read path:**

```
1. Check MemTable
2. Check Immutable MemTable (if flushing)
3. Check L0 SSTables (all of them - they overlap)
4. For L1+: binary search to find correct SSTable, then search within
5. Use Bloom filters to skip SSTables that don't contain the key
```

**Amplification trade-offs:**


| Type                    | Definition                                   | B-Tree                | LSM-Tree                     |
| ----------------------- | -------------------------------------------- | --------------------- | ---------------------------- |
| **Write amplification** | Bytes written to disk / bytes written by app | ~2-3x (page split)    | ~10-30x (compaction)         |
| **Read amplification**  | Disk reads per query                         | 1 (direct lookup)     | Multiple (check levels)      |
| **Space amplification** | Disk used / actual data size                 | ~1.5x (fragmentation) | ~1.1x (compaction cleans up) |


### B-Tree vs LSM-Tree Summary


| Aspect        | B-Tree                 | LSM-Tree                       |
| ------------- | ---------------------- | ------------------------------ |
| Write pattern | Random I/O             | Sequential I/O                 |
| Read pattern  | O(log n), single path  | May check multiple levels      |
| Write speed   | Slower (random)        | Faster (sequential)            |
| Read speed    | Faster (single lookup) | Slower (multiple levels)       |
| Space usage   | Fragmentation          | Compaction overhead            |
| Concurrency   | Page-level locking     | MemTable + immutable SSTables  |
| Best for      | Read-heavy OLTP        | Write-heavy workloads          |
| Used by       | PostgreSQL, MySQL      | RocksDB, Cassandra, YugabyteDB |


---

## 3. Indexing Deep Dive

### B+ Tree Index

The most common index type. Different from B-Tree:

```
B-Tree:  Data in ALL nodes (internal + leaf)
B+ Tree: Data ONLY in leaf nodes, internal nodes are just signposts

                    B+ Tree Index on 'age'

                         [30 | 60]                    ← Internal (keys only)
                        /    |    \
                  [10|20]  [40|50]  [70|80]           ← Internal (keys only)
                 /  |  \   /  |  \   /  |  \
               [5,8,10] [15,20,25] [35,40,45] ...    ← Leaf (key + row pointer)
                  ↔         ↔          ↔               ← Doubly linked
```

**Why B+ Tree over B-Tree?**

- All data in leaves → internal nodes hold more keys → tree is shallower
- Leaf linked list → fast range scans
- Predictable performance (always traverse to leaf)

**Fan-out calculation:**

```
Page size: 8KB
Key size: 8 bytes, Pointer size: 8 bytes
Fan-out = 8192 / (8 + 8) = 512 children per node

Depth 1: 512 keys
Depth 2: 512 * 512 = 262,144 keys
Depth 3: 512^3 = ~134 million keys
Depth 4: 512^4 = ~68 billion keys

Most lookups: 3-4 disk reads (root usually cached)
```

### Hash Index

```
key → hash(key) → bucket → scan bucket for exact key

Bucket 0: [(key5, val5), (key12, val12)]
Bucket 1: [(key1, val1)]
Bucket 2: [(key3, val3), (key7, val7), (key9, val9)]
```


| Aspect        | B+ Tree            | Hash                 |
| ------------- | ------------------ | -------------------- |
| Point lookup  | O(log n)           | O(1) average         |
| Range query   | Fast (leaf scan)   | NOT supported        |
| Sorted output | Yes                | No                   |
| Disk-friendly | Yes (page-aligned) | Less (random access) |


### Bloom Filters (Critical for LSM-Trees)

Probabilistic data structure: "Is key in this SSTable?"

- **Yes** → key MIGHT be there (false positive possible)
- **No** → key is DEFINITELY NOT there

```
Insert "hello":
  hash1("hello") = 3  → set bit 3
  hash2("hello") = 7  → set bit 7
  hash3("hello") = 1  → set bit 1

Bit array: [0,1,0,1,0,0,0,1,0,0]
                ^   ^           ^

Check "world":
  hash1("world") = 3  → bit 3 is 1 ✓
  hash2("world") = 5  → bit 5 is 0 ✗ → DEFINITELY NOT HERE
```

**In LSM-Tree:**

- Each SSTable has a bloom filter
- Before reading SSTable from disk, check bloom filter
- If negative → skip this SSTable entirely
- Saves potentially thousands of disk reads

**Tuning:**

- More bits per key → lower false positive rate
- 10 bits/key → ~1% false positive rate
- Trade-off: memory usage vs read performance

### Clustered vs Non-Clustered Index

```
CLUSTERED (table sorted by index key):
  Index leaf → actual data rows (data IS the index leaf)
  Only ONE per table (data can only be sorted one way)
  Range scan = sequential disk read

  Leaf: [id=1, name="Alice", age=25] → [id=2, name="Bob", age=30] → ...

NON-CLUSTERED (separate structure pointing to data):
  Index leaf → pointer to data page
  Multiple per table
  Range scan = potentially random disk reads

  Index leaf: [age=25 → page5,slot3] → [age=30 → page2,slot1] → ...
```

### Covering Index

```sql
-- Query:
SELECT name, age FROM users WHERE age > 25;

-- Regular index on (age):
  1. Search index for age > 25 → get row pointers
  2. Go to table pages to fetch name → RANDOM I/O

-- Covering index on (age) INCLUDE (name):
  1. Search index for age > 25 → name is IN the index
  2. No need to visit table → INDEX-ONLY SCAN
```

### Partial Index

```sql
-- Only index active users
CREATE INDEX idx_active ON users(email) WHERE active = true;

-- Smaller index, faster lookups for common query pattern
```

### Multi-Column Index

```sql
CREATE INDEX idx_multi ON orders(customer_id, order_date);

-- Can use for:
WHERE customer_id = 5                         ✓ (prefix)
WHERE customer_id = 5 AND order_date > '2024' ✓ (full)
WHERE order_date > '2024'                     ✗ (not a prefix)
```

**Leftmost prefix rule:** Index on (A, B, C) supports queries on (A), (A, B), (A, B, C), but NOT (B), (C), or (B, C).

---

## 4. Buffer Pool & Memory Management

### What is the Buffer Pool?

The buffer pool is an in-memory cache of disk pages. It's the **most critical component** for performance.

```
┌─────────────────────────────────────────────────────────────┐
│                    APPLICATION                                │
│                   (SQL queries)                               │
└─────────────────┬───────────────────────────────────────────┘
                  │ request page
                  ▼
┌─────────────────────────────────────────────────────────────┐
│                    BUFFER POOL                                │
│                                                               │
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐             │
│  │Page 5│ │Page 2│ │Page 8│ │Page 1│ │Page 3│  (in memory) │
│  │dirty │ │clean │ │dirty │ │clean │ │clean │             │
│  └──────┘ └──────┘ └──────┘ └──────┘ └──────┘             │
│                                                               │
│  Page Table: { page_id → frame_id, dirty_bit, pin_count }   │
│  Free List:  [frame_6, frame_7, ...]                        │
└─────────────────┬───────────────────────────────────────────┘
                  │ page fault (not in pool)
                  ▼
┌─────────────────────────────────────────────────────────────┐
│                    DISK                                       │
│  [Page 1][Page 2][Page 3][Page 4][Page 5][Page 6]...        │
└─────────────────────────────────────────────────────────────┘
```

### Page Table (not OS page table)

```cpp
struct FrameInfo {
    page_id_t page_id;
    bool dirty;        // modified since read from disk?
    int pin_count;     // number of threads using this page
    // eviction metadata (varies by policy)
};

// Hash map for O(1) lookup
unordered_map<page_id_t, frame_id_t> page_table;
```

### Pin/Unpin Protocol

```
1. Thread wants page 5:
   - buffer_pool.fetch_page(5)
   - If in pool → pin_count++ → return pointer
   - If not → load from disk → put in frame → pin_count = 1

2. Thread done with page 5:
   - buffer_pool.unpin_page(5, is_dirty)
   - pin_count--
   - If modified, set dirty = true

3. Eviction (need free frame):
   - Find page with pin_count == 0
   - If dirty → flush to disk first
   - Remove from page table
   - Frame is now free
```

**CRITICAL:** Never evict a pinned page. Pin count prevents eviction while page is in use.

### Eviction Policies

**LRU (Least Recently Used):**

```
Access order: P1, P2, P3, P1, P4 (pool size = 3)

[P1] → [P1, P2] → [P1, P2, P3] → [P2, P3, P1] → evict P2 → [P3, P1, P4]
```

Problem: Sequential scan floods cache (scan all pages, never reuse)

**Clock (Approximation of LRU):**

```
Pages in circular buffer with reference bit:

     ┌─→ P1(1) → P2(0) → P3(1) → P4(0) ─┐
     └────────────────────────────────────┘
                    ^ hand

Eviction:
  - Hand moves clockwise
  - If ref bit = 1 → set to 0, move on (second chance)
  - If ref bit = 0 → evict this page
  - On access → set ref bit = 1
```

**LRU-K (used by SQL Server):**

- Track last K accesses for each page
- Evict page with oldest K-th access
- Better than LRU: distinguishes frequently vs recently accessed

---

## 5. Write-Ahead Logging & Recovery

### WAL Basics

**Rule:** Write the log record BEFORE writing the data page to disk.

```
┌─────────────────────────────────────────────────────────────┐
│  1. Transaction starts                                       │
│  2. Modify page in buffer pool (in-memory)                  │
│  3. Write LOG RECORD to WAL (on disk) ← MUST happen first  │
│  4. Acknowledge to client                                    │
│  5. Eventually flush dirty page to disk (lazy)              │
└─────────────────────────────────────────────────────────────┘
```

**Why?** If crash after step 4, we can replay WAL to recover.

### WAL Record Format

```
┌──────────────────────────────────────────────────────────────┐
│  LSN | txn_id | prev_lsn | type | page_id | offset |        │
│  before_image | after_image                                  │
└──────────────────────────────────────────────────────────────┘

LSN: Log Sequence Number (monotonically increasing)
prev_lsn: Previous LSN for this transaction (linked list)
type: INSERT, UPDATE, DELETE, COMMIT, ABORT, CHECKPOINT
before_image: Old value (for undo)
after_image: New value (for redo)
```

### ARIES Recovery Algorithm

The gold standard for crash recovery (used by most databases).

**Three phases:**

```
                    WAL
  ──────────────────────────────────────────→
  ...  checkpoint  ...  crash
         ↓                 ↓
  PHASE 1: ANALYSIS (scan forward from checkpoint)
  → Build dirty page table (which pages were dirty)
  → Build active transaction table (which txns were running)

  PHASE 2: REDO (scan forward from earliest dirty page LSN)
  → Replay ALL logged actions
  → Brings database to exact pre-crash state
  → Even replay actions of uncommitted transactions

  PHASE 3: UNDO (scan backward)
  → Undo all uncommitted transactions
  → Use before_image to restore old values
  → Write CLR (Compensation Log Record) for each undo
```

**Why redo then undo?**

- REDO restores the exact state at crash time
- UNDO rolls back incomplete transactions
- CLRs (written during undo) ensure undo is idempotent if we crash again during recovery

### Checkpointing

Periodically save state to reduce recovery time:

**Fuzzy Checkpoint (used in practice):**

```
1. Write BEGIN_CHECKPOINT to WAL
2. Record dirty page table and active txns
3. Write END_CHECKPOINT to WAL
4. DON'T flush dirty pages (that's what makes it "fuzzy")
5. Update master record to point to this checkpoint
```

Recovery starts from checkpoint instead of beginning of WAL → faster recovery.

### Steal vs No-Steal, Force vs No-Force

```
STEAL:    Can flush uncommitted dirty pages to disk
NO-STEAL: Cannot flush uncommitted pages (simpler, but limits buffer pool)

FORCE:    Flush ALL dirty pages on commit (durable, but slow commits)
NO-FORCE: Don't flush on commit (fast commits, rely on WAL for durability)

Most systems: STEAL + NO-FORCE (best performance, needs ARIES for recovery)
```


| Policy       | Steal                     | No-Steal                  |
| ------------ | ------------------------- | ------------------------- |
| **Force**    | Needs undo                | Simple (no undo, no redo) |
| **No-Force** | Needs undo + redo (ARIES) | Needs redo only           |


---

## 6. Concurrency Control

### Why Concurrency Control?

Without it, concurrent transactions cause anomalies:

**Lost Update:**

```
T1: READ(X) = 100
T2: READ(X) = 100
T1: WRITE(X = 100 + 10)  → X = 110
T2: WRITE(X = 100 + 20)  → X = 120  (T1's update LOST)
```

**Dirty Read:**

```
T1: WRITE(X = 200)
T2: READ(X) = 200    ← reads uncommitted data
T1: ABORT             ← T2 has phantom value
```

**Non-Repeatable Read:**

```
T1: READ(X) = 100
T2: WRITE(X = 200), COMMIT
T1: READ(X) = 200    ← different value in same transaction!
```

**Phantom Read:**

```
T1: SELECT * WHERE age > 20  → returns 5 rows
T2: INSERT (age = 25), COMMIT
T1: SELECT * WHERE age > 20  → returns 6 rows!
```

### Two-Phase Locking (2PL)

```
GROWING PHASE:   Acquire locks, never release
SHRINKING PHASE: Release locks, never acquire

     locks
      held
       │
       │     /\
       │    /  \
       │   /    \
       │  /      \
       │ /        \
       └───────────────→ time
         grow  shrink

Lock types:
  - Shared (S): Multiple readers OK
  - Exclusive (X): Single writer only

Compatibility:
       │ S │ X │
    ───┼───┼───┤
    S  │ ✓ │ ✗ │
    ───┼───┼───┤
    X  │ ✗ │ ✗ │
```

**Strict 2PL:** Hold ALL locks until commit/abort → prevents cascading aborts.

**Problem:** Deadlocks are possible. Solutions:

- Deadlock detection (waits-for graph, detect cycles)
- Deadlock prevention (wait-die, wound-wait)
- Lock timeout

### MVCC (Multi-Version Concurrency Control)

Used by: PostgreSQL, MySQL InnoDB, YugabyteDB

**Core idea:** Don't lock. Keep multiple versions. Readers see a consistent snapshot.

```
Row: id=1

T100 writes value=10:
  ┌────────────────────────────────────┐
  │ xmin=100 | xmax=∞ | value=10      │ ← Version 1
  └────────────────────────────────────┘

T200 updates to value=20:
  ┌────────────────────────────────────┐
  │ xmin=100 | xmax=200 | value=10    │ ← Version 1 (now "deleted" by T200)
  └────────────────────────────────────┘
  ┌────────────────────────────────────┐
  │ xmin=200 | xmax=∞ | value=20      │ ← Version 2
  └────────────────────────────────────┘

T150 (started before T200) reads → sees value=10 (Version 1)
T250 (started after T200 committed) reads → sees value=20 (Version 2)
```

**Visibility rules (simplified):**

```
A tuple is visible to transaction T if:
  1. xmin is committed AND xmin < T's snapshot
  2. xmax is uncommitted OR xmax > T's snapshot

In other words: "Was created before my snapshot, not deleted before my snapshot"
```

**Garbage collection (VACUUM in PostgreSQL):**

- Old versions that no active transaction can see → dead tuples
- VACUUM reclaims space from dead tuples
- Without vacuum → table bloat

### MVCC Implementations

**PostgreSQL approach (multi-version in heap):**

- Old and new versions stored in same table
- Requires VACUUM to clean up
- HOT (Heap-Only Tuples) optimization for index-free updates

**MySQL InnoDB approach (undo log):**

- Latest version in table
- Old versions in undo log (rollback segment)
- Undo log is purged when no longer needed

**Append-only (CockroachDB, YugabyteDB):**

- Versions stored with timestamps in LSM-tree
- Compaction cleans up old versions
- Natural for LSM-tree based storage

### Optimistic vs Pessimistic Concurrency

```
PESSIMISTIC (2PL):
  - Lock before accessing
  - Block if can't get lock
  - Good when conflicts are common

OPTIMISTIC (OCC):
  1. READ PHASE: Read freely, track read/write sets
  2. VALIDATE PHASE: Check if any conflicts occurred
  3. WRITE PHASE: If no conflicts → commit, else → abort and retry
  
  Good when conflicts are rare
```

---

## 7. Transaction Management

### ACID Properties

```
ATOMICITY:    All or nothing. If any part fails, entire txn rolls back.
              Implemented via: WAL (undo log)

CONSISTENCY:  Transaction takes DB from one valid state to another.
              Implemented via: constraints, triggers

ISOLATION:    Concurrent txns don't interfere with each other.
              Implemented via: MVCC, 2PL

DURABILITY:   Once committed, data survives crashes.
              Implemented via: WAL (redo log), fsync
```

### Isolation Levels Deep Dive

```
READ UNCOMMITTED:
  - No read locks, no MVCC snapshot
  - Can see uncommitted changes (dirty reads)
  - Almost never used

READ COMMITTED (PostgreSQL default):
  - Each STATEMENT sees latest committed data
  - Snapshot refreshed per statement
  - No dirty reads, but non-repeatable reads possible

REPEATABLE READ (MySQL InnoDB default):
  - Snapshot taken at TRANSACTION start
  - Same query always returns same rows
  - No dirty/non-repeatable reads, but phantoms possible (in theory)
  - InnoDB actually prevents phantoms with gap locks

SERIALIZABLE:
  - Transactions execute as if serial (one after another)
  - PostgreSQL: uses SSI (Serializable Snapshot Isolation)
  - InnoDB: uses gap locks + next-key locks
  - Highest safety, lowest concurrency
```

### Serializable Snapshot Isolation (SSI)

Used by PostgreSQL for SERIALIZABLE level:

```
1. Each transaction gets MVCC snapshot (like Repeatable Read)
2. Track read dependencies between concurrent transactions
3. Detect "dangerous structures" (potential serialization anomalies)
4. Abort one transaction if anomaly detected

Advantage: No blocking! Only aborts when actual conflict detected.
Better than 2PL (which blocks) and better than plain snapshot (which misses anomalies)
```

---

## 8. Query Processing & Optimization

### Query Execution Pipeline 

```
SQL Query
    │
    ▼
┌──────────────┐
│   PARSER     │ → SQL string → Abstract Syntax Tree (AST)
└──────────────┘
    │
    ▼
┌──────────────┐
│   BINDER     │ → Resolve table/column names, check types
└──────────────┘
    │
    ▼
┌──────────────┐
│  OPTIMIZER   │ → Choose best execution plan
└──────────────┘
    │
    ▼
┌──────────────┐
│  EXECUTOR    │ → Run the plan, return results
└──────────────┘
```

### Query Optimizer

**Cost-based optimization:** Estimate cost of different plans, pick cheapest.

```sql
SELECT * FROM orders o JOIN customers c ON o.cust_id = c.id WHERE c.country = 'US';
```

**Plan 1:** Scan all orders, join with customers, filter country

```
Filter(country='US')
  └─ NestedLoopJoin(o.cust_id = c.id)
      ├─ SeqScan(orders)       -- 1M rows
      └─ SeqScan(customers)    -- 100K rows
Cost: 1M × 100K = 100 BILLION comparisons
```

**Plan 2:** Filter customers first, then join

```
NestedLoopJoin(o.cust_id = c.id)
  ├─ SeqScan(orders)             -- 1M rows
  └─ Filter(country='US')
      └─ SeqScan(customers)      -- 10K rows (after filter)
Cost: 1M × 10K = 10 BILLION comparisons
```

**Plan 3:** Use index + hash join

```
HashJoin(o.cust_id = c.id)
  ├─ SeqScan(orders)             -- 1M rows
  └─ IndexScan(customers, country='US')  -- 10K rows
Cost: 10K (build hash) + 1M (probe) = ~1M operations
```

### Join Algorithms

**Nested Loop Join:** O(M × N)

```
for each row r in R:
    for each row s in S:
        if r.key == s.key: emit(r, s)
```

**Hash Join:** O(M + N)

```
Build phase: hash smaller table into memory hash table
Probe phase: scan larger table, probe hash table for matches
```

**Sort-Merge Join:** O(M log M + N log N)

```
Sort both tables on join key
Merge: walk through both sorted tables simultaneously
Good when data is already sorted (index)
```

### Statistics and Cost Estimation

```
The optimizer needs to know:
  - Table size (number of rows)
  - Column statistics (min, max, distinct values, histogram)
  - Index availability and selectivity
  - I/O cost model (sequential vs random)

Selectivity estimation:
  WHERE age = 25    → 1/num_distinct_values
  WHERE age > 25    → (max - 25) / (max - min)
  WHERE age > 25 AND city = 'NYC' → multiply selectivities (assumes independence)
```

---

## 9. Distributed Databases

### Why Distribute?

```
Single node limits:
  - Storage: one disk/machine
  - Throughput: one CPU
  - Availability: single point of failure

Distribution strategies:
  1. REPLICATION: Copy data to multiple nodes (availability + read scaling)
  2. PARTITIONING/SHARDING: Split data across nodes (write scaling + storage)
  3. Usually BOTH: partition data, replicate each partition
```

### Replication

**Single-leader replication:**

```
  Client writes → Leader → Follower 1
                        → Follower 2
                        → Follower 3

  Client reads  → Any node (leader or follower)
```

**Synchronous vs Asynchronous:**

```
Synchronous:
  Write → Leader → wait for ALL followers → ack client
  + Strong consistency
  - Slow (wait for slowest follower)
  - One slow node blocks everything

Semi-synchronous:
  Write → Leader → wait for MAJORITY → ack client
  + Good consistency
  + Tolerates minority failures
  (This is what Raft does)

Asynchronous:
  Write → Leader → ack client → replicate later
  + Fast
  - May lose data on leader crash
  - Followers may serve stale reads
```

**Multi-leader replication:**

```
  Leader 1 ←──→ Leader 2 ←──→ Leader 3
  
  Problem: Conflicts! Both leaders update same row.
  Resolution: Last-write-wins, merge, custom logic
  Used by: CockroachDB (for geo-distributed writes)
```

### Partitioning (Sharding)

**Hash partitioning:**

```
partition = hash(key) % num_partitions

  key="alice" → hash=7 → partition 7 % 3 = 1
  key="bob"   → hash=4 → partition 4 % 3 = 1
  key="carol" → hash=2 → partition 2 % 3 = 2

+ Even distribution
- No range queries across partitions
- Resharding is painful (all data moves)
```

**Consistent hashing (improvement):**

```
     0 ─── Node A ─── Node B ─── Node C ─── 0
     │                                        │
     └────────────── hash ring ───────────────┘

  key hashed to position on ring → assigned to next node clockwise
  Adding/removing node only affects neighbors
```

**Range partitioning:**

```
  Partition 1: keys A-M
  Partition 2: keys N-Z

+ Range queries within partition are fast
- Hot spots (if data skewed, e.g., all new users start with 'A')
- Need to split/merge partitions as data grows
```

**YugabyteDB:** Uses hash sharding by default, supports range sharding. Automatic tablet splitting when tablets grow too large.

---

## 10. Consensus Protocols

### The Problem

Multiple nodes need to agree on a value, even if some nodes crash.

### Raft Consensus (Used by YugabyteDB)

**Roles:**

```
LEADER:    Handles all writes, replicates to followers
FOLLOWER:  Receives replicated log entries, responds to leader
CANDIDATE: Temporarily during leader election
```

**Leader Election:**

```
1. All nodes start as FOLLOWERS with random election timeout
2. If follower doesn't hear from leader → timeout → becomes CANDIDATE
3. Candidate increments TERM, votes for self, requests votes from others
4. Each node votes at most ONCE per term
5. Majority votes → becomes LEADER
6. Leader sends periodic heartbeats to prevent new elections

Split vote: If no majority → timeout → new election with higher term
Random timeouts make split votes unlikely
```

**Log Replication:**

```
Client write request → Leader

Leader:
  1. Append entry to local log
  2. Send AppendEntries RPC to all followers
  3. Followers append to their logs, respond with success
  4. Once MAJORITY acknowledges → entry is COMMITTED
  5. Leader applies to state machine
  6. Responds to client
  7. Followers learn about commit on next heartbeat

  Leader log:    [1:set x=1] [2:set y=2] [3:set x=3]  (committed: 1,2,3)
  Follower A:    [1:set x=1] [2:set y=2] [3:set x=3]  (committed: 1,2)
  Follower B:    [1:set x=1] [2:set y=2]               (committed: 1,2)
```

**Safety guarantees:**

- Election safety: at most one leader per term
- Leader append-only: never overwrites log entries
- Log matching: if two logs have entry with same index and term, all preceding entries are identical
- Leader completeness: if entry committed in term T, present in all future leaders

**Raft vs Paxos:**


| Aspect            | Raft                                      | Paxos                        |
| ----------------- | ----------------------------------------- | ---------------------------- |
| Understandability | Designed to be understandable             | Notoriously difficult        |
| Leader            | Strong leader (all writes through leader) | Can be leaderless            |
| Phases            | 2 phases (election + replication)         | 2 phases (prepare + accept)  |
| Used by           | YugabyteDB, etcd, CockroachDB             | Google Spanner (Multi-Paxos) |


---

## 11. Distributed Transactions

### The Problem

Transaction spans multiple partitions/nodes:

```
Transfer $100 from Account A (Node 1) to Account B (Node 2)
  Node 1: A = A - 100
  Node 2: B = B + 100
  
Both MUST succeed or both MUST fail.
```

### Two-Phase Commit (2PC)

```
PHASE 1: PREPARE
  Coordinator → "Can you commit?" → Participant 1
  Coordinator → "Can you commit?" → Participant 2
  
  Each participant:
    - Acquires locks
    - Writes WAL
    - Responds YES or NO

PHASE 2: COMMIT (if all said YES)
  Coordinator → "COMMIT" → Participant 1
  Coordinator → "COMMIT" → Participant 2
  
  OR ABORT (if any said NO)
  Coordinator → "ABORT" → all participants
```

**Problems with 2PC:**

- **Blocking:** If coordinator crashes after PREPARE, participants are stuck holding locks
- **Latency:** Two network round trips
- **Single point of failure:** Coordinator

### Three-Phase Commit (3PC)

Adds a PRE-COMMIT phase to reduce blocking. Rarely used in practice.

### Distributed MVCC + 2PC (YugabyteDB approach)

```
1. Transaction starts, gets timestamp
2. Reads use MVCC snapshot (no locks for reads)
3. Writes buffered until commit
4. On commit:
   a. Provisional writes to each involved tablet
   b. 2PC: prepare on all tablets, then commit
   c. Clean up provisional writes → committed writes
5. Conflict detection: if two txns write same key, one aborts
```

### Clock Synchronization

**The problem:** In distributed systems, "what happened first?"

**Lamport Clocks (Logical):**

```
Each node maintains counter
On event: counter++
On send: attach counter
On receive: counter = max(local, received) + 1

Gives total ordering but NOT real-time ordering
```

**Vector Clocks:**

```
Each node maintains vector of counters [N1, N2, N3]
Can detect concurrent events (neither happened before the other)
```

**Hybrid Logical Clock (HLC) - Used by YugabyteDB:**

```
Combines physical time + logical counter
  - Physical part: wall clock (NTP synchronized)
  - Logical part: counter for ordering within same physical time
  
Benefits:
  - Close to real time (meaningful timestamps)
  - Always increases (no going backward)
  - No need for tight clock synchronization (unlike Spanner's TrueTime)
```

**Google Spanner TrueTime:**

```
Uses GPS + atomic clocks
Returns time interval [earliest, latest]
Wait out uncertainty before committing
Guarantees real-time ordering but adds latency
```

---

## 12. YugabyteDB Architecture

### High-Level Architecture

```
┌────────────────────────────────────────────────────────────────┐
│                      YugabyteDB Cluster                       │
│                                                                │
│  ┌────────────────────────────────────────────────────────────┐│
│  │                  YB-TServer (per node)                     ││
│  │  ┌────────────────┐  ┌────────────────────────────────┐    ││
│  │  │ YSQL (SQL API) │  │ YCQL (Cassandra-compatible)    │    ││
│  │  │ (PostgreSQL    │  │                                │    ││
│  │  │  compatible)   │  │                                │    ││
│  │  └───────┬────────┘  └──────────┬─────────────────────┘    ││
│  │          │                       │                         ││
│  │          └───────────┬───────────┘                         ││
│  │                      ▼                                     ││
│  │  ┌────────────────────────────────────────────────────┐    ││
│  │  │              DocDB (Document Store)                │    ││
│  │  │  ┌──────────┐ ┌──────────┐ ┌──────────┐            │    ││
│  │  │  │ Tablet 1 │ │ Tablet 2 │ │ Tablet 3 │            │    ││
│  │  │  │(RocksDB) │ │(RocksDB) │ │(RocksDB) │            │    ││
│  │  │  └──────────┘ └──────────┘ └──────────┘            │    ││
│  │  │              Raft Consensus                        │    ││
│  │  └────────────────────────────────────────────────────┘    ││
│  └────────────────────────────────────────────────────────────┘│
│                                                                │
│  ┌────────────────────────────────────────────────────────────┐│
│  │                  YB-Master (cluster metadata)              ││
│  │  - Tablet assignments                                      ││
│  │  - Schema changes (DDL)                                    ││
│  │  - Load balancing                                          ││
│  │  - Also uses Raft for HA                                   ││
│  └────────────────────────────────────────────────────────────┘│
└────────────────────────────────────────────────────────────────┘
```

### Key Components

**YB-TServer (Tablet Server):**

- Handles read/write requests
- Hosts tablets (data partitions)
- Each tablet is a Raft group
- One TServer per physical node

**YB-Master:**

- Cluster metadata management
- Tablet-to-TServer assignment
- DDL operations (CREATE TABLE, etc.)
- Load balancing (move tablets between TServers)
- Runs as Raft group for HA (3 or 5 masters)

**DocDB (Storage Layer):**

- Document-oriented storage on top of RocksDB
- Each tablet = one RocksDB instance
- Keys encoded as: `(table_id, partition_key, sort_key, column, timestamp)`
- Supports both SQL and document semantics

### Tablet Architecture

```
TABLE: users (id INT PRIMARY KEY, name TEXT, age INT)
Hash partitioned into 3 tablets:

Tablet 1: hash(id) in [0x0000, 0x5555]
Tablet 2: hash(id) in [0x5556, 0xAAAA]  
Tablet 3: hash(id) in [0xAAAB, 0xFFFF]

Each tablet replicated via Raft (RF=3):
  Tablet 1: Node A (LEADER), Node B (FOLLOWER), Node C (FOLLOWER)
  Tablet 2: Node B (LEADER), Node C (FOLLOWER), Node A (FOLLOWER)
  Tablet 3: Node C (LEADER), Node A (FOLLOWER), Node B (FOLLOWER)
  
Leaders distributed across nodes → balanced load
```

### Read/Write Path in YugabyteDB

**Write path:**

```
1. Client sends write to YB-TServer
2. TServer determines target tablet (hash of primary key)
3. If local TServer is tablet leader → proceed
   If not → forward to correct leader
4. Leader:
   a. Write to Raft log (WAL)
   b. Replicate to followers via Raft
   c. Wait for majority acknowledgment
   d. Apply to local RocksDB (DocDB)
   e. Respond to client
```

**Read path (strong consistency):**

```
1. Client sends read to TServer
2. Route to tablet leader
3. Leader checks: am I still the leader? (lease check)
4. Read from local RocksDB with MVCC timestamp
5. Return to client

Read from follower (for follower reads):
  - May serve stale data
  - Useful for read scaling when staleness is acceptable
```

### MVCC in YugabyteDB

```
Key encoding in RocksDB:

Key: (hash, range_key, column_id, hybrid_timestamp)
Value: column_value

Example:
  (0xAB12, "user_1", "name", T=100) → "Alice"
  (0xAB12, "user_1", "name", T=200) → "Bob"     ← updated at T=200
  (0xAB12, "user_1", "age",  T=100) → 25

Transaction at T=150 sees: name="Alice", age=25
Transaction at T=250 sees: name="Bob", age=25

Old versions cleaned up by compaction when no active reader needs them.
```

### Automatic Tablet Splitting

```
When tablet grows too large (e.g., >10GB):
  1. Choose split key (middle of key range)
  2. Create two new tablets
  3. Raft group splits into two groups
  4. Update YB-Master metadata
  5. Transparent to application
```

---

## 13. Interview Questions

### Storage Engine Questions

**Q: Explain the difference between B-Tree and LSM-Tree. When would you choose each?**

> B-Tree: in-place updates, good for read-heavy OLTP. O(log n) reads with single path.
> LSM-Tree: sequential writes, good for write-heavy. Reads may check multiple levels.
> Choose B-Tree for read-heavy (PostgreSQL), LSM for write-heavy or when write throughput matters (YugabyteDB, Cassandra).

**Q: What is compaction in LSM-Trees? Why is it needed?**

> Over time, keys accumulate across multiple SSTables at different levels. Compaction merges SSTables: combines entries for same key (keeping latest), removes deleted entries (tombstones), and produces sorted, non-overlapping files at the next level. Without it, reads degrade (more files to check) and space grows (dead entries not reclaimed).

**Q: What are Bloom filters and why are they important for LSM-Trees?**

> Probabilistic structure that answers "is key in this SSTable?" with no false negatives. Each SSTable has one. Before reading from disk, check bloom filter. If negative, skip that SSTable. Critical because LSM reads must potentially check many SSTables - bloom filters eliminate most unnecessary reads.

### Concurrency Questions

**Q: Explain MVCC. How does it differ from locking?**

> MVCC keeps multiple versions of each row, tagged with transaction IDs. Readers see a consistent snapshot without acquiring locks. Writers create new versions. Readers never block writers, writers never block readers. Locking (2PL) blocks conflicting operations. MVCC has overhead of version storage and garbage collection.

**Q: What is a write skew anomaly?**

> Under snapshot isolation, two transactions read the same data, make decisions based on it, and write to different rows. Neither sees the other's write. Example: Two doctors both check "at least one doctor on call", both decide to go off call → nobody on call. Prevented by serializable isolation.

**Q: How do you handle deadlocks in a database?**

> Detection: Build waits-for graph, detect cycles, abort one transaction.
> Prevention: Lock ordering, wait-die/wound-wait schemes.
> Timeout: Abort if waiting too long.
> Most databases use detection (periodic cycle check) + timeout.

### Distributed Systems Questions

**Q: Explain the Raft consensus algorithm.**

> Leader-based consensus. One leader per term handles all writes. Leader replicates log entries to followers. Entry committed when majority acknowledges. Leader election via random timeouts and majority voting. Guarantees safety (never disagree on committed entries) and liveness (eventually makes progress if majority alive).

**Q: What happens during a network partition in YugabyteDB?**

> Each tablet uses Raft with replication factor 3. During partition: majority side elects leader, continues serving reads/writes. Minority side cannot commit (no majority). When partition heals, minority syncs from majority's log. No data loss for committed transactions.

**Q: How does YugabyteDB handle distributed transactions?**

> Uses 2PC with MVCC. Transaction gets hybrid logical timestamp. Reads use MVCC snapshot (no locks). On commit: provisional writes sent to involved tablets, then 2PC (prepare → commit). Conflict detection: if two txns write same key, later one aborts. Clock skew handled by Hybrid Logical Clocks.

**Q: Explain the difference between synchronous and asynchronous replication.**

> Synchronous: wait for replica acknowledgment before responding to client. Strong consistency but higher latency. Asynchronous: respond immediately, replicate in background. Lower latency but may lose data on leader crash. Semi-synchronous (Raft): wait for majority → good balance of consistency and availability.

### Recovery Questions

**Q: What is Write-Ahead Logging and why is it essential?**

> Log changes before applying to data pages. Ensures durability (log is on disk before acknowledging) and atomicity (can redo committed and undo uncommitted after crash). Sequential log writes are fast. Recovery replays log to bring database to consistent state.

**Q: Explain the ARIES recovery algorithm.**

> Three phases: (1) Analysis - scan log forward from checkpoint, identify dirty pages and active transactions. (2) Redo - replay all logged operations to restore crash-time state. (3) Undo - rollback uncommitted transactions using before-images. CLR records written during undo ensure idempotency.

**Q: Why doesn't YugabyteDB need traditional VACUUM like PostgreSQL?**

> YugabyteDB uses RocksDB (LSM-tree). Old versions are naturally cleaned up during compaction. Compaction merges SSTables and discards versions no longer visible to any active transaction. PostgreSQL stores all versions in heap, requiring explicit VACUUM to reclaim dead tuples.

