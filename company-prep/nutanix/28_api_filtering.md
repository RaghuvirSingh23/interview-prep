# API Design: API Filtering

**Difficulty:** Medium
**Tags:** API Development, API Design, Database

## Problem Statement

There is an existing high-volume API that typically handles thousands of requests per minute. You are tasked with optimizing the existing architecture of an API that allows a user to input between 1-10 items and currently utilizes a 'CONTAINS' query.

In one scenario, a corporate client uses your platform to order bulk quantities of items for its inventory. They frequently input a list of 10 item IDs to streamline their procurement process. With 'CONTAINS,' the query retrieves all matching items from the list, which could result in hundreds or thousands of results. This situation leads to performance issues, slower response times, and unnecessary data transfer. As a result, the corporate client experiences delays in processing their orders, impacting their efficiency and overall satisfaction.

**How would you improve the existing architecture by optimizing the query for faster response time? What factors have you taken into account?**

## Interviewer Guidelines

### Transition to '=' Operator

To optimize the API that currently uses a 'CONTAINS' query, we can significantly enhance performance by transitioning to the '=' operator. This change ensures that only exact matches for the provided item IDs are retrieved, significantly reducing the amount of data transferred and processed. Because the unnecessary transfer of data is minimized, conserving network resources and reducing data processing overhead. However, this transition requires careful consideration of several factors:

1. **Exact-Match Requirement** — It's important to ensure that the user's search intent aligns with exact matches for the provided item IDs. The '=' operator retrieves only items with precisely matching IDs. Suitable for scenarios where users expect or require exact matches, such as searching for specific products by their unique identifiers.
2. **Indexing** — To optimize the '=' operator, we must implement proper indexing for the item ID column. Indexing improves retrieval times, especially when dealing with a large dataset. Indexing ensures that database queries for exact matches are executed efficiently, reducing response times.

### Trade-offs

1. **Exact-Match Constraint** — Users must input the exact item IDs to retrieve results. Any minor typos or variations can lead to no results, potentially frustrating users. This reduces flexibility of search. The CONTAINS queries typically allow for fuzzy matching or partial matching. If users frequently rely on partial matching or searching for items based on similar keywords or identifiers, this transition may negatively impact their experience.
2. **Storage and Indexing Impact** — While indexing can significantly speed up exact-match queries, it comes with trade-offs. Indexes consume additional storage space, and maintaining indexes can have an impact on database write performance. Essential to carefully manage and monitor these aspects.
3. **Rollback Strategies** — Important to have rollback strategies in place. In case the transition to the '=' operator leads to unforeseen issues or negatively impacts performance or user experience, having a way to revert to the previous 'CONTAINS' query structure is essential. The rollback plan should be well-defined and tested.
4. **Data Migration and Transformation** — If historical data was originally structured for 'CONTAINS' queries, transitioning to the '=' operator may necessitate data migration and transformation. The existing data may need to be restructured to fit the requirements of the new query structure.
5. **Complex Queries** — If users frequently need to perform complex queries that involve multiple criteria or search parameters, the '=' operator may not be the most suitable approach. Complex queries might require additional adjustments and custom logic.

### Alternative Solutions

1. **Implementing Query Caching** — For frequently requested item lists, implementing a query cache can significantly speed up common queries. This cache stores the results of frequent item list searches, reducing the need to query the database for every request.
2. **Pagination and Result Limiting** — Implementing pagination and result limiting. For users who don't need to view all items simultaneously, dividing the results into pages or limiting the number of displayed items per query can decrease the time it takes to load a page.

---

## Answer (Hello Interview Format)

### Functional Requirements

1. Users can **query items by ID** — input 1 to 10 item IDs and get matching results
2. API returns **exact matches** for the provided item IDs
3. Support **pagination** for large result sets
4. Handle **thousands of requests per minute** from corporate and regular clients

### Non-Functional Requirements

1. **Low latency** — queries should return in <50ms for up to 10 item IDs
2. **High throughput** — support thousands of RPM without degradation
3. **Backward compatibility** — migration from CONTAINS to = should not break existing clients
4. **Scalability** — handle growing dataset sizes

### Core Entities

