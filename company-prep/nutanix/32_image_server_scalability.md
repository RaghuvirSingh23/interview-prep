# System Design: Image Server Scalability

**Difficulty:** Hard
**Tags:** System Design, E-Commerce, MERN

## Problem Statement

The current e-commerce repository employs placeholder images for products. The challenge involves redesigning the platform to manage product images using a centralized server efficiently. This server should handle HTTP requests from users, retrieve corresponding images from a database, and serve them back to users seamlessly.

```javascript
class ImageServer {
  constructor(db) {
    this.db = db;
  }

  async handleRequest(imageId) {
    const image = await this.db.getImage(imageId);
    return image;
  }
}
```

Currently, the system handles 2,000 requests per second (RPS) for images, with an average image size of 2MB. However, recent user traffic analysis indicates that the platform anticipates a surge in traffic, with projected rates reaching 5,000 requests per second (RPS) in the near future.

## Instructions

- Calculate the total data bandwidth requirement for the system when it has to handle 5,000 RPS
- Assess whether the server, with a bandwidth limit of 4 Gbps, will be able to handle this increased traffic
- If not, propose an improved system architecture to support this requirement

## Interviewer Guidelines

### Step 1: Bandwidth Calculation

- Total data bandwidth required = 5,000 images/second × 2 MB/image = 10,000 MB/s = **10 GB/s**
- Converting to Gbps: 10 GB/s × 8 = **80 Gbps**
- The server would require 80 Gbps bandwidth, which is **significantly greater than the current 4 Gbps limit**

### Step 2: Server Assessment

The current 4 Gbps limit cannot handle 80 Gbps. Need a new architecture.

### Step 3: System Architecture

#### Existing Architecture (Unoptimized)
```
User → Request Image → Central Server (Web Server) → Fetch Image → Image Database
                    ← Deliver Image ← Return Image ←
```

Problems: No caching, no edge nodes, no CDN optimization. Single central server = bottleneck.

#### Improved Architecture (CDN-based)
```
User → Edge Nodes (CDN) → Cache Servers
                              ├── Cache HIT → Deliver Cached Image → User
                              └── Cache MISS → Central Server → Web Server → Image Database
                                                    → Return Image → Edge Nodes → User
```

### Component Explanation

1. **User** — Interacts with the system by requesting an image
2. **Edge Nodes** — Distributed nodes located closer to users, serving as the first point of contact. They act as a gateway to the CDN
3. **Cache Servers** — Store cached copies of frequently accessed images. When a request is made, edge nodes first check if the image is available in cache. If it is, the cached image is delivered directly to the user, reducing load on the central server
4. **Image Database** — Central image database where all images are stored. If the image is not available in cache, edge nodes forward the request to the central server
5. **Central Server** — Web server responsible for handling requests that couldn't be satisfied by cache servers. Acts as the main control center for the CDN
6. **Web Server** — Receives the request from the edge nodes and interacts with the image database to fetch the requested image
7. **Delivering Image** — Once fetched from the database, image is sent back to the central server, which delivers it to the requesting edge nodes. Finally, edge nodes deliver the image to the user

### Key Insight

This optimized system design leverages the distributed nature of edge nodes and cache servers to serve frequently accessed images from nearby locations, reducing latency and improving user experience.

---

## Answer (Hello Interview Format)

### Functional Requirements

1. Users can **request product images** by image ID via HTTP
2. System **serves images** from a centralized database
3. Replace placeholder images with **real product images** managed centrally
4. Handle **5,000 requests per second** with 2MB average image size

### Non-Functional Requirements

1. **High bandwidth** — system must handle 80 Gbps (5,000 RPS × 2MB × 8 bits)
2. **Low latency** — images should load in <500ms for a good user experience
3. **High availability** — image serving must be always-on (revenue-critical for e-commerce)
4. **Cost efficient** — minimize origin server bandwidth costs

### Core Entities

- **Image** — id, product_id, url, format (JPEG/WebP/AVIF), width, height, size_bytes, created_at
- **ImageVariant** — id, image_id, variant (thumbnail/medium/large), url, size_bytes
- **Product** — id, name, image_ids[]

### API Design

```
GET    /images/{image_id}
  Query: ?width=800&format=webp  (optional transformations)
  Returns: image binary (Content-Type: image/webp)

GET    /images/{image_id}/metadata
  Returns: { id, format, width, height, size_bytes, variants }

POST   /admin/images
  Body: multipart/form-data { product_id, image }
  Returns: { image_id, url, variants: { thumbnail, medium, large } }
```

