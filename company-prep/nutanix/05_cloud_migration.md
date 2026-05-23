# System Design: Cloud Migration

## Problem Statement

A property dealer currently owns a website that lists out the details of properties including photos, location, and rent details. As they also deal with international customers, a fast and secure user experience is a major concern. They are planning to migrate their traditional website to a cloud-based infrastructure.

You have been given the basic layout of the system design of the web application. Review the additional information below and redesign the whole system.

- The system should include all the necessary services and resources to achieve the functionality per the requirements above. It should contain services like payment, view, guest, search, and update.
- The 'process' in the basic layout should be replaced with the actual workflow.

### Usage of Services

- **Payment services** — allow users to make online payments
- **View services** — make the data available for the guest to see
- **Guest services** — maintain the guest's favorites, last checked records, etc.
- **Search services** — allow the user to search across different properties

### Constraints

- There is no need to include the code for the services. Assume it is already available. Map the services according to the use case and the given basic layout.
- The front-end framework and the back-end language are not of concern. Just design a proper workflow using smart assumptions as needed.

## Interviewer Guidelines / Key Discussion Points

- The interviewee should be clear on what type of database to use and why. Here, an **elastic search database** is recommended as this use case would require the user to search for the property details. Elastic search will provide better performance than other databases like MongoDB.
- The interviewee should know how a user experience can be made faster for global users. It can be done by **CDN (Content Delivery Networks)** as the data can be stored at locations where the latency is minimum for a user.
- The interviewee should be able to make proper places for the payment service, search service, guest service, and update service.
- The interviewee should be able to answer the correct replacement for 'process' in the diagram.

## Suggested Solution Architecture

```
User → DNS (Route53) → Load Balancer → Payment Service ──→ Enqueue → Queue
                            │              View Service                    ↑
                            │              Guest Service        Update Service ← Dequeue
                            │              Search Service            │
                            ↓                   │                    ↓
                           CDN               Database          Host Service → Bucket (S3) for pics
                                                                    ↓
                                                                   Host
```

### Flow

1. User hits DNS (Route53 on AWS) → routed to Load Balancer
2. Load Balancer distributes across microservices: Payment, View, Guest, Search
3. CDN serves static assets (property photos, etc.) for low-latency global access
4. Updates go through a Queue (enqueue/dequeue pattern)
5. Update Service processes changes and writes to Database
6. Host Service manages property images, stored in S3 Bucket
7. Search Service backed by Elasticsearch for property lookups

---

## Answer (Hello Interview Format)

### Functional Requirements

1. Users can **search** properties by location, price range, type, and other filters
2. Users can **view** property details including photos, location, and rent
3. Users can **make payments** online for rent/deposits
4. Users can **save favorites** and view their last checked records (guest services)
5. Property managers can **update** property listings

### Non-Functional Requirements

1. **Low latency globally** — international customers need fast page loads
2. **High availability** — site should be up 24/7
3. **Scalability** — handle increasing property listings and user traffic
4. **Search performance** — property search should return results in <200ms

### Core Entities

- **Property** — id, title, description, location, rent, type, photos[], status, owner_id
- **User** — id, email, name, role (guest/owner), created_at
- **Favorite** — id, user_id, property_id, created_at
- **Payment** — id, user_id, property_id, amount, status, method, created_at
- **BrowsingHistory** — id, user_id, property_id, viewed_at

### API Design

```
GET    /api/properties/search?location=...&min_rent=...&max_rent=...&type=...&page=1
  Returns: { properties: [...], total, page }

GET    /api/properties/{id}
  Returns: { property details with photos }

POST   /api/payments
  Body: { property_id, amount, payment_method }
  Returns: { payment_id, status }

POST   /api/favorites
  Body: { property_id }

GET    /api/favorites
  Returns: { properties: [...] }

GET    /api/history
  Returns: { recently_viewed: [...] }

PUT    /api/properties/{id}
  Body: { updated fields }
```

### High-Level Design

```
┌──────────┐     ┌──────────┐     ┌────────────────┐
│  Client  │────▶│   DNS    │────▶│ Load Balancer  │
│ (Browser)│     │(Route53) │     └───────┬────────┘
└──────────┘     └──────────┘             │
      │                          ┌────────┼─────────────┬──────────────┐
      │                          ▼        ▼             ▼              ▼
      │                    ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐
      │                    │ Search   │ │  View    │ │  Guest   │ │ Payment  │
      │                    │ Service  │ │ Service  │ │ Service  │ │ Service  │
      │                    └────┬─────┘ └────┬─────┘ └──-─┬─────┘ └──┬────--─┘
      │                         │            │            │          │
      │                    ┌────▼─────┐ ┌────▼─────┐ ┌───-▼─────┐    │
      │                    │Elastic-  │ │ Postgres │ │  Redis   │    │
      │                    │search    │ │   (DB)   │ │ (Cache)  │    │ 
      │                    └──────────┘ └──────────┘ └──────────┘    │
      │                                                              ▼
      │                                                     ┌──────────────┐
      │              ┌──────────┐                           │   Message    │
      └─────────────▶│   CDN    │                           │   Queue      │
       static assets │(CloudFront)│                         └──────┬───────┘
                     └─────┬────┘                                  ▼
                           │                               ┌──────────────┐
                      ┌────▼─────┐                         │   Update     │
                      │  S3      │                         │   Service    │
                      │ (Photos) │                         └──────┬───────┘
                      └──────────┘                                ▼
                                                           ┌──────────────┐
                                                           │ Host Service │
                                                           │ → S3 (pics)  │
                                                           └──────────────┘
```

### Deep Dives

#### Why Elasticsearch for search?

Property search involves full-text queries across multiple fields (location, description, type) combined with range filters (rent price) and geo-spatial queries (nearby properties). Elasticsearch provides:

- **Inverted index** for full-text search — fast text matching
- **Geo-point queries** — "properties within 5km of this location"
- **Aggregations** — price range facets, property type counts
- **Sub-200ms** response times on millions of documents

Data is synced from the primary PostgreSQL database to Elasticsearch via Change Data Capture (CDC) or a periodic indexing pipeline. PostgreSQL remains the source of truth; Elasticsearch is a read-optimized projection.

#### How does CDN help with global latency?

- Property photos (often the heaviest payload) are served from **CloudFront edge nodes** closest to the user
- Static assets (JS, CSS, HTML templates) also cached at the edge
- First request may hit origin (S3), but subsequent requests are served from cache — TTL set to hours/days since property photos rarely change
- For international users, this can reduce photo load times from ~2-3s to <200ms

#### How does the update flow work?

1. Property manager submits update via API
2. Request enters a **Message Queue** (SQS/Kafka) to decouple writes from reads
3. **Update Service** consumes the message, writes to PostgreSQL
4. CDC stream or queue triggers Elasticsearch re-indexing for the updated property
5. **Host Service** handles any new photo uploads → stores in S3 → CDN cache invalidated for changed images

#### How do we handle payment service reliability?

- Payment Service is isolated — failures don't cascade to search/view
- Uses **idempotency keys** to prevent double-charging on retries
- Integrates with third-party payment provider (Stripe/PayPal) via webhook for async confirmation
- Transaction records stored in PostgreSQL with status tracking (pending → completed → failed)
- Failed payments trigger a retry queue with exponential backoff

