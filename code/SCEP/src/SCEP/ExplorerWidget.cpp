#include <SCEP/ExplorerWidget.h>
//
#include <Windows.h>
#include <psapi.h>
//
#include <QWindow>
#include <QVBoxLayout>
//
#include <iostream>
//
struct EnumWindowStruct
{
	// Input
	DWORD	pid = 0;
	QString	className = {};
	// Output
	HWND	hwnd = 0;
};

BOOL enumWindowsProc(HWND hwnd, LPARAM lParam)
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
		std::wstring wClassName = className;
		//std::wcerr << wClassName << std::endl;

		// Test correct class name
		if (wClassName == pParams->className.toStdWString())
		{
			// Stop enumerating
			SetLastError(-1);
			pParams->hwnd = hwnd;
			return FALSE;
		}
	}

	// Continue enumerating
	return TRUE;
}

HWND FindTopWindow(DWORD pid, const QString& className)
{
	EnumWindowStruct params = {pid, className, 0};

	BOOL bResult = EnumWindows(enumWindowsProc, (LPARAM)&params);

	if (!bResult && GetLastError() == -1 && params.hwnd)
	{
		return params.hwnd;
	}

	return 0;
}
//
bool ProcessHasName(DWORD processID, const QString& name)
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
		std::wstring processName = szProcessName;
		return processName == name.toStdWString();
	}
	else
	{
		return false;
	}
}

std::vector<DWORD> FindPIDs(const QString& name)
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
std::vector<DWORD> GetNewPIDS(const std::vector<DWORD>& oldPIDs, const std::vector<DWORD>& newPIDs)
{
	std::vector<DWORD> pids;
	for (DWORD pid : newPIDs)
	{
		if (std::find(oldPIDs.begin(), oldPIDs.end(), pid) == oldPIDs.end())
			pids.push_back(pid);
	}
	return pids;
}
//
LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
	{
	case WM_CLOSE:
		DestroyWindow(hwnd); break;
	case WM_DESTROY:
		PostQuitMessage(0); break;
	default:
		return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}
