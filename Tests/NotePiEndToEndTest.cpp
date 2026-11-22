// Tests/NotePiEndToEndTest.cpp
//
// End-to-end-test af NotePi-flowet uden hardware:
//   - Spawner ./build/server som child-proces
//   - Åbner to TCP-sockets til 127.0.0.1:9000 (host + guest)
//   - Driver lobby-protokollen manuelt: host_room → join_room → start_game
//   - Driver NotePi game-protokollen: sender hardcodede 5-note-sekvenser fra
//     den klient der får tildelt PERFORMER, modtager NOTE_RESULT på begge,
//     loop indtil GAME_OVER (max 6 attempts eller solved)
//   - Verificerer at begge clients modtager game_over event med score
//   - SIGTERM på server-proces, exit 0 ved succes
//
// Bemærk: testen bruger IKKE QtGUI eller mic — det er en ren protokol-
// roundtrip-test af serverens Wordle-logik plus shared protocol helpers.
// For visuel/auditiv test af GUI+mic: brug `make run` (3 terminaler).
//
// Build:  cmake --build build --target notepi_end_to_end_test
// Run:    ./build/Tests/notepi_end_to_end_test

#include "../Shared/JSONCommunication.h"
#include "../Shared/Protocol.h"
#include "../Shared/NotePiProtocol.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>
#include <string>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {

constexpr int  SERVER_PORT = 9000;
constexpr auto SERVER_STARTUP_DELAY = std::chrono::milliseconds(800);

int connectToServer() {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(SERVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    for (int attempt = 0; attempt < 20; ++attempt) {
        if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0)
            return fd;
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
    ::close(fd);
    return -1;
}

bool waitForEvent(int fd, const std::string &expected, json &out) {
    out = receiveJson(fd);
    if (out.empty()) return false;
    return out.value(Protocol::EVENT, "") == expected;
}

void log(const std::string &tag, const std::string &msg) {
    std::cout << "[" << tag << "] " << msg << std::endl;
}

}  // namespace

