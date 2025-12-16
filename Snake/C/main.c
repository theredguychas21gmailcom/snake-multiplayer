#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "libs/jansson/jansson.h"
#include "libs/MultiplayerApi.h"


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

GameState current_state = STATE_MENU; // Start the game in the menu

typedef struct {
    int x, y;
} Segment;

Segment snake[MAX_LEN];
int snake_length = 3;

int dirX = 1;
int dirY = 0;

int foodX;
int foodY;

char retry;

struct termios orig;

// -------------------------------
// Terminalhantering
// -------------------------------
void enableRawMode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig);
    raw = orig;
    raw.c_lflag &= ~(ICANON | ECHO);     // inga enter-krav, ingen echo
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    // gör stdin icke-blockerande
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
}

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
}

// -------------------------------
// Spel-logik
// -------------------------------

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

// -------------------------------
// Ritfunktioner
// -------------------------------

void drawMenu() {
    printf("\033[H\033[2J"); // Clear screen and move cursor to 1,1
    
    printf("==========================================\n");
    printf("|          SNAKE TERMINAL GAME           |\n");
    printf("==========================================\n\n");

    printf("Choose Mode:\n");
    printf("  1. Single Player\n");
    printf("  2. Local Multiplayer (Coming Soon)\n");
    printf("  3. Online Multiplayer (Coming Soon)\n");
    printf("  Q. Quit\n\n");

    printf("Enter your choice (1, 2, 3, or Q): ");
}

// -------------------------------
// Inmatning/Input
// -------------------------------

void pollMenuInput() {
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == '1') {
            current_state = STATE_SINGLEPLAYER;
            printf("\033[2J"); // Clear screen for game start
            game_restart(); // Initialize game state for SP
        } else if (c == '2') {
            current_state = STATE_MULTIPLAYER_LOCAL;
            printf("\033[2J");
            game_restart(); // Initialize for MP
        } else if (c == '3') {
            // Placeholder: maybe connect to online server later
            printf("\033[2J");
            current_state = STATE_MULTIPLAYER_ONLINE;
        } else if (c == 'q' || c == 'Q') {
            exit(0);
        }
    }
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

// -------------------------------
// Main
// -------------------------------

static void on_multiplayer_event(
    const char *event,
    int64_t messageId,
    const char *clientId,
    json_t *data,
    void *user_data)
{
	
	char* strData = NULL;
	
	if(data)
		strData = json_dumps(data, JSON_INDENT(2));

	printf("Multiplayer event: %s (msgId: %lld, clientId: %s)\n", event, (long long)messageId, clientId ? clientId : "null");
	if (strData)
	{
		printf("Data: %s\n", strData);
	}

    /* event: "joined", "leaved" (om servern skickar det), eller "game" */
    if (strcmp(event, "joined") == 0) {

	} else if (strcmp(event, "leaved") == 0) {

    } else if (strcmp(event, "game") == 0) {
        
    }

	free(strData);

    /* data är ett json_t* (object); anropa json_incref(data) om du vill spara det efter callbacken */
}

int main_host(MultiplayerApi* api)
{
	char *session = NULL;
	char *clientId = NULL;
	json_t *hostData = NULL;

	int rc = mp_api_host(api, &session, &clientId, &hostData);
	if (rc != MP_API_OK) {
		printf("Kunde inte skapa session: %d\n", rc);
		return -1;
	}

	printf("Du hostar session: %s (clientId: %s)\n", session, clientId);
	/* hostData kan innehålla extra data från servern (oftast tomt objekt) */
	if (hostData) json_decref(hostData);
	free(session);
	free(clientId);

	return 0;
}

/*int main_list(MultiplayerApi* api)
{
	printf("Hämtar lista över publika sessioner...\n");

	json_t *sessionList = NULL;
	int rc = mp_api_list(api, &sessionList);
	if (rc != MP_API_OK) {
		printf("Kunde inte hämta session-lista: %d\n", rc);
		return -1;
	}

	

	if (json_array_size(sessionList) == 0) {
		printf("Inga publika sessioner tillgängliga.\n");

	} else{
		printf("Totalt %zu sessioner.\n", json_array_size(sessionList));

		size_t index;
		json_t *value;
		printf("Tillgängliga publika sessioner:\n");
		json_array_foreach(sessionList, index, value) {
			json_t *sess_val = json_object_get(value, "id");
			if (json_is_string(sess_val)) {
				const char *sessionId = json_string_value(sess_val);
				printf(" - %s\n", sessionId);
			}
		}

		printf("\n");
	}


	json_decref(sessionList);
	return 0;
}*/

