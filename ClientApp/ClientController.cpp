// ClientApp/ClientController.h — Top-level Control class
//
// ECB role: CONTROL. The main system loop for the client CPU.
//
// Collaborators (per class diagram):
//   - GUI          «boundary» — all Player I/O (prompts, status messages)
//   - ClientSocket «boundary» — TCP transport (socket / send / recv)
//   - GameSession  «control»  — abstract base for UC3/UC4 (preserved)
//   - GameFactory              — helper that picks the right GameSession
//
// Every Player interaction
// goes through the GUI member. Every socket operation goes through
// ClientSocket.

#include <string>
#include <memory>
#include <nlohmann/json.hpp>

#include "ClientController.h"

#include "ClientSocket.h"
#include "gui/QtGui.h"
#include "GameSession.h"
#include "GameFactory.h"
#include "../Shared/JSONCommunication.h"

#include <QEventLoop>
#include <QMetaObject>
#include <thread>

using json = nlohmann::json;

// Modtag én event fra socket uden at fryse Qt's event-loop. Socket-recv
// kører i baggrunds-tråd; main-tråden kører QEventLoop og processerer
// paint/timer/click-events indtil data er læst. Bruges hvor receive kan
// vente længe (host venter på guest, both venter på game_start).
static json receiveEventPumpingGui(ClientSocket &socket) {
    QEventLoop loop;
    json result;

    std::thread receiver([&]() {
        result = socket.receiveEvent();
        QMetaObject::invokeMethod(&loop, "quit", Qt::QueuedConnection);
    });

    loop.exec();
    receiver.join();
    return result;
}


    ClientController::ClientController(const char *serverIp, int port)
        : serverIp_(serverIp), port_(port), credentials_() {}

    // Main system loop for the client. Returns the process exit code.
    int ClientController::run() {
        // ── Step 1: setup ─────────────────────────────────────────────────────
        {
            std::string prefill = credentials_.exists() ? credentials_.load() : "";
            username_ = gui_.promptUsername(prefill);
            if (username_.empty()) return 0; // bruger lukkede ✖ på username-skærm
            credentials_.save(username_);
        }

        // QtGUI signalerer cancellation når en lobby-skærm lukkes via ✖.
        // Vi lukker socketten så det blokerende receive vågner og lobby-flowet
        // falder tilbage til main-menu.
        QObject::connect(&gui_, &QtGUI::cancelRequested, &gui_, [this]() {
            cancelRequested_ = true;
            socket_.disconnect();
        });

        // ── Outer loop: tilbage til main-menu efter hver session/cancel ──────
        while (true) {
            cancelRequested_ = false;
            int choice = gui_.promptMainMenu();
            if (choice <= 0) break; // 0 = ✖ på MainWindow → afslut app

            // ── Step 2: connect ───────────────────────────────────────────
            gui_.showConnecting(serverIp_, port_);
            if (!socket_.connectToServer(serverIp_, port_)) {
                gui_.showError("Kunne ikke forbinde til server");
                continue;
            }
            gui_.showConnected();

            // ── Step 3: lobby flow (UC1 or UC2) ───────────────────────────
            bool isHost = (choice == 1);
            bool lobbyOk = false;
            if (isHost) {
                logTag_ = "host";
                lobbyOk = !hostFlow().empty();
            } else {
                logTag_ = "guest";
                lobbyOk = guestFlow();
            }
            if (!lobbyOk) {
                socket_.disconnect();
                continue; // tilbage til main-menu (cancel eller fejl)
            }

            // ── Steps 4 + 5: game loop — await start, run game, play again ─
            while (true) {
                if (!awaitGameStart()) break;
                bool playAgainHandled = runGameSession();

                if (playAgainHandled) {
                    // GyroPi: either player clicked PLAY AGAIN and sent start_game,
                    // or the other player's scoreboard auto-closed because game_start
                    // arrived. Either way, game_start is waiting in the socket buffer —
                    // just loop back to awaitGameStart().
                    continue;
                }

                // NotePi (or BACK clicked in GyroPi): use the host-driven mechanism.
                if (isHost) {
                    int gameChoice = gui_.promptPlayAgain();
                    if (gameChoice == 0) break;

                    std::string newGame = (gameChoice == 2) ? "NOTEPI" : "GYROPI";
                    socket_.sendCommand({{"command", "start_game"},
                                         {"data",    {{"game", newGame}}}});
                }
                // Guest: loop back and wait for next game_start from server
            }

            socket_.disconnect();
            // Tilbage til outer loop → promptMainMenu vises igen
        }

        gui_.showDisconnected();
        return 0;
    }

    // ── UC1 Host Room ─────────────────────────────────────────────────────────
    // Returns the chosen game name ("GYROPI" / "NOTEPI"), or "" on failure/cancel.
    std::string ClientController::hostFlow() {
        socket_.sendCommand({{"command", "host_room"},
                             {"data",    {{"username", username_}}}});

        json serverReply = socket_.receiveEvent();
        if (serverReply.value("event", "") != "room_created") {
            gui_.showRoomCreationFailed();
            return "";
        }

        std::string roomCode = serverReply["data"]["roomCode"];
        gui_.showRoomCreated(roomCode);
        gui_.showWaitingForGuest();

        // Block until the guest connects and the server confirms both players.
        // Receive på baggrund-tråd så hostRoom_-vinduet kan tegnes & opdateres
        // mens vi venter (ellers er det blankt indtil guest joiner). Hvis bruger
        // lukker ✖ → QtGUI::cancelRequested → socket lukkes → recv returnerer tomt.
        json playerJoinedMessage = receiveEventPumpingGui(socket_);
        if (cancelRequested_ || playerJoinedMessage.empty()) return "";
        if (playerJoinedMessage.value("event", "") == "player_joined") {
            std::vector<std::string> players;
            for (auto &player : playerJoinedMessage["data"]["players"])
                players.push_back(player.get<std::string>());
            gui_.showPlayersJoined(players);
        }

        int gameChoice = gui_.promptGameChoice();
        if (cancelRequested_ || gameChoice <= 0) return ""; // bruger annullerede
        std::string gameName = (gameChoice == 2) ? "NOTEPI" : "GYROPI";
        gui_.showStartingGame(gameName);

        socket_.sendCommand({{"command", "start_game"},
                             {"data",    {{"roomCode", roomCode},
                                          {"game",     gameName}}}});
        return gameName;
    }

    // ── UC2 Join Room ─────────────────────────────────────────────────────────
    bool ClientController::guestFlow() {
        std::string roomCode = gui_.promptRoomCode();
        if (roomCode.empty()) return false; // ✖ på JoinGameRoom

        socket_.sendCommand({{"command", "join_room"},
                             {"data",    {{"username", username_},
                                          {"roomCode", roomCode}}}});

        json serverReply = socket_.receiveEvent();
        if (serverReply.value("event", "") == "error") {
            gui_.showError(serverReply["data"]["detail"]);
            return false;
        }
        if (serverReply.value("event", "") == "player_joined") {
            std::vector<std::string> players;
            for (auto &player : serverReply["data"]["players"])
                players.push_back(player.get<std::string>());
            gui_.showJoinedRoom(roomCode, players);
        }
        gui_.showWaitingForHost();
        return true;
    }

    // ── Await game_start event from the server ────────────────────────────────
    bool ClientController::awaitGameStart() {
        // Begge clients venter her — host efter han trykker start, guest fra
        // han er joinet. Pump GUI-events så lobby-widget'er ikke fryser.
        json gameStartMessage = receiveEventPumpingGui(socket_);
        if (cancelRequested_ || gameStartMessage.empty() ||
            gameStartMessage.value("event", "") != "game_start") {
            if (!cancelRequested_) gui_.showGameStartFailed();
            return false;
        }

        role_     = gameStartMessage["data"]["role"];
        partner_  = gameStartMessage["data"]["partner"];
        gameName_ = gameStartMessage["data"]["game"];

        gui_.showGameStartHeader(gameName_, role_, partner_);
        return true;
    }

    // ── Hand off to the matching GameSession (UC3/UC4) ────────────────────────
    bool ClientController::runGameSession() {
        std::unique_ptr<GameSession> gameSession = createGameSession(gameName_);

        // Vis rolle-skærm før gameplay 
        if (gameName_ == "NOTEPI") {
            gui_.showNotePiRole(role_, logTag_);
        } else if (gameName_ == "GYROPI") {
            gui_.showGyroPiRole(role_, partner_, logTag_);
        }

        // GameSession subclasses still take a raw fd. See ClientSocket.h note.
        if (role_ == "PERFORMER")
            return gameSession->playAsPerformer   (socket_.getSocketFd(), logTag_, gameName_);
        else
            return gameSession->playAsCommunicator(socket_.getSocketFd(), logTag_, gameName_);
    }



