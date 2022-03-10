#pragma once
//
#include <SCEP/Error.h>
#include <SCEP/SCEP.h>
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
class ExplorerWrapper2 : public QObject, public IServiceProvider, public IExplorerBrowserEvents
{
	Q_OBJECT

public:
	ExplorerWrapper2(Theme* ptrTheme, QObject* pParent = nullptr);
	~ExplorerWrapper2();

public:
	ErrorPtr				initialize(const QString& path = {});
	HWND					hwnd() const;
	void					setVisible(bool visible);
	ErrorPtr				setCurrentPath(const QString& path);
	QString					currentPath() const;
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
	void					loading(const QString& path);
	void					pathChanged(const QString& path, bool success, bool virtualFolder);
	void					openNewTab(const QString& path, NewTabPosition position, NewTabBehaviour behaviour);
	void					closed();

private:
	static INT_PTR CALLBACK	s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR					wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	ErrorPtr				onInitialize(const QString& path);
	ErrorPtr				getSelectedItem(REFIID riid, void **ppv);

	static LRESULT CALLBACK	ShellWindowProcHook(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
	HMENU					CreateCustomPopupMenu();
	std::map<long, QString>	getContextMenuCustomOptions(const QString& contextMenuFocusedPath);
	void					notifyContextMenuCustomOption(int iOption, const QString& contextMenuFocusedPath);

private:
	Theme*					ptr_theme = nullptr;

	long					m_cRef = 0;
	HWND					m_hwnd = 0;
	HRESULT					m_hrOleInit = 0;
	IExplorerBrowser*		p_peb = nullptr;
	bool					m_fPerformRenavigate = false;
	DWORD					m_dwCookie = 0;
	QString					m_currentPath;

	IShellView*				p_psv = nullptr;
	WNDPROC					p_shellWindowProcOld = nullptr;
	IContextMenu2*			p_contextMenu2 = nullptr;
	QString					m_contextMenuFocusedPath = nullptr;

	std::optional<QDateTime> m_middleClickDateTime;
};
//
