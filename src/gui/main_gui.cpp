#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("FORGE");
    app.setApplicationVersion("1.0.0");

    forge::gui::MainWindow window;
    window.show();

    return app.exec();
}
