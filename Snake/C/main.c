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
    void *user_data) {

		char* strData = NULL;
	
		if(data)
			strData = json_dumps(data, JSON_INDENT(2));

	printf("Multiplayer event: %s (msgId: %lld, clientId: %s)\n", event, (long long)messageId, clientId ? clientId : "null");
		if (strData) {
			printf("Data: %s\n", strData);
		}

		if (strcmp(event, "game") == 0) {
        	// 1. Sync Snake
        	json_t *body = json_object_get(data, "body");
        		if (json_is_array(body)) {
            		snake2_length = json_array_size(body);
            		for (size_t i = 0; i < snake2_length && i < MAX_LEN; i++) {
                		json_t *seg = json_array_get(body, i);
                		snake2[i].x = json_integer_value(json_object_get(seg, "x"));
                		snake2[i].y = json_integer_value(json_object_get(seg, "y"));
            		}
        		}

        		// 2. Sync Map Size (Royale)
        		json_t *w = json_object_get(data, "w");
        		json_t *h = json_object_get(data, "h");
        		if (w && h) {
            		currentWidth = json_integer_value(w);
            		currentHeight = json_integer_value(h);
        		}

        		// 3. Sync Food (Single OR Array for Royale)
				if (!is_host) {
	    			json_t *foods = json_object_get(data, "foods"); // Look for the array
	    			if (json_is_array(foods)) {
	        			active_food_count = json_array_size(foods);
	        			for (size_t i = 0; i < active_food_count && i < 50; i++) {
	            			json_t *f = json_array_get(foods, i);
	            			foodX_array[i] = json_integer_value(json_object_get(f, "x"));
	            			foodY_array[i] = json_integer_value(json_object_get(f, "y"));
	        			}
	    			} else {
	        			// Fallback for standard 1v1 mode
	        			json_t *fx = json_object_get(data, "fx");
	        			json_t *fy = json_object_get(data, "fy");
	        			if (fx && fy) {
	            			foodX = json_integer_value(fx);
	            			foodY = json_integer_value(fy);
	            			foodX_array[0] = foodX; 
	            			foodY_array[0] = foodY; 
	            			active_food_count = 1;
	        			}
	    			}
				}
			}
    		if (strData)
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

	if (rc == MP_API_OK) {
        is_host = 1; // Mark as host
        printf("Du hostar session: %s\n", session);
    }  
  
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
		is_host = 0;
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
    			pollLocalMultiplayerInput();
    
    			// Move both
    			for (int i = snake_length - 1; i > 0; i--) snake[i] = snake[i - 1];
    			snake[0].x += dirX; snake[0].y += dirY;
    
    			for (int i = snake2_length - 1; i > 0; i--) snake2[i] = snake2[i - 1];
    			snake2[0].x += dirX2; snake2[0].y += dirY2;

    			int result = checkMultiplayerCollision();
    			if (result > 0) {
    			    current_state = STATE_GAME_OVER;
    			}

    			// Food check for both...
    			if (snake[0].x == foodX && snake[0].y == foodY) { snake_length++; spawnFood(); }
    			if (snake2[0].x == foodX && snake2[0].y == foodY) { snake2_length++; spawnFood(); }

    			draw(); // Make sure draw() is updated to loop through snake2 as well!
    			usleep(100000);
    		break;

			// --- JOIN STATE (Restored) ---
            case STATE_MULTIPLAYER_JOIN: {
                char joinCode[64];
                disableRawMode(); // Let user see what they type
                printf("\033[2J\033[H");
                printf("Enter Room Code to Join: ");
                if (scanf("%63s", joinCode) == 1) {
                    printf("Joining %s...\n", joinCode);
                    main_join(api, joinCode);
                    current_state = STATE_MULTIPLAYER_ONLINE;
                    game_restart();
                } else {
                    current_state = STATE_MENU;
                }
                enableRawMode();
            } break;

            case STATE_MULTIPLAYER_HOST:
                if (strcmp(currentSessionId, "") == 0) {
                    if (main_host(api) == 0) {
                        printf("\033[2J\033[H");
                        printf("Session Created! Room Code: %s\n", currentSessionId);
                        printf("Waiting for opponent...\n");
                    } else {
                        current_state = STATE_MENU;
                    }
                }
                if (active_players >= 2) {
                    current_state = STATE_MULTIPLAYER_ONLINE;
                    game_restart();
                }
                usleep(100000); 
            break;

            case STATE_MULTIPLAYER_ONLINE: 
                last_active_mode = STATE_MULTIPLAYER_ONLINE;

                pollSinglePlayerInput(); 
                moveSnake(); 

                if (checkCollision()) {
                    current_state = STATE_GAME_OVER;
                }
            
                if (is_host) {
                    if (snake[0].x == foodX && snake[0].y == foodY) {
                        snake_length++;
                        spawnFood(); 
                    }
                }
            
                // --- PACKING DATA ---
                json_t *syncData = json_object();
                json_t *body = json_array();
                for (int i = 0; i < snake_length; i++) {
                    json_t *seg = json_object();
                    json_object_set_new(seg, "x", json_integer(snake[i].x));
                    json_object_set_new(seg, "y", json_integer(snake[i].y));
                    json_array_append_new(body, seg);
                }
                json_object_set_new(syncData, "body", body);
            
                if (is_host) {
                    json_object_set_new(syncData, "fx", json_integer(foodX));
                    json_object_set_new(syncData, "fy", json_integer(foodY));
                }
            
                mp_api_game(api, syncData);
                json_decref(syncData); 
            
                draw(); 
                usleep(100000); 
            break;

			case STATE_STARVATION_ROYALE: 
			    static time_t lobby_start = 0;
			    static int game_started = 0;
			    static int initial_players = 0;

			    if (!game_started) {
			        if (lobby_start == 0) lobby_start = time(NULL);
			        int countdown = 60 - (int)(time(NULL) - lobby_start);
				
			        printf("\033[H\033[2J=== LOBBY ===\nPlayers: %d\nStarts in: %d\n", active_players, countdown);
				
			        if (countdown <= 0) {
			            game_started = 1;
			            initial_players = active_players;
			            game_restart();
			            updateArenaSize(active_players); // 20x20 for 10 players, etc.
			        }
			        usleep(500000);
			    } else {
			        pollSinglePlayerInput();
			        moveSnake();
				
			        // SHRINK LOGIC: If half the players are gone, shrink map 1.5x
			        if (active_players <= initial_players / 2) {
			            currentWidth /= 1.5;
			            currentHeight /= 1.5;
			            // Ensure we don't shrink to 0
			            if (currentWidth < 5) currentWidth = 5; 
			            if (currentHeight < 5) currentHeight = 5;
			        }
				
			        // Pack data for others
			        json_t *syncData = json_object();
			        json_object_set_new(syncData, "w", json_integer(currentWidth));
			        json_object_set_new(syncData, "h", json_integer(currentHeight));


			        mp_api_game(api, syncData);
			        json_decref(syncData);
				
			        if (checkCollision()) {
			            current_state = STATE_ROYALE_SPECTATOR; 
			        }
				
			        draw(); 
			        usleep(100000);
			    }
			break;
			
			case STATE_ROYALE_SPECTATOR:
			    draw();
			    printf("\n[ SPECTATING ] - %d Players remaining.\n", active_players);
			    printf("Press M for Menu\n");
			    char c_spec;
			    if (read(STDIN_FILENO, &c_spec, 1) == 1 && (c_spec == 'm' || c_spec == 'M')) {
			        current_state = STATE_MENU;
			    }
			    usleep(100000);
			break;

            case STATE_GAME_OVER: 
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