### High-Level Design

```
┌──────────┐     ┌──────────────────────────────────────────────┐
│  User     │────▶│           CDN (CloudFront / Akamai)          │
│ (Browser) │     │  Edge Nodes distributed globally              │
└──────────┘     └────────────────────┬─────────────────────────┘
                                      │
                            Cache HIT │ Cache MISS
                            (serve)   │ (fetch from origin)
                                      ▼
                              ┌──────────────────┐
                              │  Origin Shield   │
                              │  (Regional Cache)│
                              └────────┬─────────┘
                                       │
                              Cache HIT│ Cache MISS
                                       ▼
                              ┌──────────────────┐
                              │  Image Service   │
                              │  (Origin Server) │
                              └────────┬─────────┘
                                       │
                              ┌────────┼─────────┐
                              ▼        ▼         ▼
                        ┌──────────┐ ┌─────────┐ ┌──────────────┐
                        │  S3      │ │  Redis  │ │  Image       │
                        │ (Object  │ │ (Meta-  │ │ Processing   │
                        │  Storage)│ │  data   │ │ (Resize/     │
                        └──────────┘ │  Cache) │ │  Convert)    │
                                     └─────────┘ └──────────────┘
```

### The Math

- **Current**: 2,000 RPS × 2 MB = 4 GB/s = 32 Gbps → fits in 4 Gbps? No! Already over capacity.
- **Projected**: 5,000 RPS × 2 MB = 10 GB/s = **80 Gbps**
- **Single server limit**: 4 Gbps → can serve only 250 images/second at 2MB each
- **Required**: 80 Gbps / 4 Gbps = **20 origin servers** minimum without CDN

With CDN (assuming 90% cache hit rate):
- Origin only serves 10% of requests = 500 RPS = 8 Gbps → 2 origin servers sufficient
- CDN edge nodes absorb the remaining 4,500 RPS across hundreds of global PoPs

### Deep Dives

#### How does the CDN solve the bandwidth problem?

CDN fundamentals for this use case:
1. **Edge caching**: First request for an image goes to origin, response cached at the edge node with TTL (e.g., 24 hours for product images). All subsequent requests from users near that edge are served locally.
2. **Geographic distribution**: CloudFront has 400+ edge locations globally. User in Tokyo gets the image from a Tokyo edge, not from a US origin server.
3. **Origin Shield**: An intermediate caching layer between edge nodes and origin. When multiple edge nodes get a cache miss simultaneously (thundering herd), Origin Shield deduplicates — only one request reaches the origin.

Expected cache hit rates for product images: **85-95%** (product images rarely change, many users view the same products).

#### How do we reduce the 2MB average image size?

Image optimization pipeline on upload:
1. **Format conversion**: Convert to WebP (30% smaller than JPEG) or AVIF (50% smaller). Serve based on `Accept` header.
2. **Generate variants**: Create thumbnail (200px), medium (800px), large (1600px) on upload. Client requests appropriate size.
3. **Lazy loading**: Below-the-fold images use `loading="lazy"` — not requested until visible.
4. **Progressive JPEG**: For large images, use progressive encoding so a low-quality preview appears immediately.

If average size drops from 2MB to 500KB with WebP + appropriate sizing:
- 5,000 RPS × 500KB = 2.5 GB/s = 20 Gbps
- With 90% CDN cache hit: origin serves 2 Gbps → **comfortably within a single server's capacity**

#### How do we handle cache invalidation when a product image changes?

- Each image URL includes a **content hash**: `/images/abc123?v=sha256hash`
- When an image is updated, the hash changes → new URL → CDN treats it as a new resource
- For urgent invalidation: CDN APIs support `invalidate /images/abc123*` — clears all edge caches within ~60 seconds
- Metadata (which image ID → which product) cached in Redis with short TTL (5 min) so updates propagate quickly

#### What if we need even more scale?

- **S3 as origin**: Instead of our own servers, use S3 directly as the CDN origin. S3 handles effectively unlimited bandwidth.
- **Image resizing at the edge**: Use CloudFront Functions or Lambda@Edge to resize images on-the-fly at the edge node, rather than pre-generating all variants.
- **Multi-CDN**: Use multiple CDN providers (CloudFront + Akamai) with DNS-based routing for redundancy and to negotiate better pricing.
