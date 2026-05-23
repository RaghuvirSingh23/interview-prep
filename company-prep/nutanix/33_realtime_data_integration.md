# System Design: Enhancing Real-Time Data Integration in E-Commerce

**Difficulty:** Hard
**Tags:** API Integration, Data Processing

## Problem Statement

An e-commerce company handles approximately 10,000 API requests per second and aims to enhance its customer experience by integrating various data sources and services using APIs. The data includes customer behavior, inventory levels, supplier data, and shipping information. The goal is to process this data in real-time to provide personalized recommendations, real-time inventory updates, and efficient order processing.

The company's current infrastructure is a mix of cloud-based and on-premise systems. The challenge is to integrate these diverse data sources efficiently and process the data in real-time to derive meaningful insights, while ensuring data security and compliance.

## Challenge

Design improvements in the API integration and data processing layers to enable real-time data processing and integration for enhanced decision-making and user experience, while considering potential security risks.

## Focus Areas

1. **Real-time Data Processing** — Implement a solution to process data from various sources in real time
2. **Scalability and Performance** — Ensure the architecture can scale to handle large volumes of data and maintain high performance
3. **Data Accuracy and Consistency** — Maintain data accuracy and consistency across various systems
4. **Analytics and Insight Generation** — Facilitate efficient analysis and insight generation for better decision-making
5. **Security and Compliance** — Ensure that customer data processed in real-time remains secure

## Task

- Propose architectural changes to the existing setup to tackle the highlighted challenges
- Extend the provided diagram to include your enhancements
- Annotate and briefly explain your design decisions on the whiteboard
- Discuss potential challenges, fallback strategies, and solutions in your proposed architecture

## Interviewer Guidelines — Step-by-Step Solution

### Step 1: Introduction of Serverless Architecture with Message Queue

To accommodate the volume of API requests, integrate a serverless architecture using **Lambda functions** (or equivalent) triggered upon data ingestion. A **Message Queuing System** ensures data is processed in real-time and with reliable order, even with potential latency.

```
Inventory System ─┐
Supplier System  ─┤
Shipping System  ─┼──→ Lambda Functions → Message Queue → API Service
User Behavior    ─┘         → Data Processing Engine → Centralized Database → User Interface
```

### Step 2: Implementing Cloud-based Stream Processing

To effectively handle real-time data, adopt **AWS Kinesis or Azure Stream Analytics**. These managed services ease the complexity of maintaining a stream processing unit.

```
[Data Sources] → Lambda Functions → Message Queue → Cloud Stream Processing
    → API Service → Data Processing Engine → Centralized Database → User Interface
```

### Step 3: Incorporating a Cloud-based Analytics Engine

Introducing a cloud-based Analytics Engine ensures real-time insights. While pre-built machine learning models from cloud providers can speed up implementation, a hybrid approach — combining custom algorithms tailored to business needs with pre-built models — would be ideal.

```
[Data Sources] → Lambda Functions → Message Queue → Cloud Stream Processing
    → API Service → Data Processing Engine → Centralized Database
    → Cloud Analytics Engine → User Interface
```

---

## Answer (Hello Interview Format)

### Functional Requirements

1. **Ingest data** from multiple sources in real-time: customer behavior, inventory, supplier data, shipping
2. Provide **personalized product recommendations** based on real-time customer behavior
3. Show **real-time inventory updates** (e.g., "only 3 left in stock")
4. Enable **efficient order processing** with real-time data from all sources
5. Generate **analytics and insights** for business decision-making

### Non-Functional Requirements

1. **High throughput** — handle 10,000 API requests/second
2. **Low latency** — data processed and available within seconds of ingestion (near real-time)
3. **Data consistency** — accurate inventory counts across cloud and on-premise systems
4. **Scalability** — handle 10X traffic spikes during sales events
5. **Security & compliance** — customer data encrypted, access controlled, audit-logged

### Core Entities

- **CustomerEvent** — user_id, event_type (view/click/purchase), product_id, timestamp, session_id
- **InventoryRecord** — product_id, warehouse_id, quantity, last_updated
- **SupplierOrder** — id, supplier_id, product_id, quantity, status, eta
- **ShipmentEvent** — order_id, status, location, timestamp

### API Design

