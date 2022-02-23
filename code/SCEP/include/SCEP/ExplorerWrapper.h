#pragma once
//
#include <SCEP/Error.h>
//
#include <gears/base/ie/browser_listener.h>
//
#include <QObject>
//
class Theme;
//
class ExplorerWrapper : public QObject, public BrowserListener
{
	Q_OBJECT

public:
	ExplorerWrapper(QObject* pParent = nullptr);
	~ExplorerWrapper();

public:
	ErrorPtr initialize(Theme* ptr_theme, const QString& path = {});
	HWND hwnd() const;
	void setVisible(bool visible);
	ErrorPtr setCurrentPath(const QString& path);
	QString currentPath() const;
	ErrorPtr finalize();

signals:
	void loading(QString path);
	void pathChanged(QString path);
	void closed();

private:
	static ErrorPtr initializeExplorer(IWebBrowser2*& pWebBrower, HWND& wid);


	/////////////////////////////////////////////
	// BrowserListener method reimplementation //
	/////////////////////////////////////////////

	// Event handler methods.
	void OnBeforeNavigate2(IWebBrowser2 *window, const CString &url, bool *cancel) override;
	void OnDocumentComplete(IWebBrowser2 *window, const CString &url) override;
	void OnDownloadBegin() override;
	void OnDownloadComplete() override;
	void OnNavigateComplete2(IWebBrowser2 *window, const CString &url) override;
	void OnProgressChange(LONG progress, LONG progressMax) override;

	// Derivative events build on top of IE events.
	void OnPageDownloadBegin(const CString &url) override;
	void OnPageDownloadComplete() override;

private:
	IWebBrowser2* p_webBrowser = nullptr; //!< Web browser ? No, file explorer !
	DWORD m_pid = 0;
	HWND m_wid = 0;
};
//
