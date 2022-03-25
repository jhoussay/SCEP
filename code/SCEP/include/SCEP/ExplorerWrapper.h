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
class ExplorerWrapper : public QObject, public IServiceProvider, public IExplorerBrowserEvents
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

signals:
	void					loading(const NavigationPath& path);
	void					pathChanged(const NavigationPath& path, bool success);
	void					openNewTab(const NavigationPath& path, NewTabPosition position, NewTabBehaviour behaviour);
	void					closed();

private:
	static INT_PTR CALLBACK	s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR					wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	ErrorPtr				onInitialize(const NavigationPath& path);
	ErrorPtr				getSelectedItem(REFIID riid, void **ppv);

	static LRESULT CALLBACK	ShellWindowProcHook(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
	HMENU					CreateCustomPopupMenu();
	std::map<long, QString>	getContextMenuCustomOptions(const NavigationPath& contextMenuFocusedPath);
	void					notifyContextMenuCustomOption(int iOption, const NavigationPath& contextMenuFocusedPath);

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
	IContextMenu2*			p_contextMenu2 = nullptr;
	NavigationPath			m_contextMenuFocusedPath;

	std::optional<QDateTime> m_middleClickDateTime;
};
//
