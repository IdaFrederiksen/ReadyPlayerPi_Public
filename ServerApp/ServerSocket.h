// ServerApp/ServerSocket.h — Pure TCP transport («boundary»)
//
// ECB role: BOUNDARY. The only class on the server that touches sockets.
//
// What's in this class:
//   - socket() + bind() + listen()         (startListening)
//   - accept()                             (acceptClient)
//   - select() over listen + host sockets  (waitForActivity)
//   - sendEvent() / receiveCommand()       (thin wrappers over JSON protocol)
//   - closeClient() / closeListenSocket()  (fd management)


#pragma once
#include <string>
#include <map>
#include <algorithm>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <nlohmann/json.hpp>

#include "../Shared/JSONCommunication.h"

using json = nlohmann::json;

class ServerSocket {
public:
    ServerSocket() = default;
    ~ServerSocket() { closeListenSocket(); }

    // Non-copyable (owns the listen socket)
    ServerSocket(const ServerSocket &)            = delete;
    ServerSocket &operator=(const ServerSocket &) = delete;

    // socket() + bind() + listen() on the given port. Returns true on success.
    bool startListening(int port, int backlog = 10);

    // Block on select() over the listen socket + all host-pending sockets.
    // Outputs:
    //   listenReadable  = true if a new connection is waiting
    //   hostReadable    = subset of hostPendingMap keys whose fd has data
    // Returns false on select() error.
    bool waitForActivity(const std::map<int, std::string> &hostPendingMap,
                         bool                              &listenReadable,
                         std::map<int, std::string>        &hostReadable);

    // Accept one new client connection. Returns the client fd, or -1 on error.
    int acceptClient();

    // Send a framed JSON event to the given client fd.
    bool sendEvent(int clientFd, const json &message);

    // Read one framed JSON command from the given client fd.
    // Returns {} on disconnect, error or parse failure.
    json receiveCommand(int clientFd);

    // Close a client connection.
    void closeClient(int clientFd);

    // Close the listen socket (call once, before child fork()s).
    void closeListenSocket();

    int getListenSocket() const;

private:
    int listenSocket_ = -1;
};
