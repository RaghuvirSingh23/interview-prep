# Secure Payment Request and Authorization Flow

**Difficulty:** Hard
**Skills:** System Design
**Tags:** Wallet App, System Design, Frontend

## Problem Statement

Design a secure and efficient frontend system for a digital wallet application that handles payment requests and authorizations. The system should provide a user-friendly interface that allows users to request payments from other users and authorize incoming payment requests.

## Requirements

### 1. User Interface

- **User-Friendly Design:** Clear instructions, intuitive navigation, responsive design
- **Payment Request Form:** Enter recipient's identifier (username, email, or phone number), amount, and optional note
- **Validation Feedback:** Real-time validation (recipient existence, sufficient funds, valid amount)
- **Notifications:** In-app and optional push notifications for payment request status (made, authorized, declined)
- **Payment Request List:** Display incoming payment requests to view, authorize, or decline

### 2. Secure Payment Request

- **Form Validation:** Validates recipient's identifier, sender's funds, and payment amount
- **Confirmation Screen:** Summarizes details before finalizing
- **Notification System:** Real-time notifications to recipients of incoming requests

### 3. Secure Payment Authorization

- **Request Overview:** View sender's identifier, amount, and note
- **Action Buttons:** Authorize or decline payment requests
- **Authorization Feedback:** Real-time status feedback (success or failure)
- **Balance Update:** Immediately updates sender's and recipient's balances upon authorization

### 4. Security

- **Data Encryption:** Encrypt all user-entered data
- **Secure Authentication:** OAuth or token-based authentication
- **Regular Security Checks:** Security audits and vulnerability scans

### 5. Performance

- **Efficient UI Rendering:** Optimized for concurrent users
- **Loading Indicators:** Skeleton screens for perceived performance during data fetching

### 6. Data Consistency

- **Real-Time Synchronization:** WebSocket-based sync with backend
- **Conflict Handling:** Mechanisms for concurrent update resolution

### 7. Fault Tolerance

- **Graceful Error Handling:** Meaningful error messages and retry options
- **Offline Mode:** Basic actions supported offline, syncing on reconnection

### 8. Caching

- **Client-Side Caching:** Local storage or IndexedDB for frequently accessed data
- **Cache Invalidation:** Proper strategies to keep data up-to-date

## Interviewer Guidelines

Designing the frontend for a digital wallet application requires careful consideration of UI design, security, real-time synchronization, performance optimization, and fault tolerance, along with a UML representation.

## UML Class Diagram

```
┌─────────────────────┐    ┌───────────────────────────────┐
│       User          │    │       ErrorHandling           │
├─────────────────────┤    ├───────────────────────────────┤
│ String id           │    │ handleError(msg: String): void│
│ String name         │    │ retryAction(): void           │
│ String email        │    └───────────────────────────────┘
│ String passwordHash │
└─────────────────────┘
         │
         ▼
┌──────────────────────────┐
│   PaymentRequestForm     │
├──────────────────────────┤
│ recipientIdentifier: Str │
│ amount: Double           │
│ note: String             │
│ validateForm(): boolean  │
└──────────────────────────┘
     │              │
     ▼              ▼
┌─────────────────────┐  ┌───────────────────────────────────────┐
│ NotificationService │  │       PaymentRequestList              │
├─────────────────────┤  ├───────────────────────────────────────┤
│ sendInApp(msg): void│  │ List<PaymentRequest>                  │
│ sendPush(msg): void │  │ handleAuthorization(id, action): void │
└─────────────────────┘  └───────────────────────────────────────┘
                                      │
                                      ▼
                         ┌──────────────────────────┐
                         │  AuthorizationRequest    │
                         ├──────────────────────────┤
                         │ senderId: String         │
                         │ amount: Double           │
                         │ note: String             │
                         │ authorize(): void        │
                         │ decline(): void          │
                         └──────────────────────────┘

┌──────────────────────────┐  ┌──────────────────────────────────┐
│    SecurityService       │  │    RealTimeSyncService           │
├──────────────────────────┤  ├──────────────────────────────────┤
│ encryptData(data): Str   │  │ initiateWebSocketConnection():   │
│ decryptData(data): Str   │  │   void                           │
└──────────────────────────┘  │ handleIncomingData(data: JSON):  │
                              │   void                           │
                              └──────────────────────────────────┘

┌──────────────────────────────────┐
│         CacheService             │
├──────────────────────────────────┤
│ storeData(key, data: JSON): void │
│ retrieveData(key): JSON          │
│ invalidateCache(key): void       │
└──────────────────────────────────┘
```

### Component Explanation

- **User:** Represents a user with basic information
- **PaymentRequestForm:** Manages the form for requesting payments, including validation
- **PaymentRequestList:** Displays and manages incoming payment requests
- **NotificationService:** Handles in-app and push notifications
- **AuthorizationRequest:** Represents the process of authorizing or declining payment requests
- **SecurityService:** Provides encryption and decryption services for sensitive data
- **RealTimeSyncService:** Manages WebSocket connections and real-time data synchronization
- **ErrorHandling:** Handles errors and retries actions for seamless user experience
- **CacheService:** Manages client-side caching for optimized performance

---

## Answer (Hello Interview Format)

### Functional Requirements

