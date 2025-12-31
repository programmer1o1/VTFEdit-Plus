#include "MainWindow.h"

#include <VTFLib.h>

#include <QApplication>
#include <QCoreApplication>
#include <QMessageBox>

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("VTFEditReloaded");
    QCoreApplication::setApplicationName("VTFEditQt");

    if(!vlInitialize()) {
        QMessageBox::critical(nullptr, "VTFLib init failed", QString::fromUtf8(vlGetLastError()));
        return 1;
    }

    MainWindow window;
    window.resize(1100, 750);
    window.show();

    const QStringList args = QCoreApplication::arguments();
    if(args.size() >= 2) {
        window.openPath(args.at(1));
    }

    const int code = app.exec();
    vlShutdown();
    return code;
}
