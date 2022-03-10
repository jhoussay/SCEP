#include <SCEP/ExplorerWidget2.h>
#include <SCEP/ExplorerWrapper2.h>
#include <SCEP/BreadcrumbsAddressBar/BreadcrumbsAddressBar.h>
#include <SCEP/Theme.h>
//
#include <QWindow>
#include <QVBoxLayout>
#include <QToolBar>
#include <QKeyEvent>
#include <QMessageBox>
#include <QToolButton>
//
//#include <uxtheme.h>
//
ExplorerWidget2::ExplorerWidget2(Theme* ptrTheme, QWidget* pParent, Qt::WindowFlags f)
	:	QWidget(pParent, f)
	,	ptr_theme(ptrTheme)
{
	setMinimumWidth(200);
	setMinimumHeight(200);

	// Backward button
	p_backwardButton = new QToolButton(this);
	p_backwardButton->setIcon(ptr_theme->icon(Theme::Icon::Left));

	// Forward button
	p_forwardButton = new QToolButton(this);
	p_forwardButton->setIcon(ptr_theme->icon(Theme::Icon::Right));

	// Parent button
	p_parentButton = new QToolButton(this);
	p_parentButton->setIcon(ptr_theme->icon(Theme::Icon::Up));

	// Address bar
	p_toolBar = new QToolBar(this);
	p_addressBar = new BreadcrumbsAddressBar(ptrTheme, this);
	connect(p_addressBar, &BreadcrumbsAddressBar::path_requested, this, &ExplorerWidget2::setCurrentPath);

	// Menu bar
	QVBoxLayout* pLayout = new QVBoxLayout(this);
	pLayout->setContentsMargins(0, 0, 0, 0);
	p_toolBar->addWidget(p_backwardButton);
	p_toolBar->addWidget(p_forwardButton);
	p_toolBar->addWidget(p_parentButton);
	p_toolBar->addWidget(p_addressBar);
	pLayout->setMenuBar(p_toolBar);
	setLayout(pLayout);

	// Explorer window wrapper
	p_wrapper = new ExplorerWrapper2(ptr_theme);
	connect(p_wrapper, &ExplorerWrapper2::loading, this, &ExplorerWidget2::onLoading);
	connect(p_wrapper, &ExplorerWrapper2::pathChanged, this, &ExplorerWidget2::onPathChanged);
	connect(p_wrapper, &ExplorerWrapper2::openNewTab, this, &ExplorerWidget2::openNewTab);
	connect(p_wrapper, &ExplorerWrapper2::closed, this, &ExplorerWidget2::closed);
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
ErrorPtr ExplorerWidget2::init(const QString& path)
{
	// Initialize the explorer
	CALL( p_wrapper->initialize(path) );

	// Create the embedding Qt widget
	QWindow* pWindow = QWindow::fromWinId((WId) p_wrapper->hwnd());
	CHECK(pWindow, "Could not create QWindow !");
	p_widget = QWidget::createWindowContainer(pWindow);
	p_widget->setMinimumWidth(100);
	p_widget->setMinimumHeight(100);
	p_widget->setParent(this);
	layout()->addWidget(p_widget);

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
void ExplorerWidget2::onLoading(const QString& path)
{
	p_addressBar->set_loading(path);
	emit loading(path);
}
//
void ExplorerWidget2::onPathChanged(const QString& path, bool success, bool virtualFolder)
{
	if (success)
	{
		p_addressBar->set_path(path, virtualFolder);
		emit pathChanged(path/*, success*/, virtualFolder);
	}
	else
	{
		p_addressBar->set_line_address_closeOnFocusOut(false);
		QMessageBox::critical(this, tr("SCEP"), tr("Could not navigate to \"%1\"").arg(path));
		p_addressBar->set_line_address_closeOnFocusOut(true);
	}
}
//