1. Users can **request payments** from other users by entering recipient identifier, amount, and note
2. Users can **view incoming payment requests** and **authorize or decline** them
3. System provides **real-time notifications** for payment events (requested, authorized, declined)
4. **Balance updates** immediately upon authorization
5. **Form validation** — real-time feedback on recipient existence, fund sufficiency, valid amounts

### Non-Functional Requirements

1. **Strong consistency** for balance updates — no double-spending
2. **Real-time updates** — payment requests and authorizations reflected within seconds via WebSockets
3. **Security** — all data encrypted in transit (TLS) and at rest; OAuth 2.0 authentication
4. **Offline resilience** — basic actions queued locally, synced on reconnection
5. **Low latency** — UI operations respond in <200ms

### Core Entities

- **User** — id, name, email, balance, password_hash
- **PaymentRequest** — id, requester_id, recipient_id, amount, note, status (pending/authorized/declined/expired), created_at, expires_at
- **Transaction** — id, payment_request_id, from_user_id, to_user_id, amount, status, completed_at
- **Notification** — id, user_id, type, message, read, created_at

### API Design

```
POST   /api/payment-requests
  Body: { recipient_identifier, amount, note }
  Returns: { request_id, status: "pending", created_at }

GET    /api/payment-requests/incoming?status=pending&page=1
  Returns: { requests: [...], total }

GET    /api/payment-requests/outgoing?page=1
  Returns: { requests: [...], total }

POST   /api/payment-requests/{id}/authorize
  Returns: { transaction_id, status: "completed", new_balance }

POST   /api/payment-requests/{id}/decline
  Returns: { status: "declined" }

GET    /api/balance
  Returns: { balance, currency }

GET    /api/notifications?unread=true
  Returns: { notifications: [...] }

WebSocket  /ws/updates
  Receives: { type: "payment_request" | "authorization" | "balance_update", data }
```

### High-Level Design

```
┌──────────────┐       ┌──────────────┐
│   Mobile /   │──────▶│  API Gateway │
│   Web Client │◀──ws──│  + Auth      │
└──────────────┘       └──────┬───────┘
                              │
                    ┌─────────┼──────────────┐
                    ▼         ▼              ▼
             ┌──────────┐ ┌──────────┐ ┌──────────────┐
             │ Payment  │ │  User    │ │ Notification │
             │ Request  │ │ Service  │ │  Service     │
             │ Service  │ └────┬─────┘ └──────┬───────┘
             └────┬─────┘      │              │
                  │       ┌────▼─────┐   ┌────▼─────────┐
                  │       │ User DB  │   │ WebSocket    │
                  │       └──────────┘   │ Server       │
                  ▼                      │ (Push to     │
           ┌──────────────┐              │  clients)    │
           │ Transaction  │              └──────────────┘
           │   Service    │
           └──────┬───────┘
                  │
        ┌─────────┼──────────┐
        ▼         ▼          ▼
  ┌──────────┐ ┌──────────┐ ┌──────────┐
  │ Txn DB   │ │  Redis   │ │  Event   │
  │(Primary) │ │ (Cache + │ │  Bus     │
  └──────────┘ │  Locks)  │ │ (Kafka)  │
               └──────────┘ └──────────┘
```

### Deep Dives

#### How does the authorization flow ensure consistency?

When a user authorizes a payment request:

1. Transaction Service acquires a **distributed lock** on both users' accounts (ordered by ID to prevent deadlock)
2. Verify sender has sufficient balance: `balance >= amount`
3. Execute in a single DB transaction:
  - Debit authorizer's balance
  - Credit requester's balance
  - Update PaymentRequest status to "authorized"
  - Create Transaction record
4. Release lock
5. Publish events to Kafka → Notification Service pushes via WebSocket

If the authorizer's balance is insufficient, return an error without modifying any state. The distributed lock (Redis SETNX with TTL) prevents race conditions where two concurrent authorizations could overdraw.

#### How do real-time notifications work?

- Each connected client maintains a **WebSocket connection** to the WebSocket Server
- When a payment event occurs (request created, authorized, declined), the Transaction Service publishes to Kafka
- Notification Service consumes the event:
  1. Persists notification to DB
  2. Pushes to the recipient's WebSocket channel
- If the user is offline, the notification is stored and delivered on next connection (pull on connect: `GET /api/notifications?unread=true`)
- **Push notifications** (mobile) sent via FCM/APNS as a fallback for offline users

#### How do we handle offline mode?

Client-side strategy:

1. **Service Worker** intercepts failed API calls when offline
2. Actions are queued in **IndexedDB** with timestamps
3. When connectivity resumes, the queue is flushed in order
4. Server validates each action (e.g., balance might have changed while offline — if authorization fails, notify user)
5. Conflicts resolved with **server-wins** strategy for financial operations — the server's state is authoritative

Read operations use cached data from IndexedDB (last known balance, recent requests) to keep the UI functional.

#### How do we prevent fraud and abuse?

- **Rate limiting** on payment request creation (e.g., max 20 requests/hour per user)
- **Amount limits** — configurable per-transaction and daily limits
- **Recipient verification** — confirm recipient exists and has a verified account before allowing request
- **Expiration** — payment requests expire after 7 days if not acted upon
- **Anomaly detection** — flag unusual patterns (many small requests to new users, requests at unusual hours)
- **Two-factor authentication** required for authorizations above a threshold amount

