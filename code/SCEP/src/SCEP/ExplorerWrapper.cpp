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
#define MIN_SHELL_ID 1
#define MAX_SHELL_ID 30000
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
void simulateHotKey(IShellView* pSV, const std::initializer_list<SHORT>& vkeys)
{
	// Common
	MSG msg;
	msg.hwnd = 0;
	msg.lParam = 0;
	msg.time = 0;
	msg.pt = {0, 0};

	// Pressing keys
	msg.message = WM_KEYDOWN;
	for (SHORT vkey : vkeys)
	{
		msg.wParam = vkey;
		pSV->TranslateAccelerator(&msg);
	}

	// Releasing keys
	msg.message = WM_KEYUP;
	for (SHORT vkey : vkeys)
	{
		msg.wParam = vkey;
		pSV->TranslateAccelerator(&msg);
	}
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
							WS_OVERLAPPEDWINDOW /*| WS_CLIPCHILDREN | WS_CLIPSIBLINGS*/,
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

	Box<IShellItem> psi = CreateShellItem(path.internalPath());
	if (psi)
	{
		HRESULT hr = p_peb->BrowseToObject(psi.get(), 0);
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
		IUnknown_SetSite(p_peb.get(), nullptr);
		p_peb->Unadvise(m_dwCookie);
		p_peb->Destroy();
		p_peb.clear();
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
	//	QITABENT(ExplorerWrapper, IExplorerPaneVisibility),
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
	if ( (guidService == SID_SExplorerBrowserFrame) /*|| (guidService == SID_ExplorerPaneVisibility)*/ )
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
	HRESULT hr = psv->GetWindow(&m_hwnd_sv);

	// Hook the window : set the new window procedure and the corresponding user data
	if (SUCCEEDED(hr))
	{
		SetWindowLongPtr(m_hwnd_sv, GWLP_USERDATA, (LONG_PTR) this);
		p_shellWindowProcOld = (WNDPROC) SetWindowLongPtr(m_hwnd_sv, GWLP_WNDPROC, (LONG_PTR) ShellWindowProcHook);
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
//HRESULT ExplorerWrapper::GetPaneState(REFEXPLORERPANE ep, EXPLORERPANESTATE *peps)
//{
////	using GUIDandString = std::pair<GUID, QString>;
////	static std::vector<GUIDandString> eps = 
////	{
////		{ EP_NavPane, "EP_NavPane" },
////		{ EP_Commands, "EP_Commands" },
////		{ EP_Commands_Organize, "EP_Commands_Organize" },
////		{ EP_Commands_View, "EP_Commands_View" },
////		{ EP_DetailsPane, "EP_DetailsPane" },
////		{ EP_PreviewPane, "EP_PreviewPane" },
////		{ EP_QueryPane, "EP_QueryPane" },
////		{ EP_AdvQueryPane, "EP_AdvQueryPane" },
////		{ EP_StatusBar, "EP_StatusBar" },
////		{ EP_Ribbon, "EP_Ribbon" },
////	};
////
////	auto ite = std::find_if(eps.begin(), eps.end(), [ep](const GUIDandString& guidAndString) {return guidAndString.first == ep;} );
////	assert(ite != eps.end());
////	if (ite != eps.end())
////	{
////		qDebug() << ite->second;
////	}
//
//	*peps = EPS_DONTCARE;
//	//*peps = EPS_DEFAULT_ON | EPS_INITIALSTATE | EPS_FORCE;
//	//*peps = EPS_DEFAULT_OFF | EPS_INITIALSTATE | EPS_FORCE;
//	//*peps = EPS_DEFAULT_OFF;
//	return S_OK;
//}
//
void ExplorerWrapper::rename()
{
	simulateHotKey(p_psv, {VK_F2});
	SetFocus(m_hwnd_sv);
}
//
void ExplorerWrapper::copy()
{
	simulateHotKey(p_psv, {VK_CONTROL, 0x43/*C*/});
	SetFocus(m_hwnd_sv);
}
//
void ExplorerWrapper::cut()
{
	simulateHotKey(p_psv, {VK_CONTROL, 0x58/*X*/});
	SetFocus(m_hwnd_sv);
}
//
void ExplorerWrapper::paste()
{
	simulateHotKey(p_psv, {VK_CONTROL, 0x56/*V*/});
	SetFocus(m_hwnd_sv);
}
//
void ExplorerWrapper::del()
{
	simulateHotKey(p_psv, {VK_DELETE});
	SetFocus(m_hwnd_sv);
}
//
void ExplorerWrapper::forceDel()
{
	simulateHotKey(p_psv, {VK_SHIFT, VK_DELETE});
	SetFocus(m_hwnd_sv);
}
//
void ExplorerWrapper::selectAll()
{
	simulateHotKey(p_psv, {VK_CONTROL, 0x41/*A*/});
	SetFocus(m_hwnd_sv);
}
//
void ExplorerWrapper::mkDir()
{
	simulateHotKey(p_psv, {VK_CONTROL, VK_SHIFT, 0x4E/*N*/});
	SetFocus(m_hwnd_sv);
}
//
void ExplorerWrapper::undo()
{
	simulateHotKey(p_psv, {VK_CONTROL, 0x5A/*Z*/});
	SetFocus(m_hwnd_sv);
}
//
void ExplorerWrapper::redo()
{
	simulateHotKey(p_psv, {VK_CONTROL, 0x59/*Y*/});
	SetFocus(m_hwnd_sv);
}
//
void ExplorerWrapper::refresh()
{
	if (p_psv)
	{
		HRESULT hr = p_psv->Refresh();
		CHECK_VOID(SUCCEEDED(hr), QString("Could not refresh : ").arg(hr));
	}
	SetFocus(m_hwnd_sv);
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
	//qDebug() << "wndProc" << Qt::hex << uMsg;

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
	IUnknown_SetSite(p_peb.get(), static_cast<IServiceProvider*>(this));

	// Initialize IExplorerBrowser
	FOLDERSETTINGS fs = {};
	fs.ViewMode = FVM_AUTO;
	fs.fFlags = FWF_AUTOARRANGE | FWF_NOWEBVIEW;
	// FWF_CHECKSELECT : Turns on the check mode for the view.
	// FWF_AUTOCHECKSELECT : Windows Vista and later. Items can be selected using checkboxes.
	// FWF_USESEARCHFOLDER : Windows Vista and later. Use the search folder for stacking and searching.
	hr = p_peb->Initialize(m_hwnd, &rc, &fs);
	CHECK(SUCCEEDED(hr), "Could not initialize explorer browser : " + GetLastErrorAsString());
	p_peb->SetOptions(EBO_SHOWFRAMES);

	// Initialize IExplorerBrowserEvents
	hr = p_peb->Advise(this, &m_dwCookie);
	CHECK(SUCCEEDED(hr), "Could not finish explorer browser initialization : " + GetLastErrorAsString());

	// Set initial path
	CALL( setCurrentPath(path) );

//	// Trick to request all the panes
//	// https://microsoft.public.platformsdk.shell.narkive.com/eeyV7YsU/iexplorerpanevisibility-problems-in-vista
//	//QTimer::singleShot(0, [=]() {
//		WPARAM wparam = 0;
//		LPARAM lparam = (LPARAM) L"ShellState";
//		/*HRESULT*/ hr = SendMessage(m_hwnd_sv, WM_SETTINGCHANGE, wparam, lparam);
//		if (! SUCCEEDED(hr))
//		{
//			qWarning() << "Failed to send \"WM_SETTINGCHANGE\" message";
//		}
//	//});

	return success();
}
//
ErrorPtr ExplorerWrapper::getSelectedItems(std::vector<Box<IShellItem>>& shellItems)
{
	// Check
	shellItems.clear();
	CHECK(p_peb && p_psv, "ExplorerWrapper::getSelectedItem() : No current instance.");

	// Get current view on IFolderView2
	Box<IFolderView2> pfv = nullptr;
	HRESULT hr = p_peb->GetCurrentView(IID_PPV_ARGS(&pfv));
	CHECK(SUCCEEDED(hr), QString("Could not get current view : %1").arg(hr));

	// Get the current selection
	Box<IShellItemArray> pItemArray;
	hr = pfv->GetSelection(FALSE, &pItemArray);
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
LRESULT CALLBACK ExplorerWrapper::ShellWindowProcHook(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ExplorerWrapper* pThis = (ExplorerWrapper*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if (! pThis->p_psv)
	{
		qCritical() << "ExplorerWrapper::ShellWindowProcHook: Invalid shell view";
		return CallWindowProc(pThis->p_shellWindowProcOld, hwnd, uMsg, wParam, lParam);
	}

	//qDebug() << "ShellWindowProcHook" << Qt::hex << uMsg;

	switch (uMsg)
	{
	case WM_CONTEXTMENU:
	{
		// Request a custom menu -> Generate a context menu on the clicked item with additional custom entries
		// If a custom menu is required, handle it
		// Else, the regular menu will be shown with the "p_shellWindowProcOld" window procedure
		if (std::shared_ptr<CustomMenu> pCustomMenu = pThis->CreateCustomPopupMenu(MenuRequest::SelectedItems))
		{
			// Show the context menu and get the selected item
			HWND hwndshell;
			pThis->p_psv->GetWindow(&hwndshell);
			long shellId = TrackPopupMenu(	pCustomMenu->hmenu,
											TPM_RETURNCMD | TPM_LEFTALIGN,
											GET_X_LPARAM(lParam),
											GET_Y_LPARAM(lParam),
											0,
											hwndshell, 
											nullptr	);
		
			// Handle a regular choice
			if ((shellId >= MIN_SHELL_ID) && (shellId <= MAX_SHELL_ID))
			{
				// Get command string
				CHAR str[MAX_PATH];
				HRESULT hr = pCustomMenu->pContextMenu->GetCommandString(shellId - MIN_SHELL_ID, GCS_VERBA, nullptr, str, MAX_PATH);
				if (SUCCEEDED(hr))
				{
					//qDebug() << QString(str);

					// If a rename is required, we have to force it : the InvokeCommand won't work...
					if (QString(str) == "rename")
					{
						hr = pCustomMenu->pCurrentFolderView->DoRename();
						if (! SUCCEEDED(hr))
						{
							qCritical() << "InvokeCommand (" << str << ") failed: " << hr;
						}
					}
					// Fortunately, all the other InvokeCommand cases seem to work
					else
					{
						CMINVOKECOMMANDINFO ici;
						ZeroMemory(&ici, sizeof(ici));
						ici.cbSize = sizeof(CMINVOKECOMMANDINFO);
						ici.lpVerb = MAKEINTRESOURCEA(shellId - MIN_SHELL_ID);
						ici.nShow = SW_SHOWNORMAL;
		
						hr = pCustomMenu->pContextMenu->InvokeCommand(&ici);
						if (! SUCCEEDED(hr))
						{
							qCritical() << "InvokeCommand (" << str << ") failed: " << hr;
						}
					}
				}
			}
			// Handle a custom choice
			else if (shellId > MAX_SHELL_ID)
			{
				pThis->notifyContextMenuCustomOption(shellId - MAX_SHELL_ID - 1, pCustomMenu->contextMenuSelectedPaths);
			}


			return 0; // handled
		}
		break;
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
						std::vector<Box<IShellItem>> shellItems;
						ErrorPtr pError = pThis->getSelectedItems(shellItems);
						if ( (! pError) && (shellItems.size() == 1) )
						{
							// Get the name of the selected item
							PWSTR pszName = nullptr;
							HRESULT hr = shellItems[0]->GetDisplayName(SIGDN_NORMALDISPLAY, &pszName);
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
std::shared_ptr<ExplorerWrapper::CustomMenu> ExplorerWrapper::CreateCustomPopupMenu(MenuRequest menuRequest)
{
	std::shared_ptr<CustomMenu> pCustomMenu = std::make_shared<CustomMenu>();

	// Get the current folder view
	// The folder view has already been used in getSelectedItems()
	pCustomMenu->pCurrentFolderView = nullptr;
	HRESULT hr = p_peb->GetCurrentView(IID_PPV_ARGS(&pCustomMenu->pCurrentFolderView));
	if (! SUCCEEDED(hr))
	{
		qCritical() << "ExplorerWrapper::CreateCustomPopupMenu: GetCurrentView(), hr: " << hr;
		return nullptr;
	}

	// Get the current shell folder
	Box<IShellFolder> ppshf = nullptr;
	hr = pCustomMenu->pCurrentFolderView->GetFolder(IID_PPV_ARGS(&ppshf));
	if (! SUCCEEDED(hr))
	{
		qCritical() << "ExplorerWrapper::CreateCustomPopupMenu: GetFolder(), hr: " << hr;
		return nullptr;
	}

	// Get the current shell folder pidl
	PIDLIST_ABSOLUTE ppidl;
	hr = SHGetIDListFromObject(ppshf.get(), &ppidl);
	if (! SUCCEEDED(hr))
	{
		qCritical() << "ExplorerWrapper::CreateCustomPopupMenu: SHGetIDListFromObject(), hr: " << hr;
		return nullptr;
	}

	// Get the item(s) to process
	std::vector<Box<IShellItem>> shellItems;
	std::vector<PCITEMID_CHILD> ppidls;
	// Mode == Selected Items ?
	if (menuRequest == MenuRequest::SelectedItems)
	{
		// Get the selected shell items
		ErrorPtr pError = getSelectedItems(shellItems);
		if (pError)
		{
			qCritical() << "ExplorerWrapper::CreateCustomPopupMenu: getSelectedItems(), pError: " << *pError;
			return nullptr;
		}
		// No selected shell items ? then return
		if (shellItems.empty())
			return nullptr;

		// Get the path and the relative pidl of the selected items
		pCustomMenu->contextMenuSelectedPaths.resize(shellItems.size());
		ppidls.resize(shellItems.size());
		for (size_t i = 0; i < (shellItems.size()) && SUCCEEDED(hr); i++)
		{
			PIDLIST_ABSOLUTE ppidl_tmp;
			hr = SHGetIDListFromObject(shellItems[i].get(), &ppidl_tmp);
			pCustomMenu->contextMenuSelectedPaths[i] = NavigationPath(ppidl_tmp);
			ppidls[i] = ILFindLastID(ppidl_tmp);
		}
		if (! SUCCEEDED(hr))
		{
			qCritical() << "ExplorerWrapper::CreateCustomPopupMenu: SHGetIDListFromObject(), hr: " << hr;
			return nullptr;
		}
	}
	// Mode == Current Folder
	else
	{
		ppshf.clear();
		ppidls.resize(1);
		hr = SHBindToParent(ppidl, IID_PPV_ARGS(&ppshf), &ppidls[0]);
		if (! SUCCEEDED(hr))
		{
			qCritical() << "ExplorerWrapper::CreateCustomPopupMenu: SHBindToParent(), hr: " << hr;
			return nullptr;
		}
	}


//	// Create the context menu
//	DEFCONTEXTMENU defContextMenu;
//	defContextMenu.hwnd = nullptr;
//	defContextMenu.pcmcb = nullptr; // IContextMenuCB
//	defContextMenu.pidlFolder = ppidl;
//	defContextMenu.psf = ppshf.get();
//	defContextMenu.cidl = (UINT) ppidls.size();
//	defContextMenu.apidl = (PCUITEMID_CHILD_ARRAY) ppidls.data();
//	defContextMenu.punkAssociationInfo = nullptr;
//	defContextMenu.cKeys = 0;
//	defContextMenu.aKeys = nullptr;
//	hr = SHCreateDefaultContextMenu(&defContextMenu, IID_PPV_ARGS(&pCustomMenu->pContextMenu));
//	if (! SUCCEEDED(hr))
//	{
//		qCritical() << "ExplorerWrapper::CreateCustomPopupMenu: SHCreateDefaultContextMenu(), hr: " << hr;
//		return nullptr;
//	}

	//hr = ppshf->CreateViewObject(nullptr, IID_PPV_ARGS(&pCustomMenu->pContextMenu));
	hr = ppshf->GetUIObjectOf(nullptr, (UINT) ppidls.size(), (PCUITEMID_CHILD_ARRAY) ppidls.data(), IID_IContextMenu, nullptr, (void**)&pCustomMenu->pContextMenu);
	if (! SUCCEEDED(hr))
	{
		qCritical() << "ExplorerWrapper::CreateCustomPopupMenu: GetUIObjectOf(), hr: " << hr;
		return nullptr;
	}

	// Query the menu
	pCustomMenu->hmenu = CreatePopupMenu();
	hr = pCustomMenu->pContextMenu->QueryContextMenu(pCustomMenu->hmenu, 0, MIN_SHELL_ID, MAX_SHELL_ID, /*CMF_EXPLORE*/CMF_NORMAL | CMF_CANRENAME);
	if (! SUCCEEDED(hr))
	{
		qCritical() << "ExplorerWrapper::CreateCustomPopupMenu: QueryContextMenu(), hr: " << hr;
		return nullptr;
	}

	// Add custom options to the menu
	std::map<long, QString> mapReturn = getContextMenuCustomOptions(pCustomMenu->contextMenuSelectedPaths);
	if (! mapReturn.empty())
	{
		UINT position = 0;
	
		BOOL ok = TRUE;
		for (std::map<long, QString>::iterator p = mapReturn.begin(); p != mapReturn.end(); ++p)
		{
			long lOption = p->first;
			std::wstring option = p->second.toStdWString();
			ok = InsertMenu(pCustomMenu->hmenu, position++, MF_BYPOSITION | MF_STRING, (UINT_PTR) MAX_SHELL_ID + lOption +1, option.c_str());
		}
	
		ok = InsertMenu(pCustomMenu->hmenu, position++, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
	}
	// TODO
	//CoTaskMemFree(ppidl);
	//CoTaskMemFree(ppidls[i]);

	return pCustomMenu;
}
//
std::map<long, QString> ExplorerWrapper::getContextMenuCustomOptions(const std::vector<NavigationPath>& contextMenuSelectedPaths)
{
	std::map<long, QString> rslt;
	if (! contextMenuSelectedPaths.empty())
	{
		bool allFolders = true;
		for (const NavigationPath& path : contextMenuSelectedPaths)
			allFolders = allFolders && path.isExistingDirectory();

		if (allFolders)
			rslt[0] = tr("Open in a new tab");
	}
	return rslt;
}
//
void ExplorerWrapper::notifyContextMenuCustomOption([[maybe_unused]] int iOption, const std::vector<NavigationPath>& contextMenuSelectedPaths)
{
	assert(iOption == 0);
	if (! contextMenuSelectedPaths.empty())
	{
		bool allFolders = true;
		for (const NavigationPath& path : contextMenuSelectedPaths)
			allFolders = allFolders && path.isExistingDirectory();

		if (allFolders)
		{
			for (const NavigationPath& path : contextMenuSelectedPaths)
				emit openNewTab(path, NewTabPosition::AfterCurrent, NewTabBehaviour::None);
		}
	}
}
//
