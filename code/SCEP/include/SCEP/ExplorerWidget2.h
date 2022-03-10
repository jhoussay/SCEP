#pragma once
//
#include <SCEP/Error.h>
#include <SCEP/SCEP.h>
//
#include <QWidget>
//
class ExplorerWrapper2;
class BreadcrumbsAddressBar;
class Theme;
//
class QToolBar;
class QToolButton;
//
/**
 *	@ingroup			SCEP
 *	@brief				
 */
class ExplorerWidget2 : public QWidget
{
	Q_OBJECT

public:
	/**
	 *	@brief		
	 */
	ExplorerWidget2(Theme* ptrTheme, QWidget* pParent = nullptr, Qt::WindowFlags f = {});
	/**
	 *	@brief		
	 */
	virtual ~ExplorerWidget2();

public:
	ErrorPtr			init(const QString& path = {});

	ErrorPtr			setCurrentPath(const QString& path);
	QString				currentPath() const;

signals:
	void				loading(const QString& path);
	void				pathChanged(const QString& path, bool virtualFolder);
	void				openNewTab(const QString& path, NewTabPosition position, NewTabBehaviour behaviour);
	void				closed();

protected slots:
	void				onLoading(const QString& path);
	void				onPathChanged(const QString& path, bool success, bool virtualFolder);

private:
	Theme*				ptr_theme			=	nullptr;	//!< Application theme
	ExplorerWrapper2*	p_wrapper			=	nullptr;	//!< Wrapper for explorer win32 window (without address bar)
	QWidget*			p_widget			=	nullptr;	//!< Widget embedding explorer win32 window
	QToolButton*		p_backwardButton	=	nullptr;	//!< Navigate backward button
	QToolButton*		p_forwardButton		=	nullptr;	//!< Navigate forward button
	QToolButton*		p_parentButton		=	nullptr;	//!< Navigate to parent folder button
	BreadcrumbsAddressBar* p_addressBar		=	nullptr;	//!< Address bar widget
	QToolBar*			p_toolBar			=	nullptr;	//!< ToolBar containing navigation buttons and address bar
};