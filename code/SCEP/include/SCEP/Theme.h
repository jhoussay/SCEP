#pragma once
//
#include <map>
//
class QSettings;
//
#include <QString>
#include <QPixmap>
#include <QIcon>
//
/**
 *	@ingroup			SCEP
 *	@brief				Theme and utilities
 */
class Theme
{
public:
	/**
	 *	@brief			Style
	 *
	 *	This enumeration can be used for 2 distincts cases :
	 *	- User settings: Auto/Dark/Light
	 *	- Effective theme type : Dark/Light
	 */
	enum class Style
	{
		Auto,			//!< Auto (system based)
		Dark,			//!< Dark (user settings or effective theme style)
		Light			//!< Light (user settings or effective theme style)
	};

	/**
	 *	@brief			Icon
	 */
	enum class Icon
	{
		AddTab,			//!< Add new tab
		CloseTab,		//!< Close tab
		Menu,			//!< Application menu
		About,			//!< About window
		Up,				//!< Up arrow
		Left,			//!< Left arrow
		Right,			//!< Right arrow
		Chevron_Left,	//!< Left chevron
		Chevron_Right,	//!< Right chevron
		Refresh			//!< Refresh
	};

public:
	Theme(QSettings* pSettings);
	~Theme() = default;

public:
	/**
	 *	@brief			Returns the user style
	 *	@return			Auto/Dark/Light
	 */
	Style				userStyle() const;
	/**
	 *	@brief			Sets the user style.
	 *
	 *	This settings will take effect on restart.
	 *
	 *	@param userStyle Auto/Dark/Light
	 */
	void				setUserStyle(Style userStyle);

	/**
	 *	@brief			Returns the effective style
	 *	@return			Dark/Light
	 */
	Style				effectiveStyle() const;

	/**
	 *	@brief			Returns the path (in Qt resource filesystem) of the specified icon
	 *	@param iconType	Requested icon
	 */
	QString				path(Icon iconType) const;
	/**
	 *	@brief			Returns the pixmap of the specified icon
	 *	@param iconType	Requested icon
	 */
	QPixmap				pixmap(Icon iconType) const;
	/**
	 *	@brief			Returns the icon of the specified icon
	 *	@param iconType	Requested icon
	 */
	QIcon				icon(Icon iconType) const;

private:
	QSettings*					ptr_settings	=	nullptr;		//!< Settings
	Style						m_userStyle		=	Style::Auto;	//!< User style
	Style						m_effectiveStyle=	Style::Light;	//!< Effective style
	std::map<Icon, QString>		m_iconPath;							//!< Icon paths
};
