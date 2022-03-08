﻿#include <SCEP/Theme.h>
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
Theme::Theme()
{
	m_iconPath = 
	{
		{Icon::AddTab,		":/SCEP/icons/plus-48-%1.png"},
		{Icon::CloseTab,	":/SCEP/icons/close-48-%1.png"},
		{Icon::Menu,		":/SCEP/icons/menu-48-%1.png"},
		{Icon::About,		":/SCEP/icons/about-48-%1.png"}
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
		darkPalette.setColor(QPalette::Button, darkGray);
		darkPalette.setColor(QPalette::ButtonText, Qt::white);
		darkPalette.setColor(QPalette::BrightText, Qt::red);
		darkPalette.setColor(QPalette::Link, blue);
	
		darkPalette.setColor(QPalette::Highlight, blue);
		darkPalette.setColor(QPalette::HighlightedText, Qt::black);
	
		darkPalette.setColor(QPalette::Active, QPalette::Button, gray.darker());
		darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, gray);
		darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, gray);
		darkPalette.setColor(QPalette::Disabled, QPalette::Text, gray);
		darkPalette.setColor(QPalette::Disabled, QPalette::Light, darkGray);



		qApp->setPalette(darkPalette);
		qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
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
