#include <SCEP/AboutDialog.h>
#include <SCEP/Version.h>
#include <SCEP/Date.h>
//
#include <ui_AboutDialog.h>
//
#include <QApplication>
#include <QFile>
#include <QtDebug>
//
QString getFullPath(const QString& fileName, const QStringList& directories)
{
	for (const QString& directory : directories)
	{
		QString path = directory + "/" + fileName;
		if (QFile(path).exists())
		{
			return path;
		}
	}

	qWarning() << "Could not locate \"" << fileName << "\".";
	return "";
}
//
AboutDialog::AboutDialog(QWidget* pParent, Qt::WindowFlags flags)
	:	QDialog(pParent, flags)
{
	p_ui = new Ui::AboutDialog();
	p_ui->setupUi(this);

	connect(p_ui->closeButton, SIGNAL(clicked()), this, SLOT(close()));

	QString version = tr("%1.%2.%3").arg(SCEP_MAJ_VERSION).arg(SCEP_MIN_VERSION).arg(SCEP_PATCH_VERSION);

	QString appDir = qApp->applicationDirPath();
	QString licenseFileSCEP = getFullPath("LICENSE", {appDir, appDir + "/../.."});
	QString licenseFileQt = getFullPath("Qt.license", {appDir + "/third_parties_licenses", appDir + "/../../setup/third_parties_licenses"});
	QString licenseFileGears = getFullPath("/gears.license", {appDir + "/third_parties_licenses", appDir + "/../../setup/third_parties_licenses"});

	QString sparseAbout = p_ui->aboutLabel->text();
	QString about = sparseAbout.arg(version).arg(SCEP_DATE).arg(licenseFileSCEP).arg(licenseFileQt).arg(licenseFileGears);
	p_ui->aboutLabel->setText(about);
}
//
AboutDialog::~AboutDialog()
{
	delete p_ui;
	p_ui = nullptr;
}
//
