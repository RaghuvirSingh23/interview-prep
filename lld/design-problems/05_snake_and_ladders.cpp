#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <cstdlib>
#include <ctime>
#include <map>
using namespace std;

/*

Design a Snake and Ladders game that supports:

- Configurable board size (default 100 cells, numbered 1 to 100)
- Multiple players (2-4), each starting at position 0 (off the board)
- Players take turns rolling a single die (1-6)
- Snakes: if you land on a snake's head, you slide down to its tail
- Ladders: if you land on a ladder's bottom, you climb to its top
- A player must land exactly on 100 to win (if roll overshoots, stay in place)
- The game ends when one player reaches 100
- Print the board state and moves as the game progresses

Rules to handle:
- Snakes and ladders are configured at game setup (list of start->end pairs)
- A snake always goes DOWN (start > end)
- A ladder always goes UP (start < end)
- No chaining: if a snake/ladder lands you on another snake/ladder, do NOT follow it again
- If two players land on the same cell, both stay (no collision)

Example run:
  Players: Alice, Bob
  Snakes: 16->6, 47->26, 49->11, 56->53, 62->19, 64->60, 87->24, 93->73, 95->75, 98->78
  Ladders: 1->38, 4->14, 9->31, 21->42, 28->84, 36->44, 51->67, 71->91, 80->100

  Turn 1: Alice rolls 4 -> moves to 4 -> ladder to 14
  Turn 2: Bob rolls 6 -> moves to 6
  Turn 3: Alice rolls 3 -> moves to 17
  ...
  Turn N: Bob rolls 3 -> moves to 100 -> Bob wins!

Think about:
- How to model the Board, Players, Snakes/Ladders, Die, and the Game itself
- Where does the game loop live?
- How would you make the die injectable/mockable for testing?

Entities
  Interfaced to user
  - Game
    % Game(Player[])
    - Player[]
    + nextTurn()
    + roll() --> return randint()

  - Board (100 cells) --> array [1-100]; --> can be singleton
    % Board()
    - Snake<int, int>
    - Ladder<int,int>
    - move() --> return new position
    + addSnakes(start, end) --> only DOWN
    + addLadder(start, end) --> only UP

  - Player
    - position
*/

class Player{
public:
  int position;
  Player(int pos) : position(pos){};


};

class Board{
private:
  bool isAllowed(int start, int end){
    if(  Snakes.find(start) == Snakes.end() 
      && Ladders.find(start) == Ladders.end()
      && start <= 100 
      && end <= 100
    ){
      return true;
    }
    return false;
  }
public:
  map <int, int> Snakes;
  map <int, int> Ladders;

  static Board& getInstance(){
    static Board board;
    return board;
  }

  void addSnake(int start, int end){
    if(isAllowed(start, end)){
      Snakes[start] = end;
    }
  }

  void addLadder(int start, int end){
    if(isAllowed(start, end)){
      Ladders[start] = end;
    }
  }

  int move(int pos, int val){
    if(pos + val <= 100){
      pos = pos + val;
      if(Snakes.find(pos) != Snakes.end()){
        pos = Snakes[pos];
      } else if (Ladders.find(pos) != Ladders.end()){
        pos = Ladders[pos];
      }

      return pos;
    } else {
      return pos;
    }
  }
};

class Game{
public:
  vector <Player> players;
  int currTurn = 0;
  int totalPlayers;
  Board& board;

  Game()
  : board(Board::getInstance()){};

  void addPlayers(int num){
    totalPlayers = num;
    while(num--){
      players.push_back(Player(1));
    }
  }

  int rollDice(){
    return rand() % 6 + 1;
  }

  void nextTurn(){
    Player& currPlayer = players[currTurn];
    
    int roll = rollDice();
    currPlayer.position = board.move(currPlayer.position, roll);
    if(currPlayer.position == 100) {
      cout << "Player " << currTurn << " Wins" << endl; 
    }

    if(currTurn == totalPlayers - 1) currTurn = 0;
    else currTurn++;
  }
};

// ──────────────── Tests ────────────────

int tests_passed = 0;
int tests_failed = 0;

