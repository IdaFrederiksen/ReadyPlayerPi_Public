// ServerApp/Game.h — Abstract base class for all mini-games
// Each game receives two sockets (performer + communicator) and a player-names
// string, then runs until the game ends (goal reached or disconnect).

#pragma once
#include <string>

class Game {
public:
    virtual void run(int         performerSocket,
                     int         communicatorSocket,
                     const std::string &playerNames) = 0;
    virtual ~Game() = default;
};
