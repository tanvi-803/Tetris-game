#include <iostream>   // For standard I/O
#include <vector>     // For using vector containers
#include <cstdlib>    // For rand() and srand()
#include <ctime>      // For time()
#include <unistd.h>   // For POSIX operating system API
#include <ncurses.h>  // For terminal-based graphical interface
#include <locale.h>   // For handling locale-specific settings

using namespace std;

// Game constants
const int WIDTH = 30;      // Width of the game grid
const int HEIGHT = 30;     // Height of the game grid
const int BLOCK_SIZE = 4;  // Size of each tetromino block (4x4)

// Define all possible tetromino shapes 
const int SHAPES[7][BLOCK_SIZE][BLOCK_SIZE] = {
    {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}}, // I shape 
    {{0,0,0,0}, {0,1,1,0}, {0,1,1,0}, {0,0,0,0}},  // O shape 
    {{0,0,0,0}, {0,1,0,0}, {1,1,1,0}, {0,0,0,0}},  // T shape
    {{0,0,0,0}, {0,1,1,0}, {1,1,0,0}, {0,0,0,0}},  // S shape
    {{0,0,0,0}, {1,1,0,0}, {0,1,1,0}, {0,0,0,0}},  // Z shape
    {{0,0,0,0}, {1,0,0,0}, {1,1,1,0}, {0,0,0,0}},  // J shape
    {{0,0,0,0}, {0,0,1,0}, {1,1,1,0}, {0,0,0,0}}   // L shape
};

// Color enumeration for different tetromino pieces & boundry.
enum Color {
    CYAN = 1,    
    YELLOW = 2,  
    MAGENTA = 3, 
    GREEN = 4,   
    RED = 5,     
    BLUE = 6,    
    BROWN = 7,   
    BOUNDARY = 8 
};

// Global variables for screen positioning
int centerX, centerY; // Center position of the game on screen

// Function to print a colored block
void printBlock(int color) {
    if (has_colors()) {
        attron(COLOR_PAIR(color)); // Enable color attribute
    }
    printw("\u2588"); // Print a full block character
    if (has_colors()) {
        attroff(COLOR_PAIR(color)); // Disable color attribute
    }
}

// Initialize color pairs for ncurses
void initColors() {
    setlocale(LC_ALL, ""); // Set locale for Unicode support
    
    if (!has_colors()) {
        printw("Terminal doesn't support colors!\n");
        refresh();
        return;
    }
    
    // Initialize color pairs (foreground and background)
    start_color();
    use_default_colors();
    init_pair(CYAN, COLOR_CYAN, COLOR_CYAN);
    init_pair(YELLOW, COLOR_YELLOW, COLOR_YELLOW);
    init_pair(MAGENTA, COLOR_MAGENTA, COLOR_MAGENTA);
    init_pair(GREEN, COLOR_GREEN, COLOR_GREEN);
    init_pair(RED, COLOR_RED, COLOR_RED);
    init_pair(BLUE, COLOR_BLUE, COLOR_BLUE);
    init_pair(BROWN, COLOR_YELLOW, COLOR_YELLOW);
    init_pair(BOUNDARY, COLOR_WHITE, COLOR_BLACK);
}

// Move cursor to position (relative to game center)
void gotoxy(int x, int y) {
    move(centerY + y, centerX + x);
}

// Tetromino class representing each falling piece
class Tetromino {
public:
    int shape[BLOCK_SIZE][BLOCK_SIZE]; // 4x4 grid representing the piece
    int x, y;                          // Position on the game grid
    int type;                          // Type of tetromino (0-6)
    Color color;                       // Color of the piece

    // Constructor - creates a random tetromino
    Tetromino() {
        type = rand() % 7; // Randomly select a shape
        // Copy the shape from the predefined array
        for (int i = 0; i < BLOCK_SIZE; i++)
            for (int j = 0; j < BLOCK_SIZE; j++)
                shape[i][j] = SHAPES[type][i][j];
        // Set initial position (centered at top)
        x = WIDTH / 2 - BLOCK_SIZE / 2;
        y = 0;
        // Set color based on piece type
        switch (type) {
            case 0: color = CYAN; break;
            case 1: color = YELLOW; break;
            case 2: color = MAGENTA; break;
            case 3: color = GREEN; break;
            case 4: color = RED; break;
            case 5: color = BLUE; break;
            case 6: color = BROWN; break;
        }
    }

