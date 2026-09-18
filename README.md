# 🎮 Super Tic-Tac-Toe

A console-based implementation of **Super Tic-Tac-Toe** written in **C**, featuring a 9×9 game board divided into nine 3×3 sub-boards, interactive mouse-based gameplay on Windows, and two-player multiplayer over a TCP connection.

---

## 📌 Overview

Super Tic-Tac-Toe expands the traditional Tic-Tac-Toe game into a larger **3×3 grid of 3×3 Tic-Tac-Toe boards**.

The game contains:

* A **9×9 cell board**
* **9 individual 3×3 sub-boards**
* Two players: **X** and **O**
* Sub-board win and draw detection
* Overall game win detection
* Move restrictions based on the previous move
* Local two-player gameplay
* Two-player TCP network gameplay
* Mouse-based board selection on Windows
* Manual coordinate input as an alternative

The program runs as a console application and uses ANSI escape sequences for screen clearing, colors, and formatted output.

---

## ✨ Features

### 🎯 Super Tic-Tac-Toe Rules

The program maintains two levels of game state:

* `board[9][9]` stores the individual cells.
* `bigBoard[3][3]` stores the result of each 3×3 sub-board.

A sub-board can have one of the following states:

| State | Meaning                   |
| ----- | ------------------------- |
| ` `   | Sub-board is still active |
| `X`   | X won the sub-board       |
| `O`   | O won the sub-board       |
| `D`   | Sub-board ended in a draw |

The program checks rows, columns, and diagonals to determine whether a sub-board has been won.

---

### 🔀 Move-Based Sub-Board Restriction

After a player makes a move, the position of that move determines the sub-board where the opponent must play next.

For example, if a player places their mark in the **bottom-right cell of a sub-board**, the opponent is directed to the **bottom-right sub-board** of the overall 3×3 arrangement.

The program calculates this using:

```c
nextR = r % 3;
nextC = c % 3;
```

If the required destination sub-board has already been completed, the next player is allowed to play in **any unfinished sub-board**.

---

### 🏆 Overall Win Detection

Once sub-boards have been completed, the program checks the 3×3 `bigBoard` for:

* Three X sub-boards in a row
* Three O sub-boards in a row
* Three X sub-boards in a column
* Three O sub-boards in a column
* Diagonal combinations

A completed sub-board marked `D` does not contribute toward an overall player victory.

---

### 🖥️ Console Interface

The game renders the complete 9×9 board in the terminal.

It uses ANSI escape sequences to provide:

* Colored X and O marks
* Highlighted valid cells
* Colored board borders
* Turn information
* Current required sub-board
* Win/draw messages

Empty valid cells are displayed using `.` when a specific sub-board is required.

---

### 🖱️ Mouse Input on Windows

On Windows, the program enables console mouse input using the Windows Console API.

Players can click directly on cells of the displayed board.

The program converts the mouse position into a row and column on the 9×9 board.

It also supports keyboard commands:

```text
m → switch to manual coordinate input
q → quit
```

---

### ⌨️ Manual Input

Manual input can be used instead of mouse selection.

The player can enter:

```text
row column
```

using coordinates from 1 to 9.

For example:

```text
4 7
```

represents row 4, column 7.

On non-Windows builds, the source contains a manual-input fallback rather than mouse input.

---

## 🌐 Multiplayer

The program supports two-player network gameplay using **TCP sockets**.

There are two modes:

### Host Mode

The first player starts the game as the host:

```bash
supertiktaktoe.exe --host 5000
```

The host:

* Creates a TCP socket
* Binds it to the specified port
* Starts listening for one connection
* Waits for another player
* Plays as `X`
* Makes the first move

---

### Join Mode

The second player connects using:

```bash
supertiktaktoe.exe --join <HOST_IP> 5000
```

For example:

```bash
supertiktaktoe.exe --join 192.168.1.10 5000
```

The joining player:

* Connects to the host's IP address and port
* Plays as `O`
* Goes second

The program sends the selected row and column between the two players using TCP.

---

## 🔌 Network Communication

The program uses the Windows **Winsock** API for networking.

The following networking operations are implemented:

* Socket creation
* TCP connection
* Server binding
* Listening
* Accepting a client
* Connecting to a host
* Sending moves
* Receiving moves
* Socket cleanup

A move is transmitted as two integer coordinates.

The program does **not** implement a separate game server or matchmaking system. One instance of the program acts as the host and the other connects directly to it.

---

## 🧩 Program Structure

The source is organized around several main responsibilities.

### Network Functions

```c
initNetwork()
cleanupNetwork()
hostGame()
joinGame()
sendMove()
receiveMove()
```

These functions handle Winsock initialization, TCP connections, move transmission, and cleanup.

### Game Functions

```c
initGame()
checkSubBoardWin()
checkBigBoardWin()
```

These functions initialize the board and determine sub-board and overall winners.

### Display/Input Functions

```c
clearScreen()
enableANSI()
enableMouse()
drawGame()
```

These handle console preparation, screen rendering, colors, and mouse support.

### Main Game Loop

`main()`:

