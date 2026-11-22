// ClientApp/GyroPi/GyroPiSession.h — GyroPi client-side game logic (declaration)
//
// PERFORMER:    sender thread replays live IMU updates
//               receiver thread routes server events to the GyroPiPerformerWindow
// COMMUNICATOR: receiver thread routes course_info / ball_update / blackhole_hit
//               to the GyroPiCommunicatorWindow
//
// Implementation lives in GyroPiSession.cpp so Qt headers don't leak into every
// includer (GameFactory.h includes this).

#pragma once
#include <string>

#include "../GameSession.h"

class GyroPiSession : public GameSession {
public:
    bool playAsPerformer   (int socketFileDescriptor, const std::string &logTag,
                            const std::string &gameName) override;
    bool playAsCommunicator(int socketFileDescriptor, const std::string &logTag,
                            const std::string &gameName) override;
};
