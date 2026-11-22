// ServerApp/NotePi/NotePi.h — NotePi domain types
// Server-side only: used by NotePiGame for sequence comparison

#pragma once
#include <string>
#include <vector>

// Secret sequence the Communicator must guide the Performer to reproduce
struct NoteSequence {
    std::vector<std::string> notes; // e.g. {"C4", "E4", "G4"}
};

// Per-note comparison result sent back to the Communicator
enum class NoteFeedback {
    CORRECT,
    WRONG_PITCH,
    WRONG_ORDER
};
