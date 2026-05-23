# Rate Limiting

A comprehensive guide to rate limiting for system design interviews: algorithms, Redis implementations, distributed limits, edge vs service placement, headers, fairness, failure modes, and production trade-offs.

---

## Table of Contents

1. [What is Rate Limiting?](#what-is-rate-limiting)
2. [Why Rate Limiting Matters](#why-rate-limiting-matters)
3. [Where to Apply Rate Limits](#where-to-apply-rate-limits)
4. [Rate Limit Dimensions](#rate-limit-dimensions)
5. [Algorithms](#algorithms)
6. [Fixed Window Counter](#fixed-window-counter)
7. [Sliding Window Log](#sliding-window-log)
8. [Sliding Window Counter](#sliding-window-counter)
9. [Token Bucket](#token-bucket)
10. [Leaky Bucket](#leaky-bucket)
11. [Distributed Rate Limiting with Redis](#distributed-rate-limiting-with-redis)
12. [Hierarchical and Multi-Dimensional Limits](#hierarchical-and-multi-dimensional-limits)
13. [Response Behavior and Headers](#response-behavior-and-headers)
14. [Failure Modes](#failure-modes)
15. [Production Design Patterns](#production-design-patterns)
16. [Hands-On Exercises](#hands-on-exercises)
17. [Interview Questions](#interview-questions)
18. [Quick Reference](#quick-reference)

---

## What is Rate Limiting?

Rate limiting controls how many requests or actions a caller can perform over time.

Examples:

- 100 requests per minute per IP
- 1,000 API calls per hour per API key
- 5 login attempts per 15 minutes per account
- 10 payment attempts per minute per merchant
- 1 password reset email per minute per user

When a limit is exceeded, the system rejects, delays, queues, or degrades the request.

### Simple Mental Model

```
Request -> identify caller -> check limit -> allow or reject
```

---

## Why Rate Limiting Matters

Rate limiting protects:

- backend services from overload
- databases from traffic spikes
- expensive third-party APIs
- login endpoints from brute force
- APIs from abusive tenants
- user experience from noisy neighbors
- infrastructure costs

### Rate Limiting vs Throttling vs Quotas

| Term | Meaning |
| --- | --- |
| Rate limit | Maximum actions per time window |
| Throttling | Slowing or delaying requests |
| Quota | Larger allocation over longer period, such as monthly API calls |
| Concurrency limit | Maximum in-flight requests at once |

They are related but not identical.

---

## Where to Apply Rate Limits

### Edge / CDN / API Gateway

Good for:

- coarse IP/API-key limits
- blocking obvious abuse early
- protecting origin services

Pros:

- traffic blocked before reaching app
- centralized policy
- lower backend cost

Cons:

- less business context
- hard to enforce complex per-entity rules

### Load Balancer / Reverse Proxy

Good for:

- per-IP or per-route limits
- simple service protection

Examples:

- Nginx `limit_req`
- Envoy rate limit service
- HAProxy stick tables

### Application Service

Good for:

- per-user limits
- per-tenant limits
- business action limits
- plan-based limits

Pros:

- full context
- precise errors

Cons:

- traffic already reached service
- must be implemented consistently

### Database / Downstream Protection

Concurrency limits and backpressure near expensive dependencies.

Example:

```
only 100 concurrent DB-heavy report jobs
```

### Best Practice

Use layered rate limiting:

```
CDN/IP limit -> API gateway/API key limit -> service/business limit -> dependency concurrency limit
```

---

## Rate Limit Dimensions

Choose what identity the limit applies to.

Common dimensions:

- IP address
- user ID
- account ID
- tenant ID
- API key
- device ID
- route/endpoint
- HTTP method
- region
- organization plan

### Composite Keys

Examples:

```text
rate:ip:203.0.113.10:/login:minute:202605231030
rate:user:u123:/payments:minute:202605231030
rate:tenant:t99:all:hour:2026052310
```

### NAT Problem

Many users can share one IP address, such as office networks or mobile carriers.

Per-IP limits can accidentally block legitimate users. Use per-IP for unauthenticated endpoints, then per-user/API-key after authentication.

### IPv6 Problem

Attackers may rotate IPv6 addresses. Consider prefix-based limits for some endpoints.

### Cardinality Problem

If you create one key for every random attacker input, Redis/memory can blow up.

Mitigations:

- validate identity
- expire keys
- limit key cardinality
- use approximate structures for some abuse detection

---

## Algorithms

### Algorithm Comparison

| Algorithm | Memory | Accuracy | Burst support | Common use |
| --- | --- | --- | --- | --- |
| Fixed window | low | rough | poor at boundary | simple limits |
| Sliding window log | high | exact | good | strict low-volume limits |
| Sliding window counter | low/medium | approximate | good | API limits |
| Token bucket | low | good | configurable burst | most APIs |
| Leaky bucket | low | smooth output | limited burst | traffic shaping |

---

## Fixed Window Counter

Counts requests in fixed time windows.

Example:

```
Limit: 100/minute
Window: 10:00:00-10:00:59
```

Implementation:

```text
key = user_id + current_minute
count = INCR key
EXPIRE key 60
allow if count <= 100
```

### Redis Example

```bash
INCR rate:user:42:202605231030
EXPIRE rate:user:42:202605231030 60
```

Use Lua to make `INCR` and `EXPIRE` atomic.

### Boundary Problem

User can send 100 requests at 10:00:59 and 100 requests at 10:01:00.

That is 200 requests in 2 seconds despite a 100/min limit.

### Pros

- simple
- low memory
- fast

### Cons

- burst at window boundaries
- rough fairness

Use when exact smoothing is not critical.

---

## Sliding Window Log

Stores timestamp for each request.

For each request:

1. Remove timestamps older than window.
2. Count remaining timestamps.
3. Allow if count < limit.
4. Add current timestamp.

### Redis Sorted Set Implementation

```text
key = rate:user:42
now = current_time_ms
window_start = now - 60000

ZREMRANGEBYSCORE key -inf window_start
count = ZCARD key
if count < limit:
    ZADD key now request_id
    EXPIRE key 60
    allow
else:
    reject
```

### Pros

- precise
- no boundary burst

### Cons

- memory grows with number of requests
- more Redis operations
- expensive for very high traffic keys

Use for strict low-volume limits like login attempts.

---

## Sliding Window Counter

Approximates sliding window using current and previous fixed windows.

Example:

```
Limit: 100/min
Current window count: 40
Previous window count: 80
Current window progress: 25%

Weighted count = current + previous * (1 - progress)
               = 40 + 80 * 0.75
               = 100
```

### Pros

- smoother than fixed window
- less memory than log
- good for API rate limits

### Cons

- approximate
- more logic than fixed window

---

## Token Bucket

Token bucket allows bursts while enforcing average rate.

Concept:

- bucket holds up to `capacity` tokens
- tokens refill at `rate`
- each request consumes tokens
- reject if not enough tokens

```
capacity = 100 tokens
refill = 10 tokens/sec
request cost = 1 token
```

### Diagram

```
       refill tokens
            |
            v
      +-----------+
      |  bucket   | max capacity
      +-----------+
            |
            | consume token per request
            v
         allow/reject
```

### Why Token Bucket Is Popular

It allows short bursts but caps long-term rate.

Example:

```
User can burst 100 requests immediately,
then continues at 10 requests/sec.
```

### Stored State

For each key:

- current token count
- last refill timestamp

Pseudo-code:

```python
def allow(now, state, rate, capacity, cost=1):
    elapsed = now - state.last_refill
    tokens = min(capacity, state.tokens + elapsed * rate)

    if tokens < cost:
        return False, state

    tokens -= cost
    return True, State(tokens=tokens, last_refill=now)
```

### Pros

- burst-friendly
- memory efficient
- common API behavior

### Cons

- requires timestamp math
- distributed implementation needs atomicity

---

## Leaky Bucket

Leaky bucket smooths traffic at a constant output rate.

Concept:

- requests enter a queue/bucket
- bucket drains at fixed rate
- if bucket full, reject/drop

```
Bursty input -> bucket -> steady output
```

Use when you want traffic shaping, not just rejection.

Pros:

- smooths bursts
- protects downstream with steady rate

Cons:

- can add latency if queued
- queue overflow decisions needed

Token bucket controls whether to allow requests. Leaky bucket controls how quickly requests leave.

---

## Distributed Rate Limiting with Redis

In a single app instance, in-memory counters work. In a distributed system, multiple app instances need shared state.

```
App 1 --+
App 2 --+--> Redis rate limit state
App 3 --+
```

Redis is common because commands are fast and atomic.

### Fixed Window Lua Script

```lua
local key = KEYS[1]
local limit = tonumber(ARGV[1])
local ttl = tonumber(ARGV[2])

local current = tonumber(redis.call("GET", key) or "0")
if current >= limit then
  return {0, current}
end

current = redis.call("INCR", key)
if current == 1 then
  redis.call("EXPIRE", key, ttl)
end

return {1, current}
```

### Sliding Log Lua Sketch

```lua
local key = KEYS[1]
local now = tonumber(ARGV[1])
local window = tonumber(ARGV[2])
local limit = tonumber(ARGV[3])
local request_id = ARGV[4]

redis.call("ZREMRANGEBYSCORE", key, "-inf", now - window)
local count = redis.call("ZCARD", key)

if count >= limit then
  return {0, count}
end

redis.call("ZADD", key, now, request_id)
redis.call("PEXPIRE", key, window)
return {1, count + 1}
```

### Token Bucket Lua Sketch

```lua
local key = KEYS[1]
local now = tonumber(ARGV[1])
local rate = tonumber(ARGV[2])
local capacity = tonumber(ARGV[3])
local cost = tonumber(ARGV[4])
local ttl = tonumber(ARGV[5])

local data = redis.call("HMGET", key, "tokens", "ts")
local tokens = tonumber(data[1])
local ts = tonumber(data[2])

if tokens == nil then
  tokens = capacity
  ts = now
end

local elapsed = math.max(0, now - ts)
tokens = math.min(capacity, tokens + elapsed * rate)

if tokens < cost then
  redis.call("HMSET", key, "tokens", tokens, "ts", now)
  redis.call("PEXPIRE", key, ttl)
  return {0, tokens}
end

tokens = tokens - cost
redis.call("HMSET", key, "tokens", tokens, "ts", now)
redis.call("PEXPIRE", key, ttl)
return {1, tokens}
```

Note: be consistent with time units. If `now` is milliseconds, rate should be tokens per millisecond.

### Redis Cluster Considerations

Use hash tags when multiple keys must be touched atomically:

```text
rate:{user42}:minute
rate:{user42}:day
```

Keys with the same hash tag go to the same cluster slot.

### Local Caching for Performance

For extremely high QPS, hitting Redis for every request may be expensive.

Options:

- local token bucket with periodic global reconciliation
- allocate token batches from central store
- edge rate limiting
- probabilistic early rejection

Trade-off: local limits can allow temporary overshoot.

---

## Hierarchical and Multi-Dimensional Limits

Real systems often need multiple limits.

Example API:

- per IP: 100/min
- per user: 1,000/min
- per tenant: 100,000/min
- per endpoint: 50/min for expensive reports
- global: 1M/min

Request allowed only if all relevant limits allow it.

```
Check IP -> Check user -> Check tenant -> Check endpoint -> allow
```

### Cost-Based Limits

Not all requests cost the same.

Example:

| Request | Cost |
| --- | --- |
| GET /profile | 1 token |
| GET /search | 5 tokens |
| POST /report | 50 tokens |

Token bucket supports variable request cost naturally.

### Plan-Based Limits

```text
free: 100 req/min
pro: 1,000 req/min
enterprise: custom
```

Store plan limits in configuration and cache them carefully.

### Concurrency Limits

Rate limits control rate over time. Concurrency limits cap in-flight work.

Example:

```text
tenant_42 can run at most 3 report jobs concurrently
```

Use for expensive slow operations.

---

## Response Behavior and Headers

### HTTP Status

Use:

```text
429 Too Many Requests
```

For overloaded service rather than client-specific limit, `503 Service Unavailable` may be more appropriate.

### Headers

Common headers:

```text
X-RateLimit-Limit: 100
X-RateLimit-Remaining: 0
X-RateLimit-Reset: 1710000060
Retry-After: 30
```

Modern APIs may use:

```text
RateLimit-Limit
RateLimit-Remaining
RateLimit-Reset
```

### Error Body

Example:

```json
{
  "error": "rate_limit_exceeded",
  "message": "Too many requests. Try again later.",
  "retry_after_seconds": 30
}
```

Do not expose sensitive anti-abuse logic in detail.

### Reject vs Queue

Reject:

- good for APIs
- fast feedback
- avoids unbounded latency

Queue:

- good for background jobs
- can smooth bursts
- needs max queue size and timeout

---

## Failure Modes

### Redis Down

Question: fail open or fail closed?

Fail open:

- allow requests if limiter unavailable
- preserves availability
- risks overload/abuse

Fail closed:

- reject requests if limiter unavailable
- protects system
- can cause outage for legitimate users

Decision depends on endpoint.

Examples:

| Endpoint | Failure policy |
| --- | --- |
| public login | fail closed or strict degraded local limit |
| internal metrics ingest | fail open with safeguards |
| payment creation | fail closed or conservative local limit |
| product browsing | fail open |

### Clock Skew

Token bucket and sliding windows use time. If app servers compute time, skew can affect limits.

Mitigations:

- use Redis server time in Lua
- keep clocks synced
- use monotonic time when local

### Retry Amplification

Clients may retry after 429 too aggressively.

Mitigations:

- include `Retry-After`
- client SDK backoff
- server-side jitter
- educate API consumers

### Key Explosion

Attackers create many distinct keys.

Mitigations:

- TTL every key
- cap unauthenticated identity dimensions
- use IP/prefix before user identity exists
- monitor key cardinality

### Boundary Burst

Fixed windows allow burst at boundaries. Use token bucket or sliding window for stricter smoothing.

### Race Conditions

Multiple Redis commands can race if not atomic. Use Lua or transactions where needed.

---

## Production Design Patterns

### Pattern 1: Login Protection

Limits:

- per IP: 20 attempts / 10 minutes
- per account: 5 failed attempts / 15 minutes
- per IP + account pair: 5 / 15 minutes

Algorithm:

- sliding window log or counter

Behavior:

- return generic login failure
- add CAPTCHA or step-up auth after threshold
- alert on credential stuffing patterns

### Pattern 2: Public API Key Limit

Limits:

- per API key token bucket
- per tenant daily quota
- per endpoint cost

Placement:

- API gateway for coarse checks
- app service for business-specific checks

### Pattern 3: Expensive Report Jobs

Use:

- concurrency limit per tenant
- queue with max pending jobs
- token cost per report type

Reject or delay when too many jobs are running.

### Pattern 4: Downstream Protection

If payment provider allows 100 req/sec:

- global token bucket at 90 req/sec
- per-merchant limits
- queue non-urgent retries
- circuit breaker on provider failures

### Pattern 5: Multi-Region Rate Limiting

Options:

1. Regional limits only: fast but global overshoot possible.
2. Central global limiter: accurate but higher latency and regional dependency.
3. Token allocation per region: compromise.

Example:

```
Global tenant limit: 10,000/min
Region A allocated: 5,000
Region B allocated: 3,000
Region C allocated: 2,000
```

Rebalance allocations periodically.

---

## Hands-On Exercises

### Exercise 1: Fixed Window in Redis

Run Redis:

```bash
docker run --rm --name redis -p 6379:6379 redis:7
```

Try:

```bash
redis-cli INCR rate:user:1:minute:1
redis-cli EXPIRE rate:user:1:minute:1 60
redis-cli TTL rate:user:1:minute:1
```

Explain why Lua is safer than separate `INCR` and `EXPIRE`.

### Exercise 2: Sliding Log with Sorted Set

```bash
redis-cli ZADD rate:user:1 1000 req1
redis-cli ZADD rate:user:1 2000 req2
redis-cli ZREMRANGEBYSCORE rate:user:1 -inf 1500
redis-cli ZCARD rate:user:1
```

Explain memory growth with high request rates.

### Exercise 3: Token Bucket Simulation

Given:

```
capacity = 10
refill = 1 token/sec
starting tokens = 10
```

User sends 15 requests at time 0. Then waits 3 seconds and sends 4 requests.

Calculate allowed/rejected counts.

### Exercise 4: Choose Algorithms

Pick algorithms for:

- login attempts
- public API requests
- sending password reset emails
- report generation jobs
- payment provider calls

Explain each choice.

### Exercise 5: Design Headers

For a limit of 100/min and user has used 100, return:

- status code
- headers
- body

### Exercise 6: Multi-Region Overshoot

Global limit is 1,000 requests/min. There are 3 regions using local Redis only.

How much can the system overshoot? How would you reduce overshoot?

---

## Interview Questions

### Basic Questions

**Q: What is rate limiting?**

Controlling how many requests/actions a caller can perform over time to protect services, prevent abuse, enforce fairness, and manage cost.

**Q: Where should rate limiting happen?**

At multiple layers: CDN/API gateway for coarse limits, application for business-specific limits, and dependency-level concurrency limits for expensive resources.

**Q: What HTTP status code for rate limit exceeded?**

`429 Too Many Requests`, usually with `Retry-After` and rate limit headers.

### Algorithm Questions

**Q: Fixed window vs sliding window?**

Fixed window is simple but allows bursts at boundaries. Sliding window smooths limits and can be exact with logs or approximate with counters.

**Q: Token bucket vs leaky bucket?**

Token bucket allows bursts up to bucket capacity while enforcing average rate. Leaky bucket smooths output at a steady rate and may queue/drop overflow.

**Q: Which algorithm would you use for public API limits?**

Token bucket is a strong default because it supports bursts and average rate limits with small state.

**Q: Which algorithm for login attempts?**

Sliding window log/counter because limits are low volume and stricter precision helps security.

### Distributed Systems Questions

**Q: How do you implement distributed rate limiting?**

Use a shared atomic store such as Redis. Store counters/token state by key and update atomically with Lua. For very high QPS, combine edge/local limits with central coordination.

**Q: What happens if Redis is unavailable?**

Choose fail open or fail closed based on endpoint risk. Critical abuse-sensitive endpoints may fail closed or use conservative local limits; low-risk browsing may fail open.

**Q: How do you handle multi-region rate limits?**

Use regional local limits for low latency with possible overshoot, central global limiter for accuracy, or allocate token budgets per region as a compromise.

**Q: How do you avoid race conditions?**

Use atomic Redis commands, Lua scripts, or transactions. Do not separate read/check/update into non-atomic operations.

### Design Questions

**Q: Design a rate limiter for an API gateway.**

Identify caller by API key/IP, use token bucket per key and route, store state in Redis with Lua, return 429 with headers, monitor allowed/rejected counts, layer tenant quotas, define Redis failure policy, and add local fallback for resilience.

**Q: Design rate limiting for login.**

Track attempts per IP, per account, and per IP-account pair. Use sliding windows. On threshold, slow down, CAPTCHA, or temporary lock. Return generic errors and monitor credential stuffing.

**Q: How do you prevent one tenant from starving others?**

Use per-tenant limits plus global limits, weighted quotas by plan, and concurrency isolation for expensive operations.

---

## Quick Reference

### Algorithm Selection

| Need | Algorithm |
| --- | --- |
| Simple coarse limit | fixed window |
| Strict login attempts | sliding window log |
| API limit with bursts | token bucket |
| Smooth traffic to downstream | leaky bucket |
| Low-memory smoother limit | sliding window counter |
| Expensive long jobs | concurrency limit + queue |

### Redis Key Examples

```text
rate:ip:{ip}:login:10m
rate:user:{user_id}:api:1m
rate:tenant:{tenant_id}:api:1h
rate:api_key:{key_id}:route:{route}:1m
bucket:user:{user_id}:payments
```

### Response Template

```text
HTTP/1.1 429 Too Many Requests
Retry-After: 30
RateLimit-Limit: 100
RateLimit-Remaining: 0
RateLimit-Reset: 1710000060
```

### Interview Checklist

When designing a rate limiter, specify:

1. Limit identity: IP/user/API key/tenant
2. Algorithm and why
3. Time window/refill rate/burst capacity
4. Storage and atomicity
5. Distributed/multi-region behavior
6. Failure policy
7. Response code and headers
8. Monitoring and abuse detection
