#pragma once

/**
 *	@defgroup		SCEP SCEP
 */

/**
 *	@ingroup		SCEP
 *	@brief			Enumeration on the requested position of a newly created tab
 */
enum class NewTabPosition
{
	AfterCurrent,	//!< Add the new tab just after the current one
	Last			//!< Add the new tab on last position
};

/**
 *	@ingroup		SCEP
 *	@brief			Enumeration on the requested behaviour of a newly created tab
 */
enum class NewTabBehaviour
{
	None,			//!< The new tab is not set current
	Current			//!< The new tab is set current
};
