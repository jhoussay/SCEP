#include <SCEP/ExplorerWidget.h>
#include <SCEP/ExplorerWrapper.h>
//
#include <QWindow>
#include <QVBoxLayout>
//
#include <uxtheme.h>
//
LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
	{
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}
//
HWND CreateTheWindow(LPCWSTR WindowTitle)
{
	static LPCWSTR sClassName = L"MyClass";

	// Create & register the class
	WNDCLASSEX WndClass;
	WndClass.cbSize = sizeof(WNDCLASSEX);
	WndClass.style = 0u;
	WndClass.lpfnWndProc = WndProc; 
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.lpszClassName = sClassName;
	WndClass.hInstance = nullptr;
	WndClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION); 
	WndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	WndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	WndClass.lpszMenuName  = nullptr;
	WndClass.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
	RegisterClassEx(&WndClass);

	// Create & show the window
	HWND hwnd = CreateWindowEx(	WS_EX_STATICEDGE,
								sClassName,
								WindowTitle,
								WS_OVERLAPPEDWINDOW,
								CW_USEDEFAULT,
								CW_USEDEFAULT,
								320,
								240,
								nullptr,
								nullptr,
								nullptr,
								nullptr);

	UpdateWindow(hwnd);
	return hwnd;
}
//
//
//
ExplorerWidget::ExplorerWidget(QWidget* pParent, Qt::WindowFlags f)
	:	QWidget(pParent, f)
{
	p_wrapper = new ExplorerWrapper();
	connect(p_wrapper, SIGNAL(pathChanged(QString)), this, SIGNAL(pathChanged(QString)));
	connect(p_wrapper, SIGNAL(closed()), this, SIGNAL(closed()));

	setMinimumWidth(200);
	setMinimumHeight(200);
}
//
ExplorerWidget::~ExplorerWidget()
{
	delete p_wrapper;
	p_wrapper = nullptr;
}
//
ErrorPtr ExplorerWidget::init(const QString& path)
{
	// Initialize the explorer
	CALL( p_wrapper->initialize(path) );

	// The trick !!!
	// - new win32 window (proxy) -> can embed a window from another process
	// - embedding Qt widget : can embed the proxy window because it belongs to the same process
	//                         while it can't directly embed the explorer window !

	// Create a new empty window
	HWND windowId = CreateTheWindow(L"Win32 SCEP");
	CHECK(windowId, "Could not create win32 window");

	// Change explorer window parent to the newly created window
	HWND explorerId = p_wrapper->hwnd();
	HWND oldId = SetParent(explorerId, windowId);
	CHECK(oldId, "Could not change explorer window parent : " + QString::number(GetLastError()));
	LONG rslt = SetWindowLong(explorerId, GWL_STYLE, WS_CHILDWINDOW | WS_VISIBLE);
	CHECK(rslt != 0, "Could not change explorer window style : " + QString::number(GetLastError()));
	BOOL ok = UpdateWindow(explorerId);
	CHECK(ok, "Could not update explorer window");
	ok = UpdateWindow(windowId);
	CHECK(ok, "Could not update widget window");

	// Create the embedding Qt widget
	QWindow* pWindow = QWindow::fromWinId((WId) windowId);
	CHECK(pWindow, "Could not create QWindow !");
	p_widget = QWidget::createWindowContainer(pWindow);
	p_widget->setMinimumWidth(100);
	p_widget->setMinimumHeight(100);
	p_widget->setParent(this);
	QVBoxLayout* pLayout = new QVBoxLayout(this);
	pLayout->setContentsMargins(0, 0, 0, 0);
	pLayout->addWidget(p_widget);
	setLayout(pLayout);

	// Resize
	resize(500, 300);

	// Done !
	return success();
}
//
ErrorPtr ExplorerWidget::setCurrentPath(const QString& path)
{
	CHECK( p_wrapper, "Unitialized object !" );
	CALL( p_wrapper->setCurrentPath(path) );
	return success();
}
//
QString ExplorerWidget::currentPath() const
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
void ExplorerWidget::moveEvent(QMoveEvent* pEvent)
{
	QWidget::moveEvent(pEvent);
	//updateEmbeddedWidget();
}
void ExplorerWidget::resizeEvent(QResizeEvent* pEvent)
{
	QWidget::resizeEvent(pEvent);
	updateEmbeddedWidget();
}
void ExplorerWidget::showEvent(QShowEvent* pEvent)
{
	QWidget::showEvent(pEvent);
	updateEmbeddedWidget();
}
//
ErrorPtr ExplorerWidget::updateEmbeddedWidget_p()
{
	if (QWidget* pWidget = p_widget)
	{
		HWND explorerId = p_wrapper->hwnd();

		// Estimate the title bar height
		HTHEME htheme = GetWindowTheme(explorerId);
		int h = GetThemeSysSize(htheme, SM_CXBORDER) + GetThemeSysSize(htheme, SM_CYSIZE) + GetThemeSysSize(htheme, SM_CXPADDEDBORDER) * 2;

		// Set the position
		QPoint pos = p_widget->mapToGlobal(QPoint(0, 0));
		BOOL okPos = SetWindowPos(	explorerId,
									nullptr,
									0,//pos.x(),
									-h,//pos.y(),
									p_widget->width(),
									p_widget->height() + h,
									SWP_NOZORDER);
		CHECK(okPos, "Position and size change error");

		UpdateWindow(explorerId);
	}

	return success();
}
//
void ExplorerWidget::updateEmbeddedWidget()
{
	if (ErrorPtr pError = updateEmbeddedWidget_p())
	{
		displayError(pError);
	}
}
//
