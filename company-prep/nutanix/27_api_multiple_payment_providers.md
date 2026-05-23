# API Design for Multiple Payment Providers

**Difficulty:** Medium
**Skills:** System Design
**Tags:** API Design, API Development

## Problem Statement

You are designing a real-time payment gateway for an e-commerce platform that processes transactions with multiple payment providers. Currently, there is a unified API that deals with multiple payment providers. The platform experienced significant expansion, and the existing unified API is under strain.

- The platform is now processing an average of 10,000 transactions per second, with the transaction volume varying throughout the day. It peaks at 30,000 transactions per second during the holiday season.
- Latency is a crucial concern, as customers expect fast and responsive payment processing. The current unified API occasionally experiences delays during peak load, impacting the user experience.
- There are specific regulations that must be adhered to, especially regarding the handling of sensitive payment data and transaction security. Non-compliance could result in severe financial penalties and damage to the platform's reputation.

The goal is to improve the existing design in multiple ways, including performance, flexibility, cost, etc. Mention which factors you would improve in the existing system design and the trade-offs.

## Interviewer Guidelines

### 1. Dedicated APIs for Each Payment Provider

The existing system design can be improved if there is a dedicated API for each payment provider. In the updated architecture, there will be 3 APIs, each dealing with their dedicated payment provider. If the user wants to perform a transaction using payment provider 1, the app will call API1. Similarly, for payment providers 2 and 3, the app calls API 2 and API 3 respectively.

Specialized APIs can be optimized for performance based on provider-specific needs, potentially leading to better transaction speeds and efficiency. They reduce overall latency and are also cost-effective. A unified API may provide consistency but may not offer the same performance optimization opportunities.

**Trade-offs and Drawbacks:**

- **Development Overhead:** Developing and maintaining multiple APIs can introduce significant development and maintenance overhead. Each specialized API requires its own codebase, documentation, and infrastructure, leading to increased complexity and resource requirements.
- **Diverging Standards:** With dedicated APIs for each provider, there's potential for diverging standards. Each provider may have unique technical requirements, data formats, and operational procedures, leading to challenges in maintaining consistent standards.

**Alternative Approach — API Gateway and Microservices:**
Consider using an API gateway that routes requests to different microservices, with each microservice responsible for interacting with a particular payment provider. This architecture retains a level of abstraction while allowing optimization for each provider.

While the API gateway approach offers abstraction, it may not provide the same level of customization as dedicated APIs. Optimization for each provider may be limited, potentially impacting performance. Implementing an API gateway and multiple microservices can introduce added complexity. Routing requests through an API gateway may introduce a slight increase in latency compared to direct communication with dedicated APIs.

### 2. Efficient Data Caching

Implementing efficient data caching within each specialized API. This involves storing frequently accessed or critical data in high-speed in-memory databases or distributed caches, minimizing the need for redundant data retrieval from external servers.

**Trade-offs and Drawbacks:**

- **Stale Data:** Cached data may become outdated if not properly refreshed, leading to inconsistencies. Essential to establish cache expiration policies and mechanisms for data freshness.
- **Cache Invalidation Complexities:** Determining when and how to invalidate cached data can be complex. Handling cache invalidation for dynamic or frequently changing data is challenging, and a poorly managed strategy can lead to serving outdated or incorrect information.
- **Resource Utilization:** While caching can significantly enhance read performance, caches consume memory and processing resources. Striking the right balance between cache size and performance gains is crucial.

**Alternative Approach — Just-in-Time Data Retrieval and Processing Pipeline:**
Instead of relying on data caching, implement a just-in-time data retrieval and processing pipeline. The system fetches and computes data on demand, reducing latency without relying on cached data. Ensures data is always up-to-date, eliminating the risk of serving stale data. Reduces complexity by eliminating the need for caching, simplifying the architecture.

While just-in-time data retrieval can reduce latency compared to serving stale data from a cache, it may introduce a slight increase in latency compared to the instantaneous retrieval of cached data. The system needs to retrieve and compute the data in real time. It can be resource-intensive, especially during peak traffic, as it requires real-time processing and potentially more frequent access to underlying data sources.

### 3. Asynchronous Processing

The performance can be improved by utilizing asynchronous processing, performing certain tasks in the background rather than immediately during a user's transaction. For instance, when a user makes a payment, instead of waiting for tasks like sending a transaction confirmation email or processing post-payment data to complete before showing a confirmation message, these tasks are scheduled to run in the background. Users experience faster transaction completion. The main transaction processing remains responsive, ensuring a smooth user experience. Asynchronous operations can be handled separately, leading to more efficient use of resources and faster user interactions.

