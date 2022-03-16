#pragma once
//
#include <QString>
//
typedef long HRESULT;
//
/**
 *	@ingroup				SCEP
 *	@brief					Returns the last Win32 error, in string format.
 *	@return					Returns an empty string if there is no error.
 */
QString						GetLastErrorAsString();
/**
 *	@ingroup				SCEP
 *	@brief					Returns the HRESULT error, in string format.
 *	@return					Returns an empty string if there is no error.
 */
QString						GetErrorAsString(HRESULT hr);
