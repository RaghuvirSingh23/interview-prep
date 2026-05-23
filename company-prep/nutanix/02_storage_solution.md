# System Design: Storage Solution

## Problem Statement

External memory card slots are being removed from phones nowadays. An entrepreneur has decided to start a SAAS company to solve that problem.

The company will allow users to save data in the cloud using a subscription-based model. Knowing only the business side of things, the entrepreneur needs help converting the vision into a product.

Design a storage platform that will allow its users to upload files via a website. The site should give users the option to retrieve and download the files that they have stored.

## Components

- A cloud platform like AWS/GCP/Azure
- A microservices-based architecture
- Storage buckets
- A client can be anything that has a web browser

## Interviewer Guidelines / Key Discussion Points

- An event queue can be used to store file metadata in a database
- A separate service can be made that stores the list of a user's uploaded files to a database
- The actual files should be stored in an S3 bucket and can be fetched from the bucket at the user's request
- Bonus points if there is a duplicate S3 bucket running for backup as it makes the whole system resilient

## Suggested Solution Architecture

```
Client → Server → Load Balancer
                       ├── File Processing Service → Upload to Cloud → AWS S3 → AWS S3 Backup
                       ├── File Fetch Service ← Fetch Files from Cloud ← AWS S3
                       └── Profile Details Service
                               ↓
                           Event Queue
                               ↓
                       File Metadata Capture Service
                               ↓
                         Database + Cache

Database houses profile details linked with the files stored
```

### Components Breakdown

1. **Client** — web browser interface for upload/download
2. **Load Balancer** — distributes requests across services
3. **File Processing Service** — handles file uploads, pushes to cloud storage
4. **File Fetch Service** — retrieves files from S3 on user request
5. **Profile Details Service** — manages user profile and file ownership
6. **Event Queue** — async processing of file metadata after upload
7. **File Metadata Capture Service** — consumes events, writes metadata to DB
8. **AWS S3** — primary object storage for files
9. **AWS S3 Backup** — replica bucket for resilience
10. **Database** — stores user profiles and file metadata
11. **Cache** — speeds up frequent file listing/metadata queries

---

## Answer (Hello Interview Format)

### Functional Requirements

1. Users can **upload** files via a web interface
2. Users can **retrieve/download** files they have stored
3. Users can **list** all their uploaded files
4. Subscription-based access — users authenticate and manage their account

### Non-Functional Requirements

1. **Durability** — uploaded files must never be lost (99.999999999% durability)
2. **Availability** — high availability for both uploads and downloads
3. **Scalability** — support millions of users and petabytes of storage
4. **Low latency** — file listing and metadata queries should be fast; downloads should start quickly

### Core Entities

- **User** — id, email, subscription_tier, created_at
- **File** — id, user_id, filename, size_bytes, content_type, s3_key, checksum, created_at
- **Subscription** — id, user_id, tier, storage_limit, expires_at

### API Design

```
POST   /api/files/upload
  Headers: Authorization: Bearer <token>
  Body: multipart/form-data { file, metadata }
  Returns: { file_id, filename, size, created_at }

GET    /api/files
  Headers: Authorization: Bearer <token>
  Query: ?page=1&limit=20
  Returns: { files: [...], total, page }

GET    /api/files/{file_id}/download
  Headers: Authorization: Bearer <token>
  Returns: 302 redirect to pre-signed S3 URL

DELETE /api/files/{file_id}
  Headers: Authorization: Bearer <token>
  Returns: 204 No Content
```

### High-Level Design

```
┌──────────┐     ┌─────────────┐     ┌──────────────────┐
│  Client  │────▶│   API       │────▶│  Load Balancer   │
│ (Browser)│     │  Gateway    │     └────────┬─────────┘
└──────────┘     └─────────────┘              │
                                    ┌─────────┼──────────┐
                                    ▼         ▼          ▼
                              ┌──────────┐ ┌──────────┐ ┌──────────────┐
                              │  Upload  │ │  File    │ │   User /     │
                              │  Service │ │  Service │ │  Auth Service│
                              └────┬─────┘ └───┬─────-┘ └──────┬───────┘
                                   │           │               │
                              ┌────▼─────┐ ┌───▼────┐    ┌─────▼──────┐
                              │  AWS S3  │ │ Redis  │    │ PostgreSQL │
                              │ (Primary)│ │ Cache  │    │  (Users)   │
                              └────┬─────┘ └───┬────┘    └────────────┘
                              ┌────▼─────┐ ┌───▼──────-──┐
                              │  AWS S3  │ │ PostgreSQL  │
                              │ (Backup) │ │ (File Meta) │
                              └──────────┘ └─────────────┘
```

**Upload Flow:**

1. Client sends file to Upload Service via API Gateway + Load Balancer
2. Upload Service generates a unique S3 key, uploads to S3 (multipart for large files)
3. On success, publishes event to message queue
4. File Metadata Service consumes event, writes metadata (filename, size, s3_key, user_id) to PostgreSQL
5. Cache is invalidated for user's file list

**Download Flow:**

1. Client requests download → File Service looks up metadata in cache (or DB)
2. Generates a pre-signed S3 URL (time-limited, ~15 min expiry)
3. Returns 302 redirect — client downloads directly from S3

### Deep Dives

#### How do we handle large file uploads reliably?

Use **multipart uploads** to S3. Files are split into chunks (e.g., 5-10 MB), uploaded in parallel, and assembled server-side. If a chunk fails, only that chunk is retried. The Upload Service tracks chunk progress and provides a resumable upload endpoint so users don't have to restart from scratch on network failures.

#### How do we ensure durability?

- S3 provides 11 nines of durability natively via cross-AZ replication
- Additionally, maintain a **replica S3 bucket** in a different region via S3 Cross-Region Replication for disaster recovery
- File metadata is stored in PostgreSQL with synchronous replication to a standby
- Checksums (SHA-256) computed on upload and verified on download to detect bit rot

#### How do we scale for millions of users?

- **Upload/Download**: S3 handles the heavy lifting — effectively infinite storage and bandwidth
- **Metadata**: PostgreSQL with read replicas; shard by user_id if needed at extreme scale
- **Caching**: Redis cluster caches file listings (most users check their file list frequently, but it changes rarely)
- **Stateless services**: Upload Service and File Service are horizontally scalable behind the load balancer
- **CDN** (optional): For frequently downloaded public files, front S3 with CloudFront

#### How do we enforce subscription storage limits?

- Each user has a `storage_used` counter in the Users table, updated atomically on upload/delete
- Upload Service checks `storage_used + new_file_size <= storage_limit` before accepting the upload
- Use a database transaction to ensure the check-and-update is atomic, preventing race conditions with concurrent uploads

