#include "MainWindow.h"

#include <VTFLib.h>

#include <QApplication>
#include <QCoreApplication>
#include <QMessageBox>
#include <QTimer>

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("VTFEditReloaded");
    QCoreApplication::setApplicationName("VTFEditQt");

    if(!vlInitialize()) {
        QMessageBox::critical(nullptr, "VTFLib init failed", QString::fromUtf8(vlGetLastError()));
        return 1;
    }

    MainWindow window;
    window.show();

    const QStringList args = QCoreApplication::arguments();
    if(args.size() >= 2) {
        const QString path = args.at(1);
        // Defer opening until the event loop starts so the main window is fully realized.
        QTimer::singleShot(0, &window, [w = &window, path] { w->openPath(path); });
    }

    const int code = app.exec();
    vlShutdown();
    return code;
}
