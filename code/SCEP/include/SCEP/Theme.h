#pragma once
//
#include <map>
//
#include <QString>
#include <QPixmap>
#include <QIcon>
//
class Theme
{
public:
	enum class Type
	{
		Dark,
		Light
	};

	enum class Icon
	{
		AddTab,
		CloseTab,
		Menu,
		About,
		Up,
		Left,
		Right,
		Chevron_Left,
		Chevron_Right
	};

public:
	Theme();
	~Theme() = default;

public:
	Type type() const;
	QString path(Icon iconType) const;
	QPixmap pixmap(Icon iconType) const;
	QIcon icon(Icon iconType) const;

private:
	Type m_type = Type::Light;

	std::map<Icon, QString> m_iconPath;
};
