#include <SCEP_CORE/ExplorerWrapper.h>
//
#include <QUrl>
#include <QFileInfo>
#include <QTimer>
#include <QtDebug>
#include <QApplication>
//
#include <windowsx.h>
#include <propvarutil.h>
#include <psapi.h>
#include <shobjidl_core.h>
#include <shlguid.h>
#include <exdispid.h>
#include <uxtheme.h>
//
//
//
static constexpr qint64 OffsetMs = 200;
static constexpr qint64 DelayMs = 200;
//
//
//
//#define ONE_SINGLE_HOOK
////
////
////
//extern "C" LRESULT CALLBACK sv_call_message(int nCode, WPARAM wParam, LPARAM lParam);
//extern "C" LRESULT CALLBACK sv_call_shell(int nCode, WPARAM wParam, LPARAM lParam);
//extern "C" LRESULT CALLBACK sv_call_wndproc(int nCode, WPARAM wParam, LPARAM lParam);
//extern "C" LRESULT CALLBACK sv_mouseproc(int nCode, WPARAM wParam, LPARAM lParam);
////
////
////
//HINSTANCE dll_hins = nullptr;
////
//BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID /*lpReserved*/)
//{
//	dll_hins = (HINSTANCE)hModule;
//	
//	switch (ul_reason_for_call)
//	{
//		case DLL_PROCESS_ATTACH:
//		case DLL_THREAD_ATTACH:
//		case DLL_THREAD_DETACH:
//		case DLL_PROCESS_DETACH:
//			break;
//	}
//	return TRUE;
//}
////
////
////
//class ShellViewWndProc
//{
//private:
//	static ShellViewWndProc* p_instance;
//	static unsigned int m_instanceRefCount;
//
//public:
//	static inline ErrorPtr Register(HWND sv_hwnd, DWORD threadId, ExplorerWrapper* pExplorerWrapper)
//	{
//		if (! p_instance)
//		{
//			assert(m_instanceRefCount == 0);
//			p_instance = new ShellViewWndProc();
//			CALL( p_instance->initialize() );
//		}
//		m_instanceRefCount++;
//
//		CALL( p_instance->registerExplorerWrapper(sv_hwnd, threadId, pExplorerWrapper) );
//
//		return success();
//	}
//
//	static inline ErrorPtr Unregister(HWND sv_hwnd)
//	{
//		if (p_instance)
//		{
//			CALL( p_instance->unregisterExplorerWrapper(sv_hwnd) );
//
//			m_instanceRefCount--;
//			if (m_instanceRefCount == 0)
//			{
//				CALL( p_instance->finalize() );
//				delete p_instance;
//				p_instance = nullptr;
//			}
//		}
//
//		return success();
//	}
//
//	static inline ExplorerWrapper* GetExplorerWrapper(HWND sv_hwnd)
//	{
//		return p_instance ? p_instance->explorerWrapper(sv_hwnd) : nullptr;
//	}
//
//private:
//#ifdef ONE_SINGLE_HOOK
//	HHOOK m_hook = nullptr;
//	std::map<HWND, ExplorerWrapper*> m_explorerWrappers;
//#else
//	struct WrapperStruct
//	{
//		ExplorerWrapper*	ptr_wrapper = nullptr;
//		DWORD				threadId = 0;
//		HHOOK				hook = 0;
//	};
//	std::map<HWND, WrapperStruct> m_explorerWrappers;
//#endif
//
//public:
//	ShellViewWndProc() = default;
//	~ShellViewWndProc() = default;
//
//	ErrorPtr initialize();
//	ErrorPtr finalize();
//
//	ErrorPtr registerExplorerWrapper(HWND sv_hwnd, DWORD threadId, ExplorerWrapper* pExplorerWrapper);
//	ErrorPtr unregisterExplorerWrapper(HWND sv_hwnd);
//
//	ExplorerWrapper* explorerWrapper(HWND sv_hwnd);
//};
////
//ShellViewWndProc* ShellViewWndProc::p_instance = nullptr;
//unsigned int ShellViewWndProc::m_instanceRefCount = 0;
//
//
//
static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	bool ignore = false;

	//qDebug() << message;

	switch (message)
	{
	case WM_CLOSE:
		ignore = true;
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		ignore = true;
		//PostQuitMessage(0);
		break;
	case WM_ERASEBKGND:
		// To avoid flicking when the window is moved, we have to ignore this message
		ignore = true;
		break;
	case WM_PARENTNOTIFY:
		if (LOWORD(wParam) == WM_MBUTTONDOWN)
		{
			//qDebug() << "Middle click !";

			ExplorerWrapper* pThis = (ExplorerWrapper*) GetWindowLongPtr(hwnd, GWLP_USERDATA);

			//// The event needs to be processed by DefWindowProc in order to update
			//// the focused window if necessary
			//QTimer::singleShot(0, [pThis]() { pThis->onMiddleClick(); });

			POINT windowPos;
			windowPos.x = GET_X_LPARAM(lParam);
			windowPos.y = GET_Y_LPARAM(lParam);

			// map to screen
			POINT screenPos = windowPos;
			MapWindowPoints(hwnd, HWND_DESKTOP, &screenPos, 1);

			pThis->onMiddleClick(hwnd, windowPos, screenPos);
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




	//{
	//	//RECT rc;
	//	//GetWindowRect(hwndStatic, &rc);
	//	//MapWindowRect(HWND_DESKTOP, _hdlg, &rc);
	//
	//
	//	m_parentWnd = CreateTheWindow(L"Test");
	//	RECT rc;
	//	rc.left = 100;
	//	rc.top = 500;
	//	rc.right = 500;
	//	rc.bottom = 100;
	//	//MapWindowRect(HWND_DESKTOP, m_parentWnd, &rc);
	//	
	//	INameSpaceTreeControl2 *_pnstc2 = nullptr;
	//
	//	HRESULT hr = CoCreateInstance(CLSID_NamespaceTreeControl, NULL, CLSCTX_INPROC, IID_PPV_ARGS(&m_shellView.p_nstc));
	//	if (SUCCEEDED(hr))
	//	{
	//		const NSTCSTYLE nsctsFlags = NSTCS_HASEXPANDOS |            // Show expandos
	//										NSTCS_ROOTHASEXPANDO |         // Root nodes have expandos
	//										NSTCS_FADEINOUTEXPANDOS |      // Fade-in-out based on focus
	//										NSTCS_NOINFOTIP |              // Don't show infotips
	//										NSTCS_ALLOWJUNCTIONS |         // Show folders such as zip folders and libraries
	//										NSTCS_SHOWSELECTIONALWAYS |    // Show selection when NSC doesn't have focus
	//										NSTCS_FULLROWSELECT;           // Select full width of item
	//		hr = m_shellView.p_nstc->Initialize(m_parentWnd, &rc, nsctsFlags);
	//		if (SUCCEEDED(hr))
	//		{
	//			// New Windows 7 features
	//			if (SUCCEEDED(m_shellView.p_nstc->QueryInterface(IID_PPV_ARGS(&_pnstc2))))
	//			{
	//				NSTCSTYLE2 nscts2Flags = NSTCS2_DISPLAYPADDING |            // Padding between top-level nodes
	//											NTSCS2_NOSINGLETONAUTOEXPAND |     // Don't auto-expand nodes with a single child node
	//											NSTCS2_INTERRUPTNOTIFICATIONS |    // Register for interrupt notifications on a per-node basis
	//											NSTCS2_DISPLAYPINNEDONLY |         // Filter on pinned property
	//											NTSCS2_NEVERINSERTNONENUMERATED;   // Don't insert items with property SFGAO_NONENUMERATED
	//				hr = _pnstc2->SetControlStyle2(nscts2Flags, nscts2Flags);
	//			}
	//			if (SUCCEEDED(hr))
	//			{
	//				hr = m_shellView.p_nstc->TreeAdvise(static_cast<INameSpaceTreeControlEvents *>(this), &m_shellView.m_nstcCookie);
	//				if (SUCCEEDED(hr))
	//				{
	//					hr = IUnknown_SetSite(m_shellView.p_nstc.get(), static_cast<IServiceProvider *>(this));
	//					if (SUCCEEDED(hr))
	//					{
	//						qDebug() << "yes !";
	//					}
	//				}
	//			}
	//
	//			 m_shellView.p_nstc->RemoveAllRoots();
	//
	//			BOOL fEnableStyleChange;
	//			NSTCSTYLE  nsctsFlags;
	//			NSTCSTYLE2 nsctsFlags2;
	//			if (_pnstc2)
	//			{
	//				fEnableStyleChange = TRUE;
	//				_pnstc2->GetControlStyle(NSTCS_HASEXPANDOS | NSTCS_HASLINES | NSTCS_FULLROWSELECT | NSTCS_HORIZONTALSCROLL | NSTCS_RICHTOOLTIP | NSTCS_AUTOHSCROLL | NSTCS_EMPTYTEXT, &nsctsFlags);
	//				_pnstc2->GetControlStyle2(NSTCS2_DISPLAYPADDING | NSTCS2_DISPLAYPINNEDONLY | NTSCS2_NOSINGLETONAUTOEXPAND, &nsctsFlags2);
	//			}
	//			else
	//			{
	//				// When running downlevel INameSpaceTreeControl2 may not be available
	//				// Set styles to defaults.
	//				fEnableStyleChange = FALSE;
	//				nsctsFlags  = NSTCS_HASEXPANDOS | NSTCS_ROOTHASEXPANDO | NSTCS_FADEINOUTEXPANDOS | NSTCS_NOINFOTIP | NSTCS_ALLOWJUNCTIONS | NSTCS_SHOWSELECTIONALWAYS | NSTCS_FULLROWSELECT;
	//				nsctsFlags2 = NSTCS2_DEFAULT;
	//
	//			}
	//
	//			//auto _SetCheckBoxState = [this](int id, BOOL fChecked, BOOL fEnabled)
	//			//{
	//			//	EnableWindow(GetDlgItem(m_parentWnd, id), fEnabled);
	//			//	CheckDlgButton(m_parentWnd, id, fChecked);
	//			//};
	//			//
	//			//_SetCheckBoxState(IDC_EXPANDOS,         nsctsFlags  & NSTCS_HASEXPANDOS,            fEnableStyleChange);
	//			//_SetCheckBoxState(IDC_LINES,            nsctsFlags  & NSTCS_HASLINES,               fEnableStyleChange);
	//			//_SetCheckBoxState(IDC_FULLROWSELECT,    nsctsFlags  & NSTCS_FULLROWSELECT,          fEnableStyleChange);
	//			//_SetCheckBoxState(IDC_HORIZONTALSCROLL, nsctsFlags  & NSTCS_HORIZONTALSCROLL,       fEnableStyleChange);
	//			//_SetCheckBoxState(IDC_PADDING,          nsctsFlags2 & NSTCS2_DISPLAYPADDING,        fEnableStyleChange);
	//			//_SetCheckBoxState(IDC_FILTERPINNED,     nsctsFlags2 & NSTCS2_DISPLAYPINNEDONLY,     fEnableStyleChange);
	//			//_SetCheckBoxState(IDC_AUTOEXPAND,       nsctsFlags2 & NTSCS2_NOSINGLETONAUTOEXPAND, fEnableStyleChange);
	//
	//			// CLSID_CommonPlacesFolder
	//
	//			IShellItem *psiFavorites;
	//			HRESULT hr = SHCreateItemFromParsingName(L"shell:::{323CA680-C24D-4099-B94D-446DD2D7249E}", NULL, IID_PPV_ARGS(&psiFavorites));
	//			if (SUCCEEDED(hr))
	//			{
	//				// Add a visible root
	//				m_shellView.p_nstc->AppendRoot(psiFavorites, SHCONTF_NONFOLDERS, NSTCRS_VISIBLE | NSTCRS_EXPANDED, NULL); // ignore result
	//
	//				IShellItem *psiDesktop;
	//				hr = SHCreateItemInKnownFolder(FOLDERID_Desktop, 0, NULL, IID_PPV_ARGS(&psiDesktop));
	//				if (SUCCEEDED(hr))
	//				{
	//					// Add hidden root
	//					m_shellView.p_nstc->AppendRoot(psiDesktop, SHCONTF_FOLDERS, NSTCRS_HIDDEN | NSTCRS_EXPANDED, NULL); // ignore result
	//					psiDesktop->Release();
	//				}
	//				psiFavorites->Release();
	//			}
	//		}
	//	}
	//
	//	//ShowWindow(m_parentWnd, SW_NORMAL);
	//	//UpdateWindow(_hdlg);
	//
	//	if (m_parentWnd)
	//	{
	//		LONG rslt = SetWindowLong(, GWL_STYLE, WS_CHILDWINDOW);
	//		CHECK(rslt != 0, "Could not change explorer window style : " + QString::number(GetLastError()));
	//		HWND oldId = SetParent(, m_parentWnd);
	//		CHECK(oldId, "Could not change explorer window parent : " + QString::number(GetLastError()));
	//	}
	//
	//	return success();
	//}



	// WebBrowser
	/////////////

	// The trick !!!
	// - new win32 window (proxy) -> can embed a window from another process
	// - embedding Qt widget : can embed the proxy window because it belongs to the same process
	//                         while it can't directly embed the explorer window !

	// Create a new empty window
	m_parentWnd = CreateTheWindow(L"Win32 SCEP");
	CHECK(m_parentWnd, "Could not create win32 window");
	SetWindowLongPtr(m_parentWnd, GWLP_USERDATA, (LONG_PTR) this);

	//m_parentWnd = parentWnd;

	// Create web browser
	HRESULT hr = CoCreateInstance(	CLSID_ShellBrowserWindow,
									nullptr,
									CLSCTX_LOCAL_SERVER,
									IID_PPV_ARGS(&m_webBrowser.p_wb)	);
	CHECK(SUCCEEDED(hr), "Unable to create web browser window : " + GetErrorAsString(hr));


	// Paramétrage fenêtre
	//////////////////////

//	Sleep(200);
//
	//hr = CoAllowSetForegroundWindow(m_webBrowser.p_wb.get(), 0);
	//CHECK(SUCCEEDED(hr), "Unable to allow foreground explorer window : " + GetErrorAsString(hr));

	// Get web browser window id
	m_webBrowser.m_wb_hwnd = 0;
	hr = m_webBrowser.p_wb->get_HWND((SHANDLE_PTR*) &m_webBrowser.m_wb_hwnd);
	CHECK(SUCCEEDED(hr), "Could not get web browser window handle : " + GetErrorAsString(hr));
	//qDebug() << "Web browser window handle = " << m_webBrowser.m_wb_hwnd;

	// If a parent window is given, reparent explorer window
	if (m_parentWnd)
	{
		LONG rslt = SetWindowLong(m_webBrowser.m_wb_hwnd, GWL_STYLE, WS_CHILDWINDOW);
		CHECK(rslt != 0, "Could not change web browser window style : " + QString::number(GetLastError()));
		HWND oldId = SetParent(m_webBrowser.m_wb_hwnd, m_parentWnd);
		CHECK(oldId, "Could not change web browser window parent : " + QString::number(GetLastError()));
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
	CHECK(SUCCEEDED(hr), "Could not set initial folder for explorer window : " + GetErrorAsString(hr));
	
	hr = m_webBrowser.p_wb->Navigate2(&varTarget, &vEmpty, &vEmpty, &vEmpty, &vEmpty);
	CHECK(SUCCEEDED(hr), "Unable to set initial folder for explorer window : " + GetErrorAsString(hr));

	hr = VariantClear(&varTarget);
	if (! SUCCEEDED(hr))
	{
		qWarning() << "Could not set clear initial folder variant : " + GetErrorAsString(hr);
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
//	if (m_wndproc_hwn)
//	{
//		CALL( ShellViewWndProc::Unregister(m_wndproc_hwn) );
//	}

	m_treeControl.finalize();

	m_shellView.finalize();

	Teardown();

	m_webBrowser.finalize();

	return success();
}
//
//IFACEMETHODIMP ExplorerWrapper::QueryInterface(REFIID riid, void **ppv)
//{
//	static const QITAB qit[] =
//	{
//		QITABENT(ExplorerWrapper, IServiceProvider),
//		QITABENT(ExplorerWrapper, IExplorerBrowserEvents),
//		QITABENT(ExplorerWrapper, INameSpaceTreeControlEvents),
//		QITABENT(ExplorerWrapper, IExplorerPaneVisibility),
//		{ 0 },
//	};
//	return QISearch(this, qit, riid, ppv);
//}
////
//IFACEMETHODIMP_(ULONG) ExplorerWrapper::AddRef()
//{
//	return InterlockedIncrement(&m_cRef);
//}
////
//IFACEMETHODIMP_(ULONG) ExplorerWrapper::Release()
//{
//	long cRef = InterlockedDecrement(&m_cRef);
//	if (!cRef)
//	{
//		delete this;
//	}
//	return cRef;
//}
////
//IFACEMETHODIMP ExplorerWrapper::QueryService(REFGUID guidService, REFIID riid, void **ppv)
//{
//	*ppv = NULL;
//
//	HRESULT hr = E_NOINTERFACE;
//	if (guidService == SID_ExplorerPaneVisibility)
//	{
//		hr = QueryInterface(riid, ppv);
//	}
//	return hr;
//}
////
//IFACEMETHODIMP ExplorerWrapper::OnViewCreated(IShellView * psv)
//{
//	//// Store the shell view
//	//p_psv = psv;
//	//
//	//// Get the corresponding window
//	//HWND hwndshell;
//	//HRESULT hr = psv->GetWindow(&hwndshell);
//	//
//	//// Hook the window : set the new window procedure and the corresponding user data
//	//if (SUCCEEDED(hr))
//	//{
//	//	SetWindowLongPtr(hwndshell, GWLP_USERDATA, (LONG_PTR) this);
//	//	p_shellWindowProcOld = (WNDPROC) SetWindowLongPtr(hwndshell, GWLP_WNDPROC, (LONG_PTR) ShellWindowProcHook);
//	//}
//
//	return S_OK;
//}
////
//IFACEMETHODIMP ExplorerWrapper::OnNavigationPending(PCIDLIST_ABSOLUTE pidlFolder)
//{
//	NavigationPath loadingPath(pidlFolder);
//	emit loading(loadingPath);
//
//	return S_OK;
//}
////
//IFACEMETHODIMP ExplorerWrapper::OnNavigationComplete(PCIDLIST_ABSOLUTE pidlFolder)
//{
//	//m_currentPath = NavigationPath(pidlFolder);
//	//emit pathChanged(m_currentPath, true);
//
//	return S_OK;
//}
////
//IFACEMETHODIMP ExplorerWrapper::OnNavigationFailed(PCIDLIST_ABSOLUTE pidlFolder)
//{
//	//qDebug() << "Failed to navigated to " << NavigationPath(pidlFolder).displayPath();
//	//
//	//// Force GUI cleaning
//	//emit pathChanged(m_currentPath, false);
//
//	return S_OK;
//}
////
//IFACEMETHODIMP ExplorerWrapper::OnItemClick(IShellItem * /*psi*/, NSTCEHITTEST /*nstceHitTest*/, NSTCECLICKTYPE /*nstceClickType*/)
//{
//	return S_FALSE;
//}
////
//IFACEMETHODIMP ExplorerWrapper::OnPropertyItemCommit(IShellItem * /*psi*/)
//{
//	return S_FALSE;
//}
////
//IFACEMETHODIMP ExplorerWrapper::OnItemStateChanging(IShellItem * /*psi*/, NSTCITEMSTATE /*nstcisMask*/, NSTCITEMSTATE /*nstcisState*/)
//{
//	return S_OK;
//}
////
//IFACEMETHODIMP ExplorerWrapper::OnItemStateChanged(IShellItem * /*psi*/, NSTCITEMSTATE /*nstcisMask*/, NSTCITEMSTATE /*nstcisState*/)
//{
//	return S_OK;
//}
////
//IFACEMETHODIMP ExplorerWrapper::OnSelectionChanged(IShellItemArray * /*psiaSelection*/)
//{
//	return S_OK;
//}
////
//IFACEMETHODIMP ExplorerWrapper::OnKeyboardInput(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
//{
//	return S_FALSE;
//}
////
//IFACEMETHODIMP ExplorerWrapper::OnBeforeExpand(IShellItem * /*psi*/)
//{
//	return S_OK;
//}
////
//IFACEMETHODIMP ExplorerWrapper::OnAfterExpand(IShellItem * /*psi*/)
//{
//	return S_OK;
//}
////
//IFACEMETHODIMP ExplorerWrapper::OnBeginLabelEdit(IShellItem * /*psi*/)
//{
//	return S_OK;
//}
////
//IFACEMETHODIMP ExplorerWrapper::OnEndLabelEdit(IShellItem * /*psi*/)
//{
//	return S_OK;
//}
////
//IFACEMETHODIMP ExplorerWrapper::OnGetToolTip(IShellItem * /*psi*/, LPWSTR /*pszTip*/, int /*cchTip*/)
//{
//	return E_NOTIMPL;
//}
////
//IFACEMETHODIMP ExplorerWrapper::OnBeforeItemDelete(IShellItem * /*psi*/)
//{
//	return E_NOTIMPL;
//}
////
//IFACEMETHODIMP ExplorerWrapper::OnItemAdded(IShellItem * /*psi*/, BOOL /*fIsRoot*/)
//{
//	return E_NOTIMPL;
//}
////
//IFACEMETHODIMP ExplorerWrapper::OnItemDeleted(IShellItem * /*psi*/, BOOL /*fIsRoot*/)
//{
//	return E_NOTIMPL;
//}
////
//IFACEMETHODIMP ExplorerWrapper::OnBeforeContextMenu(IShellItem * /*psi*/, REFIID /*riid*/, void **ppv)
//{
//	*ppv = NULL;
//	return E_NOTIMPL;
//}
////
//IFACEMETHODIMP ExplorerWrapper::OnAfterContextMenu(IShellItem * /*psi*/, IContextMenu * /*pcmIn*/, REFIID /*riid*/, void **ppv)
//{
//	*ppv = NULL;
//	return E_NOTIMPL;
//}
////
//IFACEMETHODIMP ExplorerWrapper::OnBeforeStateImageChange(IShellItem * /*psi*/)
//{
//	return S_OK;
//}
////
//IFACEMETHODIMP ExplorerWrapper::OnGetDefaultIconIndex(IShellItem * /*psi*/, int * /*piDefaultIcon*/, int * /*piOpenIcon*/)
//{
//	return E_NOTIMPL;
//}
////
//HRESULT ExplorerWrapper::GetPaneState(REFEXPLORERPANE ep, EXPLORERPANESTATE *peps)
//{
//	using GUIDandString = std::pair<GUID, QString>;
//	static std::vector<GUIDandString> eps = 
//	{
//		{ EP_NavPane, "EP_NavPane" },
//		{ EP_Commands, "EP_Commands" },
//		{ EP_Commands_Organize, "EP_Commands_Organize" },
//		{ EP_Commands_View, "EP_Commands_View" },
//		{ EP_DetailsPane, "EP_DetailsPane" },
//		{ EP_PreviewPane, "EP_PreviewPane" },
//		{ EP_QueryPane, "EP_QueryPane" },
//		{ EP_AdvQueryPane, "EP_AdvQueryPane" },
//		{ EP_StatusBar, "EP_StatusBar" },
//		{ EP_Ribbon, "EP_Ribbon" },
//	};
//
//	auto ite = std::find_if(eps.begin(), eps.end(), [ep](const GUIDandString& guidAndString) {return guidAndString.first == ep;} );
//	assert(ite != eps.end());
//	if (ite != eps.end())
//	{
//		qDebug() << ite->second;
//	}
//
//	//*peps = EPS_DONTCARE;
//	*peps = EPS_DEFAULT_ON | EPS_INITIALSTATE | EPS_FORCE;
//	//*peps = EPS_DEFAULT_OFF | EPS_INITIALSTATE | EPS_FORCE;
//	//*peps = EPS_DEFAULT_OFF;
//	return S_OK;
//}
// BrowserListener
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

				// IShellBrowser !!!
				// - InsertMenuSB !
				hr = psp->QueryService(SID_STopLevelBrowser, IID_IShellBrowser, (void**)&m_shellView.p_sb);
				CHECK(SUCCEEDED(hr), "Could not get top level browser service : " + GetErrorAsString(hr));

				// Namespace tree control
				hr = psp->QueryService(SID_SNavigationPane, IID_PPV_ARGS(&m_treeControl.p_nstc));
				CHECK(SUCCEEDED(hr), "Could not get namespace tree control : " + GetErrorAsString(hr));

				// Namespace tree control window handle
				hr = m_shellView.p_sb->GetControlWindow(FCW_TREE, &m_treeControl.m_nstc_hwnd);
				CHECK(SUCCEEDED(hr), "Could not get namespace tree control window handle : " + GetErrorAsString(hr));

				return success();
			};

			ErrorPtr pError = endInit();
			if (pError)
			{
				qCritical() << "End of ExplorerWrapper initialization failed : " << *pError;
			}
		}

		auto update = [this]() -> ErrorPtr
		{
			// IShellView
			m_shellView.p_sv.clear();
			HRESULT hr = m_shellView.p_sb->QueryActiveShellView(&m_shellView.p_sv);
			CHECK(SUCCEEDED(hr), "Could not get active shell view : " + GetErrorAsString(hr));

	//		hr = IUnknown_SetSite(m_shellView.p_sv.get(), static_cast<IServiceProvider*>(this));
	//		CHECK(SUCCEEDED(hr), "Could not set site to web browser: " + GetErrorAsString(hr));

			// IShellView window handle
			[[maybe_unused]] HWND sv_hwnd = m_shellView.m_sv_hwnd;
			hr = m_shellView.p_sv->GetWindow(&m_shellView.m_sv_hwnd);
			CHECK(SUCCEEDED(hr), "Could not get shell view window : " + GetErrorAsString(hr));

			// IFolderView
			m_shellView.p_fv.clear();
			hr = m_shellView.p_sv->QueryInterface(IID_IFolderView2, (void**)&m_shellView.p_fv);
			CHECK(SUCCEEDED(hr), "Could not get current folder view  : " + GetErrorAsString(hr));

			// Register IShellView in order to get OnSelectionChanged notification 
			Box<IDispatch> spdispView;
			m_shellView.p_sv->GetItemObject(SVGIO_BACKGROUND, IID_PPV_ARGS(&spdispView));
			spdispView->QueryInterface(IID_PPV_ARGS(&m_shellView.p_fvd));
			BrowserListener::InitView(m_shellView.p_fvd.get());

			return success();
		};

		ErrorPtr pError = update();
		if (pError)
		{
			qCritical() << "ExplorerWrapper update failed : " << *pError;
		}
		
		m_navigationCount++;
	}
}
//
void ExplorerWrapper::OnSelectionChanged()
{
	// Are we dealing with a middle click ?
	if (m_middleClickDateTime.has_value())
	{
		// Get click time and reset it
		const QDateTime middleClickDateTime = m_middleClickDateTime.value();
		m_middleClickDateTime = std::nullopt;

		// Is the click recent enough ?
		if (middleClickDateTime.msecsTo(QDateTime::currentDateTime()) < OffsetMs + DelayMs)
		{
			auto process = [this]() -> ErrorPtr
			{
				std::vector<NavigationPath> paths;
				CALL( getSelectedItems(paths, Pane::ShellView) );
				CHECK( paths.size() == 1, QString("[ExplorerWrapper::OnSelectionChanged] getSelectedItems failed : %1 selected items, expecting 1.").arg(paths.size()));
				
				const NavigationPath& path = paths[0];
				if (path.isExistingDirectory())
				{
					qDebug() << "Requesting new tab for " << path.displayPath();
					emit openNewTab(path, NewTabPosition::AfterCurrent, NewTabBehaviour::None);
				}

				return success();
			};

			// Need a certain delay in order to get everything processed and the item selection done
			// But don't know why...
			QTimer::singleShot(OffsetMs, [process]()
				{
					if (ErrorPtr pError = process())
						qCritical() << *pError;
				}
			);
		}
	}


}
////
//NavigationPath GetDisplayNameOf(IShellFolder* shellFolder, LPITEMIDLIST pidl, SHGDNF /*uFlags*/)
//{
//	STRRET StrRet;
//	StrRet.uType = STRRET_WSTR;
//
//	HRESULT hr = shellFolder->GetDisplayNameOf((PCUITEMID_CHILD) pidl, SHGDN_NORMAL + SHGDN_FORPARSING, &StrRet);
//	if (FAILED(hr))
//	{
//		qCritical() << "GetDisplayNameOf:hr:  " << hr;
//		return {};
//	}
//
//	switch (StrRet.uType)
//	{
//	case STRRET_CSTR :
//		qCritical() << "GetDisplayNameOf:unexpected return type:STRRET_CSTR :  " << StrRet.cStr;
//		return QString(StrRet.cStr);
//	case STRRET_WSTR :
//		return QString::fromWCharArray(StrRet.pOleStr);
//	case STRRET_OFFSET :
//		qCritical() << "GetDisplayNameOf:unexpected return type:STRRET_OFFSET :  " << (((char*)pidl) + StrRet.uOffset);
//		return QString(((char*)pidl) + StrRet.uOffset);
//	default:
//		return {};
//	}
//}
//
ErrorPtr ExplorerWrapper::getSelectedItems(std::vector<NavigationPath>& items, Pane pane)
{
	// Init
	items.clear();
	HRESULT hr = S_OK;


	// Get the current selection
	Box<IShellItemArray> pItemArray;
	if (pane == Pane::ShellView)
	{
		CHECK(m_shellView.p_sv, "ExplorerWrapper::getSelectedItem() : No current shell view.");

		hr = m_shellView.p_fv->GetSelection(FALSE, &pItemArray);
		if (hr != S_OK && hr != S_FALSE)
		{
			CHECK(SUCCEEDED(hr), "Could not get shell view selected items : " + GetErrorAsString(hr));
		}
	}
	else
	{
		CHECK(m_treeControl.p_nstc, "ExplorerWrapper::getSelectedItem() : No current tree control.");

		hr = m_treeControl.p_nstc->GetSelectedItems(&pItemArray);
		CHECK(SUCCEEDED(hr), "Could not get tree view selected items : " + GetErrorAsString(hr));
	}
	// 
	if (! pItemArray)
	{
		return success();
	}

	// Count
	DWORD count = 0;
	hr = pItemArray->GetCount(&count);
	CHECK(SUCCEEDED(hr), "Could not get selected items count : " + GetErrorAsString(hr));

	// To NavigationPath
	items.clear();
	items.reserve(count);

	for (DWORD i = 0; i < count; i++)
	{
		Box<IShellItem> pItem = nullptr;
		hr = pItemArray->GetItemAt(i, &pItem);
		CHECK(SUCCEEDED(hr), QString("Could not get selected item #%1 : %2").arg(i).arg(GetErrorAsString(hr)));

		PIDLIST_ABSOLUTE pidl;
		hr = SHGetIDListFromObject(pItem.get(), &pidl);
		CHECK(SUCCEEDED(hr), QString("Could not get selected item path #%1 : %2").arg(i).arg(GetErrorAsString(hr)));

		items.push_back( NavigationPath(pidl) );
	}

	return success();
}
//
ErrorPtr ExplorerWrapper::onTreeControlMiddleClick(POINT windowPos)
{
	Box<IShellItem> psi = nullptr;

	HRESULT hr = m_treeControl.p_nstc->HitTest(&windowPos, &psi);
	CHECK(SUCCEEDED(hr), QString("Could not get clicked item : ") + GetErrorAsString(hr));

	if (psi)
	{
		PIDLIST_ABSOLUTE pidl;
		hr = SHGetIDListFromObject(psi.get(), &pidl);
		CHECK(SUCCEEDED(hr), QString("Could not get selected item path : %1").arg(GetErrorAsString(hr)));

		NavigationPath path(pidl);
		emit openNewTab(path, NewTabPosition::AfterCurrent, NewTabBehaviour::None);
	}

	return success();
}
//
void ExplorerWrapper::onMiddleClick(HWND hwnd, POINT windowPos, POINT screenPos)
{
	// Active Window
	HWND active_hwn = GetActiveWindow();
	//qDebug() << "Active window = " << active_hwn;
	if (! active_hwn)
		return;


	// Inside ?
	auto isInside = [screenPos](HWND source_hwnd, HWND expected_hwnd, const QString& expectedClassName) -> bool
	{
		RECT rc;
		bool ok = GetWindowRect(expected_hwnd, &rc);
		if (! ok)
		{
			qWarning() << "GetWindowRect failed : " << GetLastErrorAsString();

			// This failure sometimes happen, we now relie on explorer implementation details

			// Parent window
			HWND parent_source_HWND = GetParent(source_hwnd);
			if (parent_source_HWND != nullptr)
			{
				// Parent window class name
				static constexpr int ClassNameSize = 1024;
				TCHAR className[ClassNameSize];
				int count = GetClassName(parent_source_HWND, className, ClassNameSize);
				assert(count != 0);

				// Comparison to the expected one
				QString qClassName = QString::fromWCharArray(className);
				return qClassName == expectedClassName;
			}
			else
			{
				return false;
			}
		}

		return PtInRect(&rc, screenPos);
	};

	bool sh_clicked = isInside(hwnd, m_shellView.m_sv_hwnd, "SHELLDLL_DefView");
	bool nstc_clicked = isInside(hwnd, m_treeControl.m_nstc_hwnd, "NamespaceTreeControl");

	//qDebug() << "    Shell view clicked :           : " << sh_clicked;
	//qDebug() << "    NameSpace Tree Control clicked : " << nstc_clicked;


	/*
	if (sh_clicked)
	{
		auto tryOpenNewTab = [&]() -> ErrorPtr
		{
			int count = 0;
			HRESULT hr = m_shellView.p_fv->ItemCount(SVGIO_ALLVIEW, &count);
			CHECK(SUCCEEDED(hr), "Could not get folder view item count : " + GetErrorAsString(hr));

			for (int i = 0; i < count; i++)
			{
				PITEMID_CHILD pidl;
				hr = m_shellView.p_fv->Item(i, &pidl);
				CHECK(SUCCEEDED(hr), QString("Could not get folder view item #%1 : ").arg(i) + GetErrorAsString(hr));

				POINT upperLeft;
				hr = m_shellView.p_fv->GetItemPosition(pidl, &upperLeft);
				CoTaskMemFree(pidl);
				CHECK(SUCCEEDED(hr), QString("Could not get upper left position for folder view item #%1 : ").arg(i) + GetErrorAsString(hr));

				POINT dims;
				hr = m_shellView.p_fv->GetSpacing(&dims); // TODO factoriser ?
				CHECK(SUCCEEDED(hr), QString("Could not get dimensions for folder view item #%1 : ").arg(i) + GetErrorAsString(hr));


				bool ok_x = (upperLeft.x <= windowPos.x) && (windowPos.x < upperLeft.x+dims.x);
				bool ok_y = (upperLeft.y <= windowPos.y) && (windowPos.y < upperLeft.y+dims.y);
				if (ok_x && ok_y)
				{
					Box<IShellFolder> ppshf;
					hr = m_shellView.p_fv->GetFolder(IID_IShellFolder, (void**) &ppshf);
					CHECK(SUCCEEDED(hr), "Could not get shell folder : " + GetErrorAsString(hr));
					
					NavigationPath path = GetDisplayNameOf(ppshf.get(), pidl, SHGDN_NORMAL + SHGDN_FORPARSING);

					qDebug() << "Requesting new tab for " << path.displayPath();
					emit openNewTab(path, NewTabPosition::AfterCurrent, NewTabBehaviour::None);

					return success();
				}
			}

			return success();

		
		};

		QTimer::singleShot(0, [=]() {
				ErrorPtr pError = tryOpenNewTab();
				if (pError)
				{
					qCritical() << "Could not open new tab : " << *pError;
				}
			}
		);
	}
	else if (nstc_clicked)
	{

	}*/

	
	if (sh_clicked)
	{
		// Clear selection

		auto clearSelection = [this]() -> ErrorPtr
		{
			int count = 0;
			HRESULT hr = m_shellView.p_fv->ItemCount(SVGIO_ALLVIEW, &count);
			CHECK(SUCCEEDED(hr), "Could not get folder view item count : " + GetErrorAsString(hr));

			for (int i = 0; i < count; i++)
			{
				PITEMID_CHILD pidl = nullptr;
				hr = m_shellView.p_fv->Item(i, &pidl);
				CHECK(SUCCEEDED(hr), QString("Could not get folder view item #%1 : ").arg(i) + GetErrorAsString(hr));

				DWORD flags = 0;
				hr = m_shellView.p_fv->GetSelectionState(pidl, &flags);
				CHECK(SUCCEEDED(hr), QString("Could not get selection state for folder view item #%1 : ").arg(i) + GetErrorAsString(hr));

				if (flags & SVSI_SELECT)
				{
					flags = flags & ~SVSI_SELECT;
					hr = m_shellView.p_fv->SelectItem(i, flags);
					CHECK(SUCCEEDED(hr), QString("Could not set selection state for folder view item #%1 : ").arg(i) + GetErrorAsString(hr));
				}
			}

			return success();
		};

		auto endProcess = [this, clearSelection]()
		{
			m_middleClickDateTime = std::nullopt;
			ErrorPtr pError = clearSelection();
			if (pError)
			{
				qCritical() << "Failed to clear shell view selection : " << *pError;
			}

			// Left click in order to try selecting
			m_middleClickDateTime = QDateTime::currentDateTime();
			INPUT inputs[2];
			inputs[0].type = INPUT_MOUSE;
			inputs[0].mi.dx = 0;
			inputs[0].mi.dy = 0;
			inputs[0].mi.mouseData = 0;
			inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
			inputs[0].mi.time = 0;
			inputs[0].mi.dwExtraInfo = 0;

			inputs[1].type = INPUT_MOUSE;
			inputs[1].mi.dx = 0;
			inputs[1].mi.dy = 0;
			inputs[1].mi.mouseData = 0;
			inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
			inputs[1].mi.time = 0;
			inputs[1].mi.dwExtraInfo = 0;

			SendInput(2, inputs, sizeof(INPUT));

			// see OnSelectionChanged for the following !
		};

		// clearSelection() fails if called in this event processing method
		// so we require to postpone it after the event has been processed !
		QTimer::singleShot(0, endProcess);

		

		//// Let the win 32 event loop process the inputs and try to open a new tab on the selected item
		//
		//auto requestNewTabOnSelectedItem = [=]() -> ErrorPtr
		//{
		//	std::vector<NavigationPath> paths;
		//	CALL( getSelectedItems(paths, sh_clicked ? Pane::ShellView : Pane::TreeView) );
		//	CHECK(paths.size() == 1, QString("%1 selected items, expecting 1.").arg(paths.size()));
		//
		//	const NavigationPath& path = paths[0];
		//	qDebug() << "Requesting new tab for " << path.displayPath();
		//	emit openNewTab(path, NewTabPosition::AfterCurrent, NewTabBehaviour::None);
		//
		//	return success();
		//};
		//
		//
		//QTimer::singleShot(0, [=]()
		//	{
		//		ErrorPtr pError = requestNewTabOnSelectedItem();
		//		if (pError)
		//		{
		//			qWarning() << "Unable to open new tab : " << *pError;
		//		}
		//	}
		//);

	}
	/*
	if (sh_clicked)
	{
		// Left click in order to try selecting
		m_middleClickDateTime = QDateTime::currentDateTime();
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

		

		QTimer::singleShot(0, [this]() { qApp->processEvents(); qApp->processEvents(); OnSelectionChanged();} );
	}*/

	else if (nstc_clicked)
	{
	
		// map to window
		POINT nstcPos = windowPos;
		MapWindowPoints(hwnd, m_treeControl.m_nstc_hwnd, &nstcPos, 1);

		// INamespaceTreeControl::HitTest() fails if called in this event processing method
		// so we require to postpone it after the event has been processed !
		QTimer::singleShot(0, [this, nstcPos]() {
				ErrorPtr pError = this->onTreeControlMiddleClick(nstcPos);
				if (pError)
				{
					qCritical() << "Namespace tree control middle click failure : " << *pError;
				}
			}
		);
	}

	/*if (hwn == m_webBrowser.m_wb_hwnd)
	{
		qDebug() << "Focused window = Web Browser (1)";
	}
	else if (hwn == m_treeControl.m_nstc_hwnd)
	{
		qDebug() << "Focused window = NameSpace Tree Control (1)";
	}
	else
	{
		// Unfortunately relies on explorer implementation details..

		// Let's watch the parent
		static constexpr int ClassNameSize = 1024;
		TCHAR className[ClassNameSize];
		int count = GetClassName(GetParent(hwn), className, ClassNameSize);
		assert(count != 0);

		QString qClassName = QString::fromWCharArray(className);
		if (qClassName == "SHELLDLL_DefView")
		{
			qDebug() << "Focused window = Web Browser (2)";
		}
		else if (qClassName == "NamespaceTreeControl")
		{
			qDebug() << "Focused window = NameSpace Tree Control (2)";
		}
		else
		{
			qDebug() << "Focused window = Unknown --> " << hwn;
		}
	}*/
}
//
////
//NavigationPath GetDisplayNameOf(IShellFolder* shellFolder, LPITEMIDLIST pidl, SHGDNF /*uFlags*/)
//{
//	STRRET StrRet;
//	StrRet.uType = STRRET_WSTR;
//
//	HRESULT hr = shellFolder->GetDisplayNameOf((PCUITEMID_CHILD) pidl, SHGDN_NORMAL + SHGDN_FORPARSING, &StrRet);
//	if (FAILED(hr))
//	{
//		qCritical() << "GetDisplayNameOf:hr:  " << hr;
//		return {};
//	}
//
//	switch (StrRet.uType)
//	{
//	case STRRET_CSTR :
//		qCritical() << "GetDisplayNameOf:unexpected return type:STRRET_CSTR :  " << StrRet.cStr;
//		return QString(StrRet.cStr);
//	case STRRET_WSTR :
//		return QString::fromWCharArray(StrRet.pOleStr);
//	case STRRET_OFFSET :
//		qCritical() << "GetDisplayNameOf:unexpected return type:STRRET_OFFSET :  " << (((char*)pidl) + StrRet.uOffset);
//		return QString(((char*)pidl) + StrRet.uOffset);
//	default:
//		return {};
//	}
//}
////
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
////
//void ExplorerWrapper::requestingFakeContextMenu()
//{
//	m_middleClickDateTime = QDateTime::currentDateTime();
//}
////
//bool ExplorerWrapper::contextMenuRequired()
//{
//	bool handled = false;
//
//	static constexpr qint64 MaxDelayMs = 200;
//	static const QDateTime Epoch = QDateTime::fromSecsSinceEpoch(0);
//
//	// Are we requesting a fake context menu ?
//	const QDateTime middleClickDateTime = m_middleClickDateTime.value_or(Epoch);
////	if (middleClickDateTime.msecsTo(QDateTime::currentDateTime()) < MaxDelayMs)
//	{
//		// Get the focused item
//		std::optional<NavigationPath> path;
//
//		//{
//		//	int iItem = -1;
//		//	HRESULT hr = m_shellView.p_fv->GetFocusedItem(&iItem);
//		//	if (SUCCEEDED(hr))
//		//	{
//		//		PITEMID_CHILD ppidl; // leak ??
//		//		hr = m_shellView.p_fv->Item(iItem, &ppidl);
//		//		if (SUCCEEDED(hr))
//		//		{
//		//			Box<IShellFolder> ppshf;
//		//			hr = m_shellView.p_fv->GetFolder(IID_IShellFolder, (void**) &ppshf);
//		//			if (SUCCEEDED(hr))
//		//			{
//		//				path = GetDisplayNameOf(ppshf.get(), ppidl, SHGDN_NORMAL + SHGDN_FORPARSING);
//		//			}
//		//		}
//		//	}
//		//}
//
//		std::vector<Box<IShellItem>> shellItems;
//		ErrorPtr pError = getSelectedItems(shellItems);
//		if ( (! pError) && (shellItems.size() == 1) )
//		{
//			// Get the name of the selected item
//			PWSTR pszName = nullptr;
//			HRESULT hr = shellItems[0]->GetDisplayName(SIGDN_NORMALDISPLAY, &pszName);
//			if (SUCCEEDED(hr))
//			{
//				// Path of the clicked item
//				path = currentPath().childPath(QString::fromWCharArray(pszName));
//			}
//			
//			// Clean up
//			if (pszName)
//				CoTaskMemFree(pszName);
//		}
//
//		// If we have a focused item and it corresponds to an existing folder, we ask for a new tab
//		if (path.has_value() && path.value().isExistingDirectory())
//		{
//			qDebug() << "Requesting new tab for " << path.value().displayPath();
//			emit openNewTab(path.value(), NewTabPosition::AfterCurrent, NewTabBehaviour::None);
//		}
//
//		// We handled it !
//		handled = true;
//	}
//
//	// End
//	m_middleClickDateTime = std::nullopt;
//	return handled;
//}
//
//
//
//ErrorPtr ShellViewWndProc::initialize()
//{
//#ifdef ONE_SINGLE_HOOK
//	m_hook = SetWindowsHookEx(WH_CALLWNDPROC, &sv_call_wndproc, dll_hins, 0/*threadId*/);
//	//m_hook = SetWindowsHookEx(WH_CALLWNDPROCRET, &sv_call_wndretproc, dll_hins, 0/*threadId*/);
//	//m_hook = SetWindowsHookEx(WH_GETMESSAGE, &sv_call_message, dll_hins, 0/*threadId*/);
//	//m_hook = SetWindowsHookEx(WH_SHELL, &sv_call_shell, dll_hins, 0/*threadId*/);
//
//	
//	//m_hook = SetWindowsHookEx(WH_MOUSE, &sv_mouseproc, nullptr, GetCurrentThreadId());
//	CHECK(m_hook, "Could not set window procedure callback on shell view window: " + GetLastErrorAsString());
//#endif //ONE_SINGLE_HOOK
//
//	return success();
//}
////
//ErrorPtr ShellViewWndProc::finalize()
//{
//#ifdef ONE_SINGLE_HOOK
//	if (m_hook)
//	{
//		BOOL ok = UnhookWindowsHookEx(m_hook);
//		if (! ok)
//			qDebug() << "Failure to unhook : " << GetLastErrorAsString();
//		m_hook = nullptr;
//	}
//#endif //ONE_SINGLE_HOOK
//
//	return success();
//}
////
//ErrorPtr ShellViewWndProc::registerExplorerWrapper(HWND sv_hwnd, [[maybe_unused]] DWORD threadId, ExplorerWrapper* pExplorerWrapper)
//{
//	assert(sv_hwnd);
//	assert(pExplorerWrapper);
//	assert(m_explorerWrappers.find(sv_hwnd) == m_explorerWrappers.end());
//
//#ifdef ONE_SINGLE_HOOK
//	m_explorerWrappers[sv_hwnd] = pExplorerWrapper;
//#else
//	HHOOK hook = SetWindowsHookEx(WH_CALLWNDPROC, &sv_call_wndproc, dll_hins, threadId/*GetCurrentThreadId()*/);
//
//	//HINSTANCE hins = nullptr;
//	//BOOL ok = GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, L"shell32", &hins);
//	//CHECK(ok == TRUE, "Could not load shell32.dll: " + GetLastErrorAsString());
//	////HINSTANCE hins = LoadLibrary(L"shell32.dll");
//	//CHECK(hins != nullptr, "Could not load shell32.dll: " + GetLastErrorAsString());
//	//HHOOK hook = SetWindowsHookEx(WH_CALLWNDPROC, &sv_call_wndproc, hins/*dll_hins*/, threadId);
//
//	//HWND g_ProgWin = NULL;
//	//while (!g_ProgWin)
//	//{
//	//    g_ProgWin = GetShellWindow();
//	//    if (!g_ProgWin)
//	//    {
//	//        Sleep(100);
//	//    }
//	//}
//	////printf("Progman: %d\n", g_ProgWin);
//	//DWORD progThread = GetWindowThreadProcessId(
//	//    g_ProgWin,
//	//    NULL
//	//);
//	//
//	//HHOOK hook = SetWindowsHookEx(WH_CALLWNDPROC, &sv_call_wndproc, dll_hins, progThread);
//	
//
//	//m_hook = SetWindowsHookEx(WH_CALLWNDPROCRET, &sv_call_wndretproc, hins/*dll_hins*/, threadId);
//
//	//HHOOK hook = SetWindowsHookEx(WH_GETMESSAGE, &sv_call_message, nullptr, 0/*threadId*/);
//
//	CHECK(hook, "Could not set window procedure callback on shell view window: " + GetLastErrorAsString());
//
//	m_explorerWrappers[sv_hwnd] = {pExplorerWrapper, threadId, hook};
//#endif //ONE_SINGLE_HOOK
//
//	return success();
//}
////
//ErrorPtr ShellViewWndProc::unregisterExplorerWrapper(HWND sv_hwnd)
//{
//	assert(sv_hwnd);
//	auto ite = m_explorerWrappers.find(sv_hwnd);
//	if (ite != m_explorerWrappers.end())
//	{
//#ifndef ONE_SINGLE_HOOK
//		if (ite->second.hook)
//		{
//			BOOL ok = UnhookWindowsHookEx(ite->second.hook);
//			if (! ok)
//				qDebug() << "Failure to unhook : " << GetLastErrorAsString();
//		}
//#endif //ONE_SINGLE_HOOK
//		m_explorerWrappers.erase(ite);
//	}
//
//	return success();
//}
////
//ExplorerWrapper* ShellViewWndProc::explorerWrapper(HWND sv_hwnd)
//{
//	assert(sv_hwnd);
//	auto ite = m_explorerWrappers.find(sv_hwnd);
//#ifdef ONE_SINGLE_HOOK
//	return (ite != m_explorerWrappers.end()) ? ite->second : nullptr;
//#else
//	return (ite != m_explorerWrappers.end()) ? ite->second.ptr_wrapper : nullptr;
//#endif //ONE_SINGLE_HOOK
//}
////
////
////
//extern "C"
//LRESULT CALLBACK sv_call_message(int nCode, WPARAM wParam, LPARAM lParam)
//{
//	if (nCode >= 0)
//	{
//		if (MSG* pMSG = (MSG*) lParam)
//		{
//			qDebug() << pMSG->message;
//
//			if (pMSG->message == WM_PARENTNOTIFY && (LOWORD(pMSG->wParam) == WM_MBUTTONDOWN))
//			{
//				qDebug() << "Middle click !";
//
//			}
//			else if (pMSG->message == WM_CONTEXTMENU)
//			{
//				qDebug() << "Context menu !";
//
//			}
//		}
//	}
//
//	return CallNextHookEx(	HHOOK(),
//							nCode,
//							wParam,
//							lParam	);
//}
////
////
////
//extern "C"
//LRESULT CALLBACK sv_call_shell(int nCode, WPARAM wParam, LPARAM lParam)
//{
//	qDebug() << nCode;
//
//	if (nCode == HSHELL_APPCOMMAND)
//	{
//		//short cmd  = GET_APPCOMMAND_LPARAM(lParam);
//
//		WORD uDevice = GET_DEVICE_LPARAM(lParam);
//
//		WORD dwKeys = GET_KEYSTATE_LPARAM(lParam);
//
//		if ( (uDevice == FAPPCOMMAND_MOUSE) && (dwKeys == MK_RBUTTON) )
//		{
//			qDebug() << "Middle click !";
//		}
//		else if ( (uDevice == FAPPCOMMAND_MOUSE) && (dwKeys == MK_MBUTTON) )
//		{
//			qDebug() << "Middle click !";
//		}
//	}
//
//	return CallNextHookEx(	HHOOK(),
//							nCode,
//							wParam,
//							lParam	);
//}
////
////
////
//extern "C"
//LRESULT CALLBACK sv_call_wndproc(int nCode, WPARAM wParam, LPARAM lParam)
//{
//	if (nCode == HC_ACTION)
//	{
//		if (CWPSTRUCT* pCWP = (CWPSTRUCT*) lParam)
//		{
//			//if (pCWP->message != 49661)
//			//	qDebug() << pCWP->message;
//
//			// Middle click
//			///////////////
//
//			if (pCWP->message == WM_PARENTNOTIFY && (LOWORD(pCWP->wParam) == WM_MBUTTONDOWN))
//			{
//				qDebug() << "Middle click !";
//
//				if (ExplorerWrapper* pExplorerWrapper = ShellViewWndProc::GetExplorerWrapper(pCWP->hwnd))
//				{
//					// Cannot directly identify the item that was clicked (if any)
//					// - Requesting a context menu
//					// - Handling the request with the WM_CONTEXTMENU message (see later)
//					// - Getting the focused item (if any) and requesting a new tab
//
//					pExplorerWrapper->requestingFakeContextMenu();
//
//					//qDebug() << "requestingFakeContextMenu !";
//
//					{
//						INPUT input;
//						input.type = INPUT_MOUSE;
//						input.mi.dx = 0;
//						input.mi.dy = 0;
//						input.mi.mouseData = 0;
//						input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
//						input.mi.time = 0;
//						input.mi.dwExtraInfo = 0;
//						SendInput(1, &input, sizeof(INPUT));
//					}
//					{
//						INPUT input;
//						input.type = INPUT_MOUSE;
//						input.mi.dx = 0;
//						input.mi.dy = 0;
//						input.mi.mouseData = 0;
//						input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
//						input.mi.time = 0;
//						input.mi.dwExtraInfo = 0;
//						SendInput(1, &input, sizeof(INPUT));
//					}
//
//					//qApp->processEvents();
//
//
//					//// Simulate right clic
//					//{
//					//	INPUT input;
//					//	input.type = INPUT_MOUSE;
//					//	input.mi.dx = 0;
//					//	input.mi.dy = 0;
//					//	input.mi.mouseData = 0;
//					//	input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
//					//	input.mi.time = 0;
//					//	input.mi.dwExtraInfo = 0;
//					//	SendInput(1, &input, sizeof(INPUT));
//					//}
//					//{
//					//	INPUT input;
//					//	input.type = INPUT_MOUSE;
//					//	input.mi.dx = 0;
//					//	input.mi.dy = 0;
//					//	input.mi.mouseData = 0;
//					//	input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
//					//	input.mi.time = 0;
//					//	input.mi.dwExtraInfo = 0;
//					//	SendInput(1, &input, sizeof(INPUT));
//					//}
//					//
//					//// Process
//					//qApp->processEvents();
//					//
//					//// Simulate esc
//					//{
//					//	INPUT input;
//					//	input.type = INPUT_KEYBOARD;
//					//	input.ki.wVk = VK_ESCAPE;
//					//	input.ki.wScan = 0;
//					//	input.ki.dwFlags = 0;
//					//	input.ki.time = 0;
//					//	input.ki.dwExtraInfo = 0;
//					//	SendInput(1, &input, sizeof(INPUT));
//					//}
//					//{
//					//	INPUT input;
//					//	input.type = INPUT_KEYBOARD;
//					//	input.ki.wVk = VK_ESCAPE;
//					//	input.ki.wScan = 0;
//					//	input.ki.dwFlags = KEYEVENTF_KEYUP;
//					//	input.ki.time = 0;
//					//	input.ki.dwExtraInfo = 0;
//					//	SendInput(1, &input, sizeof(INPUT));
//					//}
//
//					QTimer::singleShot(0, [=]() { pExplorerWrapper->contextMenuRequired(); });
//
//					//pExplorerWrapper->processMiddleClickEvent();
//					// We do not return, so CallNextHookEx will be called !
//				}
//			}
//			//else if (pCWP->message == WM_CONTEXTMENU)
//			//{
//			//	qDebug() << "Context menu !";
//			//
//			//	if (ExplorerWrapper* pExplorerWrapper = ShellViewWndProc::GetExplorerWrapper(pCWP->hwnd))
//			//	{
//			//		bool handled = pExplorerWrapper->contextMenuRequired();
//			//		if (handled)
//			//			return 0;
//			//	}
//			//}
//
//			//*/
//		}
//	}
//	//else if (nCode < 0)
//	//{
//		return CallNextHookEx(	HHOOK(),
//								nCode,
//								wParam,
//								lParam	);
//	//}
//}
////
//
//extern "C"
//LRESULT CALLBACK sv_call_wndretproc(int nCode, WPARAM wParam, LPARAM lParam)
//{
//	if (nCode == HC_ACTION)
//	{
//		if (CWPRETSTRUCT* pCWP = (CWPRETSTRUCT*) lParam)
//		{
//			// Middle click ?
//			if (pCWP->message == WM_PARENTNOTIFY && (LOWORD(pCWP->wParam) == WM_MBUTTONDOWN))
//			{
//				if (ExplorerWrapper* pExplorerWrapper = ShellViewWndProc::GetExplorerWrapper(pCWP->hwnd))
//				{
//					pExplorerWrapper->processMiddleClickEvent();
//					// We do not return, so CallNextHookEx will be called !
//				}
//			}
//		}
//	}
//	//else if (nCode < 0)
//	//{
//		return CallNextHookEx(	HHOOK(),
//								nCode,
//								wParam,
//								lParam	);
//	//}
//}
//
//extern "C" LRESULT CALLBACK sv_mouseproc(int nCode, WPARAM wParam, LPARAM lParam)
//{
//	return CallNextHookEx(	HHOOK(),
//							nCode,
//							wParam,
//							lParam	);
//}

