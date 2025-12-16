#ifndef GAMELOGIC_H
#define GAMELOGIC_H

#include <termios.h>

#include "MultiplayerApi.h"

// --- 1. Constants and Enums ---

#define WIDTH 40
#define HEIGHT 20
#define MAX_LEN 200

typedef enum {
    STATE_MENU,
    STATE_SINGLEPLAYER,
    STATE_MULTIPLAYER_LOCAL, //Local multiplayer using different keys.
    STATE_MULTIPLAYER_ONLINE, //Online Multiplayer (WIP)
    STATE_MULTIPLAYER_HOST, //(WIP)
    STATE_MULTIPLAYER_JOIN, //(WIP)
    STATE_GAME_OVER
} GameState;

typedef struct {
    int x, y;
} Segment;

// --- 2. Global Variables (External Declarations) ---

extern GameState current_state;
extern Segment snake[MAX_LEN];
extern int snake_length;
extern int dirX;
extern int dirY;
extern int foodX;
extern int foodY;
extern char retry;
extern struct termios orig; // Used by Terminal Handling functions

// --- 3. Function Prototypes (Declarations) ---

// -------------------------------
// Terminal Handling
// -------------------------------

void enableRawMode();
void disableRawMode();

// -------------------------------
// Game Logic
// -------------------------------

void spawnFood();
void moveSnake();
void game_restart();
int checkCollision();

// -------------------------------
// Inmatning/Input
// -------------------------------

void pollMenuInput();
void pollSinglePlayerInput();

// -------------------------------
// Draw Functions
// -------------------------------

void drawMenu();
void draw();

// -------------------------------
// Game Loop Tick
// -------------------------------

void runSinglePlayerGameTick(MultiplayerApi* api, json_t* gameData);

#endif //GAMELOGIC_H