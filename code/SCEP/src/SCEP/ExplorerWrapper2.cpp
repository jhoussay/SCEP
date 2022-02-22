#include <SCEP/ExplorerWrapper2.h>
#include <SCEP/win32_utils.h>
//
#include <QUrl>
#include <QFileInfo>
#include <QtDebug>
//
#include <shobjidl.h>
#include <shlwapi.h>
#include <knownfolders.h>
#include <shlobj_core.h>
//
inline RECT GetWindowRectInClient(HWND hwnd)
{
	RECT rc;

	GetWindowRect(hwnd, &rc);
	MapWindowPoints(GetDesktopWindow(), hwnd, (POINT*)&rc, 2);

	// +/- 1 (or 2 on the other side) in order to hide borders
	// +left/top in order to recenter the window

	rc.right += rc.left + 2;
	// Warning : not a bug here, use left to get correct bottom offset correction !
	rc.bottom += rc.left + 2;

	rc.left = -1;
	rc.top = -1;

	return rc;
}
//
inline IShellItem* CreateShellItem(const QString& path)
{
	static constexpr int BufferfSize = 4096;

	IShellItem* pShellItem = nullptr;

	QFileInfo fi(path);
	if (! path.isEmpty() && fi.exists(path) && fi.isDir())
	{
		std::wstring wpath = path.toStdWString();
		std::array<wchar_t, BufferfSize> fullPath;

		if (GetFullPathName(wpath.c_str(), BufferfSize, fullPath.data(), nullptr))
		{
			if (SUCCEEDED(SHCreateItemFromParsingName(fullPath.data(), nullptr, IID_PPV_ARGS(&pShellItem))))
			{
				return pShellItem;
			}
		}
	}
	else
	{
		qWarning() << "Invalid path " << path << ", defaulting to desktop";
		if (SUCCEEDED(SHCreateItemInKnownFolder(FOLDERID_Desktop, 0, nullptr, IID_PPV_ARGS(&pShellItem))))
		{
			return pShellItem;
		}
	}

	return nullptr;
}
//
inline HRESULT GetItemFromView(IFolderView2 *pfv, int iItem, REFIID riid, void **ppv)
{
	*ppv = nullptr;

	HRESULT hr = S_OK;

	if (iItem == -1)
	{
		hr = pfv->GetSelectedItem(-1, &iItem); // Returns S_FALSE if none selected
	}

	if (S_OK == hr)
	{
		hr = pfv->GetItem(iItem, riid, ppv);
	}
	else
	{
		hr = E_FAIL;
	}
	return hr;
}
//
inline ErrorPtr ShellExecuteItem(HWND hwnd, PCWSTR pszVerb, IShellItem *psi)
{
	// how to activate a shell item, use ShellExecute().
	PIDLIST_ABSOLUTE pidl;
	HRESULT hr = SHGetIDListFromObject(psi, &pidl);
	CHECK(SUCCEEDED(hr), "Could not get absolute ID list from current item : " + GetLastErrorAsString());
	
	SHELLEXECUTEINFO ei = { sizeof(ei) };
	ei.fMask = SEE_MASK_INVOKEIDLIST;
	ei.hwnd = hwnd;
	ei.nShow = SW_NORMAL;
	ei.lpIDList = pidl;
	ei.lpVerb = pszVerb;

	BOOL ok = ShellExecuteEx(&ei);
	CoTaskMemFree(pidl);

	CHECK(ok == TRUE, "Could not execute : " + GetLastErrorAsString());

	return success();
}
//
inline QString path(PCIDLIST_ABSOLUTE pidlFolder)
{
	static constexpr int BufferfSize = 4096;
	std::array<wchar_t, BufferfSize> fullPath;
	SHGetPathFromIDList(pidlFolder, fullPath.data());

	return QString::fromWCharArray(fullPath.data());
}
//
//
//
ExplorerWrapper2::ExplorerWrapper2(QObject* pParent)
	:	QObject(pParent)
	,	m_cRef(1)
	,	m_hwnd(nullptr)
	,	m_fPerformRenavigate(false)
	,	p_peb(nullptr)
	//,	m_hrOleInit(OleInitialize(nullptr))
{}
//
ExplorerWrapper2::~ExplorerWrapper2()
{
	//if (SUCCEEDED(m_hrOleInit))
	//{
	//	OleUninitialize();
	//}
}
//
ErrorPtr ExplorerWrapper2::initialize(const QString& path)
{
	static LPCWSTR sClassName = L"MyClass";

	// Create & register the class
	WNDCLASSEX WndClass;
	WndClass.cbSize = sizeof(WNDCLASSEX);
	WndClass.style = 0u;
	WndClass.lpfnWndProc = s_WndProc;
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
	m_hwnd = CreateWindowEx(WS_EX_STATICEDGE,
							sClassName,
							L"SCEP Win32 Window",
							WS_OVERLAPPEDWINDOW,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							320,//640,
							240,//480,
							nullptr,
							nullptr,
							nullptr,
							this);
	CHECK(m_hwnd != nullptr, GetLastErrorAsString());

//	UpdateWindow(m_hwnd);

	CALL( onInitialize(path) );

//	ShowWindow(m_hwnd, SW_SHOW);

	return success();
}
//
HWND ExplorerWrapper2::hwnd() const
{
	return m_hwnd;
}
//
void ExplorerWrapper2::setVisible(bool visible)
{
	if (m_hwnd != 0)
	{
		ShowWindow(m_hwnd, visible ? SW_SHOW : SW_HIDE);
	}
}
//
ErrorPtr ExplorerWrapper2::setCurrentPath(const QString& path)
{
	CHECK(p_peb, "ExplorerWrapper2::setCurrentPath() : No current instance.");

	QFileInfo pathInfo(path);
	CHECK(pathInfo.exists() && pathInfo.isDir(), "Invalid path");

	IShellItem* psi = CreateShellItem(path);
	if (psi)
	{
		HRESULT hr = p_peb->BrowseToObject(psi, 0);
		psi->Release();
		CHECK(SUCCEEDED(hr), "Unable to set explorer current path : " + GetLastErrorAsString());
	}
	else
	{
		return createError("Unable to set explorer current path.");
	}

	return success();
}
//
QString ExplorerWrapper2::currentPath() const
{
	// TODO
	return {};

	//if (p_webBrowser)
	//{
	//	BSTR url;
	//	HRESULT hr = p_webBrowser->get_LocationURL(&url);
	//	if (SUCCEEDED(hr))
	//	{
	//		QUrl qUrl( QString::fromWCharArray(url) );
	//		SysFreeString(url);
	//
	//		return qUrl.toLocalFile();
	//	}
	//	else
	//	{
	//		qWarning() << "ExplorerWrapper2::currentPath() : Could not get current location.";
	//		return {};
	//	}
	//}
	//else
	//{
	//	qWarning() << "ExplorerWrapper2::currentPath() : No current instance.";
	//	return {};
	//}
}
//
// IUnknown
IFACEMETHODIMP ExplorerWrapper2::QueryInterface(REFIID riid, void **ppv)
{
	static const QITAB qit[] =
	{
		QITABENT(ExplorerWrapper2, IServiceProvider),
		QITABENT(ExplorerWrapper2, IExplorerBrowserEvents),
		{ 0 },
	};
	return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) ExplorerWrapper2::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

