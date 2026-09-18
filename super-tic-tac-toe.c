#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#pragma comment(lib, "ws2_32.lib")
#endif

// ANSI Color Codes
#define RESET "\033[0m"
#define RED "\033[31m"
#define BLUE "\033[34m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define CYAN "\033[36m"
#define BOLD "\033[1m"

// Network State
SOCKET clientSocket = INVALID_SOCKET;
bool isNetworkGame = false;
bool isHost = false;
char myMark = 'X'; // 'X' for host, 'O' for client

// Game State
char board[9][9]; // 9 sub-boards, each with 9 cells (3x3)
char bigBoard[3][3]; // The state of the 9 sub-boards (' ', 'X', 'O', 'D' for draw)
int nextSubBoardRow = -1; // -1 means any board
int nextSubBoardCol = -1;
char currentPlayer = 'X';

void clearScreen() {
    printf("\033[H\033[J");
}

void sleepMs(int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

void enableANSI() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
    SetConsoleOutputCP(65001);
#endif
}

void enableMouse() {
#ifdef _WIN32
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD dwMode;
    GetConsoleMode(hIn, &dwMode);
    dwMode |= ENABLE_MOUSE_INPUT;
    dwMode &= ~ENABLE_QUICK_EDIT_MODE; // Disable Quick Edit to allow mouse events
    dwMode |= ENABLE_EXTENDED_FLAGS;
    SetConsoleMode(hIn, dwMode);
#endif
}

bool initNetwork() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return false;
    }
#endif
    return true;
}

void cleanupNetwork() {
#ifdef _WIN32
    if (clientSocket != INVALID_SOCKET) {
        closesocket(clientSocket);
    }
    WSACleanup();
#endif
}

bool hostGame(int port) {
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        return false;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Bind failed\n");
        closesocket(listenSocket);
        return false;
    }

    if (listen(listenSocket, 1) == SOCKET_ERROR) {
        printf("Listen failed\n");
        closesocket(listenSocket);
        return false;
    }

    printf(CYAN "Waiting for opponent to connect on port %d...\n" RESET, port);
    printf(YELLOW "Tell your friend to run: supertiktaktoe.exe --join YOUR_IP %d\n" RESET, port);
    clientSocket = accept(listenSocket, NULL, NULL);
    closesocket(listenSocket);

    if (clientSocket == INVALID_SOCKET) {
        printf("Accept failed\n");
        return false;
    }

    printf(GREEN "Opponent connected! You are X (goes first)\n" RESET);
    sleepMs(1500);
    return true;
}

bool joinGame(const char* host, int port) {
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        return false;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(host);

    printf(CYAN "Connecting to %s:%d...\n" RESET, host, port);
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf(RED "Connection failed\n" RESET);
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
        return false;
    }

    printf(GREEN "Connected! You are O (goes second)\n" RESET);
    sleepMs(1500);
    return true;
}

bool sendMove(int row, int col) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d %d\n", row, col);
    int sent = send(clientSocket, buffer, (int)strlen(buffer), 0);
    return sent > 0;
}

bool receiveMove(int* row, int* col) {
    char buffer[32] = {0};
    int received = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) return false;
    return sscanf(buffer, "%d %d", row, col) == 2;
}

void initGame() {
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            board[i][j] = ' ';
        }
    }
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            bigBoard[i][j] = ' ';
        }
    }
}

// Check if a sub-board is won
char checkSubBoardWin(int r, int c) {
    int startRow = r * 3;
    int startCol = c * 3;

    // Rows
    for (int i = 0; i < 3; i++) {
        if (board[startRow + i][startCol] != ' ' &&
            board[startRow + i][startCol] == board[startRow + i][startCol + 1] &&
            board[startRow + i][startCol] == board[startRow + i][startCol + 2]) {
            return board[startRow + i][startCol];
        }
    }
    // Cols
    for (int j = 0; j < 3; j++) {
        if (board[startRow][startCol + j] != ' ' &&
            board[startRow][startCol + j] == board[startRow + 1][startCol + j] &&
            board[startRow][startCol + j] == board[startRow + 2][startCol + j]) {
            return board[startRow][startCol + j];
        }
    }
    // Diagonals
    if (board[startRow][startCol] != ' ' &&
        board[startRow][startCol] == board[startRow + 1][startCol + 1] &&
        board[startRow][startCol] == board[startRow + 2][startCol + 2]) {
        return board[startRow][startCol];
    }
    if (board[startRow][startCol + 2] != ' ' &&
        board[startRow][startCol + 2] == board[startRow + 1][startCol + 1] &&
        board[startRow][startCol + 2] == board[startRow + 2][startCol]) {
        return board[startRow][startCol + 2];
    }

    // Check Draw
    bool full = true;
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++) {
            if(board[startRow+i][startCol+j] == ' ') {
                full = false;
                break;
            }
        }
    }
    if(full) return 'D';

    return ' ';
}

