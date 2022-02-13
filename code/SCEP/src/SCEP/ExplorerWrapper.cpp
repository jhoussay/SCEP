#include <SCEP/ExplorerWrapper.h>
//
#include <QUrl>
#include <QFileInfo>
#include <QtDebug>
//
#include <propvarutil.h>
#include <psapi.h>
//
static QString ExecutableName = "explorer.exe";
//
//
//
ExplorerWrapper::ExplorerWrapper(QObject* pParent)
	:	QObject(pParent)
	,	BrowserListener()
{}
//
ExplorerWrapper::~ExplorerWrapper()
{
	finalize();
}
//
ErrorPtr ExplorerWrapper::initialize(const QString& path)
{
	// Can we start ?
	CHECK(p_webBrower == nullptr && m_pid == 0, "Already initialized explorer wrapper.");

	// Before we start
	// What are the existing explorer instances ?
	std::vector<Instance> instances;
	CALL( getExplorerInstances(instances) );

	// Start the explorer
	//CALL( startExplorer(path) );
	CALL( initializeExplorer(p_webBrower) );

	// Hook on the launched explorer
	BrowserListener::Init(p_webBrower);

	// Get window handle
	CALL( getNewExplorerWindow(instances, p_webBrower, m_pid, m_wid) );

	// And set the current directory
	if (! path.isEmpty())
	{
		CALL( setCurrentPath(path) );
	}

	return success();
}
//
HWND ExplorerWrapper::hwnd() const
{
	return m_wid;
}
//
ErrorPtr ExplorerWrapper::setCurrentPath(const QString& path)
{
	CHECK(p_webBrower, "ExplorerWrapper::setCurrentPath() : No current instance.");

	QFileInfo pathInfo(path);
	CHECK(pathInfo.exists() && pathInfo.isDir(), "Invalid path");
	
	VARIANT vEmpty = {0};
	VARIANT varTarget = {0};

	std::wstring wpath = path.toStdWString();
	HRESULT hr = InitVariantFromString(wpath.c_str(), &varTarget);
	CHECK(SUCCEEDED(hr), "Could not set initial folder for explorer window.")
	
	hr = p_webBrower->Navigate2(&varTarget, &vEmpty, &vEmpty, &vEmpty, &vEmpty);
	CHECK(SUCCEEDED(hr), "Unable to set initial folder for explorer window.");

	hr = VariantClear(&varTarget);
	if (! SUCCEEDED(hr))
	{
		qWarning() << "Could not set clear initial folder variant.";
	}

	return success();
}
//
QString ExplorerWrapper::currentPath() const
{
	if (p_webBrower)
	{
		BSTR url;
		HRESULT hr = p_webBrower->get_LocationURL(&url);

		QUrl qUrl( QString::fromWCharArray(url) );
		SysFreeString(url);

		return qUrl.toLocalFile();
	}
	else
	{
		qWarning() << "ExplorerWrapper::currentPath() : No current instance.";
		return {};
	}
}
//
ErrorPtr ExplorerWrapper::finalize()
{
	Teardown();

	if (p_webBrower)
	{
		p_webBrower->Quit();
		//p_webBrower->Release();
		p_webBrower = nullptr;
	}

	m_pid = 0;
	m_wid = 0;

	return success();
}
//
ErrorPtr ExplorerWrapper::initializeExplorer(IWebBrowser2*& pWebBrower)
{
	HRESULT hr = CoCreateInstance(	CLSID_ShellBrowserWindow,
									NULL,
									CLSCTX_LOCAL_SERVER, 
									IID_PPV_ARGS(&pWebBrower)	);
	CHECK(SUCCEEDED(hr), "Unable to create explorer window.");


	// Paramétrage fenêtre
	//////////////////////

	hr = CoAllowSetForegroundWindow(pWebBrower, 0);
	CHECK(SUCCEEDED(hr), "Unable to allow foreground explorer window.");

	// pwb->put_Left(100);
	// pwb->put_Top(100);
	// pwb->put_Height(600);
	// pwb->put_Width(800);

	// https://www.codeproject.com/Articles/12029/Automate-the-Active-Windows-Explorer-or-Internet-E

	//	hr = m_pWebBrowser2->put_StatusBar(VARIANT_TRUE);
	//	hr = m_pWebBrowser2->put_ToolBar(VARIANT_TRUE);
	//	hr = m_pWebBrowser2->put_MenuBar(VARIANT_TRUE);
	//	hr = m_pWebBrowser2->put_Visible(VARIANT_TRUE);

	return success();
}
//
struct EnumWindowStruct
{
	// Input
	DWORD	pid = 0;
	QString	className = {};
	// Output
	std::vector<HWND> hwnds = {};
};
//
static BOOL enumWindowsProc(HWND hwnd, LPARAM lParam)
{
	EnumWindowStruct* pParams = (EnumWindowStruct*) lParam;

	// Get process and thread ids
	DWORD processId;
	DWORD threadId = GetWindowThreadProcessId(hwnd, &processId);

	// Test correct process id
	if (threadId && processId == pParams->pid)
	{
		// Get class name
		static constexpr unsigned int Size = 1024u;
		static WCHAR className[Size];
		[[maybe_unused]] int size = GetClassName(hwnd, className, Size);
		QString qClassName = QString::fromWCharArray(className);
		//std::wcerr << wClassName << std::endl;

		// Test correct class name
		if (qClassName == pParams->className)
		{
			pParams->hwnds.push_back(hwnd);
		}
	}

	// Continue enumerating
	return TRUE;
}
//
static std::vector<HWND> FindTopWindows(DWORD pid, const QString& className)
{
	EnumWindowStruct params = {pid, className, {}};

	[[maybe_unused]] BOOL bResult = EnumWindows(enumWindowsProc, (LPARAM)&params);

	return params.hwnds;
}
//
static bool ProcessHasName(DWORD processID, const QString& name)
{
	TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

	// Get a handle to the process
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
	if (hProcess)
	{
		// Get the process name.
		HMODULE hMod;
		DWORD cbNeeded;
		if (EnumProcessModules( hProcess, &hMod, sizeof(hMod), &cbNeeded))
		{
			GetModuleBaseName( hProcess, hMod, szProcessName, sizeof(szProcessName)/sizeof(TCHAR) );
		}

		// Release the handle to the process.
		CloseHandle(hProcess);

		// Compare the names.
		QString processName = QString::fromWCharArray(szProcessName);
		return processName == name;
	}
	else
	{
		return false;
	}
}
//
std::vector<DWORD> ExplorerWrapper::findPIDs(const QString& name)
{
	// Get the list of process identifiers.
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
	{
		return {};
	}

	// Calculate how many process identifiers were returned.
	cProcesses = cbNeeded / sizeof(DWORD);

	// Keep the PIDs of the processes with the desired name
	std::vector<DWORD> pids;
	for (DWORD i = 0; i < cProcesses; i++ )
	{
		if (aProcesses[i] != 0)
		{
			if (ProcessHasName(aProcesses[i], name))
			{
				pids.push_back(aProcesses[i]);
			}
		}
	}
	return pids;
}
//
template <typename T>
static std::vector<T> GetNew(const std::vector<T>& olds, const std::vector<T>& news)
{
	std::vector<T> rslt;
	for (const T& t : news)
	{
		if (std::find(olds.begin(), olds.end(), t) == olds.end())
			rslt.push_back(t);
	}
	return rslt;
}
//
ErrorPtr ExplorerWrapper::getExplorerInstances(std::vector<Instance>& instances)
{
	std::vector<DWORD> pids = findPIDs(ExecutableName);
	instances.resize(pids.size());
	for (size_t i = 0; i < pids.size(); i++)
	{
		Instance& instance = instances[i];

		instance.pid = pids[i];
		CALL( getExplorerWindows(instance.pid, instance.wids) );
	}

	return success();
}
//
ErrorPtr ExplorerWrapper::getExplorerWindows(DWORD pid, std::vector<HWND>& wids)
{
	//Sleep(WaitDurationMs);
	wids = FindTopWindows(pid, "CabinetWClass");
	return success();
}
//
ErrorPtr ExplorerWrapper::getNewExplorerWindow(const std::vector<Instance>& instances, IWebBrowser2* pWebBrower, DWORD& pid, HWND& wid)
{
	// A handle to a window belonging to the explorer
	// This handle permits to identify the explorer process
	// but does not correspond to the window we are looking after
	HWND hwnd = 0;
	HRESULT hr = pWebBrower->get_HWND((SHANDLE_PTR*) &hwnd);
	CHECK(SUCCEEDED(hr), "error");

	// Get the explorer pid
	[[maybe_unused]] DWORD tid = GetWindowThreadProcessId(hwnd, &pid);

	// Get the new explorer windows
	std::vector<HWND> newWids;
	CALL( getExplorerWindows(pid, newWids) );

	// Get the old explorer windows
	std::vector<HWND> oldWids;
	auto instanceIte = std::find_if(instances.begin(), instances.end(), [pid](const Instance& instance) {return pid == instance.pid;});
	if (instanceIte != instances.end())
	{
		oldWids = instanceIte->wids;
	}

	// Identify the new window
	std::vector<HWND> diffWids = GetNew(oldWids, newWids);
	CHECK(diffWids.size() == 1, "Could not identify the right window.");
	wid = diffWids[0];

	return success();
}
//
//
/////////////////////////////////////////////
// BrowserListener method reimplementation //
/////////////////////////////////////////////
//
inline QString toQString(const CString& cstring)
{
	return QString::fromWCharArray( cstring.GetString() );
}
//
void ExplorerWrapper::OnBeforeNavigate2(IWebBrowser2 *window, const CString &url, bool * /*cancel*/)
{
	if (window == p_webBrower)
	{
		qDebug() << "[ExplorerWrapper::OnBeforeNavigate2] : " << toQString(url);
	}
}
//
void ExplorerWrapper::OnDocumentComplete(IWebBrowser2 *window, const CString &url)
{
	if (window == p_webBrower)
	{
		qDebug() << "[ExplorerWrapper::OnDocumentComplete] : " << toQString(url);
	}
}
//
void ExplorerWrapper::OnDownloadBegin()
{
	qDebug() << "[ExplorerWrapper::OnDownloadBegin]";
}
//
void ExplorerWrapper::OnDownloadComplete()
{
	qDebug() << "[ExplorerWrapper::OnDownloadComplete]";
}
//
void ExplorerWrapper::OnNavigateComplete2(IWebBrowser2 *window, const CString &url)
{
	if (window == p_webBrower)
	{
		qDebug() << "[ExplorerWrapper::OnNavigateComplete2] : " << toQString(url) << " --> emit pathChanged()";

		emit pathChanged( toQString(url) );
	}
}
//
void ExplorerWrapper::OnProgressChange(LONG progress, LONG progressMax)
{
	qDebug() << "[ExplorerWrapper::OnProgressChange] " << progress << "/" << progressMax;
}
//
void ExplorerWrapper::OnPageDownloadBegin(const CString &url)
{
	qDebug() << "[ExplorerWrapper::OnPageDownloadBegin] : " << toQString(url);
}
//
void ExplorerWrapper::OnPageDownloadComplete()
{
	qDebug() << "[ExplorerWrapper::OnDownloadComplete]";
}
//