**Trade-offs and Drawbacks:**

- **Handling Failures:** If background tasks fail, it can be more complex to detect and recover from errors, potentially leading to data inconsistencies or incomplete transactions.
- **Complex Error Handling:** Managing errors in an asynchronous system requires a robust error detection and recovery mechanism. Ensuring failed tasks are retried or properly addressed can introduce complexity.
- **Resource Management:** Asynchronous tasks consume server resources, and managing these resources effectively is crucial. Poor resource management can lead to performance degradation.

**Alternative Approach — Message Queue (e.g., RabbitMQ):**
An alternative solution is to use a message queue, such as RabbitMQ, for handling tasks that can be deferred to the background. A message queue provides a robust way to manage background tasks, ensuring tasks are not lost and can be retried in case of failures. They are well-suited for scaling background task processing. Message queues often come with built-in monitoring and management tools.

However, implementing a message queue introduces additional complexity — configuring and maintaining the infrastructure, team training and adaptation. Message queues also consume resources for message queuing and processing.

### 4. Content Delivery Networks

Implementing CDNs can improve performance. When a user accesses the website, the CDN delivers static assets from servers located closer to the user's geographical location, reducing load time.

**Trade-offs and Drawbacks:**

- **Limited Impact on Dynamic Transactions:** CDNs primarily focus on optimizing delivery of static content and may not have a direct impact on optimizing payment transactions, which are inherently dynamic. More suitable for images, videos, stylesheets, and other static resources.

## Considerations Beyond Performance

While transitioning to multiple APIs, consider things beyond performance:

- **Fault Tolerance:** Ensure the payment system can handle errors, outages, and unexpected issues without disrupting the user experience.
- **Scalability:** The system should scale efficiently to accommodate increasing transaction volumes. Essential for handling peak loads during promotions or holidays.
- **Maintainability:** Multiple APIs can enhance performance but may introduce complexities around data consistency and system-wide changes. Strike a balance between performance optimization and maintainability.
- **Security:** Multiple APIs may introduce potential security vulnerabilities if not managed properly. Implementing stringent security measures is crucial to protect sensitive payment data.
- **Data Consistency:** In multiple APIs, there is a greater chance of data inconsistency, as data may be distributed across various specialized APIs and databases. Each API may have its own copy of data or store different subsets. Maintaining consistency across multiple data sources is challenging, especially achieving real-time synchronization.

## When to Introduce Multiple APIs

Only introduce multiple APIs where:

- There exists diversity among the payment providers (distinct features, pricing structures, or APIs)
- Having a single API is costly (all providers share same resources during peak loads)
- In the case of a single API, if the server goes down, all payment providers will be unable to fulfill requests
- Having a unified API makes the architecture complex, difficult to customize — all providers expected to adhere to common protocols, data formats, and transaction flows which may not align with individual provider requirements. Different payment providers may have varying security protocols and encryption standards.

---

## Answer (Hello Interview Format)

### Functional Requirements

1. Process **real-time payment transactions** via multiple payment providers (Stripe, PayPal, Adyen, etc.)
2. Users select a **payment provider** at checkout; system routes to the correct provider
3. **Transaction status tracking** — users see real-time status (pending, completed, failed)
4. **Unified transaction history** — regardless of which provider processed the payment

### Non-Functional Requirements

1. **High throughput** — 10K TPS average, 30K TPS peak (holiday season)
2. **Low latency** — payment processing in <2 seconds
3. **Regulatory compliance** — PCI-DSS for sensitive payment data
4. **Fault tolerance** — if one provider is down, others remain functional
5. **Extensibility** — adding a new payment provider should take days, not months

### Core Entities

- **PaymentProvider** — id, name, api_endpoint, auth_config, is_active, priority
- **Transaction** — id, user_id, provider_id, amount, currency, status, idempotency_key, created_at
- **ProviderConfig** — provider_id, rate_limit, timeout_ms, retry_policy, circuit_breaker_config

### API Design

```
POST   /api/payments
  Body: { amount, currency, provider: "stripe", payment_details, idempotency_key }
  Returns: { transaction_id, status, provider_transaction_id }

GET    /api/payments/{id}
  Returns: { transaction details, provider, status }

GET    /api/payments?user_id=...&page=1&provider=...
  Returns: { transactions: [...], total }

GET    /api/providers
  Returns: { providers: [{ id, name, is_active, supported_currencies }] }

POST   /api/payments/{id}/refund
  Body: { amount, reason }
  Returns: { refund_id, status }
```

