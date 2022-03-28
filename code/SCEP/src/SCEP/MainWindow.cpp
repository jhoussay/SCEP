#include <SCEP/MainWindow.h>
#include <SCEP/AboutDialog.h>
#include <SCEP/ExplorerWidget.h>
//
#include <ui_MainWindow.h>
//
#include <QSettings>
#include <QTabBar>
#include <QToolButton>
#include <QMenu>
#include <QStandardPaths>
#include <QLabel>
#include <QMovie>
#include <QShortcut>
#include <QWidgetAction>
#include <QRadioButton>
#include <QButtonGroup>
#include <QMouseEvent>
//
#ifdef FRAMELESS
#include <QPushButton>
#endif //FRAMELESS
//
static const QString GeometryStr = "Geometry";
static const QString StateStr = "State";
static const QString TabsStr = "Tabs";
static const QString CurrentTabStr = "CurrentTab";
//
MainWindow::MainWindow(Theme* ptrTheme, QSettings* ptrSettings)
	:	QMainWindow()
	,	ptr_theme(ptrTheme)
	,	ptr_settings(ptrSettings)
{
#ifdef FRAMELESS
	//new FrameLess(this);
	setWindowFlag(Qt::FramelessWindowHint);
	setMouseTracking(true);
#endif //FRAMELESS

	p_ui = new Ui::MainWindow();
	p_ui->setupUi(this);
	p_ui->tabWidget->setTabsClosable(false);
	p_ui->tabWidget->tabBar()->installEventFilter(this);

#ifdef FRAMELESS
	QIcon icon = qApp->windowIcon();
	QLabel* pLabel = new QLabel(this);
	//pLabel->setPixmap(icon.pixmap(24));
	pLabel->setText("Icon");
	p_ui->tabWidget->setCornerWidget(pLabel, Qt::TopLeftCorner);

	QPushButton* pCloseButton = new QPushButton("Close");
	connect(pCloseButton, SIGNAL(clicked()), qApp, SLOT(quit()));
	p_ui->tabWidget->setCornerWidget(pCloseButton, Qt::TopRightCorner);
#endif //FRAMELESS


	// Actions
	//////////

	p_addTabAction = new QAction(tr("Add new tab"), this);
	connect(p_addTabAction, SIGNAL(triggered()), this, SLOT(addNewTab()));
	p_addTabAction->setShortcutContext(Qt::ApplicationShortcut);
	p_addTabAction->setShortcut(Qt::CTRL | Qt::Key_T);
	//QShortcut* pAddTabShortcut = new QShortcut(Qt::CTRL | Qt::Key_T, this, [this](){ this->addNewTab(); });
	//pAddTabShortcut->setContext(Qt::ApplicationShortcut);

	p_closeTabAction = new QAction(tr("Close current tab"), this);
	connect(p_closeTabAction, SIGNAL(triggered()), this, SLOT(closeCurrentTab()));
	p_closeTabAction->setShortcutContext(Qt::ApplicationShortcut);
	p_closeTabAction->setShortcut(Qt::CTRL | Qt::Key_W);
	//QShortcut* pCloseTabShortcut = new QShortcut(Qt::CTRL | Qt::Key_W, this, [this](){ this->closeCurrentTab(); });
	//pCloseTabShortcut->setContext(Qt::ApplicationShortcut);


	QWidget* pThemeWidget = new QWidget(this);
	QHBoxLayout* pThemeLayout = new QHBoxLayout(pThemeWidget);
	QLabel* pThemeLabel = new QLabel(tr("Theme : "), pThemeWidget);
	pThemeLayout->addWidget(pThemeLabel);
	p_themeButtonGroup = new QButtonGroup(pThemeWidget);
	p_autoThemeButton = new QRadioButton(tr("Auto"), pThemeWidget);
	p_autoThemeButton->setChecked(ptr_theme->userStyle() == Theme::Style::Auto);
	p_themeButtonGroup->addButton(p_autoThemeButton, (int) Theme::Style::Auto);
	pThemeLayout->addWidget(p_autoThemeButton);
	p_lightThemeButton = new QRadioButton(tr("Light"), pThemeWidget);
	p_lightThemeButton->setChecked(ptr_theme->userStyle() == Theme::Style::Light);
	p_themeButtonGroup->addButton(p_lightThemeButton, (int) Theme::Style::Light);
	pThemeLayout->addWidget(p_lightThemeButton);
	p_darkThemeButton = new QRadioButton(tr("Dark"), pThemeWidget);
	p_darkThemeButton->setChecked(ptr_theme->userStyle() == Theme::Style::Dark);
	p_themeButtonGroup->addButton(p_darkThemeButton, (int) Theme::Style::Dark);
	pThemeLayout->addWidget(p_darkThemeButton);
	pThemeWidget->setLayout(pThemeLayout);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	connect(p_themeButtonGroup, qOverload<int>(&QButtonGroup::buttonPressed), this, &MainWindow::styleChanged);
#else
	connect(p_themeButtonGroup, &QButtonGroup::idToggled, this, &MainWindow::styleChanged);
#endif
	p_themeAction = new QWidgetAction(this);
	p_themeAction->setDefaultWidget(pThemeWidget);

	pThemeWidget->setToolTip(tr("Any style change will take effect on restart"));
	//QLabel* pThemeTooltipLabel = new QLabel(tr("(restart required)"), this);
	//pThemeTooltipLabel->setAlignment(Qt::AlignRight);
	//QFont font = pThemeTooltipLabel->font();
	//font.setItalic(true);
	//pThemeTooltipLabel->setFont(font);
	//p_themeTooltipAction = new QWidgetAction(this);
	//p_themeTooltipAction->setDefaultWidget(pThemeTooltipLabel);

	p_aboutAction = new QAction(tr("About SCEP..."), this);
	connect(p_aboutAction, SIGNAL(triggered()), this, SLOT(about()));
	//p_aboutAction->setShortcut();


	// Buttons
	//////////

	p_addTabButton = new QToolButton(this);
	p_addTabButton->setAutoRaise(true);
	p_addTabButton->setToolTip( p_addTabAction->text() );
	connect(p_addTabButton, SIGNAL(clicked()), p_addTabAction, SLOT(trigger()));
	p_ui->tabWidget->setCornerWidget(p_addTabButton, Qt::TopLeftCorner);

	p_menuButton = new QToolButton(this);
	p_menuButton->setAutoRaise(true);
	p_menuButton->setToolTip( tr("Menu") );
	connect(p_menuButton, SIGNAL(clicked()), this, SLOT(showMenu()));
	p_ui->tabWidget->setCornerWidget(p_menuButton, Qt::TopRightCorner);

	updateIcons();

	// Geometry
	///////////

	restoreState(ptr_settings->value(StateStr).toByteArray());
	restoreGeometry(ptr_settings->value(GeometryStr).toByteArray());


	// Tabs
	///////

	QStringList tabs = ptr_settings->value(TabsStr).toStringList();
	if (tabs.isEmpty())
	{
		addNewTab();
	}
	else
	{
		for (const QString& tab : tabs)
		{
			NavigationPath path(tab);
			if (path.isExistingDirectory())
			{
				addNewTab(path, NewTabPosition::Last, NewTabBehaviour::None);
			}
			else
			{
				qWarning() << "Cannot restore tab on " << tab << " because it does not exist anymore or cannot be reached.";
			}
		}

		int currentTab = ptr_settings->value(CurrentTabStr, 0).toInt();
		if ( (currentTab >= 0) && (currentTab < p_ui->tabWidget->count()) )
		{
			p_ui->tabWidget->setCurrentIndex(currentTab);
		}
		else
		{
			qWarning() << "Invalid current index : " << currentTab;
		}
	}
}
//
MainWindow::~MainWindow()
{
	ptr_settings->setValue(StateStr, saveState());
	ptr_settings->setValue(GeometryStr, saveGeometry());
	ptr_settings->sync();

	delete p_ui;
	p_ui = nullptr;
}
//
#ifdef FRAMELESS
//
void MainWindow::mouseDoubleClickEvent(QMouseEvent* pEvent)
{
	int y_tabBarFrame = p_ui->tabWidget->tabBar()->mapFromGlobal(pEvent->globalPos()).y();
	int tabBarHeight = p_ui->tabWidget->tabBar()->rect().height();
	if (y_tabBarFrame < tabBarHeight)
	{
		if (isMaximized())
			showNormal();
		else
			showMaximized();
		pEvent->accept();
	}
	else
	{
		QMainWindow::mouseDoubleClickEvent(pEvent);
	}
}

