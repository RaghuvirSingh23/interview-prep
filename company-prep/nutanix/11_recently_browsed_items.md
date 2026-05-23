# System Design: Recently Browsed Items

## Problem Statement

An e-commerce company called HackerWear has blown up recently. Their backend is made in NodeJS and is getting 5X the traffic as compared to last week.

The company uses a microservices-based architecture and has scaled up to handle the load. To capitalize on the flow, the company has decided to implement a component that shows the user their last 5 browsed products.

They believe that by building this, the user will spend more time on their site, and hence the chances of them ordering something will increase.

Design the system of the upcoming component such that the existing functionality of the site remains as it is. Provide a solution with a design optimized to integrate seamlessly with the existing design.

## Components

- Cloud Infrastructure — Cloud like AWS, OpenConnect which act as a content delivery network
- Backend — NodeJS with Mongo
- Client — Any device which can browse the internet
- Services Running — User Service, Product Service, Order Service, Payment Service

## Interviewer Guidelines / Key Discussion Points

1. This can be solved by **publishing an event on a queue when a user views an item**
2. The feature can eventually become inconsistent as it is not critical
3. **Handling edge cases** — e.g. if the number of items stored is greater than 5, then older items will be removed
4. **Caching** the results for faster access

## Suggested Solution Architecture

```
Client → Server → AWS → Load Balancer → User Service     ─┐
                                         Product Service  ├──→ Database ↔ Cache Layer
                                         Order Service   ─┘
                                              │
                                         Message Queue → Recently Browsed Tracker
```

### Flow

1. Client request goes through Server → AWS → Load Balancer
2. Load Balancer routes to appropriate microservice (User, Product, Order)
3. When a user views a product, an event is published to the **Message Queue**
4. **Recently Browsed Tracker** service consumes the event and stores the last 5 viewed items
5. Data stored in **Database** with **Cache Layer** for fast retrieval
6. On next page load, recently browsed items fetched from cache

### Key Design Decisions

- **Event-driven**: product view events go to a message queue — decoupled from the main flow
- **Eventual consistency**: this feature is non-critical, so eventual consistency is acceptable
- **LRU / Bounded list**: keep only last 5 items; evict oldest when limit exceeded (Redis LPUSH + LTRIM pattern)
- **Cache-first reads**: recently browsed data served from cache for low latency

---

## Answer (Hello Interview Format)

### Functional Requirements

1. When a user views a product, the system **records the browsing event**
2. Users can see their **last 5 browsed products** on the site
3. The list updates as the user browses more products (oldest item drops off when >5)
4. Existing site functionality (User, Product, Order, Payment services) remains unchanged

### Non-Functional Requirements

1. **Eventually consistent** — this is a non-critical feature; slight delay in showing the last item is acceptable
2. **Low read latency** — recently browsed list should load in <50ms
3. **Decoupled** — adding this feature must not slow down or affect the product viewing flow
4. **Scalable** — works under 5X traffic spike

### Core Entities

- **BrowsingEvent** — user_id, product_id, timestamp
- **RecentlyBrowsed** — user_id, product_ids[] (bounded list of 5)

### API Design

```
GET    /api/recently-browsed
  Headers: Authorization: Bearer <token>
  Returns: { products: [{ id, name, image_url, price }] }  // max 5 items

(No explicit POST — browsing events captured as a side effect of viewing a product)

Internal event published when user hits:
GET    /api/products/{id}
  → publishes { user_id, product_id, timestamp } to message queue
```

### High-Level Design

```
┌──────────┐     ┌──────────────┐     ┌─────────────────┐
│  Client  │────▶│ API Gateway  │────▶│  Load Balancer  │
└──────────┘     └──────────────┘     └────────┬────────┘
                                               │
                              ┌────────────────┼───────────────┐
                              ▼                ▼               ▼
                        ┌──────────┐    ┌──────────┐    ┌──────────┐
                        │ Product  │    │  User    │    │  Order   │
                        │ Service  │    │ Service  │    │ Service  │
                        └────┬─────┘    └──────────┘    └──────────┘
                             │
                   ┌─────────┤ (on product view)
                   ▼         ▼
            ┌──────────┐  ┌───────────────────┐
            │Product DB│  │  Message Queue    │
            └──────────┘  │  (Kafka / SQS)    │
                          └─────────┬─────────┘
                                    ▼
                          ┌───────────────────┐
                          │ Recently Browsed  │
                          │   Tracker Service │
                          └─────────┬─────────┘
                                    ▼
                          ┌───────────────────┐
                          │   Redis           │
                          │ (Bounded Lists)   │
                          └───────────────────┘
```

**Write Path (async):**

1. User views product → Product Service returns product details
2. Product Service publishes `{ user_id, product_id, timestamp }` to message queue (fire-and-forget, doesn't block the response)
3. Recently Browsed Tracker consumes the event
4. Tracker executes Redis commands: `LREM` (remove duplicate if exists) → `LPUSH` → `LTRIM 0 4` (keep only 5)

**Read Path:**

1. Client requests `/api/recently-browsed`
2. Service reads from Redis: `LRANGE recently:{user_id} 0 4`
3. Hydrates product IDs with product details (from cache or Product Service)
4. Returns the 5 products

### Deep Dives

#### Why Redis with a bounded list?

Redis Lists are the perfect data structure for this:

- `LPUSH` + `LTRIM` atomically maintains a fixed-size list — O(1) operations
- `LRANGE` reads the full list in O(N) where N=5 — effectively O(1)
- Sub-millisecond latency for both reads and writes
- Memory footprint is tiny: 5 product IDs per user ≈ 50 bytes × 1M users = ~50MB

Alternative considered: PostgreSQL with `ORDER BY timestamp DESC LIMIT 5` — works but adds unnecessary DB load for a non-critical feature. Redis is purpose-built for this pattern.

#### How do we handle deduplication?

If a user views the same product twice, it should move to the top, not appear twice:

1. `LREM recently:{user_id} 0 {product_id}` — remove all occurrences of this product
2. `LPUSH recently:{user_id} {product_id}` — push to the front
3. `LTRIM recently:{user_id} 0 4` — trim to 5

All three commands wrapped in a Redis `MULTI/EXEC` transaction for atomicity.

#### What if the message queue has lag?

Since this feature is eventually consistent by design, queue lag is acceptable. A user who views product A, then product B, might briefly see an outdated list missing product B. This resolves within seconds as the consumer catches up.

If lag becomes excessive (>30s), we can:

- Scale up queue consumers horizontally
- Use Kafka partitioning by user_id so events for the same user are processed in order

#### What about users who aren't logged in?

For anonymous users, store recently browsed items **client-side** in `localStorage`:

- On product view, JS pushes the product ID to a local array (capped at 5)
- On page load, the recently browsed component reads from localStorage
- If the user later logs in, merge the local list with their server-side list