```
GET    /api/recommendations?user_id=...
  Returns: { products: [{ id, name, score, reason }] }

GET    /api/inventory/{product_id}
  Returns: { product_id, available_quantity, warehouses: [...] }

GET    /api/orders/{id}/tracking
  Returns: { status, events: [...], eta }

POST   /api/events  (internal — data ingestion)
  Body: { source, event_type, payload, timestamp }

GET    /api/analytics/dashboard?metric=...&range=...
  Returns: { data_points: [...], aggregations }
```

### High-Level Design

```
┌────────────────────────────────────────┐
│          Data Sources                   │
├──────────┬──────────┬────────┬─────────┤
│Inventory │ Supplier │Shipping│  User   │
│ System   │  System  │ System │Behavior │
└────┬─────┴────┬─────┴───┬────┴────┬────┘
     │          │         │         │
     └──────────┼─────────┼─────────┘
                ▼         ▼
     ┌──────────────────────────────┐
     │   Ingestion Layer            │
     │   (Lambda / Serverless)      │
     └──────────────┬───────────────┘
                    ▼
     ┌──────────────────────────────┐
     │   Message Queue (Kafka)      │
     │   Partitioned by source type │
     └──────────────┬───────────────┘
                    │
          ┌─────────┼──────────┐
          ▼         ▼          ▼
   ┌───────────┐ ┌──────────┐ ┌──────────────┐
   │  Stream   │ │ Inventory│ │Recommendation│
   │ Processor │ │ Updater  │ │  Engine      │
   │ (Kinesis/ │ │          │ │              │
   │  Flink)   │ └────┬─────┘ └──────┬───────┘
   └─────┬─────┘      │              │
         │       ┌────▼─────┐  ┌─────▼───────┐
         │       │ Inventory│  │   User      │
         │       │   DB     │  │  Profile    │
         │       │ (Redis)  │  │  Store      │
         │       └──────────┘  └─────────────┘
         ▼
   ┌──────────────┐    ┌──────────────────┐
   │ Analytics    │    │  Centralized DB  │
   │ Engine       │    │  (Data Warehouse)│
   │ (Redshift/   │    └────────┬─────────┘
   │  BigQuery)   │             │
   └──────┬───────┘    ┌───────▼──────────┐
          │            │  API Service      │
          └───────────▶│  (REST Layer)     │
                       └───────┬──────────┘
                               ▼
                       ┌──────────────┐
                       │ User Interface│
                       └──────────────┘
```

### Deep Dives

#### How does real-time inventory tracking work?

The critical path for "only 3 left in stock":

1. **Inventory System** publishes stock changes to Kafka (via CDC or direct integration)
2. **Inventory Updater** consumes events, updates Redis with atomic `DECRBY` / `INCRBY`
3. API reads inventory from Redis — sub-millisecond response
4. For accuracy, Redis is reconciled against the source-of-truth Inventory DB every 5 minutes

Why Redis for inventory?
- Atomic operations prevent race conditions (100 users buying the last item simultaneously)
- `DECRBY stock:{product_id} 1` returns the new value — if <0, the purchase is rejected
- Sub-millisecond reads for "X items left" display

#### How does the recommendation engine work in real-time?

Two-tier approach:
1. **Batch layer**: Nightly model training on full historical data → generates base recommendations per user segment
2. **Real-time layer**: As the user browses, events flow through Kafka → Recommendation Engine updates the user's feature vector in real-time → re-ranks recommendations

The real-time re-ranking uses a lightweight model (e.g., logistic regression on recent features) that runs in <10ms. The batch model provides the candidate set; the real-time model re-orders it based on the current session.

#### How do we handle the hybrid cloud/on-premise challenge?

- **Data ingestion**: On-premise systems push events to Kafka via a secure VPN or direct connect link. Lambda functions normalize the data format.
- **Consistency**: Each data source has an **event sequence number**. The stream processor detects gaps and requests re-sends. Exactly-once delivery via Kafka consumer group offsets.
- **Latency**: On-premise → cloud network latency (~10-50ms) is acceptable for near-real-time. For truly latency-sensitive operations (inventory decrement), maintain a Redis replica on-premise with cross-datacenter sync.

#### How do we ensure security for real-time customer data?

- **Encryption in transit**: All data flows over TLS 1.3 (Kafka SSL, HTTPS APIs, VPN for on-prem)
- **Encryption at rest**: All databases and data warehouse encrypted (AES-256)
- **Access control**: RBAC on all data services; analytics users get access to aggregated/anonymized data only
- **PII handling**: Customer events are stripped of PII before entering the analytics pipeline; original data retained only in the secure operational DB
- **Audit logging**: All data access logged with user identity, timestamp, and query details
