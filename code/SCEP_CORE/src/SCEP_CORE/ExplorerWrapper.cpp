#include <SCEP_CORE/ExplorerWrapper.h>
//
#include <QUrl>
#include <QFileInfo>
#include <QTimer>
#include <QtDebug>
#include <QApplication>
//
#include <propvarutil.h>
#include <psapi.h>
#include <shobjidl_core.h>
#include <shlguid.h>
#include <exdispid.h>
#include <uxtheme.h>
//
//
//
#define ONE_SINGLE_HOOK
//
//
//
extern "C" LRESULT CALLBACK sv_call_message(int nCode, WPARAM wParam, LPARAM lParam);
extern "C" LRESULT CALLBACK sv_call_shell(int nCode, WPARAM wParam, LPARAM lParam);
extern "C" LRESULT CALLBACK sv_call_wndproc(int nCode, WPARAM wParam, LPARAM lParam);
extern "C" LRESULT CALLBACK sv_mouseproc(int nCode, WPARAM wParam, LPARAM lParam);
//
//
//
HINSTANCE dll_hins = nullptr;
//
BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	dll_hins = (HINSTANCE)hModule;
	
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}
//
//
//
class ShellViewWndProc
{
private:
	static ShellViewWndProc* p_instance;
	static unsigned int m_instanceRefCount;

public:
	static inline ErrorPtr Register(HWND sv_hwnd, DWORD threadId, ExplorerWrapper* pExplorerWrapper)
	{
		if (! p_instance)
		{
			assert(m_instanceRefCount == 0);
			p_instance = new ShellViewWndProc();
			CALL( p_instance->initialize() );
		}
		m_instanceRefCount++;

		CALL( p_instance->registerExplorerWrapper(sv_hwnd, threadId, pExplorerWrapper) );

		return success();
	}

	static inline ErrorPtr Unregister(HWND sv_hwnd)
	{
		if (p_instance)
		{
			CALL( p_instance->unregisterExplorerWrapper(sv_hwnd) );

			m_instanceRefCount--;
			if (m_instanceRefCount == 0)
			{
				CALL( p_instance->finalize() );
				delete p_instance;
				p_instance = nullptr;
			}
		}

		return success();
	}

	static inline ExplorerWrapper* GetExplorerWrapper(HWND sv_hwnd)
	{
		return p_instance ? p_instance->explorerWrapper(sv_hwnd) : nullptr;
	}

private:
#ifdef ONE_SINGLE_HOOK
	HHOOK m_hook = nullptr;
	std::map<HWND, ExplorerWrapper*> m_explorerWrappers;
#else
	struct WrapperStruct
	{
		ExplorerWrapper*	ptr_wrapper = nullptr;
		DWORD				threadId = 0;
		HHOOK				hook = 0;
	};
	std::map<HWND, WrapperStruct> m_explorerWrappers;
#endif

public:
	ShellViewWndProc() = default;
	~ShellViewWndProc() = default;

	ErrorPtr initialize();
	ErrorPtr finalize();

	ErrorPtr registerExplorerWrapper(HWND sv_hwnd, DWORD threadId, ExplorerWrapper* pExplorerWrapper);
	ErrorPtr unregisterExplorerWrapper(HWND sv_hwnd);

