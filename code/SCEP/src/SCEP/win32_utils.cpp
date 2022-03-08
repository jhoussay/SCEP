#include <SCEP/win32_utils.h>
//
#include <QFileInfo>
#include <QtDebug>
//
#include <windows.h>
#include <shlobj_core.h>
//
QString GetLastErrorAsString()
{
	// Get the error message ID, if any.
	DWORD errorMessageID = GetLastError();
	if (errorMessageID == 0)
		return {};

	LPWSTR messageBuffer = nullptr;

	// Ask Win32 to give us the string version of that message ID.
	// The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
	size_t size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
								 NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, NULL);

	// Copy the error message into a QString.
	QString message = QString::fromWCharArray(messageBuffer, (int) size);

	// Free the Win32's string's buffer.
	LocalFree(messageBuffer);

	return message;
}
//
QString getPathLabel(const QString& path)
{
	QString nativePath = QFileInfo(path).absoluteFilePath().replace("/", "\\");
	std::wstring wpath = nativePath.toStdWString();

	ITEMIDLIST* idlist;
	HRESULT hr = SHParseDisplayName(wpath.c_str(), 0, &idlist, 0, 0);
	if (! SUCCEEDED(hr))
	{
		QString label = QFileInfo(path).fileName();
		qDebug() << "Could not get path label for " << path << ", defaulting to file name : " << label;
		return label;
	}

	PWSTR name_ptr;
	hr = SHGetNameFromIDList(idlist, SIGDN_PARENTRELATIVEFORADDRESSBAR, //PARENTRELATIVEEDITING,
							&name_ptr);
	
	QString name = name_ptr && SUCCEEDED(hr) ? QString::fromWCharArray(name_ptr) : QString();

	CoTaskMemFree(idlist);
	CoTaskMemFree(name_ptr);

	return name;
}
//
