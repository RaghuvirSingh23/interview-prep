# Load Balancers & Nginx

A comprehensive guide to load balancing for system design interviews: L4 vs L7, algorithms, health checks, reverse proxies, TLS termination, sticky sessions, Nginx configuration, failure modes, and production patterns.

---

## Table of Contents

1. [What is a Load Balancer?](#what-is-a-load-balancer)
2. [Why Load Balancers Matter](#why-load-balancers-matter)
3. [Layer 4 vs Layer 7](#layer-4-vs-layer-7)
4. [Load Balancing Algorithms](#load-balancing-algorithms)
5. [Health Checks and Failover](#health-checks-and-failover)
6. [Reverse Proxy Concepts](#reverse-proxy-concepts)
7. [Nginx Basics](#nginx-basics)
8. [TLS Termination](#tls-termination)
9. [Session Affinity and State](#session-affinity-and-state)
10. [Global Load Balancing](#global-load-balancing)
11. [High Availability for Load Balancers](#high-availability-for-load-balancers)
12. [Common System Design Patterns](#common-system-design-patterns)
13. [Operational Playbook](#operational-playbook)
14. [Hands-On Exercises](#hands-on-exercises)
15. [Interview Questions](#interview-questions)
16. [Quick Reference](#quick-reference)

---

## What is a Load Balancer?

A load balancer distributes traffic across multiple backend servers.

```
Clients
   |
   v
+----------------+
| Load Balancer  |
+---+--------+---+
    |        |
    v        v
 Server A  Server B  Server C
```

Load balancers improve:

- availability
- scalability
- failover
- deployment safety
- traffic routing control
- TLS and security management

### Load Balancer vs Reverse Proxy

A reverse proxy accepts requests from clients and forwards them to backend servers. A load balancer is a reverse proxy that also distributes load across multiple backends.

In practice, products like Nginx, HAProxy, Envoy, and cloud load balancers can do both.

---

## Why Load Balancers Matter

### Problem: Single Server

```
Client -> Server
```

Issues:

- Server failure causes outage
- Limited CPU/memory/network
- Deployments interrupt traffic
- No traffic shaping

### Solution: Multiple Servers Behind a Load Balancer

```
Client -> LB -> Server 1
             -> Server 2
             -> Server 3
```

Benefits:

- Add servers to scale horizontally
- Remove unhealthy servers automatically
- Deploy gradually
- Centralize TLS termination
- Route by hostname/path/header

### Load Balancer in a Typical Web Architecture

```
Internet
   |
   v
DNS
   |
   v
CDN / Edge
   |
   v
External Load Balancer
   |
   v
API Gateway / Ingress
   |
   v
Service Load Balancer
   |
   v
Application Pods/Servers
```

Not every system needs all layers. Mention only what the design requires.

---

## Layer 4 vs Layer 7

### Layer 4 Load Balancing

L4 load balancers operate at TCP/UDP level.

They see:

- source IP/port
- destination IP/port
- protocol

They do not understand HTTP paths, headers, cookies, or methods.

```
TCP connection -> L4 LB -> backend server
```

Pros:

- Fast
- Lower overhead
- Protocol-agnostic
- Good for TCP services, databases, gRPC pass-through

Cons:

- Cannot route by HTTP path/header
- Less application-aware

Examples:

- AWS Network Load Balancer
- Linux IPVS
- HAProxy TCP mode
- Envoy TCP proxy

### Layer 7 Load Balancing

L7 load balancers understand application protocols such as HTTP.

They can route by:

- hostname
- path
- header
- cookie
- method
- request body in some gateways

```
GET /api/orders -> order-service
GET /api/users  -> user-service
```

Pros:

- Smart routing
- TLS termination
- auth integration
- rate limiting
- compression
- redirects
- canary traffic

Cons:

- More CPU overhead
- More complex
- Protocol-specific

Examples:

- Nginx
- HAProxy HTTP mode
- Envoy
- AWS Application Load Balancer
- Kubernetes Ingress controllers

### Interview Rule

Use L4 when you need simple fast connection-level distribution.

Use L7 when you need HTTP-aware routing, TLS termination, canarying, auth, or request-level policies.

---

## Load Balancing Algorithms

### Round Robin

Requests are sent in order to each backend.

```
Req1 -> A
Req2 -> B
Req3 -> C
Req4 -> A
```

Pros:

- Simple
- Good when servers are equal

Cons:

- Ignores server load and request cost

### Weighted Round Robin

Servers get traffic proportional to weights.

```
A weight 5
B weight 1
```

A receives about 5x B's traffic.

Use when servers have different capacity.

### Least Connections

Send request to server with fewest active connections.

Good for long-lived connections or uneven request durations.

### Least Response Time

Prefer servers with lower latency and fewer active connections.

Useful but can be unstable if measurements are noisy.

### Random / Power of Two Choices

Pick two random servers, choose the less loaded one.

This is simple and performs surprisingly well at scale.

### IP Hash

Hash client IP to choose backend.

```
backend = hash(client_ip) % N
```

Provides basic stickiness, but breaks when backend count changes and performs poorly behind NAT.

### Consistent Hashing

Maps keys and servers onto a hash ring. Adding/removing a server moves only part of the keys.

Use for:

- cache routing
- sticky state by user/session
- sharded services

```
hash(user_id) -> ring -> backend
```

### Algorithm Selection

| Workload | Good algorithm |
| --- | --- |
| Equal stateless HTTP servers | round robin |
| Unequal server sizes | weighted round robin |
| Long-lived connections | least connections |
| Cache affinity | consistent hashing |
| High-scale simple balancing | random two choices |
| Session stickiness | cookie or consistent hash |

---

## Health Checks and Failover

### Passive Health Checks

Load balancer observes real traffic failures:

- connection refused
- timeout
- 5xx errors

### Active Health Checks

Load balancer probes backends periodically.

```
GET /healthz
```

If checks fail, backend is removed from rotation.

### Health Endpoint Design

Use separate endpoints:

- `/live`: process is alive
- `/ready`: can serve traffic

Readiness should fail when:

- app is starting
- app is shutting down
- critical local dependency is unavailable

Be careful with deep dependency checks. If every service marks itself unhealthy because a shared database has a small issue, the whole system can flap.

### Failover Timeline

```
Backend fails
   |
Health check fails N times
   |
LB removes backend
   |
Traffic goes to remaining backends
```

Detection is not instant. Tune:

- interval
- timeout
- healthy threshold
- unhealthy threshold

### Outlier Detection

Advanced proxies such as Envoy can eject backends with high error rates or latency even if health checks pass.

---

## Reverse Proxy Concepts

### Forward Proxy vs Reverse Proxy

Forward proxy represents clients:

```
Client -> Forward Proxy -> Internet
```

Reverse proxy represents servers:

```
Internet -> Reverse Proxy -> Backend Servers
```

### Common Reverse Proxy Features

- TLS termination
- compression
- request buffering
- response buffering
- static file serving
- routing
- rate limiting
- auth integration
- caching
- header manipulation

### Important Headers

When traffic passes through proxies, preserve original request context:

```text
X-Forwarded-For: client_ip, proxy1, proxy2
X-Forwarded-Proto: https
X-Forwarded-Host: example.com
X-Request-Id: req_123
```

Security warning: only trust forwarded headers from trusted proxies. Clients can spoof them.

### Connection Draining

When removing a backend:

1. Stop sending new requests to it.
2. Allow in-flight requests to complete.
3. Shutdown after grace period.

This is essential for zero-downtime deployments.

---

## Nginx Basics

Nginx can serve static files, reverse proxy HTTP/gRPC/WebSocket traffic, terminate TLS, and load balance upstreams.

### Basic Reverse Proxy

```nginx
events {}

http {
    server {
        listen 80;

        location / {
            proxy_pass http://app:8080;
            proxy_set_header Host $host;
            proxy_set_header X-Real-IP $remote_addr;
            proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
            proxy_set_header X-Forwarded-Proto $scheme;
        }
    }
}
```

### Upstream Load Balancing

```nginx
events {}

http {
    upstream app_backend {
        server app1:8080;
        server app2:8080;
        server app3:8080;
    }

    server {
        listen 80;

        location / {
            proxy_pass http://app_backend;
        }
    }
}
```

Default is round robin.

### Least Connections

```nginx
upstream app_backend {
    least_conn;
    server app1:8080;
    server app2:8080;
    server app3:8080;
}
```

### Weighted Servers

```nginx
upstream app_backend {
    server app1:8080 weight=5;
    server app2:8080 weight=1;
}
```

### Basic Health Behavior

Open-source Nginx marks a server failed based on proxy failures:

```nginx
upstream app_backend {
    server app1:8080 max_fails=3 fail_timeout=10s;
    server app2:8080 max_fails=3 fail_timeout=10s;
}
```

Active health checks require Nginx Plus or other tooling/modules. In Kubernetes, health is often handled by readiness probes and endpoint removal.

### Timeouts

```nginx
location / {
    proxy_connect_timeout 2s;
    proxy_send_timeout 10s;
    proxy_read_timeout 10s;
    proxy_pass http://app_backend;
}
```

Timeouts protect the proxy and clients from hanging forever.

### Buffering

Nginx can buffer requests/responses. This can protect upstreams but may increase memory/disk use.

For streaming endpoints, buffering may need to be disabled.

```nginx
proxy_buffering off;
```

### WebSockets

```nginx
location /ws {
    proxy_pass http://app_backend;
    proxy_http_version 1.1;
    proxy_set_header Upgrade $http_upgrade;
    proxy_set_header Connection "upgrade";
}
```

### gRPC Proxying

```nginx
server {
    listen 80 http2;

    location / {
        grpc_pass grpc://grpc_backend;
    }
}

upstream grpc_backend {
    server app1:50051;
    server app2:50051;
}
```

---

## TLS Termination

TLS termination means the load balancer decrypts HTTPS and forwards to backends.

```
Client --HTTPS--> LB --HTTP or HTTPS--> Backend
```

Pros:

- Central certificate management
- Offloads TLS work from apps
- Enables HTTP routing by path/header
- Simplifies application servers

Cons:

- Traffic from LB to backend may be plaintext unless re-encrypted
- LB becomes sensitive security boundary

### TLS Passthrough

The load balancer passes encrypted traffic to backend without decrypting.

Pros:

- End-to-end encryption to app
- LB cannot inspect sensitive payload

Cons:

- Less L7 routing
- Backend manages certs

### Re-Encryption

```
Client --HTTPS--> LB --HTTPS--> Backend
```

Useful for zero-trust or regulated environments.

### Certificate Rotation

Production needs:

- automated renewal
- safe reload
- monitoring expiration
- support for multiple hostnames/SNI

---

## Session Affinity and State

### Sticky Sessions

Sticky sessions route the same client to the same backend.

Methods:

- cookie-based affinity
- IP hash
- consistent hash by session/user ID

### Why Sticky Sessions Are Risky

They can hide stateful app design problems.

Problems:

- backend failure loses session state
- uneven traffic distribution
- scaling changes move users
- hard deployments

Better:

- store session state in Redis/database
- make app servers stateless
- use sticky sessions only when truly needed

### WebSockets and Long-Lived Connections

Long-lived connections need special care:

- least connections may work better than round robin
- deployments need connection draining
- backend capacity is connection-count sensitive
- idle timeouts must match application expectations

---

## Global Load Balancing

Global load balancing routes users across regions.

```
User in India -> Asia region
User in Europe -> Europe region
User in US -> US region
```

Approaches:

- DNS-based routing
- Anycast
- global load balancer products
- CDN edge routing

### DNS Load Balancing

DNS returns different IPs based on latency, geo, health, or weights.

Pros:

- Simple
- Global reach

Cons:

- DNS caching delays failover
- Clients may ignore low TTLs
- Less precise than request-level routing

### Active-Active vs Active-Passive

Active-active:

- multiple regions serve traffic simultaneously
- lower latency
- harder data consistency

Active-passive:

- one primary region serves traffic
- standby region takes over on failure
- simpler consistency
- slower failover

---

## High Availability for Load Balancers

The load balancer itself can be a single point of failure.

### HA Pattern

```
             DNS / VIP
                |
       +--------+--------+
       |                 |
   Load Balancer A   Load Balancer B
       |                 |
       +--------+--------+
                |
             Backends
```

### Techniques

- managed cloud load balancer
- multiple LB instances behind DNS
- virtual IP with keepalived/VRRP
- Anycast
- active-active proxy layer

### Capacity Planning

Load balancers need capacity for:

- requests per second
- concurrent connections
- TLS handshakes
- bandwidth
- header/body buffering
- logging overhead

TLS handshakes and long-lived connections are common bottlenecks.

---

## Common System Design Patterns

### Pattern 1: Stateless Web Service

```
Clients -> L7 LB -> app servers -> database/cache
```

Use:

- round robin or least connections
- health checks
- readiness before traffic
- stateless app servers
- session in Redis/database

### Pattern 2: Path-Based Microservice Routing

```
/api/users  -> user-service
/api/orders -> order-service
/api/pay    -> payment-service
```

This is common at API gateway/ingress layer.

### Pattern 3: Canary Release

```
99% traffic -> v1
1% traffic  -> v2
```

Increase traffic if metrics are healthy:

- error rate
- p95/p99 latency
- saturation
- business metrics

### Pattern 4: Blue-Green Deployment

```
Blue: current production
Green: new version warmed up
Switch LB from Blue to Green
```

Pros:

- quick rollback
- clear separation

Cons:

- requires duplicate capacity
- database migrations still hard

### Pattern 5: Cache-Aware Routing

For cache servers, use consistent hashing so the same key usually goes to the same node.

```
hash(cache_key) -> cache node
```

This improves hit ratio and reduces reshuffling when nodes change.

---

## Operational Playbook

### Metrics to Monitor

Load balancer:

- request rate
- active connections
- connection errors
- 4xx/5xx counts
- upstream 5xx
- p50/p95/p99 latency
- TLS handshake failures
- backend health
- bytes in/out

Backend pool:

- per-backend request count
- per-backend latency
- per-backend error rate
- number of healthy hosts

### Common Incidents

| Symptom | Likely cause | Checks |
| --- | --- | --- |
| 502 Bad Gateway | backend connection failed | app up, port, network, timeouts |
| 503 Service Unavailable | no healthy backends | health checks, readiness, endpoints |
| 504 Gateway Timeout | backend too slow | app latency, DB latency, proxy timeout |
| Uneven traffic | bad algorithm/stickiness/hot clients | per-backend metrics |
| TLS errors | cert expired/mismatch | cert chain, SNI, renewal |
| WebSocket drops | idle timeout too short | LB/proxy timeouts |

### Timeout Strategy

Timeouts should decrease as requests go deeper:

```
Client timeout: 10s
External LB: 9s
API gateway: 8s
Service A -> B: 2s
Database query: 1s
```

This prevents upstreams from waiting longer than callers.

### Rate Limiting at the Edge

Load balancers/gateways often enforce coarse rate limits:

- per IP
- per API key
- per tenant
- per route

Application services may enforce finer business limits.

---

## Hands-On Exercises

### Exercise 1: Run Two Backends

Start two simple HTTP servers on different ports:

```bash
python3 -m http.server 8001
python3 -m http.server 8002
```

Use Nginx upstream to balance between them.

### Exercise 2: Nginx Round Robin

Create `nginx.conf`:

```nginx
events {}

http {
    upstream app_backend {
        server 127.0.0.1:8001;
        server 127.0.0.1:8002;
    }

    server {
        listen 8080;

        location / {
            proxy_pass http://app_backend;
        }
    }
}
```

Run:

```bash
nginx -c /absolute/path/to/nginx.conf
curl http://localhost:8080
```

### Exercise 3: Add Headers

Add:

```nginx
proxy_set_header X-Real-IP $remote_addr;
proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
proxy_set_header X-Forwarded-Proto $scheme;
```

Explain why applications need forwarded headers.

### Exercise 4: Test Failure

Stop one backend and observe:

- Does traffic continue?
- How many failures before Nginx avoids the backend?
- What status code do clients see?

### Exercise 5: Path Routing

Configure:

```nginx
location /api/users {
    proxy_pass http://user_backend;
}

location /api/orders {
    proxy_pass http://order_backend;
}
```

Explain why this is L7 routing.

### Exercise 6: Design a Canary

Sketch how to route:

- 95% to stable
- 5% to canary

Then list metrics you would watch before increasing canary traffic.

---

## Interview Questions

### Basic Questions

**Q: What is a load balancer?**

A component that distributes traffic across multiple backend servers to improve availability, scalability, and failover.

**Q: L4 vs L7 load balancing?**

L4 balances at TCP/UDP connection level. L7 understands application protocols like HTTP and can route by host, path, headers, cookies, and methods.

**Q: What is a reverse proxy?**

A server-facing proxy that accepts client requests and forwards them to backend servers.

**Q: What is TLS termination?**

The load balancer decrypts HTTPS traffic, handles certificates, and forwards requests to backends over HTTP or HTTPS.

### Algorithm Questions

**Q: Round robin vs least connections?**

Round robin distributes requests evenly by count. Least connections sends traffic to the backend with fewest active connections, which is better for long-lived or uneven requests.

**Q: When use consistent hashing?**

When requests for the same key should go to the same backend, such as cache routing or stateful shard routing, while minimizing key movement when nodes change.

**Q: What are sticky sessions?**

A routing strategy that sends the same client/session to the same backend. Useful sometimes, but stateless app servers with shared session storage are usually better.

### Reliability Questions

**Q: How do health checks work?**

The load balancer probes backends or observes failures. Unhealthy backends are removed from rotation until they recover.

**Q: How do you avoid the load balancer as a single point of failure?**

Run multiple load balancer instances, use managed cloud LBs, virtual IP failover, DNS/Anycast, and monitor LB capacity and health.

**Q: What is connection draining?**

Stop sending new requests to a backend while allowing existing requests to complete before shutting it down.

**Q: Why can a 504 happen?**

The proxy timed out waiting for the upstream backend. Causes include slow app, slow database, network issues, or too-short proxy timeout.

### Design Questions

**Q: Design load balancing for a web app.**

Use DNS to route to an external L7 load balancer, terminate TLS, route to app servers using round robin/least connections, health-check `/ready`, drain connections on deploy, keep servers stateless, store sessions in Redis/database, and monitor latency/errors/backend health.

**Q: How would you route traffic across regions?**

Use DNS/geolocation/latency-based routing or a global load balancer. Choose active-active for low latency but solve data consistency, or active-passive for simpler failover.

**Q: How do you support WebSockets?**

Use a proxy that supports connection upgrade, tune idle timeouts, use least connections, handle long-lived connection capacity, and drain gracefully during deploys.

**Q: How do load balancers interact with Kubernetes?**

Kubernetes Services provide stable internal load balancing. Ingress/Gateway controllers provide L7 external routing. Cloud LoadBalancer Services provision external LBs.

---

## Quick Reference

### Algorithm Cheat Sheet

| Algorithm | Best use |
| --- | --- |
| Round robin | equal stateless servers |
| Weighted round robin | unequal server capacity |
| Least connections | long-lived or uneven requests |
| Least response time | latency-sensitive traffic |
| IP hash | simple client stickiness |
| Consistent hash | cache/shard affinity |

### Status Code Cheat Sheet

| Code | Often means |
| --- | --- |
| 400 | bad client request |
| 401/403 | auth/authz failure |
| 404 | route/resource missing |
| 429 | rate limited |
| 500 | app error |
| 502 | bad gateway/upstream connection issue |
| 503 | no healthy upstream or overloaded |
| 504 | upstream timeout |

### Nginx Commands

```bash
nginx -t
nginx -s reload
nginx -s stop
```

### Interview Checklist

When discussing a load balancer, mention:

1. L4 or L7 and why
2. Load balancing algorithm
3. Health checks
4. TLS termination or passthrough
5. Connection draining
6. Stateless backends or session strategy
7. LB high availability
8. Metrics and failure modes
