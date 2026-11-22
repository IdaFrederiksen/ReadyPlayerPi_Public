// Shared/Player.h — Player domain entity
// Holds identity and assigned role for a connected player

#pragma once
#include <string>

struct Player {
    std::string username;
    std::string role;    // "PERFORMER" or "COMMUNICATOR"
    std::string partner; // partner's username
};
