#pragma once
//
#include <SCEP_CORE/SCEP_CORE.h>
#include <SCEP_CORE/Error.h>
#include <SCEP_CORE/BrowserListener.h>
#include <SCEP_CORE/Navigation.h>
#include <SCEP_CORE/win32_utils.h>
//
#include <QObject>
#include <QDateTime>
//
#include <windows.h>
#include <shobjidl_core.h>
#include <shobjidl.h>
#include <shlwapi.h>
//
#include <optional>
//
class Theme;
//
struct SCEP_CORE_DLL WebBrowser2
{
	Box<IWebBrowser2> p_wb = nullptr;
	HWND			m_wb_hwnd = 0;

	void			finalize()
	{
		if (p_wb)
			p_wb->Quit();
		p_wb.clear();
		m_wb_hwnd = 0;
	}

	~WebBrowser2()
	{
		finalize();
	}
};
//
struct SCEP_CORE_DLL ShellView
{
	Box<IShellBrowser> p_sb = nullptr;
	Box<IShellView>	p_sv = nullptr;
	Box<IFolderView2> p_fv = nullptr;
//	DWORD m_nstcCookie = 0;
//	Box<INameSpaceTreeControl> p_nstc = nullptr;
	HWND			m_sv_hwnd = 0;
	//WNDPROC			p_sv_wndproc_old = nullptr;

	Box<IShellFolderViewDual> p_fvd = nullptr;

	void			finalize()
	{
		m_sv_hwnd = 0;
		//p_sv_wndproc_old = nullptr;

//		if (m_nstcCookie)
//		{
//			IUnknown_SetSite(p_nstc.get(), nullptr);
//			p_nstc->TreeUnadvise(m_nstcCookie);
//		}
	}

	~ShellView()
	{
		finalize();
	}
};
//
struct SCEP_CORE_DLL TreeControl
{
	Box<INameSpaceTreeControl> p_nstc = nullptr;
	HWND			m_nstc_hwnd = 0;

	void			finalize()
	{
		m_nstc_hwnd = 0;
	}

	~TreeControl()
	{
		finalize();
	}
};
//
class SCEP_CORE_DLL ExplorerWrapper : public QObject, public BrowserListener//, public IServiceProvider, public IExplorerBrowserEvents, public INameSpaceTreeControlEvents, public IExplorerPaneVisibility
{
	Q_OBJECT

public:
	ExplorerWrapper(QObject* pParent = nullptr);
	~ExplorerWrapper();

public:
	ErrorPtr		initialize(/*HWND parentWnd, */Theme* ptr_theme, const NavigationPath& path = {});
	HWND			hwnd() const;
	void			setVisible(bool visible);
	ErrorPtr		updateWindow(int width, int height);
	ErrorPtr		setCurrentPath(const NavigationPath& path);
	NavigationPath	currentPath() const;
	ErrorPtr		finalize();

//	// IUnknown
//	IFACEMETHODIMP			QueryInterface(REFIID riid, void **ppv) override;
//	IFACEMETHODIMP_(ULONG)	AddRef() override;
//	IFACEMETHODIMP_(ULONG)	Release() override;
//
//	// IServiceProvider
//	IFACEMETHODIMP			QueryService(REFGUID guidService, REFIID riid, void **ppv) override;
//
//	// IExplorerBrowserEvents
//	IFACEMETHODIMP			OnViewCreated(IShellView * psv) override;
//	IFACEMETHODIMP			OnNavigationPending(PCIDLIST_ABSOLUTE pidlFolder) override;
//	IFACEMETHODIMP			OnNavigationComplete(PCIDLIST_ABSOLUTE pidlFolder) override;
//	IFACEMETHODIMP			OnNavigationFailed(PCIDLIST_ABSOLUTE pidlFolder) override;
//
//	// INameSpaceTreeControlEvents
//	IFACEMETHODIMP			OnItemClick(IShellItem *psi, NSTCEHITTEST nstceHitTest, NSTCECLICKTYPE nstceClickType) override;
//	IFACEMETHODIMP			OnPropertyItemCommit(IShellItem *psi) override;
//	IFACEMETHODIMP			OnItemStateChanging(IShellItem *psi, NSTCITEMSTATE nstcisMask, NSTCITEMSTATE nstcisState) override;
//	IFACEMETHODIMP			OnItemStateChanged(IShellItem *psi, NSTCITEMSTATE nstcisMask, NSTCITEMSTATE nstcisState) override;
//	IFACEMETHODIMP			OnSelectionChanged(IShellItemArray *psiaSelection) override;
//	IFACEMETHODIMP			OnKeyboardInput(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
//	IFACEMETHODIMP			OnBeforeExpand(IShellItem *psi) override;
//	IFACEMETHODIMP			OnAfterExpand(IShellItem *psi) override;
//	IFACEMETHODIMP			OnBeginLabelEdit(IShellItem *psi) override;
//	IFACEMETHODIMP			OnEndLabelEdit(IShellItem *psi) override;
//	IFACEMETHODIMP			OnGetToolTip(IShellItem *psi, LPWSTR pszTip, int cchTip) override;
//	IFACEMETHODIMP			OnBeforeItemDelete(IShellItem *psi) override;
//	IFACEMETHODIMP			OnItemAdded(IShellItem *psi, BOOL fIsRoot) override;
//	IFACEMETHODIMP			OnItemDeleted(IShellItem *psi, BOOL fIsRoot) override;
//	IFACEMETHODIMP			OnBeforeContextMenu(IShellItem *psi, REFIID riid, void **ppv) override;
//	IFACEMETHODIMP			OnAfterContextMenu(IShellItem *psi, IContextMenu *pcmIn, REFIID riid, void **ppv) override;
//	IFACEMETHODIMP			OnBeforeStateImageChange(IShellItem *psi) override;
//	IFACEMETHODIMP			OnGetDefaultIconIndex(IShellItem *psi, int *piDefaultIcon, int *piOpenIcon) override;
//
//	// IExplorerPaneVisibility
//	HRESULT					GetPaneState(REFEXPLORERPANE ep, EXPLORERPANESTATE *peps) override;

signals:
	void			loading(const NavigationPath& path);
	void			pathChanged(const NavigationPath& path);
	void			closed();
	void			openNewTab(const NavigationPath& path, NewTabPosition position, NewTabBehaviour behaviour);

private:
	// BrowserListener method reimplementation
	void			OnNavigateComplete2(IWebBrowser2 *window, const std::wstring &url) override;
	void			OnSelectionChanged() override;

	/**
	 *	@brief		Explorer Pane
	 */
	enum class Pane
	{
		ShellView,	//!< Shell view (right part)
		TreeView	//!< Tree view (left part)
	};

	ErrorPtr		getSelectedItems(std::vector<NavigationPath>& items, Pane pane);

public:
//	void			processMiddleClickEvent();

	void			onMiddleClick(HWND hwnd, const POINT& windowPos, const POINT& screePos);

//	void			requestingFakeContextMenu();
//	bool			contextMenuRequired();

private:
	long			m_cRef = 0;

	size_t			m_navigationCount = 0;
	HWND			m_parentWnd = nullptr;
	mutable WebBrowser2 m_webBrowser;
	ShellView		m_shellView;
	TreeControl		m_treeControl;
//	HWND			m_wndproc_hwn = nullptr;

	std::optional<QDateTime> m_middleClickDateTime;
};
//
