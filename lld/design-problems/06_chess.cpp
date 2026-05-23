#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <utility>
using namespace std;

/*

Design a Chess game that supports:

- Standard 8x8 board with all 16 pieces per side (King, Queen, 2 Rooks, 2 Bishops, 2 Knights, 8 Pawns)
- Two players (White and Black), White moves first
- Turn-based: players alternate moves
- Input: a move is specified as source and destination (e.g. "e2 e4")
- Validate that the move is legal for that piece type:
    - Pawn: 1 forward (2 from starting row), diagonal capture
    - Rook: horizontal/vertical any distance
    - Knight: L-shape (2+1)
    - Bishop: diagonal any distance
    - Queen: horizontal/vertical/diagonal any distance
    - King: 1 square in any direction
- A piece cannot move through other pieces (except Knight which jumps)
- A piece can capture an opponent's piece by moving to its square
- A player cannot make a move that leaves their own King in check
- Detect check: warn when a King is under attack
- Detect checkmate: game ends when a player has no legal moves and is in check
- Detect stalemate: game ends in draw when a player has no legal moves but is NOT in check

You do NOT need to implement (keep it scoped):
- Castling
- En passant
- Pawn promotion (just ignore when pawn reaches the end, or auto-promote to Queen)
- Draw by repetition or 50-move rule
- Timers

Example run:
  White: e2 e4   (pawn moves forward 2)
  Black: e7 e5   (pawn moves forward 2)
  White: d1 h5   (queen moves)
  Black: b8 c6   (knight moves)
  White: f1 c4   (bishop moves)
  Black: g8 f6   (knight moves)
  White: h5 f7   (queen captures pawn — checkmate!)
  "White wins by checkmate!"

Think about:
- How to model Board, Cell/Square, Piece (base class), each piece type
- Where does move validation live — in the Piece? in the Board? in a MoveValidator?
- How to check if a move puts your own King in check
- How to detect checkmate vs stalemate

  - Board
    - map <Position, Piece> boardState;
    + movePiece(int src, int dest)
    + removePiece(int pos)
    + isCheckmate()

  - pieceType :: Piece
    - color
    - position
    + string movesPossible (int pos)
  
  - Game
    - Board&
    - Players[]
    - currPlayer
    + turn(int src, int dest)

*/

int main() {
    return 0;
}
