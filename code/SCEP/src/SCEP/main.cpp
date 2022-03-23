#include <SCEP/SCEP.h>
#include <SCEP/MainWindow.h>
#include <SCEP/Theme.h>
//
#include <QApplication>
#include <QSettings>
//
static const QString OrganizationStr = "SCEP";
static const QString ApplicationStr = "SCEP";
//
int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	int rslt = 0;
	{
		QSettings settings(QSettings::IniFormat, QSettings::UserScope, OrganizationStr, "SCEP");
		Theme theme(&settings);
		MainWindow mainWindow(&theme, &settings);
		mainWindow.show();

		rslt = app.exec();
	}

	return rslt;
}
