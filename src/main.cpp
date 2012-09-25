#include <QtGui/QApplication>
#include "widget.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));

	a.setOrganizationName("RuScenery");
    a.setApplicationName("RuSceneryInstaller");
    a.setApplicationVersion("0.1.0");

    QTranslator t;
    t.load("ru", ":/Resources/Translations");
    a.installTranslator(&t);

    Widget w;
    w.show();

    return a.exec();
}
