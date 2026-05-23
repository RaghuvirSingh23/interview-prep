# HLD: URL Shortener (TinyURL)

## Requirements

**Functional:**

- Generate a short link given a long URL (+ optional custom alias, optional TTL)
- Redirect to original URL when short link is visited
- Analytics: track click count per short link

**Non-Functional:**

- Highly available (reads can't go down)
- Low latency redirects (< 50ms p99)
- 100M new URLs/month (~40 writes/sec)
- 10B reads/month (~3,800 reads/sec)
- Default expiry: 5 years

---

## Capacity Estimation

**Storage per record:** ~200 bytes

- short_code: 7B
- long_url: 100B (avg)
- metadata (timestamps, counter): ~93B

**Total over 5 years:**

- 100M/month × 60 months = 6 billion URLs
- 6B × 200B = 1.2 TB

**Bandwidth:**

- Writes: 40/sec × 200B = 8 KB/s (negligible)
- Reads: 3,800/sec × 200B = 760 KB/s (negligible)

Storage and bandwidth are easy. The challenge is read latency at scale.

---

## Short Code Generation

### Character set: base62

`[a-zA-Z0-9]` = 62 characters


| Length | Unique codes        |
| ------ | ------------------- |
| 6      | 62^6 = 56 billion   |
| 7      | 62^7 = 3.5 trillion |


We need 6B codes over 5 years. **7 characters** gives us 3.5 trillion — plenty
of headroom. 6 characters (56B) also works but is tighter.

### How to generate unique codes

**Option 1: Hash + Truncate**

```
MD5(long_url) → 128-bit hash → take first 43 bits → base62 encode → 7 chars
```

Problem: collisions. Two different URLs could hash to the same 7 chars.
Fix: on collision, check DB and retry with a salt (append counter).

```
Attempt 1: base62(MD5("https://example.com"))[:7]      → "a8Kx3Lm"
Collision! →
Attempt 2: base62(MD5("https://example.com" + "1"))[:7] → "bR4nQ2x"
```

Pros: Same long URL always generates the same short code (deduplication).
Cons: Collision handling adds complexity. DB read before every write.

**Option 2: Counter-based (preferred)**

A global counter increments for each new URL. Convert counter to base62.

```
Counter: 1000000  →  base62  →  "4c92"
Counter: 1000001  →  base62  →  "4c93"
```

No collisions, ever. But how do you distribute the counter across servers?

**Range-based allocation:**

- A coordination service (Zookeeper or a simple DB table) hands out ranges
- Server A gets range [1M, 2M)
- Server B gets range [2M, 3M)
- Each server increments locally within its range. No coordination per request.
- When a server exhausts its range, it requests a new one.

```
┌──────────────┐
│  Zookeeper   │  Hands out counter ranges
│  / DB table  │  
└──────┬───────┘
       │
   ┌───┴───────────────────┐
   │                       │
   ▼                       ▼
┌──────────┐         ┌──────────┐
│ Server A │         │ Server B │
│ range:   │         │ range:   │
│ 1M - 2M  │         │ 2M - 3M  │
│ current: │         │ current: │
│ 1,000,042│         │ 2,000,017│
└──────────┘         └──────────┘
```

Pros: Zero collisions, no DB read before write, very fast.
Cons: Codes are sequential (predictable). Attackers can guess the next
code and enumerate all shortened URLs.

**Fix: Counter + Format-Preserving Encryption (FPE)**

Before base62-encoding, encrypt the counter with a secret key using a
Feistel cipher. The output looks random but is still 1-to-1 (no collisions).

```
Counter: 1000000  →  FPE(key)  →  7391842  →  base62  →  "kR4nQ2"
Counter: 1000001  →  FPE(key)  →  2058116  →  base62  →  "bX9mLp"
Counter: 1000002  →  FPE(key)  →  5823467  →  base62  →  "hT1wYz"
```

Sequential inputs, random-looking outputs. Zero collisions. Not guessable
without the key. This is the industry-standard approach.

**Option 3: Snowflake-style IDs**

Twitter Snowflake: 64-bit ID = timestamp + machine ID + sequence.
Encode the 64-bit ID to base62 → ~11 chars. Longer than ideal but
globally unique without coordination.

### Chosen approach: Counter + range allocation + FPE

Best balance of simplicity, speed, zero collisions, and unpredictability.

---

## API Design

```
POST /api/shorten
Body: { "long_url": "https://...", "custom_alias": "my-link", "ttl_days": 365 }
Response: { "short_url": "https://tiny.url/a8Kx3Lm", "expires_at": "..." }

GET /{short_code}
Response: HTTP 301 (permanent redirect)
          Location: https://original-long-url.com/path

GET /api/stats/{short_code}
Response: { "long_url": "...", "clicks": 42091, "created_at": "..." }
```

**301 vs 302:**


| Code | Meaning           | Behavior                            |
| ---- | ----------------- | ----------------------------------- |
| 301  | Moved Permanently | Browser caches it. Next visit skips |
|      |                   | our server entirely.                |
| 302  | Found (Temporary) | Browser always hits our server.     |


Use **302** if we want analytics (we need to see every click).
Use **301** if we want to reduce server load (browser handles it).

Best of both: use **302** so we capture every click for analytics.

---

## Database Schema

```sql
CREATE TABLE url_mappings (
    short_code   VARCHAR(7) PRIMARY KEY,
    long_url     TEXT NOT NULL,
    created_at   TIMESTAMP DEFAULT NOW(),
    expires_at   TIMESTAMP,
    click_count  BIGINT DEFAULT 0
);

CREATE INDEX idx_expires ON url_mappings(expires_at);
```

**SQL vs NoSQL?**

Either works. The data model is simple key-value (short_code → long_url).

SQL (Postgres):

- Strong consistency on writes (no duplicate short codes)
- ACID transactions
- Familiar, well-tooled

NoSQL (DynamoDB / Cassandra):

- Built for key-value lookups at massive scale
- Easier horizontal scaling
- Eventually consistent (fine for reads here)

For this scale (3,800 reads/sec), a single Postgres with read replicas
handles it easily. If scaling to 100x, switch to DynamoDB.

---

## High-Level Architecture

```
                    ┌──────────────┐
                    │   Clients    │
                    │ (browsers)   │
                    └──────┬───────┘
                           │
                           ▼
                    ┌──────────────┐
                    │ Load Balancer│
                    └──────┬───────┘
                           │
              ┌────────────┼────────────┐
              ▼            ▼            ▼
        ┌──────────┐ ┌──────────┐ ┌──────────┐
        │ App Srv 1│ │ App Srv 2│ │ App Srv 3│
        └────┬─────┘ └────┬─────┘ └────┬─────┘
             │             │             │
             └──────┬──────┘─────────────┘
                    │
              ┌─────┴──────┐
              ▼            ▼
        ┌──────────┐ ┌──────────┐
        │  Cache   │ │ Counter  │
        │ (Redis)  │ │  Service │
        └────┬─────┘ │(Zookeeper│
             │       │ or DB)   │
             │       └──────────┘
             ▼
        ┌──────────┐
        │ Database │──── Read Replicas
        │ (Postgres)│
        └──────────┘
```

### Read Path (redirect): GET /{short_code}

```
1. Browser → Load Balancer → App Server
2. App Server checks Redis cache
   - Cache HIT → return 302 redirect (done)
   - Cache MISS → query Postgres
3. Postgres returns long_url
4. App Server writes to Redis cache (TTL = 1 hour)
5. App Server returns 302 redirect
6. Async: increment click_count (batched, see below)
```

**Why cache?** Most URLs follow power-law distribution — a small % of URLs
get the vast majority of clicks. Caching hot URLs in Redis avoids DB
hits for 90%+ of reads.

**Redis sizing:** Cache the top 20% of URLs.

- 20% of 6B = 1.2B entries × 200B = 240 GB
- A Redis cluster of 3-4 nodes handles this.

### Write Path (shorten): POST /api/shorten

```
1. Browser → Load Balancer → App Server
2. If custom alias provided:
   - Check DB for uniqueness
   - If taken, return 409 Conflict
3. If no custom alias:
   - App Server takes next counter from its pre-allocated range
   - base62 encode → short_code
   - (If range exhausted, request new range from counter service)
4. INSERT into Postgres
5. Write to Redis cache
6. Return short URL
```

### Analytics (click counting)

Don't do `UPDATE click_count = click_count + 1` on every read — that's
a write on the read path, and it'll kill your DB under load.

Options:

1. **Batch in memory:** App server accumulates counts in memory, flushes
  to DB every 30 seconds.
2. **Redis INCR:** `INCR clicks:{short_code}` in Redis (atomic, fast).
  Background job syncs to Postgres periodically.
3. **Kafka + Consumer:** Each click produces a Kafka event. A consumer
  aggregates and writes to DB. Best for detailed analytics (timestamp,
   geo, referrer).

For our scale, **Redis INCR + periodic flush** is simplest.

---

## Handling Expiration

URLs expire after 5 years (or custom TTL).

**Approach: Lazy deletion + background cleanup**

1. **Lazy:** On read, check `expires_at`. If expired, return 404 and
  delete async. No wasted work on URLs nobody visits.
2. **Background job:** A cron job runs periodically:
  ```sql
   DELETE FROM url_mappings
   WHERE expires_at < NOW()
   LIMIT 10000;
  ```
   Small batches to avoid locking the table.
3. **Redis TTL:** Set Redis expiry to match. Expired entries auto-evict.

---

## Scaling & Reliability

### Database scaling

At 40 writes/sec and 3,800 reads/sec:

- Single Postgres writer handles writes easily
- Add 2-3 read replicas for read traffic
- Redis absorbs 90%+ of reads, so replicas are lightly loaded

If scaling beyond this:

- Shard by short_code (hash-based partitioning)
- Each shard handles a subset of the key space

### Availability

- **Multiple app servers** behind load balancer (any can serve any request)
- **Redis cluster** with replication (if primary fails, replica promotes)
- **Postgres failover** with streaming replication
- **Counter service:** If Zookeeper is down, servers use their remaining
pre-allocated range. Only a problem when ranges run out.

### What can go wrong?


| Failure              | Impact                    | Mitigation                              |
| -------------------- | ------------------------- | --------------------------------------- |
| App server dies      | Other servers handle load | Stateless, LB routes around it          |
| Redis dies           | Reads go to DB (slower)   | Redis cluster with replicas             |
| DB primary dies      | Writes fail temporarily   | Auto-failover to replica                |
| Counter service dies | Can't allocate new ranges | Pre-allocate large ranges (1M+)         |
| Network partition    | Some reads may be stale   | Acceptable — URL mappings rarely change |


---

## Summary Diagram (Complete)

```
                         ┌──────────────┐
                         │   Browser    │
                         └──────┬───────┘
                                │
                         ┌──────▼───────┐
                         │     DNS      │
                         │ tiny.url →   │
                         │  LB IP       │
                         └──────┬───────┘
                                │
                    ┌───────────▼───────────┐
                    │    Load Balancer      │
                    │   (round-robin)       │
                    └───────────┬───────────┘
                                │
            ┌───────────────────┼───────────────────┐
            ▼                   ▼                   ▼
      ┌───────────┐      ┌───────────┐      ┌───────────┐
      │  App Srv  │      │  App Srv  │      │  App Srv  │
      │  counter: │      │  counter: │      │  counter: │
      │  1M-2M    │      │  2M-3M    │      │  3M-4M    │
      └─────┬─────┘      └─────┬─────┘      └─────┬─────┘
            │                   │                   │
            └─────────┬─────────┘───────────────────┘
                      │
         ┌────────────┼────────────┐
         ▼            ▼            ▼
   ┌──────────┐ ┌──────────┐ ┌──────────┐
   │  Redis   │ │ Postgres │ │ Counter  │
   │  Cache   │ │ Primary  │ │ Service  │
   │ (reads)  │ │ (writes) │ │(Zookeeper│
   └──────────┘ └────┬─────┘ └──────────┘
                     │
               ┌─────┼─────┐
               ▼     ▼     ▼
           Replica Replica Replica
           (reads) (reads) (reads)
```

---

## Interview Follow-ups They'll Ask

**Q: What if the same long URL is shortened twice?**
Two choices: (a) return the same short code (deduplicate by checking
a secondary index on long_url), or (b) generate a new one each time
(simpler, and different users may want different TTLs). Choice (b) is
usually preferred.

**Q: How do you prevent abuse / rate limit?**
Rate limit by IP: max 100 shortens per hour per IP. Use a rate limiter
(token bucket in Redis). Also reject obviously malicious URLs.

**Q: What about vanity/custom URLs?**
Just check if the custom code already exists in DB. If taken, 409 Conflict.
Custom codes are stored in the same table as generated ones.

**Q: Can you make redirects even faster?**
Put a CDN in front. Configure CDN to cache 302 responses for a short TTL
(e.g., 5 min). CDN edge nodes serve redirects without hitting your servers.

**Q: How would you handle a URL that goes viral (millions of hits/sec)?**
Redis handles it — hot keys stay in cache. If a single Redis node becomes
a bottleneck, replicate the hot key across multiple Redis nodes. The app
server randomly picks which replica to read from.