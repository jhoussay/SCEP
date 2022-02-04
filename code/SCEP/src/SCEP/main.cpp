#include <SCEP/SCEP.h>
//
#include <QApplication>
#include <SCEP/ExplorerWidget.h>
//
int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	ExplorerWidget widget;
	ErrorPtr pError = widget.init();
	displayError(pError);
	widget.setWindowTitle("Hello SCEP !");
	widget.show();

	int rslt = app.exec();

	return rslt;
}