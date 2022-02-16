#pragma once
//
#include <QDialog>
//
namespace Ui
{
	class AboutDialog;
}
//
class AboutDialog : public QDialog
{
public:
	AboutDialog(QWidget* pParent = nullptr, Qt::WindowFlags flags = {});
	~AboutDialog();

private:
	Ui::AboutDialog* p_ui = nullptr;
};
