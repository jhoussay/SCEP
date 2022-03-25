#include <SCEP/ExplorerWrapper.h>
#include <SCEP/Theme.h>
#include <SCEP/win32_utils.h>
//
#include <linkollector-win/dark_mode.h>
//
#include <QTimer>
#include <QtDebug>
//
#include <array>
//
#include <shobjidl.h>
#include <shlwapi.h>
#include <knownfolders.h>
#include <shlobj_core.h>
#include <shobjidl_core.h>
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

	if (! path.isEmpty())
	{
		std::wstring wpath = path.toStdWString();
		if (SUCCEEDED(SHCreateItemFromParsingName(wpath.c_str(), nullptr, IID_PPV_ARGS(&pShellItem))))
		{
			return pShellItem;
		}
	}

	return nullptr;
}
//
//
//
ExplorerWrapper::ExplorerWrapper(Theme* ptrTheme, QObject* pParent)
	:	QObject(pParent)
	,	ptr_theme(ptrTheme)
	,	m_cRef(1)
	,	m_hwnd(nullptr)
	,	m_fPerformRenavigate(false)
	,	p_peb(nullptr)
	//,	m_hrOleInit(OleInitialize(nullptr))
{}
//
ExplorerWrapper::~ExplorerWrapper()
{
	//if (SUCCEEDED(m_hrOleInit))
	//{
	//	OleUninitialize();
	//}
}
//
ErrorPtr ExplorerWrapper::initialize(const NavigationPath& path)
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
	m_hwnd = CreateWindowEx(WS_EX_APPWINDOW,
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
	if (ptr_theme && (ptr_theme->effectiveStyle() == Theme::Style::Dark) )
	{
		linkollector::win::init_dark_mode_support();
		linkollector::win::enable_dark_mode(m_hwnd, true);
	}

	CALL( onInitialize(path) );

	return success();
}
//
HWND ExplorerWrapper::hwnd() const
{
	return m_hwnd;
}
//
void ExplorerWrapper::setVisible(bool visible)
{
	if (m_hwnd != 0)
	{
		ShowWindow(m_hwnd, visible ? SW_SHOW : SW_HIDE);
	}
}
//
ErrorPtr ExplorerWrapper::setCurrentPath(const NavigationPath& path)
{
	CHECK(p_peb, "ExplorerWrapper::setCurrentPath() : No current instance.");

	IShellItem* psi = CreateShellItem(path.internalPath());
	if (psi)
	{
		HRESULT hr = p_peb->BrowseToObject(psi, 0);
		psi->Release();
		CHECK(SUCCEEDED(hr), "Unable to set explorer current path \"" + path.displayPath() + "\" : " + GetLastErrorAsString());
	}
	else
	{
		emit pathChanged(path, false);
		return createError("Unable to set explorer current path \"" + path.displayPath() + "\"");
	}

	return success();
}
//
const NavigationPath& ExplorerWrapper::currentPath() const
{
	return m_currentPath;
}
//
ErrorPtr ExplorerWrapper::finalize()
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
IFACEMETHODIMP ExplorerWrapper::QueryInterface(REFIID riid, void **ppv)
{
	static const QITAB qit[] =
	{
		QITABENT(ExplorerWrapper, IServiceProvider),
		QITABENT(ExplorerWrapper, IExplorerBrowserEvents),
		{ 0 },
	};
	return QISearch(this, qit, riid, ppv);
}
//
IFACEMETHODIMP_(ULONG) ExplorerWrapper::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}
//
IFACEMETHODIMP_(ULONG) ExplorerWrapper::Release()
{
	long cRef = InterlockedDecrement(&m_cRef);
	if (!cRef)
	{
		delete this;
	}
	return cRef;
}
//
IFACEMETHODIMP ExplorerWrapper::QueryService(REFGUID guidService, REFIID riid, void **ppv)
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
IFACEMETHODIMP ExplorerWrapper::OnViewCreated(IShellView * psv)
{
	// Store the shell view
	p_psv = psv;

	// Get the corresponding window
	HWND hwndshell;
	HRESULT hr = psv->GetWindow(&hwndshell);

	// Hook the window : set the new window procedure and the corresponding user data
	if (SUCCEEDED(hr))
	{
		SetWindowLongPtr(hwndshell, GWLP_USERDATA, (LONG_PTR) this);
		p_shellWindowProcOld = (WNDPROC) SetWindowLongPtr(hwndshell, GWLP_WNDPROC, (LONG_PTR) ShellWindowProcHook);
	}

	return S_OK;
}
//
IFACEMETHODIMP ExplorerWrapper::OnNavigationPending(PCIDLIST_ABSOLUTE pidlFolder)
{
	NavigationPath loadingPath(pidlFolder);
	emit loading(loadingPath);

	return S_OK;
}
//
IFACEMETHODIMP ExplorerWrapper::OnNavigationComplete(PCIDLIST_ABSOLUTE pidlFolder)
{
	m_currentPath = NavigationPath(pidlFolder);
	emit pathChanged(m_currentPath, true);

	return S_OK;
}
//
IFACEMETHODIMP ExplorerWrapper::OnNavigationFailed(PCIDLIST_ABSOLUTE pidlFolder)
{
	qDebug() << "Failed to navigated to " << NavigationPath(pidlFolder).displayPath();

	// Force GUI cleaning
	emit pathChanged(m_currentPath, false);

	return S_OK;
}
//
INT_PTR CALLBACK ExplorerWrapper::s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Get (and store on CREATE) the current instance
	ExplorerWrapper* pThis = nullptr;
	if (uMsg == WM_CREATE)
	{
		if (pThis = (ExplorerWrapper*) LPCREATESTRUCT(lParam)->lpCreateParams)
		{
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) pThis);
		}
	}
	else
	{
		pThis = (ExplorerWrapper*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
	}

	// Call the window procedure
	if (pThis)
	{
		return pThis->wndProc(hwnd, uMsg, wParam, lParam);
	}
	else
	{
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}
//
LRESULT CALLBACK ExplorerWrapper::wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
	default:
		iRet = DefWindowProc(hwnd, uMsg, wParam, lParam);
		break;
	}
	return iRet;
}
//
ErrorPtr ExplorerWrapper::onInitialize(const NavigationPath& path)
{
	// Get window size
	RECT rc = GetWindowRectInClient(m_hwnd);

	// Create IExplorerBrowser
	HRESULT hr = CoCreateInstance(CLSID_ExplorerBrowser, nullptr, CLSCTX_INPROC, IID_PPV_ARGS(&p_peb));
	CHECK(SUCCEEDED(hr), "Could not create explorer browser : " + GetLastErrorAsString());

	// Register
	// note, SetSite(nullptr) happens in finalize()
	IUnknown_SetSite(p_peb, static_cast<IServiceProvider*>(this));

	// Initialize IExplorerBrowser
	FOLDERSETTINGS fs = {};
	fs.ViewMode = FVM_AUTO;
	fs.fFlags = FWF_AUTOARRANGE | FWF_NOWEBVIEW;
	hr = p_peb->Initialize(m_hwnd, &rc, &fs);
	CHECK(SUCCEEDED(hr), "Could not initialize explorer browser : " + GetLastErrorAsString());
	p_peb->SetOptions(EBO_SHOWFRAMES);

	// Initialize IExplorerBrowserEvents
	hr = p_peb->Advise(this, &m_dwCookie);
	CHECK(SUCCEEDED(hr), "Could not finish explorer browser initialization : " + GetLastErrorAsString());

	// Set initial path
	CALL( setCurrentPath(path) );

	return success();
}
//
ErrorPtr ExplorerWrapper::getSelectedItem(REFIID riid, void **ppv)
{
	// Check
	*ppv = nullptr;
	CHECK(p_peb && p_psv, "ExplorerWrapper::getSelectedItem() : No current instance.");

	// Get current view
	IFolderView2* pfv = nullptr;
	HRESULT hr = p_peb->GetCurrentView(IID_PPV_ARGS(&pfv));
	CHECK(SUCCEEDED(hr), "Could not get current view : " + GetLastErrorAsString());

	// Get current item in current view
	int iItem = -1;
	hr = pfv->GetSelectedItem(-1, &iItem); // Returns S_FALSE if none selected
	if (SUCCEEDED(hr))
	{
		hr = pfv->GetItem(iItem, riid, ppv);
	}
	pfv->Release();
	CHECK(SUCCEEDED(hr), "Could not get current item in current view : " + GetLastErrorAsString());

	return success();
}
//
#define MIN_SHELL_ID 1
#define MAX_SHELL_ID 30000
//
LRESULT CALLBACK ExplorerWrapper::ShellWindowProcHook(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ExplorerWrapper* pThis = (ExplorerWrapper*) GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (uMsg)
	{
	case WM_CONTEXTMENU:
	{
		// Generate a context menu on the clicked item with additional custom entries

		// Create the context menu
		HMENU hmenu = pThis->CreateCustomPopupMenu();
		if (hmenu != nullptr)
		{
			// Show the context menu and get the selected item
			HWND hwndshell;
			pThis->p_psv->GetWindow(&hwndshell);
			long shellId = TrackPopupMenu(	hmenu,
											TPM_RETURNCMD | TPM_LEFTALIGN,
											GET_X_LPARAM(lParam),
											GET_Y_LPARAM(lParam),
											0,
											hwndshell, 
											nullptr	);
			CloseHandle(hmenu);

			// Handle a regular choice
			if ((shellId >= 0) && (shellId <= MAX_SHELL_ID))
			{
				CMINVOKECOMMANDINFO ici;
				ZeroMemory(&ici, sizeof(ici));
				ici.cbSize = sizeof(CMINVOKECOMMANDINFO);
				ici.lpVerb = MAKEINTRESOURCEA(shellId - MIN_SHELL_ID);
				ici.nShow = SW_SHOWNORMAL;

				HRESULT hr1 = pThis->p_contextMenu2->InvokeCommand(&ici);

			}
			// Handle a custom choice
			else if (shellId > MAX_SHELL_ID)
			{
				pThis->notifyContextMenuCustomOption(shellId - MAX_SHELL_ID - 1, pThis->m_contextMenuFocusedPath);
				pThis->m_contextMenuFocusedPath = {};
			}
			// Clean up
			pThis->p_contextMenu2->Release();
			pThis->p_contextMenu2 = nullptr;
		}
		return 0; // handled
	}
	case WM_PARENTNOTIFY:
	{
		// Handle middle clicks and request a new tab if the click corresponds to an existing folder
		if (LOWORD(wParam) == WM_MBUTTONDOWN)
		{
			// Register the middle click
			pThis->m_middleClickDateTime = QDateTime::currentDateTime();
		
			// Simulate a left click a the same position in order to select the item
			// --> Handled through the win32 message loop
			INPUT input;
			input.type = INPUT_MOUSE;
			input.mi.dx = 0;
			input.mi.dy = 0;
			input.mi.mouseData = 0;
			input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
			input.mi.time = 0;
			input.mi.dwExtraInfo = 0;
			SendInput(1, &input, sizeof(INPUT));
		
			// Request a new tab on the newly selected item
			// --> Handled through the Qt event loop
			// --> Seems to always be handled AFTER the win32 input message !
			QTimer::singleShot(0, [=]()
			{
				// Are we dealing with a middle click ?
				if (pThis->m_middleClickDateTime.has_value())
				{
					static constexpr qint64 DelayMs = 200;
		
					// Is the click recent enough ?
					const QDateTime middleClickDateTime = pThis->m_middleClickDateTime.value();
					if (middleClickDateTime.msecsTo(QDateTime::currentDateTime()) < DelayMs)
					{
						// Get the selected item
						IShellItem* psi = nullptr;
						ErrorPtr pError = pThis->getSelectedItem(IID_PPV_ARGS(&psi));
						if (! pError)
						{
							// Get the name of the selected item
							PWSTR pszName = nullptr;
							HRESULT hr = psi->GetDisplayName(SIGDN_NORMALDISPLAY, &pszName);
							if (SUCCEEDED(hr))
							{
								// Path of the clicked item
								NavigationPath path = pThis->currentPath().childPath(QString::fromWCharArray(pszName));
		
								// Ask for a new tab if the clicked item corresponds to an existing folder
								if (path.isExistingDirectory())
								{
									emit pThis->openNewTab(NavigationPath(path), NewTabPosition::AfterCurrent, NewTabBehaviour::None);
								}
							}
							// Clean up
							if (pszName)
								CoTaskMemFree(pszName);
						}
						// Clean up
						if (psi)
							psi->Release();
					}
		
					// Clean up
					pThis->m_middleClickDateTime = std::nullopt;
				}
			});
		}
		break;
	}
	default:
		break;
	}

	return CallWindowProc(pThis->p_shellWindowProcOld, hwnd, uMsg, wParam, lParam);
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
HMENU ExplorerWrapper::CreateCustomPopupMenu()
{
	IFolderView2 * ppifw;
	HRESULT hr = p_psv->QueryInterface(IID_IFolderView2, (void**) &ppifw);

	int iItem = -1;
	hr = ppifw->GetFocusedItem(&iItem);
	if (! SUCCEEDED(hr))
	{
		qCritical() << "ExplorerWrapper::CreateCustomPopupMenu: GetFocusedItem(), iItem: " << iItem << ", hr: " << hr;
		ppifw->Release();
		return nullptr;
	}

	PITEMID_CHILD ppidl;
	hr = ppifw->Item(iItem, &ppidl);
	if (! SUCCEEDED(hr))
	{
		qCritical() << "ExplorerWrapper::CreateCustomPopupMenu: Item(), iItem: " << iItem << ", hr: " << hr;
		ppifw->Release();
		return nullptr;
	}

	IShellFolder * ppshf;
	hr = ppifw->GetFolder(IID_IShellFolder, (void**) &ppshf);
	if (! SUCCEEDED(hr))
	{
		qCritical() << "ExplorerWrapper::CreateCustomPopupMenu: GetFolder(), iItem: " << iItem << ", hr: " << hr;
		ppifw->Release();
		return nullptr;
	}

	m_contextMenuFocusedPath = GetDisplayNameOf(ppshf, ppidl, SHGDN_NORMAL + SHGDN_FORPARSING);

	hr = ppshf->GetUIObjectOf(nullptr, 1, (PCUITEMID_CHILD_ARRAY) &ppidl, IID_IContextMenu, nullptr, (void**) &p_contextMenu2);
	if (! SUCCEEDED(hr))
	{
		qCritical() << "ExplorerWrapper::CreateCustomPopupMenu: GetUIObjectOf(), iItem: " << iItem << ", hr: " << hr;
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
std::map<long, QString> ExplorerWrapper::getContextMenuCustomOptions(const NavigationPath& contextMenuFocusedPath)
{
	std::map<long, QString> rslt;
	if (contextMenuFocusedPath.isExistingDirectory())
	{
		rslt[0] = tr("Open in a new tab");
	}
	return rslt;
}
//
void ExplorerWrapper::notifyContextMenuCustomOption([[maybe_unused]] int iOption, const NavigationPath& contextMenuFocusedPath)
{
	assert(iOption == 0);
	if (contextMenuFocusedPath.isExistingDirectory())
	{
		emit openNewTab(contextMenuFocusedPath, NewTabPosition::AfterCurrent, NewTabBehaviour::None);
	}
}
//
