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
#include <shobjidl_core.h>
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
	Box<IShellView>	p_sv = nullptr;
	Box<IFolderView2> p_fv = nullptr;
	HWND			m_sv_hwnd = 0;
	//WNDPROC			p_sv_wndproc_old = nullptr;

	Box<IShellFolderViewDual> p_fvd = nullptr;

	void			finalize()
	{
		m_sv_hwnd = 0;
		//p_sv_wndproc_old = nullptr;
	}

	~ShellView()
	{
		finalize();
	}
};
//
class SCEP_CORE_DLL ExplorerWrapper : public QObject, public BrowserListener
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

signals:
	void			loading(const NavigationPath& path);
	void			pathChanged(const NavigationPath& path);
	void			closed();
	void			openNewTab(const NavigationPath& path, NewTabPosition position, NewTabBehaviour behaviour);

private:
	// BrowserListener method reimplementation
	void			OnNavigateComplete2(IWebBrowser2 *window, const std::wstring &url) override;

	ErrorPtr		getSelectedItems(std::vector<Box<IShellItem>>& shellItems);

public:
//	void			processMiddleClickEvent();

	void			requestingFakeContextMenu();
	bool			contextMenuRequired();

private:
	size_t			m_navigationCount = 0;
	HWND			m_parentWnd = nullptr;
	mutable WebBrowser2 m_webBrowser;
	ShellView		m_shellView;
	HWND			m_wndproc_hwn = nullptr;

	std::optional<QDateTime> m_middleClickDateTime;
};
//
