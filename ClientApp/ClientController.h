#pragma once
#include <string>
#include <nlohmann/json.hpp>

#include "gui/QtGui.h"
#include "ClientSocket.h"
#include "CredentialsFile.h"

using json = nlohmann::json;

class ClientController {
    public:
    int run();
    explicit ClientController(const char *serverIp = "127.0.0.1", int port = 9000);

    private:
    std::string hostFlow();
    bool guestFlow();
    bool awaitGameStart();
    bool runGameSession();

    // ── State ─────────────────────────────────────────────────────────────────
    const char  *serverIp_;
    int          port_;
    QtGUI        gui_;        // «boundary» — all Player I/O (Qt6)
    ClientSocket socket_;     // «boundary» — TCP transport
    std::string  username_;
    CredentialsFile credentials_;
    std::string  logTag_;
    std::string  role_;
    std::string  partner_;
    std::string  gameName_;

    // Sættes når en lobby-skærm lukkes via ✖ mens controlleren blokerer på
    // socket-recv. QtGUI::cancelRequested → vi disconnect'er socketten;
    // det blokerende receive returnerer tomt og lobby-flowet falder tilbage
    // til main menu.
    bool cancelRequested_ = false;
};