char checkBigBoardWin() {
    // Rows
    for (int i = 0; i < 3; i++) {
        if (bigBoard[i][0] != ' ' && bigBoard[i][0] != 'D' &&
            bigBoard[i][0] == bigBoard[i][1] &&
            bigBoard[i][0] == bigBoard[i][2]) {
            return bigBoard[i][0];
        }
    }
    // Cols
    for (int j = 0; j < 3; j++) {
        if (bigBoard[0][j] != ' ' && bigBoard[0][j] != 'D' &&
            bigBoard[0][j] == bigBoard[1][j] &&
            bigBoard[0][j] == bigBoard[2][j]) {
            return bigBoard[0][j];
        }
    }
    // Diagonals
    if (bigBoard[0][0] != ' ' && bigBoard[0][0] != 'D' &&
        bigBoard[0][0] == bigBoard[1][1] &&
        bigBoard[0][0] == bigBoard[2][2]) {
        return bigBoard[0][0];
    }
    if (bigBoard[0][2] != ' ' && bigBoard[0][2] != 'D' &&
        bigBoard[0][2] == bigBoard[1][1] &&
        bigBoard[0][2] == bigBoard[2][0]) {
        return bigBoard[0][2];
    }
    return ' ';
}

void drawGame() {
    clearScreen();
    printf(BOLD "SUPER TIC TAC TOE\n" RESET);
    printf("Player %s%c%s's turn\n", currentPlayer == 'X' ? RED : BLUE, currentPlayer, RESET);
    if (nextSubBoardRow != -1) {
        printf("Must play in grid (%d, %d)\n", nextSubBoardRow + 1, nextSubBoardCol + 1);
    } else {
        printf("Play anywhere!\n");
    }
    printf("\n");

    // Top border
    printf("    1   2   3   4   5   6   7   8   9\n");
    printf("  " YELLOW "╔═══════════╦═══════════╦═══════════╗" RESET "\n");

    for (int bigRow = 0; bigRow < 3; bigRow++) {
        for (int subRow = 0; subRow < 3; subRow++) {
            printf("%d " YELLOW "║" RESET, bigRow * 3 + subRow + 1);
            for (int bigCol = 0; bigCol < 3; bigCol++) {
                for (int subCol = 0; subCol < 3; subCol++) {
                    int r = bigRow * 3 + subRow;
                    int c = bigCol * 3 + subCol;
                    
                    char cell = board[r][c];
                    char displayChar = cell;
                    char* color = RESET;

                    // Determine color
                    if (cell == 'X') color = RED;
                    else if (cell == 'O') color = BLUE;
                    
                    // Highlight valid moves if constrained
                    bool isValidBoard = (nextSubBoardRow == -1) || (nextSubBoardRow == bigRow && nextSubBoardCol == bigCol);
                    bool isBoardWon = bigBoard[bigRow][bigCol] != ' ';
                    
                    if (isValidBoard && !isBoardWon && cell == ' ') {
                        color = GREEN;
                        displayChar = '.';
                    }

                    // If the big board is won, show the big letter
                    if (isBoardWon) {
                         if (subRow == 1 && subCol == 1) {
                             displayChar = bigBoard[bigRow][bigCol];
                             if (displayChar == 'X') color = RED BOLD;
                             else if (displayChar == 'O') color = BLUE BOLD;
                             else if (displayChar == 'D') { displayChar = '#'; color = CYAN; }
                         }
                    }

                    printf(" %s%c " RESET, color, displayChar);
                    
                    if (subCol < 2) printf("│");
                }
                if (bigCol < 2) printf(YELLOW "║" RESET);
            }
            printf(YELLOW "║" RESET "\n");
            if (subRow < 2) {
                printf("  " YELLOW "║" RESET "───┼───┼───" YELLOW "║" RESET "───┼───┼───" YELLOW "║" RESET "───┼───┼───" YELLOW "║" RESET "\n");
            }
        }
        if (bigRow < 2) {
            printf("  " YELLOW "╠═══════════╬═══════════╬═══════════╣" RESET "\n");
        }
    }
    printf("  " YELLOW "╚═══════════╩═══════════╩═══════════╝" RESET "\n");
}

