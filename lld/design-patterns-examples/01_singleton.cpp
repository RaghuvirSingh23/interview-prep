/*
 * SINGLETON PATTERN - Database Connection Pool
 * 
 * Problem:
 * --------
 * Design a DatabaseConnectionPool class that:
 * 
 * 1. Has only ONE instance throughout the application
 * 2. Maintains a pool of available connections (use a simple counter for now)
 * 3. Provides methods:
 *    - getConnection()  -> decrements available count, returns true if successful
 *    - releaseConnection() -> increments available count
 *    - getAvailableCount() -> returns current available connections
 * 4. Initialize with a max pool size (e.g., 5 connections)
 * 
 * Requirements:
 * - Private constructor
 * - Static method getInstance()
 * - Prevent copying and assignment
 * 
 * Example Usage:
 *   DatabaseConnectionPool& pool = DatabaseConnectionPool::getInstance();
 *   pool.getConnection();      // available: 4
 *   pool.getConnection();      // available: 3
 *   pool.releaseConnection();  // available: 4
 */

#include <iostream>
#include <string>

using namespace std;

class DatabaseConnectionPool{
private:
    int connectionCount;
    DatabaseConnectionPool(){
        connectionCount = 5;
        cout << "DB Conn initialised" << endl;
    }

    DatabaseConnectionPool(const DatabaseConnectionPool&) = delete;
public:
    static DatabaseConnectionPool& getInstance(){
        static DatabaseConnectionPool instance;
        return instance;
    }

    bool getConnection() {
        if(connectionCount){
            connectionCount--;
            return true;
        }
        return false;
    }

    bool releaseConnection() {
        if(connectionCount < 5){
            connectionCount++;
            return true;
        }
        return false;
    }

    int getAvailableCount(){
        return connectionCount;
    }
};

int main(){
    DatabaseConnectionPool& pool = DatabaseConnectionPool::getInstance();
    cout << pool.getConnection() << endl;     // available: 4
    cout << pool.getAvailableCount() << endl;
    cout << pool.getConnection() << endl;      // available: 3
    cout << pool.getAvailableCount() << endl;
    cout << pool.releaseConnection() << endl;  // available: 4
    cout << pool.getAvailableCount() << endl;
}

// TODO: Implement DatabaseConnectionPool singleton here
