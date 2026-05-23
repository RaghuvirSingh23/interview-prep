# Docker & Kubernetes for Build Engineering

Deep dive into containers and orchestration for NVIDIA compiler team interview. Focused on build infrastructure, not app deployment. For Docker basics, see `system-design-topics/01_docker.md`.

---

## Table of Contents

1. [Why Containers for Compiler CI?](#1-why-containers-for-compiler-ci)
2. [Docker for Build Environments](#2-docker-for-build-environments)
3. [Kubernetes for Build Infrastructure](#3-kubernetes-for-build-infrastructure)
4. [Container Registries](#4-container-registries)
5. [Hands-On Exercises](#5-hands-on-exercises)
6. [Interview Questions](#6-interview-questions)

---

## 1. Why Containers for Compiler CI?


| Problem                                              | How containers solve it                                      |
| ---------------------------------------------------- | ------------------------------------------------------------ |
| "Works on my machine" for GCC/Clang versions         | Immutable images pin exact toolchain + libc + Python         |
| Huge dependency trees (LLVM, CUDA headers, sysroots) | Layer caching + cache mounts speed rebuilds                  |
| Cross-compilation (ARM, different glibc)             | Dedicated images per target triple                           |
| CI farm inconsistency                                | Same Dockerfile = same build on laptop, Jenkins agent, cloud |
| Credentials leaking into build artifacts             | BuildKit secrets never baked into layers                     |


**Build engineering mindset:** A container is a **reproducible build host**, not just an app runtime. You optimize for cache hit rate, determinism, and fast cold starts on ephemeral agents.

---

## 2. Docker for Build Environments

### 2.1 Multi-Stage Builds for C/C++

The core pattern: compile with full dev headers, ship only what you need.

```dockerfile
# syntax=docker/dockerfile:1

# Stage 1: Build
FROM ubuntu:22.04 AS builder
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build pkg-config libssl-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY CMakeLists.txt ./
COPY src ./src
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --parallel $(nproc)

# Stage 2: Minimal runtime
FROM ubuntu:22.04 AS runtime
RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3 ca-certificates \
    && rm -rf /var/lib/apt/lists/*
COPY --from=builder /app/build/my_binary /usr/local/bin/
ENTRYPOINT ["/usr/local/bin/my_binary"]
```

**For compiler teams, you often keep two images:**

1. **Dev/build image** — full headers, static analysis tools, sanitizers, debug symbols
2. **Minimal test runner** — shared libs only, used to run compiled tests

### 2.2 Building Compiler Toolchains in Docker

Pattern: separate **deps image** (rarely changes) from **source compile** (changes often).

```dockerfile
# syntax=docker/dockerfile:1

# Stage 1: stable dependencies (cached layer)
FROM ubuntu:22.04 AS toolchain-deps
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build python3 git \
    libzstd-dev libxml2-dev \
    && rm -rf /var/lib/apt/lists/*

# Stage 2: build LLVM
FROM toolchain-deps AS llvm-build
ARG LLVM_VERSION=18.1.0
RUN git clone --depth 1 --branch llvmorg-${LLVM_VERSION} \
    https://github.com/llvm/llvm-project.git /src
WORKDIR /src
RUN cmake -S llvm -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_ENABLE_PROJECTS="clang;lld" \
    -DLLVM_TARGETS_TO_BUILD="X86;NVPTX" \
    && cmake --build build --parallel $(nproc)

# Stage 3: only the installed binaries
FROM ubuntu:22.04 AS llvm-dist
COPY --from=llvm-build /src/build/bin /usr/local/llvm/bin
COPY --from=llvm-build /src/build/lib /usr/local/llvm/lib
ENV PATH="/usr/local/llvm/bin:${PATH}"
```

**Interview points:**

- You may need a "stage0" compiler to bootstrap the new one
- `LLVM_TARGETS_TO_BUILD` controls size vs coverage (NVPTX for GPU work)
- Use `cmake --install` with `DESTDIR` for clean final images

### 2.3 Layer Caching Optimization

Docker caches layers when instruction text + inputs haven't changed. For large C++ repos:

**Rule 1: Copy dependency manifests first, then source code.**

```dockerfile
# Good: dependency layer is cached unless conanfile changes
COPY conanfile.txt conan.lock* ./
RUN conan install . --build=missing

COPY . .
RUN cmake --build build
```

**Rule 2: Order instructions from least to most frequently changing.**

**Rule 3: Use `.dockerignore` aggressively.**

```gitignore
.git
**/build
**/bazel-*
**/.cache
*.o
*.a
*.so
```

**Registry-backed caching with BuildKit:**

```bash
docker buildx build \
  --cache-to type=registry,ref=myregistry/buildcache:llvm,mode=max \
  --cache-from type=registry,ref=myregistry/buildcache:llvm \
  -t myregistry/llvm-ci:18 .
```

### 2.4 BuildKit Features

BuildKit (default in Docker 23+) adds powerful features for build engineers.

**Cache mounts — persist ccache, Conan caches, apt archives across builds WITHOUT adding them to image layers:**

```dockerfile
# syntax=docker/dockerfile:1
ENV CCACHE_DIR=/ccache
RUN --mount=type=cache,target=/ccache \
    --mount=type=bind,source=.,target=/src,ro \
    cmake -S /src -B /build -G Ninja -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    && cmake --build /build
```

This is huge for compiler CI — ccache on a cache mount can cut build times dramatically on warm agents.

**Secret mounts — never bake tokens into layers:**

```dockerfile
RUN --mount=type=secret,id=netrc,target=/root/.netrc \
    curl --netrc-file /root/.netrc https://private.example.com/toolchain.tgz -o /tmp/t.tgz
```

```bash
docker build --secret id=netrc,src=/tmp/netrc .
```

**SSH mounts — clone private repos:**

```dockerfile
RUN --mount=type=ssh \
    git clone git@github.com:org/private-llvm-fork.git /src
```

```bash
eval $(ssh-agent) && ssh-add ~/.ssh/id_ed25519
docker build --ssh default=$SSH_AUTH_SOCK .
```

### 2.5 Base Images for C/C++ Development


| Base                                     | Pros                                | Cons                                                              |
| ---------------------------------------- | ----------------------------------- | ----------------------------------------------------------------- |
| **ubuntu:22.04** / **debian:bookworm**   | Familiar, glibc, easy CUDA packages | Larger images                                                     |
| **alpine**                               | Small (~5MB)                        | musl libc breaks binary compatibility with glibc-heavy GPU stacks |
| **nvidia/cuda:12.2.0-devel-ubuntu22.04** | CUDA toolkit + dev headers          | Heavy; use multi-stage to trim                                    |
| **Custom "golden" image**                | Org-wide consistency                | Needs versioning + deprecation policy                             |


**Compiler team tip:** Match glibc and libstdc++ in the image to what production/CI agents expect, or you get subtle ABI/test failures.

### 2.6 Cross-Compilation in Docker

**Approach A: Distro cross-compiler packages**

```dockerfile
RUN apt-get install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
ENV CC=aarch64-linux-gnu-gcc CXX=aarch64-linux-gnu-g++
```

**Approach B: CMake toolchain file**

```cmake
# aarch64-toolchain.cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
```

```dockerfile
RUN cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=aarch64-toolchain.cmake
```

**Approach C: Clang with --target=**

Use Clang's cross-target support plus `--sysroot` — common in LLVM-based workflows.

### 2.7 Docker Compose for Build Environments

When builds need sidecars (artifact cache, test database, mock service):

```yaml
services:
  builder:
    build: .
    volumes:
      - .:/src:cached
      - ccache:/ccache
    environment:
      CCACHE_DIR: /ccache
    depends_on:
      - minio

  minio:
    image: minio/minio:latest
    command: server /data
    ports: ["9000:9000"]
    volumes:
      - minio-data:/data

volumes:
  ccache:
  minio-data:
```

### 2.8 Best Practices Summary


| Practice                                     | Why                                     |
| -------------------------------------------- | --------------------------------------- |
| Pin base image by digest                     | Prevents silent dependency drift        |
| Use minimal runtime bases for shipped images | Smaller attack surface, faster pulls    |
| Single-purpose images                        | "LLVM 18 + CUDA 12.2" vs one mega image |
| ccache/sccache with cache mounts             | Speed on incremental CI                 |
| `-ffile-prefix-map`                          | Reproducible debug info paths           |


---

## 3. Kubernetes for Build Infrastructure

### 3.1 Architecture

```
┌─────────────────────────────────────────────────────┐
│                  Control Plane                      │
│  ┌────────────┐ ┌──────────┐ ┌─────────────────┐    │
│  │ API Server │ │  etcd    │ │   Scheduler     │    │
│  │            │ │ (state)  │ │ (places pods)   │    │
│  └──────┬─────┘ └──────────┘ └─────────────────┘    │
│         │       ┌──────────────────────┐            │
│         │       │ Controller Manager   │            │
│         │       └──────────────────────┘            │
└─────────┼───────────────────────────────────────────┘
          │
    ┌─────┴─────────────────────────────┐
    │                                   │
┌───▼──────────────┐    ┌──────────────▼───────┐
│   Worker Node 1  │    │   Worker Node 2      │
│  ┌─────────────┐ │    │  ┌─────────────────┐ │
│  │   kubelet   │ │    │  │    kubelet      │ │
│  └──────┬──────┘ │    │  └───────┬─────────┘ │
│  ┌──────▼──────┐ │    │  ┌───────▼─────────┐ │
│  │ Pod: build  │ │    │  │ Pod: build      │ │
│  │   agent     │ │    │  │   agent (GPU)   │ │
│  └─────────────┘ │    │  └─────────────────┘ │
│  ┌─────────────┐ │    │                      │
│  │ kube-proxy  │ │    │                      │
│  └─────────────┘ │    │                      │
└──────────────────┘    └──────────────────────┘
```


| Component      | Role                                                                        |
| -------------- | --------------------------------------------------------------------------- |
| **API Server** | REST API, source of truth for all cluster operations                        |
| **etcd**       | Distributed key-value store holding cluster state                           |
| **Scheduler**  | Assigns Pods to Nodes based on resource requests and constraints            |
| **kubelet**    | Runs on each node; starts/stops Pods, mounts volumes, reports status        |
| **kube-proxy** | Handles Service networking (iptables/IPVS)                                  |
| **Pod**        | Smallest deployable unit — one or more containers sharing network + storage |


### 3.2 Core Objects for Build Infrastructure


| Object                    | Purpose for builds                                |
| ------------------------- | ------------------------------------------------- |
| **Pod**                   | One-off compile/test run; sidecars for caching    |
| **Job**                   | Run to completion — perfect for batch builds      |
| **CronJob**               | Nightly full builds, scheduled regression tests   |
| **Deployment**            | Long-running services (cache server, registry)    |
| **Service**               | Stable DNS/IP to reach Pods (in-cluster registry) |
| **ConfigMap**             | Non-secret config (cmake flags, feature toggles)  |
| **Secret**                | Registry creds, API tokens                        |
| **PersistentVolumeClaim** | Shared ccache, build caches                       |
| **Namespace**             | Isolation between teams/environments              |


### 3.3 Jobs and CronJobs

**Job** — runs Pod(s) to completion (perfect for builds):

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: nightly-full-build
spec:
  backoffLimit: 1
  ttlSecondsAfterFinished: 86400
  template:
    spec:
      restartPolicy: Never
      containers:
        - name: build
          image: myregistry/ci-builder:1.4.0
          command: ["bash", "-lc", "./scripts/nightly.sh"]
          resources:
            requests:
              cpu: "16"
              memory: "64Gi"
            limits:
              cpu: "32"
              memory: "96Gi"
```

**CronJob** — scheduled batch work:

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: nightly-regression
spec:
  schedule: "0 2 * * *"
  jobTemplate:
    spec:
      template:
        spec:
          restartPolicy: OnFailure
          containers:
            - name: tests
              image: myregistry/llvm-ci:18
              args: ["ctest", "-L", "nightly", "--output-on-failure"]
```

### 3.4 Resource Requests and Limits

```yaml
resources:
  requests:        # scheduler uses this for placement
    cpu: "8"       # guaranteed minimum
    memory: "32Gi"
  limits:          # hard ceiling
    cpu: "16"      # throttled (CFS) if exceeded
    memory: "48Gi" # OOMKilled if exceeded
```

**Build-specific guidance:**

- **Memory:** Link jobs (especially LTO, huge templates) need lots of RAM. Set realistic requests so the scheduler doesn't pack too many builds on one node.
- **CPU limits can hurt:** CPU limits throttle parallel compiles. Some clusters use requests-only (no limits) for build namespaces.
- **OOMKilled** shows exit code 137. Fix by raising limits, reducing `-j`, using ThinLTO instead of full LTO, or split DWARF.

### 3.5 Persistent Volumes for Build Caches

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: shared-ccache
spec:
  accessModes:
    - ReadWriteMany          # needs NFS, CephFS, EFS, etc.
  resources:
    requests:
      storage: 500Gi
```

Mount in a Job:

```yaml
volumeMounts:
  - name: ccache
    mountPath: /ccache
volumes:
  - name: ccache
    persistentVolumeClaim:
      claimName: shared-ccache
```

**ReadWriteMany (RWX)** enables shared cache across concurrent build agents. But watch for cache poisoning — use per-branch keys or ccache's direct mode for correctness.

### 3.6 Node Selectors, Taints, Tolerations

**Route builds to GPU nodes for testing:**

```yaml
spec:
  nodeSelector:
    accelerator: nvidia-gpu
  tolerations:
    - key: "nvidia.com/gpu"
      operator: "Exists"
      effect: "NoSchedule"
```

**How taints work:** A taint on a node says "don't schedule here unless you tolerate me." This keeps expensive GPU nodes reserved for workloads that actually need them.

**Affinity (more fine-grained):**

```yaml
spec:
  affinity:
    nodeAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
          - matchExpressions:
              - key: kubernetes.io/arch
                operator: In
                values: ["amd64"]
```

### 3.7 Jenkins on Kubernetes (Dynamic Agents)

Jenkins controller runs in-cluster (or outside). The **Kubernetes plugin** creates Pod templates per pipeline label.

```groovy
pipeline {
    agent {
        kubernetes {
            yaml """
                apiVersion: v1
                kind: Pod
                spec:
                  containers:
                  - name: jnlp
                    image: jenkins/inbound-agent:4.11-1-jdk17
                  - name: llvm-build
                    image: myregistry/llvm-ci:18
                    command: ['cat']
                    tty: true
                    resources:
                      requests: { cpu: "8", memory: "32Gi" }
                  volumes:
                  - name: ccache
                    persistentVolumeClaim:
                      claimName: shared-ccache
            """
        }
    }
    stages {
        stage('Build') {
            steps {
                container('llvm-build') {
                    sh 'cmake --build build'
                }
            }
        }
    }
}
```

**Why dynamic agents:** Scale to zero when idle. Each build gets a clean environment. Bin pack large builds on beefy nodes.

### 3.8 Networking Basics


| Service Type     | What it does                                      |
| ---------------- | ------------------------------------------------- |
| **ClusterIP**    | Internal only (in-cluster registry, cache server) |
| **NodePort**     | Exposes on each node's port (quick dev access)    |
| **LoadBalancer** | Cloud load balancer to pods (production)          |
| **Ingress**      | HTTP/S routing (NGINX, ALB)                       |


**DNS:** A Service named `my-reg` in namespace `registry` is reachable at `my-reg.registry.svc.cluster.local`.

### 3.9 kubectl Cheat Sheet

```bash
# Context and cluster
kubectl config get-contexts
kubectl cluster-info

# Pods and Jobs
kubectl get pods -n team-compiler-ci
kubectl describe pod llvm-build-1 -n team-compiler-ci
kubectl logs -f job/nightly-build -n team-compiler-ci --all-containers

# Apply manifests
kubectl apply -f manifests/
kubectl delete job nightly-build -n team-compiler-ci

# Debug stuck build
kubectl exec -it llvm-build-1 -n team-compiler-ci -- bash

# Port-forward to in-cluster registry
kubectl port-forward svc/docker-registry 5000:5000 -n registry

# See scheduling failures, OOM events
kubectl get events -n team-compiler-ci --sort-by=.lastTimestamp

# Resource usage
kubectl top nodes
kubectl top pods -n team-compiler-ci
```

### 3.10 Helm Basics

Helm packages Kubernetes YAML as **charts** with templated values.

```bash
# Install a chart
helm repo add jenkins https://charts.jenkins.io
helm install jenkins jenkins/jenkins -n jenkins -f values.yaml

# Upgrade
helm upgrade jenkins jenkins/jenkins -n jenkins -f values.yaml

# Render without applying (preview)
helm template jenkins jenkins/jenkins -f values.yaml
```

`**values.yaml**` overrides image tags, resources, PVC sizes — ideal for per-environment CI configs (dev/staging/prod clusters).

### 3.11 Health Checks


| Probe         | Purpose                                   |
| ------------- | ----------------------------------------- |
| **Liveness**  | Restart if deadlocked                     |
| **Readiness** | Remove from Service endpoints until ready |


```yaml
livenessProbe:
  httpGet:
    path: /healthz
    port: 8080
  initialDelaySeconds: 10
  periodSeconds: 20
readinessProbe:
  httpGet:
    path: /readyz
    port: 8080
  periodSeconds: 5
```

**Anti-pattern:** Liveness probe on a batch compile Pod that takes 45 minutes — the probe will kill a perfectly valid long build. Use Jobs without liveness for batch work.

### 3.12 Namespaces for Build Isolation

```yaml
apiVersion: v1
kind: Namespace
metadata:
  name: team-compiler-ci
```

**Use namespaces to:**

- Apply ResourceQuota (cap total CPU/RAM per team)
- Scope RBAC (separate Role/RoleBinding per team)
- NetworkPolicy (restrict build pods to artifact registry + Git only)

---

## 4. Container Registries

### 4.1 Registry Options


| Registry              | Notes                                                  |
| --------------------- | ------------------------------------------------------ |
| **Docker Hub**        | Public default; rate limits; org namespaces            |
| **Harbor**            | Open source; vulnerability scanning, replication, RBAC |
| **JFrog Artifactory** | Universal binaries + Docker; promotion workflows       |
| **Cloud registries**  | ECR (AWS), GCR/Artifact Registry (GCP), ACR (Azure)    |


### 4.2 Image Tagging Strategy


| Strategy   | Example                     | Use case                  |
| ---------- | --------------------------- | ------------------------- |
| Git SHA    | `llvm-ci:7a3f9c2`           | Traceability to source    |
| Semver     | `llvm-ci:18.1.0`            | Human-friendly releases   |
| Date + SHA | `llvm-ci:20260124-7a3f9c2`  | Sortable + unique         |
| Digest     | `@sha256:abc...`            | Strongest reproducibility |
| `:latest`  | **Avoid** for CI/production | Moves, breaks rollback    |


**Promotion model:**

```
dev:       compiler/ci:7a3f9c2            (auto on green build)
            │
            ▼  (tests pass)
staging:   compiler/ci:7a3f9c2-staging    (integration QA)
            │
            ▼  (sign-off)
prod:      compiler/ci:7a3f9c2-prod       (or use immutable digest)
```

### 4.3 Vulnerability Scanning

- **Harbor** integrates Trivy/Clair on push
- CI gate: fail build if critical CVEs in base image
- **Compiler team reality:** dev images bundle lots of tools (build-essential, Python, Git) — many CVEs in leaf packages. Distinguish build images (internal network only) from shipped runtime.

---

## 5. Hands-On Exercises

### Exercise 1: Multi-Stage C++ Build with ccache

1. Create a hello-world CMake C++ project
2. Write a Dockerfile with `# syntax=docker/dockerfile:1` and `RUN --mount=type=cache,target=/ccache`
3. Build twice: `docker buildx build --progress=plain .`
4. Verify second build shows ccache hit statistics

### Exercise 2: Kubernetes Job + OOM Lab

1. Use `kind` or Minikube
2. Apply a Job that compiles a C file from a ConfigMap
3. Set memory limit too low, observe OOMKilled: `kubectl describe pod ...`
4. Explain exit code 137 and how you'd right-size memory

### Exercise 3: PVC Design for Shared Cache

1. List storage classes: `kubectl get sc`
2. Document why RWX requires NFS/Ceph/EFS
3. Write a short design note: per-branch vs global ccache, corruption risks

### Exercise 4: Helm Values

1. `helm create ci-cache`
2. Edit `values.yaml` for image, PVC size, resource requests
3. `helm template` to preview — no cluster needed

---

## 6. Interview Questions

**Q1: Why Docker for compiler CI instead of bare-metal agents?**
Reproducible toolchains (fixed libc, CMake, Ninja), faster onboarding, consistent flags. Trade-off: need discipline around caching and I/O performance.

**Q2: How does Docker layer caching interact with `COPY . .` in a monorepo?**
Any file change invalidates that COPY layer and all subsequent layers. Mitigate with `.dockerignore`, copy lockfiles first, use BuildKit cache mounts.

**Q3: Volume vs bind mount for builds?**
Bind mount maps host path into container (great for live edits). Named volumes are Docker-managed and can perform better. In K8s, you use emptyDir, PVCs, or CSI volumes.

**Q4: Explain BuildKit secret mounts vs ARG.**
ARG values can leak into image history. Secret mounts expose files only during that RUN instruction and are never persisted in layers.

**Q5: musl (Alpine) vs glibc (Ubuntu) for C++ compiler images?**
Alpine is smaller but musl can cause ABI/runtime issues with prebuilt binaries and GPU stacks. Ubuntu/Debian is safer for LLVM/CUDA ecosystems.

**Q6: What is a multi-stage build?**
Multiple FROM stages; copy only artifacts into a final slim image. Drops headers, object files, build trees — shrinks image size and attack surface.

**Q7: Name K8s objects for ephemeral CI builds.**
Job (one-shot work), ConfigMap/Secret (config/creds), PVC (shared caches), Service (registry access), ResourceQuota, optionally PriorityClass.

**Q8: Job vs Deployment — when each?**
Job runs to completion (batch build). Deployment keeps replicas running (registries, dashboards, cache servers). Long compiles should be Jobs.

**Q9: What happens when a container exceeds memory limit?**
Kernel OOM killer terminates it; Pod shows OOMKilled. Fix by raising limits, reducing parallelism, using ThinLTO, or split DWARF.

**Q10: How do CPU limits affect `make -j`?**
CPU limits throttle the cgroup. Heavy parallel compiles run slower. Some orgs omit CPU limits for build namespaces and use only requests for scheduling.

**Q11: What is a PVC and who provisions the volume?**
PVC is a user's storage request. A StorageClass controller dynamically binds it to a PersistentVolume (EBS, NFS, etc.). Shared caches need ReadWriteMany.

**Q12: ClusterIP vs NodePort vs LoadBalancer?**
ClusterIP = internal VIP. NodePort = each node's port. LoadBalancer = external cloud LB. In-cluster registry: ClusterIP. Human access: port-forward or Ingress.

**Q13: How does Jenkins Kubernetes plugin scale agents?**
Controller requests Pod specs from API server; scheduler places Pods on nodes; JNLP sidecar connects back. Agents terminate after build — elastic scale.

**Q14: Liveness vs readiness probes — common mistake?**
Aggressive liveness on long compile Jobs kills valid work. Use Jobs without liveness for batch builds. Liveness is for long-running services.

**Q15: Why Helm over raw YAML?**
Templating reduces duplication. Versioned releases enable rollbacks. Values files per environment. Charts compose subcharts.

**Q16: How to tag images for compiler CI?**
Immutable tags per commit (`:sha`) plus semver for releases. Never use `:latest` for production. Promote by retagging or digest reference.

**Q17: Node selectors for GPU testing?**
Schedule pods onto GPU nodes with correct drivers. Combine with taints/tolerations so only GPU workloads land there.

**Q18: What is cross-compilation with a sysroot?**
Point the compiler at a root filesystem tree for the target (headers/libs) so it emits binaries for another architecture while running on the host.