1. Parses command-line arguments
2. Initializes networking when required
3. Enables console features
4. Initializes the game
5. Draws the board
6. Checks for a winner or draw
7. Waits for the current player's move
8. Validates the move
9. Updates the board
10. Sends the move in network mode
11. Checks the completed sub-board
12. Determines the next required sub-board
13. Switches players
14. Continues until the game ends

---

## 🛠️ Technologies Used

* **C**
* **TCP/IP sockets**
* **Windows Winsock**
* **Windows Console API**
* **ANSI escape sequences**
* Standard C libraries

### Headers Used

The source includes:

```c
stdio.h
stdlib.h
stdbool.h
string.h
```

and, on Windows:

```c
winsock2.h
ws2tcpip.h
windows.h
```

The Windows build also links against:

```text
ws2_32
```

---

## 💻 Requirements

The source is designed primarily for **Windows**.

For the Windows version, you need:

* Windows
* A C compiler such as GCC/MinGW
* A terminal capable of displaying the program's console output

The source explicitly uses Windows-specific APIs for:

* Winsock networking
* Mouse input
* Console configuration
* Sleep functionality

---

## 🚀 Compilation

Using GCC/MinGW:

```bash
gcc supertiktaktoe.c -o supertiktaktoe.exe -lws2_32
```

The `-lws2_32` option links the Windows Winsock library required by the networking code.

---

## ▶️ Running the Game

### Local Two-Player Game

Run without command-line arguments:

```bash
supertiktaktoe.exe
```

Both players use the same console.

---

### Host a Network Game

On the host computer:

```bash
supertiktaktoe.exe --host 5000
```

The program waits for another player to connect.

---

### Join a Network Game

On the second computer:

```bash
supertiktaktoe.exe --join <HOST_IP> 5000
```

Example:

```bash
supertiktaktoe.exe --join 192.168.1.10 5000
```

The host plays `X` and the joining player plays `O`.

---

## 📋 Command-Line Usage

The program supports:

```text
Local game:
supertiktaktoe.exe

Host game:
supertiktaktoe.exe --host <port>

Join game:
supertiktaktoe.exe --join <ip> <port>
```

---

## 🗺️ Board Representation

The game uses a 9×9 board:

```text
        1   2   3   4   5   6   7   8   9

     ┌───────────┬───────────┬───────────┐
  1  │ .   .   . │ .   .   . │ .   .   . │
  2  │ .   .   . │ .   .   . │ .   .   . │
  3  │ .   .   . │ .   .   . │ .   .   . │
     ├───────────┼───────────┼───────────┤
  4  │ .   .   . │ .   .   . │ .   .   . │
  5  │ .   .   . │ .   .   . │ .   .   . │
  6  │ .   .   . │ .   .   . │ .   .   . │
     ├───────────┼───────────┼───────────┤
  7  │ .   .   . │ .   .   . │ .   .   . │
  8  │ .   .   . │ .   .   . │ .   .   . │
  9  │ .   .   . │ .   .   . │ .   .   . │
     └───────────┴───────────┴───────────┘
```

The larger board is composed of nine smaller 3×3 Tic-Tac-Toe boards.

---

## ⚠️ Project Scope

This project was developed as a **college-level C programming project**.

It focuses on demonstrating:

* C programming
* Arrays and game-state management
* Functions
* Conditional logic
* Console interaction
* Windows console APIs
* TCP socket programming
* Basic client-server communication

It is not intended to be a production-grade networking application.

The source does not include features such as:

* User accounts
* Matchmaking
* Persistent game storage
* Game history
* Authentication
* Encryption
* Internet-wide matchmaking
* A dedicated game server
* Graphical UI

---

## 🧪 Current Input / Networking Considerations

The implementation assumes normal gameplay input.

For example, manual coordinate input is converted directly into zero-based board indexes, and network coordinates are received directly from the connected peer.

Therefore, the project should be considered a **college project rather than hardened production software**.

The network implementation is designed around a direct connection between two program instances rather than a general-purpose multiplayer infrastructure.

---

## 📁 Project Structure

A minimal project setup can contain:

```text
SuperTicTacToe/
│
├── supertiktaktoe.c
└── README.md
```

After compilation:

```text
SuperTicTacToe/
│
├── supertiktaktoe.c
├── supertiktaktoe.exe
└── README.md
```

---

## 🎓 Project Highlights

This project demonstrates the combination of **game logic and networking in C**.

Key implementation areas include:

* 9×9 multidimensional board representation
* Nested 3×3 game-state management
* Sub-board and overall win detection
* Dynamic next-board selection
* Console rendering with ANSI formatting
* Windows mouse-event handling
* TCP socket communication
* Host/client architecture
* Command-line argument parsing

---

## 👨‍💻 Project Status

**Language:** C

**Platform:** Windows

**Game Mode:** Local 2-player / TCP network 2-player

**Interface:** Console-based

**Networking:** TCP sockets

---

## 📬 Contact & Suggestions

For suggestions, feedback, or improvements, contact: nagi1262@protonmail.com

Suggestions and feedback are welcome.
