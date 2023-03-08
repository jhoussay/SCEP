#pragma once
//
#include <SCEP/SCEP.h>
#include <SCEP_CORE/Error.h>
#include <SCEP_CORE/Navigation.h>
//
#include <QWidget>
//
class ExplorerWrapper;
class Theme;
//
/**
 *	@brief				
 */
class ExplorerWidget : public QWidget
{
	Q_OBJECT

public:
	/**
	 *	@brief		
	 */
	ExplorerWidget(Theme* ptrTheme, QWidget* pParent = nullptr, Qt::WindowFlags f = {});
	/**
	 *	@brief		
	 */
	virtual ~ExplorerWidget();

public:
	ErrorPtr init(const NavigationPath& path = {});

	ErrorPtr setCurrentPath(const NavigationPath& path);
	NavigationPath currentPath() const;

signals:
	void	loading(const NavigationPath& path);
	void	pathChanged(const NavigationPath& path);
	void	openNewTab(const NavigationPath& path, NewTabPosition position, NewTabBehaviour behaviour);
	void	closed();

protected:
	void	paintEvent(QPaintEvent* pEvent) override;

private:
	void	updateEmbeddedWidget();

private:
	Theme* ptr_theme = nullptr;
	ExplorerWrapper* p_wrapper = nullptr;
	//HWND m_windowId = 0;
	bool m_visibleExplorer = false;
	QWidget* p_widget = nullptr;
};
