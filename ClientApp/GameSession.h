// ClientApp/GameSession.h — Abstract base class for client-side game logic
//
// Mirrors ServerApp/Game.h on the client side.
// Each concrete game knows how to behave as PERFORMER and COMMUNICATOR.
// client.cpp calls the factory in GameFactory.h and dispatches here —
// adding a new game requires only a new subclass and one line in the factory.

#pragma once
#include <string>

class GameSession {
public:
    // Returns true  = play same game again (start_game already sent or game_start received).
    // Returns false = back / use old mechanism.
    virtual bool playAsPerformer   (int socketFileDescriptor, const std::string &logTag,
                                    const std::string &gameName) = 0;
    virtual bool playAsCommunicator(int socketFileDescriptor, const std::string &logTag,
                                    const std::string &gameName) = 0;
    virtual ~GameSession() = default;
};
