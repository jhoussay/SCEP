#include <SCEP/MainWindow.h>
#include <SCEP/ExplorerWidget.h>
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
//
#ifdef FRAMELESS
#include <QMouseEvent>
#include <QPushButton>
#include <QLabel>
#endif //FRAMELESS
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

	//connect(p_ui->tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(onTabCloseRequested(int)));
	p_ui->tabWidget->setTabsClosable(false);

	p_addTabButton = new QToolButton(this);
	p_addTabButton->setAutoRaise(true);
	p_addTabButton->setToolTip( tr("Add new tab") );
	connect(p_addTabButton, SIGNAL(clicked()), this, SLOT(addNewTab()));
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
void MainWindow::addNewTab(QString path)
{
	if (path.isEmpty())
	{
		path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
	}

	ExplorerWidget* pExplorerWidget = new ExplorerWidget();
	connect(pExplorerWidget, SIGNAL(pathChanged(QString)), this, SLOT(pathChanged(QString)));
	connect(pExplorerWidget, SIGNAL(closed()), this, SLOT(closed()));
	if (ErrorPtr pError = pExplorerWidget->init(path))
	{
		displayError(pError);
		delete pExplorerWidget;
		pExplorerWidget = nullptr;
	}
	else
	{
		p_ui->tabWidget->addTab(pExplorerWidget, tabName(pExplorerWidget->currentPath()));

		int tabIndex = p_ui->tabWidget->count()-1;
		QToolButton* pCloseButton = new QToolButton();
		pCloseButton->setToolTip(tr("Close tab"));
		pCloseButton->setAutoRaise(true);
		pCloseButton->setIcon( ptr_theme->icon(Theme::Icon::CloseTab) );
		pCloseButton->setMaximumSize( p_addTabButton->size() );
		connect(pCloseButton, SIGNAL(clicked()), this, SLOT(onTabCloseRequested()));
		p_ui->tabWidget->tabBar()->setTabButton(tabIndex, QTabBar::RightSide, pCloseButton);

		p_ui->tabWidget->setCurrentIndex(tabIndex);
	}
}
//
void MainWindow::showMenu()
{
	// Create menu
	QMenu menu(this);
	menu.addAction(ptr_theme->icon(Theme::Icon::AddTab), tr("Add tab"));
	menu.addAction(ptr_theme->icon(Theme::Icon::CloseTab), tr("Close current tab"));
	menu.addAction(ptr_theme->icon(Theme::Icon::About), tr("About SCEP..."), this, &MainWindow::about);

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
				QWidget* pWidget = p_ui->tabWidget->widget(tabIndex);
				p_ui->tabWidget->removeTab(tabIndex);
				delete pWidget;
				pWidget = nullptr;

				if (p_ui->tabWidget->count() == 0)
					close();

				break;
			}
		}
	}
}
//
void MainWindow::pathChanged(QString path)
{
	if (ExplorerWidget* pExplorerWidget = dynamic_cast<ExplorerWidget*>(sender()))
	{
		for (int tabIndex = 0; tabIndex < p_ui->tabWidget->count(); tabIndex++)
		{
			if (p_ui->tabWidget->widget(tabIndex) == pExplorerWidget)
			{
				p_ui->tabWidget->setTabText(tabIndex, tabName(path));
				break;
			}
		}
	}
}
//
void MainWindow::closed()
{
	// TODO
}
//
void MainWindow::updateIcons()
{
	p_addTabButton->setIcon( ptr_theme->icon(Theme::Icon::AddTab) );
	p_menuButton->setIcon( ptr_theme->icon(Theme::Icon::Menu) );
}
//
QString MainWindow::tabName(const QString& tabPath)
{
	QFileInfo fi(tabPath);
	QString name = fi.isDir() ? fi.fileName() : fi.absoluteDir().dirName();
	// '&' creates a shortcut --> TODO need to espace it using '&&' instead
	return tabPath.isEmpty() || name.isEmpty() ? "Explorer" : name;
}
//
