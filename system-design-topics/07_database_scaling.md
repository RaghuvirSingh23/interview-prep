# Database Sharding & Replication

A comprehensive guide to database scaling for system design interviews: vertical scaling, read replicas, replication lag, partitioning, sharding, consistency, resharding, hot spots, and operational trade-offs.

---

## Table of Contents

1. [Why Databases Become Bottlenecks](#why-databases-become-bottlenecks)
2. [Scaling Strategy Ladder](#scaling-strategy-ladder)
3. [Vertical Scaling](#vertical-scaling)
4. [Read Replication](#read-replication)
5. [Replication Models](#replication-models)
6. [Partitioning vs Sharding](#partitioning-vs-sharding)
7. [Sharding Strategies](#sharding-strategies)
8. [Routing and Metadata](#routing-and-metadata)
9. [Resharding and Rebalancing](#resharding-and-rebalancing)
10. [Hot Shards and Skew](#hot-shards-and-skew)
11. [Transactions Across Shards](#transactions-across-shards)
12. [Consistency and Replication Lag](#consistency-and-replication-lag)
13. [Denormalization, CQRS, and Materialized Views](#denormalization-cqrs-and-materialized-views)
14. [Capacity Planning](#capacity-planning)
15. [Operational Playbook](#operational-playbook)
16. [Hands-On Exercises](#hands-on-exercises)
17. [Interview Questions](#interview-questions)
18. [Quick Reference](#quick-reference)

---

## Why Databases Become Bottlenecks

Databases are often the hardest part of system scaling because they hold durable state and must preserve correctness.

Common bottlenecks:

- CPU from query execution
- memory from working set and connections
- disk I/O from reads/writes/checkpoints
- locks and transaction contention
- network bandwidth
- replication lag
- connection storms
- inefficient indexes
- unbounded table growth

### First Rule

Do not jump to sharding too early.

Before sharding, check:

- query plans
- missing or bad indexes
- N+1 query patterns
- cache opportunities
- read replicas
- connection pooling
- archiving old data
- schema/data model changes

Sharding is powerful but expensive. It makes queries, operations, migrations, and consistency harder.

---

## Scaling Strategy Ladder

A practical scaling ladder:

1. Fix inefficient queries and indexes.
2. Add connection pooling.
3. Add caching for hot reads.
4. Vertically scale primary database.
5. Add read replicas.
6. Split workloads by domain or service.
7. Partition large tables.
8. Denormalize read models.
9. Shard by tenant/user/entity.
10. Use specialized stores for specialized workloads.

### Example Evolution

```
Single DB
   |
   v
Single DB + Redis cache
   |
   v
Primary + read replicas
   |
   v
Domain split: user DB, order DB, payment DB
   |
   v
Shard order DB by merchant_id
```

Each step should solve a concrete bottleneck.

---

## Vertical Scaling

Vertical scaling means using a bigger machine:

- more CPU
- more RAM
- faster disks
- more IOPS
- better network

### Pros

- Simple
- No application routing changes
- Strong transaction model preserved
- Operationally familiar

### Cons

- Hard ceiling
- Expensive at high end
- Failover still needed
- Does not solve write hot spots forever

### When Vertical Scaling Is Enough

For many interview systems, a well-tuned primary database with read replicas handles surprisingly large traffic.

Example:

```
4,000 reads/sec and 100 writes/sec
```

This may be fine for a tuned Postgres/MySQL deployment with caching and replicas.

Do not over-engineer unless scale requires it.

---

## Read Replication

Read replicas copy data from primary and serve read traffic.

```
Writes
  |
  v
+---------+        replication        +----------+
| Primary | ------------------------> | Replica1 |
+---------+                           +----------+
     |
     +------------------------------> +----------+
                                      | Replica2 |
                                      +----------+
```

### Benefits

- Scale read-heavy workloads
- Isolate analytics/reporting reads
- Provide failover targets
- Offload backups

### Limitations

- Writes still go to primary
- Replicas can lag
- Read-after-write may be stale
- More operational complexity

### Read Routing

Common routing:

- writes to primary
- strongly consistent reads to primary
- eventually consistent reads to replicas
- analytics reads to dedicated replica

Example:

```text
User updates profile -> write primary
Immediately show updated profile -> read primary
Browse public profile later -> read replica
```

### Replication Lag

Lag is delay between primary commit and replica visibility.

Causes:

- high write volume
- slow replica hardware
- network delay
- long-running transactions
- replication apply bottleneck

Mitigations:

- read-your-writes from primary for a short window
- monitor lag and stop routing reads to lagging replicas
- use synchronous replication for critical paths
- keep transactions small
- scale write workload or shard

---

## Replication Models

### Single-Leader Replication

One primary accepts writes. Replicas follow.

Pros:

- Simple conflict model
- Common and well supported
- Strong consistency possible on primary

Cons:

- Write bottleneck at primary
- Failover complexity
- Replica lag

Use for most OLTP systems.

### Multi-Leader Replication

Multiple leaders accept writes.

```
Region A leader <----replicate----> Region B leader
```

Pros:

- Lower write latency in multiple regions
- Region-level write availability

Cons:

- Write conflicts
- Complex conflict resolution
- Harder correctness

Use only when multi-region writes are required.

### Leaderless Replication

Clients write/read multiple replicas with quorums.

Concept:

```
N = replicas
W = write acknowledgements
R = read acknowledgements
If R + W > N, reads overlap latest write quorum
```

Used by Dynamo-style systems.

Pros:

- High availability
- Flexible consistency

Cons:

- Conflict resolution
- Read repair
- More complex mental model

### Synchronous vs Asynchronous Replication

Synchronous:

- primary waits for replica ack before commit
- stronger durability
- higher write latency
- replica/network issue can affect writes

Asynchronous:

- primary commits without waiting
- lower latency
- possible data loss on failover
- replica lag

---

## Partitioning vs Sharding

These terms are often confused.

### Partitioning

Partitioning splits one logical table into smaller pieces, often inside the same database cluster.

Example:

```sql
orders_2026_01
orders_2026_02
orders_2026_03
```

Benefits:

- faster pruning for queries
- easier archival
- smaller indexes per partition
- maintenance by partition

### Sharding

Sharding splits data across separate database nodes.

```
Shard 1: users 0..999999
Shard 2: users 1000000..1999999
Shard 3: users 2000000..2999999
```

Benefits:

- horizontal write scaling
- horizontal storage scaling
- fault isolation by shard

Costs:

- application routing
- cross-shard queries
- distributed transactions
- resharding
- hot shard handling
- operational complexity

### Interview Distinction

Partitioning helps manage large tables. Sharding helps scale across machines.

---

## Sharding Strategies

### Range-Based Sharding

Shard by ranges of key values.

```
user_id 1-1M     -> shard A
user_id 1M-2M    -> shard B
user_id 2M-3M    -> shard C
```

Pros:

- Range queries are efficient
- Easy to understand

Cons:

- Hot spots for monotonically increasing keys
- Manual splitting/rebalancing

Bad for write-heavy increasing IDs unless mitigated.

### Hash-Based Sharding

Hash the shard key.

```
shard = hash(user_id) % N
```

Pros:

- Even distribution
- Simple routing

Cons:

- Range queries across shards
- Changing N moves many keys

### Consistent Hashing

Hash keys and shards onto a ring. Adding/removing shards moves only a subset of keys.

Use for systems needing smoother rebalancing.

### Directory-Based Sharding

A lookup service maps each key/tenant to a shard.

```
tenant_123 -> shard_7
tenant_456 -> shard_2
```

Pros:

- Flexible
- Easy to move specific tenants
- Handles large tenants specially

Cons:

- Directory service is critical
- Extra lookup/cache
- More operational complexity

### Geographic Sharding

Shard by region.

```
US users -> us-east DB
EU users -> eu-west DB
India users -> ap-south DB
```

Pros:

- lower latency
- data residency
- region isolation

Cons:

- global queries are hard
- cross-region consistency
- user movement between regions

### Tenant-Based Sharding

For SaaS:

```
tenant_id -> shard
```

Pros:

- tenant isolation
- easier enterprise tenant placement
- predictable routing

Cons:

- large tenants can dominate a shard
- cross-tenant analytics need aggregation

### Choosing a Shard Key

A good shard key:

- appears in most queries
- has high cardinality
- distributes traffic evenly
- avoids hot spots
- supports data locality for transactions

Common choices:

- `user_id`
- `tenant_id`
- `merchant_id`
- `account_id`
- `order_id` if operations are order-local

Bad choices:

- timestamp alone for write-heavy workloads
- status with few values
- country if traffic is uneven
- auto-increment ID with range sharding

---

## Routing and Metadata

### Application-Level Routing

Application computes shard:

```python
shard = hash(user_id) % shard_count
db = shard_connections[shard]
```

Pros:

- simple
- no extra proxy

Cons:

- shard logic in application
- harder migrations

### Proxy-Based Routing

Application connects to a database proxy. Proxy routes query.

```
App -> DB proxy -> shard
```

Pros:

- central routing
- easier app code

Cons:

- proxy is critical infrastructure
- query parsing/routing complexity

### Directory Service

Use metadata table/service:

```sql
tenant_shards(tenant_id, shard_id)
```

Cache mapping in application for speed.

### Scatter-Gather Queries

Some queries must hit all shards.

```
SELECT count(*) FROM orders WHERE created_at > yesterday
```

If orders are sharded by user, global time query becomes:

```
query shard 1
query shard 2
query shard 3
merge results
```

Scatter-gather is expensive. Use sparingly or build read models/analytics pipelines.

---

## Resharding and Rebalancing

Resharding means moving data between shards.

### Why Reshard?

- add capacity
- fix hot shard
- isolate large tenant
- rebalance uneven growth
- migrate regions

### Naive Problem

With `hash(key) % N`, changing N moves many keys.

```
N=4 -> N=5 changes most mappings
```

### Better: Virtual Shards

Use many logical buckets mapped to physical shards.

```
bucket = hash(user_id) % 4096
bucket -> physical shard
```

To add a shard, move some buckets.

Pros:

- smoother rebalancing
- less application change
- easier online migration

### Online Migration Pattern

1. Mark bucket/tenant as migrating.
2. Copy historical data to new shard.
3. Dual-write or capture changes during copy.
4. Verify counts/checksums.
5. Switch reads to new shard.
6. Stop writes to old shard.
7. Cleanup old data later.

### Dual-Write Danger

Dual writes can fail halfway. Prefer change data capture or transactional outbox where possible.

If dual-writing:

- make writes idempotent
- retry safely
- reconcile differences
- monitor divergence

---

## Hot Shards and Skew

### Hot Shard

A shard receives much more load than others.

Causes:

- celebrity user
- large tenant
- hot product
- bad shard key
- time-based sharding with current period receiving all writes

### Detection

Monitor per shard:

- QPS
- CPU
- disk IOPS
- lock waits
- replication lag
- p95/p99 latency
- storage growth

### Mitigations

### Split Large Tenant

Move a big tenant to its own shard.

```
tenant_big -> dedicated shard
```

### Sub-Shard Hot Entity

Split by compound key:

```
merchant_id + hash(order_id) % 16
```

### Add Cache

For hot reads, use Redis/CDN/in-process cache.

### Queue Writes

For bursty writes, use Kafka/queue to smooth ingestion, if business allows async writes.

### Change Data Model

Avoid single counter row or single global index for all writes.

Example:

```
global_counter -> counter:0..99 then sum
```

---

## Transactions Across Shards

### Best Strategy: Avoid Them

Design shard key so most transactions are single-shard.

Example:

```
All order rows for one merchant on same shard
```

### Two-Phase Commit

2PC coordinates commit across multiple databases.

```
prepare all shards
if all prepared -> commit all
else -> abort all
```

Pros:

- atomic across shards

Cons:

- slow
- blocking
- coordinator failure complexity
- not always supported

### Saga Pattern

Break workflow into local transactions with compensating actions.

Example order flow:

```
Create order -> Reserve inventory -> Authorize payment -> Confirm order
```

If payment fails:

```
Release inventory -> Cancel order
```

Pros:

- scalable
- fits microservices

Cons:

- eventual consistency
- compensation logic
- harder reasoning

### Idempotency

Distributed workflows must be idempotent:

- each step has unique command/event ID
- retries do not duplicate effects
- state machine rejects invalid transitions

---

## Consistency and Replication Lag

### Read-After-Write Consistency

User writes data, then immediately reads it.

Problem:

```
write primary
read replica before replication catches up
stale result
```

Solutions:

- read from primary after write
- sticky primary reads for user/session for short period
- track last write timestamp/LSN and choose replica caught up to it
- synchronous replication for critical data

### Monotonic Reads

Once a user sees version 5, they should not later see version 4.

Solutions:

- route same user to same replica
- version-aware reads
- primary reads for sensitive flows

### Eventual Consistency

Eventual consistency means replicas converge if no new writes occur.

Use for:

- feeds
- counters
- recommendations
- analytics
- search indexes

Avoid for:

- account balances
- inventory checkout without reservation
- permission/security decisions

---

## Denormalization, CQRS, and Materialized Views

### Denormalization

Store duplicated data to speed reads.

Example:

```
orders table stores customer_name snapshot
```

Pros:

- fewer joins
- faster reads

Cons:

- update consistency
- larger storage

### CQRS

Command Query Responsibility Segregation separates write model and read model.

```
Write DB -> events/CDC -> read-optimized views
```

Use when read queries differ greatly from write model.

### Materialized Views

Precomputed query results.

Examples:

- daily sales by merchant
- user feed
- product search document

Refresh methods:

- synchronous update on write
- async event processing
- periodic batch job

### Search and Analytics

Do not force OLTP DB to serve every workload.

Use specialized systems:

- Elasticsearch/OpenSearch for search
- ClickHouse/BigQuery/Snowflake for analytics
- Redis for cache/counters
- Kafka for event streams

---

## Capacity Planning

### Storage Estimate

```
rows_per_day * average_row_size * retention_days * replication_factor
```

Example:

```
10M orders/day
1 KB/order row including indexes estimate
365 days
RF 3

10M * 1 KB * 365 * 3 = 10.95 TB
```

Indexes often add significant storage. Include them.

### QPS Estimate

Break down:

- reads/sec
- writes/sec
- peak multiplier
- queries per request
- cache hit ratio

Example:

```
10,000 requests/sec
80% require DB read
cache hit ratio 90%

DB reads/sec = 10,000 * 0.8 * 0.1 = 800
```

### Connection Planning

Too many app instances can overload database connections.

```
200 pods * 20 connections = 4,000 DB connections
```

Use connection pooling:

- PgBouncer for Postgres
- ProxySQL for MySQL
- application pool limits

### Index Cost

Indexes speed reads but slow writes.

Each write may update:

- table page
- primary key index
- secondary indexes
- WAL/binlog

Do not index everything.

---

## Operational Playbook

### Metrics to Monitor

Primary:

- CPU
- memory/cache hit ratio
- disk IOPS
- write latency
- lock waits/deadlocks
- active connections
- slow queries
- replication lag
- WAL/binlog growth

Replicas:

- lag
- query latency
- replay/apply rate
- connection count
- disk usage

Shards:

- per-shard QPS
- per-shard storage
- per-shard p99 latency
- hot key/tenant distribution

### Common Problems

| Symptom | Likely cause | Fix |
| --- | --- | --- |
| Slow reads | missing index, bad query, cache miss | EXPLAIN, index, cache |
| Slow writes | too many indexes, locks, disk I/O | reduce indexes, batch, tune |
| Replica stale | replication lag | monitor, route to primary, scale |
| Connection errors | too many app connections | pooling, limits |
| One shard overloaded | skew/hot tenant | split, move, cache |
| Cross-shard query slow | scatter-gather | read model, analytics store |

### Migration Safety

For schema migrations:

1. Add backward-compatible column/table.
2. Deploy code that writes old and new if needed.
3. Backfill in batches.
4. Verify.
5. Switch reads.
6. Remove old path later.

Avoid long locks on large tables.

### Backups and Restore

Backups are useful only if restore is tested.

Need:

- full backups
- point-in-time recovery logs
- restore drills
- retention policy
- encryption
- access control

---

## Hands-On Exercises

### Exercise 1: Read Replica Routing

Design pseudo-code:

```python
def execute(query, consistency):
    if query.is_write:
        return primary.execute(query)
    if consistency == "strong":
        return primary.execute(query)
    return choose_healthy_replica().execute(query)
```

Explain when you require strong reads.

### Exercise 2: Choose a Shard Key

For each system, choose a shard key:

- social media posts
- payment transactions
- multi-tenant SaaS projects
- chat messages
- product catalog

Discuss trade-offs and common queries.

### Exercise 3: Estimate Database Load

Given:

```
1M daily active users
20 reads/user/day
2 writes/user/day
peak = 5x average
cache hit ratio = 80%
```

Calculate peak DB reads/sec and writes/sec.

### Exercise 4: Design Read-After-Write

For a profile update flow:

1. User updates display name.
2. User refreshes profile page immediately.

Design routing so the user sees the new value.

### Exercise 5: Reshard a Tenant

Design steps to move `tenant_123` from shard A to shard B without downtime.

Include:

- copy
- change capture
- verification
- routing switch
- rollback

### Exercise 6: Avoid Cross-Shard Transaction

A money transfer crosses two account shards.

Discuss:

- can accounts be co-located?
- do you need 2PC?
- can you use a ledger and saga?
- what invariants must hold?

---

## Interview Questions

### Basic Questions

**Q: How do you scale a database?**

Start with query/index optimization, connection pooling, caching, vertical scaling, and read replicas. Then split by domain, partition large tables, denormalize read models, and shard only when necessary.

**Q: What is read replication?**

Replicas copy data from a primary and serve read traffic. This scales reads but introduces replication lag.

**Q: What is sharding?**

Splitting data across multiple database nodes by a shard key to scale storage and writes horizontally.

**Q: Partitioning vs sharding?**

Partitioning splits data within a database/table for manageability and pruning. Sharding distributes data across separate database nodes.

### Replication Questions

**Q: What is replication lag and why does it matter?**

Lag is delay before primary writes appear on replicas. It can cause stale reads after writes.

**Q: How do you handle read-after-write consistency with replicas?**

Read from primary after writes, use sticky primary reads for a short window, or route to replicas known to have replayed beyond the user's write position.

**Q: Synchronous vs asynchronous replication?**

Synchronous waits for replica acknowledgement, improving durability but increasing latency and reducing availability. Asynchronous is faster but can lose recent writes on failover.

### Sharding Questions

**Q: How do you choose a shard key?**

Pick a high-cardinality key present in most queries, evenly distributes load, avoids hot spots, and keeps transactions local.

**Q: What is a hot shard?**

A shard receiving disproportionate traffic or storage. Fix by splitting hot tenants/entities, changing key strategy, caching, or rebalancing.

**Q: How do you handle cross-shard queries?**

Avoid them on hot paths. Use scatter-gather only when acceptable, or build read models/materialized views/analytics stores.

**Q: How do you reshard without downtime?**

Use virtual shards or directory mapping, copy data online, capture ongoing changes, verify, switch routing, and cleanup later.

### Advanced Questions

**Q: How do distributed transactions work across shards?**

Two-phase commit can provide atomicity but is slow and complex. Often systems avoid cross-shard transactions or use sagas with idempotency and compensation.

**Q: What is CQRS?**

Separating write model from read model. Writes go to normalized transactional storage; events/CDC update denormalized read views.

**Q: When would you use multi-leader replication?**

When multiple regions must accept writes locally. It requires conflict resolution and is more complex than single-leader replication.

**Q: How do you prevent database connection storms in Kubernetes?**

Limit app pool sizes, use PgBouncer/ProxySQL, cap autoscaling, and monitor active connections.

---

## Quick Reference

### Scaling Decision Tree

```text
Slow DB?
  |
  +-- Bad queries/indexes? Fix first.
  |
  +-- Too many repeated reads? Add cache.
  |
  +-- Read-heavy? Add replicas.
  |
  +-- Table too large? Partition/archive.
  |
  +-- Domain boundaries clear? Split databases.
  |
  +-- Writes/storage exceed one node? Shard.
```

### Shard Key Checklist

1. Present in most queries
2. High cardinality
3. Even load distribution
4. Avoids monotonically increasing hot ranges
5. Keeps transactions local
6. Supports tenant/entity isolation
7. Has a migration story

### Common Patterns

| Need | Pattern |
| --- | --- |
| Scale reads | read replicas |
| Reduce read load | cache |
| Manage huge table | partitioning |
| Scale writes/storage | sharding |
| Avoid stale reads | primary reads or version-aware replicas |
| Avoid cross-shard queries | read model/materialized view |
| Move data online | virtual shards + CDC/backfill |

### Interview Sound Bites

- "Sharding is a last-resort scaling tool because it moves complexity into application and operations."
- "Replication scales reads, not writes."
- "The shard key determines your future pain."
- "Avoid cross-shard transactions by designing data locality."
- "Always discuss replication lag when using read replicas."
