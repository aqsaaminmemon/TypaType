#include "ui/MainWindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QScreen>
int main(int argc,char **argv) {
    QApplication app(argc,argv);app.setOrganizationName("TypaType");app.setApplicationName("TypaType");app.setApplicationVersion("1.0.0");
    QCommandLineParser parser;parser.setApplicationDescription("A little practice. A little progress.");parser.addHelpOption();parser.addVersionOption();
    QCommandLineOption data("data-dir","Use an alternative local data folder (for testing or portable use).","directory");parser.addOption(data);parser.process(app);
    try {
        typatype::MainWindow window(parser.value(data));
        if(auto *screen=app.primaryScreen()) {
            const auto available=screen->availableGeometry().size()-QSize(40,70);
            window.resize(window.size().boundedTo(available));
        }
        window.show();return app.exec();
    }
    catch(const std::exception &e) {QMessageBox::critical(nullptr,"TypaType could not start",QString::fromUtf8(e.what()));return 1;}
}
