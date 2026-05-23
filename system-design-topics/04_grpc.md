# gRPC

A comprehensive guide to gRPC for system design interviews: RPC semantics, Protocol Buffers, HTTP/2, streaming, deadlines, retries, load balancing, API evolution, and production readiness.

---

## Table of Contents

1. [What is gRPC?](#what-is-grpc)
2. [REST vs gRPC](#rest-vs-grpc)
3. [Core Concepts](#core-concepts)
4. [Protocol Buffers](#protocol-buffers)
5. [RPC Types](#rpc-types)
6. [HTTP/2 Under the Hood](#http2-under-the-hood)
7. [Deadlines, Timeouts, and Cancellation](#deadlines-timeouts-and-cancellation)
8. [Errors and Status Codes](#errors-and-status-codes)
9. [Load Balancing and Service Discovery](#load-balancing-and-service-discovery)
10. [Retries, Hedging, and Idempotency](#retries-hedging-and-idempotency)
11. [Security and Authentication](#security-and-authentication)
12. [Schema Evolution](#schema-evolution)
13. [Production Design Patterns](#production-design-patterns)
14. [Hands-On Exercises](#hands-on-exercises)
15. [Interview Questions](#interview-questions)
16. [Quick Reference](#quick-reference)

---

## What is gRPC?

gRPC is a high-performance remote procedure call framework. A service defines methods in a `.proto` file, and gRPC generates strongly typed clients and servers in many languages.

```
Client code calls local-looking method
        |
        v
Generated gRPC client stub
        |
        | HTTP/2 + protobuf
        v
Generated gRPC server handler
        |
        v
Service implementation
```

Example:

```proto
service UserService {
  rpc GetUser(GetUserRequest) returns (GetUserResponse);
}
```

Client code feels like:

```python
response = user_client.GetUser(GetUserRequest(user_id="u123"))
```

But it is a network call, so you must still design for latency, timeouts, retries, partial failure, authentication, and observability.

### Where gRPC Fits

Common use cases:

- Internal microservice communication
- Low-latency service-to-service APIs
- Strongly typed APIs across languages
- Streaming APIs
- Mobile clients when binary efficiency matters
- Control-plane APIs

Less ideal for:

- Public browser-first APIs without a proxy
- Simple CRUD APIs where REST is enough
- Human-debuggable APIs without tooling
- Systems where clients cannot easily generate stubs

---

## REST vs gRPC

| Aspect | REST/JSON | gRPC/Protobuf |
| --- | --- | --- |
| Transport | Usually HTTP/1.1 or HTTP/2 | HTTP/2 |
| Payload | JSON text | Protobuf binary |
| Contract | OpenAPI optional | `.proto` required |
| Browser support | Native | Needs gRPC-Web/proxy |
| Streaming | Limited/SSE/WebSocket | Built-in unary and streaming |
| Debugging | Easy with curl | Needs grpcurl or generated client |
| Performance | Good enough for many APIs | Lower overhead, strong typing |
| Best for | Public APIs, CRUD, web apps | Internal APIs, high-performance RPC |

### Interview Rule of Thumb

Use REST when:

- API is public and browser-friendly
- Human readability matters
- Resource-oriented CRUD maps naturally
- Clients are diverse and simple integration matters

Use gRPC when:

- Services are internal
- Strong contracts matter
- Low latency and efficient serialization matter
- You need streaming
- You control both client and server generation

---

## Core Concepts

### Service

A service is a collection of RPC methods.

```proto
service PaymentService {
  rpc AuthorizePayment(AuthorizePaymentRequest) returns (AuthorizePaymentResponse);
  rpc CapturePayment(CapturePaymentRequest) returns (CapturePaymentResponse);
}
```

### Method

An RPC method has request and response message types.

```proto
rpc GetOrder(GetOrderRequest) returns (GetOrderResponse);
```

### Stub

Generated client code that exposes service methods.

### Channel

A client-side abstraction representing a connection to a gRPC server. Channels are usually long-lived and reused.

### Metadata

Key-value data sent with RPC calls, similar to HTTP headers.

Use metadata for:

- auth tokens
- request IDs
- tenant IDs
- tracing context

Do not put business payloads in metadata.

### Interceptors

Middleware for clients and servers.

Use interceptors for:

- authentication
- logging
- tracing
- metrics
- retries
- request validation

---

## Protocol Buffers

Protocol Buffers, or protobuf, define the API contract and binary message format.

### Example `.proto`

```proto
syntax = "proto3";

package interview.users.v1;

service UserService {
  rpc GetUser(GetUserRequest) returns (GetUserResponse);
  rpc ListUsers(ListUsersRequest) returns (ListUsersResponse);
}

message GetUserRequest {
  string user_id = 1;
}

message GetUserResponse {
  User user = 1;
}

message ListUsersRequest {
  int32 page_size = 1;
  string page_token = 2;
}

message ListUsersResponse {
  repeated User users = 1;
  string next_page_token = 2;
}

message User {
  string user_id = 1;
  string email = 2;
  string display_name = 3;
  int64 created_at_epoch_ms = 4;
}
```

### Field Numbers Matter

In protobuf, field numbers are part of the wire format.

```proto
string email = 2;
```

Do not reuse field number `2` for a different meaning later.

### Common Types

```proto
string id = 1;
int64 count = 2;
bool enabled = 3;
repeated string tags = 4;
map<string, string> labels = 5;
google.protobuf.Timestamp created_at = 6;
```

### `proto3` Defaults

In proto3, fields have default values:

- string: empty string
- numeric: 0
- bool: false
- repeated: empty list

This can make "unset" vs "set to default" ambiguous. Use wrapper types or `optional` when presence matters.

### Protobuf vs JSON

Protobuf advantages:

- Smaller payloads
- Faster serialization
- Strong generated types
- Backward-compatible evolution if used correctly

JSON advantages:

- Human-readable
- Easy ad hoc calls
- Native browser support
- Flexible for loosely typed integrations

---

## RPC Types

gRPC supports four call shapes.

### Unary RPC

One request, one response.

```proto
rpc GetUser(GetUserRequest) returns (GetUserResponse);
```

Use for normal request-response APIs.

### Server Streaming RPC

One request, stream of responses.

```proto
rpc WatchOrder(WatchOrderRequest) returns (stream OrderEvent);
```

Use for:

- live status updates
- server push
- log tailing
- watch APIs

### Client Streaming RPC

Stream of requests, one response.

```proto
rpc UploadMetrics(stream MetricPoint) returns (UploadSummary);
```

Use for:

- uploading chunks
- telemetry ingestion
- batch upload

### Bidirectional Streaming RPC

Stream in both directions.

```proto
rpc Chat(stream ChatMessage) returns (stream ChatMessage);
```

Use for:

- chat
- collaborative editing
- real-time control protocols
- interactive sessions

### Streaming Design Warnings

Streaming is powerful but operationally harder:

- Long-lived connections complicate load balancing
- Backpressure matters
- Deadlines and cancellation need thought
- Retries are more complex
- Server resource leaks hurt more

For many systems, unary RPC plus Kafka/events is simpler.

---

## HTTP/2 Under the Hood

gRPC uses HTTP/2.

Important HTTP/2 features:

- Multiplexing: multiple streams over one TCP connection
- Header compression
- Binary framing
- Flow control
- Long-lived connections

### Multiplexing

HTTP/1.1 often needs many connections to parallelize requests.

HTTP/2 can multiplex:

```
One TCP connection:
  stream 1: GetUser
  stream 3: ListOrders
  stream 5: AuthorizePayment
```

### Head-of-Line Blocking

HTTP/2 avoids application-layer head-of-line blocking between streams on one connection, but TCP-level packet loss can still affect all streams on that connection.

### Flow Control

Flow control prevents a sender from overwhelming a receiver.

In streaming RPCs, respect backpressure. Do not blindly buffer infinite messages in memory.

---

## Deadlines, Timeouts, and Cancellation

### Always Set Deadlines

A gRPC call without a deadline can hang too long and consume resources.

```
Client -> Service A -> Service B -> Service C
```

If each call waits indefinitely, failures cascade.

Set deadlines based on user-facing latency budgets.

Example:

```
Overall request budget: 500 ms
Service A internal work: 50 ms
Service B deadline: 200 ms
Service C deadline: 150 ms
Buffer: 100 ms
```

### Deadline Propagation

Services should pass remaining deadline downstream.

```
Client deadline: 500 ms
Service A receives after 50 ms
Service A calls B with remaining ~450 ms, not a fresh 500 ms
```

### Cancellation

If the client cancels or deadline expires, the server should stop unnecessary work.

Server handlers should check cancellation context before expensive operations.

### Interview Sound Bite

"Every gRPC call gets a deadline. I propagate deadlines downstream, cancel wasted work, and expose deadline exceeded metrics."

---

## Errors and Status Codes

gRPC has canonical status codes.

Common codes:

| Code | Use |
| --- | --- |
| `OK` | Success |
| `INVALID_ARGUMENT` | Bad request regardless of system state |
| `NOT_FOUND` | Entity not found |
| `ALREADY_EXISTS` | Create conflicts with existing entity |
| `PERMISSION_DENIED` | Authenticated but not allowed |
| `UNAUTHENTICATED` | Missing/invalid authentication |
| `RESOURCE_EXHAUSTED` | Quota/rate limit exceeded |
| `FAILED_PRECONDITION` | System state prevents operation |
| `ABORTED` | Concurrency conflict, retry higher-level transaction |
| `UNAVAILABLE` | Service temporarily unavailable |
| `DEADLINE_EXCEEDED` | Deadline expired |
| `INTERNAL` | Server bug/unexpected error |

### Retryable vs Non-Retryable

Usually retryable:

- `UNAVAILABLE`
- `DEADLINE_EXCEEDED` in some cases
- `RESOURCE_EXHAUSTED` with backoff if quota allows
- `ABORTED` for transaction conflicts

Usually not retryable:

- `INVALID_ARGUMENT`
- `NOT_FOUND`
- `PERMISSION_DENIED`
- `UNAUTHENTICATED` without new credentials

### Rich Error Details

Use structured error details for machine-readable context.

Examples:

- validation field violations
- retry delay
- quota failure
- localized message

Keep error messages safe. Do not leak secrets or internal stack traces to clients.

---

## Load Balancing and Service Discovery

### Problem

gRPC uses long-lived HTTP/2 connections. A normal L4 load balancer may send a client connection to one backend and keep all multiplexed requests there.

```
Client connection -> Backend A
All streams on that connection may stay on A
```

This can reduce balancing quality.

### Approaches

### Proxy Load Balancing

Use Envoy, Nginx, HAProxy, or a cloud load balancer that understands HTTP/2/gRPC.

```
Client -> Envoy/LB -> Backend pool
```

Pros:

- Central control
- Good observability
- Language-agnostic

Cons:

- Extra hop
- Proxy config complexity

### Client-Side Load Balancing

Client gets backend list from service discovery and balances itself.

```
Client -> service discovery -> [backend1, backend2, backend3]
Client picks backend per policy
```

Pros:

- Avoids extra proxy hop
- Can make smart per-RPC decisions

Cons:

- More client complexity
- Must keep service discovery updated

### Kubernetes Pattern

Common production path:

```
gRPC client -> Kubernetes Service / service mesh -> gRPC pods
```

For advanced traffic management, use a service mesh or Envoy-based ingress that supports gRPC.

---

## Retries, Hedging, and Idempotency

### Retries

Retries can improve availability for transient failures, but they can also amplify load.

Good retry policy:

- Retry only safe/idempotent operations
- Use exponential backoff with jitter
- Respect deadlines
- Cap attempts
- Do not retry validation errors

### Retry Storm

If Service B is overloaded, aggressive retries from Service A can make it worse.

Mitigations:

- Backoff with jitter
- Circuit breakers
- Rate limiting
- Deadline budgets
- Load shedding

### Hedging

Hedging sends a duplicate request after a delay to reduce tail latency.

```
send request to backend A
if no response after 50 ms, send duplicate to backend B
use first successful response
cancel the other
```

Use only for idempotent reads and with strict caps, because hedging increases load.

### Idempotency

For writes, include idempotency keys.

```proto
message CreatePaymentRequest {
  string merchant_id = 1;
  string idempotency_key = 2;
  int64 amount_cents = 3;
}
```

Server stores:

```
(merchant_id, idempotency_key) -> result
```

If the client retries after a timeout, the server returns the same result instead of creating duplicate payments.

---

## Security and Authentication

### Transport Security

Use TLS for encryption in transit.

For internal service-to-service traffic, mTLS is common:

```
Client cert proves client identity
Server cert proves server identity
Traffic is encrypted
```

### Authentication

Common methods:

- Bearer token in metadata
- mTLS identity
- API keys for machine clients
- JWTs for user context

Example metadata:

```text
authorization: Bearer <token>
x-request-id: req_123
x-tenant-id: tenant_42
```

### Authorization

Authentication answers "who are you?" Authorization answers "what are you allowed to do?"

Enforce authorization on the server side. Do not rely on clients hiding methods.

### Security Checklist

- TLS/mTLS
- Auth interceptor
- Per-method authorization
- Request size limits
- Rate limits
- Audit logs for sensitive methods
- Safe error messages
- Dependency and protobuf generation hygiene

---

## Schema Evolution

### Backward-Compatible Changes

Generally safe:

- Add new fields with new field numbers
- Add new optional fields
- Add new RPC methods
- Add enum values if clients handle unknowns

### Breaking Changes

Avoid:

- Reusing field numbers
- Changing field type
- Removing fields still used by clients
- Renaming fields if JSON mapping matters
- Changing meaning of existing field
- Changing request/response semantics silently

### Reserved Fields

When removing a field, reserve its number and name.

```proto
message User {
  string user_id = 1;
  reserved 2;
  reserved "old_email";
}
```

This prevents accidental reuse.

### Versioning Services

Use package or service versioning:

```proto
package payments.v1;
```

For breaking changes, introduce `v2` and run both during migration.

### Compatibility Strategy

1. Add new fields/methods.
2. Deploy servers that understand old and new.
3. Migrate clients.
4. Stop using old fields.
5. Later remove and reserve old fields.

---

## Production Design Patterns

### Pattern 1: Internal Microservice API

```
API Gateway -> Order Service -> Payment Service
                         |--> Inventory Service
```

Use:

- Unary RPCs for request-response
- Deadlines on every call
- Metadata for trace/request ID
- Interceptors for auth/logging/metrics
- Idempotency keys for write methods

### Pattern 2: Watch API

```proto
rpc WatchDeployments(WatchDeploymentsRequest) returns (stream DeploymentEvent);
```

Design details:

- Client passes starting version/cursor
- Server streams events
- Client reconnects with last seen version
- Server enforces max stream lifetime
- Heartbeats detect dead connections

### Pattern 3: Bulk Upload

```proto
rpc UploadLogs(stream LogChunk) returns (UploadSummary);
```

Design details:

- Bound max chunk size
- Apply backpressure
- Return per-chunk errors if needed
- Use resumable upload ID for large uploads

### Pattern 4: Public API via gRPC-Gateway

Expose REST/JSON externally but use gRPC internally:

```
Public client -> REST/JSON gateway -> internal gRPC services
```

Pros:

- Browser/public clients get REST
- Internal services get typed gRPC

Cons:

- Need mapping layer
- Error/status translation matters

### Pattern 5: Observability

Every RPC should emit:

- service/method
- status code
- latency histogram
- request/response size
- deadline exceeded count
- retry count
- trace ID

Log request IDs, not full sensitive payloads.

---

## Hands-On Exercises

### Exercise 1: Write a Proto Contract

Create `users.proto`:

```proto
syntax = "proto3";

package users.v1;

service UserService {
  rpc GetUser(GetUserRequest) returns (GetUserResponse);
}

message GetUserRequest {
  string user_id = 1;
}

message GetUserResponse {
  string user_id = 1;
  string email = 2;
  string display_name = 3;
}
```

Explain:

- service
- method
- request type
- response type
- field numbers

### Exercise 2: Add a Backward-Compatible Field

Add:

```proto
string avatar_url = 4;
```

Explain why this is safe for older clients.

### Exercise 3: Design Deadlines

For a request with 800 ms total budget:

```
API Gateway -> Search Service -> Inventory Service
                         |--> Pricing Service
```

Assign deadlines for each downstream call and explain your reasoning.

### Exercise 4: Classify Status Codes

Choose gRPC status codes for:

- missing auth token
- user not found
- invalid email format
- quota exceeded
- downstream service unavailable
- optimistic concurrency conflict

### Exercise 5: Idempotent Write

Design `CreatePayment` with an idempotency key.

Explain:

- where the key is stored
- what response is returned on duplicate retry
- how long keys are retained
- what happens if original request is still processing

### Exercise 6: Streaming Trade-Off

Design a live order tracking API.

Compare:

- `GetOrderStatus` polled every 5 seconds
- `WatchOrderStatus` server-streaming RPC
- Kafka event consumed by notification service

Explain which you choose and why.

---

## Interview Questions

### Basic Questions

**Q: What is gRPC?**

gRPC is a high-performance RPC framework using Protocol Buffers for contracts and HTTP/2 for transport. It generates typed clients and servers across languages.

**Q: Why use gRPC instead of REST?**

Use gRPC for internal low-latency strongly typed service-to-service APIs, especially when streaming or generated clients are valuable. REST is often better for public browser-friendly APIs.

**Q: What is a `.proto` file?**

It defines service methods and message schemas. Code generators use it to produce clients, servers, and serializers.

**Q: What are field numbers in protobuf?**

Field numbers identify fields in the wire format. They must not be reused for different meanings.

### System Design Questions

**Q: How do you design a reliable gRPC API?**

Use deadlines, cancellation, retries only for idempotent operations, structured errors, authentication interceptors, request size limits, metrics, tracing, and backward-compatible proto evolution.

**Q: How do you handle gRPC load balancing?**

Use an HTTP/2-aware proxy such as Envoy or client-side load balancing with service discovery. Be aware that long-lived HTTP/2 connections can reduce fairness with naive L4 balancing.

**Q: How do you prevent cascading failures?**

Set deadlines, propagate remaining time, use bounded retries with jitter, circuit breakers, load shedding, and bulkheads.

**Q: How do you make write RPCs safe to retry?**

Use idempotency keys and store the result keyed by client/entity plus idempotency key. Return the original result on retry.

### Advanced Questions

**Q: What are the four gRPC call types?**

Unary, server streaming, client streaming, and bidirectional streaming.

**Q: Why is streaming harder operationally?**

Long-lived connections consume resources, require backpressure, complicate retries/load balancing, and need heartbeat/cancellation handling.

**Q: What status code should you use for rate limiting?**

`RESOURCE_EXHAUSTED`, optionally with retry details.

**Q: How do you evolve protobuf schemas safely?**

Add fields with new numbers, avoid changing field meanings or types, reserve removed field numbers/names, and introduce versioned services for breaking changes.

**Q: Does gRPC guarantee exactly-once execution?**

No. Network retries and timeouts can cause uncertainty. Exactly-once business behavior requires idempotency and deduplication at the application layer.

---

## Quick Reference

### gRPC Design Checklist

1. Define clear service ownership and method purpose
2. Use request/response messages even for single fields
3. Set deadlines on every call
4. Propagate cancellation and trace context
5. Use correct status codes
6. Make retries bounded and idempotency-aware
7. Add auth and authorization interceptors
8. Monitor latency, errors, payload size, and retries
9. Plan protobuf evolution
10. Document streaming behavior and backpressure

### Common Status Codes

| Scenario | Code |
| --- | --- |
| Invalid request field | `INVALID_ARGUMENT` |
| Missing login token | `UNAUTHENTICATED` |
| User lacks permission | `PERMISSION_DENIED` |
| Entity missing | `NOT_FOUND` |
| Duplicate create | `ALREADY_EXISTS` |
| Rate limited | `RESOURCE_EXHAUSTED` |
| Timeout | `DEADLINE_EXCEEDED` |
| Temporary outage | `UNAVAILABLE` |
| Server bug | `INTERNAL` |

### Interview Sound Bites

- "gRPC is still a network call, so deadlines and retries are mandatory design choices."
- "Protobuf field numbers are forever; reserve removed fields."
- "HTTP/2 multiplexing changes load-balancing behavior."
- "Use idempotency keys for retryable writes."
- "REST is often better externally; gRPC is often better internally."
