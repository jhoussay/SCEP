#pragma once
//
#include <SCEP/SCEP.h>
#include <SCEP/Theme.h>
//
#include <QMainWindow>
#include <QFileIconProvider>
//
//#define FRAMELESS
//
#ifdef FRAMELESS
#include <optional>
#endif //FRAMELESS
//
class QAction;
class QToolButton;
//
namespace Ui
{
	class MainWindow;
}
//
/**
 *	@ingroup				SCEP
 *	@brief					SCEP main window
 */
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
	void	addNewTab(QString path = {}, NewTabPosition position = NewTabPosition::Last, NewTabBehaviour behaviour = NewTabBehaviour::Current);
	void	closeCurrentTab();
	void	showMenu();
	void	about();

	void	onTabCloseRequested();

	void	loading(const QString& path);
	void	pathChanged(const QString& path, bool virtualFolder);
	void	tabClosed();

protected:
	void	closeEvent(QCloseEvent* pEvent) override;

private:
	void	closeTab(int tabIndex, bool closeAppIfNoRemainingTab = true);
	void	updateIcons();
	static QString tabName(const QString& tabPath, bool virtualFolder);

private:
	Theme* ptr_theme = nullptr;
	Ui::MainWindow* p_ui = nullptr;

	QAction* p_addTabAction = nullptr;
	QAction* p_closeTabAction = nullptr;
	QAction* p_aboutAction = nullptr;

	QToolButton* p_addTabButton = nullptr;
	QToolButton* p_menuButton = nullptr;

	QFileIconProvider m_fileIconProvider;

#ifdef FRAMELESS
	std::optional<QPoint> m_dragPosition;
#endif //FRAMELESS
};