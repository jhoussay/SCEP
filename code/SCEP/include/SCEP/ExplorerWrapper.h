#pragma once
//
#include <SCEP/Error.h>
//
#include <gears/base/ie/browser_listener.h>
//
#include <QObject>
//
class ExplorerWrapper : public QObject, public BrowserListener
{
	Q_OBJECT

public:
	ExplorerWrapper(QObject* pParent = nullptr);
	~ExplorerWrapper();

public:
	ErrorPtr initialize(const QString& path = {});
	HWND hwnd() const;
	ErrorPtr setCurrentPath(const QString& path);
	QString currentPath() const;
	ErrorPtr finalize();

signals:
	void pathChanged(QString path);
	void closed();

private:

	struct Instance
	{
		DWORD pid;
		std::vector<HWND> wids;
	};

	static ErrorPtr initializeExplorer(IWebBrowser2*& pWebBrower);
	static std::vector<DWORD> findPIDs(const QString& name);
	static ErrorPtr getExplorerInstances(std::vector<Instance>& instances);
	static ErrorPtr getExplorerWindows(DWORD pid, std::vector<HWND>& wids);
	static ErrorPtr getNewExplorerWindow(const std::vector<Instance>& instances, IWebBrowser2* pWebBrower, DWORD& pid, HWND& wid);


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
	IWebBrowser2* p_webBrower = nullptr; //!< Web browser ? No, file explorer !
	DWORD m_pid = 0;
	HWND m_wid = 0;
};