IFACEMETHODIMP_(ULONG) ExplorerWrapper2::Release()
{
	long cRef = InterlockedDecrement(&m_cRef);
	if (!cRef)
	{
		delete this;
	}
	return cRef;
}

// IServiceProvider
IFACEMETHODIMP ExplorerWrapper2::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
	*ppv = NULL;

	HRESULT hr = E_NOINTERFACE;
	if (guidService == SID_SExplorerBrowserFrame)
	{
		hr = QueryInterface(riid, ppv);
	}
	return hr;
}

// IExplorerBrowserEvents
IFACEMETHODIMP ExplorerWrapper2::OnViewCreated(IShellView * /* psv */)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP ExplorerWrapper2::OnNavigationPending(PCIDLIST_ABSOLUTE /* pidlFolder */)
{
	return S_OK;
}

IFACEMETHODIMP ExplorerWrapper2::OnNavigationComplete(PCIDLIST_ABSOLUTE pidlFolder)
{
	//if (m_fPerformRenavigate)
	{
		QString currentPath = path(pidlFolder);
		emit pathChanged(currentPath);

		//KillTimer(_hdlg, IDT_SEARCHSTART);
		//_OnSearch();
	//	m_fPerformRenavigate = FALSE;
	}
	return S_OK;
}

IFACEMETHODIMP ExplorerWrapper2::OnNavigationFailed(PCIDLIST_ABSOLUTE /* pidlFolder */)
{
	return E_NOTIMPL;
}
//
INT_PTR CALLBACK ExplorerWrapper2::s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ExplorerWrapper2 *pssa = nullptr;
	if (uMsg == WM_CREATE)
	{
		if (pssa = (ExplorerWrapper2*) LPCREATESTRUCT(lParam)->lpCreateParams)
		{
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) pssa);
		}
	}
	else
	{
		pssa = (ExplorerWrapper2*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
	}

	if (pssa)
	{
		return pssa->wndProc(hwnd, uMsg, wParam, lParam);
	}
	else
	{
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}
//
LRESULT CALLBACK ExplorerWrapper2::wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//switch(uMsg)
	//{
	//case WM_CLOSE:
	//	DestroyWindow(hwnd);
	//	break;
	//case WM_DESTROY:
	//	PostQuitMessage(0);
	//	break;
	//default:
	//	return DefWindowProc(hwnd, uMsg, wParam, lParam);
	//}
	//return 0;



	INT_PTR iRet = 1;   // default for all handled cases in switch below

	switch (uMsg)
	{
		//case WM_INITDIALOG:
		//	_OnInitializeDialog();
		//	break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		hwnd = 0;
		break;

	case WM_DESTROY:
		// Clean up the search timer
		//KillTimer(_hdlg, IDT_SEARCHSTART);
		if (ErrorPtr pError = onDestroy())
			displayError(pError);

		//PostQuitMessage(0); // JHO
		break;

	case WM_SIZE:
		if (p_peb)
		{
			RECT rc = GetWindowRectInClient(m_hwnd);
			p_peb->SetRect(nullptr, rc);
		}

		RedrawWindow(m_hwnd, nullptr, nullptr, RDW_INVALIDATE);
		break;

		//case WM_COMMAND:
		//{
		//	const UINT idCmd = LOWORD(wParam);
		//	switch (idCmd)
		//	{
		//	case IDOK:
		//	case IDCANCEL:
		//	case IDCLOSE:
		//		return EndDialog(_hdlg, idCmd);
		//
		//	case IDC_OPEN_ITEM:
		//		_OnOpenItem();
		//		break;
		//
		//	case IDC_SEARCHBOX:
		//		switch (HIWORD(wParam))
		//		{
		//		case EN_CHANGE:
		//			// Update search box icon if necessary
		//			_UpdateSearchIcon();
		//			// Delay search processing to aggregate keystrokes
		//			SetTimer(_hdlg, IDT_SEARCHSTART, SEARCH_TIMER_DELAY, nullptr);
		//			break;
		//		}
		//		break;
		//	}
		//}
		//break;

	case WM_TIMER:
	{
		// Search timer delay expired, process search
		//KillTimer(_hdlg, IDT_SEARCHSTART);
		//_OnSearch();
	}
	break;

	default:
		iRet = DefWindowProc(hwnd, uMsg, wParam, lParam);
		break;
	}
	return iRet;
}
//
ErrorPtr ExplorerWrapper2::onInitialize(const QString& path)
{
	RECT rc = GetWindowRectInClient(m_hwnd);

	HRESULT hr = CoCreateInstance(CLSID_ExplorerBrowser, nullptr, CLSCTX_INPROC, IID_PPV_ARGS(&p_peb));
	CHECK(SUCCEEDED(hr), "Could not create explorer browser : " + GetLastErrorAsString());

	// note, SetSite(nullptr) happens in _OnDestroyDialog()
	IUnknown_SetSite(p_peb, static_cast<IServiceProvider *>(this));

	FOLDERSETTINGS fs = {};
	//fs.ViewMode = FVM_ICON;
	//fs.fFlags = FWF_HIDEFILENAMES | FWF_NOSUBFOLDERS | FWF_NOCOLUMNHEADER;
	fs.ViewMode = FVM_AUTO;
	fs.fFlags = FWF_AUTOARRANGE | FWF_NOWEBVIEW;

	hr = p_peb->Initialize(m_hwnd, &rc, &fs);
	CHECK(SUCCEEDED(hr), "Could not initialize explorer browser : " + GetLastErrorAsString());

	hr = p_peb->Advise(this, &m_dwCookie);
	CHECK(SUCCEEDED(hr), "Could not finish explorer browser initialization : " + GetLastErrorAsString());

	p_peb->SetOptions(EBO_SHOWFRAMES);

	CALL( setCurrentPath(path) );

	return success();
}
//
ErrorPtr ExplorerWrapper2::onDestroy()
{
	if (p_peb)
	{
		IUnknown_SetSite(p_peb, nullptr);
		p_peb->Unadvise(m_dwCookie);
		p_peb->Destroy();
		p_peb->Release();
		p_peb = nullptr;
	}

	return success();
}
//
ErrorPtr ExplorerWrapper2::getSelectedItem(REFIID riid, void **ppv)
{
	*ppv = nullptr;
	CHECK(p_peb, "ExplorerWrapper2::getSelectedItem() : No current instance.");

	IFolderView2* pfv = nullptr;
	HRESULT hr = p_peb->GetCurrentView(IID_PPV_ARGS(&pfv));
	CHECK(SUCCEEDED(hr), "Could not get current view : " + GetLastErrorAsString());

	hr = GetItemFromView(pfv, -1, riid, ppv);
	pfv->Release();
	CHECK(SUCCEEDED(hr), "Could not get current item in current view : " + GetLastErrorAsString());

	return success();
}
//
ErrorPtr ExplorerWrapper2::onSelChange()
{
	IShellItem *psi = nullptr;
	CALL( getSelectedItem(IID_PPV_ARGS(&psi)) );

	PWSTR pszName = nullptr;
	HRESULT hr = psi->GetDisplayName(SIGDN_NORMALDISPLAY, &pszName);
	psi->Release();
	CHECK(SUCCEEDED(hr), "Could not get display name : " + GetLastErrorAsString());

	//SetDlgItemText(_hdlg, IDC_NAME, pszName);
	CoTaskMemFree(pszName);

	return success();
}
//
ErrorPtr ExplorerWrapper2::onOpenItem()
{
	IShellItem *psi = nullptr;

	CALL( getSelectedItem(IID_PPV_ARGS(&psi)) );
	CALL( ShellExecuteItem(m_hwnd, nullptr, psi) );

	psi->Release();

	return success();
}
//
