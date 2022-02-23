#pragma once
//
#include <SCEP/Error.h>
//
#include <QWidget>
//
class ExplorerWrapper2;
class Theme;
//
/**
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
	ErrorPtr init(Theme* ptr_theme, const QString& path = {});

	ErrorPtr setCurrentPath(const QString& path);
	QString currentPath() const;

signals:
	void	loading(QString path);
	void	pathChanged(QString path);
	void	openNewTab(QString path);
	void	closed();
//
//protected:
//	void	paintEvent(QPaintEvent* pEvent) override;
//
//private:
//	ErrorPtr updateEmbeddedWidget_p();
//	void updateEmbeddedWidget();

private:
	ExplorerWrapper2* p_wrapper = nullptr;
	bool m_visibleExplorer = false;
	QWidget* p_widget = nullptr;
};