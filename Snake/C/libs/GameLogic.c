#include "GameLogic.h" // Must include its own header
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
// NOTE: You must also include the headers for MultiplayerApi and jansson if needed for runSinglePlayerGameTick

// ---------------------------------------------------------
// --- 1. Global Variable Definitions (WITHOUT 'extern') ---
// ---------------------------------------------------------
int foodX_array[MAX_FOOD];
int foodY_array[MAX_FOOD];
int active_food_count = 0;

int is_host = 0;
int active_players = 1;

GameState current_state = STATE_MENU; 
Segment snake[MAX_LEN];
int snake_length = 3;
int dirX = 1;
int dirY = 0;
int foodX;
int foodY;
Segment snake2[MAX_LEN];
int snake2_length = 3;
int dirX2 = -1; // Starts moving left
int dirY2 = 0;

char retry;
struct termios orig;

int royale_tick_counter = 0;
int currentWidth = WIDTH;   // Default 40
int currentHeight = HEIGHT; // Default 20

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
    // Use currentWidth/Height instead of static WIDTH/HEIGHT
    foodX = rand() % currentWidth;
    foodY = rand() % currentHeight;

    // IMPORTANT: Sync the array that draw() actually looks at
    foodX_array[0] = foodX;
    foodY_array[0] = foodY;
    active_food_count = 1; 
}

void spawnRoyaleFood() {
    // 10 players = 5 food; 5 players = 2 food
    int amount_to_spawn = active_players / 2;
    if (amount_to_spawn < 1) amount_to_spawn = 1;

    // Use a loop to spawn into food array
    for (int i = 0; i < amount_to_spawn; i++) {
        foodX_array[i] = rand() % currentWidth;
        foodY_array[i] = rand() % currentHeight;
    }
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

    // Reset snake2 state
    snake2_length = 3;
    dirX2 = -1; 
    dirY2 = 0;

    //Reset snake2 position
    snake2[0] = (Segment){ WIDTH - 6, HEIGHT / 2 };
    snake2[1] = (Segment){ WIDTH - 5, HEIGHT / 2 };
    snake2[2] = (Segment){ WIDTH - 4, HEIGHT / 2 };
    
    // Spawn new food
    spawnFood();
}

// Returnerar 1 om kollision inträffat
int checkCollision() {
    int x = snake[0].x;
    int y = snake[0].y;

    // Use current bounds instead of hardcoded constants
    if (x < 0 || x >= currentWidth || y < 0 || y >= currentHeight)
        return 1;

    for (int i = 1; i < snake_length; i++) {
        if (snake[i].x == x && snake[i].y == y)
            return 1;
    }
    return 0;
}   

// Return 0: No collision, 1: P1 Died, 2: P2 Died, 3: Both Died
int checkMultiplayerCollision() {
    // Check P1 against dynamic walls
    if (snake[0].x < 0 || snake[0].x >= currentWidth || snake[0].y < 0 || snake[0].y >= currentHeight) return 1;
    for (int i = 1; i < snake_length; i++) if (snake[i].x == snake[0].x && snake[i].y == snake[0].y) return 1;

    // Check P2 against dynamic walls
    if (snake2[0].x < 0 || snake2[0].x >= currentWidth || snake2[0].y < 0 || snake2[0].y >= currentHeight) return 2;
    for (int i = 1; i < snake2_length; i++) if (snake2[i].x == snake2[0].x && snake2[i].y == snake2[0].y) return 2;

    // Check P1 Head into P2 Body
    for (int i = 0; i < snake2_length; i++) if (snake[0].x == snake2[i].x && snake[0].y == snake2[i].y) return 1;
    
    // Check P2 Head into P1 Body
    for (int i = 0; i < snake_length; i++) if (snake2[0].x == snake[i].x && snake2[0].y == snake[i].y) return 2;

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

void pollLocalMultiplayerInput() {
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1) {
        // Player 1 (WASD)
        if (c == 'w' && dirY != 1)  { dirX = 0; dirY = -1; }
        if (c == 's' && dirY != -1) { dirX = 0; dirY = 1; }
        if (c == 'a' && dirX != 1)  { dirX = -1; dirY = 0; }
        if (c == 'd' && dirX != -1) { dirX = 1; dirY = 0; }

        // Player 2 (IJKL - cleaner for terminal than escaped arrow keys)
        if (c == 'i' && dirY2 != 1)  { dirX2 = 0; dirY2 = -1; }
        if (c == 'k' && dirY2 != -1) { dirX2 = 0; dirY2 = 1; }
        if (c == 'j' && dirX2 != 1)  { dirX2 = -1; dirY2 = 0; }
        if (c == 'l' && dirX2 != -1) { dirX2 = 1; dirY2 = 0; }
        
        if (c == 'q') exit(0);
    }
}

