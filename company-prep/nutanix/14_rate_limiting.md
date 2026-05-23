# System Design: Rate Limiting

**Difficulty:** Easy
**Tags:** System Design, Database Design

## Problem Statement

Design a rate-limiting mechanism for an API service that controls the number of requests a user can make within specific time windows, such as hourly and daily limits. The system should enforce these rate limits to prevent users from exceeding the allowed number of requests. If a user exceeds the limit, the system should return an HTTP 429 (Too Many Requests) response with relevant headers to inform the user about their remaining rate limits and reset time.

## Components

- Use a cloud platform like AWS, GCP, or Azure to host the application
- Use a microservices architecture
- The client should be able to access the system from any device with a browser

## Interviewer Guidelines / Key Discussion Points

1. **Describe the process for implementing the rate-limiting mechanism**
  - Hint: Discuss how the system tracks requests using counters or token buckets for each user, handling both hourly and daily limits. Explain how the system resets or rolls over the limits after the time window expires, ensuring accurate tracking across multiple servers.
2. **Explain how the system handles and responds to throttled requests**
  - Hint: Describe how the system checks the user's request count before processing. If limits are exceeded, it returns an HTTP 429 error and includes headers (e.g., X-RateLimit-HourlyLimit, X-RateLimit-HourlyRemaining, X-RateLimit-HourlyReset, X-RateLimit-DailyLimit) that inform the user of the remaining requests and reset times.
3. **Discuss the impact of 10x scalability on the system components**
  - Hint: Consider how the rate limit storage (in-memory or database) must scale to handle increased traffic. Discuss strategies for optimizing lookup times and maintaining consistent performance as user traffic grows, such as distributing rate limit checks across multiple servers or using distributed caches.

## Whiteboard Solution

### Functional Requirements

- Track the number of requests each user makes within specified hourly and daily time windows
- Enforce rate limits, ensuring that users do not exceed the allowed number of requests within the given time window
- Return an HTTP 429 (Too Many Requests) error when the rate limit is exceeded, including relevant rate-limit headers in the response

### Non-Functional Requirements

- Able to scale to handle an increasing number of users
- Ensure minimal latency when checking and enforcing rate limits

### API Routes

#### POST /api/request

Handles an incoming API request, checks the rate limit for the user, and processes or throttles the request.

```
{ "user_id": "string", "request_data": "object" }
```

**Response Headers (Status 200):**

- X-RateLimit-HourlyLimit: 1000
- X-RateLimit-HourlyRemaining: 750
- X-RateLimit-HourlyReset: 3600 (seconds until reset)
- X-RateLimit-DailyLimit: 10000
- X-RateLimit-DailyRemaining: 9000

**Response Headers (Status 429):**

- X-RateLimit-HourlyLimit: 1000
- X-RateLimit-HourlyRemaining: 0
- X-RateLimit-HourlyReset: 1800 (seconds until reset)

#### GET /api/rate_limit_status

Retrieves the current rate limit status for a user.

### Database Schema

**Rate_Limits Table:**


| Column                       | Description                              |
| ---------------------------- | ---------------------------------------- |
| user_id                      | assumed to be provided by the API client |
| hourly_request_count         | current count in hourly window           |
| daily_request_count          | current count in daily window            |
| hourly_limit_reset_timestamp | when the hourly window resets            |
| daily_limit_reset_timestamp  | when the daily window resets             |


### Estimations

**Rate Limit Data:**

- Average size per user entry: 150 bytes (including user ID, request counts for hourly and daily limits, and reset timestamps)
- Number of users: 100,000 users
- Total storage: 15 MB (in-memory or database)

---

## Answer (Hello Interview Format)

### Functional Requirements

1. **Track** the number of requests each user makes within hourly and daily time windows
2. **Enforce** rate limits — block requests exceeding the allowed count
3. **Return HTTP 429** with headers showing remaining quota and reset time when limits are exceeded
4. Users can **query their rate limit status** at any time

### Non-Functional Requirements

1. **Sub-millisecond overhead** — rate limit check must not add perceptible latency
2. **Distributed consistency** — rate limits enforced correctly across multiple API servers
3. **10X scalable** — handle growth from 100K to 1M+ users without redesign
4. **Accurate tracking** — no significant over- or under-counting of requests

### Core Entities

- **RateLimitConfig** — tier, hourly_limit, daily_limit
- **RateLimitBucket** — user_id, window_type (hourly/daily), request_count, window_start_ts, window_end_ts

### API Design

