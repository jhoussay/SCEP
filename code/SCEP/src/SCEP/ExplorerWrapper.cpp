#include <SCEP/ExplorerWrapper.h>
#include <SCEP/win32_utils.h>
//
#include <QUrl>
#include <QFileInfo>
#include <QtDebug>
//
#include <propvarutil.h>
#include <psapi.h>
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
	CHECK(p_webBrowser == nullptr, "Already initialized explorer wrapper.");

	// Start the explorer
	CALL( initializeExplorer(p_webBrowser, m_wid) );

	// Hook on the launched explorer
	BrowserListener::Init(p_webBrowser);

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
void ExplorerWrapper::setVisible(bool visible)
{
	if (p_webBrowser)
	{
		p_webBrowser->put_Visible(visible ? VARIANT_TRUE : VARIANT_FALSE);
	}
}
//
ErrorPtr ExplorerWrapper::setCurrentPath(const QString& path)
{
	CHECK(p_webBrowser, "ExplorerWrapper::setCurrentPath() : No current instance.");

	QFileInfo pathInfo(path);
	CHECK(pathInfo.exists() && pathInfo.isDir(), "Invalid path");
	
	VARIANT vEmpty = {0};
	VARIANT varTarget = {0};

	std::wstring wpath = path.toStdWString();
	HRESULT hr = InitVariantFromString(wpath.c_str(), &varTarget);
	CHECK(SUCCEEDED(hr), "Could not set initial folder for explorer window.")
	
	hr = p_webBrowser->Navigate2(&varTarget, &vEmpty, &vEmpty, &vEmpty, &vEmpty);
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
	if (p_webBrowser)
	{
		BSTR url;
		HRESULT hr = p_webBrowser->get_LocationURL(&url);
		if (SUCCEEDED(hr))
		{
			QUrl qUrl( QString::fromWCharArray(url) );
			SysFreeString(url);

			return qUrl.toLocalFile();
		}
		else
		{
			qWarning() << "ExplorerWrapper::currentPath() : Could not get current location.";
			return {};
		}
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

	if (p_webBrowser)
	{
		p_webBrowser->Quit();
		//p_webBrowser->Release();
		p_webBrowser = nullptr;
	}

	m_wid = 0;

	return success();
}
//
ErrorPtr ExplorerWrapper::initializeExplorer(IWebBrowser2*& pWebBrower, HWND& wid)
{
	HRESULT hr = CoCreateInstance(	CLSID_ShellBrowserWindow,
									NULL,
									CLSCTX_LOCAL_SERVER, 
									IID_PPV_ARGS(&pWebBrower)	);
	CHECK(SUCCEEDED(hr), "Unable to create explorer window : " + GetLastErrorAsString());

	// Paramétrage fenêtre
	//////////////////////

//	Sleep(200);
//
//	hr = CoAllowSetForegroundWindow(pWebBrower, 0);
//	CHECK(SUCCEEDED(hr), "Unable to allow foreground explorer window : " + GetLastErrorAsString());

	// pwb->put_Left(100);
	// pwb->put_Top(100);
	// pwb->put_Height(600);
	// pwb->put_Width(800);

	// https://www.codeproject.com/Articles/12029/Automate-the-Active-Windows-Explorer-or-Internet-E

	//	hr = m_pWebBrowser2->put_StatusBar(VARIANT_TRUE);
	//	hr = m_pWebBrowser2->put_ToolBar(VARIANT_TRUE);
	//	hr = m_pWebBrowser2->put_MenuBar(VARIANT_TRUE);
	//	hr = m_pWebBrowser2->put_Visible(VARIANT_TRUE);


	wid = 0;
	hr = pWebBrower->get_HWND((SHANDLE_PTR*) &wid);
	CHECK(SUCCEEDED(hr), "Could not get explorer window handle : " + GetLastErrorAsString());

	return success();
}
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
	if (window == p_webBrowser)
	{
		qDebug() << "[ExplorerWrapper::OnBeforeNavigate2] : " << toQString(url);
	}
}
//
void ExplorerWrapper::OnDocumentComplete(IWebBrowser2 *window, const CString &url)
{
	if (window == p_webBrowser)
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
	if (window == p_webBrowser)
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
