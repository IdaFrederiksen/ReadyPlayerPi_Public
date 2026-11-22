// GyroPiRoleAnnouncement.h — Role announcement screen shown briefly at game start
//
// Displayed for 1–5 seconds (NF9) on both the Performer and Communicator
// RPi as soon as the server has assigned roles. Shows the game name and
// the player's role in large centred text on the standard BaseMenu canvas.
//
// Lifecycle:
//   QtGUI creates the window in showGameStartHeader(), shows it for the
//   configured duration, then closes it and opens the matching gameplay
//   window (GyroPiCommunicatorWindow or GyroPiPerformerWindow).
//
// ECB role: BOUNDARY (part of the GUI subsystem).

#pragma once

#include "../base/BaseMenu.h"
#include <QLabel>
#include <QString>
#include <string>

class GyroPiRoleAnnouncement : public BaseMenu
{
    Q_OBJECT

public:
    // gameName: e.g. "GYRO-PI"
    // role:     "PERFORMER" or "COMMUNICATOR"
    // partner:  the teammates's username (shown as a subtitle)
    // autoCloseMs: vises i [1..5] sek pr. NF9, default 3000.
    GyroPiRoleAnnouncement(const std::string &gameName,
                           const std::string &role,
                           const std::string &partner,
                           int autoCloseMs = 3000,
                           QWidget *parent = nullptr);

signals:
    void done();

private:
    QLabel *gameLabel_    = nullptr;
    QLabel *roleLabel_    = nullptr;
    QLabel *partnerLabel_ = nullptr;
};
