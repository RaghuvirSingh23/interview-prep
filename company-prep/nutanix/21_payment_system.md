# System Design: Payment System

**Difficulty:** Easy
**Tags:** System Design, Microservices Architecture, Database Design

## Problem Statement

Design a payment system for businesses that supports real-time transactions. The system should maintain a detailed transaction history and include robust error-handling mechanisms to ensure reliable and accurate financial processing, even during transaction failures.

## Components

- Use a cloud platform like AWS, GCP, or Azure to host the application
- Use a microservices architecture
- The client should be able to access the system from any device with a browser

## Interviewer Guidelines / Key Discussion Points

1. **Describe the process for real-time transaction processing**
   - Hint: Discuss how the system validates transactions, ensuring data integrity and security using Web Sockets or Server-Sent Events.

2. **Describe how the system handles transaction failures**
   - Hint: Explain the error handling mechanisms in place and how the system retries transactions, performs rollbacks, and informs users of failures.

3. **Describe the strategies used to ensure system scalability and performance**
   - Hint: Discuss load balancing, caching, and database optimization techniques.

4. **Describe the security measures in place to protect user data and transactions**
   - Hint: Explain encryption, access controls, and compliance with industry standards.

## Whiteboard Solution

### Functional Requirements

- Process payments in real-time
- Manage transaction history
- Handle errors in transactions

### Non-Functional Requirements

- Handle high transaction volumes and user loads
- Ensure low latency and high performance
- Maintain high availability and reliability

### API Routes

#### POST /api/payments
Processes a payment transaction.
```json
{
  "amount": "number",
  "payment_method": "string",
  "to_user_id": "string",
  "from_user_id": "string"
}
```

#### GET /api/transactions?user_id=1
Retrieves the transaction history for a user.

#### POST /api/transactions/failure
Handles a failed transaction.
```json
{ "transaction_id": "string" }
```

### Database Schema

**Users Table:**
- id, username, email, password, created_at, updated_at

**Transactions Table:**
- id, from_user_id (Foreign), to_user_id (Foreign), amount, payment_method, timestamp, status

**Invoices Table:**
- id, transaction_id (Foreign), issued_at, due_date

### Estimations

**User Data:**
- Average size per user: 1 KB
- Users: 1,000,000
- Total user data storage: 1 GB

**Transactions:**
- Average size of a transaction: 1 KB
- Transactions per day: 100,000 transactions
- Daily Storage: 100 MB
- Monthly Storage: 3 GB

### Transaction Flow

```
Start
  ↓
User accesses the homepage
  │
  ├── Initiates a payment
  │     ↓
  │   User enters payment details
  │     ↓
  │   Validates and processes payment
  │     ↓
  │   System validates and processes the payment
  │     ├── Payment Successful          ├── Payment Failed
  │     ↓                               ↓
  │   Transaction is recorded      System handles the transaction failure
  │     ↓                               ↓
  │   Confirm payment              Initiates rollback
  │     ↓                               ↓
  │   User receives confirmation   Transaction is rolled back
  │     ↓                               ↓
  │     │                          Notifies user of failure
  │     │                               ↓
  │     │                          User is notified of failure
  │     ↓                               ↓
  │     End ←───────────────────────── End
  │
  └── Views transaction history
        ↓
      System retrieves and displays transaction records
        ↓
      Retrieves transaction records
        ↓
      System retrieves and displays transaction records
        ↓
       End
```

---

## Answer (Hello Interview Format)

### Functional Requirements

1. Users can **send payments** to other users in real-time
2. Users can **view transaction history** with filtering and pagination
3. System **handles failed transactions** with automatic rollback and user notification
4. Support **multiple payment methods** (card, bank transfer, wallet)

### Non-Functional Requirements

1. **Strong consistency** — money must never be lost or double-counted
2. **High availability** — 99.99% uptime for payment processing
3. **Low latency** — transactions should complete in <2 seconds
4. **Auditability** — every financial event must be logged and traceable
5. **Security** — PCI-DSS compliance, encrypted data at rest and in transit

### Core Entities

- **User** — id, username, email, balance, created_at
- **Transaction** — id, from_user_id, to_user_id, amount, payment_method, status (pending/completed/failed/rolled_back), idempotency_key, created_at
- **Invoice** — id, transaction_id, issued_at, due_date
- **Ledger** — id, transaction_id, account_id, debit/credit, amount, timestamp (double-entry bookkeeping)

### API Design

