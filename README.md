# Interview Prep

A comprehensive preparation kit for software engineering interviews.

---

## Interview Process Overview

Based on common software engineering interview loops:

| Round | Focus | Duration |
|-------|-------|----------|
| **Coding Assessment** | DSA, Problem Solving | ~60 min |
| **Design Round** | LLD/System Design, API Design | ~60 min |
| **Hiring Manager** | Technical Deep Dive, Experience | ~45 min |
| **HR Discussion** | Culture Fit, Compensation | ~30 min |

---

## Repository Structure

```
interview-prep/
├── README.md                    # This file - study guide
├── docs/                        # CS Fundamentals & Language Reference
│   ├── 01_oop_cpp.md           # OOP concepts in C++
│   ├── 02_design_patterns.md   # Common design patterns
│   ├── 03_cpp_fundamentals.md  # Pointers, memory, C/C++
│   ├── 04_c_programming.md     # C language deep dive
│   ├── 05_os_fundamentals.md   # OS concepts (processes, threads, memory)
│   └── 06_networking_fundamentals.md  # Networking (TCP/IP, HTTP, sockets)
├── system-design-topics/        # Backend & Infrastructure (with exercises)
│   ├── 01_docker.md            # Containerization fundamentals
│   ├── 02_redis.md             # Caching, pub/sub, data structures
│   ├── 03_kafka.md             # Event streaming, messaging
│   ├── 04_grpc.md              # High-performance RPC
│   ├── 05_kubernetes.md        # Container orchestration
│   ├── 06_load_balancers.md    # Nginx, HAProxy, load balancing
│   ├── 07_database_scaling.md  # Sharding, replication
│   ├── 08_cap_theorem.md       # Distributed systems theory
│   ├── 09_rate_limiting.md     # Throttling algorithms
│   └── 10_observability.md     # Logging, metrics, tracing
├── system-engineering/          # Hands-on systems tutorials
│   └── tutorials/               # CI/CD, Make, Python/Kubernetes, C/Linux debugging
├── lld/                         # Low-Level Design practice
│   ├── 01_lru_cache_basic.cpp
│   ├── 02_lru_cache_threadsafe.cpp
│   ├── 03_lru_cache_generic.cpp
│   └── solutions/
└── dsa/                         # Data Structures & Algorithms
    ├── linked_lists/
    │   ├── 01_split_odd_even.cpp
    │   ├── 02_reverse_linked_list.cpp
    │   ├── 03_merge_sorted_lists.cpp
    │   ├── 04_detect_cycle.cpp
    │   ├── 05_lru_cache_dll.cpp
    │   └── solutions/
    ├── trees/
    │   ├── 01_shortest_path_nodes.cpp
    │   ├── 02_level_order_traversal.cpp
    │   ├── 03_lowest_common_ancestor.cpp
    │   ├── 04_validate_bst.cpp
    │   ├── 05_serialize_deserialize.cpp
    │   └── solutions/
    └── heaps/
        ├── 01_kth_largest.cpp
        ├── 02_merge_k_sorted.cpp
        ├── 03_top_k_frequent.cpp
        ├── 04_sjf_scheduler.cpp
        └── solutions/
```

---

## Backend & Infrastructure Learning Path

### Priority Topics (Must Learn)

These are high-impact, frequently asked in interviews, and used daily in production:

| # | Topic | Why It's Essential |
|---|-------|-------------------|
| 1 | **Docker** | Containerization is everywhere - you can't avoid it |
| 2 | **Redis** | Caching, sessions, rate limiting - used in almost every system |
| 3 | **Kafka** | Event-driven systems, async processing - standard in microservices |
| 4 | **gRPC** | High-performance service communication - replacing REST internally |
| 5 | **Kubernetes (basics)** | Container orchestration - industry standard for deployment |
| 6 | **Load Balancers (Nginx)** | Every production system needs this - interview favorite |
| 7 | **Database Sharding & Replication** | Core scaling concept - asked in every system design interview |
| 8 | **CAP Theorem & Consistency Models** | Foundation for distributed systems discussions |
| 9 | **Rate Limiting** | Security + scalability - common interview question |
| 10 | **Observability (Logging, Metrics, Tracing)** | Production debugging - separates junior from senior devs |

### Nice to Have (Learn Later)

