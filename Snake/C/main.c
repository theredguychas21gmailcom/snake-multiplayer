#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "libs/jansson/jansson.h"
#include "libs/MultiplayerApi.h"
#include "libs/GameLogic.h"

// -------------------------------
// Main
// -------------------------------

char currentSessionId[64] = {0};

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
    
    // SAVE THE SESSION ID so we can draw it later
    if(session) {
        snprintf(currentSessionId, sizeof(currentSessionId), "%s", session);
    }

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

    printf("\033[2J"); 

    MultiplayerApi* api = mp_api_create("mpapi.se", 9001, "94929f46-845c-4832-9564-5cbef51c68df");
    if (!api) {
        return 1;
    }

    int listener_id = mp_api_listen(api, on_multiplayer_event, NULL);
	int menu_needs_redraw = 1;

    json_t *gameData = json_object();
    json_object_set_new(gameData, "x", json_integer(12));
    json_object_set_new(gameData, "y", json_integer(34));
    json_object_set_new(gameData, "dir", json_string("right"));
    
    GameState last_active_mode = STATE_SINGLEPLAYER;

    // --- MAIN PROGRAM LOOP ---
    while (1) {
        switch (current_state) {
            case STATE_MENU:
    		if (menu_needs_redraw) {
        		drawMenu();
        	menu_needs_redraw = 0; // Stop drawing until something changes
    		}
    		pollMenuInput(); 
    		usleep(50000); 
    		break;

            case STATE_SINGLEPLAYER:
                last_active_mode = STATE_SINGLEPLAYER;
                runSinglePlayerGameTick(api, gameData);
                usleep(100000);
                break;

            case STATE_MULTIPLAYER_LOCAL:
                last_active_mode = STATE_MULTIPLAYER_LOCAL;
                // Placeholder for local MP tick
                usleep(100000);
                break;

            case STATE_MULTIPLAYER_HOST:
                if (main_host(api) == 0) {
                    current_state = STATE_MULTIPLAYER_ONLINE;
                } else {
                    current_state = STATE_MENU;
                }
                break;

            case STATE_MULTIPLAYER_ONLINE:
                usleep(100000);
                break;

            case STATE_STARVATION_ROYALE:
                printf("\033[H Mode coming soon! Press M for menu.\n");
                char c_wait;
                if (read(STDIN_FILENO, &c_wait, 1) == 1 && (c_wait == 'm' || c_wait == 'M')) {
                    current_state = STATE_MENU;
                }
                usleep(100000);
                break;

            case STATE_GAME_OVER: {
                static int has_saved = 0;
                if (!has_saved) {
                    check_and_save_highscore(last_active_mode, snake_length - 3);
                    has_saved = 1;
                }

                int best = get_highscore(last_active_mode);
                printf("\033[H"); // Move cursor to top
                printf("==========================================\n");
                printf("                GAME OVER!                \n");
                printf("==========================================\n");
                printf(" Mode: %s\n", get_highscore_filename(last_active_mode));
                printf(" Score: %d\n", snake_length - 3);
                printf(" BEST SCORE: %d\n", best);
                printf("==========================================\n");
                printf(" [R] Try Again   [M] Menu   [Q] Quit      \n");
                
                char c_go = 0;
                if (read(STDIN_FILENO, &c_go, 1) == 1) {
                    if (c_go == 'r' || c_go == 'm') {
                        has_saved = 0; 
                        printf("\033[2J");
                        if (c_go == 'r') {
                            game_restart();
                            current_state = last_active_mode;
                        } else {
                            current_state = STATE_MENU;
                        }
                    } else if (c_go == 'q') {
                        goto cleanup;
                    }
                }
                usleep(50000);
                break;
            }

            default:
                current_state = STATE_MENU;
                break;
        } // End of switch
    } // End of while

cleanup:
    json_decref(gameData);
    mp_api_unlisten(api, listener_id);
    mp_api_destroy(api);

    return 0;
}