void MainWindow::mousePressEvent(QMouseEvent* pEvent)
{
	int y_tabBarFrame = p_ui->tabWidget->tabBar()->mapFromGlobal(pEvent->globalPos()).y();
	int tabBarHeight = p_ui->tabWidget->tabBar()->rect().height();
	if (pEvent->button() == Qt::LeftButton && (y_tabBarFrame < tabBarHeight))
	{
		m_dragPosition = pEvent->globalPos() - frameGeometry().topLeft();
		pEvent->accept();
	}
	else
	{
		QMainWindow::mousePressEvent(pEvent);
	}
}

void MainWindow::mouseReleaseEvent(QMouseEvent* pEvent)
{
	m_dragPosition = std::nullopt;
	QMainWindow::mouseReleaseEvent(pEvent);
}

void MainWindow::mouseMoveEvent(QMouseEvent* pEvent)
{
	if (/*pEvent->button() == Qt::LeftButton &&*/ m_dragPosition.has_value())
	{
		move(pEvent->globalPos() - m_dragPosition.value());
		pEvent->accept();
	}
	else
	{
		QMainWindow::mouseMoveEvent(pEvent);
	}
}
//
#endif //FRAMELESS
//
void MainWindow::addNewTab(NavigationPath path, NewTabPosition position, NewTabBehaviour behaviour)
{
	if (path.empty())
	{
		path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
	}

	setUpdatesEnabled(false);

	ExplorerWidget* pExplorerWidget = new ExplorerWidget(ptr_theme);
	connect(pExplorerWidget, &ExplorerWidget::loading, this, &MainWindow::loading);
	connect(pExplorerWidget, &ExplorerWidget::pathChanged, this, &MainWindow::pathChanged);
	connect(pExplorerWidget, &ExplorerWidget::openNewTab, this, &MainWindow::addNewTab);
	connect(pExplorerWidget, &ExplorerWidget::closed, this, &MainWindow::tabClosed);
	if (ErrorPtr pError = pExplorerWidget->init(path))
	{
		displayError(pError);
		delete pExplorerWidget;
		pExplorerWidget = nullptr;
	}
	else
	{
		int tabIndex = p_ui->tabWidget->count();
		if ( (position == NewTabPosition::AfterCurrent) && (p_ui->tabWidget->count() > 0) )
		{
			tabIndex = p_ui->tabWidget->currentIndex() + 1;
		}
		p_ui->tabWidget->insertTab(tabIndex, pExplorerWidget, tabName(pExplorerWidget->currentPath()));

		// Tab close button
		QWidget* pCloseWidget = new QWidget(this);
		QToolButton* pCloseButton = new QToolButton(pCloseWidget);
		pCloseButton->setToolTip(tr("Close tab"));
		pCloseButton->setAutoRaise(true);
		pCloseButton->setIcon( ptr_theme->icon(Theme::Icon::CloseTab) );
		pCloseButton->setMaximumSize( p_addTabButton->size() );
		connect(pCloseButton, SIGNAL(clicked()), this, SLOT(onTabCloseRequested()));
		// Layout
		QSpacerItem* pCloseSpacer = new QSpacerItem(10, 5, QSizePolicy::Fixed, QSizePolicy::Minimum);
		QHBoxLayout* pCloseLayout = new QHBoxLayout(pCloseWidget);
		pCloseWidget->setLayout(pCloseLayout);
		pCloseLayout->setContentsMargins(0, 0, 0, 0);
		pCloseLayout->addWidget(pCloseButton);
		pCloseLayout->addItem(pCloseSpacer);
		p_ui->tabWidget->tabBar()->setTabButton(tabIndex, QTabBar::RightSide, pCloseWidget);

		// Tab icon
		QWidget* pIconWidget = new QWidget(this);
		QLabel* pIconLabel = new QLabel(pIconWidget);
		pIconLabel->setText({});
		pIconLabel->setMinimumSize( 20, 20 );
		pIconLabel->setMaximumSize( 20, 20 );
		pIconLabel->setScaledContents(true);
		pIconLabel->setPixmap(path.pixmap(QSize(32, 32)));
		// Layout
		QSpacerItem* pIconSpacer = new QSpacerItem(10, 5, QSizePolicy::Fixed, QSizePolicy::Minimum);
		QHBoxLayout* pIconLayout = new QHBoxLayout(pIconWidget);
		pIconWidget->setLayout(pIconLayout);
		pIconLayout->setContentsMargins(0, 0, 0, 0);
		pIconLayout->addItem(pIconSpacer);
		pIconLayout->addWidget(pIconLabel);
		p_ui->tabWidget->tabBar()->setTabButton(tabIndex, QTabBar::LeftSide, pIconWidget);

		if (behaviour == NewTabBehaviour::Current)
			p_ui->tabWidget->setCurrentIndex(tabIndex);
	}

	setUpdatesEnabled(true);
}
//
void MainWindow::closeCurrentTab()
{
	closeTab(p_ui->tabWidget->currentIndex());
}
//
void MainWindow::showMenu()
{
	// Create menu
	QMenu menu(this);
	menu.addAction(p_addTabAction);
	menu.addAction(p_closeTabAction);
	menu.addSeparator();
	menu.addAction(p_aboutAction);
	menu.addSeparator();
	menu.addAction(p_themeAction);
	//menu.addAction(p_themeTooltipAction);

	// Get menu real width
	menu.show();
	menu.hide();
	int w = menu.width();

	// Set menu position right aligned with the menu button and just besides it
	QPoint topRightPos = p_menuButton->rect().bottomRight();
	QPoint topLeftPos(topRightPos.x() - w, topRightPos.y());

	// Show menu
	menu.exec(p_menuButton->mapToGlobal(topLeftPos));
}
//
void MainWindow::about()
{
	AboutDialog about(this);
	about.exec();
}
//
int getCloseButtonIndex(QToolButton* pToolButton, QTabWidget* pTabWidget)
{
	for (int tabIndex = 0; tabIndex < pTabWidget->count(); tabIndex++)
	{
		if (QWidget* pWidget = pTabWidget->tabBar()->tabButton(tabIndex, QTabBar::RightSide))
		{
			if (QLayout* pLayout = pWidget->layout())
			{
				if (QLayoutItem* pLayoutItem = pLayout->itemAt(0))
				{
					if (QToolButton* pToolButtonTmp = dynamic_cast<QToolButton*>(pLayoutItem->widget()))
					{
						if (pToolButton == pToolButtonTmp)
						{
							return tabIndex;
						}
					}
				}
			}
		}
	}
	return -1;
}
//
int getTabIndex(ExplorerWidget* pExplorerWidget, QTabWidget* pTabWidget)
{
	for (int tabIndex = 0; tabIndex < pTabWidget->count(); tabIndex++)
	{
		if (pTabWidget->widget(tabIndex) == pExplorerWidget)
		{
			return tabIndex;
		}
	}
	return -1;
}
//
QLabel* getIconLabel(ExplorerWidget* pExplorerWidget, QTabWidget* pTabWidget)
{
	int tabIndex = getTabIndex(pExplorerWidget, pTabWidget);
	if (tabIndex >= 0)
	{
		if (QWidget* pWidget = pTabWidget->tabBar()->tabButton(tabIndex, QTabBar::LeftSide))
		{
			if (QLayout* pLayout = pWidget->layout())
			{
				if (QLayoutItem* pLayoutItem = pLayout->itemAt(1))
				{
					if (QLabel* pLabel = dynamic_cast<QLabel*>(pLayoutItem->widget()))
					{
						return pLabel;
					}
				}
			}
		}
	}
	return nullptr;
}
//
void MainWindow::styleChanged(int style)
{
	Theme::Style userStyle = (Theme::Style) style;
	ptr_theme->setUserStyle(userStyle);
}
//
void MainWindow::onTabCloseRequested()
{
	if (QToolButton* pToolButton = dynamic_cast<QToolButton*>(sender()))
	{
		int tabIndex = getCloseButtonIndex(pToolButton, p_ui->tabWidget);
		if (tabIndex >= 0)
		{
			closeTab(tabIndex);
		}
	}
}
//
void MainWindow::loading(const NavigationPath& /*path*/)
{
	if (ExplorerWidget* pExplorerWidget = dynamic_cast<ExplorerWidget*>(sender()))
	{
		if (QLabel* pLabel = getIconLabel(pExplorerWidget, p_ui->tabWidget))
		{
			pLabel->setPixmap({});
			QMovie* pMovie = new QMovie(":/SCEP/icons/buffering2.gif", QByteArray(), pLabel);
			pLabel->setMovie(pMovie);
			pMovie->start();
		}
	}
}
//
void MainWindow::pathChanged(const NavigationPath& path)
{
	if (ExplorerWidget* pExplorerWidget = dynamic_cast<ExplorerWidget*>(sender()))
	{
		int tabIndex = getTabIndex(pExplorerWidget, p_ui->tabWidget);
		if (tabIndex >= 0)
			p_ui->tabWidget->setTabText(tabIndex, tabName(path));

		if (QLabel* pLabel = getIconLabel(pExplorerWidget, p_ui->tabWidget))
		{
			pLabel->setMovie({});
			pLabel->setPixmap(path.pixmap(QSize(32, 32)));
		}
	}
}
//
void MainWindow::tabClosed()
{
	// TODO
}
//
void MainWindow::closeEvent(QCloseEvent* pEvent)
{
	// Current tabs init
	int currentTab = p_ui->tabWidget->currentIndex();
	QStringList tabs;

	// Current tabs (while closing them in order to prevent any handle leak)
	while (p_ui->tabWidget->count() > 0)
	{
		ExplorerWidget* pExplorerWidget = dynamic_cast<ExplorerWidget*>(p_ui->tabWidget->widget(0));
		assert(pExplorerWidget);
		if (pExplorerWidget)
			tabs << pExplorerWidget->currentPath().internalPath();
		closeTab(0, false);
	}

	// Saving tabs
	ptr_settings->setValue(TabsStr, tabs);
	ptr_settings->setValue(CurrentTabStr, currentTab);
	ptr_settings->sync();

	// Close
	QMainWindow::closeEvent(pEvent);
}
//
bool MainWindow::eventFilter([[maybe_unused]] QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::MouseButtonPress)
	{
		QMouseEvent* pMouseEvent = dynamic_cast<QMouseEvent*>(event);
		assert(pMouseEvent);
		if (pMouseEvent->button() == Qt::MiddleButton)
		{
			QTabBar* pTabBar = p_ui->tabWidget->tabBar();
			assert(obj == pTabBar);
			QPoint pos = pTabBar->mapFromGlobal(pMouseEvent->globalPos());
			int tabIndex = p_ui->tabWidget->tabBar()->tabAt(pos);
			if (tabIndex >= 0)
			{
				closeTab(tabIndex);
				return true;
			}
		}
	}
	return false;
}
//
void MainWindow::closeTab(int tabIndex, bool closeAppIfNoRemainingTab)
{
	if (tabIndex >= 0)
	{
		if (QWidget* pWidget = p_ui->tabWidget->widget(tabIndex))
		{
			p_ui->tabWidget->removeTab(tabIndex);
			delete pWidget;
			pWidget = nullptr;

			if (closeAppIfNoRemainingTab && p_ui->tabWidget->count() == 0)
				close();
		}
	}
}
//
void MainWindow::updateIcons()
{
	p_addTabAction->setIcon( ptr_theme->icon(Theme::Icon::AddTab) );
	p_addTabButton->setIcon( ptr_theme->icon(Theme::Icon::AddTab) );

	p_closeTabAction->setIcon( ptr_theme->icon(Theme::Icon::CloseTab) );

	p_menuButton->setIcon( ptr_theme->icon(Theme::Icon::Menu) );
}
//
QString MainWindow::tabName(const NavigationPath& path)
{
	if (path.valid())
		return path.label();
	else
		return "Explorer";
}
//