    // Copy constructor
    Tetromino(const Tetromino& other) {
        type = other.type;
        for (int i = 0; i < BLOCK_SIZE; i++)
            for (int j = 0; j < BLOCK_SIZE; j++)
                shape[i][j] = other.shape[i][j];
        x = other.x;
        y = other.y;
        color = other.color;
    }

    // Rotate the piece 90 degrees clockwise
    void rotate() {
        int temp[BLOCK_SIZE][BLOCK_SIZE];
        // Perform rotation
        for (int i = 0; i < BLOCK_SIZE; i++)
            for (int j = 0; j < BLOCK_SIZE; j++)
                temp[j][BLOCK_SIZE - 1 - i] = shape[i][j];
        // Copy rotated shape back
        for (int i = 0; i < BLOCK_SIZE; i++)
            for (int j = 0; j < BLOCK_SIZE; j++)
                shape[i][j] = temp[i][j];
    }
};

// Main game class
class TetrisGame {
private:
    vector<vector<int>> grid;       // Game grid (0 = empty, 1 = filled)
    vector<vector<int>> colorGrid;  // Color information for each cell
    Tetromino currentPiece;         // Currently active falling piece
    vector<Tetromino> nextPieces;   // Queue of next pieces to appear
    Tetromino* heldPiece = nullptr; // Currently held piece (can be null)
    bool canHold = true;            // Flag to prevent multiple holds
    int score;                      // Current game score
    bool paused = false;            // Game pause state
    bool quit = false;              // Game quit flag

    // Calculate how far the current piece can drop
    int calculateHardDropPosition() {
        int dropDistance = 0;
        while (isValidMove(currentPiece.x, currentPiece.y + dropDistance + 1)) {
            dropDistance++;
        }
        return dropDistance;
    }

    // Check if a move is valid (no collisions)
    bool isValidMove(int newX, int newY) {
        for (int i = 0; i < BLOCK_SIZE; i++) {
            for (int j = 0; j < BLOCK_SIZE; j++) {
                if (currentPiece.shape[i][j]) {
                    int gridX = newX + j;
                    int gridY = newY + i;
                    // Check boundaries and collisions
                    if (gridX < 0 || gridX >= WIDTH || gridY >= HEIGHT || 
                        (gridY >= 0 && grid[gridY][gridX]))
                        return false;
                }
            }
        }
        return true;
    }

    // Merge the current piece into the game grid
    void mergePiece() {
        for (int i = 0; i < BLOCK_SIZE; i++)
            for (int j = 0; j < BLOCK_SIZE; j++)
                if (currentPiece.shape[i][j]) {
                    grid[currentPiece.y + i][currentPiece.x + j] = 1;
                    colorGrid[currentPiece.y + i][currentPiece.x + j] = currentPiece.color;
                }
    }

    // Check for and clear completed lines
    void clearLines() {
        for (int i = HEIGHT - 1; i >= 0; i--) {
            bool full = true;
            // Check if line is full
            for (int j = 0; j < WIDTH; j++) {
                if (!grid[i][j]) {
                    full = false;
                    break;
                }
            }
            // If line is full, remove it and add empty line at top
            if (full) {
                grid.erase(grid.begin() + i);
                grid.insert(grid.begin(), vector<int>(WIDTH, 0));
                colorGrid.erase(colorGrid.begin() + i);
                colorGrid.insert(colorGrid.begin(), vector<int>(WIDTH, 0));
                score += 100; // Increase score
            }
        }
    }

    // Generate next pieces if queue is running low
    void generateNextPieces() {
        while (nextPieces.size() < 3) {
            nextPieces.push_back(Tetromino());
        }
    }

    // Hold or swap the current piece
    void holdCurrentPiece() {
        if (!canHold) return;
        
        if (heldPiece == nullptr) {
            // If nothing is held, store current piece and get new one
            heldPiece = new Tetromino(currentPiece);
            getNewPiece();
        } else {
            // Swap current piece with held piece
            Tetromino temp = currentPiece;
            currentPiece = *heldPiece;
            *heldPiece = temp;
            // Reset position of swapped-in piece
            currentPiece.x = WIDTH / 2 - BLOCK_SIZE / 2;
            currentPiece.y = 0;
        }
        canHold = false; // Prevent holding again until next piece
    }

    // Get a new piece from the queue
    void getNewPiece() {
        if (nextPieces.empty()) {
            // If queue is empty, generate new piece
            currentPiece = Tetromino();
            generateNextPieces();
        } else {
            // Take first piece from queue
            currentPiece = nextPieces[0];
            nextPieces.erase(nextPieces.begin());
            generateNextPieces();
        }
        // Reset position to top center
        currentPiece.x = WIDTH / 2 - BLOCK_SIZE / 2;
        currentPiece.y = 0;
        canHold = true; // Allow holding again
    }

