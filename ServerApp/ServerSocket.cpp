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


#include <string>
#include <map>
#include <algorithm>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <nlohmann/json.hpp>

#include "ServerSocket.h"

#include "../Shared/JSONCommunication.h"

using json = nlohmann::json;



    // socket() + bind() + listen() on the given port. Returns true on success.
    bool ServerSocket::startListening(int port, int backlog) {
        listenSocket_ = socket(AF_INET, SOCK_STREAM, 0);
        int reuseOption = 1;
        setsockopt(listenSocket_, SOL_SOCKET, SO_REUSEADDR,
                   &reuseOption, sizeof(reuseOption));

        sockaddr_in listenAddress{};
        listenAddress.sin_family      = AF_INET;
        listenAddress.sin_addr.s_addr = INADDR_ANY;
        listenAddress.sin_port        = htons(port);

        if (bind(listenSocket_,
                 (sockaddr *)&listenAddress,
                 sizeof(listenAddress)) < 0) {
            perror("bind");
            return false;
        }
        listen(listenSocket_, backlog);
        return true;
    }

    // Block on select() over the listen socket + all host-pending sockets.
    // Outputs:
    //   listenReadable  = true if a new connection is waiting
    //   hostReadable    = subset of hostPendingMap keys whose fd has data
    // Returns false on select() error.
    bool ServerSocket::waitForActivity(const std::map<int, std::string> &hostPendingMap,
                         bool                              &listenReadable,
                         std::map<int, std::string>        &hostReadable) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(listenSocket_, &readSet);
        int maxSocketFd = listenSocket_;

        for (auto &[hostSocket, roomCode] : hostPendingMap) {
            FD_SET(hostSocket, &readSet);
            maxSocketFd = std::max(maxSocketFd, hostSocket);
        }

        if (select(maxSocketFd + 1, &readSet, nullptr, nullptr, nullptr) < 0) {
            perror("select");
            return false;
        }

        listenReadable = FD_ISSET(listenSocket_, &readSet);
        hostReadable.clear();
        for (auto &[hostSocket, roomCode] : hostPendingMap) {
            if (FD_ISSET(hostSocket, &readSet))
                hostReadable[hostSocket] = roomCode;
        }
        return true;
    }

    // Accept one new client connection. Returns the client fd, or -1 on error.
    int ServerSocket::acceptClient() {
        sockaddr_in clientAddress{};
        socklen_t   addressLength = sizeof(clientAddress);
        return accept(listenSocket_,
                      (sockaddr *)&clientAddress,
                      &addressLength);
    }

    // Send a framed JSON event to the given client fd.
    bool ServerSocket::sendEvent(int clientFd, const json &message) {
        return sendJson(clientFd, message);
    }

    // Read one framed JSON command from the given client fd.
    // Returns {} on disconnect, error or parse failure.
    json ServerSocket::receiveCommand(int clientFd) {
        return receiveJson(clientFd);
    }

    // Close a client connection.
    void ServerSocket::closeClient(int clientFd) {
        if (clientFd >= 0) close(clientFd);
    }

    // Close the listen socket (call once, before child fork()s).
    void ServerSocket::closeListenSocket() {
        if (listenSocket_ >= 0) {
            close(listenSocket_);
            listenSocket_ = -1;
        }
    }

    int ServerSocket::getListenSocket() const { return listenSocket_; }