- **Item** — id (indexed), name, description, price, category, stock_count
- **QueryLog** — id, client_id, item_ids[], response_time_ms, timestamp (for analytics)

### API Design

```
GET    /api/items?ids=item1,item2,...,item10
  Returns: { items: [...], total }

GET    /api/items?ids=item1,item2&page=1&limit=20
  Returns: { items: [...], total, page, pages }

GET    /api/items/{id}
  Returns: { item details }
```

### High-Level Design

```
┌──────────┐     ┌──────────────┐     ┌─────────────────┐
│  Client  │────▶│ API Gateway  │────▶│  Load Balancer  │
└──────────┘     │ (Rate Limit) │     └────────┬────────┘
                 └──────────────┘              │
                                        ┌──────▼───────┐
                                        │  Item Query  │
                                        │   Service    │
                                        └──────┬───────┘
                                               │
                                     ┌─────────┼──────────┐
                                     ▼         ▼          ▼
                              ┌──────────┐ ┌──────────┐ ┌──────────┐
                              │  Redis   │ │PostgreSQL│ │  Query   │
                              │ (Query   │ │ (Items)  │ │  Cache   │
                              │  Cache)  │ │  + Index │ │ (Result) │
                              └──────────┘ └──────────┘ └──────────┘
```

**Query Flow:**

1. Client sends `GET /api/items?ids=A,B,C`
2. Item Query Service checks Redis cache for the exact query hash
3. **Cache HIT** → return cached result
4. **Cache MISS** → execute SQL: `SELECT * FROM items WHERE id IN ($1, $2, $3)` (parameterized)
5. Store result in Redis with TTL (e.g., 60s)
6. Return to client

### Deep Dives

#### Why switch from CONTAINS to = (exact match)?

The CONTAINS query (`WHERE id LIKE '%item%'`) is problematic:

- **Full table scan** — cannot use B-tree indexes, must scan every row
- **Overly broad results** — returns items with partial ID matches, potentially thousands of irrelevant results
- **Data transfer waste** — sending hundreds of unneeded rows over the network

The `=` operator with `IN` clause:

- **Uses the primary key index** — O(log N) per ID lookup instead of O(N) table scan
- **Exact results** — only returns the requested items, no noise
- **Predictable performance** — up to 10 IDs means at most 10 index lookups

For 10 item IDs on a table with millions of rows:

- CONTAINS: ~500ms (full scan)
- IN with index: ~2ms (10 index seeks)

#### How do we handle the migration without breaking clients?

Phased rollout:

1. **Phase 1 — Dual mode**: New endpoint supports both `?ids=` (exact) and `?search=` (contains). Default to exact match.
2. **Phase 2 — Feature flag**: Route a percentage of traffic to the new query engine. Monitor latency, error rates, and client complaints.
3. **Phase 3 — Deprecation**: Mark CONTAINS endpoint as deprecated with a sunset date. Add response header: `Deprecation: true; Sunset: 2025-06-01`.
4. **Phase 4 — Removal**: Remove CONTAINS support after clients have migrated.

Rollback plan: The feature flag can instantly revert to CONTAINS if issues arise.

#### How do we optimize for the bulk corporate use case?

Corporate clients query the same item lists repeatedly:

1. **Query result caching**: Hash the sorted item ID list → use as Redis cache key. TTL of 60s covers repeated queries.
2. **Batch-optimized SQL**: Use `WHERE id = ANY($1::text[])` in PostgreSQL — passes the array as a single parameter, more efficient than individual `=` checks.
3. **Database connection pooling**: Prevent connection exhaustion under high request volume.
4. **Cursor-based pagination**: For queries that return many results, use `?cursor=last_id&limit=20` instead of OFFSET — constant time regardless of page depth.

#### What about indexing trade-offs?

- The `id` column should already have a **primary key index** (B-tree) — no additional indexing needed for exact match
- If clients also filter by category or price, add **composite indexes**: `CREATE INDEX idx_items_category_price ON items(category, price)`
- Index overhead: slight write slowdown (each INSERT/UPDATE must update the index), and storage cost (~20% additional)
- Monitor index usage with `pg_stat_user_indexes` — drop unused indexes

