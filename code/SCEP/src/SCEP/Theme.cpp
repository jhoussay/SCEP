#include <SCEP/Theme.h>
//
#include <Windows.h>
//
#include <QSettings>
#include <QApplication>
#include <QStyleFactory>
#include <QPalette>
#include <QPainter>
#include <QtDebug>
//
#include <vector>
//
static QString ThemeStr = "Theme";
//
static std::map<Theme::Style, QString> StylesStr = 
{
	{Theme::Style::Auto, "Auto"},
	{Theme::Style::Dark, "Dark"},
	{Theme::Style::Light, "Light"}
};
//
static QString Application_DarkStyleSheet = 
R"(QToolTip {
	color: #ffffff;
	background-color: #35322f;
	border: 1px solid #5d5b59;
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
	color: #5d5b59;
	background-color: white;
	border: 1px solid #5d5b59; })";
//
//
//
Theme::Theme(QSettings* pSettings)
	:	ptr_settings(pSettings)
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


	// Get the style
	////////////////

	// User style
	QString userStyleStr = ptr_settings->value(ThemeStr, StylesStr[Theme::Style::Auto]).toString();
	bool found = false;
	for (const auto& [style, styleStr] : StylesStr)
	{
		if (styleStr == userStyleStr)
		{
			found = true;
			m_userStyle = style;
			break;
		}
	}
	if (! found)
	{
		m_userStyle = Theme::Style::Auto;
		qWarning() << "Invalid user style " << userStyleStr << ". Defaulting to " << StylesStr[m_userStyle];
	}

	// Auto theme
	if (m_userStyle == Theme::Style::Auto)
	{
		// based on https://stackoverflow.com/questions/51334674/how-to-detect-windows-10-light-dark-mode-in-win32-application

		// The value is expected to be a REG_DWORD, which is a signed 32-bit little-endian
		std::vector<char> buffer(4);
		DWORD cbData = static_cast<DWORD>(buffer.size() * sizeof(char));
		LSTATUS res = RegGetValueW(
			HKEY_CURRENT_USER,
			L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
			L"AppsUseLightTheme",
			RRF_RT_REG_DWORD, // expected value type
			nullptr,
			buffer.data(),
			&cbData);

		if (res != ERROR_SUCCESS)
		{
			m_effectiveStyle = Style::Light;
			qWarning() << "Could not read system theme (error_code =" << res << "). Defaulting to " << StylesStr[m_effectiveStyle];
		}
		else
		{
			// convert bytes written to our buffer to an int, assuming little-endian
			auto i = int(buffer[3] << 24 |
						 buffer[2] << 16 |
						 buffer[1] << 8 |
						 buffer[0]);

			m_effectiveStyle = (i == 1) ? Style::Light : Style::Dark;
		}
	}
	// Other themes
	else
	{
		m_effectiveStyle = m_userStyle;
	}


	// Apply the theme
	//////////////////

	qApp->setStyle(QStyleFactory::create("Fusion"));

	if (m_effectiveStyle == Style::Dark)
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
Theme::Style Theme::userStyle() const
{
	return m_userStyle;
}
//
void Theme::setUserStyle(Style userStyle)
{
	m_userStyle = userStyle;
	ptr_settings->setValue(ThemeStr, StylesStr[m_userStyle]);
	ptr_settings->sync();
}
//
Theme::Style Theme::effectiveStyle() const
{
	return m_effectiveStyle;
}
//
QString Theme::path(Icon iconType) const
{
	return m_iconPath.at(iconType).arg(m_effectiveStyle == Style::Dark ? "dark" : "light");
}
//
QPixmap Theme::pixmap(Icon iconType) const
{
	return QPixmap( path(iconType) );
}
//
QIcon Theme::icon(Icon iconType) const
{
	QPixmap enabledPixmap = pixmap(iconType);
	
	QPixmap disabledPixmap(enabledPixmap.size());
	disabledPixmap.fill(Qt::transparent);
	QPainter p(&disabledPixmap);
	p.setBackgroundMode(Qt::TransparentMode);
	p.setBackground(QBrush(Qt::transparent));
	p.eraseRect(enabledPixmap.rect());
	p.setOpacity(0.5);
	p.drawPixmap(0, 0, enabledPixmap);
	p.end();

	QIcon ico(enabledPixmap);
	ico.addPixmap(disabledPixmap, QIcon::Disabled, QIcon::Off);
	ico.addPixmap(disabledPixmap, QIcon::Disabled, QIcon::On);
	//QIcon ico(disabledPixmap);
	return ico;
}
//
