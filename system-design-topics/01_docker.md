# Docker

A comprehensive guide to Docker - containerization fundamentals, commands, and hands-on exercises.

---

## Table of Contents

1. [What is Docker?](#what-is-docker)
2. [Why Docker?](#why-docker)
3. [Core Concepts](#core-concepts)
4. [Docker Architecture](#docker-architecture)
5. [Essential Commands](#essential-commands)
6. [Dockerfile Deep Dive](#dockerfile-deep-dive)
7. [Docker Compose](#docker-compose)
8. [Networking](#networking)
9. [Volumes & Data Persistence](#volumes--data-persistence)
10. [Best Practices](#best-practices)
11. [Hands-On Exercises](#hands-on-exercises)
12. [Interview Questions](#interview-questions)

---

## What is Docker?

Docker is a **containerization platform** that packages applications and their dependencies into lightweight, portable containers.

### Container vs Virtual Machine

```
┌─────────────────────────────────────────────────────────────────┐
│                    VIRTUAL MACHINES                             │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────┐  ┌─────────┐  ┌─────────┐                          │
│  │  App A  │  │  App B  │  │  App C  │                          │
│  ├─────────┤  ├─────────┤  ├─────────┤                          │
│  │Guest OS │  │Guest OS │  │Guest OS │  ← Each VM has full OS   │
│  └─────────┘  └─────────┘  └─────────┘                          │
│  ┌─────────────────────────────────────┐                        │
│  │           Hypervisor                │                        │
│  └─────────────────────────────────────┘                        │
│  ┌─────────────────────────────────────┐                        │
│  │           Host OS                   │                        │
│  └─────────────────────────────────────┘                        │
│  ┌─────────────────────────────────────┐                        │
│  │           Hardware                  │                        │
│  └─────────────────────────────────────┘                        │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                      CONTAINERS                                 │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────┐  ┌─────────┐  ┌─────────┐                          │
│  │  App A  │  │  App B  │  │  App C  │                          │
│  ├─────────┤  ├─────────┤  ├─────────┤                          │
│  │  Bins/  │  │  Bins/  │  │  Bins/  │  ← Only app + deps       │
│  │  Libs   │  │  Libs   │  │  Libs   │                          │
│  └─────────┘  └─────────┘  └─────────┘                          │
│  ┌─────────────────────────────────────┐                        │
│  │         Docker Engine               │                        │
│  └─────────────────────────────────────┘                        │
│  ┌─────────────────────────────────────┐                        │
│  │           Host OS                   │  ← Shared kernel       │
│  └─────────────────────────────────────┘                        │
│  ┌─────────────────────────────────────┐                        │
│  │           Hardware                  │                        │
│  └─────────────────────────────────────┘                        │
└─────────────────────────────────────────────────────────────────┘
```


| Aspect          | Virtual Machine           | Container             |
| --------------- | ------------------------- | --------------------- |
| **Size**        | GBs (full OS)             | MBs (app + deps only) |
| **Startup**     | Minutes                   | Seconds               |
| **Isolation**   | Complete (hardware level) | Process level         |
| **Performance** | ~5-10% overhead           | Near native           |
| **Portability** | Limited                   | Excellent             |


---

## Why Docker?

### The "Works on My Machine" Problem

```
Developer's Machine          Production Server
┌──────────────────┐        ┌──────────────────┐
│ Python 3.9       │        │ Python 3.7       │  ← Version mismatch
│ Ubuntu 20.04     │        │ CentOS 7         │  ← Different OS
│ OpenSSL 1.1.1    │        │ OpenSSL 1.0.2    │  ← Dependency mismatch
│ Node 16          │        │ Node 14          │  ← Runtime mismatch
└──────────────────┘        └──────────────────┘
         ↓                           ↓
      Works ✓                    Crashes ✗
```

### Docker Solution

```
┌──────────────────────────────────────────────────────────────┐
│                    Docker Image                              │
│  ┌────────────────────────────────────────────────────────┐  │
│  │ Python 3.9 + OpenSSL 1.1.1 + Node 16 + Your App        │  │
│  │ (Everything bundled together)                          │  │
│  └────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
                    ↓                    ↓
            Dev Machine            Production
              Works ✓               Works ✓
```

### Key Benefits

1. **Consistency** - Same environment everywhere
2. **Isolation** - Apps don't interfere with each other
3. **Portability** - Run anywhere Docker runs
4. **Efficiency** - Lightweight, fast startup
5. **Scalability** - Easy to replicate containers
6. **Version Control** - Images are versioned

---

## Core Concepts

### Image

A **read-only template** containing instructions for creating a container. Think of it as a "class" in OOP.

```
Image = Base OS + Dependencies + Application Code + Configuration
```

Images are built in **layers**:

```
┌─────────────────────────────┐
│  Layer 4: COPY app code     │  ← Your code
├─────────────────────────────┤
│  Layer 3: RUN npm install   │  ← Dependencies
├─────────────────────────────┤
│  Layer 2: RUN apt-get       │  ← System packages
├─────────────────────────────┤
│  Layer 1: Ubuntu 20.04      │  ← Base image
└─────────────────────────────┘
```

### Container

A **running instance** of an image. Think of it as an "object" created from a class.

```
Image (class)  →  Container (object)
nginx:latest  →  my-nginx-container
              →  another-nginx-container
              →  yet-another-nginx-container
```

### Registry

A **storage and distribution system** for Docker images.

- **Docker Hub** - Public registry (default)
- **Amazon ECR** - AWS private registry
- **Google GCR** - GCP private registry
- **Self-hosted** - Harbor, GitLab Registry

### Tag

A **label** for image versions.

```
nginx:latest      ← Latest version
nginx:1.21        ← Specific version
nginx:1.21-alpine ← Specific version + variant
myapp:v1.0.0      ← Semantic versioning
myapp:abc123      ← Git commit hash
```

---

## Docker Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         Docker Client                           │
│                    (docker build, run, pull)                    │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ REST API
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                        Docker Daemon                            │
│                         (dockerd)                               │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐              │
│  │   Images    │  │ Containers  │  │  Networks   │              │
│  └─────────────┘  └─────────────┘  └─────────────┘              │
│  ┌─────────────┐  ┌─────────────┐                               │
│  │   Volumes   │  │   Plugins   │                               │
│  └─────────────┘  └─────────────┘                               │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                        Container Runtime                        │
│                    (containerd + runc)                          │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                         Linux Kernel                            │
│              (namespaces, cgroups, union filesystem)            │
└─────────────────────────────────────────────────────────────────┘
```

### Key Linux Features Docker Uses

1. **Namespaces** - Isolation (PID, Network, Mount, User, etc.)
2. **cgroups** - Resource limits (CPU, Memory, I/O)
3. **Union Filesystem** - Layered file system (OverlayFS)

---

## Essential Commands

### Image Commands

```bash
# Pull an image from registry
docker pull nginx:latest

# List local images
docker images
docker image ls

# Build image from Dockerfile
docker build -t myapp:v1 .
docker build -t myapp:v1 -f Dockerfile.prod .

# Remove image
docker rmi nginx:latest
docker image rm nginx:latest

# Remove all unused images
docker image prune -a

# Inspect image details
docker image inspect nginx:latest

# View image history (layers)
docker history nginx:latest

# Tag an image
docker tag myapp:v1 myregistry.com/myapp:v1

# Push to registry
docker push myregistry.com/myapp:v1
```

### Container Commands

```bash
# Run a container
docker run nginx                    # Foreground
docker run -d nginx                 # Detached (background)
docker run -d --name my-nginx nginx # With custom name
docker run -it ubuntu bash          # Interactive with TTY

# Common run options
docker run -d \
  --name my-app \
  -p 8080:80 \              # Port mapping (host:container)
  -v /host/path:/container/path \  # Volume mount
  -e MY_VAR=value \         # Environment variable
  --restart unless-stopped \ # Restart policy
  --memory 512m \           # Memory limit
  --cpus 0.5 \              # CPU limit
  nginx:latest

# List containers
docker ps                   # Running only
docker ps -a                # All (including stopped)

# Stop/Start/Restart
docker stop my-nginx
docker start my-nginx
docker restart my-nginx

# Remove container
docker rm my-nginx          # Must be stopped
docker rm -f my-nginx       # Force remove running

# Remove all stopped containers
docker container prune

# Execute command in running container
docker exec my-nginx ls /etc/nginx
docker exec -it my-nginx bash   # Interactive shell

# View logs
docker logs my-nginx
docker logs -f my-nginx         # Follow (tail -f)
docker logs --tail 100 my-nginx # Last 100 lines

# Copy files
docker cp myfile.txt my-nginx:/path/
docker cp my-nginx:/path/file.txt ./

# Inspect container
docker inspect my-nginx

# View resource usage
docker stats
docker stats my-nginx
```

### System Commands

```bash
# System info
docker info
docker version

# Disk usage
docker system df

# Clean up everything unused
docker system prune -a --volumes

# View events
docker events
```

---

## Dockerfile Deep Dive

### Basic Structure

```dockerfile
# Base image
FROM python:3.9-slim

# Metadata
LABEL maintainer="you@example.com"
LABEL version="1.0"

# Set working directory
WORKDIR /app

# Set environment variables
ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1

# Install system dependencies
RUN apt-get update && apt-get install -y \
    gcc \
    libpq-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy dependency file first (for layer caching)
COPY requirements.txt .

# Install Python dependencies
RUN pip install --no-cache-dir -r requirements.txt

# Copy application code
COPY . .

# Expose port (documentation)
EXPOSE 8000

# Default command
CMD ["python", "app.py"]
```

### Key Instructions


| Instruction   | Purpose                        | Example                    |
| ------------- | ------------------------------ | -------------------------- |
| `FROM`        | Base image                     | `FROM ubuntu:20.04`        |
| `WORKDIR`     | Set working directory          | `WORKDIR /app`             |
| `COPY`        | Copy files from host           | `COPY . /app`              |
| `ADD`         | Copy + extract archives + URLs | `ADD app.tar.gz /app`      |
| `RUN`         | Execute command (build time)   | `RUN apt-get update`       |
| `CMD`         | Default command (run time)     | `CMD ["python", "app.py"]` |
| `ENTRYPOINT`  | Fixed command (run time)       | `ENTRYPOINT ["python"]`    |
| `ENV`         | Set environment variable       | `ENV NODE_ENV=production`  |
| `ARG`         | Build-time variable            | `ARG VERSION=1.0`          |
| `EXPOSE`      | Document port                  | `EXPOSE 8080`              |
| `VOLUME`      | Create mount point             | `VOLUME /data`             |
| `USER`        | Set user                       | `USER appuser`             |
| `HEALTHCHECK` | Container health check         | See below                  |


### CMD vs ENTRYPOINT

```dockerfile
# CMD - can be overridden
CMD ["python", "app.py"]
# docker run myapp              → python app.py
# docker run myapp bash         → bash (overrides CMD)

# ENTRYPOINT - fixed command
ENTRYPOINT ["python"]
CMD ["app.py"]
# docker run myapp              → python app.py
# docker run myapp script.py    → python script.py (appends to ENTRYPOINT)
```

### Multi-Stage Builds

Reduce final image size by using multiple stages:

```dockerfile
# Stage 1: Build
FROM node:16 AS builder
WORKDIR /app
COPY package*.json ./
RUN npm ci
COPY . .
RUN npm run build

# Stage 2: Production
FROM nginx:alpine
COPY --from=builder /app/dist /usr/share/nginx/html
EXPOSE 80
CMD ["nginx", "-g", "daemon off;"]
```

**Result**: Final image only contains nginx + built files, not Node.js or source code.

### Health Checks

```dockerfile
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
  CMD curl -f http://localhost:8080/health || exit 1
```

### .dockerignore

Exclude files from build context:

```
# .dockerignore
node_modules
npm-debug.log
.git
.gitignore
.env
*.md
Dockerfile
.dockerignore
```

---

## Docker Compose

Manage multi-container applications with a single YAML file.

### Basic Structure

```yaml
# docker-compose.yml
version: '3.8'

services:
  web:
    build: .
    ports:
      - "8080:80"
    environment:
      - NODE_ENV=production
    depends_on:
      - db
      - redis
    volumes:
      - ./app:/app
    networks:
      - app-network

  db:
    image: postgres:13
    environment:
      POSTGRES_DB: myapp
      POSTGRES_USER: user
      POSTGRES_PASSWORD: password
    volumes:
      - postgres-data:/var/lib/postgresql/data
    networks:
      - app-network

  redis:
    image: redis:alpine
    networks:
      - app-network

volumes:
  postgres-data:

networks:
  app-network:
    driver: bridge
```

### Compose Commands

```bash
# Start services
docker-compose up              # Foreground
docker-compose up -d           # Detached
docker-compose up --build      # Rebuild images

# Stop services
docker-compose down            # Stop and remove containers
docker-compose down -v         # Also remove volumes

# View status
docker-compose ps
docker-compose logs
docker-compose logs -f web     # Follow specific service

# Execute command
docker-compose exec web bash

# Scale services
docker-compose up -d --scale web=3

# Restart specific service
docker-compose restart web
```

### Environment Variables

```yaml
services:
  web:
    environment:
      # Direct value
      - NODE_ENV=production
      # From host environment
      - API_KEY
      # From .env file (automatic)
      - DATABASE_URL=${DATABASE_URL}
    env_file:
      - .env
      - .env.local
```

### Depends On with Health Checks

```yaml
services:
  web:
    depends_on:
      db:
        condition: service_healthy
  
  db:
    image: postgres:13
    healthcheck:
      test: ["CMD-SHELL", "pg_isready -U postgres"]
      interval: 5s
      timeout: 5s
      retries: 5
```

---

## Networking

### Network Types

```bash
# List networks
docker network ls

# Default networks:
# - bridge    (default for containers)
# - host      (share host network)
# - none      (no networking)
```

### Bridge Network (Default)

```
┌─────────────────────────────────────────────────────────────┐
│                        Host Machine                         │
│  ┌────────────────────────────────────────────────────────┐ │
│  │                   docker0 (bridge)                     │ │
│  │                    172.17.0.1                          │ │
│  │  ┌──────────┐    ┌──────────┐    ┌──────────┐          │ │
│  │  │Container1│    │Container2│    │Container3│          │ │
│  │  │172.17.0.2│    │172.17.0.3│    │172.17.0.4│          │ │
│  │  └──────────┘    └──────────┘    └──────────┘          │ │
│  └────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### Custom Bridge Network

```bash
# Create network
docker network create my-network

# Run containers on network
docker run -d --name web --network my-network nginx
docker run -d --name api --network my-network myapi

# Containers can reach each other by name
# From 'web': curl http://api:8080
```

### Port Mapping

```bash
# Map host port to container port
docker run -p 8080:80 nginx      # localhost:8080 → container:80
docker run -p 127.0.0.1:8080:80  # Only localhost
docker run -P nginx              # Random host port
```

### Host Network

```bash
# Container shares host's network stack
docker run --network host nginx
# nginx available on host's port 80 directly
```

---

## Volumes & Data Persistence

### The Problem

Containers are **ephemeral** - data is lost when container is removed.

### Volume Types

```
1. Named Volumes     - Docker-managed, persistent
2. Bind Mounts       - Host directory mounted
3. tmpfs Mounts      - In-memory only
```

### Named Volumes

```bash
# Create volume
docker volume create my-data

# Use volume
docker run -v my-data:/app/data nginx

# List volumes
docker volume ls

# Inspect volume
docker volume inspect my-data

# Remove volume
docker volume rm my-data
```

### Bind Mounts

```bash
# Mount host directory
docker run -v /host/path:/container/path nginx
docker run -v $(pwd):/app nginx

# Read-only mount
docker run -v $(pwd):/app:ro nginx
```

### Volume in Compose

```yaml
services:
  db:
    image: postgres
    volumes:
      # Named volume
      - postgres-data:/var/lib/postgresql/data
      # Bind mount
      - ./init.sql:/docker-entrypoint-initdb.d/init.sql:ro

volumes:
  postgres-data:
```

### When to Use What


| Type             | Use Case                                     |
| ---------------- | -------------------------------------------- |
| **Named Volume** | Database storage, persistent data            |
| **Bind Mount**   | Development (live code reload), config files |
| **tmpfs**        | Sensitive data, temporary cache              |


---

## Best Practices

### 1. Use Specific Tags

```dockerfile
# Bad
FROM python:latest

# Good
FROM python:3.9.7-slim-buster
```

### 2. Minimize Layers

```dockerfile
# Bad - 3 layers
RUN apt-get update
RUN apt-get install -y curl
RUN apt-get install -y vim

# Good - 1 layer
RUN apt-get update && apt-get install -y \
    curl \
    vim \
    && rm -rf /var/lib/apt/lists/*
```

### 3. Order Instructions for Caching

```dockerfile
# Dependencies change less often than code
COPY requirements.txt .
RUN pip install -r requirements.txt

# Code changes frequently - put last
COPY . .
```

### 4. Use .dockerignore

```
node_modules
.git
*.md
.env
```

### 5. Don't Run as Root

```dockerfile
RUN useradd -m appuser
USER appuser
```

### 6. Use Multi-Stage Builds

```dockerfile
FROM node:16 AS builder
# ... build steps

FROM node:16-alpine
COPY --from=builder /app/dist ./dist
```

### 7. One Process Per Container

```
# Bad: web server + database in one container
# Good: separate containers for each
```

### 8. Use Health Checks

```dockerfile
HEALTHCHECK CMD curl -f http://localhost/ || exit 1
```

### 9. Log to stdout/stderr

```dockerfile
# Application should log to stdout, not files
# Docker captures stdout/stderr automatically
```

### 10. Set Resource Limits

```bash
docker run --memory 512m --cpus 0.5 myapp
```

---

## Hands-On Exercises

### Exercise 1: Basic Container Operations

```bash
# 1. Pull and run nginx
docker pull nginx:alpine
docker run -d --name my-nginx -p 8080:80 nginx:alpine

# 2. Verify it's running
curl http://localhost:8080

# 3. Check logs
docker logs my-nginx

# 4. Execute command inside
docker exec my-nginx cat /etc/nginx/nginx.conf

# 5. Stop and remove
docker stop my-nginx
docker rm my-nginx
```

### Exercise 2: Build a Simple Web App

Create these files:

**app.py**

```python
from flask import Flask
import os

app = Flask(__name__)

@app.route('/')
def hello():
    name = os.environ.get('NAME', 'World')
    return f'Hello, {name}!'

@app.route('/health')
def health():
    return 'OK'

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
```

**requirements.txt**

```
flask==2.0.1
```

**Dockerfile**

```dockerfile
FROM python:3.9-slim

WORKDIR /app

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY app.py .

EXPOSE 5000

HEALTHCHECK --interval=30s --timeout=10s \
  CMD curl -f http://localhost:5000/health || exit 1

CMD ["python", "app.py"]
```

```bash
# Build and run
docker build -t hello-flask .
docker run -d -p 5000:5000 -e NAME=Docker hello-flask

# Test
curl http://localhost:5000
# Output: Hello, Docker!
```

### Exercise 3: Multi-Container App with Compose

Create a web app with Redis counter:

**app.py**

```python
from flask import Flask
import redis
import os

app = Flask(__name__)
cache = redis.Redis(host=os.environ.get('REDIS_HOST', 'redis'), port=6379)

@app.route('/')
def hello():
    count = cache.incr('hits')
    return f'Hello! This page has been viewed {count} times.\n'

@app.route('/health')
def health():
    try:
        cache.ping()
        return 'OK'
    except:
        return 'Redis connection failed', 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
```

**requirements.txt**

```
flask==2.0.1
redis==4.0.0
```

**Dockerfile**

```dockerfile
FROM python:3.9-slim

WORKDIR /app

RUN apt-get update && apt-get install -y curl && rm -rf /var/lib/apt/lists/*

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY app.py .

EXPOSE 5000

HEALTHCHECK --interval=10s --timeout=5s \
  CMD curl -f http://localhost:5000/health || exit 1

CMD ["python", "app.py"]
```

**docker-compose.yml**

```yaml
version: '3.8'

services:
  web:
    build: .
    ports:
      - "5000:5000"
    environment:
      - REDIS_HOST=redis
    depends_on:
      redis:
        condition: service_started

  redis:
    image: redis:alpine
    volumes:
      - redis-data:/data

volumes:
  redis-data:
```

```bash
# Run
docker-compose up -d

# Test (refresh multiple times)
curl http://localhost:5000

# View logs
docker-compose logs -f

# Stop
docker-compose down
```

### Exercise 4: Multi-Stage Build

Create a Go application with minimal image:

**main.go**

```go
package main

import (
    "fmt"
    "net/http"
)

func main() {
    http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
        fmt.Fprintf(w, "Hello from Go!")
    })
    http.ListenAndServe(":8080", nil)
}
```

**Dockerfile**

```dockerfile
# Build stage
FROM golang:1.17 AS builder
WORKDIR /app
COPY main.go .
RUN CGO_ENABLED=0 GOOS=linux go build -o server main.go

# Production stage
FROM alpine:latest
RUN apk --no-cache add ca-certificates
WORKDIR /root/
COPY --from=builder /app/server .
EXPOSE 8080
CMD ["./server"]
```

```bash
# Build
docker build -t go-server .

# Check image size
docker images go-server
# Should be ~10-15MB instead of ~300MB+

# Run
docker run -d -p 8080:8080 go-server
curl http://localhost:8080
```

### Exercise 5: Debugging Containers

```bash
# Run a container that crashes
docker run -d --name buggy alpine sh -c "sleep 5 && exit 1"

# Check status
docker ps -a

# View logs
docker logs buggy

# Inspect exit code
docker inspect buggy --format='{{.State.ExitCode}}'

# Debug by running interactively
docker run -it alpine sh

# Override entrypoint to debug
docker run -it --entrypoint sh nginx:alpine
```

---

## Interview Questions

### Basic Questions

**Q: What is the difference between a Docker image and a container?**

> An image is a read-only template with instructions for creating a container. A container is a running instance of an image. You can create multiple containers from the same image.

**Q: What is the difference between CMD and ENTRYPOINT?**

> CMD provides default arguments that can be overridden when running the container. ENTRYPOINT defines the executable that always runs. They can be combined: ENTRYPOINT defines the command, CMD provides default arguments.

**Q: How do Docker layers work?**

> Each instruction in a Dockerfile creates a layer. Layers are cached and reused. If a layer changes, all subsequent layers must be rebuilt. This is why we put frequently changing instructions (like COPY code) at the end.

**Q: What is the difference between COPY and ADD?**

> COPY simply copies files from host to image. ADD has extra features: it can extract tar archives and download from URLs. Best practice is to use COPY unless you need ADD's features.

### Intermediate Questions

**Q: How would you reduce Docker image size?**

> 1. Use smaller base images (alpine)
> 2. Multi-stage builds
> 3. Combine RUN commands to reduce layers
> 4. Remove unnecessary files (apt cache, etc.)
> 5. Use .dockerignore
> 6. Don't install unnecessary packages

**Q: Explain Docker networking modes.**

> - **Bridge**: Default, containers on same bridge can communicate
> - **Host**: Container shares host's network stack
> - **None**: No networking
> - **Custom bridge**: User-defined network with DNS resolution by container name

**Q: How do you persist data in Docker?**

> - **Volumes**: Docker-managed storage, best for production data
> - **Bind mounts**: Host directory mounted into container, good for development
> - **tmpfs**: In-memory storage, for sensitive temporary data

**Q: What happens when you run `docker run nginx`?**

> 1. Docker checks if nginx image exists locally
> 2. If not, pulls from Docker Hub
> 3. Creates a new container from the image
> 4. Allocates a filesystem and mounts a read-write layer
> 5. Creates network interface and assigns IP
> 6. Starts the container and executes the default command

### Advanced Questions

**Q: How does Docker achieve isolation?**

> Docker uses Linux kernel features:
>
> - **Namespaces**: Isolate PID, network, mount, user, etc.
> - **cgroups**: Limit and account for resource usage (CPU, memory)
> - **Union filesystem**: Layered filesystem for images

**Q: What is the difference between `docker-compose up` and `docker stack deploy`?**

> `docker-compose` is for single-host development. `docker stack deploy` is for Docker Swarm, providing multi-host orchestration with features like rolling updates and service scaling.

**Q: How would you debug a container that keeps crashing?**

> 1. Check logs: `docker logs <container>`
> 2. Check exit code: `docker inspect --format='{{.State.ExitCode}}'`
> 3. Run interactively: `docker run -it <image> sh`
> 4. Override entrypoint: `docker run -it --entrypoint sh <image>`
> 5. Check events: `docker events`

**Q: Explain multi-stage builds and when to use them.**

> Multi-stage builds use multiple FROM statements. Each stage can use a different base image. You can copy artifacts from one stage to another. Use cases:
>
> - Compile code in one stage, run in minimal image
> - Separate build dependencies from runtime
> - Reduce final image size significantly

**Q: How do health checks work in Docker?**

> HEALTHCHECK instruction defines a command to test container health. Docker runs it periodically. Container states: starting, healthy, unhealthy. Orchestrators use this to restart unhealthy containers or route traffic away from them.

---

## Quick Reference Card

```bash
# Images
docker build -t name:tag .
docker pull image:tag
docker push image:tag
docker images
docker rmi image

# Containers
docker run -d -p 8080:80 --name c1 image
docker ps -a
docker logs -f c1
docker exec -it c1 bash
docker stop/start/restart c1
docker rm c1

# Compose
docker-compose up -d
docker-compose down
docker-compose logs -f
docker-compose exec service bash

# Cleanup
docker system prune -a
docker volume prune
docker network prune
```

---

## Next Steps

After mastering Docker:

1. Learn **Kubernetes** for container orchestration
2. Study **Docker security** best practices
3. Explore **CI/CD pipelines** with Docker
4. Practice **multi-container architectures**

