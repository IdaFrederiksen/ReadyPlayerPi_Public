// Shared/JSONCommunication.h — Newline-framed JSON transport helpers
// Used by both ClientApp and ServerApp.
//
// Protocol: each message is a single JSON object serialised on one line,
// terminated by '\n'.  The receiver reads until '\n', then parses.

#pragma once
#include <string>
#include <unistd.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Read one '\n'-terminated line from the socket.
// Returns an empty string on disconnect or error.
inline std::string readLine(int fileDescriptor) {
    std::string line;
    char        character;
    while (true) {
        int bytesRead = read(fileDescriptor, &character, 1);
        if (bytesRead <= 0) return ""; // disconnect or error
        if (character == '\n') break;
        line += character;
    }
    return line;
}

// Write a '\n'-terminated string to the socket.
// Loops until all bytes are sent (handles partial writes).
inline bool sendLine(int fileDescriptor, const std::string &message) {
    std::string framedMessage   = message + "\n";
    int         bytesSentSoFar  = 0;
    while (bytesSentSoFar < (int)framedMessage.size()) {
        int bytesWritten = write(fileDescriptor,
                                 framedMessage.c_str() + bytesSentSoFar,
                                 framedMessage.size()  - bytesSentSoFar);
        if (bytesWritten <= 0) return false;
        bytesSentSoFar += bytesWritten;
    }
    return true;
}

// Serialise a JSON object and send it as one framed line.
inline bool sendJson(int fileDescriptor, const json &message) {
    return sendLine(fileDescriptor, message.dump());
}

// Read one framed line and parse it as JSON.
// Returns an empty json object on disconnect, error, or parse failure.
inline json receiveJson(int fileDescriptor) {
    std::string line = readLine(fileDescriptor);
    if (line.empty()) return json();
    try   { return json::parse(line); }
    catch (...) { return json(); }
}
