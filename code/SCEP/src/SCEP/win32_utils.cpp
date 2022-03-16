#include <SCEP/win32_utils.h>
//
#include <QFileInfo>
#include <QtDebug>
//
#include <windows.h>
#include <shlobj_core.h>
#include <comdef.h>
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
								 nullptr, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, nullptr);

	// Copy the error message into a QString.
	QString message = QString::fromWCharArray(messageBuffer, (int) size);

	// Free the Win32's string's buffer.
	LocalFree(messageBuffer);

	return message;
}
//
QString GetErrorAsString(HRESULT hr)
{
	if (! SUCCEEDED(hr))
	{
		_com_error err(hr);
		if (const TCHAR* errorMsg = err.ErrorMessage())
			return QString::fromWCharArray(errorMsg);
		else
			return QString("Unknown error : hr = 0x%1").arg((qlonglong) hr, 0, 16);
	}
	else
		return {};
}
//
