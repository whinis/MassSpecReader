#include "readerwindow.h"
#include <QApplication>

bool variantLessThan(const QVariant &v1, const QVariant &v2)
{
    return v1.toString() < v2.toString();
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QIcon icon(":/icons/e7d1b151ea.ico");
    ReaderWindow w;
    w.setWindowIcon(icon);
    w.show();

    return a.exec();
}
