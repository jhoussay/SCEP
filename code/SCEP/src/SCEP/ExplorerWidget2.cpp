#include <SCEP/ExplorerWidget2.h>
#include <SCEP/ExplorerWrapper2.h>
//
#include <QWindow>
#include <QVBoxLayout>
//
//#include <uxtheme.h>
//
ExplorerWidget2::ExplorerWidget2(QWidget* pParent, Qt::WindowFlags f)
	:	QWidget(pParent, f)
{
	p_wrapper = new ExplorerWrapper2();
	connect(p_wrapper, SIGNAL(loading(QString)), this, SIGNAL(loading(QString)));
	connect(p_wrapper, SIGNAL(pathChanged(QString)), this, SIGNAL(pathChanged(QString)));
	connect(p_wrapper, SIGNAL(openNewTab(QString, NewTabPosition, NewTabBehaviour)), this, SIGNAL(openNewTab(QString, NewTabPosition, NewTabBehaviour)));
	connect(p_wrapper, SIGNAL(closed()), this, SIGNAL(closed()));

	setMinimumWidth(200);
	setMinimumHeight(200);
}
//
ExplorerWidget2::~ExplorerWidget2()
{
	if (p_wrapper)
	{
		p_wrapper->finalize();
		p_wrapper->Release();
		p_wrapper = nullptr;
	}
}
//
ErrorPtr ExplorerWidget2::init(Theme* ptr_theme, const QString& path)
{
	// Initialize the explorer
	CALL( p_wrapper->initialize(ptr_theme, path) );

	// Create the embedding Qt widget
	QWindow* pWindow = QWindow::fromWinId((WId) p_wrapper->hwnd());
	CHECK(pWindow, "Could not create QWindow !");
	p_widget = QWidget::createWindowContainer(pWindow);
	p_widget->setMinimumWidth(100);
	p_widget->setMinimumHeight(100);
	p_widget->setParent(this);
	QVBoxLayout* pLayout = new QVBoxLayout(this);
	pLayout->setContentsMargins(0, 0, 0, 0);
	pLayout->addWidget(p_widget);
	setLayout(pLayout);

	// Done !
	return success();
}
//
ErrorPtr ExplorerWidget2::setCurrentPath(const QString& path)
{
	CHECK( p_wrapper, "Unitialized object !" );
	CALL( p_wrapper->setCurrentPath(path) );
	return success();
}
//
QString ExplorerWidget2::currentPath() const
{
	if (p_wrapper)
	{
		return p_wrapper->currentPath();
	}
	else
	{
		qWarning() << "Unitialized object";
		return {};
	}
}
//
//void ExplorerWidget2::paintEvent([[maybe_unused]] QPaintEvent* pEvent)
//{
//	if (! m_visibleExplorer)
//	{
//		p_wrapper->setVisible(true);
//		m_visibleExplorer = true;
//	}
//
//	updateEmbeddedWidget();
//
//	//QWidget::paintEvent(pEvent);
//}
////
//ErrorPtr ExplorerWidget2::updateEmbeddedWidget_p()
//{
//	if (QWidget* pWidget = p_widget)
//	{
//		HWND explorerId = p_wrapper->hwnd();
//
//		// Estimate the title bar height
//		HTHEME htheme = GetWindowTheme(explorerId);
//		int h = GetThemeSysSize(htheme, SM_CXBORDER) + GetThemeSysSize(htheme, SM_CYSIZE) + GetThemeSysSize(htheme, SM_CXPADDEDBORDER) * 2;
//
//		// Set the position
//		QPoint pos = p_widget->mapToGlobal(QPoint(0, 0));
//		BOOL okPos = SetWindowPos(	explorerId,
//									nullptr,
//									0,//pos.x(),
//									-h,//pos.y(),
//									p_widget->width(),
//									p_widget->height() + h,
//									SWP_NOZORDER);
//		CHECK(okPos, "Position and size change error");
//
//		UpdateWindow(explorerId);
//		UpdateWindow(m_windowId);
//	}
//
//	return success();
//}
////
//void ExplorerWidget2::updateEmbeddedWidget()
//{
//	if (ErrorPtr pError = updateEmbeddedWidget_p())
//	{
//		displayError(pError);
//	}
//}
//
