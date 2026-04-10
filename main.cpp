#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>
#include <QFileInfo>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setAttribute(Qt::AA_Use96Dpi);
    app.setStyle(QStyleFactory::create("windows11"));

    MainWindow w;
    w.resize(960, 640);
    w.setWindowTitle("简易播放器");
    w.show();

    if (argc > 1) {
        QString filePath = QString::fromLocal8Bit(argv[1]);
        QFileInfo info(filePath);

        if (info.exists() && info.isFile()) {
            QUrl url = QUrl::fromLocalFile(filePath);
            if (w.loadLastPlaying().first == url.toString()) {
                w.playLastPlaying();
            }
            else {
                w.playSingleFile(url);
            }
        }
    } else {
        w.loadLastPlaying();
        w.playLastPlaying();
    }

    return app.exec();
}
