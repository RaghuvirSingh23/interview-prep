# System Design: Integrating AI

## Problem Statement

A garment store called HackerStyle has found overnight success. Now people are flocking to its online e-commerce portal. The traffic has increased by 50X, and the company founders want the development team to capitalize on the traffic.

The company had an AI algorithm in the pipeline to recommend items to the shopper based on their browsing history. They want to deploy it as soon as possible to boost sales. The main problem is that the design of the site is monolithic in nature. The engineers are developing a strategy to integrate the stand-alone AI model with their monolithic architecture.

Redesign the whole system using the diagram tools provided in such a way that the existing functionality of the site remains as it is. Provide a solution with a design optimized to integrate the AI algorithm seamlessly.

## Components

- Cloud Infrastructure — cloud like AWS, OpenConnect which act as a content delivery network
- Backend — MS SQL, Mongo
- Client — any device which can browse the Internet

**Assumption:** The whole site is currently running on Springboot/NodeJS, and the AI algorithm is implemented in Python.

## Interviewer Guidelines / Key Discussion Points

- Conversion of the monolithic design into a **microservices architecture**
- After converting the monolithic into a microservices-based project, figuring out a way to integrate the standalone AI Recommendation engine into the project
- Striving to make the whole design highly **decoupled** so that the AI engine which is developed on a different Technical Stack altogether, can be integrated seamlessly

## Suggested Solution Architecture

```
Client → Server → SpringBoot Eureka Service (to manage the microservices)
                        │
                        ├── SpringBoot Microservices
                        │       ├── Profile Service ↔ Database
                        │       ├── Payment Service ↔ Cache
                        │       └── Cart Service
                        │
                        └── Communication using REST ──→ Python AI Service
                                                          for Product Recommendation
                                                              ↓
                                                          AI Training Models
```

### Key Architectural Decisions

1. **Monolith → Microservices**: Break the existing Springboot app into Profile, Payment, Cart services
2. **Service Discovery**: SpringBoot Eureka manages microservice registration and discovery
3. **Decoupled AI Integration**: Python AI service communicates via REST API — completely decoupled from Java/SpringBoot stack
4. **Database per Service**: Each microservice can own its data (Profile → DB, Payment → Cache)
5. **AI Training Models**: Separate from the recommendation serving layer, can be retrained independently

---

## Answer (Hello Interview Format)

### Functional Requirements

1. Users can **browse products** on the e-commerce site (existing)
2. Users can **add to cart and purchase** (existing)
3. Users see **personalized product recommendations** based on their browsing history (new — AI powered)
4. Recommendations update as the user browses more products

### Non-Functional Requirements

1. **Zero downtime migration** — existing site functionality must not break during the transition from monolith to microservices
2. **Low latency recommendations** — AI inference must return results in <100ms so the page doesn't feel slow
3. **Scalability** — handle 50X traffic increase
4. **Decoupled architecture** — the Python AI model must integrate cleanly with the existing Java/Node stack

### Core Entities

- **User** — id, email, name, browsing_history[], preferences
- **Product** — id, name, category, price, description, image_url
- **BrowsingEvent** — id, user_id, product_id, timestamp, session_id
- **Recommendation** — id, user_id, product_ids[], score[], generated_at

### API Design

```
GET    /api/products
  Returns: { products: [...], page, total }

GET    /api/products/{id}
  Returns: { product details }
  Side effect: publishes BrowsingEvent

GET    /api/recommendations?user_id=...
  Returns: { recommendations: [{ product_id, score }] }

POST   /api/cart
  Body: { product_id, quantity }

POST   /api/orders
  Body: { cart_id, payment_method }
```

### High-Level Design

```
┌──────────┐      ┌─────────────┐      ┌──────────────────────────────┐
│  Client   │─────▶│ API Gateway │─────▶│  Service Registry (Eureka)   │
│ (Browser) │      └─────────────┘      └──────────────┬───────────────┘
└──────────┘                                           │
                                          ┌────────────┼────────────┐
                                          ▼            ▼            ▼
                                    ┌──────────┐ ┌──────────┐ ┌──────────┐
                                    │ Product  │ │ Profile  │ │  Cart /  │
                                    │ Service  │ │ Service  │ │ Payment  │
                                    └────┬─────┘ └────┬─────┘ └────┬─────┘
                                         │            │            │
                                    ┌────▼─────┐ ┌───▼──────┐ ┌───▼──────┐
                                    │Product DB│ │ User DB  │ │ Order DB │
                                    └──────────┘ └──────────┘ └──────────┘
                                         │
                                    ┌────▼──────────┐
                                    │ Event Bus     │
                                    │ (Kafka/SQS)   │
                                    └────┬──────────┘
                                         │
                              ┌──────────▼───────────┐
                              │  AI Recommendation   │
                              │  Service (Python)    │
                              ├──────────────────────┤
                              │ - REST API           │
                              │ - Model inference    │
                              │ - Feature store      │
                              └──────────┬───────────┘
                                         │
                              ┌──────────▼───────────┐
                              │ Model Training       │
                              │ Pipeline (Offline)   │
                              │ - Retrain nightly    │
                              │ - A/B test models    │
                              └──────────────────────┘
```

### Deep Dives

#### How do we migrate from monolith to microservices without downtime?

Use the **Strangler Fig pattern**:
1. Place an API Gateway in front of the monolith
2. Extract one service at a time (start with Profile Service since it has fewest dependencies)
3. Route traffic for extracted endpoints to the new microservice; everything else still hits the monolith
4. Repeat for Cart, Payment, Product services
5. Once all endpoints are extracted, decommission the monolith

During migration, both the monolith and new services can coexist. The API Gateway handles routing. Feature flags control which backend serves each endpoint.

#### How do we integrate the Python AI service with the Java/Node stack?

The AI service is a **standalone Python microservice** exposing a REST API (or gRPC for lower latency). It's completely decoupled:

- **Product Service** publishes browsing events to Kafka when a user views a product
- **AI Service** consumes these events to build real-time user profiles / feature vectors
- When the frontend requests recommendations, the API Gateway routes to the AI Service
- AI Service runs inference against the loaded model, returns top-N product recommendations
- The AI Service registers itself with Eureka like any other microservice

This means the Python service can be deployed, scaled, and updated independently of the Java services.

#### How do we keep recommendation latency low under 50X traffic?

1. **Pre-compute recommendations** — for active users, run batch inference periodically and cache results in Redis. The real-time API first checks cache, only runs live inference on cache miss
2. **Model optimization** — use ONNX Runtime or TensorFlow Serving for optimized inference; quantize the model for faster CPU inference
3. **Horizontal scaling** — AI Service is stateless (model loaded in memory), so we scale pods horizontally behind the load balancer
4. **Feature store** — pre-computed user features stored in Redis, so inference doesn't need to recompute browsing history features on each request

#### How do we handle the training pipeline?

- **Offline training** runs nightly (or weekly) on historical browsing + purchase data
- Training happens on a separate compute cluster (e.g., SageMaker, Kubernetes batch jobs) — does not affect serving
- New models are **A/B tested**: a percentage of traffic gets the new model, we measure click-through rate and conversion
- Once validated, the new model is promoted to production via blue-green deployment of the AI Service
