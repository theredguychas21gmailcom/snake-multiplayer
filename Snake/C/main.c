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

