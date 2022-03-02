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
//
/**
 *	@ingroup				SCEP
 *	@brief					Returns the "translated" leaf of a path
 *	@see					https://stackoverflow.com/a/29198314
 */
QString						getPathLabel(const QString& path);
