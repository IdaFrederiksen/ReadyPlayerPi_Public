#ifndef SCOREBOARDSCREEN_H
#define SCOREBOARDSCREEN_H

#include "../base/BaseMenu.h"
#include <QString>
#include <QVector>

// Scoreboard / Game Over.
//
// Shows:
//   - "Scorebord" + 🏆 trophy
//   - "YOUR SCORE: <X>"
//   - Top-5 (rank. names, score)
//   - Own rank below if not top-5
//   - Green "PLAY AGAIN"-button, if "host player" 
//   - Tilbage-pil øverst venstre
//   - HOST RPI / GUEST RPI tag øverst højre
//
// Reused by both NotePi and GyroPi.
struct ScoreboardEntry {
    QString names;
    int     score;
};

class ScoreboardScreen : public BaseMenu
{
    Q_OBJECT
public:
    // scoreUnit:    "" for point-based games (NotePi), "s" for time-based games (GyroPi)
    // showPlayAgain: false hides the PLAY AGAIN button (host uses promptPlayAgain instead)
    ScoreboardScreen(int yourScore,
                      int yourRank,
                      const QVector<ScoreboardEntry> &top5,
                      const QString &scoreUnit = "",
                      bool showPlayAgain = true,
                      QWidget *parent = nullptr);

signals:
    void playAgain();
    void back();
};

#endif // SCOREBOARDSCREEN_H
