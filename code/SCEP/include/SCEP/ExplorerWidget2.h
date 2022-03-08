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
	ExplorerWidget2(QWidget* pParent = nullptr, Qt::WindowFlags f = {});
	/**
	 *	@brief		
	 */
	virtual ~ExplorerWidget2();

public:
	ErrorPtr			init(Theme* ptr_theme, const QString& path = {});

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
	ExplorerWrapper2*	p_wrapper		=	nullptr;	//!< Wrapper for explorer win32 window (without address bar)
	QWidget*			p_widget		=	nullptr;	//!< Widget embedding explorer win32 window
	BreadcrumbsAddressBar* p_addressBar	=	nullptr;	//!< Address bar widget
	QToolBar*			p_toolBar		=	nullptr;	//!< ToolBar containing navigation buttons and address bar
};