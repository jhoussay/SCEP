#include <SCEP/SCEP.h>
#include <SCEP/MainWindow.h>
#include <SCEP/Theme.h>
//
#include <QApplication>
#include <QSettings>
//
static const QString OrganizationStr = "SCEP";
static const QString ApplicationStr = "SCEP";
//
// DEBUG
#include <propvarutil.h>
#include <psapi.h>
#include <exdisp.h>
#include <SCEP/ExplorerWidget.h>
#include <SCEP_CORE/ExplorerWrapper.h>
//
// 
//
static HWND CreateTheWindow(LPCWSTR WindowTitle)
{
	static LPCWSTR sClassName = L"ScepTestClass";

	// Create & register the class
	WNDCLASSEX WndClass;
	WndClass.cbSize = sizeof(WNDCLASSEX);
	WndClass.style = 0u;
	WndClass.lpfnWndProc = DefWindowProc;
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
	HWND hwnd = CreateWindowEx(	WS_EX_STATICEDGE,
								sClassName,
								WindowTitle,
								WS_OVERLAPPEDWINDOW,
								CW_USEDEFAULT,
								CW_USEDEFAULT,
								320,
								240,
								nullptr,
								nullptr,
								nullptr,
								nullptr);
	return hwnd;
}
//
int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	int rslt = 0;

	// DEBUG
	//IWebBrowser2* pWebBrower = nullptr;
	//HRESULT hr = CoCreateInstance(	CLSID_ShellBrowserWindow,
	//								NULL,
	//								CLSCTX_LOCAL_SERVER, 
	//								IID_PPV_ARGS(&pWebBrower)	);
	//pWebBrower->put_Visible(VARIANT_TRUE);

	{
		QSettings settings(QSettings::IniFormat, QSettings::UserScope, OrganizationStr, "SCEP");
		Theme theme(&settings);
	
		////HWND hwnd = nullptr;
		//HWND hwnd = CreateTheWindow(L"Test");
		//ShowWindow(hwnd, SW_SHOW);
		//ExplorerWrapper wrapper;
		//wrapper.initialize(hwnd, &theme, NavigationPath("C:\\Temp"));
		//wrapper.setVisible(true);
	
		//ExplorerWidget widget(&theme);
		//widget.init();
		//widget.setVisible(true);
	
		MainWindow mainWindow(&theme, &settings);
		mainWindow.show();
	
		rslt = app.exec();
	}
	
	return rslt;
}
