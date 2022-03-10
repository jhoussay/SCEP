#pragma once
//
#include <SCEP/Error.h>
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
	ErrorPtr init(const QString& path = {});

	ErrorPtr setCurrentPath(const QString& path);
	QString currentPath() const;

signals:
	void	loading(QString path);
	void	pathChanged(QString path);
	void	closed();

protected:
	void	paintEvent(QPaintEvent* pEvent) override;

private:
	ErrorPtr updateEmbeddedWidget_p();
	void updateEmbeddedWidget();

private:
	Theme* ptr_theme = nullptr;
	ExplorerWrapper* p_wrapper = nullptr;
	HWND m_windowId = 0;
	bool m_visibleExplorer = false;
	QWidget* p_widget = nullptr;
};