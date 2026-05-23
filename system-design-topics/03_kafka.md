# Kafka

A comprehensive guide to Apache Kafka for system design interviews: event streaming, partitions, consumer groups, durability, ordering, delivery guarantees, and production trade-offs.

---

## Table of Contents

1. [What is Kafka?](#what-is-kafka)
2. [Core Concepts](#core-concepts)
3. [Kafka Architecture](#kafka-architecture)
4. [Topics, Partitions, and Ordering](#topics-partitions-and-ordering)
5. [Producers](#producers)
6. [Consumers and Consumer Groups](#consumers-and-consumer-groups)
7. [Durability and Replication](#durability-and-replication)
8. [Delivery Semantics](#delivery-semantics)
9. [Schema Design](#schema-design)
10. [Kafka vs Queues vs Pub/Sub](#kafka-vs-queues-vs-pubsub)
11. [Stream Processing Patterns](#stream-processing-patterns)
12. [Scaling and Capacity Planning](#scaling-and-capacity-planning)
13. [Operational Playbook](#operational-playbook)
14. [Hands-On Exercises](#hands-on-exercises)
15. [Interview Questions](#interview-questions)
16. [Quick Reference](#quick-reference)

---

## What is Kafka?

Kafka is a distributed event streaming platform. It stores ordered streams of records in topics, lets producers append events, and lets consumers read those events independently.

```
Producers ---> Kafka topic partitions ---> Consumers
```

Kafka is used for:

- Event-driven microservices
- Activity feeds and audit logs
- Data pipelines
- Change data capture
- Metrics and logs ingestion
- Stream processing
- Async job/event fan-out

### The Mental Model

Kafka is best understood as a durable distributed commit log.

```
Partition 0:
offset 0 | offset 1 | offset 2 | offset 3 | ...
event A  | event B  | event C  | event D  | ...
```

Events are appended. Consumers track their own read position, called an offset.

This is different from a traditional queue where a message is usually removed once one consumer processes it.

### Why Kafka Matters in System Design

Kafka decouples services.

Without Kafka:

```
Order Service -> Payment Service
              -> Inventory Service
              -> Email Service
              -> Analytics Service
```

With Kafka:

```
Order Service -> orders topic -> Payment Service
                           |--> Inventory Service
                           |--> Email Service
                           |--> Analytics Service
```

The producer does not need to know every downstream consumer.

---

## Core Concepts

### Event / Record / Message

A Kafka record contains:

- key
- value
- timestamp
- headers
- topic
- partition
- offset

Example order event:

```json
{
  "event_id": "evt_123",
  "event_type": "OrderCreated",
  "order_id": "ord_42",
  "user_id": "user_9",
  "amount_cents": 2599,
  "created_at": "2026-05-23T10:30:00Z"
}
```

### Topic

A topic is a named stream of related records.

Examples:

- `orders`
- `payments`
- `user-events`
- `inventory-events`
- `email-notifications`

### Partition

A topic is split into partitions. Each partition is an ordered append-only log.

```
orders topic

Partition 0: [0][1][2][3][4]
Partition 1: [0][1][2][3][4]
Partition 2: [0][1][2][3][4]
```

Partitions are Kafka's unit of parallelism and ordering.

### Offset

An offset is the position of a record within a partition.

```
orders-0 offset 12345
```

Offsets are meaningful only inside a specific partition.

### Broker

A broker is a Kafka server. A cluster has multiple brokers.

### Producer

Producers write records to topics.

### Consumer

Consumers read records from topics.

### Consumer Group

A consumer group is a set of consumers sharing work. Each partition is assigned to at most one consumer within the same group.

```
Topic: orders, 3 partitions

Consumer group: payment-service
Consumer A -> partition 0
Consumer B -> partition 1
Consumer C -> partition 2
```

Different consumer groups each get their own copy of the stream.

---

## Kafka Architecture

### High-Level Architecture

```
          +-------------+
          | Producers   |
          +------+------+
                 |
                 v
      +----------+-----------+
      | Kafka Cluster        |
      |                      |
      | +------+ +------+    |
      | |Broker| |Broker|    |
      | +------+ +------+    |
      | +------+ +------+    |
      | |Broker| |Broker|    |
      | +------+ +------+    |
      +----------+-----------+
                 |
                 v
          +-------------+
          | Consumers   |
          +-------------+
```

### Broker Responsibilities

Kafka brokers:

- Store partition logs on disk
- Handle produce and fetch requests
- Replicate partitions
- Serve consumers
- Coordinate partition leadership

### Controller

The controller manages cluster metadata:

- Which brokers are alive
- Which broker is leader for each partition
- Partition reassignment
- Leader elections

Modern Kafka uses a built-in metadata quorum mode called KRaft. Older Kafka deployments used ZooKeeper.

### Partition Leadership

Each partition has one leader and zero or more followers.

```
Partition orders-0

Broker 1: leader
Broker 2: follower
Broker 3: follower
```

Producers and consumers talk to the leader. Followers replicate from the leader.

---

## Topics, Partitions, and Ordering

### Ordering Guarantee

Kafka guarantees order only within a partition.

If all events for `order_id=42` go to the same partition, they are ordered:

```
OrderCreated -> PaymentAuthorized -> OrderShipped
```

If those events go to different partitions, global order is not guaranteed.

### Choosing a Message Key

The producer uses the key to choose a partition.

```
partition = hash(key) % number_of_partitions
```

Choose a key based on the ordering you need:

| Key | Ordering guarantee |
| --- | --- |
| `order_id` | Events for same order are ordered |
| `user_id` | Events for same user are ordered |
| `merchant_id` | Events for same merchant are ordered |
| null | Better spread, no entity ordering |

### Partition Count

More partitions give:

- More producer parallelism
- More consumer parallelism
- More storage distribution

But too many partitions cause:

- More open files
- More metadata
- Slower leader elections
- More overhead during rebalancing

Start with enough partitions for expected parallelism plus growth, but avoid creating thousands casually.

### Changing Partition Count

Increasing partitions can change key-to-partition mapping.

That can break ordering assumptions for future events if consumers assume all historical and future events for a key are on one partition.

Interview answer:

"I will choose a partition count with headroom. If I must increase it later, I will check whether key ordering assumptions are affected."

---

## Producers

### Producer Flow

```
Application
   |
   v
Producer client
   |
   | serialize, partition, batch, compress
   v
Broker partition leader
   |
   v
Append to log and acknowledge
```

### Producer Acknowledgements

`acks` controls when a producer considers a write successful.

| Setting | Meaning | Trade-off |
| --- | --- | --- |
| `acks=0` | Do not wait for broker ack | Fastest, can lose data |
| `acks=1` | Wait for leader only | Good latency, can lose data on leader failure |
| `acks=all` | Wait for in-sync replicas | Strongest durability, higher latency |

For important events, use `acks=all`.

### In-Sync Replicas

ISR means replicas that are caught up enough to be eligible for acknowledgement and leadership.

Important configs/concepts:

- replication factor
- min in-sync replicas
- producer `acks=all`

Example durable setup:

```
replication.factor = 3
min.insync.replicas = 2
producer acks = all
```

This means a write succeeds only when at least two replicas acknowledge it.

### Batching and Compression

Kafka is efficient because producers batch records.

Useful settings:

- `batch.size`
- `linger.ms`
- `compression.type`

Trade-off:

- Larger batches improve throughput
- Larger linger increases latency

Common compression choices:

- `lz4`: fast
- `zstd`: strong compression
- `snappy`: common older default

### Idempotent Producer

An idempotent producer prevents duplicates caused by retries for a single producer session.

Use it for reliable event publishing:

```properties
enable.idempotence=true
acks=all
retries=2147483647
```

### Producer Failure Scenarios

| Failure | Risk | Mitigation |
| --- | --- | --- |
| Broker unavailable | Send fails | Retry with backoff |
| Ack lost after write | Duplicate on retry | Idempotent producer |
| Bad schema event | Consumer failures | Schema registry and compatibility |
| Hot key | One partition overloaded | Better key strategy or key splitting |
| Large messages | Broker/client pressure | Store blob elsewhere, send reference |

---

## Consumers and Consumer Groups

### Consumer Group Assignment

Within a group, each partition is consumed by one consumer at a time.

```
Topic has 4 partitions
Group has 2 consumers

Consumer A: partitions 0, 1
Consumer B: partitions 2, 3
```

If the group has more consumers than partitions, extra consumers are idle.

```
4 partitions, 6 consumers -> 2 idle consumers
```

### Offsets

Consumers commit offsets to remember progress.

```
Committed offset = next record to read
```

If a consumer commits offset 101, it has processed records up to 100.

### Auto Commit vs Manual Commit

Auto commit is simple but risky. It may commit before processing completes.

Manual commit is better for important processing:

```
poll records
process records
commit offsets
```

Failure cases:

| When commit happens | Crash point | Result |
| --- | --- | --- |
| Before processing | After commit, before process | Message lost |
| After processing | After process, before commit | Message processed again |

Most Kafka applications choose at-least-once processing and make handlers idempotent.

### Rebalancing

A rebalance happens when:

- Consumer joins
- Consumer leaves
- Consumer stops heartbeating
- Topic partitions change

During rebalance, partition ownership changes. Poorly tuned consumers can rebalance too often and reduce throughput.

Mitigations:

- Keep processing time below poll interval
- Use static membership for stable deployments
- Use cooperative rebalancing where appropriate
- Scale partitions and consumers carefully

### Consumer Lag

Consumer lag is:

```
latest offset - committed offset
```

Lag means consumers are behind producers.

Possible causes:

- Consumer code is slow
- Downstream dependency is slow
- Not enough partitions/consumers
- Large messages
- Poison pill event repeatedly failing
- Rebalance loops

---

## Durability and Replication

### Replication Factor

Replication factor is the number of copies of a partition.

```
RF=3:
Broker 1: leader
Broker 2: follower
Broker 3: follower
```

RF=3 is common in production.

### Leader and Followers

Writes go to leader. Followers fetch from leader.

If leader fails, a follower from the ISR becomes leader.

### Unclean Leader Election

Unclean leader election allows an out-of-sync replica to become leader.

Pros:

- Better availability

Cons:

- Can lose committed data

For important data, disable unclean leader election.

### Retention

Kafka retains messages based on time or size, not whether consumers have read them.

```properties
retention.ms=604800000
retention.bytes=...
```

This allows replay:

```
Consumer resets offset -> reprocess old events
```

### Log Compaction

Compacted topics retain the latest value per key.

Example:

```
user42 -> name=Asha
user99 -> name=Ben
user42 -> name=Asha Rao
```

After compaction, Kafka may keep only the latest `user42` record.

Use compacted topics for:

- Latest user profile by ID
- Configuration snapshots
- CDC table changelogs

Do not use compaction when every historical event must be retained.

---

## Delivery Semantics

### At-Most-Once

Commit offset before processing.

```
commit -> process
```

If consumer crashes after commit but before processing, message is lost.

Use only when loss is acceptable.

### At-Least-Once

Process first, commit after success.

```
process -> commit
```

If consumer crashes after processing but before commit, message is processed again.

This is the most common model. Make processing idempotent.

### Exactly-Once

Kafka supports exactly-once semantics for Kafka-to-Kafka workflows using idempotent producers and transactions.

But end-to-end exactly-once with external databases, payment gateways, email systems, or third-party APIs is hard.

Practical interview answer:

"I would design for at-least-once delivery and idempotent consumers. For Kafka-to-Kafka stream processing I can use Kafka transactions, but for external side effects I need idempotency keys, unique constraints, and deduplication."

### Idempotent Consumer Patterns

Use one or more:

- Unique event ID stored in database
- Upsert by deterministic key
- Idempotency table with processed event IDs
- Natural idempotency, such as setting state to `SHIPPED`
- Outbox/inbox pattern

Example:

```sql
CREATE TABLE processed_events (
    event_id TEXT PRIMARY KEY,
    processed_at TIMESTAMP NOT NULL DEFAULT now()
);
```

Consumer transaction:

```sql
BEGIN;
INSERT INTO processed_events(event_id) VALUES (?) ON CONFLICT DO NOTHING;
-- if inserted, apply business change
COMMIT;
```

---

## Schema Design

### Why Schema Matters

Kafka decouples producers and consumers. That is powerful, but it means events become contracts.

Bad event contracts cause:

- Consumer crashes
- Data loss or bad analytics
- Hard migrations
- Reprocessing failures

### Good Event Design

Include:

- `event_id`
- `event_type`
- `event_version`
- entity ID
- timestamp
- producer/service name
- payload
- correlation/request ID when useful

Example:

```json
{
  "event_id": "evt_123",
  "event_type": "PaymentAuthorized",
  "event_version": 2,
  "payment_id": "pay_42",
  "order_id": "ord_99",
  "amount_cents": 5000,
  "currency": "USD",
  "occurred_at": "2026-05-23T10:30:00Z"
}
```

### Schema Evolution Rules

General safe changes:

- Add optional fields
- Add fields with defaults
- Stop using a field but keep accepting it

Dangerous changes:

- Rename fields
- Change type
- Remove required field
- Change semantic meaning

### Avro, Protobuf, JSON

| Format | Pros | Cons |
| --- | --- | --- |
| JSON | Human-readable, easy | Large, weak schema enforcement |
| Avro | Compact, schema evolution strong | Needs schema registry |
| Protobuf | Compact, good contracts | Less convenient for analytics |

For large production Kafka systems, use a schema registry and compatibility checks.

---

## Kafka vs Queues vs Pub/Sub

### Kafka vs RabbitMQ/SQS

| Aspect | Kafka | Traditional queue |
| --- | --- | --- |
| Storage model | Append log | Message queue |
| Message removal | Retention-based | Usually removed after ack |
| Replay | Natural | Limited or not native |
| Fan-out | Multiple consumer groups | Usually exchanges/subscriptions |
| Ordering | Per partition | Queue-dependent |
| Best for | Event streams, pipelines, replay | Work queues, task dispatch |

Use Kafka when you need durable event history, replay, and multiple independent consumers.

Use a queue when you need simple task distribution and per-message ack/dead-letter semantics.

### Kafka vs Redis Streams

| Aspect | Kafka | Redis Streams |
| --- | --- | --- |
| Scale | Very high distributed log | Good but memory-centered |
| Retention | Disk-based long retention | Memory/disk but not Kafka-scale |
| Ecosystem | Stream processing, connectors | Simpler Redis ecosystem |
| Operational complexity | Higher | Lower if Redis already exists |
| Best for | Event platform | Lightweight internal stream |

### Kafka vs Pub/Sub

Pub/Sub often means live broadcast. Kafka is a durable log. Consumers can be offline and catch up later as long as retention has not expired.

---

## Stream Processing Patterns

### Event Notification

Event says something happened. Consumer fetches details if needed.

```json
{ "event_type": "OrderCreated", "order_id": "ord_42" }
```

Pros:

- Smaller events
- Consumers fetch latest state

Cons:

- Extra database reads
- Reprocessing may see newer state, not original state

### Event-Carried State Transfer

Event includes enough data for consumers.

```json
{
  "event_type": "OrderCreated",
  "order_id": "ord_42",
  "user_id": "user_1",
  "items": [...],
  "amount_cents": 5999
}
```

Pros:

- Consumers are more independent
- Reprocessing uses historical event data

Cons:

- Larger events
- Schema evolution matters more

### Outbox Pattern

Problem: database update succeeds but Kafka publish fails.

Solution: write business row and event row in the same DB transaction.

```
BEGIN;
UPDATE orders SET status = 'CREATED' WHERE id = 'ord_42';
INSERT INTO outbox(event_id, topic, payload) VALUES (...);
COMMIT;

Outbox publisher reads outbox -> publishes to Kafka -> marks sent
```

This gives atomicity between database state and event intent.

### Dead Letter Topic

Poison messages should not block a partition forever.

Pattern:

```
consume event
try process
if permanent failure -> publish to orders.dlt with error metadata
commit original offset
```

Include:

- original topic, partition, offset
- error message
- consumer name
- timestamp
- event payload

### Retry Topics

Do not tight-loop a failing event.

Use retry topics:

```
orders -> orders.retry.1m -> orders.retry.10m -> orders.dlt
```

This gives backoff and protects downstream systems.

---

## Scaling and Capacity Planning

### Throughput Estimate

Example:

```
10,000 events/sec
avg event size = 1 KB
replication factor = 3
retention = 7 days
```

Ingress:

```
10,000 * 1 KB = 10 MB/sec
```

Storage per day before overhead:

```
10 MB/sec * 86,400 sec = 864 GB/day
```

With RF=3:

```
2.6 TB/day
```

For 7 days:

```
18.1 TB
```

Add overhead and headroom.

### Partition Estimate

If one partition safely handles 5 MB/sec produce throughput and you need 50 MB/sec:

```
50 / 5 = 10 partitions minimum
```

Then consider:

- consumer parallelism
- future growth
- ordering requirements
- broker count

### Consumer Scaling

Consumer group parallelism is limited by partition count.

```
max active consumers in one group <= number of partitions
```

If a topic has 12 partitions, one consumer group can have at most 12 active consumers.

### Large Message Strategy

Avoid sending huge payloads through Kafka.

Better:

```
Store blob in S3/object storage
Kafka event contains object URL/key and metadata
```

Large messages hurt:

- batching
- memory
- network
- replication
- consumer processing

---

## Operational Playbook

### Metrics to Monitor

Broker:

- under-replicated partitions
- offline partitions
- request latency
- disk usage
- network throughput
- active controller count
- ISR shrink/expand rate

Producer:

- record send rate
- error rate
- retry rate
- batch size
- request latency

Consumer:

- consumer lag
- processing latency
- commit latency
- rebalance count
- error/DLT rate

### Common Production Problems

| Symptom | Likely causes | Fixes |
| --- | --- | --- |
| Consumer lag rising | Slow consumer, bad downstream, too few partitions | Scale consumers, optimize processing, add partitions |
| Under-replicated partitions | Broker/disk/network issue | Fix broker, rebalance partitions |
| Rebalance storms | Consumers timing out | Tune poll/heartbeat, reduce processing time |
| Duplicate processing | Retries or commit after process | Idempotent consumer |
| Lost messages | Wrong ack/commit strategy | Use `acks=all`, manual commit after processing |
| Hot partition | Bad key distribution | Change key, split hot keys |

### Security Basics

- TLS for encryption in transit
- SASL or mTLS for authentication
- ACLs for topic permissions
- Separate internal and external listeners
- Do not allow broad wildcard produce/consume in production

### Topic Naming

Use names that reveal domain and event type:

```
orders.events
payments.events
inventory.adjustments
users.profile-changelog
orders.dlt
orders.retry.1m
```

Avoid names like:

```
events
data
test
service-topic
```

---

## Hands-On Exercises

### Exercise 1: Start Kafka Locally

Using Docker Compose, create a local Kafka broker. Many images exist; choose one your environment supports.

Core learning goal:

- Create a topic
- Produce records
- Consume records
- Observe offsets

Useful commands, depending on your Kafka image:

```bash
kafka-topics --bootstrap-server localhost:9092 --create --topic orders --partitions 3 --replication-factor 1
kafka-topics --bootstrap-server localhost:9092 --describe --topic orders
```

### Exercise 2: Produce and Consume

Producer:

```bash
kafka-console-producer --bootstrap-server localhost:9092 --topic orders --property parse.key=true --property key.separator=:
```

Enter:

```text
order-1:{"event_type":"OrderCreated","order_id":"order-1"}
order-2:{"event_type":"OrderCreated","order_id":"order-2"}
```

Consumer:

```bash
kafka-console-consumer --bootstrap-server localhost:9092 --topic orders --from-beginning --property print.key=true
```

### Exercise 3: Consumer Groups

Run two consumers in the same group:

```bash
kafka-console-consumer --bootstrap-server localhost:9092 --topic orders --group payment-service
```

Observe that partitions are split between consumers.

Then run a consumer in a different group:

```bash
kafka-console-consumer --bootstrap-server localhost:9092 --topic orders --group analytics-service --from-beginning
```

It receives the stream independently.

### Exercise 4: Consumer Lag

```bash
kafka-consumer-groups --bootstrap-server localhost:9092 --describe --group payment-service
```

Explain:

- current offset
- log end offset
- lag

### Exercise 5: Ordering by Key

Produce multiple events with the same key:

```text
order-1:created
order-1:paid
order-1:shipped
```

Explain why they go to the same partition and preserve order.

### Exercise 6: Outbox Design

Sketch tables:

```sql
CREATE TABLE orders (
    id TEXT PRIMARY KEY,
    status TEXT NOT NULL
);

CREATE TABLE outbox_events (
    id TEXT PRIMARY KEY,
    topic TEXT NOT NULL,
    payload JSONB NOT NULL,
    published_at TIMESTAMP
);
```

Explain how this prevents "DB commit succeeded but Kafka publish failed."

---

## Interview Questions

### Basic Questions

**Q: What is Kafka?**

Kafka is a distributed durable log for event streams. Producers append records to topic partitions, and consumers independently read records by offset.

**Q: What is a topic?**

A named stream of records, split into partitions for scale and parallelism.

**Q: What is a partition?**

An ordered append-only log. Kafka guarantees ordering within a partition, not across a whole topic.

**Q: What is an offset?**

The position of a record in a partition. Consumers commit offsets to track progress.

### Producer and Consumer Questions

**Q: How does Kafka decide which partition receives a message?**

Usually by hashing the message key. Same key maps to the same partition as long as partition count is unchanged.

**Q: How do consumer groups work?**

Consumers in the same group share partitions. Each partition is assigned to one consumer in that group. Different groups consume independently.

**Q: Why can adding more consumers fail to increase throughput?**

Because active consumers in a group are limited by partition count. If there are 10 consumers and 4 partitions, only 4 consumers actively read.

**Q: What is consumer lag?**

The difference between the latest offset and the consumer group's committed offset. It shows how far behind consumers are.

### Reliability Questions

**Q: How do you prevent message loss?**

Use replication factor 3, `acks=all`, sufficient `min.insync.replicas`, idempotent producer, manual offset commits after processing, and monitoring for under-replicated partitions.

**Q: How do you handle duplicate messages?**

Design consumers to be idempotent using event IDs, unique constraints, upserts, or processed-event tables.

**Q: Does Kafka provide exactly-once delivery?**

Kafka can provide exactly-once semantics for Kafka-to-Kafka processing with transactions, but external side effects still require idempotency and deduplication.

**Q: What happens if a consumer crashes after processing but before committing offset?**

The message will be read again after restart or rebalance. This creates duplicate processing, so the handler must be idempotent.

### Design Questions

**Q: Design an order event pipeline.**

Order service writes order state and an outbox event in the same DB transaction. Outbox publisher sends `OrderCreated` to Kafka. Payment, inventory, email, and analytics services consume in separate groups. Consumers are idempotent. Failures go to retry topics and DLT.

**Q: How do you choose partition key for order events?**

Use `order_id` if order lifecycle ordering matters. Use `user_id` if user-level ordering matters. Be careful with hot keys and partition skew.

**Q: Kafka or SQS/RabbitMQ for background jobs?**

For simple task distribution, use a queue. For durable event history, replay, and multiple independent consumers, use Kafka.

**Q: How do you replay events?**

Reset consumer group offsets or start a new consumer group, assuming topic retention still contains the events. Ensure consumers can safely reprocess.

### Advanced Questions

**Q: What is log compaction?**

A retention mode that keeps the latest record per key, useful for changelog/state topics.

**Q: What is a dead letter topic?**

A topic where permanently failing messages are sent with error metadata so the main consumer can continue processing.

**Q: What causes rebalancing problems?**

Slow processing, long pauses, bad heartbeat/poll settings, frequent deployments, or unstable consumers.

**Q: How do you estimate Kafka storage?**

Events/sec * average event size * seconds retained * replication factor, plus overhead and headroom.

---

## Quick Reference

### Topic Design Checklist

1. Topic name and owning team/service
2. Event schema and compatibility policy
3. Partition key and ordering guarantee
4. Partition count and expected throughput
5. Replication factor and min ISR
6. Retention or compaction policy
7. Consumer groups and replay needs
8. Retry and dead-letter strategy

### Reliability Defaults

```properties
replication.factor=3
min.insync.replicas=2
acks=all
enable.idempotence=true
unclean.leader.election.enable=false
```

### Common Commands

```bash
# list topics
kafka-topics --bootstrap-server localhost:9092 --list

# describe topic
kafka-topics --bootstrap-server localhost:9092 --describe --topic orders

# create topic
kafka-topics --bootstrap-server localhost:9092 --create --topic orders --partitions 6 --replication-factor 3

# consume
kafka-console-consumer --bootstrap-server localhost:9092 --topic orders --from-beginning

# consumer group lag
kafka-consumer-groups --bootstrap-server localhost:9092 --describe --group payment-service
```

### Interview Sound Bites

- "Kafka ordering is per partition, so the partition key is a design decision."
- "Kafka stores messages by retention, not by whether they were consumed."
- "At-least-once plus idempotent consumers is the practical default."
- "Consumer group parallelism is capped by partition count."
- "The outbox pattern solves database write plus event publish atomicity."
