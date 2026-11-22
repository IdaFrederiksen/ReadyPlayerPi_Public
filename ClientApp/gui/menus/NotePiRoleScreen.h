#ifndef NOTEPIROLESCREEN_H
#define NOTEPIROLESCREEN_H

#include "../base/BaseMenu.h"
#include <QString>

// Shown shortly between lobby and gameplay.
// Auto-closes after 3 seconds via QTimer and emits done().
class NotePiRoleScreen : public BaseMenu
{
    Q_OBJECT
public:
    explicit NotePiRoleScreen(const QString &role,
                              const QString &rpiTag,
                              int autoCloseMs = 3000,
                              QWidget *parent = nullptr);

signals:
    void done();
};

#endif // NOTEPIROLESCREEN_H
