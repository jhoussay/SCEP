#pragma once
//
#include <SCEP/Error.h>
#include <SCEP/SCEP.h>
#include <SCEP/Navigation.h>
#include <SCEP/win32_utils.h>
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
class ExplorerWrapper : public QObject, public IServiceProvider, public IExplorerBrowserEvents//, public IExplorerPaneVisibility
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
//	HRESULT					GetPaneState(REFEXPLORERPANE ep, EXPLORERPANESTATE *peps) override;

public slots:
	void					rename();
	void					copy();
	void					cut();
	void					paste();
	void					del();
	void					forceDel();
	void					selectAll();
	void					mkDir();
	void					undo();
	void					redo();
	void					refresh();

signals:
	void					loading(const NavigationPath& path);
	void					pathChanged(const NavigationPath& path, bool success);
	void					openNewTab(const NavigationPath& path, NewTabPosition position, NewTabBehaviour behaviour);

private:
	static INT_PTR CALLBACK	s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR					wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	ErrorPtr				onInitialize(const NavigationPath& path);
	ErrorPtr				getSelectedItems(std::vector<Box<IShellItem>>& shellItems);

	static LRESULT CALLBACK	ShellWindowProcHook(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

	struct CustomMenu
	{
		HMENU						hmenu = nullptr;
		Box<IContextMenu>			pContextMenu = nullptr;
		std::vector<NavigationPath>	contextMenuSelectedPaths;
		Box<IFolderView2>			pCurrentFolderView = nullptr;

		~CustomMenu()
		{
			if (hmenu)
			{
				DestroyMenu(hmenu);
			}
		}
	};

	enum class MenuRequest
	{
		SelectedItems,
		CurrentFolder
	};

	std::shared_ptr<CustomMenu> CreateCustomPopupMenu(MenuRequest menuRequest);
	std::map<long, QString>	getContextMenuCustomOptions(const std::vector<NavigationPath>& contextMenuSelectedPaths);
	void					notifyContextMenuCustomOption(int iOption, const std::vector<NavigationPath>& contextMenuSelectedPaths);

	bool					hasFocus() const;

private:
	Theme*					ptr_theme = nullptr;

	long					m_cRef = 0;
	HWND					m_hwnd = 0;
	HRESULT					m_hrOleInit = 0;
	Box<IExplorerBrowser>	p_peb = nullptr;
	bool					m_fPerformRenavigate = false;
	DWORD					m_dwCookie = 0;
	NavigationPath			m_currentPath;

	IShellView*				p_psv = nullptr;
	HWND					m_hwnd_sv = 0;
	WNDPROC					p_shellWindowProcOld = nullptr;

	std::optional<QDateTime> m_middleClickDateTime;
};
//
