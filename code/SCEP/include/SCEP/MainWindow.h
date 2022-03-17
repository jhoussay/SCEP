#pragma once
//
#include <SCEP/SCEP.h>
#include <SCEP/Theme.h>
#include <SCEP/Navigation.h>
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
class QWidgetAction;
class QToolButton;
class QRadioButton;
class QButtonGroup;
class QSettings;
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
	MainWindow(Theme* ptrTheme, QSettings* ptrSettings);
	~MainWindow();

#ifdef FRAMELESS
protected:
	//  Double click title bar
	void			mouseDoubleClickEvent(QMouseEvent* pEvent) override;
	//  Click the title bar
	void			mousePressEvent(QMouseEvent* pEvent) override;
	//  Release the title bar
	void			mouseReleaseEvent(QMouseEvent* pEvent) override;
	//  Drag title bar
	void			mouseMoveEvent(QMouseEvent* pEvent) override;
#endif //FRAMELESS

private slots:
	void			addNewTab(NavigationPath path = {}, NewTabPosition position = NewTabPosition::Last, NewTabBehaviour behaviour = NewTabBehaviour::Current);
	void			closeCurrentTab();
	void			showMenu();
	void			about();

	void			styleChanged(int styleId);

	void			onTabCloseRequested();

	void			loading(const NavigationPath& path);
	void			pathChanged(const NavigationPath& path);
	void			tabClosed();

protected:
	void			closeEvent(QCloseEvent* pEvent) override;
	bool			eventFilter(QObject *obj, QEvent *event) override;

private:
	void			closeTab(int tabIndex, bool closeAppIfNoRemainingTab = true);
	void			updateIcons();
	static QString	tabName(const NavigationPath& path);

private:
	Theme* ptr_theme = nullptr;
	QSettings* ptr_settings = nullptr;
	Ui::MainWindow* p_ui = nullptr;

	QAction* p_addTabAction = nullptr;
	QAction* p_closeTabAction = nullptr;
	QWidgetAction* p_themeAction = nullptr;
	QWidgetAction* p_themeTooltipAction = nullptr;
	QRadioButton* p_autoThemeButton = nullptr;
	QRadioButton* p_lightThemeButton = nullptr;
	QRadioButton* p_darkThemeButton = nullptr;
	QButtonGroup* p_themeButtonGroup = nullptr;
	QAction* p_aboutAction = nullptr;

	QToolButton* p_addTabButton = nullptr;
	QToolButton* p_menuButton = nullptr;

#ifdef FRAMELESS
	std::optional<QPoint> m_dragPosition;
#endif //FRAMELESS
};
