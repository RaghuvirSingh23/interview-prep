/*
 * BUILDER PATTERN - Pizza Builder
 * 
 * Problem:
 * --------
 * Design a Pizza class that can be built step-by-step.
 * 
 * Pizza should have:
 * - size (small, medium, large)
 * - crust (thin, thick)
 * - toppings (vector of strings)
 * 
 * Requirements:
 * 1. Create Pizza class with a describe() method
 * 2. Create PizzaBuilder with method chaining:
 *    - setSize(string)
 *    - setCrust(string)
 *    - addTopping(string)
 *    - build() -> returns Pizza
 * 
 * Example Usage:
 *   Pizza pizza = PizzaBuilder()
 *       .setSize("large")
 *       .setCrust("thin")
 *       .addTopping("cheese")
 *       .addTopping("pepperoni")
 *       .build();
 *   
 *   pizza.describe();
 *   // Output: Large thin crust pizza with: cheese, pepperoni
 * 
 * Hint: Builder methods return *this for chaining
 */

#include <iostream>
#include <string>
#include <vector>

using namespace std;

// TODO: Implement Pizza class
class Pizza{
public:
    string size;
    string crust;
    vector<string> toppings;
    
    void describe(){
        cout << size << " " << crust << " crust pizza with: ";
        for (auto topping : toppings){
            cout << topping << " ";
        }
    }
};

// TODO: Implement PizzaBuilder class

class PizzaBuilder {
private:
    Pizza pizza;

public:
    PizzaBuilder& setSize(string size){
        pizza.size = size;
        return *this;
    }

    PizzaBuilder& setCrust(string crust){
        pizza.crust = crust;
        return *this;
    }

    PizzaBuilder& addTopping(string topping){
        pizza.toppings.push_back(topping);
        return *this;
    }

    Pizza build(){
        return pizza;
    }
};

int main(){
    Pizza pizza = PizzaBuilder()
        .setSize("large")
        .setCrust("thin")
        .addTopping("cheese")
        .addTopping("pepperoni")
        .build();

    pizza.describe();
    return 0;
}