```
POST   /api/request
  Body: { user_id, request_data }
  Success (200):
    Headers: X-RateLimit-HourlyLimit: 1000
             X-RateLimit-HourlyRemaining: 750
             X-RateLimit-HourlyReset: 3600
             X-RateLimit-DailyLimit: 10000
             X-RateLimit-DailyRemaining: 9000
  Throttled (429):
    Headers: X-RateLimit-HourlyLimit: 1000
             X-RateLimit-HourlyRemaining: 0
             X-RateLimit-HourlyReset: 1800
    Body: { error: "Rate limit exceeded", retry_after: 1800 }

GET    /api/rate_limit_status?user_id=...
  Returns: { hourly: { limit, remaining, reset }, daily: { limit, remaining, reset } }
```

### High-Level Design

```
┌──────────┐     ┌──────────────────────────────────────────┐
│  Client  │────▶│            API Gateway                   │
└──────────┘     └──────────────┬───────────────────────────┘
                                │
                         ┌──────▼───────┐
                         │ Rate Limit   │
                         │ Middleware   │◄────────┐
                         └──────┬───────┘         │
                                │            ┌────┴─────────┐
                         ┌──────▼──────┐     │ Redis Cluster│
                         │ Pass / 429  │     │ (Counters)   │
                         └──────┬──────┘     └──────────────┘
                                │
                     Pass ──────┤
                                ▼
                         ┌──────────────┐
                         │ Load Balancer│
                         └──────┬───────┘
                           ┌────┼────┐
                           ▼    ▼    ▼
                         ┌───┐┌───┐┌───┐
                         │API││API││API│
                         │ 1 ││ 2 ││ 3 │
                         └───┘└───┘└───┘
```

**Request Flow:**

1. Request arrives at API Gateway
2. Rate Limit Middleware extracts user_id
3. Executes atomic Lua script in Redis:
  - Increment hourly counter (`rate:hourly:{user_id}:{hour_bucket}`)
  - Increment daily counter (`rate:daily:{user_id}:{day_bucket}`)
  - Check both against limits
4. If either exceeds limit → return 429 with headers
5. If within limits → forward to backend; set rate limit headers on response

### Deep Dives

#### Which rate limiting algorithm is best here?

**Fixed Window** with hourly and daily buckets, enhanced by the sliding window approximation:

For hourly limits:

- Key: `rate:hourly:{user_id}:{hour_number}` with TTL = 3600s
- On each request: `INCR` the key, check against limit
- For more accurate boundary handling, apply **sliding window counter**: weight current window's count by elapsed fraction + previous window's count by remaining fraction

For daily limits:

- Key: `rate:daily:{user_id}:{day_number}` with TTL = 86400s
- Same INCR + check pattern

Both checks happen in a single Redis roundtrip via Lua script:

```lua
local hourly_key = KEYS[1]
local daily_key = KEYS[2]
local hourly_limit = tonumber(ARGV[1])
local daily_limit = tonumber(ARGV[2])
local hourly_ttl = tonumber(ARGV[3])
local daily_ttl = tonumber(ARGV[4])

local hourly_count = redis.call('INCR', hourly_key)
if hourly_count == 1 then redis.call('EXPIRE', hourly_key, hourly_ttl) end

local daily_count = redis.call('INCR', daily_key)
if daily_count == 1 then redis.call('EXPIRE', daily_key, daily_ttl) end

if hourly_count > hourly_limit or daily_count > daily_limit then
  return {0, hourly_count, daily_count,
          redis.call('TTL', hourly_key), redis.call('TTL', daily_key)}
end
return {1, hourly_count, daily_count,
        redis.call('TTL', hourly_key), redis.call('TTL', daily_key)}
```

#### How do we scale to 10X users?

- **Redis Cluster** with sharding by user_id — distributes counters across nodes
- Each rate limit check is a single Redis roundtrip (~0.5ms) — even at 1M users, Redis handles millions of ops/sec
- Storage: 150 bytes/user × 1M users = 150MB — fits entirely in memory
- If Redis becomes a bottleneck, can add read replicas for the status endpoint and shard writes by user_id hash

#### What happens if Redis goes down?

**Fail-open strategy**: If Redis is unreachable, allow the request through. Rationale:

- Brief rate limiting gap is better than blocking all legitimate users
- Set up Redis Sentinel or Cluster for automatic failover (typically <30s)
- Log the event for monitoring; alert on-call
- As a fallback, each API server can maintain a **local in-memory counter** (approximate) that kicks in during Redis outage

#### How do we handle different rate limit tiers?

- Store tier configs in a `rate_limit_configs` table: `{ tier: "free", hourly: 100, daily: 1000 }`, `{ tier: "pro", hourly: 10000, daily: 100000 }`
- User's tier is looked up once per request (cached locally with 60s TTL)
- The Lua script receives the correct limits as arguments based on the user's tier
- Tier changes take effect within the cache TTL window

