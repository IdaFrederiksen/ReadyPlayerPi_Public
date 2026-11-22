// ServerApp/GameRoom.h — Game room lifecycle
//
// Runs inside the child process after fork().
// Responsibilities:
//   1. Assign PERFORMER / COMMUNICATOR roles randomly
//   2. Send game_start to both players
//   3. Instantiate the correct Game subclass (factory)
//   4. Delegate to game->run() — blocks until the game ends
//   5. Close sockets and return (child process then calls _exit)

#pragma once
#include <iostream>
#include <string>
#include <memory>
#include <nlohmann/json.hpp>

#include "../Shared/JSONCommunication.h"
#include "Game.h"
#include "GyroPi/GyroPiGame.h"
#include "NotePi/NotePiGame.h"

using json = nlohmann::json;

class GameRoom {
public:
    GameRoom(int hostSocket, int guestSocket,
             const std::string &hostName, const std::string &guestName,
             const std::string &gameName);

    void run();

private:
    // Sockets and player identities (owned for the lifetime of this object)
    int         hostSocket_;
    int         guestSocket_;
    std::string hostName_;
    std::string guestName_;
    std::string gameName_;

    // Set by assignRoles()
    int         performerSocket_    = -1;
    int         communicatorSocket_ = -1;
    std::string performerName_;
    std::string communicatorName_;

    void assignRoles();
    void sendGameStart() const;
    std::unique_ptr<Game> createGame() const;
};
