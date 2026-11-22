#ifndef NOTEPIPERFORMERWIDGET_H
#define NOTEPIPERFORMERWIDGET_H

#include "../base/BaseMenu.h"
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QVector>

// Performer-screen.
//
// Three states:
//   1. Idle:      "Start Recording" + play-button 
//   2. Recording: 5 note-slots at the top, filled when note detected + stop button 
//   3. Result:    Short visualization of attempt-feedback before reset to Idle
//                 (or GameOver if solved)
//
// Slots (is called  via QMetaObject::invokeMethod from mic-thread,
// Qt::QueuedConnection):
//   onNoteDetected(noteName, slotIndex) — updates one slot at a time
//   onAttemptResult(feedback, solved)   — short visualization + reset to Idle
//   onGameOver(score, rank, top5)       — transition to ScoreboardScreen
class NotePiPerformerWidget : public BaseMenu
{
    Q_OBJECT
public:
    explicit NotePiPerformerWidget(QWidget *parent = nullptr);

public slots:
    void showIdle();
    void showRecording();
    void onNoteDetected(QString noteName, int slotIndex);

signals:
    void recordingStarted();   // Idle-screen play-button clicked

private:
    void buildIdleLayout();
    void buildRecordingLayout();
    void clearCentral();

    QLabel  *titleLabel_          = nullptr;
    QPushButton *recordBtn_       = nullptr;
    QLabel  *slotNoteLabels_[5]   = {nullptr, nullptr, nullptr, nullptr, nullptr};
    QLabel  *slotNameLabels_[5]   = {nullptr, nullptr, nullptr, nullptr, nullptr};
    QLabel  *slotHeaderLabels_[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
};

#endif // NOTEPIPERFORMERWIDGET_H
