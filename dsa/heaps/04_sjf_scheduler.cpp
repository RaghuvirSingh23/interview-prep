/*
 * ============================================================================
 * Problem: Shortest Job First (SJF) Scheduler
 * ============================================================================
 * Difficulty: Medium
 * Source: Interview practice guidelines (design round)
 * Time: 40-50 minutes
 * 
 * DESCRIPTION:
 * ------------
 * Design and implement a Shortest Job First (SJF) scheduler. This is a 
 * CPU scheduling algorithm that selects the job with the smallest execution 
 * time to execute next.
 * 
 * Implement the following operations:
 * - addJob(jobId, executionTime): Add a new job to the scheduler
 * - getNextJob(): Get and remove the job with shortest execution time
 * - removeJob(jobId): Remove a specific job from the scheduler
 * - updateJob(jobId, newExecutionTime): Update execution time of a job
 * - isEmpty(): Check if scheduler has any jobs
 * - size(): Get number of pending jobs
 * 
 * EXAMPLE:
 * --------
 *   SJFScheduler scheduler;
 *   scheduler.addJob(1, 10);   // Job 1 takes 10 units
 *   scheduler.addJob(2, 5);    // Job 2 takes 5 units
 *   scheduler.addJob(3, 8);    // Job 3 takes 8 units
 *   
 *   scheduler.getNextJob();    // Returns Job 2 (shortest: 5)
 *   scheduler.getNextJob();    // Returns Job 3 (next shortest: 8)
 *   scheduler.getNextJob();    // Returns Job 1 (remaining: 10)
 * 
 * CONSTRAINTS:
 * ------------
 * - Job IDs are unique positive integers
 * - Execution times are positive integers
 * - All operations should be efficient
 * 
 * HINTS:
 * ------
 * 1. Use a min-heap (priority queue) ordered by execution time
 * 2. For O(1) lookup by jobId, also maintain a hash map
 * 3. For removal/update, consider lazy deletion or indexed heap
 * 4. Think about tie-breaking (same execution time)
 * 
 * DESIGN CONSIDERATIONS:
 * ----------------------
 * - What if two jobs have the same execution time? (FIFO? Lower ID first?)
 * - How to efficiently remove a job from the middle of the heap?
 * - How to handle updates to execution time?
 * 
 * This problem bridges DSA (heaps) with System Design (scheduler design).
 */

#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <cassert>

// Job structure
struct Job {
    int id;
    int executionTime;
    int arrivalOrder;  // For tie-breaking
    
    Job(int i, int t, int order) : id(i), executionTime(t), arrivalOrder(order) {}
};

class SJFScheduler {
private:
    // TODO: Define your data structures
    // Hint: You'll need a heap and possibly a map for O(1) lookup
    
    int orderCounter = 0;  // For tie-breaking
    
public:
    SJFScheduler() {
        // TODO: Initialize
    }
    
    /*
     * Add a new job to the scheduler
     * @param jobId: Unique identifier for the job
     * @param executionTime: Time required to execute the job
     * @return: true if added successfully, false if jobId already exists
     */
    bool addJob(int jobId, int executionTime) {
        // TODO: Implement
        return false;
    }
    
    /*
     * Get and remove the job with shortest execution time
     * @return: Job ID of the shortest job, or -1 if empty
     */
    int getNextJob() {
        // TODO: Implement
        return -1;
    }
    
    /*
     * Remove a specific job from the scheduler
     * @param jobId: ID of job to remove
     * @return: true if removed, false if job not found
     */
    bool removeJob(int jobId) {
        // TODO: Implement
        // Hint: Consider lazy deletion
        return false;
    }
    
    /*
     * Update the execution time of an existing job
     * @param jobId: ID of job to update
     * @param newExecutionTime: New execution time
     * @return: true if updated, false if job not found
     */
    bool updateJob(int jobId, int newExecutionTime) {
        // TODO: Implement
        return false;
    }
    
    /*
     * Check if scheduler is empty
     */
    bool isEmpty() {
        // TODO: Implement
        return true;
    }
    
    /*
     * Get number of pending jobs
     */
    size_t size() {
        // TODO: Implement
        return 0;
    }
    
    /*
     * Peek at the next job without removing it
     * @return: Optional containing job info, or empty if scheduler is empty
     */
    std::optional<std::pair<int, int>> peekNextJob() {
        // TODO: Implement
        return std::nullopt;
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



// ============================================================================
// SOLUTION: See solutions/04_sjf_scheduler_solution.cpp
// ============================================================================