```
POST   /api/payments
  Body: { to_user_id, amount, payment_method, idempotency_key }
  Returns: { transaction_id, status, timestamp }

GET    /api/transactions?user_id=...&page=1&limit=20&status=...
  Returns: { transactions: [...], total, page }

GET    /api/transactions/{id}
  Returns: { transaction details }

POST   /api/transactions/{id}/retry
  Returns: { transaction_id, status }
```

### High-Level Design

```
┌──────────┐     ┌──────────────┐     ┌─────────────────┐
│  Client   │────▶│ API Gateway  │────▶│  Load Balancer  │
└──────────┘     └──────────────┘     └────────┬────────┘
                                               │
                              ┌────────────────┼──────────────┐
                              ▼                ▼              ▼
                        ┌──────────┐    ┌──────────┐   ┌──────────┐
                        │ Payment  │    │ History  │   │  User    │
                        │ Service  │    │ Service  │   │ Service  │
                        └────┬─────┘    └────┬─────┘   └────┬─────┘
                             │               │              │
                        ┌────▼─────┐    ┌───▼──────┐  ┌────▼─────┐
                        │ Payment  │    │  Read    │  │ User DB  │
                        │  Queue   │    │ Replica  │  └──────────┘
                        └────┬─────┘    └──────────┘
                             ▼
                   ┌───────────────────┐
                   │ Payment Processor │
                   │  (Idempotent)     │
                   └────────┬──────────┘
                            │
               ┌────────────┼────────────┐
               ▼            ▼            ▼
        ┌──────────┐  ┌──────────┐  ┌──────────┐
        │ Ledger   │  │ Txn DB   │  │ Notif.   │
        │ (Double  │  │(Primary) │  │ Service  │
        │  Entry)  │  └──────────┘  └──────────┘
        └──────────┘
```

### Deep Dives

#### How do we ensure money is never lost or double-counted?

**Double-entry bookkeeping** with database transactions:

Every payment creates two ledger entries atomically:
1. DEBIT sender's account by $X
2. CREDIT receiver's account by $X

```sql
BEGIN TRANSACTION;
  INSERT INTO ledger (txn_id, account_id, type, amount) VALUES (?, sender_id, 'DEBIT', ?);
  INSERT INTO ledger (txn_id, account_id, type, amount) VALUES (?, receiver_id, 'CREDIT', ?);
  UPDATE users SET balance = balance - ? WHERE id = sender_id AND balance >= ?;
  UPDATE users SET balance = balance + ? WHERE id = receiver_id;
  UPDATE transactions SET status = 'completed' WHERE id = ?;
COMMIT;
```

The `balance >= amount` check prevents overdraft. If any step fails, the entire transaction rolls back. The sum of all debits always equals the sum of all credits — invariant enforced at the database level.

#### How do we prevent duplicate payments?

**Idempotency keys**: Every payment request includes a client-generated `idempotency_key`. Before processing:
1. Check if a transaction with this key already exists
2. If yes → return the existing result (no re-processing)
3. If no → process and store with the key

The idempotency key is stored with a unique constraint in the transactions table, so concurrent duplicate requests are caught by the database.

#### How do we handle transaction failures and rollbacks?

State machine for transaction status:

```
PENDING → COMPLETED (success)
PENDING → FAILED (payment processor error)
FAILED  → PENDING (retry)
PENDING → ROLLED_BACK (timeout or irrecoverable error)
```

- **Timeouts**: If a transaction stays PENDING for >30 seconds, a background job marks it FAILED and triggers rollback
- **Rollback**: Reverse ledger entries (CREDIT sender, DEBIT receiver) in a new DB transaction
- **Notifications**: User notified via WebSocket (real-time) and email (async) on both success and failure
- **Dead letter queue**: Transactions that fail 3 retries go to a DLQ for manual investigation

#### How does the system scale for high transaction volumes?

- **Payment Queue** (Kafka): Decouples request acceptance from processing — absorbs traffic spikes
- **Partitioning**: Kafka topics partitioned by sender user_id, ensuring in-order processing per user
- **Database**: PostgreSQL with read replicas for the History Service; primary handles writes
- **Sharding** (at scale): Shard transactions table by user_id range — each shard handles a subset of users
- **Connection pooling**: PgBouncer in front of PostgreSQL to manage thousands of concurrent connections
- Estimations: 100K txn/day × 1KB = 100MB/day, 3GB/month — manageable on a single primary for years
