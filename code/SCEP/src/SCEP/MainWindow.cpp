#include <SCEP/MainWindow.h>
#include <SCEP/AboutDialog.h>
//
#include <ui_MainWindow.h>
//
#include <QTabBar>
#include <QToolButton>
#include <QMenu>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QLabel>
#include <QMovie>
#include <QShortcut>
//
#ifdef FRAMELESS
#include <QMouseEvent>
#include <QPushButton>
#endif //FRAMELESS
//
#define EXPLORER_WIDGET_VERSION 2
// 
#if EXPLORER_WIDGET_VERSION == 1
#	include <SCEP/ExplorerWidget.h>
#else
#	include <SCEP/ExplorerWidget2.h>
#	define ExplorerWidget ExplorerWidget2
#endif //EXPLORER_WIDGET_VERSION
//
MainWindow::MainWindow(Theme* ptrTheme)
	:	QMainWindow()
	,	ptr_theme(ptrTheme)
{
#ifdef FRAMELESS
	//new FrameLess(this);
	setWindowFlag(Qt::FramelessWindowHint);
	setMouseTracking(true);
#endif //FRAMELESS

	p_ui = new Ui::MainWindow();
	p_ui->setupUi(this);
	p_ui->tabWidget->setTabsClosable(false);

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

	addNewTab();
}
//
MainWindow::~MainWindow()
{
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
void MainWindow::addNewTab(QString path, NewTabPosition position, NewTabBehaviour behaviour)
{
	if (path.isEmpty())
	{
		path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
	}

	setUpdatesEnabled(false);

	ExplorerWidget* pExplorerWidget = new ExplorerWidget();
	connect(pExplorerWidget, &ExplorerWidget::loading, this, &MainWindow::loading);
	connect(pExplorerWidget, &ExplorerWidget::pathChanged, this, &MainWindow::pathChanged);
	connect(pExplorerWidget, &ExplorerWidget::openNewTab, this, &MainWindow::addNewTab);
	connect(pExplorerWidget, &ExplorerWidget::closed, this, &MainWindow::tabClosed);
	if (ErrorPtr pError = pExplorerWidget->init(ptr_theme, path))
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
		p_ui->tabWidget->insertTab(tabIndex, pExplorerWidget, tabName(pExplorerWidget->currentPath(), false));

		QToolButton* pCloseButton = new QToolButton();
		pCloseButton->setToolTip(tr("Close tab"));
		pCloseButton->setAutoRaise(true);
		pCloseButton->setIcon( ptr_theme->icon(Theme::Icon::CloseTab) );
		pCloseButton->setMaximumSize( p_addTabButton->size() );
		connect(pCloseButton, SIGNAL(clicked()), this, SLOT(onTabCloseRequested()));
		p_ui->tabWidget->tabBar()->setTabButton(tabIndex, QTabBar::RightSide, pCloseButton);

		QLabel* pLabel = new QLabel();
		pLabel->setText({});
		pLabel->setMinimumSize( 20, 20 );
		pLabel->setMaximumSize( 20, 20 );
		pLabel->setScaledContents(true);
		pLabel->setPixmap(m_fileIconProvider.icon(QFileInfo(path)).pixmap(QSize(32, 32)));
		p_ui->tabWidget->tabBar()->setTabButton(tabIndex, QTabBar::LeftSide, pLabel);

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
	menu.addAction(p_aboutAction);

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
void MainWindow::onTabCloseRequested()
{
	if (QToolButton* pToolButton = dynamic_cast<QToolButton*>(sender()))
	{
		for (int tabIndex = 0; tabIndex < p_ui->tabWidget->count(); tabIndex++)
		{
			if (p_ui->tabWidget->tabBar()->tabButton(tabIndex, QTabBar::RightSide) == pToolButton)
			{
				closeTab(tabIndex);
				break;
			}
		}
	}
}
//
void MainWindow::loading(const QString& /*path*/)
{
	if (ExplorerWidget* pExplorerWidget = dynamic_cast<ExplorerWidget*>(sender()))
	{
		for (int tabIndex = 0; tabIndex < p_ui->tabWidget->count(); tabIndex++)
		{
			if (p_ui->tabWidget->widget(tabIndex) == pExplorerWidget)
			{
				QLabel* pLabel = dynamic_cast<QLabel*>(p_ui->tabWidget->tabBar()->tabButton(tabIndex, QTabBar::LeftSide));
				if (pLabel)
				{
					pLabel->setPixmap({});
					QMovie* pMovie = new QMovie(":/SCEP/icons/buffering2.gif", QByteArray(), pLabel);
					pLabel->setMovie(pMovie);
					pMovie->start();
				}

				break;
			}
		}
	}
}
//
void MainWindow::pathChanged(const QString& path, bool virtualFolder)
{
	if (ExplorerWidget* pExplorerWidget = dynamic_cast<ExplorerWidget*>(sender()))
	{
		for (int tabIndex = 0; tabIndex < p_ui->tabWidget->count(); tabIndex++)
		{
			if (p_ui->tabWidget->widget(tabIndex) == pExplorerWidget)
			{
				p_ui->tabWidget->setTabText(tabIndex, tabName(path, virtualFolder));

				QLabel* pLabel = dynamic_cast<QLabel*>(p_ui->tabWidget->tabBar()->tabButton(tabIndex, QTabBar::LeftSide));
				if (pLabel)
				{
					pLabel->setMovie({});
					pLabel->setPixmap(m_fileIconProvider.icon(QFileInfo(path)).pixmap(QSize(32, 32)));
				}



				break;
			}
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
	while (p_ui->tabWidget->count() > 0)
	{
		closeTab(0, false);
	}
	QMainWindow::closeEvent(pEvent);
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
QString MainWindow::tabName(const QString& tabPath, bool virtualFolder)
{
	if (virtualFolder)
	{
		return tabPath;
	}
	else
	{
		QFileInfo fi(tabPath);
		QString name = fi.isDir() ? fi.fileName() : fi.absoluteDir().dirName();
		// '&' creates a shortcut --> TODO need to espace it using '&&' instead
		return tabPath.isEmpty() || name.isEmpty() ? "Explorer" : name;
	}
}
//
