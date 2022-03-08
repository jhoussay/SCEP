/**
 *	Ported to C++ and adapted from Andrey Makarov's breadcrumbsaddressbar, 2019, MIT license
 *	https://github.com/Winand/breadcrumbsaddressbar
 */

#pragma once

class FilenameModel;
class StyleProxy;
class AddressLineEdit;
class SwitchSpaceWidget;

class QLineEdit;
class QMenu;
class QStyle;
class QLabel;
class QHBoxLayout;
class QToolButton;

#include <QFrame>
#include <QCompleter>
#include <QProxyStyle>
#include <QPixmap>
#include <QFileIconProvider>

#include <vector>
#include <optional>

/**
 *	@ingroup				SCEP
 *
 *	From BreadcrumbsAddressBar
 *
 *	Windows Explorer-like address bar
 */
class BreadcrumbsAddressBar : public QFrame
{
	friend class AddressLineEdit;
	friend class SwitchSpaceWidget;
	friend class StyledToolButton;
	friend class ExplorerWidget2;

	Q_OBJECT

signals:
	/**
	 *	@brief				failed to list a directory
	 */
	void					listdir_error(const QString& path);
	/**
	 *	@brief				entered path does not exist
	 */
	void					path_error(const QString& path);

	/**
	 *	@brief				Signal emitted to request a new path
	 */
	void					path_requested(const QString& path);
	/**
	 *	@brief				Signal emitted when a new path has beed selected
	 */
	void					path_selected(const QString& path);

public:
	BreadcrumbsAddressBar(QWidget* parent = nullptr);

protected:
	/**
	 *	@brief				Init QCompleter to work with filesystem
	 */
	QCompleter*				init_completer(QLineEdit* edit_widget, FilenameModel* model);

	/**
	 *	@brief				Path -> QIcon
	 */
	QIcon					get_icon(const QString& path);

	void					set_line_address_closeOnFocusOut(bool closeOnFocusOut);

	void					line_address_focusOutEvent();

	/**
	 *	@brief				SLOT: fill menu with hidden breadcrumbs list
	 */
	void					_hidden_crumbs_menu_show();

	/**
	 *	@brief				Init common places actions in menu
	 */
	void					init_rootmenu_places(QMenu* menu);

	/**
	 *	@brief				Try to get path label using Shell32 on Windows
	 */
	QString					get_path_label(QString drive_path);

	struct Location
	{
		QString name;
		QString path;
	};

	/**
	 *	@brief				List (name, path) locations in Network Shortcuts folder on Windows
	 */
	static std::vector<Location> list_network_locations();

	/**
	 *	@brief				Init or rebuild device actions in menu
	 */
	void					update_rootmenu_devices();

	void					_clear_crumbs();
	
	/**
	 *	@brief				Get folder name or drive name
	 */
	static QString			path_title(const QString& path);

	void					_insert_crumb(const QString& path);

	void					crumb_mouse_move(QMouseEvent* event);

	/**
	 *	@brief				SLOT: breadcrumb menu item was clicked
	 */
	void					crumb_menuitem_clicked(const QModelIndex& index);
	/**
	 *	@brief				SLOT: breadcrumb was clicked
	 */
	void					crumb_clicked();

	/**
	 *	@brief				SLOT: fill subdirectory list on menu open
	 */
	void					crumb_menu_show();

public:
	/**
	 *	@brief				Set path displayed in this BreadcrumbsAddressBar
	 *	@param path			New path
	 *	@param virtualFolder Indicates whether it is a virtual folder
	 *	@return				Returns `False` if path does not exist or a permission error occurs (for a non virtual folder only).
	 */
	bool					set_path(QString path, bool virtualFolder);

	/**
	 *	@brief				Display loading icon
	 *	@param path			Path being loaded
	 */
	void					set_loading(const QString& path);

protected slots:
	/**
	 *	@brief				Request a new path
	 *
	 *	Can be used as a SLOT: `sender().path` is used if `path` is `None`
	 *	Will provoque the emission of the `path_requested` signal.
	 */
	void					requestPathChange(QString path = {});

protected:
	/**
	 *	@brief				Set edit line text back to current path and switch to view mode
	 */
	void					_cancel_edit();

public:
	/**
	 *	@brief				Get path displayed in this BreadcrumbsAddressBar
	 */
	const QString&			path() const;

protected:
	/**
	 *	@brief				EVENT: switch_space mouse clicked
	 */
	void					switch_space_mouse_up(QMouseEvent* event);

	/**
	 *	@brief				Show text address field
	 *	@param show			Requested state for the address field
	 */
	void					show_address_field(bool show);

	/**
	 *	@brief				SLOT: a breadcrumb is hidden/removed or shown
	 */
	void					crumb_hide_show();

public:
	QSize					minimumSizeHint() const override;

protected:
	/**
	 *	@brief				Monitor breadcrumbs under cursor and switch popup menus
	 */
	void					mouse_pos_timer_event();

private:
	StyleProxy*				p_style_crumbs = nullptr;
	QFileIconProvider		m_file_ico_prov;
	FilenameModel*			p_fs_model = nullptr;
	QLabel*					p_path_icon = nullptr;
	AddressLineEdit*		p_line_address = nullptr;
	QWidget*				p_crumbs_container = nullptr;
	QHBoxLayout*			p_crumbs_cont_layout = nullptr;
	QTimer*					p_mouse_pos_timer = nullptr;
	QToolButton*			p_btn_root_crumb = nullptr;
	QWidget*				p_crumbs_panel = nullptr;
	QWidget*				p_switch_space = nullptr;
	bool					m_ignore_resize = false;
	QString					m_path = {};
	bool					m_line_address_closeOnFocusOut = true;
	std::vector<QAction*>	m_actions_hidden_crumbs;
	std::vector<QAction*>	m_actions_devices;
	QMovie*					ptr_bufferingMovie = nullptr;
};
