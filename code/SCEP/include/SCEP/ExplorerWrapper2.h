#pragma once
//
#include <SCEP/Error.h>
//
#include <QObject>
//
#include <windows.h>
#include <shobjidl_core.h>
//
class Theme;
//
class ExplorerWrapper2 : public QObject, public IServiceProvider, public IExplorerBrowserEvents
{
	Q_OBJECT

public:
	ExplorerWrapper2(QObject* pParent = nullptr);
	~ExplorerWrapper2();

public:
	ErrorPtr initialize(Theme* ptr_theme, const QString& path = {});
	HWND hwnd() const;
	void setVisible(bool visible);
	ErrorPtr setCurrentPath(const QString& path);
	QString currentPath() const;
	ErrorPtr finalize();

	// IUnknown
	IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv) override;
	IFACEMETHODIMP_(ULONG) AddRef() override;
	IFACEMETHODIMP_(ULONG) Release() override;

	// IServiceProvider
	IFACEMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv) override;

	// IExplorerBrowserEvents
	IFACEMETHODIMP OnViewCreated(IShellView * psv) override;
	IFACEMETHODIMP OnNavigationPending(PCIDLIST_ABSOLUTE pidlFolder) override;
	IFACEMETHODIMP OnNavigationComplete(PCIDLIST_ABSOLUTE pidlFolder) override;
	IFACEMETHODIMP OnNavigationFailed(PCIDLIST_ABSOLUTE pidlFolder) override;

signals:
	void loading(QString path);
	void pathChanged(QString path);
	void openNewTab(QString path);
	void closed();

private:
	static INT_PTR CALLBACK s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	ErrorPtr onInitialize(const QString& path);
	//ErrorPtr onOpenItem();
	//ErrorPtr onSelChange();
	//ErrorPtr getSelectedItem(REFIID riid, void **ppv);

	//ErrorPtr getItem(WORD x, WORD y, REFIID riid, void **ppv);

	static LRESULT CALLBACK ShellWindowProcHook(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
	HMENU CreateCustomPopupMenu();
	std::map<long, QString> getContextMenuCustomOptions(const QString& contextMenuFocusedPath);
	void notifyContextMenuCustomOption(int iOption, const QString& contextMenuFocusedPath);

private:
	long m_cRef = 0;
	HWND m_hwnd = 0;
	HRESULT m_hrOleInit = 0;
	IExplorerBrowser *p_peb = nullptr;
	bool m_fPerformRenavigate = false;
	DWORD m_dwCookie = 0;
	QString m_currentPath;

	IShellView *p_psv = nullptr;
	WNDPROC p_shellWindowProcOld = nullptr;
	IContextMenu2 *p_contextMenu2 = NULL;
	QString m_contextMenuFocusedPath = NULL;
};
//
