#include "widget.h"

#include <QApplication>
#include <QStyleFactory>
#include "SerialEvent.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QApplication::setStyle(QStyleFactory::create("Fusion"));
    Widget w;
    a.installNativeEventFilter(w.mySerialEvent);
    w.show();
    return a.exec();
}
