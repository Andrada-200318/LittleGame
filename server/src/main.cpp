#include <QApplication>
#include <QFont>
#include <QtDebug>

#include "gui/MainWindow.h"
#include "net/TCPSocket.h"

int main(int argc, char* argv[]) {
    if (!TCPSocket::startup()) {
        qCritical("Could not start Winsock");
        return 1;
    }

    int exitCode = 0;
    {
        QApplication app(argc, argv);

        QFont appFont = QApplication::font();
        appFont.setPointSize(16);
        QApplication::setFont(appFont);

        MainWindow window;
        window.show();
        exitCode = app.exec();
    }

    TCPSocket::cleanup();
    return exitCode;
}
