#pragma once
//
#include <SCEP/Error.h>
#include <SCEP/SCEP.h>
#include <SCEP/Navigation.h>
//
#include <QObject>
#include <QDateTime>
//
#include <optional>
//
#include <windows.h>
#include <shobjidl_core.h>
//
class Theme;
//
/**
 *	@ingroup				SCEP
 *	@brief					
 */
class ExplorerWrapper : public QObject, public IServiceProvider, public IExplorerBrowserEvents, public IExplorerPaneVisibility
{
	Q_OBJECT

public:
	ExplorerWrapper(Theme* ptrTheme, QObject* pParent = nullptr);
	~ExplorerWrapper();

public:
	ErrorPtr				initialize(const NavigationPath& path = {});
	HWND					hwnd() const;
	void					setVisible(bool visible);
	ErrorPtr				setCurrentPath(const NavigationPath& path);
	const NavigationPath&	currentPath() const;
	ErrorPtr				finalize();

	// IUnknown
	IFACEMETHODIMP			QueryInterface(REFIID riid, void **ppv) override;
	IFACEMETHODIMP_(ULONG)	AddRef() override;
	IFACEMETHODIMP_(ULONG)	Release() override;

	// IServiceProvider
	IFACEMETHODIMP			QueryService(REFGUID guidService, REFIID riid, void **ppv) override;

	// IExplorerBrowserEvents
	IFACEMETHODIMP			OnViewCreated(IShellView * psv) override;
	IFACEMETHODIMP			OnNavigationPending(PCIDLIST_ABSOLUTE pidlFolder) override;
	IFACEMETHODIMP			OnNavigationComplete(PCIDLIST_ABSOLUTE pidlFolder) override;
	IFACEMETHODIMP			OnNavigationFailed(PCIDLIST_ABSOLUTE pidlFolder) override;

	// IExplorerPaneVisibility
	HRESULT					GetPaneState(REFEXPLORERPANE ep, EXPLORERPANESTATE *peps);

signals:
	void					loading(const NavigationPath& path);
	void					pathChanged(const NavigationPath& path, bool success);
	void					openNewTab(const NavigationPath& path, NewTabPosition position, NewTabBehaviour behaviour);
	void					closed();

private:
	static INT_PTR CALLBACK	s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR					wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	ErrorPtr				onInitialize(const NavigationPath& path);
	ErrorPtr				getSelectedItems(std::vector<IShellItem*>& shellItems);

	static LRESULT CALLBACK	ShellWindowProcHook(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

	struct CustomMenu
	{
		HMENU						hmenu = nullptr;
		IContextMenu*				pContextMenu = nullptr;
		std::vector<NavigationPath>	contextMenuSelectedPaths;
		IFolderView2*				pCurrentFolderView = nullptr;
	};

	std::optional<CustomMenu> CreateCustomPopupMenu();
	std::map<long, QString>	getContextMenuCustomOptions(const std::vector<NavigationPath>& contextMenuSelectedPaths);
	void					notifyContextMenuCustomOption(int iOption, const std::vector<NavigationPath>& contextMenuSelectedPaths);

private:
	Theme*					ptr_theme = nullptr;

	long					m_cRef = 0;
	HWND					m_hwnd = 0;
	HRESULT					m_hrOleInit = 0;
	IExplorerBrowser*		p_peb = nullptr;
	bool					m_fPerformRenavigate = false;
	DWORD					m_dwCookie = 0;
	NavigationPath			m_currentPath;

	IShellView*				p_psv = nullptr;
	WNDPROC					p_shellWindowProcOld = nullptr;

	std::optional<QDateTime> m_middleClickDateTime;
};
//