    // Draw game instructions on screen
    void drawInstructions() {
        gotoxy(WIDTH + 5, 5);
        printw("HOW TO PLAY:");
        gotoxy(WIDTH + 5, 6);
        printw("------------");
        gotoxy(WIDTH + 5, 7);
        printw("a - Move Left");
        gotoxy(WIDTH + 5, 8);
        printw("d - Move Right");
        gotoxy(WIDTH + 5, 9);
        printw("s - Soft Drop");
        gotoxy(WIDTH + 5, 10);
        printw("w - Rotate");
        gotoxy(WIDTH + 5, 11);
        printw("h - Hard Drop");
        gotoxy(WIDTH + 5, 12);
        printw("p - Pause");
        gotoxy(WIDTH + 5, 13);
        printw("g - Hold Piece");
        gotoxy(WIDTH + 5, 14);
        printw("q - Quit Game");
        gotoxy(WIDTH + 5, 16);
        printw("OBJECTIVE:");
        gotoxy(WIDTH + 5, 17);
        printw("Complete lines to");
        gotoxy(WIDTH + 5, 18);
        printw("score points!");
    }

public:
    // Constructor - initializes game state
    TetrisGame() : grid(HEIGHT, vector<int>(WIDTH, 0)), 
                  colorGrid(HEIGHT, vector<int>(WIDTH, 0)), score(0) {
        srand(time(0)); // Seed random number generator
        setlocale(LC_ALL, ""); // Set locale for Unicode support
        
        // Initialize ncurses
        initscr();     // Initialize screen
        cbreak();      // Disable line buffering
        noecho();      // Don't echo input
        keypad(stdscr, TRUE); // Enable special keys
        curs_set(0);   // Hide cursor
        timeout(200);  // Non-blocking input with timeout
        initColors();   // Initialize colors

        // Calculate center position based on terminal size
        int maxY, maxX;
        getmaxyx(stdscr, maxY, maxX);
        centerX = (maxX - WIDTH) / 2 - 5;
        centerY = (maxY - HEIGHT) / 2 - 5;

        // Display controls at start
        gotoxy(-10, -8);
        printw("Controls:");
        gotoxy(-10, -7);
        printw("a - Left");
        gotoxy(-10, -6);
        printw("d - Right");
        gotoxy(-10, -5);
        printw("s - Down");
        gotoxy(-10, -4);
        printw("w - Rotate");
        gotoxy(-10, -3);
        printw("h - Hard Drop");
        gotoxy(-10, -2);
        printw("p - Pause");
        gotoxy(-10, -1);
        printw("g - Hold");
        gotoxy(-10, 0);
        printw("q - Quit");
        gotoxy(-10, 2);
        printw("Press any key...");
        refresh();
        getch(); // Wait for key press
        generateNextPieces();
        clear(); // Clear screen before game starts
    }

    // Destructor - clean up resources
    ~TetrisGame() {
        if (heldPiece) delete heldPiece; // Free held piece memory
        endwin(); // Clean up ncurses
    }

