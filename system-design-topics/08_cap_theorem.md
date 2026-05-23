# CAP Theorem & Consistency Models

A comprehensive guide to distributed systems interview concepts: CAP theorem, consistency models, quorum reads/writes, consensus, replication trade-offs, clocks, partitions, and practical design choices.

---

## Table of Contents

1. [Why Distributed Systems Are Hard](#why-distributed-systems-are-hard)
2. [CAP Theorem](#cap-theorem)
3. [What CAP Does and Does Not Say](#what-cap-does-and-does-not-say)
4. [Consistency Models](#consistency-models)
5. [Availability and Partition Tolerance](#availability-and-partition-tolerance)
6. [Quorums](#quorums)
7. [Replication Strategies](#replication-strategies)
8. [Consensus](#consensus)
9. [Clocks, Ordering, and Time](#clocks-ordering-and-time)
10. [Conflict Resolution](#conflict-resolution)
11. [Practical System Design Choices](#practical-system-design-choices)
12. [Common Interview Scenarios](#common-interview-scenarios)
13. [Hands-On Thought Exercises](#hands-on-thought-exercises)
14. [Interview Questions](#interview-questions)
15. [Quick Reference](#quick-reference)

---

## Why Distributed Systems Are Hard

A distributed system is a system where components communicate over a network and can fail independently.

Problems that do not exist in a single process become central:

- messages can be delayed
- messages can be lost
- nodes can crash
- nodes can pause
- clocks can disagree
- networks can partition
- retries can create duplicates
- two clients can update the same data concurrently
- some replicas can be stale

### The Core Question

When parts of the system cannot communicate, should the system:

1. reject/stop some operations to preserve correctness?
2. keep accepting operations and reconcile later?

Most distributed system trade-offs come from this question.

---

## CAP Theorem

CAP says that under a network partition, a distributed data system must choose between consistency and availability.

The letters:

- **C: Consistency** - every read sees the latest write, or an error
- **A: Availability** - every request to a non-failing node gets a non-error response
- **P: Partition tolerance** - the system continues operating despite network partitions

### Network Partition

A partition means nodes cannot communicate reliably.

```
Client A -> Node 1   X   Node 2 <- Client B
             Region A X Region B
```

Node 1 and Node 2 are alive but cannot talk to each other.

### CAP Choice During Partition

If Client A writes `x=2` to Node 1 and Node 2 cannot hear about it:

```
Node 1: x = 2
Node 2: x = 1
```

Client B asks Node 2 for `x`.

Options:

1. Return `x=1` -> available but stale, so not strongly consistent.
2. Return error/block -> consistent but not available.

That is the CAP trade-off.

### CP System

Consistent + partition tolerant.

During partition, some requests may fail or block to avoid stale/incorrect results.

Examples by design tendency:

- ZooKeeper
- etcd
- many strongly consistent databases during quorum loss

Use CP when correctness is more important than serving every request.

### AP System

Available + partition tolerant.

During partition, nodes keep accepting reads/writes and resolve conflicts later.

Examples by design tendency:

- Dynamo-style systems
- Cassandra with low consistency levels
- Riak-style systems

Use AP when availability and low latency matter more than immediate consistency.

### CA System?

Consistency + availability without partition tolerance is possible only when partitions do not happen. In real distributed systems, partitions can happen, so practical systems must tolerate partitions somehow.

In interviews, avoid saying "CA distributed database" as a serious production category unless you are talking about a single-node system or a system that gives up under partitions.

---

## What CAP Does and Does Not Say

### CAP Is About Partitions

When the network is healthy, systems can often provide both consistency and availability.

The forced choice happens during a partition.

### CAP Is Not a Full System Classifier

Real systems are not simply "CP" or "AP" everywhere.

A system can choose:

- strong consistency for money movement
- eventual consistency for analytics
- local availability for cached profile reads
- quorum writes for inventory

CAP is a starting point, not a complete design.

### Availability Has a Strict Meaning

In CAP, availability means every request to a non-failing node gets a non-error response.

A CP system can still be highly available in normal operations. It just may reject operations during partition to preserve consistency.

### Consistency in CAP Means Linearizability

CAP consistency is strong consistency: reads behave as if there is one up-to-date copy.

This is not the same as database ACID consistency, which means transactions preserve invariants.

---

## Consistency Models

Consistency models define what reads can return after writes.

### Strong Consistency / Linearizability

After a write completes, all later reads see it.

```
write x=2 completes
read x -> 2
```

Use for:

- bank balances
- locks
- inventory reservation
- permissions/security decisions
- leader election

Cost:

- coordination
- higher latency
- lower availability during partitions

### Sequential Consistency

All operations appear in some order that is consistent with each client's program order, but not necessarily real-time order.

Less strict than linearizability.

### Causal Consistency

If operation B causally depends on operation A, everyone sees A before B.

Example:

```
Post: "I got the job"
Comment: "Congrats!"
```

Users should not see the comment before the post.

Use for social/collaboration systems where cause-effect ordering matters.

### Read-Your-Writes Consistency

After I write something, I see my own write.

Example:

```
User updates profile picture
User refreshes profile
User should see new picture
```

Can be implemented by routing the user's reads to primary or a caught-up replica.

### Monotonic Reads

Once a user sees version 5, they should not later see version 4.

Useful for feeds, profiles, and user-facing state.

### Monotonic Writes

A user's writes are applied in the order they were issued.

Important when update order matters.

### Eventual Consistency

If no new writes occur, replicas eventually converge.

Use for:

- search indexes
- feeds
- analytics
- recommendations
- counters where exactness is not critical

### Bounded Staleness

Reads may be stale, but only within a known bound.

Examples:

- at most 5 seconds old
- at most 100 versions behind

This is often a practical compromise.

### Consistency Model Selection

| Data | Recommended consistency |
| --- | --- |
| Account balance | strong |
| Payment status | strong for state transition, eventual for notifications |
| Product search | eventual |
| User profile after edit | read-your-writes |
| Social feed | eventual or causal |
| Inventory checkout | strong reservation |
| Analytics dashboard | bounded staleness/eventual |
| Feature flags | strong or bounded staleness depending risk |

---

## Availability and Partition Tolerance

### Availability in Practice

Real-world availability is often measured as uptime percentage:

| Availability | Downtime/year |
| --- | --- |
| 99% | about 3.65 days |
| 99.9% | about 8.76 hours |
| 99.99% | about 52.6 minutes |
| 99.999% | about 5.26 minutes |

CAP availability is stricter and theoretical. System design interviews usually use both ideas, so clarify what you mean.

### Partial Availability

A system may remain available for some operations but not others.

During a partition:

- allow browsing catalog
- reject checkout
- allow read-only profile view
- block permission changes

This is often the best practical design.

### Graceful Degradation

Instead of total outage:

- serve cached data
- disable non-critical writes
- queue work for later
- show stale-but-labeled analytics
- fall back to read-only mode

---

## Quorums

Quorum systems read/write multiple replicas and require acknowledgements.

Variables:

- `N`: number of replicas
- `W`: write acknowledgements required
- `R`: read acknowledgements required

If:

```
R + W > N
```

then read and write quorums overlap, so a read can see at least one replica with latest write, assuming conflict/version handling.

### Example

```
N = 3
W = 2
R = 2
R + W = 4 > 3
```

Write succeeds after 2 replicas ack. Read queries 2 replicas. At least one read replica overlaps with the write quorum.

### Trade-Offs

| Setting | Behavior |
| --- | --- |
| W=1, R=1 | fast, available, stale reads possible |
| W=2, R=2, N=3 | stronger, tolerates one failure |
| W=3, R=1, N=3 | slow writes, fast reads |
| W=1, R=3, N=3 | fast writes, slow reads |

### Quorum Is Not Magic

Quorums need:

- versioning
- conflict resolution
- read repair
- hinted handoff or anti-entropy
- careful clock assumptions

---

## Replication Strategies

### Leader-Follower

One leader accepts writes, followers replicate.

Pros:

- simple consistency model
- common in relational databases

Cons:

- leader bottleneck
- failover required
- replicas can lag

### Multi-Leader

Multiple leaders accept writes.

Pros:

- lower regional write latency
- better write availability

Cons:

- conflicts
- complex resolution

### Leaderless

Any replica can accept writes; clients use quorums.

Pros:

- high availability
- flexible read/write consistency

Cons:

- conflict handling
- read repair complexity

### Synchronous vs Asynchronous

Synchronous replication waits for replicas before acknowledging writes.

Asynchronous replication acknowledges first and replicates later.

Trade-off:

```
synchronous: stronger durability, higher latency, lower availability
asynchronous: lower latency, better availability, possible data loss/stale reads
```

---

## Consensus

Consensus lets nodes agree on a value/order even with failures.

Used for:

- leader election
- distributed locks
- metadata stores
- configuration
- strongly consistent replication

Common algorithms:

- Raft
- Paxos

Systems:

- etcd uses Raft
- ZooKeeper uses Zab
- many databases use consensus internally

### What Consensus Gives

Consensus gives a replicated log:

```
entry 1: set x=1
entry 2: set y=2
entry 3: delete z
```

All healthy nodes apply entries in the same order.

### Majority Requirement

Consensus systems usually need a majority to make progress.

For 3 nodes:

```
majority = 2
```

If only 1 node is reachable, it cannot safely accept writes.

This is CP behavior.

### Why Odd Node Counts?

3 nodes tolerate 1 failure.
5 nodes tolerate 2 failures.

4 nodes still tolerate only 1 failure for majority of 3, so the extra node often adds cost without improving failure tolerance.

---

## Clocks, Ordering, and Time

### Physical Clocks Are Imperfect

Machines have clocks, but:

- clocks drift
- NTP can adjust time
- leap seconds exist
- VM pauses happen

Do not rely on exact clock ordering for correctness unless the system is designed for it.

### Logical Clocks

Logical clocks order events without relying on wall-clock time.

Lamport clock:

- increments on local event
- sends clock with message
- receiver updates to max(local, received) + 1

It gives causal ordering but not full concurrency detection.

### Vector Clocks

Vector clocks can detect concurrent updates.

Used in some eventually consistent systems to identify conflicts.

### Last-Write-Wins

LWW chooses the write with latest timestamp.

Pros:

- simple

Cons:

- can lose updates
- depends on clock correctness

Use only when lost updates are acceptable or values are naturally replaceable.

---

## Conflict Resolution

Conflicts happen when concurrent writes update the same logical data.

### Strategies

### Last-Write-Wins

Pick latest timestamp/version.

Good for:

- cache values
- presence state
- some profile fields

Bad for:

- counters
- collaborative editing
- money movement

### Merge

Combine changes.

Example shopping cart:

```
Replica A: add item X
Replica B: add item Y
Merged: X and Y
```

### Application-Level Resolution

Expose conflict to business logic or users.

Example:

- document edit conflict
- calendar booking conflict

### CRDTs

Conflict-free replicated data types are data structures designed to merge automatically.

Examples:

- grow-only counters
- observed-remove sets
- last-writer registers

Useful for collaborative/offline systems, but more advanced than most interview designs require.

---

## Practical System Design Choices

### Payments

Need:

- strong consistency for payment state transitions
- idempotency keys
- durable ledger
- no double charge

Can be eventual:

- email receipt
- analytics
- dashboard aggregates

### Inventory

Checkout inventory needs strong reservation to prevent oversell.

Patterns:

- row-level lock on SKU
- atomic decrement with constraint
- reservation records with TTL
- single-writer per SKU/partition

Search/listing inventory can be eventually consistent.

### Social Feed

Usually eventual consistency is acceptable.

Need:

- read-your-writes for user's own post
- causal ordering for comments after posts
- fanout async

### Authentication and Authorization

Permissions should be strongly or bounded-staleness consistent.

Risky:

- user loses admin rights but stale cache still allows admin action for too long

Mitigate:

- short TTL
- versioned permission tokens
- central auth checks for sensitive operations

### Analytics

Eventual or bounded staleness is usually fine.

Be clear:

"Dashboard can be up to 5 minutes delayed."

---

## Common Interview Scenarios

### Scenario 1: Primary DB + Read Replicas

Question: user updates profile then sees old name.

Cause:

- read routed to stale replica

Fix:

- read primary after write
- session stickiness to primary for short period
- track replica LSN/version

### Scenario 2: Multi-Region Partition

Question: payment service in two regions cannot communicate. Should both accept writes?

Answer:

- For payments, prioritize consistency.
- One region should be primary for a payment/account or require quorum.
- Other region can serve reads or queue requests.

### Scenario 3: Shopping Cart Offline Mode

Question: user adds items offline on two devices.

Answer:

- Availability matters.
- Accept local writes.
- Merge carts later, maybe union item quantities.
- Resolve unavailable inventory at checkout.

### Scenario 4: Distributed Lock

Question: use Redis lock for critical section?

Answer:

- Maybe for efficiency, not sole correctness.
- Use TTL, owner token, and fencing token.
- Database constraints should protect critical invariant.

---

## Hands-On Thought Exercises

### Exercise 1: Classify Consistency

Choose consistency model for:

- bank transfer
- profile picture update
- search index update
- like counter
- password reset token
- notification count

Explain why.

### Exercise 2: Partition Decision

Two regions cannot communicate for 10 minutes. Design behavior for:

- product browsing
- checkout
- admin permission changes
- analytics dashboard

Which operations stay available?

### Exercise 3: Quorum Math

For `N=5`, choose `R` and `W` for:

- read-heavy workload
- write-heavy workload
- strongest practical consistency

Explain trade-offs.

### Exercise 4: Replication Lag

Design a read routing strategy for:

```
User writes order
User immediately opens order detail page
```

How do you guarantee the new order appears?

### Exercise 5: Conflict Resolution

Two devices edit a user's display name while offline:

```
Device A: "Asha R"
Device B: "Asha Rao"
```

Choose a conflict resolution strategy and explain the user experience.

---

## Interview Questions

### Basic Questions

**Q: What is CAP theorem?**

During a network partition, a distributed system must choose between strong consistency and availability. Partition tolerance is unavoidable in real distributed systems.

**Q: What does consistency mean in CAP?**

Linearizability: reads see the latest completed write, as if there is one copy of the data.

**Q: What is a CP system?**

A system that preserves consistency during partitions, even if some requests fail or block.

**Q: What is an AP system?**

A system that remains available during partitions, possibly serving stale data or accepting conflicting writes that must be reconciled later.

### Consistency Questions

**Q: Strong consistency vs eventual consistency?**

Strong consistency makes writes immediately visible to later reads. Eventual consistency allows stale reads temporarily but replicas converge if writes stop.

**Q: What is read-your-writes consistency?**

After a user writes data, that same user sees their update in subsequent reads.

**Q: What is monotonic reads consistency?**

Once a user sees a newer version, they do not later see an older version.

**Q: When is eventual consistency acceptable?**

Feeds, analytics, search indexes, recommendations, notifications, and approximate counters. Not for balances, permissions, or inventory reservation.

### Quorum and Consensus Questions

**Q: What is quorum consensus?**

A replication approach requiring a minimum number of replicas to acknowledge reads/writes. If `R + W > N`, read and write quorums overlap.

**Q: What does Raft/Paxos solve?**

Consensus: getting nodes to agree on an ordered log/value despite failures.

**Q: Why do consensus systems need a majority?**

Majority prevents split-brain. Two different majorities must overlap, so conflicting leaders cannot both safely commit.

**Q: Why are 3 or 5 nodes common?**

3 tolerates 1 failure. 5 tolerates 2 failures. Odd counts avoid wasting nodes that do not improve majority failure tolerance.

### Design Questions

**Q: How do you handle stale reads from replicas?**

Route strong reads to primary, use read-your-writes stickiness, track replica freshness/version, or accept bounded staleness where safe.

**Q: How do you design during regional partition?**

Classify operations by consistency need. Keep safe reads available, reject or queue consistency-critical writes, and reconcile eventual data after partition heals.

**Q: How do you resolve concurrent updates?**

Options include last-write-wins, merge, application-specific conflict resolution, or CRDTs. Choose based on data semantics.

---

## Quick Reference

### Consistency Cheat Sheet

| Model | Guarantee | Use |
| --- | --- | --- |
| Linearizable | latest write visible to all later reads | balances, locks, inventory |
| Read-your-writes | user sees own writes | profile updates |
| Monotonic reads | user does not go backward | feeds, profiles |
| Causal | cause appears before effect | comments, collaboration |
| Eventual | replicas converge eventually | analytics, search |
| Bounded staleness | stale within known bound | dashboards, config |

### CAP Cheat Sheet

| Choice during partition | Result |
| --- | --- |
| Return stale/local data | AP behavior |
| Reject/block unsafe operations | CP behavior |
| Serve cached read-only data | partial availability |
| Queue writes for later | available intake, eventual processing |

### Interview Sound Bites

- "CAP's forced choice happens during a partition."
- "CAP consistency means linearizability, not ACID consistency."
- "Most real systems choose consistency per operation, not globally."
- "Read replicas introduce lag, so read-after-write needs explicit handling."
- "For money, inventory, and permissions, I bias toward consistency; for feeds/search/analytics, eventual consistency is usually acceptable."