void check(bool cond, const string& name) {
    if (cond) {
        cout << "  PASS: " << name << endl;
        tests_passed++;
    } else {
        cout << "  FAIL: " << name << endl;
        tests_failed++;
    }
}

void test_board_normal_move() {
    Board& b = Board::getInstance();
    b.Snakes.clear();
    b.Ladders.clear();

    check(b.move(0, 5) == 5, "normal move 0+5=5");
    check(b.move(10, 3) == 13, "normal move 10+3=13");
}

void test_board_snake() {
    Board& b = Board::getInstance();
    b.Snakes.clear();
    b.Ladders.clear();
    b.addSnake(16, 6);

    check(b.move(10, 6) == 6, "land on snake 16 -> slide to 6");
    check(b.move(10, 5) == 15, "land on 15, no snake");
}

void test_board_ladder() {
    Board& b = Board::getInstance();
    b.Snakes.clear();
    b.Ladders.clear();
    b.addLadder(4, 14);

    check(b.move(1, 3) == 14, "land on ladder 4 -> climb to 14");
    check(b.move(1, 4) == 5, "land on 5, no ladder");
}

void test_board_overshoot() {
    Board& b = Board::getInstance();
    b.Snakes.clear();
    b.Ladders.clear();

    check(b.move(98, 5) == 98, "overshoot 98+5=103 -> stay at 98");
    check(b.move(99, 1) == 100, "exact landing 99+1=100");
    check(b.move(95, 5) == 100, "exact landing 95+5=100");
}

void test_board_invalid_snake_ladder() {
    Board& b = Board::getInstance();
    b.Snakes.clear();
    b.Ladders.clear();

    b.addSnake(50, 30);
    b.addLadder(50, 70);
    check(b.Ladders.find(50) == b.Ladders.end(), "can't place ladder where snake exists");

    b.Snakes.clear();
    b.Ladders.clear();
    b.addLadder(50, 70);
    b.addSnake(50, 30);
    check(b.Snakes.find(50) == b.Snakes.end(), "can't place snake where ladder exists");
}

void test_game_turns_alternate() {
    Board& b = Board::getInstance();
    b.Snakes.clear();
    b.Ladders.clear();

    Game g;
    g.addPlayers(3);

    check(g.currTurn == 0, "starts at player 0");
    g.nextTurn();
    check(g.currTurn == 1, "after turn 1 -> player 1");
    g.nextTurn();
    check(g.currTurn == 2, "after turn 2 -> player 2");
    g.nextTurn();
    check(g.currTurn == 0, "after turn 3 -> wraps to player 0");
}

void test_game_player_moves() {
    Board& b = Board::getInstance();
    b.Snakes.clear();
    b.Ladders.clear();

    Game g;
    g.addPlayers(1);

    g.nextTurn();
    check(g.players[0].position > 1, "player moved from starting position");
    check(g.players[0].position <= 7, "player moved at most 6 from pos 1");
}

void test_game_full_run() {
    Board& b = Board::getInstance();
    b.Snakes.clear();
    b.Ladders.clear();
    b.addSnake(16, 6);
    b.addSnake(47, 26);
    b.addLadder(4, 14);
    b.addLadder(9, 31);
    b.addLadder(80, 100);

    Game g;
    g.addPlayers(2);

    bool someone_won = false;
    for (int i = 0; i < 1000; i++) {
        g.nextTurn();
        for (auto& p : g.players) {
            if (p.position == 100) {
                someone_won = true;
                break;
            }
        }
        if (someone_won) break;
    }
    check(someone_won, "game finishes within 1000 turns");
}

int main() {
    srand(time(nullptr));

    cout << "[Board Tests]" << endl;
    test_board_normal_move();
    test_board_snake();
    test_board_ladder();
    test_board_overshoot();
    test_board_invalid_snake_ladder();

    cout << "\n[Game Tests]" << endl;
    test_game_turns_alternate();
    test_game_player_moves();
    test_game_full_run();

    cout << "\n=============================" << endl;
    cout << "Results: " << tests_passed << " passed, " << tests_failed << " failed" << endl;
    return tests_failed > 0 ? 1 : 0;
}
