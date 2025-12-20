#include "GameLogic.h" // Must include its own header
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
// NOTE: You must also include the headers for MultiplayerApi and jansson if needed for runSinglePlayerGameTick

// ---------------------------------------------------------
// --- 1. Global Variable Definitions (WITHOUT 'extern') ---
// ---------------------------------------------------------
GameState current_state = STATE_MENU; 
Segment snake[MAX_LEN];
int snake_length = 3;
int dirX = 1;
int dirY = 0;
int foodX;
int foodY;
char retry;
struct termios orig;

// ----------------------------------------
// --- 2. Terminal Handling Definitions ---
// ----------------------------------------

void enableRawMode()  {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig);
    raw = orig;
    raw.c_lflag &= ~(ICANON | ECHO);     // inga enter-krav, ingen echo
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    // gör stdin icke-blockerande
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    printf("\033[?25l");
}

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
    printf("\033[?25l");
}

// ---------------------------------
// --- 3. Game Logic Definitions ---
// ---------------------------------

void spawnFood() {
    foodX = rand() % WIDTH;
    foodY = rand() % HEIGHT;
}

// Flytta ormen
void moveSnake() {
    // flytta kroppen
    for (int i = snake_length - 1; i > 0; i--) {
        snake[i] = snake[i - 1];
    }

    // nytt huvud
    snake[0].x += dirX;
    snake[0].y += dirY;
}

void game_restart() {
    // Reset snake state
    snake_length = 3;
    dirX = 1;
    dirY = 0;
    
    // Reset snake position
    snake[0] = (Segment){ WIDTH / 2, HEIGHT / 2 };
    snake[1] = (Segment){ WIDTH / 2 - 1, HEIGHT / 2 };
    snake[2] = (Segment){ WIDTH / 2 - 2, HEIGHT / 2 };
    
    // Spawn new food
    spawnFood();
}

// Returnerar 1 om kollision inträffat
int checkCollision() {
    int x = snake[0].x;
    int y = snake[0].y;

    // väggar
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
        return 1;

    // egen kropp
    for (int i = 1; i < snake_length; i++) {
        if (snake[i].x == x && snake[i].y == y)
            return 1;
    }
    return 0;
}

// --- 4. Input Definitions ---
void pollSinglePlayerInput() {
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == 'w' && dirY != 1)  { dirX = 0; dirY = -1; }
        if (c == 's' && dirY != -1) { dirX = 0; dirY = 1; }
        if (c == 'a' && dirX != 1)  { dirX = -1; dirY = 0; }
        if (c == 'd' && dirX != -1) { dirX = 1; dirY = 0; }
        
        if (c == 'r') {
            game_restart();
            printf("\033[2J"); 
        }
        if (c == 'q') exit(0);
    }
}

void pollMenuInput() {
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == '1') {
            current_state = STATE_SINGLEPLAYER;
            printf("\033[2J");
            game_restart();
        } else if (c == '2') {
            current_state = STATE_MULTIPLAYER_LOCAL;
            printf("\033[2J");
            game_restart();
        } else if (c == '3') {
            // Map 3 to Multiplayer
            current_state = STATE_MULTIPLAYER_ONLINE;
            printf("\033[2J");
        } else if (c == '4') {
             // Map 4 to ROYALE Mode 
             current_state = STATE_STARVATION_ROYALE;
        } else if (c == 'q' || c == 'Q') {
            exit(0);
        }
    }
}

// ------------------------------
// --- 5. Drawing Definitions ---
// ------------------------------

void drawMenu() {
    printf("\033[H"); // Clear screen and move cursor to 1,1
    
    printf("==========================================\n");
    printf("|          SNAKE TERMINAL GAME           |\n");
    printf("==========================================\n\n");

    printf("Choose Mode:\n");
    printf("  1. Single Player (Best: %d)\n", get_highscore(STATE_SINGLEPLAYER));
    printf("  2. Local Multiplayer (Best: %d)\n", get_highscore(STATE_MULTIPLAYER_LOCAL));
    printf("  3. Online Multiplayer (Coming Soon)\n");
    printf("  4. Starvation Royale Mode (Won Games: %d)\n", get_highscore(STATE_STARVATION_ROYALE));
    printf("  Q. Quit\n\n");

    printf("Enter your choice (1, 2, 3, or Q): ");
}

// Rita spelplanen
void draw() {
    printf("\033[H"); // flytta markören till 1,1

    // Övre ram
    for (int x = 0; x < WIDTH + 2; x++)
        printf("-");
    printf("\n");

    // Spelplan + sidramar
    for (int y = 0; y < HEIGHT; y++) {
        printf("|"); // vänster ram

        for (int x = 0; x < WIDTH; x++) {

            // Mat
            if (x == foodX && y == foodY) {
                printf("Ó");
                continue;
            }

            // Orm
            int printed = 0;
            for (int i = 0; i < snake_length; i++) {
                if (snake[i].x == x && snake[i].y == y) {
                    printf(i == 0 ? "@" : "#");
                    printed = 1;
                    break;
                }
            }

            if (!printed) printf(" ");
        }

        printf("|\n"); // höger ram
    }

    // Nedre ram
    for (int x = 0; x < WIDTH + 2; x++)
        printf("-");
    printf("\n");

    printf("Score: %d\n", snake_length - 3);
    printf("Move: WASD\n");
    printf("Restart: R, Exit: Q.\n");
}

// -------------------------------------
// --- 6. Game Loop Tick Definitions ---
// -------------------------------------

// Main logic for the single-player game tick
void runSinglePlayerGameTick(MultiplayerApi* api, json_t* gameData) {
    pollSinglePlayerInput();
    moveSnake();

    // Check collision and handle Game Over
    if (checkCollision()) {
        current_state = STATE_GAME_OVER; // Change state to Game Over
        return; // Exit the game tick immediately
    }

    // Check for food consumption
    if (snake[0].x == foodX && snake[0].y == foodY) {
        if (snake_length < MAX_LEN)
            snake_length++;
        spawnFood();
    }

    draw();
    
    // Multiplayer logic is still called, even in SP mode (which may be odd)
    // You should probably remove the multiplayer API calls if not hosting/joining.
    /*
    int rc = mp_api_game(api, gameData);
    printf("Skickade game-data, rc=%d\n", rc);
    */
}

// ---------------------------
// --- 7. Highscore system --- 
// ---------------------------

const char* get_highscore_filename(GameState mode) {
    switch(mode) {
        case STATE_SINGLEPLAYER:       return "high_single.txt";
        case STATE_MULTIPLAYER_LOCAL:  return "high_local.txt";
        case STATE_MULTIPLAYER_ONLINE: return "high_online.txt";
        case STATE_STARVATION_ROYALE:  return "high_royale.txt";
        default: return "high_general.txt";
    }
}

int get_highscore(GameState mode) {
    int score = 0;
    FILE *f = fopen(get_highscore_filename(mode), "r");
    if (f) {
        if (fscanf(f, "%d", &score) != 1) score = 0;
        fclose(f);
    }
    return score;
}

void check_and_save_highscore(GameState mode, int current_score) {
    int best = get_highscore(mode);
    
    if (current_score > best) {
        FILE *f = fopen(get_highscore_filename(mode), "w");
        if (f) {
            fprintf(f, "%d", current_score);
            fclose(f);
        }
    }
}