# Redis

A comprehensive guide to Redis for system design interviews: caching, data structures, rate limiting, queues, replication, persistence, and production trade-offs.

---

## Table of Contents

1. [What is Redis?](#what-is-redis)
2. [Why Redis is Fast](#why-redis-is-fast)
3. [Core Data Structures](#core-data-structures)
4. [Caching Patterns](#caching-patterns)
5. [Eviction and Expiration](#eviction-and-expiration)
6. [Persistence](#persistence)
7. [Replication and High Availability](#replication-and-high-availability)
8. [Redis Cluster and Sharding](#redis-cluster-and-sharding)
9. [Transactions, Lua, and Atomicity](#transactions-lua-and-atomicity)
10. [Streams, Pub/Sub, and Queues](#streams-pubsub-and-queues)
11. [Distributed Locks](#distributed-locks)
12. [System Design Patterns](#system-design-patterns)
13. [Operational Playbook](#operational-playbook)
14. [Hands-On Exercises](#hands-on-exercises)
15. [Interview Questions](#interview-questions)
16. [Quick Reference](#quick-reference)

---

## What is Redis?

Redis is an in-memory data store commonly used as a:

- Cache
- Session store
- Rate limiter
- Distributed lock coordinator
- Pub/Sub broker
- Lightweight queue
- Leaderboard or counter store
- Real-time feature store

Redis is often described as a key-value store, but that undersells it. A Redis key can hold a string, hash, list, set, sorted set, stream, bitmap, HyperLogLog, or geospatial index. This makes Redis useful for many high-throughput backend patterns.

### Redis in One Diagram

```
Application servers
       |
       | GET user:42
       v
+--------------------+
| Redis              |
| - in-memory data   |
| - optional WAL/AOF |
| - TTLs             |
| - atomic commands  |
+--------------------+
       |
       | cache miss
       v
+--------------------+
| Primary database   |
+--------------------+
```

### When to Use Redis

| Use case | Why Redis fits |
| --- | --- |
| Cache expensive reads | Microsecond to low-millisecond access, TTL support |
| Session store | Fast key lookup, easy expiration |
| Rate limiting | Atomic increments with expiry |
| Leaderboards | Sorted sets maintain rank efficiently |
| Deduplication | Sets provide fast membership checks |
| Lightweight queues | Lists or Streams support producer-consumer flows |
| Distributed coordination | Single-threaded command execution gives atomic primitives |

### When Not to Use Redis

Do not use Redis as the only durable source of truth unless you fully understand persistence, replication lag, memory sizing, backup, and data loss trade-offs.

Redis is usually a poor primary database for:

- Large datasets that do not fit in memory
- Complex relational queries
- Heavy ad hoc analytics
- Strict multi-record transactions across many keys
- Workloads needing long-term historical storage

---

## Why Redis is Fast

Redis is fast because it is designed around a small number of simple ideas.

### In-Memory Data

Most operations read and write memory, not disk. Disk is used only for optional persistence and replication logs.

### Single-Threaded Command Execution

Classic Redis executes commands on one main event-loop thread. That sounds limiting, but it avoids lock contention inside the data structures.

```
Client A ----+
Client B ----+--> Event loop --> Execute command atomically --> Reply
Client C ----+
```

Because each command runs to completion before the next command, commands like `INCR`, `SETNX`, `HINCRBY`, and `ZADD` are atomic.

Modern Redis can use additional threads for network I/O and background work, but command execution is still conceptually serialized for a single shard.

### Efficient Data Structures

Redis data types are implemented with compact encodings for small values and specialized structures for large values. For example, small hashes may use memory-efficient listpack encodings before converting to hash tables.

### Non-Blocking I/O

Redis uses an event loop to handle many client connections without one thread per connection.

### Important Performance Reality

Redis is not magic. Common ways to make it slow:

- Very large values, such as multi-MB JSON blobs
- Commands with large time complexity, such as `KEYS *` in production
- Hot keys receiving extreme traffic on one shard
- Too many client connections without pooling
- Network latency from cross-region access
- Memory pressure causing eviction or swapping
- Slow Lua scripts blocking the event loop

---

## Core Data Structures

### Strings

Strings store bytes. They are used for cached objects, counters, flags, tokens, and simple values.

```bash
SET user:42:name "Asha"
GET user:42:name

SET session:abc123 "{json}" EX 3600
INCR page:view:home
INCRBY inventory:sku123 -1
```

Common string patterns:

- Cache object: `cache:user:42 -> JSON`
- Counter: `views:post:99 -> 12345`
- Idempotency record: `idempotency:request-id -> response metadata`
- Feature flag: `flag:new_checkout -> enabled`

### Hashes

Hashes store field-value maps under one key.

```bash
HSET user:42 name "Asha" plan "pro" city "Bengaluru"
HGET user:42 plan
HGETALL user:42
HINCRBY user:42 login_count 1
```

Use hashes when you often read or update individual fields of an object.

Trade-off:

- One large JSON string is simpler.
- A hash avoids rewriting the whole object for field updates.

### Lists

Lists are ordered collections, useful for simple queues and recent activity.

```bash
LPUSH jobs:email '{"to":"a@example.com"}'
BRPOP jobs:email 5

LPUSH user:42:recent_items item123
LTRIM user:42:recent_items 0 49
LRANGE user:42:recent_items 0 9
```

Use lists for simple FIFO/LIFO queues. Prefer Streams for durable consumer groups.

### Sets

Sets store unique unordered values.

```bash
SADD post:99:likes user1 user2 user3
SISMEMBER post:99:likes user2
SCARD post:99:likes
SINTER group:a group:b
```

Use sets for membership, deduplication, tags, and social graph intersections.

### Sorted Sets

Sorted sets store unique members with scores and keep them ordered.

```bash
ZADD leaderboard 9800 user42
ZADD leaderboard 9900 user7
ZREVRANGE leaderboard 0 9 WITHSCORES
ZRANK leaderboard user42
ZINCRBY leaderboard 10 user42
```

Use sorted sets for:

- Leaderboards
- Priority queues
- Time-based indexes
- Top-N rankings
- Delayed jobs

Example delayed job:

```bash
ZADD delayed_jobs 1710000000 job-123
ZRANGEBYSCORE delayed_jobs -inf 1710000000 LIMIT 0 100
ZREM delayed_jobs job-123
```

### Bitmaps

Bitmaps store bit-level flags inside strings.

```bash
SETBIT active:2026-05-23 42 1
GETBIT active:2026-05-23 42
BITCOUNT active:2026-05-23
```

Use for compact daily active user tracking when user IDs are dense.

### HyperLogLog

HyperLogLog estimates cardinality with fixed memory.

```bash
PFADD unique:visitors:2026-05-23 user1 user2 user3
PFCOUNT unique:visitors:2026-05-23
```

Use when approximate unique counts are acceptable.

### Streams

Streams are append-only logs with IDs and consumer groups.

```bash
XADD orders * order_id 123 amount 4999
XGROUP CREATE orders payment-workers $ MKSTREAM
XREADGROUP GROUP payment-workers worker-1 COUNT 10 BLOCK 5000 STREAMS orders >
XACK orders payment-workers 1710000000000-0
```

Use Streams for reliable event processing within Redis. Kafka is usually better for long retention, high fan-out, and large event-streaming platforms.

---

## Caching Patterns

### Cache-Aside

The application controls cache reads and writes.

```
Read request
    |
    v
GET key from Redis
    |
    +-- hit --> return value
    |
    +-- miss --> read DB --> SET key with TTL --> return value
```

Pseudo-code:

```python
def get_user(user_id):
    key = f"user:{user_id}"
    cached = redis.get(key)
    if cached:
        return deserialize(cached)

    user = db.query("SELECT * FROM users WHERE id = ?", user_id)
    redis.set(key, serialize(user), ex=300)
    return user
```

Pros:

- Simple
- Cache only stores data that is actually read
- Works with any database

Cons:

- First read after expiry is slow
- Easy to serve stale data if invalidation is wrong

### Read-Through Cache

The cache layer knows how to load from the database. The application asks only the cache.

Pros:

- Cleaner application code
- Centralized loading behavior

Cons:

- More infrastructure complexity
- Less common with plain Redis unless wrapped by a library/service

### Write-Through Cache

Writes go to cache and database synchronously.

```
Write request -> cache -> database -> success
```

Pros:

- Cache stays fresh
- Reads after writes are consistent

Cons:

- Higher write latency
- Cache now participates in write availability

### Write-Behind Cache

Writes go to cache first, then asynchronously flush to database.

Pros:

- Very fast writes
- Can batch database writes

Cons:

- Risk of data loss if Redis fails before flush
- Harder recovery and ordering semantics

Use write-behind only when temporary data loss is acceptable or you have a durable log.

### Refresh-Ahead

Before a hot key expires, refresh it asynchronously.

Useful when:

- A small set of keys gets heavy traffic
- Cache miss latency is expensive
- The data can tolerate slight staleness

### Negative Caching

Cache "not found" results briefly to prevent repeated database misses.

```bash
SET user:999:not_found 1 EX 30
```

Use a short TTL because the object may be created soon.

---

## Eviction and Expiration

### Expiration

Redis can expire keys automatically.

```bash
SET session:abc "{json}" EX 3600
EXPIRE user:42 300
TTL session:abc
```

Expiration is not an exact timer. Redis removes expired keys using:

- Lazy expiration: key is checked when accessed
- Active expiration: Redis samples keys with TTLs and deletes expired ones

### Eviction

Expiration is about key TTLs. Eviction is about memory pressure.

When `maxmemory` is reached, Redis can evict keys based on policy.

Common policies:

| Policy | Meaning |
| --- | --- |
| `noeviction` | Return errors on writes when memory is full |
| `allkeys-lru` | Evict least recently used keys from all keys |
| `volatile-lru` | Evict least recently used keys only among keys with TTL |
| `allkeys-lfu` | Evict least frequently used keys from all keys |
| `volatile-ttl` | Evict keys with TTL, preferring shorter TTL |
| `allkeys-random` | Random eviction from all keys |

For general caching, `allkeys-lru` or `allkeys-lfu` is common.

For Redis as a store of important data, avoid eviction and size memory carefully.

### Cache Stampede

A cache stampede happens when a hot key expires and many requests hit the database simultaneously.

Mitigations:

- Add TTL jitter so many keys do not expire together
- Use request coalescing or single-flight loading
- Use distributed lock around regeneration
- Serve stale value while refreshing in background
- Pre-warm critical keys

Example TTL jitter:

```python
ttl = 300 + random.randint(0, 60)
redis.set(key, value, ex=ttl)
```

### Cache Penetration

Attackers or bugs request keys that do not exist, causing repeated DB misses.

Mitigations:

- Negative caching
- Bloom filters
- Input validation
- Rate limiting

### Cache Avalanche

Many keys expire at the same time, overwhelming the database.

Mitigations:

- TTL jitter
- Staggered warmups
- Bulk refresh jobs
- Backpressure and rate limiting

---

## Persistence

Redis is in-memory, but it can persist data to disk.

### RDB Snapshots

RDB periodically writes a compact snapshot of the dataset.

```
Memory dataset -> background fork -> dump.rdb
```

Pros:

- Compact files
- Fast restart from snapshot
- Good for backups

Cons:

- Can lose writes since the last snapshot
- Forking can be expensive for large datasets

Example config:

```conf
save 900 1
save 300 10
save 60 10000
```

### AOF (Append Only File)

AOF logs write commands.

```
SET a 1
INCR counter
HSET user:42 name Asha
```

Append fsync policies:

| Policy | Durability | Performance |
| --- | --- | --- |
| `always` | Strongest | Slowest |
| `everysec` | Lose up to about 1 second | Good default |
| `no` | OS decides | Fast but weaker |

Pros:

- Better durability than snapshots
- Human-readable-ish command log

Cons:

- Larger files
- Requires rewrite/compaction

### RDB + AOF

Many production deployments use AOF for better durability and RDB for backup/snapshot convenience.

### Persistence Interview Answer

If Redis is only a cache:

- Persistence may be disabled.
- Losing Redis data is acceptable because the DB is source of truth.

If Redis stores sessions, rate counters, or queues:

- Enable AOF `everysec`.
- Use replication.
- Define acceptable data loss.

If Redis is the primary store:

- Be explicit about durability limits.
- Use backups, replicas, monitoring, and restore drills.
- Consider whether a real durable database is a better fit.

---

## Replication and High Availability

### Primary-Replica Replication

```
Writes
  |
  v
+---------+       async replication       +----------+
| Primary | ----------------------------> | Replica1 |
+---------+                               +----------+
     |
     +-----------------------------------> +----------+
                                          | Replica2 |
                                          +----------+
```

Writes go to the primary. Replicas asynchronously receive updates and can serve reads.

Pros:

- Read scaling
- Failover target
- Backup without loading primary

Cons:

- Replication lag
- Replicas can return stale data
- Primary failure needs failover mechanism

### Replication Lag

Lag matters when users read immediately after write.

Mitigations:

- Read-your-writes from primary for a short window
- Route user-affine reads to the same region/shard
- Use `WAIT` for stronger replication acknowledgement when needed

Example:

```bash
WAIT 1 100
```

This asks Redis to wait until at least one replica acknowledges the write or 100 ms passes. It improves safety but does not make Redis a fully synchronous database.

### Sentinel

Redis Sentinel provides monitoring and automatic failover for primary-replica deployments.

```
          +-----------+
          | Sentinel  |
          +-----------+
          /     |     \
         v      v      v
   Primary   Replica  Replica
```

Sentinel responsibilities:

- Monitor primary and replicas
- Detect primary failure
- Elect a new primary
- Notify clients of the new primary

Sentinel is for high availability, not sharding.

### Failover Risks

Because replication is asynchronous, failover can lose acknowledged writes that were on the old primary but not replicated yet.

In an interview, say this clearly:

"Redis failover improves availability, but unless I wait for replica acknowledgements or design idempotent recovery, I may lose the most recent writes."

---

## Redis Cluster and Sharding

Redis Cluster shards keys across multiple primary nodes.

### Hash Slots

Redis Cluster has 16,384 hash slots. Each key maps to one slot.

```
key -> CRC16(key) % 16384 -> slot -> primary node
```

Example:

```
Slots 0-5460      -> Node A
Slots 5461-10922  -> Node B
Slots 10923-16383 -> Node C
```

Each primary can have replicas.

```
+-----------+     +-----------+     +-----------+
| Primary A |     | Primary B |     | Primary C |
| slots 0.. |     | slots ... |     | slots ... |
+-----+-----+     +-----+-----+     +-----+-----+
      |                 |                 |
      v                 v                 v
  Replica A         Replica B         Replica C
```

### Hash Tags

Multi-key operations require keys to be in the same hash slot. Hash tags force this.

```bash
user:{42}:profile
user:{42}:settings
user:{42}:sessions
```

Only the text inside `{}` is hashed, so these keys land in the same slot.

### Cluster Trade-Offs

Pros:

- Horizontal scaling
- Automatic partitioning
- Built-in failover with replicas

Cons:

- Multi-key operations are constrained
- Client must understand cluster redirects
- Resharding adds operational complexity
- Hot keys can still overload one shard

### Hot Key Problem

A hot key receives disproportionate traffic.

Examples:

- `homepage_config`
- `celebrity:profile`
- `global_rate_limit`

Mitigations:

- Local in-process cache for very hot read-only values
- Key splitting: `counter:global:0..N`
- Read replicas
- CDN or edge caching where applicable
- Avoid putting all global state behind one key

---

## Transactions, Lua, and Atomicity

### Atomic Single Commands

Redis commands are atomic for a single shard.

```bash
INCR api:user:42:minute:1710000
```

No two clients can interleave inside that command.

### MULTI/EXEC

`MULTI` queues commands and `EXEC` runs them sequentially.

```bash
MULTI
INCR account:1:balance
DECR account:2:balance
EXEC
```

Important: Redis transactions do not roll back like SQL transactions if a command fails during execution. They are atomic in ordering, not full ACID transactions.

### WATCH for Compare-and-Set

`WATCH` detects if keys changed before `EXEC`.

```bash
WATCH inventory:item42
GET inventory:item42
MULTI
DECR inventory:item42
EXEC
```

If another client changes the watched key before `EXEC`, the transaction aborts.

### Lua Scripts

Lua scripts run atomically on the Redis server.

Example token bucket-ish rate limiter:

```lua
local key = KEYS[1]
local limit = tonumber(ARGV[1])
local window = tonumber(ARGV[2])

local current = tonumber(redis.call("GET", key) or "0")
if current >= limit then
  return 0
end

current = redis.call("INCR", key)
if current == 1 then
  redis.call("EXPIRE", key, window)
end
return 1
```

Use Lua when multiple Redis operations must behave as one atomic operation.

Be careful:

- Long scripts block the server.
- Scripts should be deterministic.
- In cluster mode, script keys must usually be on one slot.

---

## Streams, Pub/Sub, and Queues

### Pub/Sub

Redis Pub/Sub is fire-and-forget messaging.

```bash
SUBSCRIBE notifications
PUBLISH notifications "hello"
```

Use for:

- Live notifications
- Cache invalidation messages
- Lightweight fan-out

Do not use for durable job processing because messages are lost if subscribers are offline.

### Lists as Queues

Simple queue:

```bash
LPUSH jobs job1
BRPOP jobs 0
```

Pros:

- Very simple
- Blocking pop support

Cons:

- Weak visibility timeout semantics
- Harder retries and consumer groups
- No built-in event history

### Reliable Queue with Lists

Move job from pending to processing atomically:

```bash
BRPOPLPUSH jobs pending_jobs 0
```

Worker later removes from `pending_jobs` after success. A reaper can retry stale pending jobs.

This works, but Redis Streams are usually cleaner.

### Streams

Streams provide:

- Append-only event log
- Consumer groups
- Pending entries list
- Acknowledgements
- Replay from IDs

```
Producer -> XADD orders -> Redis Stream -> consumer group -> workers
```

Use Streams when you need reliable Redis-native event processing.

Kafka is better when you need:

- Longer retention
- Large-scale fan-out
- Replay by many independent services
- Strong ecosystem for stream processing

---

## Distributed Locks

### Basic Lock

Use `SET key value NX PX ttl`.

```bash
SET lock:order:123 request-uuid NX PX 30000
```

Meaning:

- `NX`: set only if key does not exist
- `PX 30000`: expire after 30 seconds
- value is a unique owner token

Release safely with Lua:

```lua
if redis.call("GET", KEYS[1]) == ARGV[1] then
  return redis.call("DEL", KEYS[1])
else
  return 0
end
```

Never release a lock without checking the owner token. Otherwise one client can delete another client's lock after a timeout.

### Lock Failure Modes

Distributed locks are easy to misuse.

Failure scenarios:

- Worker pauses longer than lock TTL and continues work after lock expires
- Network partition makes client think it owns a lock
- Redis primary fails before lock replicates
- Clock assumptions break timeout logic

Mitigations:

- Keep lock TTL short and bounded by work time
- Use fencing tokens for writes to external systems
- Make operations idempotent
- Prefer database constraints when protecting database state

### Fencing Tokens

A fencing token is a monotonically increasing number returned with the lock.

```
Client A gets token 41
Client B later gets token 42
Storage rejects writes with token < latest_seen_token
```

This protects against stale lock owners continuing work after their lock expired.

---

## System Design Patterns

### Pattern 1: User Session Store

```
session:{token} -> { user_id, role, expires_at }
TTL: 30 minutes
```

Design choices:

- Store only minimal session metadata
- Use HTTPS-only secure cookies for token transport
- Expire idle sessions with TTL
- Replicate Redis for availability
- Decide whether logout must invalidate globally immediately

### Pattern 2: API Rate Limiting

Fixed window:

```bash
INCR rate:user:42:202605231030
EXPIRE rate:user:42:202605231030 60
```

Better implementation uses Lua to make increment and expiry atomic.

For more detail, see `09_rate_limiting.md`.

### Pattern 3: Leaderboard

```
ZADD game:season:2026 12345 user42
ZREVRANGE game:season:2026 0 99 WITHSCORES
ZREVRANK game:season:2026 user42
```

Scaling concerns:

- One global leaderboard may become a hot key
- Partition by region/game/season
- Cache top pages separately
- Archive old seasons

### Pattern 4: Recently Viewed Items

```
LPUSH user:42:recent item99
LREM user:42:recent 0 item99
LTRIM user:42:recent 0 49
```

Need uniqueness? Remove before push:

```bash
LREM user:42:recent 0 item99
LPUSH user:42:recent item99
LTRIM user:42:recent 0 49
```

Use Lua if the operations must be atomic.

### Pattern 5: Cache Invalidation

On database write:

```
1. Write database transaction
2. Delete cache key
3. Next read repopulates cache
```

Why delete instead of update?

- DB remains source of truth
- Avoids partial update bugs
- Works when cached value aggregates multiple tables

Race to know:

```
Reader misses cache -> reads old DB value
Writer updates DB -> deletes cache
Reader sets old value into cache
```

Mitigations:

- Short TTL
- Versioned cache keys
- Delayed double delete
- Write-through for strict use cases
- Database change data capture invalidation

### Pattern 6: Idempotency Keys

For payment or order APIs:

```bash
SET idempotency:merchant1:req123 "processing" NX EX 86400
```

If set succeeds, process request. If it fails, return the stored result or current state.

Store final response:

```
idempotency:merchant1:req123 -> { status: "success", payment_id: "pay_123" }
```

For critical payments, persist idempotency in the database too. Redis can accelerate but should not be the only source of truth.

---

## Operational Playbook

### Key Naming

Use consistent namespaces:

```
user:{id}:profile
user:{id}:sessions
rate:{tenant}:{user}:{window}
cache:product:{id}
lock:order:{id}
```

Guidelines:

- Include entity type and ID
- Avoid unbounded key names
- Use hash tags intentionally in cluster mode
- Document TTL expectations

### Memory Sizing

Estimate:

```
memory = number_of_keys * (avg_key_size + avg_value_size + Redis_overhead)
```

Redis overhead can be significant. A 20-byte value does not cost only 20 bytes. Test with realistic data.

Commands:

```bash
INFO memory
MEMORY USAGE user:42
MEMORY STATS
```

### Commands to Avoid in Production

Avoid:

```bash
KEYS *
FLUSHALL
SMEMBERS huge:set
LRANGE huge:list 0 -1
HGETALL huge:hash
```

Prefer incremental scanning:

```bash
SCAN 0 MATCH cache:user:* COUNT 100
SSCAN myset 0 COUNT 100
HSCAN myhash 0 COUNT 100
```

### Monitoring Metrics

Watch:

- `used_memory`
- `maxmemory`
- `evicted_keys`
- `expired_keys`
- `keyspace_hits` and `keyspace_misses`
- cache hit ratio
- `connected_clients`
- `blocked_clients`
- `instantaneous_ops_per_sec`
- replication lag
- `aof_last_write_status`
- `rdb_last_bgsave_status`
- command latency percentiles

Cache hit ratio:

```
hits / (hits + misses)
```

### Debugging Latency

```bash
SLOWLOG GET 20
LATENCY DOCTOR
INFO commandstats
```

Common causes:

- Large values
- Slow Lua scripts
- Fork latency during RDB/AOF rewrite
- Network latency
- Memory fragmentation
- Swapping

### Security Basics

- Bind Redis to private network interfaces
- Require authentication
- Use TLS when traffic crosses untrusted networks
- Disable dangerous commands or rename them
- Use ACLs for least privilege
- Never expose Redis directly to the public internet

---

## Hands-On Exercises

### Exercise 1: Basic Cache-Aside

Run Redis:

```bash
docker run --rm --name redis -p 6379:6379 redis:7
```

In another shell:

```bash
redis-cli
SET user:1 '{"id":1,"name":"Asha"}' EX 60
GET user:1
TTL user:1
```

What to learn:

- Key expiry
- JSON-as-string storage
- TTL behavior

### Exercise 2: Atomic Counter with Expiry

```bash
INCR rate:user:42:minute:1
EXPIRE rate:user:42:minute:1 60
TTL rate:user:42:minute:1
```

Question: what race exists if the process crashes after `INCR` but before `EXPIRE`?

Answer: the key may never expire. Fix with Lua or `MULTI/EXEC`.

### Exercise 3: Leaderboard

```bash
ZADD leaderboard 100 user1 300 user2 250 user3
ZREVRANGE leaderboard 0 2 WITHSCORES
ZREVRANK leaderboard user3
ZINCRBY leaderboard 100 user1
```

Explain why sorted sets are better than sorting in the application for every request.

### Exercise 4: Simple Queue

Producer:

```bash
LPUSH jobs:email '{"id":1,"to":"a@example.com"}'
```

Consumer:

```bash
BRPOP jobs:email 0
```

Then discuss what happens if the consumer crashes after popping but before processing.

### Exercise 5: Streams Consumer Group

```bash
XADD orders * order_id 1 amount 100
XGROUP CREATE orders workers 0 MKSTREAM
XREADGROUP GROUP workers worker-1 COUNT 1 STREAMS orders >
XACK orders workers <message-id>
```

Then inspect pending messages:

```bash
XPENDING orders workers
```

### Exercise 6: Safe Lock Release

Acquire:

```bash
SET lock:job:1 abc123 NX PX 30000
```

Release with Lua owner check. Explain why `DEL lock:job:1` is unsafe.

---

## Interview Questions

### Basic Questions

**Q: What is Redis used for?**

Redis is an in-memory data store used for caching, sessions, counters, queues, leaderboards, rate limiting, Pub/Sub, and lightweight coordination.

**Q: Why is Redis fast?**

It keeps data in memory, uses efficient data structures, uses an event-loop architecture, and avoids lock contention by serializing command execution per shard.

**Q: What is the difference between Redis and Memcached?**

Memcached is a simple distributed cache for string values. Redis supports richer data structures, persistence, replication, Lua scripting, streams, sorted sets, and more coordination patterns.

**Q: Is Redis single-threaded?**

Redis command execution is primarily single-threaded per shard, which gives simple atomicity. Modern Redis can use extra threads for I/O and background work.

### Caching Questions

**Q: Explain cache-aside.**

The app checks cache first. On miss, it reads the database, stores the result in cache with TTL, then returns it. Writes update the DB and usually invalidate the cache.

**Q: How do you prevent cache stampede?**

Use TTL jitter, single-flight loading, distributed locks, refresh-ahead, or stale-while-revalidate.

**Q: What is negative caching?**

Caching "not found" responses for a short TTL so repeated requests for missing data do not keep hitting the database.

**Q: How do you handle cache consistency?**

Treat DB as source of truth, delete cache after DB writes, use short TTLs, use versioned keys or CDC invalidation for stricter cases, and design reads to tolerate bounded staleness.

### Scaling Questions

**Q: How do you scale Redis reads?**

Use replicas, local in-process caches for ultra-hot values, split hot keys, and ensure clients can tolerate stale reads.

**Q: How do you scale Redis writes?**

Shard with Redis Cluster, partition by tenant/entity, avoid hot keys, and use batching/pipelining where safe.

**Q: What is Redis Cluster?**

A Redis deployment that partitions keys across 16,384 hash slots assigned to primary nodes, with optional replicas for failover.

**Q: What is a hot key and how do you fix it?**

A hot key receives too much traffic for one shard. Fix with local caching, key splitting, read replicas, request coalescing, or moving the workload to a better architecture.

### Reliability Questions

**Q: What happens if Redis goes down?**

If it is only a cache, the system should fall back to the database, possibly with rate limiting to protect it. If Redis stores sessions or queues, users may be logged out or jobs may be delayed/lost depending on persistence and replication.

**Q: Does Redis replication guarantee no data loss?**

No. Replication is usually asynchronous, so a primary failure can lose writes that were not replicated. `WAIT` can reduce risk but does not turn Redis into a fully synchronous database.

**Q: RDB vs AOF?**

RDB is point-in-time snapshots: compact and fast to load but can lose recent writes. AOF logs write commands: better durability but more disk and rewrite overhead.

**Q: How would you use Redis for distributed locks?**

Use `SET lock value NX PX ttl` with a unique owner token and release via Lua only if the token matches. For critical correctness, add fencing tokens or prefer database constraints.

### Design Deep Dives

**Q: Design a rate limiter with Redis.**

Use atomic counters with TTL for fixed window, sorted sets for sliding logs, or Lua for token bucket. Include key design, TTL, cluster shard strategy, failure behavior, and what happens when Redis is unavailable.

**Q: Design a leaderboard.**

Use sorted sets per game/season/region. `ZADD` updates score, `ZREVRANGE` gets top N, `ZREVRANK` gets rank. Handle hot global boards with partitioning, cached top pages, and archived seasons.

**Q: Design cache invalidation for product details.**

Read path uses cache-aside. Write path updates DB then deletes `product:{id}`. Use TTL as backstop. If product pages aggregate inventory/prices/reviews, either invalidate all related keys or use versioned composition keys.

---

## Quick Reference

### Data Type Selection

| Need | Redis type |
| --- | --- |
| Simple value/cache/counter | String |
| Object fields | Hash |
| Queue or recent list | List |
| Unique membership | Set |
| Ranking by score | Sorted set |
| Approx unique count | HyperLogLog |
| Compact boolean flags | Bitmap |
| Durable-ish event log | Stream |

### Essential Commands

```bash
# strings
SET key value EX 60
GET key
INCR counter

# hashes
HSET user:1 name Asha plan pro
HGET user:1 name

# sets
SADD set a b c
SISMEMBER set a

# sorted sets
ZADD leaderboard 100 user1
ZREVRANGE leaderboard 0 9 WITHSCORES

# expiry
EXPIRE key 60
TTL key

# scanning
SCAN 0 MATCH cache:* COUNT 100

# diagnostics
INFO
SLOWLOG GET 10
MEMORY USAGE key
```

### Interview Checklist

When Redis appears in a design, always mention:

1. What Redis stores and what the source of truth is
2. TTL and invalidation strategy
3. Memory sizing and eviction policy
4. Failure behavior when Redis is unavailable
5. Replication, failover, and possible data loss
6. Hot key and sharding strategy
7. Metrics to monitor in production
