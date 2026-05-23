# Observability (Logging, Metrics, Tracing)

A comprehensive guide to observability for system design interviews: logs, metrics, traces, SLOs, alerting, dashboards, instrumentation, debugging, and production incident readiness.

---

## Table of Contents

1. [What is Observability?](#what-is-observability)
2. [Why Observability Matters](#why-observability-matters)
3. [The Three Pillars](#the-three-pillars)
4. [Logs](#logs)
5. [Metrics](#metrics)
6. [Tracing](#tracing)
7. [SLIs, SLOs, and Error Budgets](#slis-slos-and-error-budgets)
8. [Alerting](#alerting)
9. [Dashboards](#dashboards)
10. [Instrumentation Strategy](#instrumentation-strategy)
11. [OpenTelemetry Mental Model](#opentelemetry-mental-model)
12. [High Cardinality and Cost Control](#high-cardinality-and-cost-control)
13. [Debugging Playbooks](#debugging-playbooks)
14. [Observability in System Design](#observability-in-system-design)
15. [Hands-On Exercises](#hands-on-exercises)
16. [Interview Questions](#interview-questions)
17. [Quick Reference](#quick-reference)

---

## What is Observability?

Observability is the ability to understand a system's internal state from its external outputs.

In practice, observability answers:

- Is the system healthy?
- What changed?
- Where is latency coming from?
- Why are errors increasing?
- Which users/tenants/routes are affected?
- Is the problem in app code, database, cache, queue, network, or dependency?

### Monitoring vs Observability

Monitoring tells you known things are wrong.

Observability helps you investigate unknown failure modes.

Example:

```text
Monitoring: p99 latency is above 1 second.
Observability: p99 is high only for POST /checkout, tenant=t42, because payment_provider latency increased.
```

---

## Why Observability Matters

In interviews, observability separates toy architecture from production architecture.

A production system needs:

- metrics to detect symptoms
- logs to inspect details
- traces to follow requests across services
- alerts to notify humans
- dashboards to understand system health
- correlation IDs to connect signals
- SLOs to define what "good" means

### Common Failure Without Observability

```
User: checkout is slow
Engineer: app logs look fine
DB team: database looks fine
Network team: no obvious issue
Everyone guesses for 2 hours
```

With observability:

```
Alert: checkout p99 latency high
Dashboard: latency increase starts at payment RPC
Trace: PaymentService -> provider API takes 3s
Logs: provider timeout rate increased for region ap-south
Action: fail over provider or degrade payment method
```

---

## The Three Pillars

### Logs

Discrete events, usually text or structured JSON.

Best for:

- errors
- request details
- audit events
- debugging specific failures

### Metrics

Numeric time series.

Best for:

- alerting
- dashboards
- trends
- capacity planning

### Traces

Request path across services.

Best for:

- distributed latency debugging
- dependency mapping
- finding failing spans

### How They Work Together

```
Metric alert: checkout error rate high
    |
    v
Trace: errors happen in PaymentService authorize call
    |
    v
Logs: provider returned timeout for request_id=req_123
```

---

## Logs

### Structured Logs

Prefer structured logs over free-form strings.

Example:

```json
{
  "timestamp": "2026-05-23T10:30:00Z",
  "level": "ERROR",
  "service": "payment-service",
  "message": "payment authorization failed",
  "request_id": "req_123",
  "user_id": "u42",
  "order_id": "ord_99",
  "provider": "stripe",
  "error_code": "timeout",
  "latency_ms": 2500
}
```

Benefits:

- queryable fields
- easier aggregation
- consistent parsing

### Log Levels

| Level | Use |
| --- | --- |
| DEBUG | detailed development/debug info |
| INFO | normal meaningful events |
| WARN | unexpected but handled condition |
| ERROR | operation failed and needs attention |
| FATAL | process cannot continue |

Do not log every line as ERROR. Alert fatigue follows.

### What to Log

Log:

- request start/end for important boundaries if sampling/cost allows
- errors with context
- state transitions
- retries and final failures
- security-sensitive audit events
- background job lifecycle

Avoid:

- passwords, tokens, secrets
- full credit card numbers
- unnecessary PII
- giant payloads
- high-volume debug logs in production

### Correlation IDs

Every request should have a request ID.

```
Client -> API Gateway -> Service A -> Service B -> DB
           request_id=req_123 propagated everywhere
```

This lets you search all logs for one request.

### Audit Logs

Audit logs answer "who did what, when?"

Use for:

- admin actions
- permission changes
- financial operations
- data exports
- login/security events

Audit logs should be tamper-resistant and retained according to policy.

---

## Metrics

Metrics are numeric measurements over time.

### Metric Types

### Counter

Monotonically increasing value.

Examples:

- total requests
- total errors
- total bytes sent

### Gauge

Value that can go up or down.

Examples:

- memory usage
- queue depth
- active connections
- CPU usage

### Histogram

Distribution of values, usually for latency or size.

Examples:

- request duration
- database query duration
- response size

Histograms are essential for percentiles.

### RED Method

For request-driven services, monitor:

- **Rate**: requests/sec
- **Errors**: error rate
- **Duration**: latency

Example:

```text
http_requests_total
http_request_errors_total
http_request_duration_seconds
```

### USE Method

For resources, monitor:

- **Utilization**: how busy
- **Saturation**: queued work
- **Errors**: failures

Examples:

- CPU utilization
- disk queue depth
- disk errors
- network drops

### Golden Signals

From SRE practice:

- latency
- traffic
- errors
- saturation

### Percentiles

Averages hide tail latency.

Use:

- p50: typical user
- p95: slow users
- p99: very slow tail

Example:

```text
p50 = 80 ms
p95 = 400 ms
p99 = 2,000 ms
```

This means most users are fine but a small fraction suffer badly.

### Business Metrics

Also track business outcomes:

- checkout success rate
- payment authorization failures
- signup conversion
- search zero-result rate
- emails sent
- orders created

Technical health can look fine while business flow is broken.

---

## Tracing

Distributed tracing follows one request across services.

### Trace, Span, Context

Trace:

- whole request journey

Span:

- one operation in the trace

Context:

- trace ID/span ID propagated between services

Example:

```
Trace req_123
  API Gateway                20 ms
  OrderService.CreateOrder   180 ms
    DB insert                30 ms
    PaymentService.Authorize 130 ms
      Provider API           110 ms
```

### Why Tracing Helps

Without tracing:

```
Order API is slow
```

With tracing:

```
Order API p99 is slow because PaymentService provider call is slow for provider=abc.
```

### Trace Propagation

Propagate trace context through:

- HTTP headers
- gRPC metadata
- Kafka message headers
- job payload metadata

Important headers include W3C `traceparent` and `tracestate` in modern systems.

### Sampling

Tracing every request may be expensive.

Sampling options:

- head-based: decide at start
- tail-based: decide after seeing outcome
- always sample errors
- sample high-latency requests

Tail-based sampling is useful because you can keep interesting traces.

### Span Attributes

Useful span tags:

- service name
- route
- HTTP status
- tenant tier
- database statement class, not raw query with PII
- dependency name
- retry count
- region/zone

Avoid high-cardinality tags like raw user ID unless your backend supports it and cost is understood.

---

## SLIs, SLOs, and Error Budgets

### SLI

Service Level Indicator: a measurement of reliability.

Examples:

- percentage of successful requests
- p99 latency under 300 ms
- percentage of jobs processed within 5 minutes

### SLO

Service Level Objective: target for an SLI.

Example:

```text
99.9% of checkout requests succeed over 30 days.
99% of checkout requests complete under 500 ms over 30 days.
```

### SLA

Service Level Agreement: contractual promise to customers, often with penalties.

### Error Budget

Allowed unreliability.

If SLO is 99.9%, error budget is 0.1%.

For 1,000,000 requests:

```
0.1% = 1,000 bad requests allowed
```

### Why Error Budgets Matter

They balance reliability and product velocity.

If error budget is healthy:

- ship normally

If error budget is burning fast:

- slow risky releases
- fix reliability

### Good SLOs Are User-Centered

Bad:

```text
CPU < 80%
```

Good:

```text
99.9% of users can create orders successfully.
```

CPU matters, but users care about successful actions.

---

## Alerting

Alerts should indicate user-impacting problems requiring action.

### Good Alerts

Good alert:

- actionable
- tied to user impact
- has severity
- includes runbook link
- avoids noise

Bad alert:

- fires for every small CPU spike
- no action required
- duplicates another alert
- wakes people for non-urgent issues

### Symptom-Based Alerts

Prefer symptom alerts:

- high error rate
- high p99 latency
- low checkout success
- queue lag too high

Cause alerts can be useful but should not be the only signal:

- CPU high
- disk near full
- pod restarts

### Burn Rate Alerts

Burn rate alerts detect how quickly error budget is being consumed.

Example:

```
Fast burn: page immediately if 1-hour error rate is very high.
Slow burn: ticket if 6-hour error rate is moderately high.
```

### Alert Severity

| Severity | Meaning |
| --- | --- |
| Page | immediate human action needed |
| Ticket | needs action but not urgent |
| Info | useful context, no action |

### Common Alerts

For web APIs:

- 5xx rate high
- p99 latency high
- availability SLO burn
- saturation high

For queues:

- queue lag too high
- oldest message age too high
- consumer error rate high

For databases:

- disk near full
- replication lag high
- connection count high
- slow query latency high

---

## Dashboards

Dashboards should answer questions quickly.

### Service Dashboard

Include:

- request rate by route
- error rate by route/status
- p50/p95/p99 latency
- saturation: CPU/memory/connections/thread pools
- dependency latency/errors
- deployment markers

### Dependency Dashboard

For each dependency:

- call rate
- error rate
- latency
- timeout count
- retry count
- circuit breaker state

### Business Dashboard

Examples:

- checkout attempts
- checkout success rate
- payment provider approval rate
- signup conversion
- orders per minute

### Dashboard Anti-Patterns

Avoid:

- giant dashboards nobody reads
- averages only
- no route/status breakdown
- no deployment markers
- no link from alerts to dashboards
- panels with unclear units

---

## Instrumentation Strategy

### What Every Service Should Emit

For each inbound request:

- request count
- error count
- duration histogram
- request ID
- trace ID
- route/method/status

For outbound dependencies:

- dependency name
- operation
- duration
- error/timeout count
- retry count

For background jobs:

- jobs received
- jobs succeeded/failed
- processing duration
- queue lag
- oldest message age

For databases:

- query duration by operation type
- connection pool usage
- transaction failures
- deadlocks/retries

### Label Design

Good labels:

```text
service=checkout
route=/orders/{id}
method=GET
status=200
region=us-east
```

Bad labels:

```text
user_id=u123456
url=/orders/ord_987654
request_id=req_abc
```

Bad labels create high cardinality and cost/performance problems.

Use templated route names, not raw paths.

### Logs vs Metrics Decision

Use metrics when:

- you need alerting
- you need trends
- values are numeric

Use logs when:

- you need detailed context
- events are rare or important
- humans inspect examples

Use traces when:

- request crosses services
- latency attribution matters

---

## OpenTelemetry Mental Model

OpenTelemetry is a vendor-neutral standard for collecting telemetry.

### Components

```
Application SDK
    |
    v
OpenTelemetry Collector
    |
    +--> metrics backend
    +--> tracing backend
    +--> logs backend
```

### Why Use a Collector?

The collector can:

- receive telemetry from apps
- batch and retry exports
- filter/scrub data
- sample traces
- route to multiple backends
- reduce vendor lock-in

### Instrumentation Types

Automatic instrumentation:

- quick setup
- captures common libraries

Manual instrumentation:

- custom business spans/metrics
- better semantic context

Use both.

---

## High Cardinality and Cost Control

Cardinality means number of unique label combinations.

### Example Explosion

Metric:

```text
http_requests_total{user_id, route, status}
```

If:

- 1M users
- 100 routes
- 10 statuses

Potential series:

```
1M * 100 * 10 = 1B time series
```

This can break or bankrupt observability systems.

### Control Techniques

- use route templates
- avoid user/request IDs in metric labels
- sample logs/traces
- set retention by signal importance
- aggregate at edge/collector
- drop noisy debug data
- use exemplars to connect metrics to traces

### Sensitive Data

Scrub:

- passwords
- tokens
- credit card numbers
- session cookies
- PII where not needed

Observability data often spreads widely. Treat it as sensitive.

---

## Debugging Playbooks

### High Latency

Check:

1. Is latency global or specific route/tenant/region?
2. Did traffic increase?
3. Did errors increase?
4. Which span is slow?
5. Are dependencies slow?
6. Is saturation high: CPU, DB connections, thread pool, queue?
7. Did a deployment/config change happen?

Signals:

- p95/p99 latency
- trace waterfall
- dependency metrics
- deployment markers

### High Error Rate

Check:

1. Which status codes?
2. Which route?
3. Which version/deployment?
4. Which dependency?
5. Are errors user-caused or server-caused?
6. Is there a spike in retries/timeouts?

### Queue Lag

Check:

1. Producer rate vs consumer rate
2. Consumer error rate
3. Oldest message age
4. Downstream dependency latency
5. Partition skew
6. Recent deployment

### Database Saturation

Check:

1. slow queries
2. connection pool saturation
3. lock waits/deadlocks
4. replication lag
5. cache hit ratio
6. disk I/O

---

## Observability in System Design

When presenting an architecture, include observability explicitly.

### Example: URL Shortener

Metrics:

- redirects/sec
- shorten requests/sec
- redirect p99 latency
- cache hit ratio
- DB lookup latency
- 404/expired link rate
- Kafka/click analytics lag

Logs:

- failed short code generation
- invalid URL attempts
- suspicious abuse

Traces:

- `GET /{code}` through cache and DB
- `POST /shorten` through ID generator and storage

Alerts:

- redirect availability below SLO
- p99 redirect latency high
- cache hit ratio drops sharply
- DB errors high

### Example: Payment System

Metrics:

- payment authorization success rate
- provider latency/error by provider
- idempotency conflict count
- ledger write failures
- retry queue depth

Logs:

- payment state transitions
- provider error codes
- idempotency key reuse

Traces:

- API -> PaymentService -> provider -> ledger

Alerts:

- payment success rate burn
- provider timeout spike
- ledger write failures

---

## Hands-On Exercises

### Exercise 1: Design Metrics for an API

For `POST /orders`, define:

- request counter
- error counter
- latency histogram
- labels

Avoid high-cardinality labels.

### Exercise 2: Write Structured Logs

Write a structured log for:

- user login failed
- payment authorization timed out
- background job retried

Include request ID and safe context.

### Exercise 3: Draw a Trace

Draw spans for:

```
Client -> API Gateway -> Order Service -> Payment Service -> Payment Provider
                         -> Inventory Service -> Database
```

Mark which spans are internal vs external.

### Exercise 4: Define SLOs

Define SLIs and SLOs for:

- URL shortener redirect
- checkout
- image upload
- Kafka consumer processing

### Exercise 5: Alert Review

Classify these alerts as page/ticket/remove:

- CPU > 70% for 5 minutes
- checkout 5xx rate > 5% for 10 minutes
- disk 85% full and growing
- one pod restarted once
- queue oldest message age > 15 minutes

Explain your reasoning.

### Exercise 6: Debug Scenario

Users report slow checkout.

Given:

- API p99 latency high
- DB latency normal
- payment provider p99 high
- retries increased
- error rate slightly high

Write the first 5 actions you would take.

---

## Interview Questions

### Basic Questions

**Q: What is observability?**

The ability to understand a system's internal state and behavior from outputs such as metrics, logs, traces, and events.

**Q: Logs vs metrics vs traces?**

Logs provide detailed events, metrics provide numeric time-series for trends/alerts, and traces show request flow across services.

**Q: What is a correlation ID?**

An ID propagated across services for a request, allowing logs/traces from different components to be connected.

**Q: Why are averages bad for latency?**

Averages hide tail latency. p95 and p99 show the experience of slower users and reveal outliers.

### SLO Questions

**Q: What is an SLI?**

A measured reliability indicator, such as request success rate or p99 latency.

**Q: What is an SLO?**

A target for an SLI, such as 99.9% of requests succeeding over 30 days.

**Q: What is an error budget?**

The allowed amount of unreliability implied by an SLO. If SLO is 99.9%, error budget is 0.1%.

**Q: Why alert on symptoms?**

Symptoms reflect user impact. CPU spikes alone may not matter; high checkout failure rate does.

### Design Questions

**Q: What observability would you add to a microservice?**

RED metrics for inbound requests, dependency metrics, structured logs with request/trace IDs, distributed tracing, business metrics, dashboards, and SLO-based alerts.

**Q: How do you debug high latency in distributed services?**

Use metrics to identify affected route/region, traces to locate slow spans, logs for detailed errors, and saturation metrics for CPU/DB/thread/queue bottlenecks.

**Q: How do you instrument Kafka consumers?**

Track consume rate, processing success/failure, processing latency, consumer lag, oldest message age, retry/DLT count, and trace context from message headers.

**Q: What is high cardinality and why is it dangerous?**

High cardinality means many unique label combinations. It can overload metrics systems and increase cost. Avoid labels like user ID, request ID, or raw URL.

### Advanced Questions

**Q: Head-based vs tail-based trace sampling?**

Head-based decides before request outcome is known. Tail-based decides after seeing latency/error, allowing retention of interesting traces.

**Q: How do you prevent observability from leaking secrets?**

Scrub tokens/passwords/PII, avoid logging full payloads, restrict access, encrypt telemetry pipelines, and define retention policies.

**Q: What should be on a service dashboard?**

Traffic, errors, latency percentiles, saturation, dependency health, deployment markers, and relevant business metrics.

---

## Quick Reference

### RED Metrics

| Signal | Meaning |
| --- | --- |
| Rate | requests/sec |
| Errors | failed requests/sec or percentage |
| Duration | latency distribution |

### USE Metrics

| Signal | Meaning |
| --- | --- |
| Utilization | how busy a resource is |
| Saturation | queued work/backpressure |
| Errors | resource errors |

### Good Metric Labels

```text
service
route template
method
status code
region
dependency
```

Avoid:

```text
user_id
request_id
raw URL
email
token
```

### Minimum Production Observability

1. Structured logs with request ID
2. Metrics for rate, errors, duration
3. Distributed traces for critical paths
4. Dashboards for service and dependencies
5. SLO-based alerts
6. Deployment markers
7. Runbooks for common alerts

### Interview Sound Bites

- "Metrics tell me something is wrong; traces show where; logs explain details."
- "I alert on user-impacting symptoms, not every internal fluctuation."
- "Every request should carry a trace ID/request ID."
- "Avoid high-cardinality labels like user ID in metrics."
- "A production design is incomplete without SLOs, dashboards, and failure playbooks."