### High-Level Design

```
┌──────────┐     ┌──────────────┐     ┌─────────────────┐
│  Client  │────▶│ API Gateway  │────▶│  Load Balancer  │
└──────────┘     └──────────────┘     └────────┬────────┘
                                               │
                                        ┌──────▼───────┐
                                        │  Payment     │
                                        │  Orchestrator│
                                        └──────┬───────┘
                                               │
                              ┌────────────────┼──────────────────┐
                              ▼                ▼                  ▼
                     ┌──────────────┐  ┌──────────────┐  ┌──────────────┐
                     │   Stripe     │  │   PayPal     │  │   Adyen      │
                     │   Adapter    │  │   Adapter    │  │   Adapter    │
                     └──────┬───────┘  └──────┬───────┘  └──────┬───────┘
                            │                 │                  │
                     ┌──────▼───────┐  ┌──────▼───────┐  ┌──────▼───────┐
                     │  Stripe API  │  │ PayPal API   │  │  Adyen API   │
                     └──────────────┘  └──────────────┘  └──────────────┘
                                               │
                              ┌────────────────┼──────────┐
                              ▼                ▼          ▼
                     ┌──────────────┐  ┌──────────┐  ┌──────────┐
                     │ Transaction  │  │  Redis   │  │  Kafka   │
                     │   DB         │  │ (Cache + │  │ (Events) │
                     └──────────────┘  │  Circuit │  └──────────┘
                                       │ Breakers)│
                                       └──────────┘
```

**Key pattern: Adapter / Strategy Pattern**

The Payment Orchestrator uses a **common PaymentProvider interface**. Each adapter (Stripe, PayPal, Adyen) implements this interface, translating the unified internal request to the provider-specific API format.

```
interface PaymentProvider {
  processPayment(amount, currency, details) → TransactionResult
  refund(transactionId, amount) → RefundResult
  getStatus(transactionId) → Status
}
```

Adding a new provider = implementing one adapter class. No changes to the orchestrator or API layer.

### Deep Dives

#### How does the routing/orchestration work?

The **Payment Orchestrator** is the core:

1. Receives payment request with specified provider
2. Looks up provider config (rate limits, timeouts, circuit breaker state) from Redis
3. If provider's circuit breaker is OPEN → return error suggesting alternative provider
4. Routes to the appropriate Adapter
5. Adapter translates request format, calls provider API, translates response back
6. Orchestrator records transaction in DB, publishes event to Kafka

If no provider is specified by the user, the orchestrator can use **smart routing**: pick the provider with lowest latency, lowest fees, or highest success rate based on real-time metrics.

#### How do we handle 30K TPS during peak?

1. **Horizontal scaling**: Orchestrator and Adapters are stateless — scale to N instances behind load balancer
2. **Async post-processing**: After the payment provider confirms, secondary tasks (email receipt, analytics, loyalty points) are published to Kafka and processed asynchronously
3. **Connection pooling**: Each adapter maintains a pool of HTTP connections to its provider API
4. **Provider-specific rate limiting**: Each adapter respects the provider's rate limits. If Stripe allows 10K TPS but PayPal only 5K TPS, the orchestrator can route overflow to the higher-capacity provider
5. **Caching**: Provider configs, fee structures, and exchange rates cached in Redis (refreshed every 60s)

#### How do circuit breakers protect the system?

Each provider adapter has an independent circuit breaker (implemented via Redis state):

- **CLOSED** (normal): Requests flow through. Track failure rate in a sliding window
- **OPEN** (provider down): After failure rate exceeds threshold (e.g., 50% failures in 30s), circuit opens. All requests immediately fail-fast with "provider unavailable"
- **HALF-OPEN** (probing): After a cooldown period (e.g., 30s), allow one test request. If it succeeds, close the circuit. If it fails, reopen

This prevents cascading failures — if Stripe is down, Stripe requests fail fast without consuming thread pool / connections, and the system keeps processing PayPal/Adyen payments normally.

#### How do we maintain a unified transaction history across providers?

- All transactions are recorded in **our** Transaction DB with a common schema, regardless of provider
- Each transaction stores both our `transaction_id` and the `provider_transaction_id` for cross-reference
- **Webhook handlers** for each provider listen for async status updates (e.g., Stripe webhook for charge.succeeded) and update our DB
- The History API queries our DB — never the provider APIs — so the view is always consistent and fast
- For reconciliation, a nightly batch job compares our records against each provider's settlement reports

