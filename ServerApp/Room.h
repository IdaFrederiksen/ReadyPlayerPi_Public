// ServerApp/Room.h — Lobby room record («domain»)
//
// ECB role: DOMAIN. The lobby's in-memory record of a pending room before
//                    the game starts.
//
// Holds the room code and, as players join, the sockets and usernames of
// host and guest. The ServerController owns a std::map<roomCode, Room>
// containing every pending room.
//
// Not to be confused with the GameRoom «control» class, which orchestrates
// what happens inside a running game after fork(). This class is pure data.


#pragma once
#include <string>

struct Room {
    std::string code;
    int         hostSocket  = -1;
    int         guestSocket = -1;
    std::string hostName;
    std::string guestName;
};
