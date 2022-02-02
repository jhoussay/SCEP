#include <SCEP/SCEP.h>
//
#include <QApplication>
#include <QWidget>
//
int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	QWidget w;
	w.setWindowTitle("Hello SCEP !");
	w.show();

	int rslt = app.exec();

	return rslt;
}