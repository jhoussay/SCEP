#include <SCEP/SCEP.h>
#include <SCEP/MainWindow.h>
#include <SCEP/Theme.h>
//
#include <QApplication>
//
int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	int rslt = 0;
	{
		Theme theme;
		MainWindow mainWindow(&theme);
		mainWindow.show();

		rslt = app.exec();
	}

	return rslt;
}