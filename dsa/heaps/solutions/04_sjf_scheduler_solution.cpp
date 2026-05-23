/*
 * SOLUTION: Shortest Job First (SJF) Scheduler
 * 
 * Approach: Min-Heap + HashMap + Lazy Deletion
 * - Min-Heap ordered by execution time (then arrival order for ties)
 * - HashMap for O(1) lookup by jobId
 * - Lazy deletion for efficient remove/update
 * 
 * Time Complexity:
 * - addJob: O(log n)
 * - getNextJob: O(log n) amortized
 * - removeJob: O(1)
 * - updateJob: O(log n)
 */

#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <cassert>

struct Job {
    int id;
    int executionTime;
    int arrivalOrder;
    
    Job(int i, int t, int order) : id(i), executionTime(t), arrivalOrder(order) {}
};

class SJFScheduler {
private:
    // Comparator for min-heap: sort by execution time, then by arrival order
    struct JobComparator {
        bool operator()(const Job& a, const Job& b) const {
            if (a.executionTime != b.executionTime) {
                return a.executionTime > b.executionTime;  // Min-heap
            }
            return a.arrivalOrder > b.arrivalOrder;  // FIFO for ties
        }
    };
    
    std::priority_queue<Job, std::vector<Job>, JobComparator> heap;
    std::unordered_map<int, int> jobTimes;  // jobId -> executionTime
    std::unordered_set<int> removed;        // Lazy deletion set
    int orderCounter = 0;
    
    // Clean up removed or stale jobs from top of heap
    void cleanupHeap() {
        while (!heap.empty()) {
            const Job& top = heap.top();
            // Remove if: 1) explicitly removed, or 2) stale (execution time changed)
            if (removed.count(top.id)) {
                removed.erase(top.id);
                heap.pop();
            } else if (!jobTimes.count(top.id) || jobTimes[top.id] != top.executionTime) {
                // Stale entry - job was updated with new execution time
                heap.pop();
            } else {
                break;  // Valid entry found
            }
        }
    }
    
public:
    SJFScheduler() = default;
    
    bool addJob(int jobId, int executionTime) {
        if (jobTimes.count(jobId)) {
            return false;  // Already exists
        }
        
        jobTimes[jobId] = executionTime;
        heap.push(Job(jobId, executionTime, orderCounter++));
        return true;
    }
    
    int getNextJob() {
        cleanupHeap();
        
        if (heap.empty()) {
            return -1;
        }
        
        Job next = heap.top();
        heap.pop();
        jobTimes.erase(next.id);
        
        return next.id;
    }
    
    bool removeJob(int jobId) {
        if (!jobTimes.count(jobId)) {
            return false;
        }
        
        // Lazy deletion: mark as removed, will be cleaned up later
        removed.insert(jobId);
        jobTimes.erase(jobId);
        return true;
    }
    
    bool updateJob(int jobId, int newExecutionTime) {
        if (!jobTimes.count(jobId)) {
            return false;
        }
        
        // For update, we use a version-based approach
        // The old entry will be skipped because its execution time won't match
        jobTimes[jobId] = newExecutionTime;
        heap.push(Job(jobId, newExecutionTime, orderCounter++));
        return true;
    }
    
    bool isEmpty() {
        cleanupHeap();
        return heap.empty();
    }
    
    size_t size() {
        return jobTimes.size();
    }
    
    std::optional<std::pair<int, int>> peekNextJob() {
        cleanupHeap();
        
        if (heap.empty()) {
            return std::nullopt;
        }
        
        const Job& next = heap.top();
        return std::make_pair(next.id, next.executionTime);
    }
};


// ============================================================================
// TEST CASES
// ============================================================================

void runTests() {
    std::cout << "Running SJF Scheduler Tests...\n" << std::endl;
    
    // Test 1: Basic add and get
    {
        SJFScheduler scheduler;
        scheduler.addJob(1, 10);
        scheduler.addJob(2, 5);
        scheduler.addJob(3, 8);
        
        assert(scheduler.size() == 3);
        assert(scheduler.getNextJob() == 2);  // Shortest: 5
        assert(scheduler.getNextJob() == 3);  // Next: 8
        assert(scheduler.getNextJob() == 1);  // Last: 10
        assert(scheduler.isEmpty());
        
        std::cout << "✓ Test 1 passed: Basic add and get" << std::endl;
    }
    
    // Test 2: Duplicate job ID
    {
        SJFScheduler scheduler;
        assert(scheduler.addJob(1, 10) == true);
        assert(scheduler.addJob(1, 5) == false);  // Duplicate
        assert(scheduler.size() == 1);
        
        std::cout << "✓ Test 2 passed: Duplicate job ID rejected" << std::endl;
    }
    
    // Test 3: Remove job
    {
        SJFScheduler scheduler;
        scheduler.addJob(1, 10);
        scheduler.addJob(2, 5);
        scheduler.addJob(3, 8);
        
        assert(scheduler.removeJob(2) == true);
        assert(scheduler.size() == 2);
        assert(scheduler.getNextJob() == 3);  // 2 was removed, 3 is next
        
        std::cout << "✓ Test 3 passed: Remove job" << std::endl;
    }
    
    // Test 4: Update job
    {
        SJFScheduler scheduler;
        scheduler.addJob(1, 10);
        scheduler.addJob(2, 5);
        scheduler.addJob(3, 8);
        
        scheduler.updateJob(1, 3);  // Job 1 now has shortest time
        assert(scheduler.getNextJob() == 1);
        
        std::cout << "✓ Test 4 passed: Update job" << std::endl;
    }
    
    // Test 5: Empty scheduler
    {
        SJFScheduler scheduler;
        assert(scheduler.isEmpty());
        assert(scheduler.getNextJob() == -1);
        assert(scheduler.removeJob(1) == false);
        
        std::cout << "✓ Test 5 passed: Empty scheduler" << std::endl;
    }
    
    // Test 6: Tie-breaking (FIFO for same execution time)
    {
        SJFScheduler scheduler;
        scheduler.addJob(3, 5);
        scheduler.addJob(1, 5);
        scheduler.addJob(2, 5);
        
        // Should return in arrival order: 3, 1, 2
        assert(scheduler.getNextJob() == 3);
        assert(scheduler.getNextJob() == 1);
        assert(scheduler.getNextJob() == 2);
        
        std::cout << "✓ Test 6 passed: Tie-breaking (FIFO)" << std::endl;
    }
    
    // Test 7: Peek without remove
    {
        SJFScheduler scheduler;
        scheduler.addJob(1, 10);
        scheduler.addJob(2, 5);
        
        auto next = scheduler.peekNextJob();
        assert(next.has_value());
        assert(next->first == 2);  // Job ID
        assert(next->second == 5); // Execution time
        assert(scheduler.size() == 2);  // Not removed
        
        std::cout << "✓ Test 7 passed: Peek without remove" << std::endl;
    }
    
    std::cout << "\n=== All tests passed! ===" << std::endl;
}

int main() {
    runTests();
    return 0;
}

