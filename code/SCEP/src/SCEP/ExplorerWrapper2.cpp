#include <SCEP/ExplorerWrapper2.h>
#include <SCEP/Theme.h>
#include <SCEP/win32_utils.h>
//
#include <linkollector-win/dark_mode.h>
//
#include <QUrl>
#include <QFileInfo>
#include <QtDebug>
//
#include <shobjidl.h>
#include <shlwapi.h>
#include <knownfolders.h>
#include <shlobj_core.h>
#include <strsafe.h>
#include <windowsx.h>
#include <winuser.h>
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
//inline HRESULT GetItemFromView(IFolderView2 *pfv, int iItem, REFIID riid, void **ppv)
//{
//	*ppv = nullptr;
//
//	HRESULT hr = S_OK;
//
//	if (iItem == -1)
//	{
//		hr = pfv->GetSelectedItem(-1, &iItem); // Returns S_FALSE if none selected
//	}
//
//	if (S_OK == hr)
//	{
//		hr = pfv->GetItem(iItem, riid, ppv);
//	}
//	else
//	{
//		hr = E_FAIL;
//	}
//	return hr;
//}
////
//inline ErrorPtr ShellExecuteItem(HWND hwnd, PCWSTR pszVerb, IShellItem *psi)
//{
//	// how to activate a shell item, use ShellExecute().
//	PIDLIST_ABSOLUTE pidl;
//	HRESULT hr = SHGetIDListFromObject(psi, &pidl);
//	CHECK(SUCCEEDED(hr), "Could not get absolute ID list from current item : " + GetLastErrorAsString());
//	
//	SHELLEXECUTEINFO ei = { sizeof(ei) };
//	ei.fMask = SEE_MASK_INVOKEIDLIST;
//	ei.hwnd = hwnd;
//	ei.nShow = SW_NORMAL;
//	ei.lpIDList = pidl;
//	ei.lpVerb = pszVerb;
//
//	BOOL ok = ShellExecuteEx(&ei);
//	CoTaskMemFree(pidl);
//
//	CHECK(ok == TRUE, "Could not execute : " + GetLastErrorAsString());
//
//	return success();
//}
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
ErrorPtr ExplorerWrapper2::initialize(Theme* ptr_theme, const QString& path)
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

	// Dark mode
	if (ptr_theme)
	{
		using namespace linkollector::win;
		init_dark_mode_support();
		enable_dark_mode(m_hwnd, ptr_theme->type() == Theme::Type::Dark);
	}

	CALL( onInitialize(path) );

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
	return m_currentPath;
}
//
ErrorPtr ExplorerWrapper2::finalize()
{
	if (p_peb)
	{
		IUnknown_SetSite(p_peb, nullptr);
		p_peb->Unadvise(m_dwCookie);
		p_peb->Destroy();
		p_peb->Release();
		p_peb = nullptr;
	}

	if (m_hwnd != 0)
	{
		DestroyWindow(m_hwnd);
		m_hwnd = 0;
	}

	return success();
}
//
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
//
IFACEMETHODIMP_(ULONG) ExplorerWrapper2::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}
//
IFACEMETHODIMP_(ULONG) ExplorerWrapper2::Release()
{
	long cRef = InterlockedDecrement(&m_cRef);
	if (!cRef)
	{
		delete this;
	}
	return cRef;
}
//
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
//
IFACEMETHODIMP ExplorerWrapper2::OnViewCreated(IShellView * psv)
{
	p_psv = psv;
	HWND hwndshell;
	HRESULT hr = psv->GetWindow(&hwndshell);

	SetWindowLongPtr(hwndshell, GWLP_USERDATA, (LONG_PTR) this);
	p_shellWindowProcOld = (WNDPROC) SetWindowLongPtr(hwndshell, GWLP_WNDPROC, (LONG_PTR) ShellWindowProcHook);

	return S_OK;
}
//
IFACEMETHODIMP ExplorerWrapper2::OnNavigationPending(PCIDLIST_ABSOLUTE pidlFolder)
{
	QString loadingPath = path(pidlFolder);
	emit loading(loadingPath);

	return S_OK;
}
//
IFACEMETHODIMP ExplorerWrapper2::OnNavigationComplete(PCIDLIST_ABSOLUTE pidlFolder)
{
	m_currentPath = path(pidlFolder);
	emit pathChanged(m_currentPath);

	return S_OK;
}
//
IFACEMETHODIMP ExplorerWrapper2::OnNavigationFailed(PCIDLIST_ABSOLUTE /* pidlFolder */)
{
	// Force GUI cleaning
	emit pathChanged(m_currentPath);

	return S_OK;
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
	INT_PTR iRet = 1; // default for all handled cases in switch below

	switch (uMsg)
	{
	case WM_ERASEBKGND:
		// To avoid flicking when the window is moved, we have to ignore this message
		break;
	case WM_SIZE:
		if (p_peb)
		{
			RECT rc = GetWindowRectInClient(m_hwnd);
			p_peb->SetRect(nullptr, rc);
		}
		RedrawWindow(m_hwnd, nullptr, nullptr, RDW_INVALIDATE);
		break;
	/*case WM_PARENTNOTIFY:
	{
		WORD event = LOWORD(wParam);
		switch (event)
		{
		case WM_RBUTTONDOWN:
		{
			WORD x = LOWORD(lParam);
			WORD y = HIWORD(lParam);
			qDebug() << "WM_RBUTTONDOWN (" << x << ", " << y << ")";
			IShellItem* psi = nullptr;
			[[maybe_unused]] ErrorPtr pError = getItem(x, y, IID_PPV_ARGS(&psi));
		
			// TODO handle context menu request
			// For now, just use the default implementation
			iRet = DefWindowProc(hwnd, uMsg, wParam, lParam);
			break;
		}
		case WM_MBUTTONDOWN:
		{
			//WORD x = LOWORD(lParam);
			//WORD y = HIWORD(lParam);
			//qDebug() << "WM_MBUTTONDOWN (" << x << ", " << y << ")";
			//IShellItem* psi = nullptr;
			//[[maybe_unused]] ErrorPtr pError = getItem(x, y, IID_PPV_ARGS(&psi));

			// TODO handle open in the tab request
			iRet = DefWindowProc(hwnd, uMsg, wParam, lParam);
			break;
		}
		default:
			iRet = DefWindowProc(hwnd, uMsg, wParam, lParam);
			break;
		}
		break;
	}*/
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

	// note, SetSite(nullptr) happens in finalize()
	IUnknown_SetSite(p_peb, static_cast<IServiceProvider *>(this));

	FOLDERSETTINGS fs = {};
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
////
//ErrorPtr ExplorerWrapper2::getSelectedItem(REFIID riid, void **ppv)
//{
//	*ppv = nullptr;
//	CHECK(p_peb, "ExplorerWrapper2::getSelectedItem() : No current instance.");
//
//	IFolderView2* pfv = nullptr;
//	HRESULT hr = p_peb->GetCurrentView(IID_PPV_ARGS(&pfv));
//	CHECK(SUCCEEDED(hr), "Could not get current view : " + GetLastErrorAsString());
//
//	hr = GetItemFromView(pfv, -1, riid, ppv);
//	pfv->Release();
//	CHECK(SUCCEEDED(hr), "Could not get current item in current view : " + GetLastErrorAsString());
//
//	return success();
//}
////
//ErrorPtr ExplorerWrapper2::onSelChange()
//{
//	IShellItem *psi = nullptr;
//	CALL( getSelectedItem(IID_PPV_ARGS(&psi)) );
//
//	PWSTR pszName = nullptr;
//	HRESULT hr = psi->GetDisplayName(SIGDN_NORMALDISPLAY, &pszName);
//	psi->Release();
//	CHECK(SUCCEEDED(hr), "Could not get display name : " + GetLastErrorAsString());
//
//	//SetDlgItemText(_hdlg, IDC_NAME, pszName);
//	CoTaskMemFree(pszName);
//
//	return success();
//}
////
//ErrorPtr ExplorerWrapper2::onOpenItem()
//{
//	IShellItem *psi = nullptr;
//
//	CALL( getSelectedItem(IID_PPV_ARGS(&psi)) );
//	CALL( ShellExecuteItem(m_hwnd, nullptr, psi) );
//
//	psi->Release();
//
//	return success();
//}
////
//ErrorPtr ExplorerWrapper2::getItem(WORD x, WORD y, REFIID riid, void **ppv)
//{
//	*ppv = nullptr;
//	CHECK(p_peb, "ExplorerWrapper2::getItem() : No current instance.");
//	
//	IFolderView2* pfv = nullptr;
//	HRESULT hr = p_peb->GetCurrentView(IID_PPV_ARGS(&pfv));
//	CHECK(SUCCEEDED(hr), "Could not get current view : " + GetLastErrorAsString());
//	
//	hr = GetItemFromView(pfv, -1, riid, ppv);
//	pfv->Release();
//	CHECK(SUCCEEDED(hr), "Could not get current item in current view : " + GetLastErrorAsString());
//	
//
//
//	return success();
//}
//
#define MIN_SHELL_ID 1
#define MAX_SHELL_ID 30000
//
LRESULT CALLBACK ExplorerWrapper2::ShellWindowProcHook(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	ExplorerWrapper2* pThis = (ExplorerWrapper2*) GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (msg)
	{
	case WM_CONTEXTMENU:
	{
		HMENU hmenu = pThis->CreateCustomPopupMenu();
		if (hmenu != nullptr)
		{
			HWND hwndshell;
			pThis->p_psv->GetWindow(&hwndshell);
			long shellId = TrackPopupMenu(hmenu, TPM_RETURNCMD | TPM_LEFTALIGN, GET_X_LPARAM(lp), GET_Y_LPARAM(lp), 0, hwndshell, nullptr);
			CloseHandle(hmenu);

			if ((shellId >= 0) && (shellId <= MAX_SHELL_ID))
			{
				CMINVOKECOMMANDINFO ici;
				ZeroMemory(&ici, sizeof(ici));
				ici.cbSize = sizeof(CMINVOKECOMMANDINFO);
				ici.lpVerb = MAKEINTRESOURCEA(shellId - MIN_SHELL_ID);
				ici.nShow = SW_SHOWNORMAL;

				HRESULT hr1 = pThis->p_contextMenu2->InvokeCommand(&ici);

			}
			else if (shellId > MAX_SHELL_ID)
			{
				pThis->notifyContextMenuCustomOption(shellId - MAX_SHELL_ID - 1, pThis->m_contextMenuFocusedPath);
				pThis->m_contextMenuFocusedPath = {};
			}
			pThis->p_contextMenu2->Release();
			pThis->p_contextMenu2 = nullptr;
		}
		return 0; // handled
	}
	default:
		break;
	}

	return CallWindowProc(pThis->p_shellWindowProcOld, hwnd, msg, wp, lp);
}
//
QString GetDisplayNameOf(IShellFolder* shellFolder, LPITEMIDLIST pidl, SHGDNF /*uFlags*/)
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
HMENU ExplorerWrapper2::CreateCustomPopupMenu()
{
	IFolderView2 * ppifw;
	HRESULT hr = p_psv->QueryInterface(IID_IFolderView2, (void**) &ppifw);

	int iItem = -1;
	hr = ppifw->GetFocusedItem(&iItem);
	if (! SUCCEEDED(hr))
	{
		qCritical() << "ExplorerWrapper2::CreateCustomPopupMenu: GetFocusedItem(), iItem: " << iItem << ", hr: " << hr;
		ppifw->Release();
		return nullptr;
	}

	PITEMID_CHILD ppidl;
	hr = ppifw->Item(iItem, &ppidl);
	if (! SUCCEEDED(hr))
	{
		qCritical() << "ExplorerWrapper2::CreateCustomPopupMenu: Item(), iItem: " << iItem << ", hr: " << hr;
		ppifw->Release();
		return nullptr;
	}

	IShellFolder * ppshf;
	hr = ppifw->GetFolder(IID_IShellFolder, (void**) &ppshf);
	if (! SUCCEEDED(hr))
	{
		qCritical() << "ExplorerWrapper2::CreateCustomPopupMenu: GetFolder(), iItem: " << iItem << ", hr: " << hr;
		ppifw->Release();
		return nullptr;
	}

	m_contextMenuFocusedPath = GetDisplayNameOf(ppshf, ppidl, SHGDN_NORMAL + SHGDN_FORPARSING);

	hr = ppshf->GetUIObjectOf(nullptr, 1, (PCUITEMID_CHILD_ARRAY) &ppidl, IID_IContextMenu, nullptr, (void**) &p_contextMenu2);
	if (! SUCCEEDED(hr))
	{
		qCritical() << "ExplorerWrapper2::CreateCustomPopupMenu: GetUIObjectOf(), iItem: " << iItem << ", hr: " << hr;
		ppifw->Release();
		ppshf->Release();
		return nullptr;
	}

	HMENU hmenu = CreatePopupMenu();
	hr = p_contextMenu2->QueryContextMenu(hmenu, 0, MIN_SHELL_ID, MAX_SHELL_ID, CMF_EXPLORE);

	std::map<long, QString> mapReturn = getContextMenuCustomOptions(m_contextMenuFocusedPath);

	if (! mapReturn.empty())
	{
		UINT position = 0;

		BOOL ok = TRUE;
		for (std::map<long, QString>::iterator p = mapReturn.begin(); p != mapReturn.end(); ++p)
		{
			long lOption = p->first;
			std::wstring option = p->second.toStdWString();
			ok = InsertMenu(hmenu, position++, MF_BYPOSITION | MF_STRING, (UINT_PTR) MAX_SHELL_ID + lOption +1, option.c_str());
		}

		ok = InsertMenu(hmenu, position++, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
	}

	return hmenu;
}
//
std::map<long, QString> ExplorerWrapper2::getContextMenuCustomOptions(const QString& contextMenuFocusedPath)
{
	std::map<long, QString> rslt;
	if (! contextMenuFocusedPath.isEmpty())
	{
		QFileInfo fi(contextMenuFocusedPath);
		if (fi.exists() && fi.isDir())
		{
			rslt[0] = tr("Open in a new tab");
		}
	}
	return rslt;
}
//
void ExplorerWrapper2::notifyContextMenuCustomOption(int iOption, const QString& contextMenuFocusedPath)
{
	assert(iOption == 0);
	if (! contextMenuFocusedPath.isEmpty())
	{
		QFileInfo fi(contextMenuFocusedPath);
		if (fi.exists() && fi.isDir())
		{
			emit openNewTab(contextMenuFocusedPath);
		}
	}
}
//
