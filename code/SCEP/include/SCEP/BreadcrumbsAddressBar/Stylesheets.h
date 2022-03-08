/**
 *	Ported to C++ and adapted from Andrey Makarov's breadcrumbsaddressbar, 2019, MIT license
 *	https://github.com/Winand/breadcrumbsaddressbar
 */

#pragma once

#include <QString>

/**
 *	@ingroup				SCEP
 *
 *	From BreadcrumbsAddressBar
 *
 *	@brief					Stylesheets to customize Qt controls
 */
QString Style_root_toolbutton =
	"QToolButton::right-arrow {\n"
	"	image: url(:/SCEP/icons/iconfinder_icon-ios7-arrow-right_211607.png);\n"
	"}\n"
	"QToolButton::left-arrow {\n"
	"	image: url(:/SCEP/icons/iconfinder_icon-ios7-arrow-left_211689.png);\n"
	"}\n"
	"QToolButton::menu-indicator {\n"
	"	image: none; /* https://stackoverflow.com/a/19993662 */\n"
	"}\n";