int main_join(MultiplayerApi* api, const char* sessionId)
{
	char *joinedSession = NULL;
	char *joinedClientId = NULL;
	json_t *joinPayload = json_object();          /* t.ex. namn, färg osv. */
	json_object_set_new(joinPayload, "name", json_string("Spelare 1"));

	json_t *joinData = NULL;
	int rc = mp_api_join(api, sessionId, joinPayload, &joinedSession, &joinedClientId, &joinData);

	json_decref(joinPayload);  /* vår lokala payload */

	if (rc == MP_API_OK) {
		printf("Ansluten till session: %s (clientId: %s)\n", joinedSession, joinedClientId);
		/* joinData kan innehålla status eller annan info */
		if (joinData) json_decref(joinData);
		free(joinedSession);
		free(joinedClientId);
	} else if (rc == MP_API_ERR_REJECTED) {
		/* t.ex. ogiltigt sessions‑ID, läs ev. joinData för mer info om du valde att ta emot det */
	} else {
		/* nätverksfel/protokollfel etc. */
	}

	return 0;
}

int main() {
    srand(time(NULL));

    enableRawMode();
    atexit(disableRawMode);

    printf("\033[2J"); // rensa skärmen

	MultiplayerApi* api = mp_api_create("kontoret.onvo.se", 9001);
	if (!api)
	{
		/* felhantering */
		return 1;
	}

	main_host(api);
	//main_join(api, "HU2J7D");



	int listener_id = mp_api_listen(api, on_multiplayer_event, NULL);

	json_t *gameData = json_object();
	json_object_set_new(gameData, "x", json_integer(12));
	json_object_set_new(gameData, "y", json_integer(34));
	json_object_set_new(gameData, "dir", json_string("right"));
	
    // --- MAIN PROGRAM LOOP ---
    while (1) {
        switch (current_state) {
            case STATE_MENU:
                drawMenu();
                pollMenuInput();
                usleep(50000); 
                break;

            case STATE_SINGLEPLAYER:
                runSinglePlayerGameTick(api, gameData);
                usleep(100000); // Game tick-rate
                break;

            // ...
            case STATE_MULTIPLAYER_LOCAL:
                // TODO: Implement local multiplayer tick (using P2 keys)
                usleep(100000);
                break;
            // ... and the new HOST and JOIN cases

            case STATE_MULTIPLAYER_ONLINE:
                // TODO: Implement online multiplayer tick
                usleep(100000);
                break;

            // --- ADDED CASES TO HANDLE THE NEW ENUM VALUES ---
            case STATE_MULTIPLAYER_HOST:
                // Placeholder for hosting logic
                usleep(100000);
                break;

            case STATE_MULTIPLAYER_JOIN:
                // Placeholder for joining logic
                usleep(100000);
                break;
            // ----------------------------------------------------
            
            case STATE_GAME_OVER:
                // Game Over logic (Waiting for R or Q)
                printf("\033[HGame Over! Score: %d.\nTo try again press R, to return to Menu press M, else Q.\n", snake_length - 3);
                
                char c = 0;
                while (c != 'r' && c != 'q' && c != 'm') {
                    if (read(STDIN_FILENO, &c, 1) == 1) {
                        if (c == 'r') {
                            game_restart();
                            printf("\033[2J"); 
                            current_state = STATE_SINGLEPLAYER; // Restart instantly
                            break;
                        }
                        else if (c == 'm') {
                            printf("\033[2J");
                            current_state = STATE_MENU; // Go back to main menu
                            break;
                        }
                        else if(c == 'q') {
                            goto cleanup;
                        }
                    }
                    usleep(10000); 
                }
                break;
        }
    }

    cleanup:
            json_decref(gameData);
            mp_api_unlisten(api, listener_id);
            mp_api_destroy(api);

    return 0;
}