    // Draw the current game state
    void draw() {
        clear(); // Clear screen

        // Create temporary grids that include the current piece
        vector<vector<int>> tempGrid = grid;
        vector<vector<int>> tempColorGrid = colorGrid;
        for (int i = 0; i < BLOCK_SIZE; i++)
            for (int j = 0; j < BLOCK_SIZE; j++)
                if (currentPiece.shape[i][j] && currentPiece.y + i >= 0) {
                    tempGrid[currentPiece.y + i][currentPiece.x + j] = 1;
                    tempColorGrid[currentPiece.y + i][currentPiece.x + j] = currentPiece.color;
                }

        // Draw next pieces preview
        gotoxy(WIDTH + 5, -8);
        printw("Next Pieces:");
        for (int i = 0; i < 4; i++) {
            gotoxy(WIDTH + 5, -7 + i);
            for (int p = 0; p < 3; p++) {
                if (p < nextPieces.size()) {
                    for (int j = 0; j < 4; j++) {
                        if (nextPieces[p].shape[i][j]) {
                            printBlock(nextPieces[p].color);
                        } else {
                            printw(" ");
                        }
                    }
                    printw("  ");
                }
            }
        }

        // Draw score
        gotoxy(-10, -9);
        printw("Score: %d", score);

        // Draw held piece
        gotoxy(-10, 5);
        printw("Held:");
        if (heldPiece != nullptr) {
            for (int i = 0; i < 4; i++) {
                gotoxy(-10, 6 + i);
                for (int j = 0; j < 4; j++) {
                    if (heldPiece->shape[i][j]) {
                        printBlock(heldPiece->color);
                    } else {
                        printw(" ");
                    }
                }
            }
        }

        // Draw game boundary
        if (has_colors()) {
            attron(COLOR_PAIR(BOUNDARY));
        }
        gotoxy(-1, -1);
        printw("+");
        for (int j = 0; j < WIDTH; j++) printw("-");
        printw("+");
        if (has_colors()) {
            attroff(COLOR_PAIR(BOUNDARY));
        }

        // Draw game grid
        for (int i = 0; i < HEIGHT; i++) {
            if (has_colors()) {
                attron(COLOR_PAIR(BOUNDARY));
            }
            gotoxy(-1, i);
            printw("|");
            if (has_colors()) {
                attroff(COLOR_PAIR(BOUNDARY));
            }
            for (int j = 0; j < WIDTH; j++) {
                if (tempGrid[i][j]) {
                    printBlock(tempColorGrid[i][j]); // Draw filled block
                } else {
                    printw("."); // Draw empty space
                }
            }
            if (has_colors()) {
                attron(COLOR_PAIR(BOUNDARY));
            }
            printw("|");
            if (has_colors()) {
                attroff(COLOR_PAIR(BOUNDARY));
            }
        }

        // Draw bottom boundary
        if (has_colors()) {
            attron(COLOR_PAIR(BOUNDARY));
        }
        gotoxy(-1, HEIGHT);
        printw("+");
        for (int j = 0; j < WIDTH; j++) printw("-");
        printw("+");
        if (has_colors()) {
            attroff(COLOR_PAIR(BOUNDARY));
        }

        drawInstructions(); // Draw game instructions

        // Draw pause message if game is paused
        if (paused) {
            gotoxy(WIDTH/2 - 3, HEIGHT/2 - 1);
            printw("Paused");
            gotoxy(WIDTH/2 - 8, HEIGHT/2);
            printw("Press 'r' to Resume");
        }

        refresh(); // Update screen
    }

    // Handle user input
    void handleInput() {
        int ch = getch();
        if (ch != ERR) {
            int newX = currentPiece.x, newY = currentPiece.y;
            switch (ch) {
                case 'a': newX--; break; // Move left
                case 'd': newX++; break; // Move right
                case 's': newY++; break; // Move down (soft drop)
                case 'w': { // Rotate
                    Tetromino temp = currentPiece;
                    temp.rotate();
                    if (isValidMove(temp.x, temp.y)) {
                        currentPiece.rotate();
                    }
                    break;
                }
                case 'h': { // Hard drop
                    int dropDistance = calculateHardDropPosition();
                    currentPiece.y += dropDistance;
                    mergePiece();
                    clearLines();
                    getNewPiece();
                    break;
                }
                case 'p': paused = true; break; // Pause game
                case 'r': paused = false; break; // Resume game
                case 'g': holdCurrentPiece(); break; // Hold piece
                case 'q': quit = true; break; // Quit game
            }
            // If not paused and not hard drop, update position if valid
            if (!paused && ch != 'h' && isValidMove(newX, newY)) {
                currentPiece.x = newX;
                currentPiece.y = newY;
            }
        }
    }

    // Update game state
    bool update() {
        if (quit) return false; // Exit if quit flag is set
        if (paused) return true; // Skip update if paused
        
        // Try to move piece down
        if (isValidMove(currentPiece.x, currentPiece.y + 1)) {
            currentPiece.y++;
        } else {
            // If can't move down, merge piece and check for game over
            mergePiece();
            clearLines();
            getNewPiece();
            if (!isValidMove(currentPiece.x, currentPiece.y)) return false;
        }
        return true;
    }

    // Main game loop
    void run() {
        while (update()) {
            draw();
            handleInput();
        }
        // Game over sequence
        if (!quit) {
            draw();
            gotoxy(WIDTH/2 - 5, HEIGHT + 2);
            printw("Game Over!");
            gotoxy(WIDTH/2 - 5, HEIGHT + 3);
            printw("Score: %d", score);
            gotoxy(WIDTH/2 - 10, HEIGHT + 5);
            printw("Press any key to exit...");
            refresh();
            getch();
        }
    }
};

int main() {
    TetrisGame game; // Create game instance
    game.run();      // Run the game
    return 0;
}
