#pragma once
//
#include <SCEP/Error.h>
//
#include <QWidget>
//
//struct ExplorerWidgetData;	// PIMPL
class ExplorerWrapper;
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
	ExplorerWidget(QWidget* pParent = nullptr, Qt::WindowFlags f = {});
	/**
	 *	@brief		
	 */
	virtual ~ExplorerWidget();

public:
	ErrorPtr init(const QString& path = {});

	ErrorPtr setCurrentPath(const QString& path);
	QString currentPath() const;

signals:
	void	pathChanged(QString path);
	void	closed();

protected:
	void 	moveEvent(QMoveEvent* pEvent) override;
	void 	resizeEvent(QResizeEvent* pEvent) override;
	void 	showEvent(QShowEvent* pEvent) override;

private:
	ErrorPtr updateEmbeddedWidget_p();
	void updateEmbeddedWidget();

private:
	ExplorerWrapper* p_wrapper = nullptr;
	QWidget* p_widget = nullptr;
};