int main(int argc, char* argv[]) {
    // Parse command-line arguments
    if (argc > 1) {
        if (strcmp(argv[1], "--host") == 0 && argc >= 3) {
            if (!initNetwork()) return 1;
            int port = atoi(argv[2]);
            if (!hostGame(port)) {
                cleanupNetwork();
                return 1;
            }
            isNetworkGame = true;
            isHost = true;
            myMark = 'X';
        } else if (strcmp(argv[1], "--join") == 0 && argc >= 4) {
            if (!initNetwork()) return 1;
            const char* host = argv[2];
            int port = atoi(argv[3]);
            if (!joinGame(host, port)) {
                cleanupNetwork();
                return 1;
            }
            isNetworkGame = true;
            isHost = false;
            myMark = 'O';
        } else {
            printf("Usage:\n");
            printf("  Local game: %s\n", argv[0]);
            printf("  Host game:  %s --host <port>\n", argv[0]);
            printf("  Join game:  %s --join <ip> <port>\n", argv[0]);
            return 1;
        }
    }

    enableANSI();
    enableMouse();
    initGame();

#ifdef _WIN32
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    INPUT_RECORD irInBuf[128];
    DWORD cNumRead;
#endif

    while (true) {
        drawGame();
        
        char winner = checkBigBoardWin();
        if (winner != ' ') {
            printf("\n" BOLD "%sPlayer %c WINS THE GAME!" RESET "\n", winner == 'X' ? RED : BLUE, winner);
            break;
        }

        bool draw = true;
        for(int i=0; i<3; i++) for(int j=0; j<3; j++) if(bigBoard[i][j] == ' ') draw = false;
        if(draw) {
            printf("\n" BOLD "GAME OVER! IT'S A DRAW!" RESET "\n");
            break;
        }

        // Input
        int r = -1, c = -1;
        
        // Network game: check if it's our turn
        if (isNetworkGame) {
            if (currentPlayer != myMark) {
                // Wait for opponent's move
                printf("\n" CYAN "Waiting for opponent's move..." RESET "\n");
                if (!receiveMove(&r, &c)) {
                    printf("\n" RED "Connection lost!" RESET "\n");
                    break;
                }
                printf("Opponent played: (%d, %d)\n", r + 1, c + 1);
                sleepMs(800);
            } else {
                // Our turn - use mouse input
                printf("\n" CYAN "Your turn! Click on the board (or press 'm' for manual input, 'q' to quit)..." RESET "\n");
            }
        } else {
            // Local game - original input method
            printf("\n" CYAN "Click on the board (or press 'm' for manual input, 'q' to quit)..." RESET "\n");
        }
        
        bool validClick = (isNetworkGame && currentPlayer != myMark); // Skip click loop if waiting for opponent
        while (!validClick) {
#ifdef _WIN32
            if (!ReadConsoleInput(hIn, irInBuf, 128, &cNumRead)) continue;
            
            for (DWORD i = 0; i < cNumRead; i++) {
                if (irInBuf[i].EventType == MOUSE_EVENT) {
                    MOUSE_EVENT_RECORD mer = irInBuf[i].Event.MouseEvent;
                    
                    // Only process clicks (press down)
                    if ((mer.dwEventFlags == 0 || mer.dwEventFlags == DOUBLE_CLICK) && 
                        (mer.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)) {
                        
                        int mx = mer.dwMousePosition.X;
                        int my = mer.dwMousePosition.Y;
                        
                        // Map coordinates
                        // X: 4 + c*4 -> c = (X-4)/4
                        // Y: 7 + r*2 -> r = (Y-7)/2  (Adjusted for new clearScreen/header)
                        
                        // Note: clearScreen uses ANSI \033[H which moves to 1,1.
                        // Title (1), Player (2), Info (3), Empty (4), Header (5), TopBorder (6)
                        // Row 0 starts at Line 7 (1-based) -> Index 6 (0-based) ?
                        // Let's re-verify visual lines.
                        // Line 1: SUPER...
                        // Line 2: Player...
                        // Line 3: Must play...
                        // Line 4: (Empty)
                        // Line 5: 1 2 3...
                        // Line 6: ╔...
                        // Line 7: 1 ║ ... (Row 0)
                        
                        // So Row 0 is at Y=6 (0-indexed).
                        // Row 1 is at Y=8.
                        // Formula: r = (my - 6) / 2.
                        
                        // X coords:
                        // "1 " (2 chars) + "║" (1) + " " (1) -> Start at 4.
                        // Cell 0 at 4. Cell 1 at 8.
                        // Formula: c = (mx - 4) / 4.

                        if (mx >= 4 && my >= 6) {
                            int clickedC = (mx - 4) / 4;
                            int clickedR = (my - 6) / 2;
                            
                            if (clickedC >= 0 && clickedC < 9 && clickedR >= 0 && clickedR < 9) {
                                r = clickedR;
                                c = clickedC;
                                validClick = true;
                                break;
                            } else {
                                // Debug output for invalid clicks inside the grid area
                                printf("\033[s\033[22;1H\033[KInvalid Click at (%d, %d) -> Grid(%d, %d)   \033[u", mx, my, clickedR, clickedC);
                            }
                        } else {
                             printf("\033[s\033[22;1H\033[KClick outside grid at (%d, %d)             \033[u", mx, my);
                        }
                    }
                } else if (irInBuf[i].EventType == KEY_EVENT) {
                    if (irInBuf[i].Event.KeyEvent.bKeyDown) {
                        char ch = irInBuf[i].Event.KeyEvent.uChar.AsciiChar;
                        if (ch == 'q') return 0;
                        if (ch == 'm') {
                            // Flush console input buffer to clear any pending events
                            FlushConsoleInputBuffer(hIn);
                            printf("\033[22;1H\033[KEnter row (1-9) and column (1-9): ");
                            int mr, mc;
                            char inputBuf[100];
                            if (fgets(inputBuf, sizeof(inputBuf), stdin)) {
                                if (sscanf(inputBuf, "%d %d", &mr, &mc) == 2) {
                                    r = mr - 1;
                                    c = mc - 1;
                                    validClick = true;
                                }
                            }
                        }
                    }
                }
            }
#else
            // Fallback for non-Windows: manual input only
            printf("\nEnter row (1-9) and column (1-9) (or 0 0 to quit): ");
            int mr, mc;
            char inputBuf[100];
            if (fgets(inputBuf, sizeof(inputBuf), stdin)) {
                if (sscanf(inputBuf, "%d %d", &mr, &mc) == 2) {
                    if (mr == 0 && mc == 0) return 0;
                    r = mr - 1;
                    c = mc - 1;
                    validClick = true;
                }
            }
#endif
        }

        int bRow = r / 3;
        int bCol = c / 3;

        // Check if move is valid based on previous move
        if (nextSubBoardRow != -1) {
            if (bRow != nextSubBoardRow || bCol != nextSubBoardCol) {
                printf("\033[22;1H\033[K" RED BOLD "Invalid move! You must play in the highlighted grid." RESET "\n");
                sleepMs(1500);
                continue;
            }
        }

        // Check if cell is empty
        if (board[r][c] != ' ') {
            printf("\033[22;1H\033[K" RED BOLD "Cell already occupied!" RESET "\n");
            sleepMs(1500);
            continue;
        }

        // Check if the sub-board is already won/full
        if (bigBoard[bRow][bCol] != ' ') {
             printf("\033[22;1H\033[K" RED BOLD "This sub-board is already finished!" RESET "\n");
             sleepMs(1500);
             continue;
        }

        // Make move
        board[r][c] = currentPlayer;
        
        // Send move to opponent in network mode
        if (isNetworkGame && currentPlayer == myMark) {
            if (!sendMove(r, c)) {
                printf("\n" RED "Failed to send move!" RESET "\n");
                break;
            }
        }

        // Check for sub-board win
        char subWin = checkSubBoardWin(bRow, bCol);
        if (subWin != ' ') {
            bigBoard[bRow][bCol] = subWin;
        }

        // Determine next sub-board
        int nextR = r % 3;
        int nextC = c % 3;
        
        if (bigBoard[nextR][nextC] != ' ') {
            nextSubBoardRow = -1; // Free move
        } else {
            nextSubBoardRow = nextR;
            nextSubBoardCol = nextC;
        }

        // Switch player
        currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
    }
    
    printf("\nPress Enter to exit...");
#ifdef _WIN32
    FlushConsoleInputBuffer(hIn);
#endif
    char exitBuf[10];
    fgets(exitBuf, sizeof(exitBuf), stdin);
    
    if (isNetworkGame) cleanupNetwork();
    return 0;
}