void runStarvationRoyaleTick(MultiplayerApi* api) {
    pollSinglePlayerInput();
    moveSnake();
    
    royale_tick_counter++;
    if (royale_tick_counter >= 50) {
        if (snake_length > 2) snake_length--; 
        royale_tick_counter = 0;
    }

    if (checkCollision()) {
        current_state = STATE_GAME_OVER;
    }

    for (int i = 0; i < active_food_count; i++) {
        if (snake[0].x == foodX_array[i] && snake[0].y == foodY_array[i]) {
            snake_length += 2;
            // Respawn just this one piece of food
            foodX_array[i] = rand() % currentWidth;
            foodY_array[i] = rand() % currentHeight;
        }
    }

    // Networking: You'll need to pack and mp_api_game here just like in STATE_MULTIPLAYER_ONLINE
    draw();
}

void pollMenuInput() {
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == '1' || c == '2') {
            // Set the state based on choice
            current_state = (c == '1') ? STATE_SINGLEPLAYER : STATE_MULTIPLAYER_LOCAL;
            
            // --- THE "PRESS ENTER" FIX ---
            printf("\033[2J\033[H");
            printf("Mode: %s Selected!\nPress [ENTER] to start the game...\n", (c == '1') ? "Single Player" : "Local 1v1");
            
            char wait_c;
            // Loop until Enter is pressed
            while (read(STDIN_FILENO, &wait_c, 1) != 1 || wait_c != '\n');
            
            game_restart();
            printf("\033[2J");
        } 
        else if (c == '3') {
            current_state = STATE_MULTIPLAYER_HOST;
            printf("\033[2J");
        } 
        else if (c == '4') {
            current_state = STATE_MULTIPLAYER_JOIN;
        } 
        else if (c == '5') {
            // --- ROYALE INITIALIZATION ---
            current_state = STATE_STARVATION_ROYALE;
            printf("\033[2J\033[H");
            printf("=== STARVATION ROYALE LOBBY ===\n");
            printf("Waiting for timer (1 minute)...\n");
            // You can add your 60s timer logic here or in main.c
        } 
        else if (c == 'q' || c == 'Q') {
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
    printf(" 1. Single Player (Best: %d)\n", get_highscore(STATE_SINGLEPLAYER));
    printf(" 2. Local Multiplayer (Best P1: %d, Best P2: )\n", get_highscore(STATE_MULTIPLAYER_LOCAL));
    printf(" 3. Host Online Game\n");
    printf(" 4. Join Online Game\n");
    printf(" 5. Starvation Royale\n");
    printf(" Q. Quit\n\n");
    printf("Enter your choice (1, 2, 3, or Q): ");
    fflush(stdout);
}

void updateArenaSize(int players) {
    // Example: 20x20 base + extra space per player
    currentWidth = 20 + (players * 2);
    currentHeight = 20 + (players * 2);
    
    // Cap it so it doesn't outgrow the terminal
    if (currentWidth > 80) currentWidth = 80;
    if (currentHeight > 40) currentHeight = 40;
}

void draw() {
    printf("\033[H"); 

    for (int x = 0; x < currentWidth + 2; x++) printf("-");
    printf("\n");

    for (int y = 0; y < currentHeight; y++) {
        printf("|"); 

        for (int x = 0; x < currentWidth; x++) { 
            
            int isFood = 0;
            for (int f = 0; f < active_food_count; f++) {
                if (x == foodX_array[f] && y == foodY_array[f]) {
                    printf("Ó");
                    isFood = 1;
                    break;
                }
            }
            if (isFood) continue;

            int printed = 0;
            // Draw Player 1
            for (int i = 0; i < snake_length; i++) {
                if (snake[i].x == x && snake[i].y == y) {
                    printf(i == 0 ? "@" : "#");
                    printed = 1;
                    break;
                }
            }

            // Draw Player 2 / Online Opponent
            if (!printed && (current_state == STATE_MULTIPLAYER_LOCAL || 
                             current_state == STATE_MULTIPLAYER_ONLINE ||
                             current_state == STATE_ROYALE_SPECTATOR)) {
                for (int i = 0; i < snake2_length; i++) {
                    if (snake2[i].x == x && snake2[i].y == y) {
                        printf(i == 0 ? "8" : "%%");
                        printed = 1;
                        break;
                    }
                }
            }

            if (!printed) printf(" ");
        } 
        printf("|\n"); 
    }

    // Lower border
    for (int x = 0; x < currentWidth + 2; x++)
        printf("-");
    printf("\n");

    // Dynamic Status Line
    if (current_state == STATE_SINGLEPLAYER) {
        printf("Score: %d | Best: %d\n", snake_length - 3, get_highscore(STATE_SINGLEPLAYER));
        fflush(stdout);
    } else if (current_state == STATE_MULTIPLAYER_ONLINE) {
        printf("YOU (@): %d | OPPONENT (8): %d [%s]\n", 
               snake_length - 3, snake2_length - 3, is_host ? "HOST" : "GUEST");
               fflush(stdout);
    } else {
        printf("P1: %d | P2: %d\n", snake_length - 3, snake2_length - 3);
        fflush(stdout);
    }
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