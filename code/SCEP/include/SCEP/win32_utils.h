#pragma once
//
#include <QString>
//
/**
 *	@ingroup				SCEP
 *	@brief					Returns the last Win32 error, in string format.
 *	@return					Returns an empty string if there is no error.
 */
QString						GetLastErrorAsString();
