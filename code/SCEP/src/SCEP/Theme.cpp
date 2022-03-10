#include <SCEP/Theme.h>
//
#include <Windows.h>
//
#include <QApplication>
#include <QStyleFactory>
#include <QPalette>
//
#include <iostream>
#include <vector>
//
static QString Application_DarkStyleSheet = 
R"(QToolTip {
	color: #ffffff;
	background-color: #2a82da;
	border: 1px solid white;
}
QMenu::separator {
	height: 1px;
	background: white;
}
/* https://gist.github.com/Calinou/ef4e40fa5974bbdb39335c3b8d1b7492*/
/* Scrollbar */
/* From Quassel Wiki: http://sprunge.us/iZGB */
QScrollBar {
	background: palette(base);
	margin: 0;
}
QScrollBar:hover {
	/* Optional: Subtle accent of scrolling area on hover */
	background: #161616; /* base +2 */
}
QScrollBar:vertical {
	width: 11px;
}
QScrollBar:horizontal {
	height: 11px;
}

QScrollBar::handle {
	padding: 0;
	margin: 2px;
	border-radius: 2px;
	border: 2px solid palette(mid);
	background: palette(mid);
}

QScrollBar::handle:vertical {
	min-height: 20px;
	min-width: 0px;
}

QScrollBar::handle:horizontal {
	min-width: 20px;
	min-height: 0px;
}
QScrollBar::handle:hover {
	border-color: palette(light);
	background: palette(light);
}
QScrollBar::handle:pressed {
	background: palette(light);
	border-color: palette(light);
}

QScrollBar::add-line , QScrollBar::sub-line {
	height: 0px;
	border: 0px;
}
QScrollBar::up-arrow, QScrollBar::down-arrow {
	border: 0px;
	width: 0px;
	height: 0px;
}

QScrollBar::add-page, QScrollBar::sub-page {
	background: none;
})";
//
QString Application_LightStyleSheet = 
R"(QToolTip {
	color: #ffffff;
	background-color: #2a82da;
	border: 1px solid white; })";
//
//
//
Theme::Theme()
{
	m_iconPath = 
	{
		{Icon::AddTab,			":/SCEP/icons/plus-48-%1.png"},
		{Icon::CloseTab,		":/SCEP/icons/close-48-%1.png"},
		{Icon::Menu,			":/SCEP/icons/menu-48-%1.png"},
		{Icon::About,			":/SCEP/icons/about-48-%1.png"},
		{Icon::Up,				":/SCEP/icons/up-48-%1.png"},
		{Icon::Left,			":/SCEP/icons/left-48-%1.png"},
		{Icon::Right,			":/SCEP/icons/right-48-%1.png"},
		{Icon::Chevron_Left,	":/SCEP/icons/chevron-left-48-%1.png"},
		{Icon::Chevron_Right,	":/SCEP/icons/chevron-right-48-%1.png"}
	};


	// Get the type
	///////////////

	// based on https://stackoverflow.com/questions/51334674/how-to-detect-windows-10-light-dark-mode-in-win32-application

	// The value is expected to be a REG_DWORD, which is a signed 32-bit little-endian
	auto buffer = std::vector<char>(4);
	auto cbData = static_cast<DWORD>(buffer.size() * sizeof(char));
	auto res = RegGetValueW(
		HKEY_CURRENT_USER,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
		L"AppsUseLightTheme",
		RRF_RT_REG_DWORD, // expected value type
		nullptr,
		buffer.data(),
		&cbData);

	if (res != ERROR_SUCCESS)
	{
		std::cerr << "Error: error_code=" + std::to_string(res) << std::endl;
		m_type = Type::Light;
	}
	else
	{
		// convert bytes written to our buffer to an int, assuming little-endian
		auto i = int(buffer[3] << 24 |
			buffer[2] << 16 |
			buffer[1] << 8 |
			buffer[0]);

		m_type = (i == 1) ? Type::Light : Type::Dark;
	}


	// Apply the theme
	//////////////////

	qApp->setStyle(QStyleFactory::create("Fusion"));

	if (m_type == Type::Dark)
	{
		QPalette darkPalette;

		QColor darkGray(53, 53, 53);
		QColor gray(128, 128, 128);
		QColor black(25, 25, 25);
		QColor blue(42, 130, 218);
	
		darkPalette.setColor(QPalette::Window, darkGray);
		darkPalette.setColor(QPalette::WindowText, Qt::white);
		darkPalette.setColor(QPalette::Base, black);
		darkPalette.setColor(QPalette::AlternateBase, darkGray);
		darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
		darkPalette.setColor(QPalette::ToolTipText, Qt::white);
		darkPalette.setColor(QPalette::Text, Qt::white);
		darkPalette.setColor(QPalette::Button, black);
		darkPalette.setColor(QPalette::ButtonText, Qt::white);
		darkPalette.setColor(QPalette::BrightText, Qt::red);
		darkPalette.setColor(QPalette::Link, blue);
	
		darkPalette.setColor(QPalette::Highlight, blue);
		darkPalette.setColor(QPalette::HighlightedText, Qt::black);
	
		//darkPalette.setColor(QPalette::Active, QPalette::Button, black);
		darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, gray);
		darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, gray);
		darkPalette.setColor(QPalette::Disabled, QPalette::Text, gray);
		darkPalette.setColor(QPalette::Disabled, QPalette::Light, darkGray);

		qApp->setPalette(darkPalette);
		qApp->setStyleSheet(Application_DarkStyleSheet);
	}
	else
	{
		qApp->setStyleSheet(Application_LightStyleSheet);
	}
}
//
Theme::Type Theme::type() const
{
	return m_type;
}
//
QString Theme::iconPath(Icon iconType) const
{
	return m_iconPath.at(iconType).arg(m_type == Type::Dark ? "dark" : "light");
}
//
QPixmap Theme::icon(Icon iconType) const
{
	return QPixmap( iconPath(iconType) );
}
//
