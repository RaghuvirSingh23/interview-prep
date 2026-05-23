#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <stack>
#include <functional>
#define logline() cout << __LINE__  << " " << __FUNCTION__ << endl;
using namespace std;

/*

Design a Vending Machine that supports:

Multiple products, each with a name, price, and quantity
User inserts coins/money (can insert multiple times to build up balance)
User selects a product
Machine dispenses product if enough balance and item is in stock
Machine returns change
Handle edge cases: insufficient funds, out of stock, no selection made

Entities:
    enum MachineState { IDLE, HAS_MONEY, DISPENSING, OUT_OF_SERVICE }
    enum Coin { ONE, FIVE, TEN, TWENTY_FIVE }  // or use denominations in ₹

    Product
        - id
        - name
        - price

    Inventory
        - map<Product, quantity>
        + addProduct(product, qty)
        + reduceStock(productId)
        + isAvailable(productId) → bool
        + getProduct(productId) → Product

    VendingMachine (the orchestrator)
        - currentState: MachineState
        - currentBalance: double
        - selectedProduct: Product*
        - inventory: Inventory
        + insertMoney(amount)
        + selectProduct(productId)
        + cancel()
        + dispense()          // private, called internally
        + returnChange()      // private, called internally
        + getBalance()
*/

enum MachineState { IDLE, HAS_MONEY, DISPENSING, OUT_OF_SERVICE };
enum Coin { ONE, FIVE, TEN, TWENTY_FIVE };

class Product{
    int id;
    string name;
    int price;
public:
    Product(int id, string name, int price)
    : id(id), name(name), price(price){};
};

class Inventory{
    int next_id = 1;
    unordered_map<string, Product> products;
    unordered_map<string, int> stock;
    unordered_map<string, int> pricing;
    
    void addProduct(string product, int quantity, int price){
        if(products.find(product) != products.end()){
            // item exists, update quantity
            stock[product]+= quantity;
            pricing[product] = price;
            cout << "product added " << product; logline(); 
        } else {
            Product newProduct = Product(next_id, product, quantity);
            products[product] = newProduct;
            stock[product] = quantity;
            pricing[product] = price;
            next_id++;
            cout << "product created " << product; logline(); 
        }
    }

    void reduceStock(string product, int quantity){
        if(products.find(product) != products.end()){
            if(stock[product] > quantity){
                stock[product]-=quantity;
            } else if(stock[product] < quantity) {
                cout << "dont have so many in stock for " << product; logline(); 
            } else {
                cout << "dont have so many in stock for " << product; logline(); 
                products.erase(products.find(product));
            }
        }
    }

    bool isAvailable(string product, )
};

int main(){
    cout << "hello "; logline();
    return 0;
}
