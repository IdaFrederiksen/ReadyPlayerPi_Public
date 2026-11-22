// ClientApp/ClientSocket.h — Pure TCP transport («boundary»)
//
// ECB role: BOUNDARY. The only class on the client that touches the socket.
//
// Header: class declaration only. Implementation in ClientSocket.cpp.

#pragma once
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ClientSocket {
public:
    ClientSocket();
    ~ClientSocket();

    // Non-copyable (owns a file descriptor)
    ClientSocket(const ClientSocket &)            = delete;
    ClientSocket &operator=(const ClientSocket &) = delete;

    // Open a TCP connection to serverIp:port. Returns true on success.
    bool connectToServer(const char *serverIp, int port);

    // Send one framed JSON command to the server.
    bool sendCommand(const json &message);

    // Block until one framed JSON event arrives. Returns {} on disconnect.
    json receiveEvent();

    // Close the connection if open.
    void disconnect();

    // Raw fd accessor. Used by GameSession subclasses (UC3/UC4) only.
    int  getSocketFd() const;
    bool isConnected() const;

private:
    int socketFd_ = -1;
};