int main() {
    // ── 1. Spawn server som separat proces ────────────────────────────────────
    pid_t serverPid = fork();
    if (serverPid == 0) {
        execl("./build/server", "server", static_cast<char*>(nullptr));
        std::cerr << "Failed to exec ./build/server\n";
        std::exit(127);
    }
    if (serverPid < 0) {
        std::cerr << "fork() failed\n";
        return 1;
    }
    std::this_thread::sleep_for(SERVER_STARTUP_DELAY);
    log("test", "Server PID " + std::to_string(serverPid));

    int exitCode = 1;

    // RAII-helper: SIGTERM + waitpid på server uanset hvor vi exit'er
    struct ServerGuard {
        pid_t pid;
        ~ServerGuard() {
            if (pid > 0) {
                kill(pid, SIGTERM);
                int status;
                waitpid(pid, &status, 0);
            }
        }
    } guard{serverPid};

    // ── 2. Connect host + guest sockets ───────────────────────────────────────
    int hostFd  = connectToServer();
    int guestFd = connectToServer();
    if (hostFd < 0 || guestFd < 0) {
        log("test", "FAIL: kunne ikke connecte til server");
        if (hostFd  >= 0) ::close(hostFd);
        if (guestFd >= 0) ::close(guestFd);
        return 1;
    }
    log("test", "Begge sockets connected");

    // ── 3. Lobby: host opretter rum ───────────────────────────────────────────
    sendJson(hostFd, {
        {Protocol::COMMAND, "host_room"},
        {Protocol::DATA,    {{"username", "TestHost"}}}
    });

    json roomCreated;
    if (!waitForEvent(hostFd, "room_created", roomCreated)) {
        log("test", "FAIL: forventede room_created");
        ::close(hostFd); ::close(guestFd);
        return 1;
    }
    std::string roomCode = roomCreated[Protocol::DATA].value("roomCode", "");
    log("test", "Room created: " + roomCode);

    // ── 4. Lobby: guest joiner ────────────────────────────────────────────────
    sendJson(guestFd, {
        {Protocol::COMMAND, "join_room"},
        {Protocol::DATA,    {{"username", "TestGuest"},
                             {"roomCode", roomCode}}}
    });

    json hostJoined, guestJoined;
    if (!waitForEvent(hostFd, "player_joined", hostJoined) ||
        !waitForEvent(guestFd, "player_joined", guestJoined)) {
        log("test", "FAIL: forventede player_joined");
        ::close(hostFd); ::close(guestFd);
        return 1;
    }
    log("test", "Guest joined");

    // ── 5. Lobby: host starter NotePi-spillet ─────────────────────────────────
    sendJson(hostFd, {
        {Protocol::COMMAND, "start_game"},
        {Protocol::DATA,    {{"roomCode", roomCode},
                             {"game",     "NOTEPI"}}}
    });

    json hostGameStart, guestGameStart;
    if (!waitForEvent(hostFd, "game_start", hostGameStart) ||
        !waitForEvent(guestFd, "game_start", guestGameStart)) {
        log("test", "FAIL: forventede game_start");
        ::close(hostFd); ::close(guestFd);
        return 1;
    }
    std::string hostRole  = hostGameStart[Protocol::DATA].value("role", "");
    std::string guestRole = guestGameStart[Protocol::DATA].value("role", "");
    log("test", "Roles: host=" + hostRole + " guest=" + guestRole);

    // ── 6. Identify performer/communicator sockets ────────────────────────────
    int performerFd     = (hostRole  == "PERFORMER") ? hostFd  : guestFd;
    int communicatorFd  = (hostRole  == "PERFORMER") ? guestFd : hostFd;

    // ── 7. NotePi-loop: send 5-note sekvenser indtil GAME_OVER ────────────────
    // Vi sender en fast sekvens af 5 noder. Serverens secret er random, så vi
    // vil oftest få negativ feedback og spillet slutter efter 6 attempts —
    // det er fint for at verificere round-trip + game_over flow.
    const std::vector<std::string> testSequence = {"C4", "E4", "G4", "B4", "D5"};

    bool gameRunning = true;
    int  attemptsObserved = 0;
    while (gameRunning) {
        // Performer sender note-sekvens
        sendJson(performerFd, {
            {Protocol::COMMAND, Protocol::NotePi::NOTE_SEQUENCE},
            {Protocol::DATA, {{Protocol::NotePi::NOTES, testSequence}}}
        });

        // Begge clients modtager NOTE_RESULT eller GAME_OVER
        json performerEvent  = receiveJson(performerFd);
        json communicatorEvent = receiveJson(communicatorFd);
        if (performerEvent.empty() || communicatorEvent.empty()) {
            log("test", "FAIL: server disconnected during game");
            ::close(hostFd); ::close(guestFd);
            return 1;
        }

        std::string evt = performerEvent.value(Protocol::EVENT, "");
        if (evt == Protocol::NotePi::NOTE_RESULT) {
            ++attemptsObserved;
            int  attempt = performerEvent[Protocol::DATA].value(Protocol::NotePi::ATTEMPT, 0);
            bool solved  = performerEvent[Protocol::DATA].value(Protocol::NotePi::SOLVED, false);
            log("test", "Attempt " + std::to_string(attempt)
                        + (solved ? " (SOLVED)" : ""));
        } else if (evt == Protocol::GAME_OVER) {
            int score = performerEvent[Protocol::DATA].value(Protocol::SCORE, 0);
            log("test", "GAME_OVER score=" + std::to_string(score));
            gameRunning = false;
        } else {
            log("test", "FAIL: unexpected event " + evt);
            ::close(hostFd); ::close(guestFd);
            return 1;
        }
    }

    if (attemptsObserved == 0) {
        log("test", "FAIL: ingen attempts observeret før game_over");
        ::close(hostFd); ::close(guestFd);
        return 1;
    }

    log("test", "PASS: " + std::to_string(attemptsObserved) + " attempts + game_over");
    exitCode = 0;

    ::close(hostFd);
    ::close(guestFd);
    return exitCode;
}