| Topic | When You'd Need It |
|-------|-------------------|
| GraphQL | If your company uses it (not universal) |
| Service Mesh (Istio) | Large microservices deployments |
| Terraform | Infrastructure teams, DevOps roles |
| Elasticsearch | Search-heavy applications |
| Cassandra/MongoDB | Specific use cases, not universal |
| Event Sourcing/CQRS | Complex domain modeling |
| Consul/etcd | Service discovery (K8s handles most of this) |
| Chaos Engineering | Mature organizations only |
| WebSockets | Real-time apps (chat, gaming) |
| AWS/GCP deep dive | Learn as needed for your job |
| CI/CD (Jenkins, ArgoCD) | Usually team-specific setup |
| RabbitMQ | Simpler alternative to Kafka |

**Recommended Order**: Docker → Redis → Kafka → gRPC → Load Balancers

---

## Study Plan

### Week 1: Foundations

**Day 1-2: C/C++ Fundamentals**
- [ ] Read `docs/03_cpp_fundamentals.md`
- [ ] Read `docs/04_c_programming.md`
- [ ] Focus on: Pointers, References, Memory Layout
- [ ] Practice: Write code without IDE assistance

**Day 3-4: OOP Concepts**
- [ ] Read `docs/01_oop_cpp.md`
- [ ] Focus on: Inheritance, Polymorphism, Virtual Functions
- [ ] Practice: Implement a simple class hierarchy

**Day 5-6: Design Patterns**
- [ ] Read `docs/02_design_patterns.md`
- [ ] Focus on: Singleton, Factory, Observer, Strategy
- [ ] Practice: Implement each pattern from scratch

**Day 7: CS Fundamentals**
- [ ] Read `docs/05_os_fundamentals.md`
- [ ] Read `docs/06_networking_fundamentals.md`
- [ ] Focus on: Processes vs Threads, TCP vs UDP, HTTP basics

### Week 2: DSA Practice

**Day 1-2: Linked Lists**
- [ ] `01_split_odd_even.cpp` - Interview practice sample
- [ ] `02_reverse_linked_list.cpp`
- [ ] `03_merge_sorted_lists.cpp`
- [ ] `04_detect_cycle.cpp`
- [ ] `05_lru_cache_dll.cpp` - Bridges to LLD

**Day 3-4: Trees**
- [ ] `01_shortest_path_nodes.cpp` - Interview practice sample
- [ ] `02_level_order_traversal.cpp`
- [ ] `03_lowest_common_ancestor.cpp`
- [ ] `04_validate_bst.cpp`
- [ ] `05_serialize_deserialize.cpp`

**Day 5-6: Heaps**
- [ ] `01_kth_largest.cpp`
- [ ] `02_merge_k_sorted.cpp`
- [ ] `03_top_k_frequent.cpp`
- [ ] `04_sjf_scheduler.cpp` - Design round prep!

**Day 7: Review**
- [ ] Re-solve any problems you struggled with
- [ ] Time yourself - aim for optimal solutions

### Week 3: LLD Focus (Design Round)

**Day 1-3: LRU Cache Deep Dive**
- [ ] `01_lru_cache_basic.cpp` - Master the basics
- [ ] `02_lru_cache_threadsafe.cpp` - Add concurrency
- [ ] `03_lru_cache_generic.cpp` - Modern C++ features

**Day 4-5: Design Practice**
- [ ] Practice explaining your design decisions
- [ ] Draw diagrams (data structures, class relationships)
- [ ] Discuss trade-offs (time vs space, simplicity vs flexibility)

**Day 6-7: Mock Interviews**
- [ ] Practice with a friend or use online platforms
- [ ] Focus on communication and problem-solving process

---

## How to Use Practice Files

Each `.cpp` file contains:

1. **Problem Statement** - Read carefully, understand constraints
2. **Examples** - Work through by hand first
3. **Hints** - Use only if stuck for >10 minutes
4. **TODO sections** - Write your solution here
5. **Test Cases** - Verify your solution
6. **Solution** - In separate `solutions/` folder (no peeking!)

### Recommended Workflow

```bash
# 1. Open a problem file
# 2. Read the problem, understand examples
# 3. Think about approach (5-10 min)
# 4. Implement your solution in the TODO section
# 5. Compile and run tests
g++ -std=c++17 -o solution filename.cpp && ./solution

# 6. If tests fail, debug
# 7. If stuck >20 min, check hints
# 8. If still stuck, study the solution
# 9. Re-implement from scratch the next day
```

---

## Key Topics to Master

### For Coding Assessment

| Topic | Key Concepts | Practice Files |
|-------|--------------|----------------|
| **Linked Lists** | Reversal, Two Pointers, Floyd's | `dsa/linked_lists/` |
| **Trees** | Traversals, LCA, BST Properties | `dsa/trees/` |
| **Heaps** | Min/Max Heap, Top-K Problems | `dsa/heaps/` |
| **Hash Maps** | O(1) Lookup, Frequency Counting | Used throughout |

