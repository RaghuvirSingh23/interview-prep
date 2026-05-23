# PostgreSQL

A comprehensive guide to PostgreSQL internals, features, and practical usage.

---

## Table of Contents

1. [What is PostgreSQL?](#what-is-postgresql)
2. [Architecture](#architecture)
3. [Data Types](#data-types)
4. [Indexing](#indexing)
5. [MVCC & Concurrency](#mvcc--concurrency)
6. [Query Execution & EXPLAIN](#query-execution--explain)
7. [Transactions & Isolation](#transactions--isolation)
8. [Performance Tuning](#performance-tuning)
9. [Replication & HA](#replication--ha)
10. [Partitioning](#partitioning)
11. [Advanced Features](#advanced-features)
12. [Common Operations](#common-operations)
13. [Interview Questions](#interview-questions)

---

## What is PostgreSQL?

- Open-source, ACID-compliant relational database
- Row-oriented storage (OLTP focused)
- Uses B-Tree indexes by default, MVCC for concurrency
- Extensible: custom types, functions, indexes, languages
- Closest open-source equivalent to Oracle

### PostgreSQL vs MySQL


| Aspect            | PostgreSQL                        | MySQL (InnoDB)                  |
| ----------------- | --------------------------------- | ------------------------------- |
| MVCC              | Stores old versions in heap       | Stores old versions in undo log |
| Default isolation | Read Committed                    | Repeatable Read                 |
| JSON support      | JSONB (binary, indexable)         | JSON (text only)                |
| Full-text search  | Built-in (GIN index)              | Separate engine                 |
| Extensions        | Rich ecosystem (PostGIS, pg_trgm) | Limited                         |
| Partitioning      | Declarative (native)              | Built-in                        |
| Replication       | Streaming (physical), Logical     | Binlog-based                    |
| Best for          | Complex queries, data integrity   | Simple reads, web apps          |


---

## Architecture

### Process Model

```
Client Connections
    │  │  │
    ▼  ▼  ▼
┌──────────────────────────────────────────────────────────┐
│                    POSTMASTER                              │
│              (Main daemon process)                         │
│   - Listens for connections                               │
│   - Forks a BACKEND process per client                    │
└──────┬──────────────────────────────┬────────────────────┘
       │                              │
       ▼                              ▼
┌──────────────┐            ┌──────────────────────────────┐
│  Backend 1   │            │   Background Processes        │
│  (client 1)  │            │                              │
├──────────────┤            │  - BGWriter (writes dirty     │
│  Backend 2   │            │    pages to disk)             │
│  (client 2)  │            │  - WAL Writer (flushes WAL)  │
├──────────────┤            │  - Checkpointer              │
│  Backend 3   │            │  - Autovacuum Launcher       │
│  (client 3)  │            │  - Stats Collector           │
└──────────────┘            │  - Logical Replication       │
                            └──────────────────────────────┘
```

**Key point:** One process per connection (not threads). This is why connection pooling (PgBouncer) matters.

### Memory Architecture

```
┌──────────────────────────────────────────────────────────┐
│                   SHARED MEMORY                           │
│  ┌────────────────────────────────────────────────────┐  │
│  │              Shared Buffer Pool                     │  │
│  │         (cached data pages, default 128MB)         │  │
│  └────────────────────────────────────────────────────┘  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │  WAL Buffers │  │  Lock Table  │  │  Proc Array  │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
└──────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────┐
│              PER-BACKEND MEMORY (each connection)         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │  work_mem    │  │  temp_buffers│  │ maintenance_  │  │
│  │  (sorting)   │  │  (temp tbls) │  │  work_mem     │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
└──────────────────────────────────────────────────────────┘
```

### Storage Layout on Disk

```
$PGDATA/
├── base/                    ← Database files
│   ├── 16384/               ← One directory per database (OID)
│   │   ├── 16385            ← Table file (OID)
│   │   ├── 16385_fsm        ← Free Space Map
│   │   └── 16385_vm         ← Visibility Map
├── global/                  ← Cluster-wide tables
├── pg_wal/                  ← WAL files (Write-Ahead Log)
├── pg_xact/                 ← Transaction commit status
├── pg_stat_tmp/             ← Statistics
└── postgresql.conf          ← Configuration
```

**Each table file:**

- Split into 1GB segments
- Each segment = array of 8KB pages
- Pages contain tuples (rows)

---

## Data Types

### Common Types


| Type                   | Size      | Use                                 |
| ---------------------- | --------- | ----------------------------------- |
| `INTEGER` / `INT`      | 4 bytes   | Most numbers                        |
| `BIGINT`               | 8 bytes   | Large numbers, IDs                  |
| `SERIAL` / `BIGSERIAL` | 4/8 bytes | Auto-incrementing                   |
| `TEXT`                 | Variable  | Strings (no limit)                  |
| `VARCHAR(n)`           | Variable  | Strings (with limit)                |
| `BOOLEAN`              | 1 byte    | true/false                          |
| `TIMESTAMP`            | 8 bytes   | Date + time                         |
| `TIMESTAMPTZ`          | 8 bytes   | Timestamp with timezone (preferred) |
| `UUID`                 | 16 bytes  | Universally unique ID               |
| `JSONB`                | Variable  | Binary JSON (indexable)             |
| `NUMERIC(p,s)`         | Variable  | Exact decimal (money)               |


### TEXT vs VARCHAR

```sql
-- No performance difference in PostgreSQL
-- TEXT is simpler, VARCHAR(n) adds a length check
name TEXT;
name VARCHAR(255);  -- rejects strings > 255 chars
```

### JSONB (Binary JSON)

```sql
CREATE TABLE events (
    id SERIAL PRIMARY KEY,
    data JSONB
);

INSERT INTO events (data) VALUES ('{"type": "click", "page": "/home", "user_id": 42}');

-- Query JSON fields
SELECT * FROM events WHERE data->>'type' = 'click';
SELECT * FROM events WHERE data @> '{"type": "click"}';

-- Index for fast lookups
CREATE INDEX idx_events_data ON events USING GIN(data);

-- Extract nested values
SELECT data->'user'->'name' FROM events;    -- returns JSON
SELECT data->>'type' FROM events;            -- returns text
```

### Arrays

```sql
CREATE TABLE posts (
    id SERIAL PRIMARY KEY,
    title TEXT,
    tags TEXT[]
);

INSERT INTO posts (title, tags) VALUES ('Intro to SQL', ARRAY['sql', 'database', 'beginner']);

-- Query
SELECT * FROM posts WHERE 'sql' = ANY(tags);
SELECT * FROM posts WHERE tags @> ARRAY['sql', 'database'];

-- Index
CREATE INDEX idx_tags ON posts USING GIN(tags);
```

---

## Indexing

### Index Types

```sql
-- B-Tree (default) - equality, range, sorting
CREATE INDEX idx_name ON users(name);

-- Hash - equality only, smaller than B-Tree
CREATE INDEX idx_email ON users USING HASH(email);

-- GIN - arrays, JSONB, full-text search
CREATE INDEX idx_tags ON posts USING GIN(tags);

-- GiST - geometric, range types, nearest-neighbor
CREATE INDEX idx_location ON places USING GIST(location);

-- BRIN - large tables with naturally ordered data
CREATE INDEX idx_created ON logs USING BRIN(created_at);
```

### Practical Index Patterns

**Partial index** (index a subset of rows):

```sql
-- Only index active users (smaller, faster)
CREATE INDEX idx_active ON users(email) WHERE active = true;

-- Only index unprocessed orders
CREATE INDEX idx_pending ON orders(created_at) WHERE status = 'pending';
```

**Covering index** (avoid table lookup):

```sql
-- Query only needs name and email
CREATE INDEX idx_cover ON users(age) INCLUDE (name, email);

-- This query is now index-only:
SELECT name, email FROM users WHERE age > 25;
```

**Multi-column index:**

```sql
CREATE INDEX idx_multi ON orders(customer_id, order_date DESC);

-- Uses index:
SELECT * FROM orders WHERE customer_id = 5 ORDER BY order_date DESC;
SELECT * FROM orders WHERE customer_id = 5;

-- DOES NOT use index (not leftmost prefix):
SELECT * FROM orders WHERE order_date > '2024-01-01';
```

**Expression index:**

```sql
CREATE INDEX idx_lower_email ON users(LOWER(email));

-- Now this uses the index:
SELECT * FROM users WHERE LOWER(email) = 'john@example.com';
```

### When NOT to Index

- Small tables (< few thousand rows) - seq scan is faster
- Columns with very low cardinality (boolean, status with 3 values)
- Write-heavy tables with rarely queried columns
- Temporary/staging tables

---

## MVCC & Concurrency

### How PostgreSQL Implements MVCC

Every row (tuple) has hidden columns:

```
┌──────────┬──────────┬────────┬──────────────────────────┐
│  xmin    │  xmax    │  ctid  │  actual data columns...  │
└──────────┴──────────┴────────┴──────────────────────────┘
  ↑           ↑          ↑
  Creating    Deleting   Physical
  txn ID      txn ID     location (page, offset)
```

### INSERT

```sql
INSERT INTO users VALUES (1, 'Alice');

Tuple created:
  xmin = 100 (current txn id)
  xmax = 0   (not deleted)
  data = (1, 'Alice')
```

### DELETE

```sql
DELETE FROM users WHERE id = 1;

-- Doesn't physically remove! Just marks xmax:
  xmin = 100
  xmax = 200 (deleting txn id)    ← marked as dead
  data = (1, 'Alice')             ← still physically there
```

### UPDATE = DELETE + INSERT

```sql
UPDATE users SET name = 'Bob' WHERE id = 1;

-- Old tuple (marked dead):
  xmin = 100, xmax = 200, data = (1, 'Alice')

-- New tuple (created):
  xmin = 200, xmax = 0, data = (1, 'Bob')
```

**This is why PostgreSQL needs VACUUM** - dead tuples accumulate.

### Visibility Rules

Transaction T sees a tuple if:

```
1. xmin is committed AND xmin started before T's snapshot
2. xmax is NOT set, OR xmax is NOT committed, OR xmax started after T's snapshot
```

### VACUUM

```sql
-- Manual vacuum
VACUUM users;           -- reclaim dead tuples
VACUUM FULL users;      -- rewrite entire table (locks table!)
VACUUM ANALYZE users;   -- vacuum + update statistics

-- Autovacuum runs automatically (configured in postgresql.conf)
```

**What VACUUM does:**

1. Scans table for dead tuples (xmax committed, not visible to any active txn)
2. Marks space as reusable in Free Space Map
3. Updates Visibility Map (pages where all tuples are visible)
4. Does NOT return space to OS (VACUUM FULL does, but locks table)

### HOT Updates (Heap-Only Tuples)

Optimization for updates that don't change indexed columns:

```sql
-- If name is NOT indexed:
UPDATE users SET name = 'Bob' WHERE id = 1;

-- New tuple placed on SAME page as old
-- No index update needed!
-- Old tuple points to new tuple (HOT chain)
```

**Requirements for HOT:**

- Updated columns are not indexed
- New tuple fits on same page

---

## Query Execution & EXPLAIN

### Reading EXPLAIN Output

```sql
EXPLAIN ANALYZE SELECT * FROM orders WHERE customer_id = 5;

-- Output:
Index Scan using idx_customer on orders  (cost=0.42..8.44 rows=10 width=48)
                                          (actual time=0.02..0.05 rows=8 loops=1)
  Index Cond: (customer_id = 5)
Planning Time: 0.1 ms
Execution Time: 0.08 ms
```

Breaking it down:

```
cost=0.42..8.44
      ↑     ↑
  startup  total cost (arbitrary units, NOT milliseconds)

rows=10          ← estimated rows
actual rows=8    ← real rows (only with ANALYZE)
width=48         ← average row size in bytes
loops=1          ← how many times this node executed
```

### Common Scan Types

```sql
-- Sequential Scan (reads entire table)
Seq Scan on users  (cost=0.00..170.00 rows=5000 width=64)

-- Index Scan (traverse index, fetch rows from table)
Index Scan using idx_email on users  (cost=0.28..8.29 rows=1 width=64)

-- Index Only Scan (answer from index alone, no table visit)
Index Only Scan using idx_cover on users  (cost=0.28..4.29 rows=1 width=32)

-- Bitmap Index Scan (index → bitmap → table)
Bitmap Heap Scan on orders  (cost=12.00..350.00 rows=500 width=48)
  → Bitmap Index Scan on idx_status  (cost=0.00..11.75 rows=500 width=0)
```

**When is each used?**

```
Few rows match    → Index Scan (direct lookup)
Moderate rows     → Bitmap Scan (batch random I/O)
Many rows         → Seq Scan (just read everything)
All data in index → Index Only Scan (fastest)
```

### Common Join Types in EXPLAIN

```sql
-- Nested Loop (small outer, indexed inner)
Nested Loop  (cost=0.42..40.00 rows=10 width=96)
  → Seq Scan on customers
  → Index Scan on orders using idx_cust_id

-- Hash Join (medium tables, equality join)
Hash Join  (cost=200.00..500.00 rows=1000 width=96)
  → Seq Scan on orders
  → Hash
    → Seq Scan on customers

-- Merge Join (pre-sorted data)
Merge Join  (cost=300.00..400.00 rows=1000 width=96)
  → Sort
    → Seq Scan on orders
  → Sort
    → Seq Scan on customers
```

---

## Transactions & Isolation

### Isolation Levels in PostgreSQL

```sql
-- Set isolation level
BEGIN TRANSACTION ISOLATION LEVEL SERIALIZABLE;

-- Or per session
SET default_transaction_isolation = 'repeatable read';
```


| Level           | Dirty Read | Non-Repeatable Read | Phantom | Serialization Anomaly |
| --------------- | ---------- | ------------------- | ------- | --------------------- |
| Read Committed  | No         | Yes                 | Yes     | Yes                   |
| Repeatable Read | No         | No                  | No*     | Yes                   |
| Serializable    | No         | No                  | No      | No                    |


*PostgreSQL's Repeatable Read actually prevents phantoms too (stronger than SQL standard).

### Read Committed (Default)

```sql
-- T1                              -- T2
BEGIN;                             BEGIN;
SELECT age FROM users WHERE id=1;  
-- returns 25                      
                                   UPDATE users SET age=30 WHERE id=1;
                                   COMMIT;
SELECT age FROM users WHERE id=1;
-- returns 30 ← sees T2's commit!
COMMIT;
```

Each **statement** gets a fresh snapshot.

### Repeatable Read

```sql
-- T1                              -- T2
BEGIN ISOLATION LEVEL 
  REPEATABLE READ;                 BEGIN;
SELECT age FROM users WHERE id=1;
-- returns 25
                                   UPDATE users SET age=30 WHERE id=1;
                                   COMMIT;
SELECT age FROM users WHERE id=1;
-- returns 25 ← still sees original!
COMMIT;
```

Snapshot taken at **first statement** in transaction. Entire transaction sees same data.

### Serializable (SSI)

```sql
-- Prevents write skew
-- T1                              -- T2
BEGIN ISOLATION LEVEL              BEGIN ISOLATION LEVEL
  SERIALIZABLE;                      SERIALIZABLE;
SELECT count(*) FROM doctors       SELECT count(*) FROM doctors
  WHERE on_call = true;              WHERE on_call = true;
-- returns 2                       -- returns 2
UPDATE doctors SET on_call=false   UPDATE doctors SET on_call=false
  WHERE name='Alice';                WHERE name='Bob';
COMMIT; ← succeeds                COMMIT; ← ERROR: serialization failure
```

PostgreSQL detects the conflict and aborts one transaction.

---

## Performance Tuning

### Key Configuration Parameters

```ini
# Memory
shared_buffers = '4GB'          # Buffer pool (25% of RAM)
work_mem = '256MB'              # Per-sort/hash operation
maintenance_work_mem = '1GB'    # VACUUM, CREATE INDEX
effective_cache_size = '12GB'   # Hint to optimizer (75% of RAM)

# WAL
wal_buffers = '64MB'
checkpoint_completion_target = 0.9
max_wal_size = '4GB'

# Query Planner
random_page_cost = 1.1          # Lower for SSD (default 4.0)
effective_io_concurrency = 200  # Higher for SSD

# Connections
max_connections = 200           # Use connection pooling!

# Autovacuum
autovacuum_vacuum_scale_factor = 0.1   # Vacuum when 10% dead tuples
autovacuum_analyze_scale_factor = 0.05 # Analyze when 5% changed
```

### Common Performance Problems

**1. Missing index:**

```sql
EXPLAIN ANALYZE SELECT * FROM orders WHERE customer_id = 5;
-- Seq Scan on orders (cost=0.00..25000.00 rows=10 ...)
-- FIX: CREATE INDEX idx_cust ON orders(customer_id);
```

**2. Index not used (stale stats):**

```sql
ANALYZE orders;  -- refresh statistics
```

**3. Table bloat (dead tuples):**

```sql
-- Check bloat
SELECT n_live_tup, n_dead_tup, last_vacuum 
FROM pg_stat_user_tables WHERE relname = 'orders';

-- Fix
VACUUM ANALYZE orders;
```

**4. Slow joins (wrong join type):**

```sql
-- Force hash join for testing
SET enable_nestloop = off;
EXPLAIN ANALYZE SELECT ...;
SET enable_nestloop = on;
```

**5. N+1 query problem:**

```sql
-- BAD: 1 query + N queries
SELECT id FROM customers;            -- 1 query
SELECT * FROM orders WHERE cust_id = 1;  -- N queries
SELECT * FROM orders WHERE cust_id = 2;
...

-- GOOD: 1 query with JOIN
SELECT c.*, o.* FROM customers c JOIN orders o ON c.id = o.cust_id;
```

---

## Replication & HA

### Streaming Replication (Physical)

```
Primary                    Replica
┌──────────┐    WAL stream   ┌──────────┐
│  Writes  │ ──────────────→ │ Applies  │
│  here    │                 │ WAL      │
│          │                 │          │
│  R/W     │                 │ Read-only│
└──────────┘                 └──────────┘
```

```sql
-- On primary: postgresql.conf
wal_level = replica
max_wal_senders = 5

-- On replica: recovery signal
standby_mode = on
primary_conninfo = 'host=primary port=5432'
```

**Synchronous vs Async:**

```ini
# Async (default) - fast, may lose data
synchronous_commit = off

# Sync - safe, slower
synchronous_standby_names = 'replica1'
```

### Logical Replication

Replicates specific tables, not entire cluster:

```sql
-- On publisher
CREATE PUBLICATION my_pub FOR TABLE users, orders;

-- On subscriber
CREATE SUBSCRIPTION my_sub 
  CONNECTION 'host=primary dbname=mydb'
  PUBLICATION my_pub;
```


| Aspect            | Physical (Streaming) | Logical                      |
| ----------------- | -------------------- | ---------------------------- |
| What's replicated | Entire cluster       | Specific tables              |
| Replica writable  | No                   | Yes (other tables)           |
| Cross-version     | No                   | Yes                          |
| Use case          | HA failover          | Data distribution, migration |


---

## Partitioning

### Declarative Partitioning

```sql
-- Range partitioning (e.g., by date)
CREATE TABLE logs (
    id BIGSERIAL,
    message TEXT,
    created_at TIMESTAMPTZ
) PARTITION BY RANGE (created_at);

CREATE TABLE logs_2024_q1 PARTITION OF logs
    FOR VALUES FROM ('2024-01-01') TO ('2024-04-01');
CREATE TABLE logs_2024_q2 PARTITION OF logs
    FOR VALUES FROM ('2024-04-01') TO ('2024-07-01');

-- List partitioning (e.g., by region)
CREATE TABLE orders (
    id BIGSERIAL,
    region TEXT,
    total NUMERIC
) PARTITION BY LIST (region);

CREATE TABLE orders_us PARTITION OF orders FOR VALUES IN ('US');
CREATE TABLE orders_eu PARTITION OF orders FOR VALUES IN ('EU', 'UK');

-- Hash partitioning (even distribution)
CREATE TABLE events (
    id BIGSERIAL,
    user_id INT,
    data JSONB
) PARTITION BY HASH (user_id);

CREATE TABLE events_0 PARTITION OF events FOR VALUES WITH (MODULUS 4, REMAINDER 0);
CREATE TABLE events_1 PARTITION OF events FOR VALUES WITH (MODULUS 4, REMAINDER 1);
CREATE TABLE events_2 PARTITION OF events FOR VALUES WITH (MODULUS 4, REMAINDER 2);
CREATE TABLE events_3 PARTITION OF events FOR VALUES WITH (MODULUS 4, REMAINDER 3);
```

### Partition Pruning

```sql
-- Query only touches relevant partition
SELECT * FROM logs WHERE created_at = '2024-02-15';
-- PostgreSQL only scans logs_2024_q1 (skips other partitions)

EXPLAIN output:
  Append
    → Seq Scan on logs_2024_q1   ← only this partition scanned
```

---

## Advanced Features

### CTEs (Common Table Expressions)

```sql
WITH regional_sales AS (
    SELECT region, SUM(total) as total_sales
    FROM orders GROUP BY region
),
top_regions AS (
    SELECT region FROM regional_sales
    WHERE total_sales > 1000000
)
SELECT * FROM orders WHERE region IN (SELECT region FROM top_regions);
```

### Window Functions

```sql
-- Rank users by order count
SELECT 
    customer_id,
    COUNT(*) as order_count,
    RANK() OVER (ORDER BY COUNT(*) DESC) as rank
FROM orders GROUP BY customer_id;

-- Running total
SELECT 
    date, amount,
    SUM(amount) OVER (ORDER BY date) as running_total
FROM transactions;

-- Compare to previous row
SELECT
    date, amount,
    LAG(amount) OVER (ORDER BY date) as prev_amount,
    amount - LAG(amount) OVER (ORDER BY date) as change
FROM daily_sales;
```

### Connection Pooling (PgBouncer)

```
Without pooling:
  100 clients → 100 PostgreSQL processes → high memory

With PgBouncer:
  100 clients → PgBouncer → 20 PostgreSQL processes
  
Modes:
  - Session pooling: conn held for session duration
  - Transaction pooling: conn returned after each txn (recommended)
  - Statement pooling: conn returned after each statement
```

### LISTEN/NOTIFY

```sql
-- Subscriber
LISTEN new_orders;

-- Publisher (in another session)
NOTIFY new_orders, '{"order_id": 123, "amount": 50.00}';

-- Subscriber receives:
-- Asynchronous notification "new_orders" with payload "{"order_id": 123}"
```

### Advisory Locks

Application-level locks managed by PostgreSQL:

```sql
-- Acquire lock (blocks if held by another session)
SELECT pg_advisory_lock(12345);

-- Do work...

-- Release
SELECT pg_advisory_unlock(12345);

-- Try without blocking
SELECT pg_try_advisory_lock(12345);  -- returns true/false
```

---

## Common Operations

### Useful Queries

```sql
-- Table sizes
SELECT relname, pg_size_pretty(pg_relation_size(relid))
FROM pg_stat_user_tables ORDER BY pg_relation_size(relid) DESC;

-- Index usage
SELECT indexrelname, idx_scan, idx_tup_read
FROM pg_stat_user_indexes ORDER BY idx_scan;

-- Unused indexes (candidates for removal)
SELECT indexrelname FROM pg_stat_user_indexes WHERE idx_scan = 0;

-- Active queries
SELECT pid, query, state, now() - query_start AS duration
FROM pg_stat_activity WHERE state = 'active';

-- Kill a query
SELECT pg_cancel_backend(pid);     -- graceful
SELECT pg_terminate_backend(pid);  -- force

-- Dead tuple ratio
SELECT relname, n_live_tup, n_dead_tup,
       ROUND(n_dead_tup::numeric / NULLIF(n_live_tup, 0) * 100, 1) AS dead_pct
FROM pg_stat_user_tables ORDER BY dead_pct DESC;

-- Cache hit ratio (should be > 99%)
SELECT 
    SUM(heap_blks_hit) / (SUM(heap_blks_hit) + SUM(heap_blks_read)) AS ratio
FROM pg_statio_user_tables;
```

---

## Interview Questions

**Q: How does PostgreSQL handle an UPDATE internally?**

> Creates a new tuple with updated values (xmin = current txn). Marks old tuple's xmax = current txn. Old tuple remains until VACUUM. Index entries may need updating unless HOT update applies.

**Q: What is table bloat and how do you fix it?**

> Dead tuples from updates/deletes accumulate, wasting space and slowing scans. Fix with regular VACUUM (marks space reusable) or VACUUM FULL (rewrites table, but locks it). Monitor with pg_stat_user_tables.

**Q: When would you use BRIN index over B-Tree?**

> BRIN for very large tables where data is naturally ordered (e.g., time-series logs). BRIN stores min/max per block range → tiny index size. But only useful when physical order correlates with value order.

**Q: Explain connection pooling. Why is it important?**

> PostgreSQL forks a process per connection (~10MB each). 500 connections = 5GB just for processes. PgBouncer multiplexes many clients over fewer database connections. Transaction pooling mode returns connections after each transaction.

**Q: How does PostgreSQL's SERIALIZABLE differ from other databases?**

> Uses SSI (Serializable Snapshot Isolation) - not locking. Transactions run with MVCC snapshots, PostgreSQL tracks read/write dependencies, detects dangerous structures, aborts one transaction if serialization anomaly is possible. Non-blocking unlike 2PL-based serializable.

**Q: What is a HOT update?**

> When an UPDATE doesn't change any indexed column and new tuple fits on the same page, PostgreSQL avoids index updates. Old tuple gets a redirect pointer to new tuple. Significantly reduces I/O for frequently updated non-indexed columns.

