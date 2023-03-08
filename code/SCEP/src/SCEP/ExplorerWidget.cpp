#include <SCEP/ExplorerWidget.h>
#include <SCEP_CORE/ExplorerWrapper.h>
//
#include <QWindow>
#include <QVBoxLayout>
//
ExplorerWidget::ExplorerWidget(Theme* ptrTheme, QWidget* pParent, Qt::WindowFlags f)
	:	QWidget(pParent, f)
	,	ptr_theme(ptrTheme)
{
	p_wrapper = new ExplorerWrapper();
	connect(p_wrapper, &ExplorerWrapper::loading, this, &ExplorerWidget::loading);
	connect(p_wrapper, &ExplorerWrapper::pathChanged, this, &ExplorerWidget::pathChanged);
	connect(p_wrapper, &ExplorerWrapper::closed, this, &ExplorerWidget::closed);
	connect(p_wrapper, &ExplorerWrapper::openNewTab, this, &ExplorerWidget::openNewTab);

	setMinimumWidth(200);
	setMinimumHeight(200);
}
//
ExplorerWidget::~ExplorerWidget()
{
	if (p_wrapper)
	{
		delete p_wrapper;
		p_wrapper = nullptr;
	}
}
//
ErrorPtr ExplorerWidget::init(const NavigationPath& path)
{
	// Initialize the explorer
	CALL( p_wrapper->initialize(/*m_windowId,*/ ptr_theme, path) );

	// Create the embedding Qt widget
	QWindow* pWindow = QWindow::fromWinId((WId) /*m_windowId*/ p_wrapper->hwnd());
	CHECK(pWindow, "Could not create QWindow !");
	p_widget = QWidget::createWindowContainer(pWindow, this);
	p_widget->setMinimumWidth(100);
	p_widget->setMinimumHeight(100);
	QVBoxLayout* pLayout = new QVBoxLayout(this);
	pLayout->setContentsMargins(0, 0, 0, 0);
	pLayout->addWidget(p_widget);
	setLayout(pLayout);

	// Done !
	return success();
}
//
ErrorPtr ExplorerWidget::setCurrentPath(const NavigationPath& path)
{
	CHECK( p_wrapper, "Unitialized object !" );
	CALL( p_wrapper->setCurrentPath(path) );
	return success();
}
//
NavigationPath ExplorerWidget::currentPath() const
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
void ExplorerWidget::paintEvent([[maybe_unused]] QPaintEvent* pEvent)
{
	if (! m_visibleExplorer)
	{
		p_wrapper->setVisible(true);
		m_visibleExplorer = true;
	}

	updateEmbeddedWidget();

	//QWidget::paintEvent(pEvent);
}
//
void ExplorerWidget::updateEmbeddedWidget()
{
	if (p_widget && p_wrapper)
	{
		if (ErrorPtr pError = p_wrapper->updateWindow(p_widget->width(), p_widget->height()))
		{
			displayError(pError);
		}
	}
}
//