### For Design Round

| Topic | Key Concepts | Practice Files |
|-------|--------------|----------------|
| **LRU Cache** | HashMap + DLL, O(1) Operations | `lld/` |
| **Scheduling** | Priority Queues, Job Management | `heaps/04_sjf_scheduler.cpp` |
| **OOP Design** | SOLID, Design Patterns | `docs/01_oop_cpp.md`, `docs/02_design_patterns.md` |

### For CS Fundamentals

| Topic | Key Concepts | Reference |
|-------|--------------|-----------|
| **OS** | Processes, Threads, Synchronization, Memory | `docs/05_os_fundamentals.md` |
| **Networking** | TCP/IP, HTTP, DNS, Sockets | `docs/06_networking_fundamentals.md` |
| **C Programming** | Pointers, Memory, Bit Manipulation | `docs/04_c_programming.md` |

### Common Patterns

1. **Two Pointers**: Fast/slow for cycles, start/end for arrays
2. **BFS/DFS**: Level order, path finding, tree traversals
3. **Heap for Top-K**: Min-heap of size K for K largest
4. **HashMap + List**: O(1) access with ordering (LRU Cache)

---

## Interview Tips

### Before the Interview

- [ ] Test your setup (camera, mic, screen share)
- [ ] Have water nearby
- [ ] Quiet environment
- [ ] IDE/editor ready with C++ configured

### During Coding Round

1. **Clarify** - Ask about constraints, edge cases
2. **Plan** - Discuss approach before coding
3. **Communicate** - Think out loud
4. **Test** - Walk through with examples
5. **Optimize** - Discuss time/space complexity

### During Design Round

1. **Requirements** - Clarify scope and constraints
2. **API First** - Define interfaces before implementation
3. **Data Structures** - Justify your choices
4. **Trade-offs** - Discuss alternatives
5. **Iterate** - Be open to feedback and modifications

### General Tips

- It's okay to not know something - show how you'd figure it out
- Ask clarifying questions - it shows thoroughness
- Treat it as a collaboration, not an exam

---

## Quick Reference

### Time Complexities

| Data Structure | Access | Search | Insert | Delete |
|----------------|--------|--------|--------|--------|
| Array | O(1) | O(n) | O(n) | O(n) |
| Linked List | O(n) | O(n) | O(1) | O(1) |
| Hash Map | O(1) | O(1) | O(1) | O(1) |
| BST (balanced) | O(log n) | O(log n) | O(log n) | O(log n) |
| Heap | O(1) top | O(n) | O(log n) | O(log n) |

### C++ STL Cheat Sheet

```cpp
// Min-Heap
priority_queue<int, vector<int>, greater<int>> minHeap;

// Max-Heap (default)
priority_queue<int> maxHeap;

// Hash Map
unordered_map<int, int> map;

// Hash Set
unordered_set<int> set;

// Doubly Linked List
list<int> dll;
dll.push_front(x);
dll.push_back(x);
dll.splice(pos, dll, it);  // Move element in O(1)
```

---

## Documentation Index

| File | Topics Covered |
|------|----------------|
| `docs/01_oop_cpp.md` | Classes, Inheritance, Polymorphism, Virtual Functions, SOLID, Rule of 3/5/0 |
| `docs/02_design_patterns.md` | Singleton, Factory, Builder, Adapter, Decorator, Observer, Strategy |
| `docs/03_cpp_fundamentals.md` | Pointers, References, Memory Layout, Smart Pointers, Move Semantics |
| `docs/04_c_programming.md` | C Pointers, Arrays, Strings, Structs, Bit Manipulation, File I/O |
| `docs/05_os_fundamentals.md` | Processes, Threads, Synchronization, Deadlocks, Virtual Memory, Scheduling |
| `docs/06_networking_fundamentals.md` | OSI Model, TCP/IP, HTTP/HTTPS, DNS, Socket Programming |

---

## Resources

- [LeetCode](https://leetcode.com) - Practice more problems
- [NeetCode](https://neetcode.io) - Curated problem lists
- [Visualgo](https://visualgo.net) - Algorithm visualizations
- [Docker Docs](https://docs.docker.com) - Official Docker documentation
- [Redis University](https://university.redis.com) - Free Redis courses

---

**Good luck with your interview preparation!**

*Remember: The goal is not just to solve problems, but to demonstrate your problem-solving process and communication skills.*
