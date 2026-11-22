// ClientApp/Client.cpp — Ready Player Pi Client (entry point)
//
// Build (CMake, Qt6):  cmake -B build && cmake --build build
// Run:                 ./build/client-hw [SERVER_IP]
//
// All orchestration lives in ClientController. This file is only the entry
// point — keeping main() out of every application-model sequence diagram.

#include <QApplication>
#include <QMetaType>
#include "ClientController.h"
#include "gui/menus/GyroPiCommunicatorWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Vores vindue-flow lukker midlertidigt alle vinduer mellem skærme (fx
    // mellem lobby og NotePi-game). Uden dette ville Qt afslutte processen
    // i det øjeblik sidste lobby-vindue lukkes, før gameplay-widget'et er
    // åbnet — og dermed lukke fx Communicator's vindue umiddelbart.
    QApplication::setQuitOnLastWindowClosed(false);

    // Registrér custom typer der skal kunne krydse tråde via
    // QMetaObject::invokeMethod(..., Qt::QueuedConnection).
    qRegisterMetaType<GyroCourseInfo>("GyroCourseInfo");

    const char *serverIp = "127.0.0.1";   // default to localhost
    if (argc > 1) serverIp = argv[1];

    ClientController controller(serverIp, 9000);
    return controller.run();
}
