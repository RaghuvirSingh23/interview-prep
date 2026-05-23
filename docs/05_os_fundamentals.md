# Operating Systems - Interview Preparation Guide

## Table of Contents
1. [Process Management](#process-management)
2. [Threads and Concurrency](#threads-and-concurrency)
3. [Synchronization](#synchronization)
4. [Deadlocks](#deadlocks)
5. [Memory Management](#memory-management)
6. [Virtual Memory](#virtual-memory)
7. [CPU Scheduling](#cpu-scheduling)
8. [Inter-Process Communication (IPC)](#inter-process-communication-ipc)
9. [File Systems](#file-systems)
10. [Common Interview Questions](#common-interview-questions)

---

## Process Management

### What is a Process?

A **process** is a program in execution. It includes:
- **Code (Text)**: The program instructions
- **Data**: Global and static variables
- **Stack**: Function calls, local variables, return addresses
- **Heap**: Dynamically allocated memory
- **Process Control Block (PCB)**: OS metadata

### Process States

```
                    +-------------+
         admit     |             |  dispatch
       +---------->|    Ready    |----------+
       |           |             |          |
       |           +-------------+          v
+------+------+          ^           +------+------+
|             |          |           |             |
|     New     |     interrupt       |   Running   |
|             |          |           |             |
+-------------+          |           +------+------+
                         |                  |
                   +-----+-----+            |
                   |           |   I/O wait |
                   |   Ready   |<-----------+
                   |           |            |
                   +-----------+            v
                         ^           +------+------+
                         |           |             |
                   I/O done          |   Waiting   |
                         +-----------+  (Blocked)  |
                                     |             |
                                     +------+------+
                                            |
                                            | exit
                                            v
                                     +-------------+
                                     | Terminated  |
                                     +-------------+
```

### Process Control Block (PCB)

```
+------------------------+
| Process ID (PID)       |
| Process State          |
| Program Counter        |
| CPU Registers          |
| CPU Scheduling Info    |
| Memory Management Info |
| I/O Status Info        |
| Accounting Info        |
+------------------------+
```

### Process Creation (fork)

```c
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();
    
    if (pid < 0) {
        // Fork failed
        perror("fork failed");
        return 1;
    } else if (pid == 0) {
        // Child process
        printf("Child: PID = %d, Parent PID = %d\n", getpid(), getppid());
        // exec replaces process image
        execl("/bin/ls", "ls", "-l", NULL);
    } else {
        // Parent process
        printf("Parent: PID = %d, Child PID = %d\n", getpid(), pid);
        wait(NULL);  // Wait for child to finish
    }
    
    return 0;
}
```

### fork() Behavior

```c
int x = 10;

pid_t pid = fork();

if (pid == 0) {
    x = 20;  // Only changes child's copy
    printf("Child: x = %d\n", x);   // 20
} else {
    printf("Parent: x = %d\n", x);  // 10
}

// fork() creates a COPY of the process
// Child gets copy of all variables (Copy-On-Write optimization)
```

### Zombie and Orphan Processes

**Zombie Process**: Child terminated but parent hasn't called `wait()`
- Entry remains in process table
- Shows as `<defunct>` in `ps`

```c
// Creating a zombie
pid_t pid = fork();
if (pid == 0) {
    exit(0);  // Child exits immediately
}
sleep(60);    // Parent doesn't call wait()
// Child is zombie for 60 seconds
```

**Orphan Process**: Parent terminated before child
- Adopted by init process (PID 1)
- init will call wait() when orphan terminates

---

## Threads and Concurrency

### Process vs Thread

| Process | Thread |
|---------|--------|
| Heavy-weight | Light-weight |
| Own address space | Shares address space |
| Expensive context switch | Cheap context switch |
| IPC needed for communication | Shared memory communication |
| Isolated (crash doesn't affect others) | Crash affects all threads |

### Thread Memory Model

```
Process Memory Space
+----------------------------------+
|           Stack (Thread 1)       |
+----------------------------------+
|           Stack (Thread 2)       |
+----------------------------------+
|           Stack (Thread 3)       |
+----------------------------------+
|               ↓                  |
|                                  |
|               ↑                  |
+----------------------------------+
|        Heap (Shared)             |
+----------------------------------+
|        Data (Shared)             |
+----------------------------------+
|        Text (Shared)             |
+----------------------------------+
```

### POSIX Threads (pthreads)

```c
#include <pthread.h>
#include <stdio.h>

void *thread_func(void *arg) {
    int id = *(int *)arg;
    printf("Thread %d running\n", id);
    return NULL;
}

int main() {
    pthread_t threads[5];
    int ids[5];
    
    // Create threads
    for (int i = 0; i < 5; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, thread_func, &ids[i]);
    }
    
    // Wait for threads to finish
    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }
    
    return 0;
}
// Compile: gcc -pthread program.c
```

### User-Level vs Kernel-Level Threads

| User-Level Threads | Kernel-Level Threads |
|-------------------|---------------------|
| Managed by user library | Managed by OS |
| Fast creation/switching | Slower (system calls) |
| If one blocks, all block | One blocking doesn't affect others |
| Can't use multiple CPUs | True parallelism |

### Thread Models

- **Many-to-One**: Many user threads → one kernel thread
- **One-to-One**: Each user thread → one kernel thread (Linux, Windows)
- **Many-to-Many**: Many user threads → many kernel threads

---

## Synchronization

### Race Condition

```c
// Shared variable
int counter = 0;

void *increment(void *arg) {
    for (int i = 0; i < 100000; i++) {
        counter++;  // NOT atomic!
        // Actually: temp = counter; temp++; counter = temp;
    }
    return NULL;
}

// Two threads running increment() might result in counter < 200000
```

### Critical Section Problem

Requirements for solution:
1. **Mutual Exclusion**: Only one process in critical section
2. **Progress**: If no one is in CS, waiting processes can enter
3. **Bounded Waiting**: Limit on how long a process waits

### Mutex (Mutual Exclusion)

```c
#include <pthread.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int counter = 0;

void *increment(void *arg) {
    for (int i = 0; i < 100000; i++) {
        pthread_mutex_lock(&lock);
        counter++;  // Critical section
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}
```

### Semaphores

```c
#include <semaphore.h>

sem_t sem;

// Binary semaphore (like mutex)
sem_init(&sem, 0, 1);  // Initial value = 1

sem_wait(&sem);   // Decrement (P operation) - blocks if 0
// Critical section
sem_post(&sem);   // Increment (V operation)

// Counting semaphore (resource pool)
sem_init(&sem, 0, 5);  // 5 resources available

sem_wait(&sem);   // Acquire resource
// Use resource
sem_post(&sem);   // Release resource
```

### Condition Variables

```c
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int ready = 0;

// Thread 1: Wait for condition
void *waiter(void *arg) {
    pthread_mutex_lock(&mutex);
    while (!ready) {  // Always use while, not if!
        pthread_cond_wait(&cond, &mutex);
    }
    // Do work
    pthread_mutex_unlock(&mutex);
    return NULL;
}

// Thread 2: Signal condition
void *signaler(void *arg) {
    pthread_mutex_lock(&mutex);
    ready = 1;
    pthread_cond_signal(&cond);   // Wake one waiter
    // pthread_cond_broadcast(&cond);  // Wake all waiters
    pthread_mutex_unlock(&mutex);
    return NULL;
}
```

### Producer-Consumer Problem

```c
#define BUFFER_SIZE 10

int buffer[BUFFER_SIZE];
int count = 0;
int in = 0, out = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

void *producer(void *arg) {
    while (1) {
        int item = produce_item();
        
        pthread_mutex_lock(&mutex);
        while (count == BUFFER_SIZE) {
            pthread_cond_wait(&not_full, &mutex);
        }
        
        buffer[in] = item;
        in = (in + 1) % BUFFER_SIZE;
        count++;
        
        pthread_cond_signal(&not_empty);
        pthread_mutex_unlock(&mutex);
    }
}

void *consumer(void *arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        while (count == 0) {
            pthread_cond_wait(&not_empty, &mutex);
        }
        
        int item = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        count--;
        
        pthread_cond_signal(&not_full);
        pthread_mutex_unlock(&mutex);
        
        consume_item(item);
    }
}
```

### Readers-Writers Problem

```c
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t write_lock = PTHREAD_MUTEX_INITIALIZER;
int readers = 0;

void *reader(void *arg) {
    pthread_mutex_lock(&mutex);
    readers++;
    if (readers == 1) {
        pthread_mutex_lock(&write_lock);  // First reader blocks writers
    }
    pthread_mutex_unlock(&mutex);
    
    // Read data (multiple readers can read simultaneously)
    
    pthread_mutex_lock(&mutex);
    readers--;
    if (readers == 0) {
        pthread_mutex_unlock(&write_lock);  // Last reader releases
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void *writer(void *arg) {
    pthread_mutex_lock(&write_lock);
    // Write data (exclusive access)
    pthread_mutex_unlock(&write_lock);
    return NULL;
}
```

### Spinlock vs Mutex

| Spinlock | Mutex |
|----------|-------|
| Busy waiting (CPU spins) | Blocks thread |
| Good for short critical sections | Good for long critical sections |
| Wastes CPU cycles | Doesn't waste CPU |
| No context switch overhead | Context switch overhead |
| Used in kernel/interrupt handlers | Used in user space |

---

## Deadlocks

### Deadlock Conditions (All 4 must hold)

1. **Mutual Exclusion**: Resource can only be held by one process
2. **Hold and Wait**: Process holds resource while waiting for another
3. **No Preemption**: Resources can't be forcibly taken
4. **Circular Wait**: Circular chain of processes waiting

### Deadlock Example

```c
pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;

void *thread1(void *arg) {
    pthread_mutex_lock(&lock1);
    sleep(1);  // Simulate work
    pthread_mutex_lock(&lock2);  // DEADLOCK! Waiting for lock2
    // ...
    pthread_mutex_unlock(&lock2);
    pthread_mutex_unlock(&lock1);
}

void *thread2(void *arg) {
    pthread_mutex_lock(&lock2);
    sleep(1);
    pthread_mutex_lock(&lock1);  // DEADLOCK! Waiting for lock1
    // ...
    pthread_mutex_unlock(&lock1);
    pthread_mutex_unlock(&lock2);
}
```

### Deadlock Prevention

**Break Mutual Exclusion**: Use sharable resources (not always possible)

**Break Hold and Wait**: Request all resources at once
```c
pthread_mutex_lock(&global_lock);
// Acquire all needed resources
pthread_mutex_unlock(&global_lock);
```

**Break No Preemption**: Release resources if can't get all
```c
if (pthread_mutex_trylock(&lock2) != 0) {
    pthread_mutex_unlock(&lock1);  // Release and retry
}
```

**Break Circular Wait**: Order resources, always acquire in order
```c
// Always lock in order: lock1 before lock2
void *thread1(void *arg) {
    pthread_mutex_lock(&lock1);
    pthread_mutex_lock(&lock2);
    // ...
}

void *thread2(void *arg) {
    pthread_mutex_lock(&lock1);  // Same order!
    pthread_mutex_lock(&lock2);
    // ...
}
```

### Deadlock Detection

**Resource Allocation Graph**: Detect cycles
- Process → Resource (request edge)
- Resource → Process (assignment edge)
- Cycle = potential deadlock

### Deadlock Recovery

1. **Process Termination**: Kill deadlocked processes
2. **Resource Preemption**: Take resources from some processes
3. **Rollback**: Restore to safe state (checkpointing)

---

## Memory Management

### Memory Allocation Strategies

**Contiguous Allocation**:
- **First Fit**: Allocate first hole big enough
- **Best Fit**: Allocate smallest hole big enough
- **Worst Fit**: Allocate largest hole

**Fragmentation**:
- **External**: Free memory scattered in small pieces
- **Internal**: Allocated memory larger than requested

### Paging

Divides physical memory into fixed-size **frames** and logical memory into **pages**.

```
Logical Address (32-bit, 4KB pages)
+------------------+------------------+
|   Page Number    |   Page Offset    |
|    (20 bits)     |    (12 bits)     |
+------------------+------------------+

Page Table Entry
+-------+---+---+---+---+----------------+
| Frame | V | R | M | P | Protection     |
+-------+---+---+---+---+----------------+
V = Valid bit
R = Reference bit
M = Modified (dirty) bit
P = Present bit
```

### Address Translation

```
Logical Address: Page 2, Offset 100

Page Table:
Page 0 → Frame 5
Page 1 → Frame 3
Page 2 → Frame 7  ← Look up
Page 3 → Frame 1

Physical Address = Frame 7 * Page Size + Offset
                 = 7 * 4096 + 100
                 = 28772
```

### Translation Lookaside Buffer (TLB)

Fast cache for page table entries.

```
CPU → TLB → (hit) → Physical Address
         → (miss) → Page Table → Update TLB → Physical Address
```

**Effective Access Time**:
```
EAT = hit_rate × (TLB_time + memory_time) + 
      miss_rate × (TLB_time + 2 × memory_time)
```

### Segmentation

Divides memory into variable-size segments (code, data, stack, heap).

```
Logical Address
+------------------+------------------+
| Segment Number   |     Offset       |
+------------------+------------------+

Segment Table Entry
+------------------+------------------+
|      Base        |      Limit       |
+------------------+------------------+

Physical Address = Base + Offset (if Offset < Limit)
```

---

## Virtual Memory

### Page Replacement Algorithms

**FIFO (First-In-First-Out)**:
```
Reference: 1 2 3 4 1 2 5 1 2 3 4 5 (3 frames)

1: [1, -, -] fault
2: [1, 2, -] fault
3: [1, 2, 3] fault
4: [4, 2, 3] fault (replace 1)
1: [4, 1, 3] fault (replace 2)
2: [4, 1, 2] fault (replace 3)
5: [5, 1, 2] fault (replace 4)
...
```

**LRU (Least Recently Used)**:
```
Reference: 1 2 3 4 1 2 5 1 2 3 4 5 (3 frames)

1: [1, -, -] fault
2: [1, 2, -] fault
3: [1, 2, 3] fault
4: [4, 2, 3] fault (1 is LRU)
1: [4, 1, 3] fault (2 is LRU)
2: [4, 1, 2] fault (3 is LRU)
5: [5, 1, 2] fault (4 is LRU)
1: [5, 1, 2] hit
2: [5, 1, 2] hit
3: [3, 1, 2] fault (5 is LRU)
...
```

**Optimal (OPT)**: Replace page not used for longest time (theoretical best)

**Clock (Second Chance)**: FIFO with reference bit
- If reference bit = 1, give second chance (set to 0)
- If reference bit = 0, replace

### Thrashing

When system spends more time paging than executing.

**Cause**: Too many processes, not enough frames per process

**Solution**: 
- Working Set Model: Keep pages used in last Δ time
- Page Fault Frequency: Adjust frames based on fault rate

### Copy-on-Write (COW)

Optimization for `fork()`:
1. Parent and child share same pages (read-only)
2. On write attempt, copy the page
3. Saves memory and time for processes that exec immediately

---

## CPU Scheduling

### Scheduling Criteria

- **CPU Utilization**: Keep CPU busy
- **Throughput**: Processes completed per time
- **Turnaround Time**: Total time from submission to completion
- **Waiting Time**: Time spent in ready queue
- **Response Time**: Time from submission to first response

### Scheduling Algorithms

**FCFS (First-Come-First-Served)**:
```
Process  Burst Time  Arrival
P1       24          0
P2       3           0
P3       3           0

Gantt: |---P1---|P2|P3|
       0       24  27 30

Average Waiting Time = (0 + 24 + 27) / 3 = 17
```

**SJF (Shortest Job First)** - Non-preemptive:
```
Gantt: |P2|P3|---P1---|
       0  3  6       30

Average Waiting Time = (6 + 0 + 3) / 3 = 3
```

**SRTF (Shortest Remaining Time First)** - Preemptive SJF:
```
Process  Burst  Arrival
P1       8      0
P2       4      1
P3       9      2
P4       5      3

Gantt: |P1|--P2--|--P4--|---P1---|----P3----|
       0  1     5      10       17         26
```

**Round Robin (RR)**:
```
Time Quantum = 4

Process  Burst
P1       24
P2       3
P3       3

Gantt: |P1|P2|P3|P1|P1|P1|P1|P1|
       0  4  7 10 14 18 22 26 30
```

**Priority Scheduling**:
- Lower number = higher priority (typically)
- Can cause **starvation** (low priority never runs)
- Solution: **Aging** (increase priority over time)

**Multilevel Queue**:
- Multiple queues with different priorities
- Each queue can have different algorithm

**Multilevel Feedback Queue**:
- Processes can move between queues
- CPU-bound processes move to lower priority
- I/O-bound processes stay at higher priority

---

## Inter-Process Communication (IPC)

### Pipes

```c
// Anonymous pipe (parent-child only)
int fd[2];
pipe(fd);  // fd[0] = read end, fd[1] = write end

if (fork() == 0) {
    // Child
    close(fd[1]);  // Close write end
    char buf[100];
    read(fd[0], buf, sizeof(buf));
    printf("Child received: %s\n", buf);
    close(fd[0]);
} else {
    // Parent
    close(fd[0]);  // Close read end
    write(fd[1], "Hello", 6);
    close(fd[1]);
    wait(NULL);
}
```

### Named Pipes (FIFOs)

```c
// Create FIFO
mkfifo("/tmp/myfifo", 0666);

// Writer process
int fd = open("/tmp/myfifo", O_WRONLY);
write(fd, "Hello", 6);
close(fd);

// Reader process
int fd = open("/tmp/myfifo", O_RDONLY);
char buf[100];
read(fd, buf, sizeof(buf));
close(fd);
```

### Shared Memory

```c
#include <sys/shm.h>

// Create shared memory
int shmid = shmget(IPC_PRIVATE, 1024, IPC_CREAT | 0666);

// Attach to address space
char *shm = shmat(shmid, NULL, 0);

// Use shared memory
strcpy(shm, "Hello from process 1");

// Detach
shmdt(shm);

// Remove (when done)
shmctl(shmid, IPC_RMID, NULL);
```

### Message Queues

```c
#include <sys/msg.h>

struct message {
    long mtype;
    char mtext[100];
};

// Create message queue
int msgid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);

// Send message
struct message msg = {1, "Hello"};
msgsnd(msgid, &msg, sizeof(msg.mtext), 0);

// Receive message
struct message recv;
msgrcv(msgid, &recv, sizeof(recv.mtext), 1, 0);

// Remove queue
msgctl(msgid, IPC_RMID, NULL);
```

### Signals

```c
#include <signal.h>

void handler(int sig) {
    printf("Caught signal %d\n", sig);
}

int main() {
    signal(SIGINT, handler);   // Ctrl+C
    signal(SIGTERM, handler);  // kill command
    
    while (1) {
        pause();  // Wait for signal
    }
}

// Send signal
kill(pid, SIGTERM);
```

### Sockets

```c
// Server
int server_fd = socket(AF_INET, SOCK_STREAM, 0);

struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_addr.s_addr = INADDR_ANY,
    .sin_port = htons(8080)
};

bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
listen(server_fd, 5);

int client_fd = accept(server_fd, NULL, NULL);
char buf[1024];
read(client_fd, buf, sizeof(buf));
write(client_fd, "Response", 8);
close(client_fd);
close(server_fd);

// Client
int sock = socket(AF_INET, SOCK_STREAM, 0);
connect(sock, (struct sockaddr *)&addr, sizeof(addr));
write(sock, "Request", 7);
read(sock, buf, sizeof(buf));
close(sock);
```

---

## File Systems

### File System Structure

```
+------------------+
|    Boot Block    |  Boot loader
+------------------+
|   Super Block    |  FS metadata (size, block count, etc.)
+------------------+
|   Inode Table    |  File metadata
+------------------+
|   Data Blocks    |  Actual file content
+------------------+
```

### Inode Structure

```
+------------------------+
| File Type & Mode       |
| Link Count             |
| Owner UID/GID          |
| File Size              |
| Timestamps (atime,     |
|   mtime, ctime)        |
| Direct Blocks [12]     | → Data blocks
| Single Indirect        | → Block of pointers
| Double Indirect        | → Block of block pointers
| Triple Indirect        | → Block of block of block pointers
+------------------------+
```

### File Allocation Methods

**Contiguous Allocation**:
- File stored in consecutive blocks
- Fast sequential and random access
- External fragmentation

**Linked Allocation**:
- Each block points to next
- No external fragmentation
- Poor random access

**Indexed Allocation** (Unix):
- Index block contains pointers to data blocks
- Good random access
- Index block overhead

### Directory Implementation

**Linear List**: Simple but slow search O(n)

**Hash Table**: Fast lookup O(1), but collisions

### Hard Links vs Soft Links

```bash
# Hard link - same inode
ln file1 file2
# Both point to same inode
# Deleting one doesn't affect other
# Can't cross filesystems

# Soft link (symbolic) - different inode
ln -s file1 file2
# file2 contains path to file1
# Deleting file1 breaks file2
# Can cross filesystems
```

---

## Common Interview Questions

### Q1: What happens when you type a command in terminal?

1. Shell forks a child process
2. Child calls exec() with command
3. Kernel loads program into memory
4. Sets up stack, heap, registers
5. Starts execution at entry point
6. Parent waits for child to complete

### Q2: Difference between process and thread?

| Process | Thread |
|---------|--------|
| Independent execution unit | Part of a process |
| Own address space | Shares address space |
| Heavy context switch | Light context switch |
| IPC for communication | Shared memory |
| Crash isolated | Crash affects all threads |

### Q3: What is a context switch?

Saving state of current process/thread and loading state of next one:
1. Save registers, PC, stack pointer
2. Update PCB
3. Move to ready queue
4. Select next process
5. Load its state
6. Resume execution

**Cost**: 1-1000 microseconds (depends on hardware, OS)

### Q4: Explain virtual memory

- Abstraction that gives each process illusion of large contiguous memory
- Pages mapped to physical frames via page table
- Not all pages need to be in memory (demand paging)
- Benefits: Process isolation, larger address space, memory protection

### Q5: What is a page fault?

1. CPU accesses page not in memory
2. MMU raises page fault exception
3. OS finds page on disk
4. Loads page into free frame
5. Updates page table
6. Restarts instruction

**Types**:
- Minor: Page in memory but not mapped
- Major: Page must be loaded from disk

### Q6: Explain deadlock with example

Four processes, four resources, each holds one and needs next in cycle:
- P1 holds R1, needs R2
- P2 holds R2, needs R3
- P3 holds R3, needs R4
- P4 holds R4, needs R1

No process can proceed → Deadlock

### Q7: What is a race condition?

When outcome depends on timing/order of thread execution:

```c
// Thread 1          // Thread 2
x = x + 1;          x = x + 1;

// If x = 0 initially, final value could be 1 or 2
// depending on interleaving
```

### Q8: Difference between mutex and semaphore?

| Mutex | Semaphore |
|-------|-----------|
| Binary (locked/unlocked) | Counting (0 to N) |
| Ownership (only locker can unlock) | No ownership |
| For mutual exclusion | For signaling & resource counting |

### Q9: What is thrashing?

System spends more time swapping pages than executing:
- Too many processes
- Not enough memory per process
- High page fault rate
- Solution: Reduce multiprogramming, add memory

### Q10: Explain the boot process

1. **BIOS/UEFI**: Hardware initialization, POST
2. **Bootloader**: Load kernel (GRUB, LILO)
3. **Kernel**: Initialize devices, mount root FS
4. **Init/Systemd**: Start system services
5. **Login**: Present user interface

---

## Quick Reference

### System Calls

| Category | Examples |
|----------|----------|
| Process | fork, exec, wait, exit, getpid |
| File | open, close, read, write, lseek |
| Directory | mkdir, rmdir, chdir, getcwd |
| Memory | mmap, munmap, brk, sbrk |
| IPC | pipe, shmget, msgget, socket |
| Signal | signal, kill, sigaction |

### Important Signals

| Signal | Number | Description |
|--------|--------|-------------|
| SIGHUP | 1 | Hangup |
| SIGINT | 2 | Interrupt (Ctrl+C) |
| SIGQUIT | 3 | Quit (Ctrl+\) |
| SIGKILL | 9 | Kill (cannot be caught) |
| SIGSEGV | 11 | Segmentation fault |
| SIGTERM | 15 | Termination |
| SIGSTOP | 19 | Stop (cannot be caught) |
| SIGCONT | 18 | Continue |
