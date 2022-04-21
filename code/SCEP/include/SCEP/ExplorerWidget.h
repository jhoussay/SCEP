#pragma once
//
#include <SCEP/Error.h>
#include <SCEP/SCEP.h>
#include <SCEP/Navigation.h>
//
#include <QWidget>
//
#include <queue>
//
class ExplorerWrapper;
class BreadcrumbsAddressBar;
class Theme;
//
class QToolBar;
class QAction;
class QMenu;
//
/**
 *	@ingroup				SCEP
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
	ErrorPtr				init(const NavigationPath& path = {});

	void					setCurrentPath(const NavigationPath& path);
	const NavigationPath&	currentPath() const;

signals:
	void					loading(const NavigationPath& path);
	void					pathChanged(const NavigationPath& path);
	void					openNewTab(const NavigationPath& path, NewTabPosition position, NewTabBehaviour behaviour);

public slots:
	void					rename();
	void					copy();
	void					cut();
	void					paste();
	void					showAddressBar();
	void					del();
	void					forceDel();
	void					selectAll();
	void					mkDir();
	void					undo();
	void					redo();
	void					navigateBackward();
	void					navigateForward();
	void					navigateUp();

protected slots:
	void					navigateTo(const NavigationRequest& request);

	void					onLoading(const NavigationPath& path);
	void					onPathChanged(const NavigationPath& path, bool success);

private:
	Theme*					ptr_theme			=	nullptr;	//!< Application theme
	ExplorerWrapper*		p_wrapper			=	nullptr;	//!< Wrapper for explorer win32 window (without address bar)
	QWidget*				p_widget			=	nullptr;	//!< Widget embedding explorer win32 window
	QAction*				p_backwardAction	=	nullptr;	//!< Navigate backward action
//	QMenu*					p_backwardMenu		=	nullptr;	//!< Navigate backward action menu
	QAction*				p_forwardAction		=	nullptr;	//!< Navigate forward action
//	QMenu*					p_forwardMenu		=	nullptr;	//!< Navigate forward action menu
	QAction*				p_parentAction		=	nullptr;	//!< Navigate to parent folder action
	BreadcrumbsAddressBar*	p_addressBar		=	nullptr;	//!< Address bar widget
	QToolBar*				p_toolBar			=	nullptr;	//!< ToolBar containing navigation buttons and address bar

	std::queue<NavigationRequest>	m_pendingRequests	=	{};	//!< Navigation pathes requested but not processed yet
	std::optional<NavigationRequest>m_currentRequest	=	{};	//!< Current request
	NavigationHistory				m_navigationHistory	=	{};	//!< Navigation history
};
