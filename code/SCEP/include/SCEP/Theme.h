#pragma once
//
#include <map>
//
#include <QString>
#include <QPixmap>
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
		About
	};

public:
	Theme();
	~Theme() = default;

public:
	Type type() const;
	QString iconPath(Icon iconType) const;
	QPixmap icon(Icon iconType) const;

private:
	Type m_type = Type::Light;

	std::map<Icon, QString> m_iconPath;
};
