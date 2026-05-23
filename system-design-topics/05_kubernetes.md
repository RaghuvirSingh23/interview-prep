# Kubernetes

A comprehensive guide to Kubernetes for system design interviews: orchestration, Pods, Deployments, Services, Ingress, scheduling, scaling, storage, security, observability, and troubleshooting.

---

## Table of Contents

1. [What is Kubernetes?](#what-is-kubernetes)
2. [Core Mental Model](#core-mental-model)
3. [Cluster Architecture](#cluster-architecture)
4. [Pods](#pods)
5. [Deployments and ReplicaSets](#deployments-and-replicasets)
6. [Services and Networking](#services-and-networking)
7. [Ingress and API Gateways](#ingress-and-api-gateways)
8. [ConfigMaps and Secrets](#configmaps-and-secrets)
9. [Health Checks and Rollouts](#health-checks-and-rollouts)
10. [Resource Requests, Limits, and Scheduling](#resource-requests-limits-and-scheduling)
11. [Autoscaling](#autoscaling)
12. [Storage](#storage)
13. [Security and RBAC](#security-and-rbac)
14. [Observability and Troubleshooting](#observability-and-troubleshooting)
15. [Production Design Patterns](#production-design-patterns)
16. [Hands-On Exercises](#hands-on-exercises)
17. [Interview Questions](#interview-questions)
18. [Quick Reference](#quick-reference)

---

## What is Kubernetes?

Kubernetes is a container orchestration platform. It runs containers across a cluster of machines and manages deployment, scaling, service discovery, load balancing, health checks, rollouts, and self-healing.

Docker packages an application into a container. Kubernetes runs many containers reliably in production.

```
Container image -> Kubernetes Deployment -> Pods running on nodes
```

### Problems Kubernetes Solves

Without orchestration, you must answer:

- Which machine should run each container?
- What if the machine dies?
- How do services find each other?
- How do we roll out new versions safely?
- How do we scale based on traffic?
- Where do logs and metrics go?
- How do we mount config and secrets?
- How do we expose services externally?

Kubernetes gives standard primitives for those problems.

### What Kubernetes Is Not

Kubernetes is not:

- A magic performance booster
- A database
- A CI/CD system by itself
- A replacement for application-level resilience
- A guarantee that bad container images become production ready

You still need good app design: timeouts, retries, graceful shutdown, observability, config management, and capacity planning.

---

## Core Mental Model

Kubernetes is declarative.

You submit desired state:

```yaml
replicas: 3
image: my-api:v1
```

Kubernetes continuously reconciles actual state toward desired state.

```
Desired state: 3 pods running
Actual state:   2 pods running
Controller:     create 1 more pod
```

### Reconciliation Loop

```
User applies YAML
      |
      v
API Server stores desired state
      |
      v
Controller observes mismatch
      |
      v
Scheduler places new Pod
      |
      v
Kubelet starts container on node
```

### Key Objects

| Object | Purpose |
| --- | --- |
| Pod | Smallest deployable unit |
| Deployment | Manages stateless replicated pods and rollouts |
| ReplicaSet | Ensures a number of pod replicas |
| Service | Stable virtual IP/DNS for pod group |
| Ingress | HTTP routing from outside cluster |
| ConfigMap | Non-secret configuration |
| Secret | Sensitive configuration |
| Job | Run-to-completion workload |
| CronJob | Scheduled job |
| StatefulSet | Stateful replicated workload with stable identity |
| DaemonSet | One pod per node or selected nodes |
| PersistentVolumeClaim | Request for durable storage |

---

## Cluster Architecture

### High-Level Architecture

```
                    Control Plane
       +-------------------------------------+
       | API Server                          |
       | Scheduler                           |
       | Controller Manager                  |
       | etcd                                |
       +------------------+------------------+
                          |
                          v
              Worker Nodes
       +----------+   +----------+   +----------+
       | Node A   |   | Node B   |   | Node C   |
       | kubelet  |   | kubelet  |   | kubelet  |
       | runtime  |   | runtime  |   | runtime  |
       | pods     |   | pods     |   | pods     |
       +----------+   +----------+   +----------+
```

### Control Plane Components

### API Server

The API server is the front door to the cluster. `kubectl`, controllers, and nodes all talk to it.

Responsibilities:

- Authenticate and authorize requests
- Validate objects
- Store desired state in etcd
- Serve watch streams to controllers

### etcd

etcd is a strongly consistent key-value store holding cluster state.

If etcd is lost, the cluster's source of truth is lost. Backups matter.

### Scheduler

The scheduler assigns unscheduled Pods to nodes based on:

- CPU/memory requests
- node capacity
- taints and tolerations
- affinity/anti-affinity
- topology spread
- volume constraints

### Controller Manager

Controllers watch objects and reconcile actual state.

Examples:

- Deployment controller
- ReplicaSet controller
- Job controller
- Node controller

### Worker Node Components

### kubelet

Agent running on each node. It receives Pod specs and ensures containers are running.

### Container Runtime

Runs containers. Examples include containerd and CRI-O.

### kube-proxy / CNI

Networking components route traffic to Services and Pods. Many clusters use a CNI plugin such as Calico, Cilium, or Flannel.

---

## Pods

A Pod is the smallest deployable unit in Kubernetes. It contains one or more containers sharing network namespace and volumes.

```
Pod
+----------------------------+
| Container: app             |
| Container: sidecar         |
| Shared IP: 10.1.2.3        |
| Shared volumes             |
+----------------------------+
```

### Why Pods Instead of Containers?

Pods allow tightly coupled containers to run together.

Examples:

- app container + log shipper sidecar
- app container + service mesh proxy
- app container + config reloader

Most application Pods contain one main container.

### Example Pod

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: nginx
spec:
  containers:
    - name: nginx
      image: nginx:1.27
      ports:
        - containerPort: 80
```

### Pod Lifecycle

Common phases:

- `Pending`: accepted but not running yet
- `Running`: bound to node, at least one container running
- `Succeeded`: all containers exited successfully
- `Failed`: at least one container failed
- `Unknown`: node communication problem

### Why You Rarely Create Pods Directly

If a standalone Pod dies, Kubernetes does not automatically replace it. Use a Deployment, StatefulSet, Job, or DaemonSet.

---

## Deployments and ReplicaSets

### Deployment

A Deployment manages stateless replicas and rolling updates.

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web
spec:
  replicas: 3
  selector:
    matchLabels:
      app: web
  template:
    metadata:
      labels:
        app: web
    spec:
      containers:
        - name: web
          image: my-web:v1
          ports:
            - containerPort: 8080
```

### Deployment -> ReplicaSet -> Pods

```
Deployment
   |
   v
ReplicaSet
   |
   +--> Pod 1
   +--> Pod 2
   +--> Pod 3
```

The Deployment handles rollout history. The ReplicaSet ensures replica count.

### Rolling Update

```
Old: v1 v1 v1
Step: v1 v1 v2
Step: v1 v2 v2
New: v2 v2 v2
```

Important settings:

```yaml
strategy:
  type: RollingUpdate
  rollingUpdate:
    maxUnavailable: 1
    maxSurge: 1
```

### Rollback

```bash
kubectl rollout history deployment/web
kubectl rollout undo deployment/web
```

### Deployment Use Cases

Use Deployment for stateless services:

- web APIs
- background workers
- frontend servers
- stateless processors

Do not use Deployment for databases requiring stable identity and storage. Use managed databases, operators, or StatefulSets with caution.

---

## Services and Networking

Pods are ephemeral. They can be recreated with new IPs. A Service gives stable discovery and load balancing.

### Service Types

| Type | Purpose |
| --- | --- |
| `ClusterIP` | Internal service inside cluster |
| `NodePort` | Expose service on each node port |
| `LoadBalancer` | Provision cloud load balancer |
| `ExternalName` | DNS alias to external service |

### ClusterIP Example

```yaml
apiVersion: v1
kind: Service
metadata:
  name: web
spec:
  type: ClusterIP
  selector:
    app: web
  ports:
    - port: 80
      targetPort: 8080
```

Clients inside cluster call:

```text
http://web.default.svc.cluster.local
```

Usually short name works in same namespace:

```text
http://web
```

### How Service Selects Pods

```
Service selector: app=web
        |
        v
Pods with label app=web
```

If labels do not match, the Service has no endpoints.

### Kubernetes Networking Model

Kubernetes expects:

- Every Pod gets its own IP
- Pods can communicate with other Pods without NAT
- Nodes can communicate with Pods
- Services provide stable virtual access to Pods

The CNI plugin implements this.

### NetworkPolicy

By default, many clusters allow broad pod-to-pod traffic. NetworkPolicy restricts traffic.

Example policy idea:

```
Only api pods can talk to db pods on port 5432
```

Use NetworkPolicies for defense in depth.

---

## Ingress and API Gateways

Services expose workloads inside the cluster. Ingress exposes HTTP routes from outside.

```
Internet -> Load Balancer -> Ingress Controller -> Service -> Pods
```

Example:

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: web
spec:
  rules:
    - host: example.com
      http:
        paths:
          - path: /
            pathType: Prefix
            backend:
              service:
                name: web
                port:
                  number: 80
```

### Ingress Controller

Ingress is only a configuration object. You need an ingress controller such as:

- Nginx Ingress
- HAProxy Ingress
- Traefik
- Envoy/Gateway API implementations
- Cloud provider ingress controllers

### Ingress vs Service LoadBalancer

Use `LoadBalancer` Service for simple L4 exposure.

Use Ingress/Gateway for:

- host/path routing
- TLS termination
- HTTP routing policies
- multiple services behind one external IP

### Gateway API

Gateway API is a newer, more expressive Kubernetes networking API for routing. In interviews, knowing Ingress is usually enough, but Gateway API is worth mentioning as the modern direction for advanced traffic management.

---

## ConfigMaps and Secrets

### ConfigMap

ConfigMaps store non-secret config.

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config
data:
  LOG_LEVEL: "info"
  FEATURE_X_ENABLED: "true"
```

Use as environment variables:

```yaml
envFrom:
  - configMapRef:
      name: app-config
```

### Secret

Secrets store sensitive values.

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: db-secret
type: Opaque
stringData:
  DB_PASSWORD: "change-me"
```

Important: Kubernetes Secrets are base64-encoded by default, not magically encrypted everywhere. Production clusters should enable encryption at rest and integrate with a real secret manager when possible.

### Config Update Behavior

Environment variables from ConfigMaps/Secrets do not update inside already running containers. Mounted volumes can update, but applications must reload them.

Common pattern:

- Change config
- Trigger rollout restart

```bash
kubectl rollout restart deployment/web
```

---

## Health Checks and Rollouts

### Liveness Probe

Liveness checks whether the container should be restarted.

```yaml
livenessProbe:
  httpGet:
    path: /healthz
    port: 8080
  initialDelaySeconds: 10
  periodSeconds: 10
```

Use for deadlocks or unrecoverable stuck states.

### Readiness Probe

Readiness checks whether the Pod should receive traffic.

```yaml
readinessProbe:
  httpGet:
    path: /ready
    port: 8080
  periodSeconds: 5
```

If readiness fails, the Pod stays running but is removed from Service endpoints.

### Startup Probe

Startup probe protects slow-starting apps from premature liveness restarts.

```yaml
startupProbe:
  httpGet:
    path: /startup
    port: 8080
  failureThreshold: 30
  periodSeconds: 2
```

### Probe Design

Good readiness checks:

- Validate app can serve requests
- Check critical local initialization
- Avoid expensive deep dependency checks on every probe

Bad readiness checks:

- Query many downstream services every few seconds
- Fail because optional dependency is down
- Do heavy database work

### Graceful Shutdown

On termination:

```
Kubernetes sends SIGTERM
Pod marked terminating
Readiness should fail / endpoint removed
App stops accepting new work
App finishes in-flight work
Kubernetes sends SIGKILL after grace period if still running
```

Set:

```yaml
terminationGracePeriodSeconds: 30
```

Application must handle SIGTERM.

---

## Resource Requests, Limits, and Scheduling

### Requests

Requests tell Kubernetes how much CPU/memory a container needs for scheduling.

```yaml
resources:
  requests:
    cpu: "250m"
    memory: "256Mi"
```

### Limits

Limits cap usage.

```yaml
resources:
  limits:
    cpu: "500m"
    memory: "512Mi"
```

### CPU vs Memory Behavior

CPU over limit:

- Container is throttled.

Memory over limit:

- Container can be killed with OOMKilled.

### Quality of Service Classes

| QoS | Condition | Eviction priority |
| --- | --- | --- |
| Guaranteed | requests == limits for CPU and memory | Lowest eviction risk |
| Burstable | some requests set | Medium |
| BestEffort | no requests/limits | Highest eviction risk |

### Scheduling Controls

### Node Selector

```yaml
nodeSelector:
  disk: ssd
```

### Affinity / Anti-Affinity

Use affinity to prefer or require placement rules.

Example goals:

- Spread replicas across nodes
- Keep app near cache
- Avoid placing primary and replica on same node

### Taints and Tolerations

Taints repel Pods unless they tolerate the taint.

Use for:

- dedicated nodes
- GPU nodes
- system workloads

### Pod Disruption Budget

PDB controls voluntary disruptions.

```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: web-pdb
spec:
  minAvailable: 2
  selector:
    matchLabels:
      app: web
```

This helps during node drains and upgrades.

---

## Autoscaling

### Horizontal Pod Autoscaler

HPA changes replica count based on metrics.

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: web
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web
  minReplicas: 3
  maxReplicas: 20
  metrics:
    - type: Resource
      resource:
        name: cpu
        target:
          type: Utilization
          averageUtilization: 60
```

HPA can scale on:

- CPU
- memory
- custom metrics
- external metrics such as queue depth

### Vertical Pod Autoscaler

VPA recommends or adjusts CPU/memory requests.

Be careful using VPA and HPA on CPU together because they can fight each other.

### Cluster Autoscaler

Cluster autoscaler adds/removes nodes based on unschedulable pods and underutilized nodes.

### Scaling Gotchas

- App startup time affects scaling response
- HPA needs metrics pipeline
- Queue workers often scale better on queue lag than CPU
- Database connections can explode as replicas scale
- Cold caches after scaling can increase latency
- Pod disruption budgets can block node scale-down

---

## Storage

### Volumes

Volumes mount storage into Pods.

### PersistentVolume and PersistentVolumeClaim

PV is actual storage. PVC is a request for storage.

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: data
spec:
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 10Gi
```

### StatefulSet

StatefulSet gives:

- stable Pod identity
- stable network identity
- stable volume per replica
- ordered rollout/termination

Use for stateful systems that need identity, such as ZooKeeper-like systems, some databases, or brokers.

### Managed Databases vs In-Cluster Databases

For interviews, default to managed databases unless there is a strong reason not to.

Running databases in Kubernetes requires deep operational maturity:

- backups
- restore drills
- disk performance
- anti-affinity
- failover
- upgrades
- data corruption handling

Kubernetes can run stateful systems, but it does not remove database operations work.

---

## Security and RBAC

### RBAC

RBAC controls who can do what.

Key objects:

- Role
- ClusterRole
- RoleBinding
- ClusterRoleBinding
- ServiceAccount

Example principle:

```
App service account can read only its own ConfigMap, not list all Secrets.
```

### Service Accounts

Pods run with a service account identity.

Set explicitly:

```yaml
serviceAccountName: web
```

Do not run everything as the default service account.

### Pod Security

Good defaults:

```yaml
securityContext:
  runAsNonRoot: true
  readOnlyRootFilesystem: true
  allowPrivilegeEscalation: false
```

Container-level example:

```yaml
securityContext:
  capabilities:
    drop:
      - ALL
```

### Image Security

- Use minimal base images
- Pin image versions
- Scan images
- Avoid running as root
- Sign images where possible
- Restrict registries

### Namespace Isolation

Namespaces group resources, but they are not a hard security boundary by themselves. Combine with RBAC, NetworkPolicy, resource quotas, and admission controls.

---

## Observability and Troubleshooting

### Useful kubectl Commands

```bash
kubectl get pods
kubectl get pods -o wide
kubectl describe pod <pod>
kubectl logs <pod>
kubectl logs <pod> -c <container>
kubectl exec -it <pod> -- sh
kubectl get events --sort-by=.lastTimestamp
kubectl top pods
kubectl rollout status deployment/web
```

### Debugging Pod Pending

Common causes:

- insufficient CPU/memory
- node selector mismatch
- taints without tolerations
- PVC not bound
- image pull secret missing

Command:

```bash
kubectl describe pod <pod>
```

Look at `Events`.

### Debugging CrashLoopBackOff

Common causes:

- app exits on startup
- missing config/secret
- failed dependency
- wrong command/args
- liveness probe killing app too early

Commands:

```bash
kubectl logs <pod> --previous
kubectl describe pod <pod>
```

### Debugging Service Not Routing

Check:

```bash
kubectl get service web
kubectl get endpoints web
kubectl get pods --show-labels
```

Likely issue: Service selector labels do not match Pod labels.

### Debugging ImagePullBackOff

Common causes:

- wrong image name/tag
- private registry auth missing
- network/DNS issue
- image does not exist for architecture

### Metrics and Logs

Production cluster needs:

- centralized logs
- metrics scraping
- traces
- Kubernetes events
- alerting on crash loops, pending pods, high restarts, node pressure

---

## Production Design Patterns

### Pattern 1: Stateless Web API

```
Deployment replicas: 3+
Service: ClusterIP
Ingress: public HTTP routing
HPA: CPU/RPS/custom metric
ConfigMap/Secret: config
Readiness/liveness probes
PDB: min available
```

Key interview points:

- graceful shutdown
- resource requests/limits
- avoid storing state on local disk
- external managed database

### Pattern 2: Background Worker

```
Deployment worker replicas
Queue: Kafka/SQS/RabbitMQ/Redis
HPA based on queue lag
Idempotent job processing
Graceful shutdown drains in-flight jobs
```

Do not scale only on CPU if queue lag is the real signal.

### Pattern 3: Canary Deployment

Basic Deployment rolling update gives gradual replacement but not traffic percentage control.

For canary by traffic percentage:

- use service mesh
- ingress controller features
- progressive delivery tool

Example:

```
95% traffic -> stable
5% traffic -> canary
```

Watch error rate and latency before increasing canary traffic.

### Pattern 4: Multi-Tenant Namespace Strategy

For each team/tenant/environment:

- namespace
- resource quota
- limit ranges
- RBAC
- network policies

This creates operational boundaries.

### Pattern 5: Config and Secret Management

Production approach:

- config in ConfigMaps
- secrets from external secret manager if possible
- rollout restart on config changes
- avoid putting secrets in images or Git

---

## Hands-On Exercises

### Exercise 1: Run a Deployment

```bash
kubectl create deployment web --image=nginx:1.27 --replicas=3
kubectl get deploy
kubectl get pods -o wide
```

Explain:

- Deployment
- ReplicaSet
- Pod replicas

### Exercise 2: Expose with a Service

```bash
kubectl expose deployment web --port=80 --target-port=80
kubectl get service web
kubectl get endpoints web
```

Explain how the Service finds Pods.

### Exercise 3: Rolling Update and Rollback

```bash
kubectl set image deployment/web nginx=nginx:1.26
kubectl rollout status deployment/web
kubectl rollout history deployment/web
kubectl rollout undo deployment/web
```

Explain max surge and max unavailable.

### Exercise 4: Add Probes

Write a Deployment with:

- readiness probe on `/ready`
- liveness probe on `/healthz`
- startup probe for slow boot

Explain when each probe should fail.

### Exercise 5: Resource Requests and Limits

Add:

```yaml
resources:
  requests:
    cpu: "100m"
    memory: "128Mi"
  limits:
    cpu: "500m"
    memory: "256Mi"
```

Then explain what happens on CPU overuse and memory overuse.

### Exercise 6: Debug a Broken Service

Create a Service selector that does not match Pod labels. Then run:

```bash
kubectl get endpoints
kubectl describe service
kubectl get pods --show-labels
```

Fix the selector.

---

## Interview Questions

### Basic Questions

**Q: What is Kubernetes?**

Kubernetes is a container orchestration system that schedules containers, keeps desired replicas running, provides service discovery, supports rollouts/rollbacks, and helps manage config, storage, and scaling.

**Q: What is a Pod?**

A Pod is the smallest deployable unit. It contains one or more containers sharing network and volumes.

**Q: What is a Deployment?**

A Deployment manages stateless replicated Pods and rolling updates through ReplicaSets.

**Q: What is a Service?**

A Service gives stable networking and load balancing for a dynamic set of Pods selected by labels.

### Architecture Questions

**Q: What does the scheduler do?**

It assigns unscheduled Pods to nodes based on resource requests, constraints, affinity, taints/tolerations, and node capacity.

**Q: What does kubelet do?**

kubelet runs on each node and ensures the containers described by Pod specs are running and healthy.

**Q: What is etcd used for?**

etcd stores Kubernetes cluster state. The API server persists desired state there.

**Q: How does Kubernetes self-heal?**

Controllers continuously compare desired state and actual state. If a Pod dies, a ReplicaSet/Deployment creates a replacement.

### Production Questions

**Q: Liveness vs readiness probe?**

Readiness controls whether a Pod receives traffic. Liveness controls whether a container should be restarted. Startup probe protects slow-starting apps from liveness failures.

**Q: Requests vs limits?**

Requests are used for scheduling. Limits cap runtime usage. CPU over limit is throttled; memory over limit can cause OOMKilled.

**Q: How would you expose an app to the internet?**

Use Ingress or Gateway API with an ingress controller, routing to a Service, which routes to Pods. For simple L4 exposure, use a LoadBalancer Service.

**Q: How do you scale an app?**

Use HPA for pod replicas based on CPU/custom metrics, cluster autoscaler for nodes, and ensure downstream dependencies can handle increased load.

**Q: How do you perform zero-downtime deploys?**

Use rolling updates, readiness probes, graceful shutdown, sufficient replicas, PDBs, and canary/progressive delivery for risky changes.

### Troubleshooting Questions

**Q: Pod is Pending. What do you check?**

`kubectl describe pod`, events, resource availability, node selectors, taints/tolerations, PVC binding, and image pull secrets.

**Q: Pod is CrashLoopBackOff. What do you check?**

Previous logs, events, config/secrets, app startup, command/args, liveness probe timing, and dependency availability.

**Q: Service returns no traffic. What do you check?**

Service selector, Pod labels, endpoints, targetPort, readiness status, network policies, and app listening address.

### Design Questions

**Q: Should you run databases on Kubernetes?**

Prefer managed databases in most interviews. Stateful workloads can run on Kubernetes with StatefulSets/operators, but require careful storage, backups, failover, anti-affinity, upgrades, and restore testing.

**Q: How do you design a resilient Kubernetes web service?**

Deployment with multiple replicas, readiness/liveness/startup probes, resource requests/limits, HPA, Service, Ingress, PDB, graceful shutdown, centralized logs/metrics/traces, RBAC, secrets management, and external managed database.

---

## Quick Reference

### Essential Commands

```bash
kubectl get pods
kubectl get deploy
kubectl get svc
kubectl describe pod <pod>
kubectl logs <pod>
kubectl logs <pod> --previous
kubectl exec -it <pod> -- sh
kubectl apply -f file.yaml
kubectl rollout status deployment/<name>
kubectl rollout undo deployment/<name>
kubectl get events --sort-by=.lastTimestamp
```

### Object Selection

| Need | Kubernetes object |
| --- | --- |
| Stateless replicated app | Deployment |
| Stable network identity/storage | StatefulSet |
| One pod per node | DaemonSet |
| Run once to completion | Job |
| Run on schedule | CronJob |
| Internal stable endpoint | Service ClusterIP |
| External HTTP routing | Ingress/Gateway |
| Non-secret config | ConfigMap |
| Sensitive config | Secret |

### Interview Sound Bites

- "Kubernetes is a reconciliation system: desired state vs actual state."
- "Pods are ephemeral; Services provide stable discovery."
- "Readiness affects traffic, liveness affects restart."
- "Requests schedule Pods; limits enforce runtime caps."
- "A Deployment is for stateless apps; stateful systems need stronger storage and operational planning."
