# 🐍 Terminal Snake Online

### A retro terminal-based game engine with online multiplayer.
Built in C using a custom Finite State Machine and JSON API synchronization.

---

## 🕹️ Game Modes

* **Single Player:** Classic Snake-game on your own terminal!
* **Local Multiplayer:** 1v1 on a single keyboard (WIP).
* **Online Host/Join:** Same 1v1 but online. Create or join rooms using a 6-digit session ID (WIP).
* **Starvation Royale:** Battle-royale style survival (WIP).

---

## 🚀 Quick Start

### 1. Requirements

You need the `jansson` library for JSON handling.

* **Debian/Ubuntu/WSL:**  
  `sudo apt install libjansson-dev`

* **macOS:**  
  `brew install jansson`

### 2. Compilation

Compilation is not nessecary, I release the updates with pre-compiled 

### 3. Execution
The game is provided with a Make-file, placed in `/Snake/C/Makefile`. It includes functions, such as:
```bash
make all //compile the project inside of the folder where Makefile is placed
make run //run the game
make clean //delete all compiled files
```

## 🛠️ Technical Overview
The game logic is separated from the rendering and networking layers to ensure smooth performance.

## Component	Responsibility
`GameLogic.c`	Snake movement, collision logic, and grid rendering
`MultiplayerApi.c`	Communicates with the mpapi.se server via JSON
`main.c`	Manages the State Machine and global application timing
`Highscore System`	Persistent `.txt` file storage for different modes

## ⌨️ Key Bindings
Movement:
`W, A, S, D`

## System Commands:
`R` - Restart the game
`M` - Return to Main Menu
`Q` - Exit Game

## 📡 Networking Details
The game synchronizes state by serializing player data into JSON objects.
A typical network tick looks like this:

```json
{
  "player": "client_id",
  "x": 12,
  "y": 34,
  "score": 5
}
```
### Project maintained by TheRedGuyLmao(Ruslan Bilonozhko, SUVX25).
