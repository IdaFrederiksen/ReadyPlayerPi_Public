// ServerApp/ServerController.h — Top-level Control class
//
// ECB role: CONTROL. The main system loop for the server CPU.
//
// Collaborators (per class diagram):
//   - ServerSocket «boundary»  — network transport
//   - Scoreboard   «boundary»  — scoreboard file I/O
//   - Room         «domain»    — lobby state (one per pending room)
//   - GameRoom     «control»   — forked-child orchestrator for running games
//
// Communication rules (Applikationsmodeller Part 3, p.3–4):
//   * Control -> Boundary  : OK   (ServerSocket, Scoreboard)
//   * Control -> Domain    : OK   (Room)
//   * Control -> Control   : OK   (GameRoom)
//   * No Boundary -> Boundary   : ServerSocket never calls Scoreboard.

#pragma once
#include <iostream>
#include <string>
#include <map>
#include <cstdlib>
#include <ctime>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <nlohmann/json.hpp>

#include "ServerSocket.h"   // class ServerSocket
#include "Scoreboard.h"
#include "Room.h"
#include "GameRoom.h"

using json = nlohmann::json;

// ── SIGCHLD handler — reaps zombie game-room child processes ──────────────────
// Kept as a free function because POSIX signal handlers can't be non-static
// methods. One line; wrapping it in a class would not improve clarity.
inline void serverSigchldHandler(int) {
    while (waitpid(-1, nullptr, WNOHANG) > 0);
}

class ServerController {
public:
    explicit ServerController(int port = 9000);

    // Main system loop for the server. Returns the process exit code.
    int run();

private:
    // ── Signal setup ──────────────────────────────────────────────────────────
    void installSigchldHandler();

    // ── Dispatch for fresh connections ────────────────────────────────────────
    void handleIncomingConnection();

    // ── UC2 Host Room (server side) ───────────────────────────────────────────
    void handleHostRoom(int clientSocket, const json &message);

    // ── UC2 Join Room (server side) ───────────────────────────────────────────
    void handleJoinRoom(int clientSocket, const json &message);

    // ── GET_SCOREBOARD ───────────────────────────────────────────────────────
    void handleGetScoreboard(int clientSocket, const json &message);

    // ── START_GAME → fork + GameRoom ──────────────────────────────────────────
    void handleStartGame(int                                  hostSocket,
                         const std::string                   &roomCode,
                         const json                          &message,
                         std::map<int, std::string>::iterator &pendingEntry);

    // ── Host disconnected before start_game ───────────────────────────────────
    void handleHostDisconnect(int                                  hostSocket,
                              const std::string                   &roomCode,
                              std::map<int, std::string>::iterator &pendingEntry);

    // ── State ─────────────────────────────────────────────────────────────────
    int                               port_;
    ServerSocket                      socket_;                 // «boundary»
    std::map<std::string, Room>       rooms_;                  // «domain» (lobby entities)
    std::map<int, std::string>        hostPendingMap_;         // hostSocket → roomCode
};
