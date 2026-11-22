// Shared/NotePiProtocol.h — JSON protocol constants specific to NotePi
//
// Shared between client (NotePiSession) and server (NotePiGame)
// so both sides use the same string keys.
//
// Game-agnostic keys (command, event, data, game_over, score, ...)
// live in Protocol.h and are inherited into this namespace.

#pragma once
#include "Protocol.h"

namespace Protocol::NotePi {

    constexpr const char* NOTE_SEQUENCE = "note_sequence";  // Command: "Here is the note secuence played by performer"
    constexpr const char* NOTE_RESULT = "note_result";      // Command: "Here is the result of note sequence after evaluation"
    constexpr const char* NOTES = "notes";                  // "Here is the note sequence played by performer"
    constexpr const char* FEEDBACK = "feedback";            // "Here is the result of note sequence after evaluation"
    constexpr const char* ATTEMPT = "attempt";              // Number of attempts used
    constexpr const char* SOLVED = "solved";                // Boolean: Has the secret sequence been solved?

    // Wordle feedback values
    constexpr const char* GREEN = "GREEN";
    constexpr const char* YELLOW = "YELLOW";
    constexpr const char* RED = "RED";

}  // namespace Protocol::NotePi
