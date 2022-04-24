#include <SCEP/ExplorerWidget.h>
#include <SCEP/ExplorerWrapper.h>
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
ExplorerWidget::ExplorerWidget(Theme* ptrTheme, QWidget* pParent, Qt::WindowFlags f)
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
	connect(p_backwardAction, &QAction::triggered, this, &ExplorerWidget::navigateBackward);
	p_toolBar->addAction(p_backwardAction);

	// Forward button
	p_forwardAction = new QAction(ptr_theme->icon(Theme::Icon::Right), tr("Navigate forward"), this);
	p_forwardAction->setShortcut(Qt::ALT | Qt::Key_Right);
	//p_forwardMenu = new QMenu(tr("Forward"), this);
	//p_forwardMenu->addAction("toto");
	//p_forwardAction->setMenu(p_backwardMenu);
	p_forwardAction->setEnabled(false);
	connect(p_forwardAction, &QAction::triggered, this, &ExplorerWidget::navigateForward);
	p_toolBar->addAction(p_forwardAction);

	// Parent button
	p_parentAction = new QAction(ptr_theme->icon(Theme::Icon::Up), tr("Navigate to parent folder"), this);
	p_parentAction->setShortcut(Qt::ALT | Qt::Key_Up);
	//p_parentMenu = new QMenu(tr("Parent"), this);
	//p_parentMenu->addAction("toto");
	//p_parentAction->setMenu(p_parentMenu);
	p_parentAction->setEnabled(false);
	connect(p_parentAction, &QAction::triggered, this, &ExplorerWidget::navigateUp);
	p_toolBar->addAction(p_parentAction);

	// Address bar
	p_addressBar = new BreadcrumbsAddressBar(ptrTheme, this);
	connect(p_addressBar, &BreadcrumbsAddressBar::path_requested, this, &ExplorerWidget::setCurrentPath);
	p_toolBar->addWidget(p_addressBar);

	// Refresh button
	p_refreshAction = new QAction(ptr_theme->icon(Theme::Icon::Refresh), tr("Refresh"), this);
	p_refreshAction->setShortcut(Qt::Key_F5);
	connect(p_refreshAction, &QAction::triggered, this, &ExplorerWidget::refresh);
	p_toolBar->addAction(p_refreshAction);

	// Menu bar
	QVBoxLayout* pLayout = new QVBoxLayout(this);
	pLayout->setContentsMargins(0, 0, 0, 0);
	pLayout->setMenuBar(p_toolBar);
	setLayout(pLayout);

	// Explorer window wrapper
	p_wrapper = new ExplorerWrapper(ptr_theme);
	connect(p_wrapper, &ExplorerWrapper::loading, this, &ExplorerWidget::onLoading);
	connect(p_wrapper, &ExplorerWrapper::pathChanged, this, &ExplorerWidget::onPathChanged);
	connect(p_wrapper, &ExplorerWrapper::openNewTab, this, &ExplorerWidget::openNewTab);
}
//
ExplorerWidget::~ExplorerWidget()
{
	if (p_wrapper)
	{
		p_wrapper->finalize();
		p_wrapper->Release();
		p_wrapper = nullptr;
	}
}
//
ErrorPtr ExplorerWidget::init(const NavigationPath& path)
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
void ExplorerWidget::setCurrentPath(const NavigationPath& path)
{
	navigateTo(path);
}
//
const NavigationPath& ExplorerWidget::currentPath() const
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
void ExplorerWidget::rename()
{
	if (p_wrapper && (GetFocus() != (HWND) window()->winId()))
	{
		p_wrapper->rename();
	}
}
//
void ExplorerWidget::copy()
{
	if (p_wrapper && (GetFocus() != (HWND) window()->winId()))
	{
		p_wrapper->copy();
	}
}
//
void ExplorerWidget::cut()
{
	if (p_wrapper && (GetFocus() != (HWND) window()->winId()))
	{
		p_wrapper->cut();
	}
}
//
void ExplorerWidget::paste()
{
	if (p_wrapper && (GetFocus() != (HWND) window()->winId()))
	{
		p_wrapper->paste();
	}
}
//
void ExplorerWidget::showAddressBar()
{
	p_addressBar->show_address_field(true);
}
//
void ExplorerWidget::del()
{
	if (p_wrapper && (GetFocus() != (HWND) window()->winId()))
	{
		p_wrapper->del();
	}
}
//
void ExplorerWidget::forceDel()
{
	if (p_wrapper && (GetFocus() != (HWND) window()->winId()))
	{
		p_wrapper->forceDel();
	}
}
//
void ExplorerWidget::selectAll()
{
	if (p_wrapper && (GetFocus() != (HWND) window()->winId()))
	{
		p_wrapper->selectAll();
	}
}
//
void ExplorerWidget::mkDir()
{
	if (p_wrapper)
	{
		p_wrapper->mkDir();
	}
}
//
void ExplorerWidget::undo()
{
	if (p_wrapper && (GetFocus() != (HWND) window()->winId()))
	{
		p_wrapper->undo();
	}
}
//
void ExplorerWidget::redo()
{
	if (p_wrapper && (GetFocus() != (HWND) window()->winId()))
	{
		p_wrapper->redo();
	}
}
//
void ExplorerWidget::refresh()
{
	if (p_wrapper)
	{
		p_wrapper->refresh();
	}
}
//
void ExplorerWidget::navigateBackward()
{
	if (m_navigationHistory.hasBackward().has_value())
	{
		NavigationHistory new_history = m_navigationHistory;
		std::optional<NavigationPath> new_path = new_history.navigateBackward();
		assert(new_path.has_value());
		if (new_path.has_value())
		{
			navigateTo({new_path.value(), new_history});
		}
	}
}
//
void ExplorerWidget::navigateForward()
{
	if (m_navigationHistory.hasForward().has_value())
	{
		NavigationHistory new_history = m_navigationHistory;
		std::optional<NavigationPath> new_path = new_history.navigateForward();
		assert(new_path.has_value());
		if (new_path.has_value())
		{
			navigateTo({new_path.value(), new_history});
		}
	}
}
//
void ExplorerWidget::navigateUp()
{
	const NavigationPath& path = currentPath();
	if (path.hasParent())
	{
		navigateTo(path.parent().value());
	}
}
//
void ExplorerWidget::navigateTo(const NavigationRequest& request)
{
	if (p_wrapper)
	{
		// Already navigating ?
		if (m_currentRequest.has_value())
		{
			// Enqueue the request
			m_pendingRequests.push(request);
		}
		// Ready for action ?
		else
		{
			// Ask for navigation
			m_currentRequest = request;
			if (ErrorPtr pError = p_wrapper->setCurrentPath(request.path) )
			{
				displayError(pError);
			}
		}
	}
}
//
void ExplorerWidget::onLoading(const NavigationPath& path)
{
	p_addressBar->set_loading(path);
	emit loading(path);
}
//
void ExplorerWidget::onPathChanged(const NavigationPath& path, bool success)
{
	// On success
	if (success)
	{
		// Get current request
		// - there is a current request if the user makes use of the navigation buttons or the crumbread bar
		// - there is no current request if the user navigates with the explorer
		//   Caution : if there was a request but the path does not correspond, simply ignore the request
		//   (and especially the associated history, if any)
		std::optional<NavigationRequest>& request = m_currentRequest;
		if (request.has_value() && request.value().path != path)
		{
			qWarning() << "Navigation to " << request.value().path.displayPath() << " was requested, but the browser navigated to " << path.displayPath();
			request = std::nullopt;
		}

		// On success :
		// - update the navigation history
		if (request.has_value() && request.value().history.has_value())
			m_navigationHistory = request.value().history.value();
		else
			m_navigationHistory.navigateTo(path);
		// - update the adress bar
		p_addressBar->set_path(path);
		// - enable/disable the buttons
		{
			std::optional<NavigationPath> backwardPath = m_navigationHistory.hasBackward();
			if (backwardPath.has_value())
			{
				p_backwardAction->setEnabled(true);
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
	//  On failure
	else
	{
		// Preserve line address focus (if present) and inform the user
		p_addressBar->set_line_address_closeOnFocusOut(false);
		QMessageBox::critical(this, tr("SCEP"), tr("Could not navigate to \"%1\"").arg(path.displayPath()));
		p_addressBar->set_line_address_closeOnFocusOut(true);
	}

	// Is there any navigation request ?
	m_currentRequest = std::nullopt;
	if (! m_pendingRequests.empty())
	{
		NavigationRequest request = m_pendingRequests.front();
		m_pendingRequests.pop();
		navigateTo(request);
	}
}
//
