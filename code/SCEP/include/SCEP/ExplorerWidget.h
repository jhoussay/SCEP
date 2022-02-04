#pragma once
//
#include <SCEP/Error.h>
//
#include <QWidget>
//
struct ExplorerWidgetData;	// PIMPL
//
/**
 *	@brief				
 */
class ExplorerWidget : public QWidget
{
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

private:
	ExplorerWidgetData* p_d = nullptr;
};