//
HWND CreateTheWindow(LPCWSTR WindowTitle)
{
	static LPCWSTR sClassName = L"MyClass";

	// Create & register the class
	WNDCLASSEX WndClass;
	WndClass.cbSize = sizeof(WNDCLASSEX);
	WndClass.style = 0u;
	WndClass.lpfnWndProc = WndProc; 
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.lpszClassName = sClassName;
	WndClass.hInstance = nullptr;
	WndClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION); 
	WndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	WndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	WndClass.lpszMenuName  = nullptr;
	WndClass.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
	RegisterClassEx(&WndClass);

	// Create & show the window
	HWND hwnd = CreateWindowEx(WS_EX_STATICEDGE, sClassName, WindowTitle, WS_OVERLAPPEDWINDOW, 
		CW_USEDEFAULT, CW_USEDEFAULT, 320, 240, nullptr, nullptr, nullptr, nullptr);

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);
	return hwnd;
}
//
//
//
struct ExplorerWidgetData
{
	STARTUPINFO				m_si;
	PROCESS_INFORMATION		m_pi;
	DWORD					m_pid = 0;
	QWidget*				p_widget = nullptr;
};
//
//
//
ExplorerWidget::ExplorerWidget(QWidget* pParent, Qt::WindowFlags f)
	:	QWidget(pParent, f)
{
	p_d = new ExplorerWidgetData();

	setMinimumWidth(200);
	setMinimumHeight(200);
}
//
ExplorerWidget::~ExplorerWidget()
{
	if (p_d->m_pid)
	{
		HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, p_d->m_pid);
		if (processHandle)
		{
			BOOL ok = TerminateProcess(processHandle, 0u);
			if (ok == FALSE)
			{
				std::cerr << "TerminateProcess failed (" << GetLastError() << ")" << std::endl;
			}
			CloseHandle(processHandle);
		}
	}

	delete p_d;
	p_d = nullptr;
}
//
ErrorPtr ExplorerWidget::init(const QString& path)
{
	static QString ExecutableName = "explorer.exe";//"explorer.exe";
	
	// Get the existing instances
	std::vector<DWORD> oldPIDs = FindPIDs(ExecutableName);
	
	// Initialize the process data
	ZeroMemory( &p_d->m_si, sizeof(p_d->m_si) );
	p_d->m_si.cb = sizeof(p_d->m_si);
	p_d->m_si.wShowWindow = SW_HIDE; // do not show explorer window for now !
	p_d->m_si.dwFlags = STARTF_USESHOWWINDOW;
	ZeroMemory( &p_d->m_pi, sizeof(p_d->m_pi) );
	
	QString qCommandLine = ExecutableName + " -Embedding";
	if (! path.isEmpty())
		qCommandLine += " " + path;
	
	std::wstring commandLine = qCommandLine.toStdWString();
	LPWSTR pCommandLine = commandLine.data();
	
	// Start the child process. 
	BOOL ok = CreateProcess(nullptr,		// No module name (use command line)
							pCommandLine,	// Command line
							nullptr,		// Process handle not inheritable
							nullptr,		// Thread handle not inheritable
							FALSE,			// Set handle inheritance to FALSE
							0,				// No creation flags
							nullptr,		// Use parent's environment block
							nullptr,		// Use parent's starting directory 
							&p_d->m_si,		// Pointer to STARTUPINFO structure
							&p_d->m_pi );	// Pointer to PROCESS_INFORMATION structure
	CHECK(ok, QString("CreateProcess failed (%1)").arg(GetLastError()) );

	// We do not need these handles any more, see following trick
	CloseHandle(p_d->m_pi.hThread);
	CloseHandle(p_d->m_pi.hProcess);
	
	// Trick : explorer.exe relaunches itself with new arguments
	// We need to :
	// - wait until it proceeds
	// - identify the new explorer.exe process
	// - ensure the new PID is not the one given by CreateProcess
	//   (i.e. explorer.exe had enough time to relaunch itself)
	oldPIDs.push_back(p_d->m_pi.dwProcessId);
	
	// Get the subprocess pid
	static constexpr unsigned int MaxIteration = 10u;
	static constexpr DWORD WaitDurationMs = 200;
	unsigned int i = 0;
	p_d->m_pid = 0;
	do
	{
		// Wait a bit
		Sleep(WaitDurationMs);
	
		// Get the new instances
		std::vector<DWORD> newPIDs = FindPIDs(ExecutableName);
		newPIDs = GetNewPIDS(oldPIDs, newPIDs);
		CHECK(newPIDs.size() <= 1, "Could not identify the right subprocess (1).");
	
		// One single new instance ?
		if (newPIDs.size() == 1)
		{
			p_d->m_pid = newPIDs[0];
			break;
		}

		// Next
		i++;
	}
	while ( (p_d->m_pid == 0) && (i < MaxIteration) );
	CHECK(p_d->m_pid, "Could not identify the right subprocess (2).");
	
	// Get the corresponding window
	HWND explorerWindowId = FindTopWindow(p_d->m_pid, "CabinetWClass");
	CHECK(explorerWindowId, "Could not get subprocess window id");

	// The trick !!!
	// - new win32 window (proxy) -> can embed a window from another process
	// - embedding Qt widget : can embed the proxy window because it belongs to the same process
	//                         while it can't directly embed the explorer window !

	// Create a new empty window
	HWND windowId = CreateTheWindow(L"Win32 SCEP");
	CHECK(windowId, "Could not create win32 window");

	// Change explorer window parent to the newly created window
	HWND embedId = explorerWindowId;
	HWND oldId = SetParent(embedId, windowId);
	CHECK(oldId, "Could not change explorer window parent");
	SetWindowLong(embedId, GWL_STYLE, WS_CHILDWINDOW | WS_VISIBLE  ); 
	UpdateWindow(embedId); 
	UpdateWindow(windowId);

	// Create the embedding Qt widget
	QWindow* pWindow = QWindow::fromWinId((WId) windowId);
	CHECK(pWindow, "Could not create QWindow !");
	p_d->p_widget = QWidget::createWindowContainer(pWindow);
	p_d->p_widget->setMinimumWidth(100);
	p_d->p_widget->setMinimumHeight(100);
	p_d->p_widget->setParent(this);
	QVBoxLayout* pLayout = new QVBoxLayout(this);
	pLayout->setContentsMargins(0, 0, 0, 0);
	pLayout->addWidget(p_d->p_widget);
	setLayout(pLayout);

	// Done !
	return success();
}
//
