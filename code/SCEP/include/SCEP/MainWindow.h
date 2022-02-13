#pragma once
//
#include <SCEP/Theme.h>
//
#include <QMainWindow>
//
//#define FRAMELESS
//
#ifdef FRAMELESS
#include <optional>
#endif //FRAMELESS
//
class QToolButton;
//
namespace Ui
{
	class MainWindow;
}
//
class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(Theme* ptrTheme);
	~MainWindow();

#ifdef FRAMELESS
protected:
	//  Double click title bar
	void mouseDoubleClickEvent(QMouseEvent* pEvent) override;
	//  Click the title bar
	void mousePressEvent(QMouseEvent* pEvent) override;
	//  Release the title bar
	void mouseReleaseEvent(QMouseEvent* pEvent) override;
	//  Drag title bar
	void mouseMoveEvent(QMouseEvent* pEvent) override;
#endif //FRAMELESS

protected slots:
	void	addNewTab(QString path = {});
	void	showMenu();

	void	onTabCloseRequested();

	void	pathChanged(QString path);
	void	closed();

private:
	void updateIcons();
	static QString tabName(const QString& tabPath);

private:
	Theme* ptr_theme = nullptr;
	Ui::MainWindow* p_ui = nullptr;
	QToolButton* p_addTabButton = nullptr;
	QToolButton* p_menuButton = nullptr;

#ifdef FRAMELESS
	std::optional<QPoint> m_dragPosition;
#endif //FRAMELESS
};