	ExplorerWrapper* explorerWrapper(HWND sv_hwnd);
};
//
ShellViewWndProc* ShellViewWndProc::p_instance = nullptr;
unsigned int ShellViewWndProc::m_instanceRefCount = 0;
//
//
//
static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	bool ignore = false;

	qDebug() << message;

	switch (message)
	{
	case WM_CLOSE:
		ignore = true;
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		ignore = true;
		PostQuitMessage(0);
		break;
	case WM_ERASEBKGND:
		// To avoid flicking when the window is moved, we have to ignore this message
		ignore = true;
		break;
	case WM_PARENTNOTIFY:
		if (LOWORD(wParam) == WM_MBUTTONDOWN)
		{
			ExplorerWrapper* pThis = (ExplorerWrapper*) GetWindowLongPtr(hwnd, GWLP_USERDATA);

			qDebug() << "Middle click !";
			ignore = true; // Vraiment ??
		}
		break;
	default:
		break;
	}

	if (! ignore)
		return DefWindowProc(hwnd, message, wParam, lParam);
	else
		return 0;
}
//
static HWND CreateTheWindow(LPCWSTR WindowTitle)
{
	static LPCWSTR sClassName = L"ScepClass";

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
	return hwnd;
}
//
//
//
ExplorerWrapper::ExplorerWrapper(QObject* pParent)
	:	QObject(pParent)
	,	BrowserListener()
{}
//
ExplorerWrapper::~ExplorerWrapper()
{
	finalize();
}
//
ErrorPtr ExplorerWrapper::initialize(/*HWND parentWnd, */Theme* /*ptr_theme*/, const NavigationPath& path)
{
	// Can we start ?
	CHECK(m_navigationCount == 0, "Already initialized / used explorer wrapper.");


	// WebBrowser
	/////////////

	// The trick !!!
	// - new win32 window (proxy) -> can embed a window from another process
	// - embedding Qt widget : can embed the proxy window because it belongs to the same process
	//                         while it can't directly embed the explorer window !

	// Create a new empty window
	m_parentWnd = CreateTheWindow(L"Win32 SCEP");
	SetWindowLongPtr(m_parentWnd, GWLP_USERDATA, (LONG_PTR) this);
	CHECK(m_parentWnd, "Could not create win32 window");

	//m_parentWnd = parentWnd;

	// Create web browser
	HRESULT hr = CoCreateInstance(	CLSID_ShellBrowserWindow,
									nullptr,
									CLSCTX_LOCAL_SERVER,
									IID_PPV_ARGS(&m_webBrowser.p_wb)	);
	CHECK(SUCCEEDED(hr), "Unable to create explorer window : " + GetErrorAsString(hr));

	// Paramétrage fenêtre
	//////////////////////

//	Sleep(200);
//
	//hr = CoAllowSetForegroundWindow(m_webBrowser.p_wb.get(), 0);
	//CHECK(SUCCEEDED(hr), "Unable to allow foreground explorer window : " + GetErrorAsString(hr));

	// Get web browser window id
	m_webBrowser.m_wb_hwnd = 0;
	hr = m_webBrowser.p_wb->get_HWND((SHANDLE_PTR*) &m_webBrowser.m_wb_hwnd);
	CHECK(SUCCEEDED(hr), "Could not get explorer window handle : " + GetErrorAsString(hr));

	// If a parent window is given, reparent explorer window
	if (m_parentWnd)
	{
		LONG rslt = SetWindowLong(m_webBrowser.m_wb_hwnd, GWL_STYLE, WS_CHILDWINDOW);
		CHECK(rslt != 0, "Could not change explorer window style : " + QString::number(GetLastError()));
		HWND oldId = SetParent(m_webBrowser.m_wb_hwnd, m_parentWnd);
		CHECK(oldId, "Could not change explorer window parent : " + QString::number(GetLastError()));
	}

	//m_webBrowser.p_wb->put_RegisterAsBrowser(VARIANT_TRUE);

	// Hook on the launched explorer
	BrowserListener::InitBrowser(m_webBrowser.p_wb.get());

	// And set the current directory
	if (path.valid())
	{
		CALL( setCurrentPath(path) );
	}

	// End
	return success();
}
//
HWND ExplorerWrapper::hwnd() const
{
	return /*m_webBrowser.m_wb_hwnd*/ m_parentWnd;
}
//
void ExplorerWrapper::setVisible(bool visible)
{
	if (m_webBrowser.p_wb)
	{
		m_webBrowser.p_wb->put_Visible(visible ? VARIANT_TRUE : VARIANT_FALSE);
	}
}
//
ErrorPtr ExplorerWrapper::updateWindow(int width, int height)
{
	HWND parentId = m_parentWnd;
	HWND explorerId = m_webBrowser.m_wb_hwnd;

	// Estimate the title bar height
	HTHEME htheme = GetWindowTheme(explorerId);
	int h = GetThemeSysSize(htheme, SM_CXBORDER) + GetThemeSysSize(htheme, SM_CYSIZE) + GetThemeSysSize(htheme, SM_CXPADDEDBORDER) * 2;

	// Set the position
	//QPoint pos = p_widget->mapToGlobal(QPoint(0, 0));
	BOOL okPos = SetWindowPos(	explorerId,
								nullptr,
								0,//pos.x(),
								-h,//pos.y(),
								width,
								height + h,
								SWP_NOZORDER);
	CHECK(okPos, "Position and size change error");

	UpdateWindow(explorerId);
	UpdateWindow(parentId);

	return success();
}
//
ErrorPtr ExplorerWrapper::setCurrentPath(const NavigationPath& path)
{
	CHECK(m_webBrowser.p_wb, "ExplorerWrapper::setCurrentPath() : No current instance.");

	CHECK(path.valid() && path.isExistingDirectory(), "Invalid path");
	
	VARIANT vEmpty = {0};
	VARIANT varTarget = {0};

	std::wstring wpath = path.internalPath().toStdWString();
	HRESULT hr = InitVariantFromString(wpath.c_str(), &varTarget);
	CHECK(SUCCEEDED(hr), "Could not set initial folder for explorer window.")
	
	hr = m_webBrowser.p_wb->Navigate2(&varTarget, &vEmpty, &vEmpty, &vEmpty, &vEmpty);
	CHECK(SUCCEEDED(hr), "Unable to set initial folder for explorer window.");

	hr = VariantClear(&varTarget);
	if (! SUCCEEDED(hr))
	{
		qWarning() << "Could not set clear initial folder variant.";
	}

	return success();
}
//
NavigationPath ExplorerWrapper::currentPath() const
{
	if (m_webBrowser.p_wb)
	{
		BSTR url;
		HRESULT hr = m_webBrowser.p_wb->get_LocationURL(&url);
		if (SUCCEEDED(hr))
		{
			QUrl qUrl( QString::fromWCharArray(url) );
			SysFreeString(url);

			return NavigationPath(qUrl.toLocalFile());
		}
		else
		{
			qWarning() << "ExplorerWrapper::currentPath() : Could not get current location.";
			return {};
		}
	}
	else
	{
		qWarning() << "ExplorerWrapper::currentPath() : No current instance.";
		return {};
	}
}
//
ErrorPtr ExplorerWrapper::finalize()
{
	if (m_wndproc_hwn)
	{
		CALL( ShellViewWndProc::Unregister(m_wndproc_hwn) );
	}

	m_shellView.finalize();

	Teardown();

	m_webBrowser.finalize();

	return success();
}
//
void ExplorerWrapper::OnNavigateComplete2(IWebBrowser2 *window, const std::wstring& url)
{
	if (window == m_webBrowser.p_wb.get())
	{
		QString qurl = QString::fromStdWString(url);
		//qDebug() << "[ExplorerWrapper::OnNavigateComplete2] : " << qurl << " --> emit pathChanged()";
		emit pathChanged(qurl);

		if (m_navigationCount == 0) // We try this initialization just once, after first navigation
		{
			auto endInit = [this]() -> ErrorPtr
			{
				Box<IServiceProvider> psp = nullptr;
				HRESULT hr = m_webBrowser.p_wb->QueryInterface(IID_IServiceProvider, (void**)&psp);
				CHECK(SUCCEEDED(hr), "Could not get service provider interface window handle : " + GetErrorAsString(hr));

				Box<IShellBrowser> psb = nullptr;
				hr = psp->QueryService(SID_STopLevelBrowser, IID_IShellBrowser, (void**)&psb);
				CHECK(SUCCEEDED(hr), "Could not get top level browser service : " + GetErrorAsString(hr));

				hr = psb->QueryActiveShellView(&m_shellView.p_sv);
				CHECK(SUCCEEDED(hr), "Could not get active shell view : " + GetErrorAsString(hr));

				hr = m_shellView.p_sv->QueryInterface(IID_IFolderView2, (void**)&m_shellView.p_fv);
				CHECK(SUCCEEDED(hr), "Could not get current folder view  : " + GetErrorAsString(hr));

				// 
				Box<IDispatch> spdispView;
				m_shellView.p_sv->GetItemObject(SVGIO_BACKGROUND, IID_PPV_ARGS(&spdispView));
				spdispView->QueryInterface(IID_PPV_ARGS(&m_shellView.p_fvd));
				BrowserListener::InitView(m_shellView.p_fvd.get());
				//m_shellView.p_sink = new CShellFolderViewEventsSink();
				//m_shellView.p_sink->Connect(m_shellView.p_fvd.get());


				// Get the corresponding window
				hr = m_shellView.p_sv->GetWindow(&m_shellView.m_sv_hwnd);
				CHECK(SUCCEEDED(hr), "Could not get shell view window : " + GetErrorAsString(hr));

			//	DWORD processId = 0;
			//	DWORD threadId = GetWindowThreadProcessId(m_shellView.m_sv_hwnd, &processId);
			//	qDebug() << "IShellView window process id : " << processId;
			//	qDebug() << "IShellView window thread id : " << threadId;
			//	qDebug() << "Current process id : " << GetCurrentProcessId();
			//
			//	m_wndproc_hwn = m_parentWnd != nullptr ? m_parentWnd : m_shellView.m_sv_hwnd;
			//	CALL( ShellViewWndProc::Register(m_wndproc_hwn, threadId, this) );

				return success();
			};

			ErrorPtr pError = endInit();
			if (pError)
			{
				qCritical() << "End of ExplorerWrapper initialization failed : " << *pError;
			}
		}
		
		m_navigationCount++;
	}
}
// 
ErrorPtr ExplorerWrapper::getSelectedItems(std::vector<Box<IShellItem>>& shellItems)
{
	// Check
	shellItems.clear();
	CHECK(m_shellView.p_sv, "ExplorerWrapper::getSelectedItem() : No current instance.");

	// Get the current selection
	Box<IShellItemArray> pItemArray;
	HRESULT hr = m_shellView.p_fv->GetSelection(FALSE, &pItemArray);
	if (SUCCEEDED(hr))
	{
		DWORD count = 0;
		hr = pItemArray->GetCount(&count);
		if (SUCCEEDED(hr))
		{
			shellItems.resize(count);

			for (DWORD i = 0; (i < count) && SUCCEEDED(hr); i++)
			{
				hr = pItemArray->GetItemAt(i, &shellItems[i]);
			}
		}
	}

	return success();
}
//
NavigationPath GetDisplayNameOf(IShellFolder* shellFolder, LPITEMIDLIST pidl, SHGDNF /*uFlags*/)
{
	STRRET StrRet;
	StrRet.uType = STRRET_WSTR;

	HRESULT hr = shellFolder->GetDisplayNameOf((PCUITEMID_CHILD) pidl, SHGDN_NORMAL + SHGDN_FORPARSING, &StrRet);
	if (FAILED(hr))
	{
		qCritical() << "GetDisplayNameOf:hr:  " << hr;
		return {};
	}

	switch (StrRet.uType)
	{
	case STRRET_CSTR :
		qCritical() << "GetDisplayNameOf:unexpected return type:STRRET_CSTR :  " << StrRet.cStr;
		return QString(StrRet.cStr);
	case STRRET_WSTR :
		return QString::fromWCharArray(StrRet.pOleStr);
	case STRRET_OFFSET :
		qCritical() << "GetDisplayNameOf:unexpected return type:STRRET_OFFSET :  " << (((char*)pidl) + StrRet.uOffset);
		return QString(((char*)pidl) + StrRet.uOffset);
	default:
		return {};
	}
}
//
/*
void ExplorerWrapper::processMiddleClickEvent()
{
	// Register the middle click
	m_middleClickDateTime = QDateTime::currentDateTime();

	//// Simulate a left click a the same position in order to select the item
	//// --> Handled through the win32 message loop
	//INPUT input;
	//input.type = INPUT_MOUSE;
	//input.mi.dx = 0;
	//input.mi.dy = 0;
	//input.mi.mouseData = 0;
	//input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
	//input.mi.time = 0;
	//input.mi.dwExtraInfo = 0;
	//SendInput(1, &input, sizeof(INPUT));
	
	
	
	//qApp->processEvents();


	// Request a new tab on the newly selected item
	// --> Handled through the Qt event loop
	// --> Seems to always be handled AFTER the win32 input message !
	QTimer::singleShot(0, [=]()
	{
		// Are we dealing with a middle click ?
		if (m_middleClickDateTime.has_value())
		{
#ifdef _DEBUG
			static constexpr qint64 DelayMs = 200;
#else
			static constexpr qint64 DelayMs = 1000;
#endif
		
			// Is the click recent enough ?
			const QDateTime middleClickDateTime = m_middleClickDateTime.value();
			if (middleClickDateTime.msecsTo(QDateTime::currentDateTime()) < DelayMs)
			{
				// Get the selected item
				std::optional<NavigationPath> path;

				{
					int iItem = -1;
					HRESULT hr = m_shellView.p_fv->GetFocusedItem(&iItem);
					if (SUCCEEDED(hr))
					{
						PITEMID_CHILD ppidl; // leak ??
						hr = m_shellView.p_fv->Item(iItem, &ppidl);
						if (SUCCEEDED(hr))
						{
							Box<IShellFolder> ppshf;
							hr = m_shellView.p_fv->GetFolder(IID_IShellFolder, (void**) &ppshf);
							if (SUCCEEDED(hr))
							{
								path = GetDisplayNameOf(ppshf.get(), ppidl, SHGDN_NORMAL + SHGDN_FORPARSING);
							}
						}
					}
				}

				//std::vector<Box<IShellItem>> shellItems;
				//ErrorPtr pError = getSelectedItems(shellItems);
				//if ( (! pError) && (shellItems.size() == 1) )
				//{
				//	// Get the name of the selected item
				//	PWSTR pszName = nullptr;
				//	HRESULT hr = shellItems[0]->GetDisplayName(SIGDN_NORMALDISPLAY, &pszName);
				//	if (SUCCEEDED(hr))
				//	{
				//		// Path of the clicked item
				//		path = currentPath().childPath(QString::fromWCharArray(pszName));
				//	}
				//	
				//	// Clean up
				//	if (pszName)
				//		CoTaskMemFree(pszName);
				//}

				if (path.has_value())
				{
					// Ask for a new tab if the clicked item corresponds to an existing folder
					if (path.value().isExistingDirectory())
					{
						qDebug() << "Requesting new tab for " << path.value().displayPath();
						emit openNewTab(path.value(), NewTabPosition::AfterCurrent, NewTabBehaviour::None);
					}
				}
			}
		
			// Clean up
			m_middleClickDateTime = std::nullopt;
		}
	});
}
*/
//
void ExplorerWrapper::requestingFakeContextMenu()
{
	m_middleClickDateTime = QDateTime::currentDateTime();
}
//
bool ExplorerWrapper::contextMenuRequired()
{
	bool handled = false;

	static constexpr qint64 MaxDelayMs = 200;
	static const QDateTime Epoch = QDateTime::fromSecsSinceEpoch(0);

	// Are we requesting a fake context menu ?
	const QDateTime middleClickDateTime = m_middleClickDateTime.value_or(Epoch);
//	if (middleClickDateTime.msecsTo(QDateTime::currentDateTime()) < MaxDelayMs)
	{
		// Get the focused item
		std::optional<NavigationPath> path;

		//{
		//	int iItem = -1;
		//	HRESULT hr = m_shellView.p_fv->GetFocusedItem(&iItem);
		//	if (SUCCEEDED(hr))
		//	{
		//		PITEMID_CHILD ppidl; // leak ??
		//		hr = m_shellView.p_fv->Item(iItem, &ppidl);
		//		if (SUCCEEDED(hr))
		//		{
		//			Box<IShellFolder> ppshf;
		//			hr = m_shellView.p_fv->GetFolder(IID_IShellFolder, (void**) &ppshf);
		//			if (SUCCEEDED(hr))
		//			{
		//				path = GetDisplayNameOf(ppshf.get(), ppidl, SHGDN_NORMAL + SHGDN_FORPARSING);
		//			}
		//		}
		//	}
		//}

		std::vector<Box<IShellItem>> shellItems;
		ErrorPtr pError = getSelectedItems(shellItems);
		if ( (! pError) && (shellItems.size() == 1) )
		{
			// Get the name of the selected item
			PWSTR pszName = nullptr;
			HRESULT hr = shellItems[0]->GetDisplayName(SIGDN_NORMALDISPLAY, &pszName);
			if (SUCCEEDED(hr))
			{
				// Path of the clicked item
				path = currentPath().childPath(QString::fromWCharArray(pszName));
			}
			
			// Clean up
			if (pszName)
				CoTaskMemFree(pszName);
		}

		// If we have a focused item and it corresponds to an existing folder, we ask for a new tab
		if (path.has_value() && path.value().isExistingDirectory())
		{
			qDebug() << "Requesting new tab for " << path.value().displayPath();
			emit openNewTab(path.value(), NewTabPosition::AfterCurrent, NewTabBehaviour::None);
		}

		// We handled it !
		handled = true;
	}

	// End
	m_middleClickDateTime = std::nullopt;
	return handled;
}
//
//
//
ErrorPtr ShellViewWndProc::initialize()
{
#ifdef ONE_SINGLE_HOOK
	m_hook = SetWindowsHookEx(WH_CALLWNDPROC, &sv_call_wndproc, dll_hins, 0/*threadId*/);
	//m_hook = SetWindowsHookEx(WH_CALLWNDPROCRET, &sv_call_wndretproc, dll_hins, 0/*threadId*/);
	//m_hook = SetWindowsHookEx(WH_GETMESSAGE, &sv_call_message, dll_hins, 0/*threadId*/);
	//m_hook = SetWindowsHookEx(WH_SHELL, &sv_call_shell, dll_hins, 0/*threadId*/);

	
	//m_hook = SetWindowsHookEx(WH_MOUSE, &sv_mouseproc, nullptr, GetCurrentThreadId());
	CHECK(m_hook, "Could not set window procedure callback on shell view window: " + GetLastErrorAsString());
#endif //ONE_SINGLE_HOOK

	return success();
}
//
ErrorPtr ShellViewWndProc::finalize()
{
#ifdef ONE_SINGLE_HOOK
	if (m_hook)
	{
		BOOL ok = UnhookWindowsHookEx(m_hook);
		if (! ok)
			qDebug() << "Failure to unhook : " << GetLastErrorAsString();
		m_hook = nullptr;
	}
#endif //ONE_SINGLE_HOOK

	return success();
}
//
ErrorPtr ShellViewWndProc::registerExplorerWrapper(HWND sv_hwnd, [[maybe_unused]] DWORD threadId, ExplorerWrapper* pExplorerWrapper)
{
	assert(sv_hwnd);
	assert(pExplorerWrapper);
	assert(m_explorerWrappers.find(sv_hwnd) == m_explorerWrappers.end());

#ifdef ONE_SINGLE_HOOK
	m_explorerWrappers[sv_hwnd] = pExplorerWrapper;
#else
	HHOOK hook = SetWindowsHookEx(WH_CALLWNDPROC, &sv_call_wndproc, dll_hins, threadId/*GetCurrentThreadId()*/);

	//HINSTANCE hins = nullptr;
	//BOOL ok = GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, L"shell32", &hins);
	//CHECK(ok == TRUE, "Could not load shell32.dll: " + GetLastErrorAsString());
	////HINSTANCE hins = LoadLibrary(L"shell32.dll");
	//CHECK(hins != nullptr, "Could not load shell32.dll: " + GetLastErrorAsString());
	//HHOOK hook = SetWindowsHookEx(WH_CALLWNDPROC, &sv_call_wndproc, hins/*dll_hins*/, threadId);

	//HWND g_ProgWin = NULL;
	//while (!g_ProgWin)
	//{
	//    g_ProgWin = GetShellWindow();
	//    if (!g_ProgWin)
	//    {
	//        Sleep(100);
	//    }
	//}
	////printf("Progman: %d\n", g_ProgWin);
	//DWORD progThread = GetWindowThreadProcessId(
	//    g_ProgWin,
	//    NULL
	//);
	//
	//HHOOK hook = SetWindowsHookEx(WH_CALLWNDPROC, &sv_call_wndproc, dll_hins, progThread);
	

	//m_hook = SetWindowsHookEx(WH_CALLWNDPROCRET, &sv_call_wndretproc, hins/*dll_hins*/, threadId);

	//HHOOK hook = SetWindowsHookEx(WH_GETMESSAGE, &sv_call_message, nullptr, 0/*threadId*/);

	CHECK(hook, "Could not set window procedure callback on shell view window: " + GetLastErrorAsString());

	m_explorerWrappers[sv_hwnd] = {pExplorerWrapper, threadId, hook};
#endif //ONE_SINGLE_HOOK

	return success();
}
//
ErrorPtr ShellViewWndProc::unregisterExplorerWrapper(HWND sv_hwnd)
{
	assert(sv_hwnd);
	auto ite = m_explorerWrappers.find(sv_hwnd);
	if (ite != m_explorerWrappers.end())
	{
#ifndef ONE_SINGLE_HOOK
		if (ite->second.hook)
		{
			BOOL ok = UnhookWindowsHookEx(ite->second.hook);
			if (! ok)
				qDebug() << "Failure to unhook : " << GetLastErrorAsString();
		}
#endif //ONE_SINGLE_HOOK
		m_explorerWrappers.erase(ite);
	}

	return success();
}
//
ExplorerWrapper* ShellViewWndProc::explorerWrapper(HWND sv_hwnd)
{
	assert(sv_hwnd);
	auto ite = m_explorerWrappers.find(sv_hwnd);
#ifdef ONE_SINGLE_HOOK
	return (ite != m_explorerWrappers.end()) ? ite->second : nullptr;
#else
	return (ite != m_explorerWrappers.end()) ? ite->second.ptr_wrapper : nullptr;
#endif //ONE_SINGLE_HOOK
}
//
//
//
extern "C"
LRESULT CALLBACK sv_call_message(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{
		if (MSG* pMSG = (MSG*) lParam)
		{
			qDebug() << pMSG->message;

			if (pMSG->message == WM_PARENTNOTIFY && (LOWORD(pMSG->wParam) == WM_MBUTTONDOWN))
			{
				qDebug() << "Middle click !";

			}
			else if (pMSG->message == WM_CONTEXTMENU)
			{
				qDebug() << "Context menu !";

			}
		}
	}

	return CallNextHookEx(	HHOOK(),
							nCode,
							wParam,
							lParam	);
}
//
//
//
extern "C"
LRESULT CALLBACK sv_call_shell(int nCode, WPARAM wParam, LPARAM lParam)
{
	qDebug() << nCode;

	if (nCode == HSHELL_APPCOMMAND)
	{
		short cmd  = GET_APPCOMMAND_LPARAM(lParam);

		WORD uDevice = GET_DEVICE_LPARAM(lParam);

		WORD dwKeys = GET_KEYSTATE_LPARAM(lParam);

		if ( (uDevice == FAPPCOMMAND_MOUSE) && (dwKeys == MK_RBUTTON) )
		{
			qDebug() << "Middle click !";
		}
		else if ( (uDevice == FAPPCOMMAND_MOUSE) && (dwKeys == MK_MBUTTON) )
		{
			qDebug() << "Middle click !";
		}
	}

	return CallNextHookEx(	HHOOK(),
							nCode,
							wParam,
							lParam	);
}
//
//
//
extern "C"
LRESULT CALLBACK sv_call_wndproc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		if (CWPSTRUCT* pCWP = (CWPSTRUCT*) lParam)
		{
			//if (pCWP->message != 49661)
			//	qDebug() << pCWP->message;

			// Middle click
			///////////////

			if (pCWP->message == WM_PARENTNOTIFY && (LOWORD(pCWP->wParam) == WM_MBUTTONDOWN))
			{
				qDebug() << "Middle click !";

				if (ExplorerWrapper* pExplorerWrapper = ShellViewWndProc::GetExplorerWrapper(pCWP->hwnd))
				{
					// Cannot directly identify the item that was clicked (if any)
					// - Requesting a context menu
					// - Handling the request with the WM_CONTEXTMENU message (see later)
					// - Getting the focused item (if any) and requesting a new tab

					pExplorerWrapper->requestingFakeContextMenu();

					//qDebug() << "requestingFakeContextMenu !";

					{
						INPUT input;
						input.type = INPUT_MOUSE;
						input.mi.dx = 0;
						input.mi.dy = 0;
						input.mi.mouseData = 0;
						input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
						input.mi.time = 0;
						input.mi.dwExtraInfo = 0;
						SendInput(1, &input, sizeof(INPUT));
					}
					{
						INPUT input;
						input.type = INPUT_MOUSE;
						input.mi.dx = 0;
						input.mi.dy = 0;
						input.mi.mouseData = 0;
						input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
						input.mi.time = 0;
						input.mi.dwExtraInfo = 0;
						SendInput(1, &input, sizeof(INPUT));
					}

					//qApp->processEvents();


					//// Simulate right clic
					//{
					//	INPUT input;
					//	input.type = INPUT_MOUSE;
					//	input.mi.dx = 0;
					//	input.mi.dy = 0;
					//	input.mi.mouseData = 0;
					//	input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
					//	input.mi.time = 0;
					//	input.mi.dwExtraInfo = 0;
					//	SendInput(1, &input, sizeof(INPUT));
					//}
					//{
					//	INPUT input;
					//	input.type = INPUT_MOUSE;
					//	input.mi.dx = 0;
					//	input.mi.dy = 0;
					//	input.mi.mouseData = 0;
					//	input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
					//	input.mi.time = 0;
					//	input.mi.dwExtraInfo = 0;
					//	SendInput(1, &input, sizeof(INPUT));
					//}
					//
					//// Process
					//qApp->processEvents();
					//
					//// Simulate esc
					//{
					//	INPUT input;
					//	input.type = INPUT_KEYBOARD;
					//	input.ki.wVk = VK_ESCAPE;
					//	input.ki.wScan = 0;
					//	input.ki.dwFlags = 0;
					//	input.ki.time = 0;
					//	input.ki.dwExtraInfo = 0;
					//	SendInput(1, &input, sizeof(INPUT));
					//}
					//{
					//	INPUT input;
					//	input.type = INPUT_KEYBOARD;
					//	input.ki.wVk = VK_ESCAPE;
					//	input.ki.wScan = 0;
					//	input.ki.dwFlags = KEYEVENTF_KEYUP;
					//	input.ki.time = 0;
					//	input.ki.dwExtraInfo = 0;
					//	SendInput(1, &input, sizeof(INPUT));
					//}

					QTimer::singleShot(0, [=]() { pExplorerWrapper->contextMenuRequired(); });

					//pExplorerWrapper->processMiddleClickEvent();
					// We do not return, so CallNextHookEx will be called !
				}
			}
			//else if (pCWP->message == WM_CONTEXTMENU)
			//{
			//	qDebug() << "Context menu !";
			//
			//	if (ExplorerWrapper* pExplorerWrapper = ShellViewWndProc::GetExplorerWrapper(pCWP->hwnd))
			//	{
			//		bool handled = pExplorerWrapper->contextMenuRequired();
			//		if (handled)
			//			return 0;
			//	}
			//}

			//*/
		}
	}
	//else if (nCode < 0)
	//{
		return CallNextHookEx(	HHOOK(),
								nCode,
								wParam,
								lParam	);
	//}
}
//
/*
extern "C"
LRESULT CALLBACK sv_call_wndretproc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		if (CWPRETSTRUCT* pCWP = (CWPRETSTRUCT*) lParam)
		{
			// Middle click ?
			if (pCWP->message == WM_PARENTNOTIFY && (LOWORD(pCWP->wParam) == WM_MBUTTONDOWN))
			{
				if (ExplorerWrapper* pExplorerWrapper = ShellViewWndProc::GetExplorerWrapper(pCWP->hwnd))
				{
					pExplorerWrapper->processMiddleClickEvent();
					// We do not return, so CallNextHookEx will be called !
				}
			}
		}
	}
	//else if (nCode < 0)
	//{
		return CallNextHookEx(	HHOOK(),
								nCode,
								wParam,
								lParam	);
	//}
}
*/
extern "C" LRESULT CALLBACK sv_mouseproc(int nCode, WPARAM wParam, LPARAM lParam)
{
	return CallNextHookEx(	HHOOK(),
							nCode,
							wParam,
							lParam	);
}

