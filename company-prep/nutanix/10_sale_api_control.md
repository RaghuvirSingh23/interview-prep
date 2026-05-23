# System Design: Sale API Control

## Problem Statement

A game shop is planning to allow preorders for a popular gaming console through their website. In past sales, some users have automated a process to repeatedly try to order the device until it succeeds. Once this caused 3 hours of downtime on their site.

They have decided to use an API limiter to filter the malicious customers and block them for future visits. They need to do some quality control and develop a way to execute API request filtering at the server. This will provide the customers a bug-free and fair chance to buy the device.

Using the tools provided, design the system such that the existing functionality of the site remains as it is. Provide an optimized design that can integrate this API level filtering to be used only on the day of the sale.

## Components

- Cloud Infrastructure — cloud like AWS, OpenConnect which act as a content delivery network
- Backend — microservice architecture developed in NodeJS with Mongo
- Client — any device which can browse the Internet

**Note:** It is best if the design is highly modular so that it can be reused in the future.

## Interviewer Guidelines / Key Discussion Points

1. **Limiting requests per IP** — Limiting the number of requests a customer can send to an API within a time window, for example, twenty requests per second from an IP address. A queue can be used to limit incoming requests.
2. **Distributed rate limiting** — Rate limiting should work for a distributed setup, as the APIs are available through a group of servers.
3. **Throttling strategy** — Strategy to handle throttling.

## Suggested Solution Architecture

```
Client → Server → Rate Limiting Middleware → Valid Request?
                                                  │
                                          No ─────┴───── Yes
                                          ↓               ↓
                                     Throw Error     Event Queue
                                                         ↓
                                                    Load Balancer
                                                    ┌────┴────┐
                                             Backend     Backend
                                             Service 1   Service 2
                                                    ↕         ↕
                                               Database ↔ Cache Layer
```

### Flow

1. **Client** sends request to **Server**
2. **Rate Limiting Middleware** checks if the request is valid (within rate limit)
3. If **No** → Throw Error (429 Too Many Requests)
4. If **Yes** → Request enters **Event Queue**
5. **Load Balancer** distributes to backend services
6. Backend services interact with **Database** and **Cache Layer**

### Key Design Decisions

- Rate limiting middleware is placed *before* the backend services — drops bad traffic early
- Event Queue acts as a buffer to absorb traffic spikes on sale day
- Modular design: rate limiter can be toggled on/off (only used on sale days)
- Distributed setup: shared rate limit state across servers (e.g., Redis-backed counter)

---

## Answer (Hello Interview Format)

### Functional Requirements

1. Users can **browse and preorder** a gaming console through the website
2. The system **rate-limits API requests** per IP/user within configurable time windows
3. **Blocked users** receive clear error responses (HTTP 429) and are blocked from future visits
4. Rate limiting is **togglable** — only active on sale days
5. Legitimate users get a **fair chance** to complete their order

### Non-Functional Requirements

1. **Low latency** — rate limiting check must add <5ms overhead per request
2. **Distributed** — works consistently across multiple backend servers
3. **High throughput** — handle massive traffic spikes on sale day (100K+ RPS)
4. **Modular / reusable** — can be applied to future sales, not just this one

### Core Entities

- **RateLimitRule** — id, name, window_seconds, max_requests, is_active
- **RateLimitEntry** — ip_address, user_id, request_count, window_start, blocked_until
- **BlockList** — ip_address, reason, blocked_at, expires_at

### API Design

```
POST   /api/preorder
  Body: { product_id, user_id, payment_info }
  Headers: X-RateLimit-Limit, X-RateLimit-Remaining, X-RateLimit-Reset
  Returns: 200 { order_id } or 429 { error: "Too Many Requests", retry_after }

GET    /api/products/{id}
  Returns: { product details, availability }

POST   /admin/rate-limits
  Body: { window_seconds, max_requests, is_active }
  Returns: { rule_id }

POST   /admin/blocklist
  Body: { ip_address, reason, duration }
```

### High-Level Design

```
┌──────────┐     ┌─────────────┐     ┌──────────────────────────────────┐
│  Client  │────▶│  CDN / WAF  │────▶│   Rate Limiting Middleware       │
│          │     │ (Layer 1)   │     │   (Nginx/API Gateway layer)      │
└──────────┘     └─────────────┘     └──────────────┬───────────────────┘
                                                    │
                                          ┌─────────▼──────────┐
                                          │   Redis Cluster    │
                                          │  (Rate Counters)   │
                                          └─────────┬──────────┘
                                                    │
                                     Pass ──────────┼──────── Block → 429
                                          │                        Response
                                    ┌─────▼──────────┐
                                    │  Message Queue  │
                                    │  (Buffer Spike) │
                                    └─────┬──────────┘
                                          │
                                    ┌─────▼──────────┐
                                    │ Load Balancer   │
                                    └───┬────────┬────┘
                                        ▼        ▼
                                  ┌──────────┐ ┌──────────┐
                                  │ Order    │ │ Order    │
                                  │ Service  │ │ Service  │
                                  │ (Node 1) │ │ (Node 2) │
                                  └────┬─────┘ └────┬─────┘
                                       │            │
                                  ┌────▼────────────▼────┐
                                  │     PostgreSQL       │
                                  │  (Orders, Inventory) │
                                  └──────────────────────┘
```

### Deep Dives

#### Which rate limiting algorithm should we use?

**Sliding Window Counter** — best balance of accuracy and performance:

1. Divide time into fixed windows (e.g., 1-second buckets)
2. For each request, use Redis `INCR` on key `rate:{ip}:{window_id}` with TTL = window size
3. Compute weighted count: `current_window_count * elapsed_fraction + previous_window_count * remaining_fraction`
4. If count > limit → reject with 429

Why not other algorithms:

- **Fixed Window** — simple but allows burst at window boundaries (2X burst)
- **Token Bucket** — great for bursty traffic but harder to implement distributed
- **Leaky Bucket** — smooths traffic well but adds latency (queuing)
- **Sliding Window Log** — most accurate but memory-intensive (stores every timestamp)

Sliding Window Counter gives ~accuracy of the log approach with O(1) memory per user.

#### How do we make it distributed?

Redis Cluster is the shared state store:

- All API servers check the **same Redis** for rate counters
- Use `MULTI/EXEC` or Lua scripts for atomic increment-and-check:

```lua
local count = redis.call('INCR', KEYS[1])
if count == 1 then
  redis.call('EXPIRE', KEYS[1], ARGV[1])
end
return count
```

- Redis is chosen for sub-millisecond latency and atomic operations
- If Redis is temporarily down, **fail open** (allow requests) rather than blocking all users — the sale must go on

#### How do we handle malicious bot detection beyond simple rate limiting?

Layer the defense:

1. **Layer 1 — WAF/CDN** (Cloudflare/AWS WAF): Block known bot IPs, CAPTCHA challenges for suspicious patterns
2. **Layer 2 — Rate Limiter**: Per-IP and per-user-session limits
3. **Layer 3 — Behavioral analysis**: Detect patterns like perfectly periodic requests, missing browser fingerprints, identical request headers → escalate to CAPTCHA or block
4. **Persistent blocklist**: IPs/users identified as bots are added to a blocklist in Redis with configurable TTL

#### How do we make it togglable for sale days only?

- Rate limit rules stored in a config table with `is_active` flag
- Admin endpoint to activate/deactivate rules
- The middleware checks `is_active` first — if false, all requests pass through unimpeded
- Configuration cached in Redis with short TTL (30s) so changes propagate quickly
- Can also schedule activation via a cron job: activate at sale start time, deactivate after

