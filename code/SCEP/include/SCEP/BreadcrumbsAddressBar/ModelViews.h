/**
 *	Ported to C++ and adapted from Andrey Makarov's breadcrumbsaddressbar, 2019, MIT license
 *	https://github.com/Winand/breadcrumbsaddressbar
 */

#pragma once

#include <QStringListModel>
#include <QListView>
#include <QMenu>

#include <optional>

class Theme;

/**
 *	@ingroup				SCEP
 *
 *	From BreadcrumbsAddressBar
 *
 *	Model used by QCompleter for file name completions.
 *	Constructor options:
 *	`filter_` (None, 'dirs') - include all entries or folders only
 *	`icon_provider` (func, 'internal', None) - a function which gets path
 *											   and returns QIcon
 */
class FilenameModel : public QStringListModel
{
public:
	enum class Filter
	{
		None,
		Dirs
	};

	enum class Mode
	{
		Completer,
		Lister
	};

	using IconProviderFn = std::function<QIcon(const QString&)>;

public:
	FilenameModel(QWidget* parent, Filter filter, IconProviderFn icon_provider);

	/**
	 *	@brief				Get names/icons of files
	 */
	QVariant				data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

	/**
	 *	@brief				List entries in `path` directory
	 */
	QStringList				get_file_list(const QString& path) const;

	void					setPathPrefix(QString prefix, Mode mode);

private:
	std::optional<QString>	m_current_path;
	Filter					m_filter = Filter::None;
	IconProviderFn			m_icon_provider;
};
//
class MenuListView;
//
class ListView : public QListView
{
public:
	ListView(MenuListView* pMenuListView, Theme* ptrTheme);

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;

protected:
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void leaveEvent(QEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void keyPressEvent(QKeyEvent *event);

private:
	MenuListView* ptr_menuListView = nullptr;
};
//
/**
 *  @ingroup				SCEP
 *
 *	From BreadcrumbsAddressBar
 *
 *	QMenu with QListView.
 *	Supports `activated`, `clicked`, `setModel`.
 */
class MenuListView : public QMenu
{
	Q_OBJECT

	friend class ListView;

public:
	MenuListView(QWidget* parent, Theme* ptrTheme);

	void setModel(QAbstractItemModel *model);

	void key_press_event(QKeyEvent *event);

	void update_current_index(QMouseEvent* event);

	void clear_selection(QEvent* event = nullptr);
	
	void mouse_press_event(QMouseEvent *event);

	/**
	 *	When item is clicked w/ left mouse button close menu, emit `clicked`.
	 *	Check if there was left button press event inside this widget.
	 */
	void mouse_release_event(QMouseEvent *event);

	QSize size_hint() const;

protected:
	bool event(QEvent *e) override;

signals:
	void activated(const QModelIndex &index);
	void clicked(const QModelIndex &index);

private:
	static constexpr int Max_visible_items = 16;

	ListView* p_listview = nullptr;
	QModelIndex m_last_index = {};
	bool m_flag_mouse_l_pressed = false;
};
//
