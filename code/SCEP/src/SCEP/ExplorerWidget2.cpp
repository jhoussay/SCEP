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
#include <QMenu>
//
ExplorerWidget2::ExplorerWidget2(Theme* ptrTheme, QWidget* pParent, Qt::WindowFlags f)
	:	QWidget(pParent, f)
	,	ptr_theme(ptrTheme)
{
	setMinimumWidth(200);
	setMinimumHeight(200);

	// Tool bar
	p_toolBar = new QToolBar(this);

	// Backward button
	p_backwardAction = new QAction(ptr_theme->icon(Theme::Icon::Left), tr("Navigate backward"), this);
	p_backwardAction->setShortcut(Qt::ALT | Qt::Key_Left);
	//p_backwardMenu = new QMenu(tr("Backward"), this);
	//p_backwardMenu->addAction("toto");
	//p_backwardAction->setMenu(p_backwardMenu);
	p_backwardAction->setEnabled(false);
	connect(p_backwardAction, &QAction::triggered, this, &ExplorerWidget2::navigateBackward);
	p_toolBar->addAction(p_backwardAction);

	// Forward button
	p_forwardAction = new QAction(ptr_theme->icon(Theme::Icon::Right), tr("Navigate forward"), this);
	p_forwardAction->setShortcut(Qt::ALT | Qt::Key_Right);
	//p_forwardMenu = new QMenu(tr("Forward"), this);
	//p_forwardMenu->addAction("toto");
	//p_forwardAction->setMenu(p_backwardMenu);
	p_forwardAction->setEnabled(false);
	connect(p_forwardAction, &QAction::triggered, this, &ExplorerWidget2::navigateBackward);
	p_toolBar->addAction(p_forwardAction);

	// Parent button
	p_parentAction = new QAction(ptr_theme->icon(Theme::Icon::Up), tr("Navigate to parent folder"), this);
	p_parentAction->setShortcut(Qt::ALT | Qt::Key_Up);
	//p_parentMenu = new QMenu(tr("Parent"), this);
	//p_parentMenu->addAction("toto");
	//p_parentAction->setMenu(p_parentMenu);
	p_parentAction->setEnabled(false);
	connect(p_parentAction, &QAction::triggered, this, &ExplorerWidget2::navigateUp);
	p_toolBar->addAction(p_parentAction);

	// Address bar
	p_addressBar = new BreadcrumbsAddressBar(ptrTheme, this);
	connect(p_addressBar, &BreadcrumbsAddressBar::path_requested, this, &ExplorerWidget2::setCurrentPath);
	p_toolBar->addWidget(p_addressBar);

	// Menu bar
	QVBoxLayout* pLayout = new QVBoxLayout(this);
	pLayout->setContentsMargins(0, 0, 0, 0);
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
ErrorPtr ExplorerWidget2::init(const NavigationPath& path)
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
void ExplorerWidget2::setCurrentPath(const NavigationPath& path)
{
	if (p_wrapper)
	{
		// Already navigating ?
		if (m_onNavigation)
		{
			// Enqueue the request
			m_pendingPathes.push(path);
		}
		// Ready for action ?
		else
		{
			// Ask for navigation
			m_onNavigation = true;
			if (ErrorPtr pError = p_wrapper->setCurrentPath(path) )
			{
				displayError(pError);
				//onPathChanged(path, false);
			}
		}
	}
}
//
const NavigationPath& ExplorerWidget2::currentPath() const
{
	if (p_wrapper)
	{
		return p_wrapper->currentPath();
	}
	else
	{
		static const NavigationPath InvalidPath = {};

		qWarning() << "Unitialized object";
		return InvalidPath;
	}
}
//
void ExplorerWidget2::onLoading(const NavigationPath& path)
{
	p_addressBar->set_loading(path);
	emit loading(path);
}
//
void ExplorerWidget2::onPathChanged(const NavigationPath& path, bool success)
{
	// Process the path changed
	if (success)
	{
		// On success :
		// - update the navigation history
		m_navigationHistory.navigateTo(path);
		// - update the adress bar
		p_addressBar->set_path(path);
		// - enable/disable the buttons
		{
			std::optional<NavigationPath> backwardPath = m_navigationHistory.hasBackward();
			if (backwardPath.has_value())
			{
				p_backwardAction->setEnabled(true);
				// TODO use backwardPath.value().userPath()
				p_backwardAction->setText(tr("Navigate backward to \"%1\"").arg(backwardPath.value().displayPath()));
			}
			else
			{
				p_backwardAction->setEnabled(false);
				p_backwardAction->setText(tr("Navigate backward"));
			}
		}
		{
			std::optional<NavigationPath> forwardPath = m_navigationHistory.hasForward();
			if (forwardPath.has_value())
			{
				p_forwardAction->setEnabled(true);
				// TODO use forwardPath.value().userPath()
				p_forwardAction->setText(tr("Navigate forward to \"%1\"").arg(forwardPath.value().displayPath()));
			}
			else
			{
				p_forwardAction->setEnabled(false);
				p_forwardAction->setText(tr("Navigate forward"));
			}
		}
		p_parentAction->setEnabled(path.hasParent());
		// - emit the "path changed" signal
		emit pathChanged(path/*, success*/);
	}
	else
	{
		// On failure : preserve line address focus (if present) and inform the user
		p_addressBar->set_line_address_closeOnFocusOut(false);
		QMessageBox::critical(this, tr("SCEP"), tr("Could not navigate to \"%1\"").arg(path.displayPath()));
		p_addressBar->set_line_address_closeOnFocusOut(true);
	}

	// Is there any navigation request ?
	m_onNavigation = false;
	if (! m_pendingPathes.empty())
	{
		NavigationPath newPath = m_pendingPathes.front();
		m_pendingPathes.pop();
		setCurrentPath(newPath);
	}
}
//
void ExplorerWidget2::navigateBackward()
{

}
//
void ExplorerWidget2::navigateForward()
{

}
//
void ExplorerWidget2::navigateUp()
{
	const NavigationPath& path = currentPath();
	if (path.hasParent())
	{
		setCurrentPath(path.parent().value());
	}
}
//

