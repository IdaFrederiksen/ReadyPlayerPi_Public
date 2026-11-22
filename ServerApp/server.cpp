// ServerApp/server.cpp — Ready Player Pi Server (entry point)
//
// Compile: make server
//
// All orchestration lives in ServerController. This file is only the entry
// point — keeping main() out of every application-model sequence diagram.

#include "ServerController.h"

int main() {
    ServerController server(9000);
    return server.run();
}
