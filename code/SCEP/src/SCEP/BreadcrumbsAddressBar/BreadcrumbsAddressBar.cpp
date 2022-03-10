/**
 *	Ported to C++ and adapted from Andrey Makarov's breadcrumbsaddressbar, 2019, MIT license
 *	https://github.com/Winand/breadcrumbsaddressbar
 */

#include <SCEP/BreadcrumbsAddressBar/BreadcrumbsAddressBar.h>
#include <SCEP/BreadcrumbsAddressBar/Layouts.h>
#include <SCEP/BreadcrumbsAddressBar/ModelViews.h>
#include <SCEP/BreadcrumbsAddressBar/Stylesheets.h>
#include <SCEP/win32_utils.h>
#include <SCEP/Theme.h>
//
#include <QStyleOptionToolButton>
#include <QStyleFactory>
#include <QToolButton>
#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QLineEdit>
#include <QLabel>
#include <QCompleter>
#include <QTimer>
#include <QStandardPaths>
#include <QDir>
#include <QStorageInfo>
#include <QFileDialog>
#include <QKeyEvent>
#include <QLayoutItem>
#include <QMovie>
#include <QtDebug>
//
#include <windows.h>
//
class StyledToolButton : public QToolButton
{
public:
	StyledToolButton(QWidget* parent, BreadcrumbsAddressBar* ptrBreadcrumbsAddressBar)
		:	QToolButton(parent)
		,	ptr_breadcrumbsAddressBar(ptrBreadcrumbsAddressBar)
	{}

public:
	// Make public a protected method
	void initStyleOption(QStyleOptionToolButton *option) const
	{
		QToolButton::initStyleOption(option);
	}

protected:
	void mouseMoveEvent(QMouseEvent *event) override
	{
		if (ptr_breadcrumbsAddressBar)
		{
			ptr_breadcrumbsAddressBar->crumb_mouse_move(event);
		}
	}

private:
	BreadcrumbsAddressBar* ptr_breadcrumbsAddressBar = nullptr;
};
//
//
//
class StyleProxy : public QProxyStyle
{
public:
	StyleProxy(QStyle* style, const QPixmap& arrow_pix)
		:	QProxyStyle(style)
		,	m_arrow_pix(arrow_pix)
	{
		m_stylename = baseStyle()->objectName();
	}

	void drawPrimitive(	QStyle::PrimitiveElement element,
						const QStyleOption *option,
						QPainter *painter,
						const QWidget *widget = nullptr) const override
	{
		// QToolButton elements:
		// 13: PE_PanelButtonCommand (Fusion) - Fusion button background, called from 15 and 24 calls
		// 15: PE_PanelButtonTool (Windows, Fusion) - left part background (XP/Vista styles do not draw it with `drawPrimitive`)
		// 19: PE_IndicatorArrowDown (Windows, Fusion) - right part down arrow (XP/Vista styles draw it in 24 call)
		// 24: PE_IndicatorButtonDropDown (Windows, XP, Vista, Fusion) - right part background (+arrow for XP/Vista)
		// 
		// Arrow is drawn along with PE_IndicatorButtonDropDown (XP/Vista)
		// https://github.com/qt/qtbase/blob/0c51a8756377c40180619046d07b35718fcf1784/src/plugins/styles/windowsvista/qwindowsxpstyle.cpp#L1406
		// https://github.com/qt/qtbase/blob/0c51a8756377c40180619046d07b35718fcf1784/src/plugins/styles/windowsvista/qwindowsxpstyle.cpp#L666
		// drawBackground paints with DrawThemeBackgroundEx WinApi function
		// https://docs.microsoft.com/en-us/windows/win32/api/uxtheme/nf-uxtheme-drawthemebackgroundex
		if (win_modern.contains(m_stylename) && (element == QStyle::PE_IndicatorButtonDropDown) )
			element = QStyle::PE_IndicatorArrowDown;  // see below
		if (element == QStyle::PE_IndicatorArrowDown)
		{
			if (const StyledToolButton* pToolButton = dynamic_cast<const StyledToolButton*>(widget))
			{
				QStyleOptionToolButton opt_;
				pToolButton->initStyleOption(&opt_);
				QRect rc = QProxyStyle::subControlRect(QStyle::CC_ToolButton, &opt_,
											QStyle::SC_ToolButtonMenu, widget);
				if (win_modern.contains(m_stylename))
				{
					// By default PE_IndicatorButtonDropDown draws arrow along
					// with right button art. Draw 2px clipped left part instead
					QPainterPath path;
					path.addRect(QRectF(rc));
					painter->setClipPath(path);
					QProxyStyle::drawPrimitive(QStyle::PE_PanelButtonTool, option, painter, widget);
				}
				// centered square
				rc.moveTop(int((rc.height() - rc.width()) / 2));
				rc.setHeight(rc.width());
				// p.setRenderHint(p.Antialiasing)
				painter->drawPixmap(rc, m_arrow_pix, QRect());
			}
		}
		else
		{
			QProxyStyle::drawPrimitive(element, option, painter, widget);
		}
	}

	QRect subControlRect(	QStyle::ComplexControl control,
							const QStyleOptionComplex *option,
							QStyle::SubControl subControl,
							const QWidget *widget = nullptr) const override
	{
		QRect rc = QProxyStyle::subControlRect(control, option, subControl, widget);
		if (win_modern.contains(m_stylename) && subControl == QProxyStyle::SC_ToolButtonMenu)
			rc.adjust(-2, 0, 0, 0); // cut 2 left pixels to create flat edge
		return rc;
	}

private:
	static QStringList win_modern;

	QPixmap	m_arrow_pix;
	QString m_stylename;
};
//
QStringList StyleProxy::win_modern = {"windowsxp", "windowsvista"};
//
//
// 
class AddressLineEdit : public QLineEdit
{
public:
	AddressLineEdit(BreadcrumbsAddressBar* ptrBreadcrumbsAddressBar)
		:	QLineEdit(ptrBreadcrumbsAddressBar)
		,	ptr_breadcrumbsAddressBar(ptrBreadcrumbsAddressBar)
	{}

	// TODO Implement canInsertFromMimeData() and insertFromMimeData() to replace every "/" by "\\" when pasting

protected:
	void keyPressEvent(QKeyEvent *event) override
	{
		if (ptr_breadcrumbsAddressBar)
		{
			if (event->key() == Qt::Key_Escape)
			{
				ptr_breadcrumbsAddressBar->_cancel_edit();
			}
			else if ( (event->key() == Qt::Key_Return) || (event->key() == Qt::Key_Enter) )
			{
				if (QFileInfo(text()).isAbsolute())
				{
					// We need to handle this key press to close QCompleter popup
					QLineEdit::keyPressEvent(event);

					qDebug() << "keyPressEvent : Enter/Return -> " << event->key();
					ptr_breadcrumbsAddressBar->requestPathChange(text());
				}
			}
			else if (event->key() == Qt::Key_Slash)
			{
				// Replace "/" by "\\" on the fly
				QKeyEvent keyEvent(event->type(), Qt::Key_Backslash, event->modifiers(), "\\", event->isAutoRepeat(), event->count());
				QLineEdit::keyPressEvent(&keyEvent);
			}
			else
			{
				QLineEdit::keyPressEvent(event);
			}
		}
	}

	void focusOutEvent(QFocusEvent * /*event*/) override
	{
		if (ptr_breadcrumbsAddressBar)
		{
			ptr_breadcrumbsAddressBar->line_address_focusOutEvent();
		}
	}

	void contextMenuEvent(QContextMenuEvent *event) override
	{
		if (ptr_breadcrumbsAddressBar)
		{
			ptr_breadcrumbsAddressBar->set_line_address_closeOnFocusOut(false);
		}
		QLineEdit::contextMenuEvent(event);
	}

private:
	BreadcrumbsAddressBar* ptr_breadcrumbsAddressBar = nullptr;
};
//
//
//
class SwitchSpaceWidget : public QWidget
{
public:
	SwitchSpaceWidget(BreadcrumbsAddressBar* ptrBreadcrumbsAddressBar)
		:	QWidget(ptrBreadcrumbsAddressBar)
		,	ptr_breadcrumbsAddressBar(ptrBreadcrumbsAddressBar)
	{}

protected:
	void mouseReleaseEvent(QMouseEvent *event)
	{
		if (ptr_breadcrumbsAddressBar)
		{
			ptr_breadcrumbsAddressBar->switch_space_mouse_up(event);
		}
	}
	

private:
	BreadcrumbsAddressBar* ptr_breadcrumbsAddressBar = nullptr;
};
//
//
//
static const char* Path_Id = "path";
//
inline QString get_path_property(const QObject* pObject)
{
	QString path;
	if (pObject)
	{
		QVariant propertyVariant = pObject->property(Path_Id);
		//qDebug() << "Reading " << Qt::hex << pObject << " -> " << (propertyVariant.isValid() ? propertyVariant.toString() : "\"\"");
		path = propertyVariant.toString();
	}
	return path;
}
//
inline void set_path_property(QObject* pObject, const QString& path)
{
	if (pObject)
	{
		pObject->setProperty(Path_Id, path);
		//qDebug() << "Writing " << Qt::hex << pObject << " -> " << path;
		assert(get_path_property(pObject) == path);
	}
}
//
//
//
QSize TRANSP_ICON_SIZE = {40, 40}; // px, size of generated semi-transparent icons
//
BreadcrumbsAddressBar::BreadcrumbsAddressBar(Theme* ptrTheme, QWidget* parent)
	:	QFrame(parent)
	,	ptr_theme(ptrTheme)
{
	setObjectName("BreadcrumbsAddressBar");
	if (ptr_theme->type() == Theme::Type::Dark)
	{
		setStyleSheet("QFrame#BreadcrumbsAddressBar { border: 1px solid gray; }");
	}

	p_style_crumbs = new StyleProxy(QStyleFactory::create(qApp->style()->objectName()), ptr_theme->icon(Theme::Icon::Chevron_Right));

	QHBoxLayout* pLayout = new QHBoxLayout(this);

	FilenameModel::IconProviderFn iconProvider = [this](const QString& path) -> QIcon {return get_icon(path);};

	p_fs_model = new FilenameModel(this, FilenameModel::Filter::Dirs, iconProvider);

	QPalette pal = palette();
	pal.setColor(QPalette::Window, pal.color(QPalette::Base));
	setPalette(pal);
	setAutoFillBackground(true);
	setFrameShape(QFrame::StyledPanel);
	// TODO JHO layout() ?? or pLayout ?
	layout()->setContentsMargins(4, 0, 0, 0);
	layout()->setSpacing(0);

	p_path_icon = new QLabel(this);
	pLayout->addWidget(p_path_icon);
	ptr_bufferingMovie = new QMovie(":/SCEP/icons/buffering2.gif", QByteArray(), p_path_icon);
	ptr_bufferingMovie->setScaledSize(QSize(16, 16));

	// Edit presented path textually
	p_line_address = new AddressLineEdit(this);
	p_line_address->setFrame(false);
	p_line_address->hide();
	pLayout->addWidget(p_line_address);
	// Add QCompleter to address line
	QCompleter* pCompleter = init_completer(p_line_address, p_fs_model);
	connect(pCompleter, qOverload<const QString&>(&QCompleter::activated), this, &BreadcrumbsAddressBar::requestPathChange);

	// Container for `btn_crumbs_hidden`, `crumbs_panel`, `switch_space`
	p_crumbs_container = new QWidget(this);
	p_crumbs_cont_layout = new QHBoxLayout(p_crumbs_container);
	p_crumbs_cont_layout->setContentsMargins(0, 0, 0, 0);
	p_crumbs_cont_layout->setSpacing(0);
	pLayout->addWidget(p_crumbs_container);

	// Monitor breadcrumbs under cursor and switch popup menus
	p_mouse_pos_timer = new QTimer(this);
	connect(p_mouse_pos_timer, &QTimer::timeout, this, &BreadcrumbsAddressBar::mouse_pos_timer_event);

	// Hidden breadcrumbs menu button
	p_btn_root_crumb = new QToolButton(this);
	p_btn_root_crumb->setAutoRaise(true);
	p_btn_root_crumb->setPopupMode(QToolButton::InstantPopup);
	p_btn_root_crumb->setArrowType(Qt::RightArrow);
	p_btn_root_crumb->setStyleSheet(Style_root_toolbutton
		.arg(ptr_theme->iconPath(Theme::Icon::Chevron_Right))
		.arg(ptr_theme->iconPath(Theme::Icon::Chevron_Left)));
	p_btn_root_crumb->setMinimumSize(p_btn_root_crumb->minimumSizeHint());
	p_crumbs_cont_layout->addWidget(p_btn_root_crumb);
	QMenu* menu = new QMenu(p_btn_root_crumb);
	connect(menu, &QMenu::aboutToShow, this, &BreadcrumbsAddressBar::_hidden_crumbs_menu_show);
	connect(menu, &QMenu::aboutToHide, p_mouse_pos_timer, &QTimer::stop);
	p_btn_root_crumb->setMenu(menu);
	init_rootmenu_places(menu); // Desktop, Home, Downloads...
	update_rootmenu_devices(); // C:, D:...

	// Container for breadcrumbs
	p_crumbs_panel = new QWidget(this);
	LeftHBoxLayout* pCrumbs_layout = new LeftHBoxLayout(p_crumbs_panel);
	connect(pCrumbs_layout, &LeftHBoxLayout::widget_state_changed, this, &BreadcrumbsAddressBar::crumb_hide_show);
	pCrumbs_layout->setContentsMargins(0, 0, 0, 0);
	pCrumbs_layout->setSpacing(0);
	p_crumbs_cont_layout->addWidget(p_crumbs_panel);

	// Clicking on empty space to the right puts the bar into edit mode
	p_switch_space = new SwitchSpaceWidget(this);
	// s_policy = self.switch_space.sizePolicy()
	// s_policy.setHorizontalStretch(1)
	// self.switch_space.setSizePolicy(s_policy)
	// crumbs_cont_layout.addWidget(self.switch_space)
	pCrumbs_layout->set_space_widget(p_switch_space);

	setMaximumHeight(p_line_address->height()); // FIXME:

	m_ignore_resize = false;
	m_path = QString();
	set_path(m_path, false);
}
//
QCompleter* BreadcrumbsAddressBar::init_completer(QLineEdit* edit_widget, FilenameModel* model)
{
	QCompleter* completer = new QCompleter(this);
	completer->setCaseSensitivity(Qt::CaseInsensitive);
	completer->setModel(model);
	// Optimize performance https://stackoverflow.com/a/33454284/1119602
	if (QListView* popup = dynamic_cast<QListView*>(completer->popup()))
	{
		popup->setUniformItemSizes(true);
		popup->setLayoutMode(QListView::Batched);
	}
	edit_widget->setCompleter(completer);
	connect(edit_widget, &QLineEdit::textEdited, [=](const QString& text) { model->setPathPrefix(text, FilenameModel::Mode::Completer); });
	return completer;
}
//
QIcon BreadcrumbsAddressBar::get_icon(const QString& path)
{
	QFileInfo fileinfo(path);
	QIcon dat = m_file_ico_prov.icon(fileinfo);
	if (fileinfo.isHidden())
	{
		QPixmap pmap(TRANSP_ICON_SIZE);
		pmap.fill(Qt::transparent);
		QPainter painter(&pmap);
		painter.setOpacity(0.5);
		dat.paint(&painter, 0, 0, TRANSP_ICON_SIZE.width(), TRANSP_ICON_SIZE.height());
		painter.end();
		dat = QIcon(pmap);
	}
	return dat;
}
//
void BreadcrumbsAddressBar::set_line_address_closeOnFocusOut(bool closeOnFocusOut)
{
	m_line_address_closeOnFocusOut = closeOnFocusOut;
}
//
void BreadcrumbsAddressBar::line_address_focusOutEvent()
{
	if (! m_line_address_closeOnFocusOut)
	{
		qDebug() << "line_address_focusOutEvent -> ! closeOnFocusOutEvent";
		// do not cancel edit on context menu
		m_line_address_closeOnFocusOut = true;
		return; 
	}
	else
	{
		qDebug() << "line_address_focusOutEvent -> closeOnFocusOutEvent";
		_cancel_edit();
	}
}
//
void BreadcrumbsAddressBar::_hidden_crumbs_menu_show()
{
	p_mouse_pos_timer->start(100);
	if (QMenu* menu = dynamic_cast<QMenu*>(sender()))
	{
		for (QAction* action : m_actions_hidden_crumbs)
			menu->removeAction(action);
		m_actions_hidden_crumbs.clear();

		QAction* first_action = menu->actions()[0]; // places section separator

		if (LeftHBoxLayout* pLayout = dynamic_cast<LeftHBoxLayout*>(p_crumbs_panel->layout()))
		{
			for (QWidget* i : pLayout->widgets(LeftHBoxLayout::Visibility::Hidden))
			{
				if (QAbstractButton* pButton = dynamic_cast<QAbstractButton*>(i))
				{
					QString path = get_path_property(i);
					QAction* action = new QAction(get_icon(path), pButton->text(), menu);
					set_path_property(action, path);
					connect(action, SIGNAL(triggered(bool)), this, SLOT(requestPathChange()));
					menu->insertAction(first_action, action);
					m_actions_hidden_crumbs.push_back(action);
					first_action = action;
				}
			}
		}
	}
}
//
void BreadcrumbsAddressBar::init_rootmenu_places(QMenu* menu)
{
	menu->addSeparator();
	
	QString uname;
	QByteArray user = qgetenv("USER");
	QByteArray username = qgetenv("USERNAME");
	if (! user.isEmpty())
		uname = user;
	else if (! username.isEmpty())
		uname = username;
	else
		uname = tr("Home");

	std::vector<std::pair<QString, QString>> location = 
	{
		{ "Desktop", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) },
		{ uname, QStandardPaths::writableLocation(QStandardPaths::HomeLocation) },
		{ "Documents", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) },
		{ "Downloads", QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) }
	};

	for (auto [name, path] : location)
	{
		name = get_path_label(path); // JHO : win32
		QAction* action = menu->addAction(get_icon(path), name);
		set_path_property(action, path);
		connect(action, SIGNAL(triggered(bool)), this, SLOT(requestPathChange()));
	}
}
//
QString BreadcrumbsAddressBar::get_path_label(QString drive_path)
{
	return getPathLabel(drive_path);
}
//
std::vector<BreadcrumbsAddressBar::Location> BreadcrumbsAddressBar::list_network_locations()
{
	std::vector<BreadcrumbsAddressBar::Location> rslt;

	QStandardPaths::StandardLocation HOME = QStandardPaths::HomeLocation;
	QString user_folder = QStandardPaths::writableLocation(HOME);
	QString network_shortcuts = user_folder + "/AppData/Roaming/Microsoft/Windows/Network Shortcuts";
	for (QString i : QDir(network_shortcuts).entryList(QDir::Dirs | QDir::NoDotAndDotDot))
	{
		QFileInfo link(i + "/target.lnk");
		if (! link.exists())
			continue;
		QString path = link.symLinkTarget();
		if (! path.isEmpty()) // `symLinkTarget` doesn't read e.g. FTP links
			rslt.push_back({QFileInfo(i).fileName(), path});
	}

	return rslt;
}
//
void BreadcrumbsAddressBar::update_rootmenu_devices()
{
	QMenu* menu = p_btn_root_crumb->menu();
	for (QAction* action : m_actions_devices)
		menu->removeAction(action);
	m_actions_devices = {menu->addSeparator()};
	for (QStorageInfo i : QStorageInfo::mountedVolumes()) // QDir.drives():
	{
		QString path = i.rootPath();
		QString label = i.displayName();
		if (label == path) // TODO JHO win32
			label = get_path_label(path);
		QString trimmedPath = path;
		if (trimmedPath.endsWith("/") || trimmedPath.endsWith("\\"))
			trimmedPath = trimmedPath.left(trimmedPath.size()-1);
		QString caption = QString("%1 (%2)").arg(label).arg(trimmedPath);
		QAction* action = menu->addAction(get_icon(path), caption);
		set_path_property(action, path);
		connect(action, SIGNAL(triggered(bool)), this, SLOT(requestPathChange()));
		m_actions_devices.push_back(action);
	}
	// TODO JHO win32
	// Network locations
	for (Location location : list_network_locations())
	{
		const QString& label = location.name;
		const QString& path = location.path;
		QAction* action = menu->addAction(get_icon(path), label);
		set_path_property(action, path);
		connect(action, SIGNAL(triggered(bool)), this, SLOT(requestPathChange()));
		m_actions_devices.push_back(action);
	}
}
//
void BreadcrumbsAddressBar::_clear_crumbs()
{
	if (LeftHBoxLayout* pLayout = dynamic_cast<LeftHBoxLayout*>(p_crumbs_panel->layout()))
	{
		while (pLayout->count()) // LeftHBoxLayout::count !!
		{
			QWidget* widget = pLayout->takeAt(0)->widget();
			if (widget)
			{
				// Unset style or `StyleProxy.drawPrimitive` is called once with
				// mysterious `QWidget` instead of `QToolButton` (Windows 7)
				widget->setStyle(nullptr);
				widget->deleteLater();
			}
		}
	}
}
//
QString BreadcrumbsAddressBar::path_title(const QString& path)
{
	return getPathLabel(path);
}
//
void BreadcrumbsAddressBar::_insert_crumb(const QString& path)
{
	StyledToolButton* btn = new StyledToolButton(p_crumbs_panel, this);
	btn->setAutoRaise(true);
	btn->setPopupMode(QToolButton::MenuButtonPopup);
	btn->setStyle(p_style_crumbs);
	btn->setMouseTracking(true);
	btn->setText(path_title(path));
	set_path_property(btn, path);
	connect(btn, &StyledToolButton::clicked, this, &::BreadcrumbsAddressBar::crumb_clicked);
	MenuListView* menu = new MenuListView(btn, ptr_theme);
	connect(menu, &MenuListView::aboutToShow, this, &BreadcrumbsAddressBar::crumb_menu_show);
	menu->setModel(p_fs_model);
	connect(menu, &MenuListView::clicked, this, &BreadcrumbsAddressBar::crumb_menuitem_clicked);
	connect(menu, &MenuListView::activated, this, &BreadcrumbsAddressBar::crumb_menuitem_clicked);
	connect(menu, &MenuListView::aboutToHide, p_mouse_pos_timer, &QTimer::stop);
	btn->setMenu(menu);
	if (LeftHBoxLayout* pCrumbsPanelLayout = dynamic_cast<LeftHBoxLayout*>(p_crumbs_panel->layout()))
	{
		pCrumbsPanelLayout->insertWidget(0, btn);
	}
	btn->setMinimumSize(btn->minimumSizeHint()); // fixed size breadcrumbs
	QSizePolicy sp = btn->sizePolicy();
	sp.setVerticalPolicy(QSizePolicy::Minimum);
	btn->setSizePolicy(sp);
	//qDebug() << path << path_title(path) << btn->size() << btn->sizeHint() << btn->minimumSizeHint();
}
//
void BreadcrumbsAddressBar::crumb_mouse_move(QMouseEvent* /*event*/)
{
	// print('move!')
}
//
void BreadcrumbsAddressBar::crumb_menuitem_clicked(const QModelIndex& index)
{
	requestPathChange(index.data(Qt::EditRole).toString());
}
//
void BreadcrumbsAddressBar::crumb_clicked()
{
	requestPathChange(get_path_property(sender()));
}
//
void BreadcrumbsAddressBar::crumb_menu_show()
{
	if (MenuListView* menu = dynamic_cast<MenuListView*>(sender()))
	{
		p_fs_model->setPathPrefix(get_path_property(menu->parent()), FilenameModel::Mode::Lister);
		menu->clear_selection(); // clear currentIndex after applying new model
		p_mouse_pos_timer->start(100);
	}
}
//
bool BreadcrumbsAddressBar::set_path(QString path, bool virtualFolder)
{
	qDebug() << "set_path" << path;

	// exit edit mode
	_cancel_edit();

	// stop loading mode
	ptr_bufferingMovie->stop();
	p_path_icon->setMovie({});
	p_path_icon->setToolTip({});
	//p_path_icon->setScaledContents(false);
	//p_path_icon->setMaximumSize(p_path_icon->width(), QWIDGETSIZE_MAX); 

	if (virtualFolder)
	{
		_clear_crumbs();
		m_path = path;
		p_line_address->setText(path);
		_insert_crumb(path);

		p_path_icon->setPixmap(get_icon(m_path).pixmap(16, 16));
		crumb_hide_show();

		emit path_selected(m_path);
	}
	else
	{
		path = QFileInfo(path).absoluteFilePath().replace("/", "\\");

		QFileInfo fi(path);
		if (path.isEmpty() || (! fi.exists()) || (! fi.isDir()))
		{
			emit path_error(path);
			return false;
		}
		QDir pathDir(path);
		if (! pathDir.isReadable())
		{
			emit listdir_error(path);
			return false;
		}

		_clear_crumbs();
		m_path = path;
		p_line_address->setText(path);
		_insert_crumb(path);
		
		while (! pathDir.isRoot())
		{
			pathDir.cdUp();
			_insert_crumb(pathDir.absolutePath().replace("/", "\\"));
			//pathDir.cdUp();
		}

		p_path_icon->setPixmap(get_icon(m_path).pixmap(16, 16));
		crumb_hide_show();

		emit path_selected(m_path);
	}

	return true;
}
//
void BreadcrumbsAddressBar::set_loading(const QString& path)
{
	p_path_icon->setToolTip(tr("Loading %1...").arg(path));
	//p_path_icon->setScaledContents(true);
	//p_path_icon->setMaximumSize(p_path_icon->width(), p_path_icon->width()); 
	p_path_icon->setPixmap({});
	p_path_icon->setMovie(ptr_bufferingMovie);
	ptr_bufferingMovie->start();
}
//
void BreadcrumbsAddressBar::requestPathChange(QString path)
{
	if (path.isEmpty() && sender())
		path = get_path_property(sender());

	path = QFileInfo(path).absoluteFilePath().replace("/", "\\");

	emit path_requested(path);
}
//
void BreadcrumbsAddressBar::_cancel_edit()
{
	qDebug() << "_cancel_edit";
	p_line_address->setText(path()); // revert path
	show_address_field(false); // switch back to breadcrumbs view
}
//
const QString& BreadcrumbsAddressBar::path() const
{
	return m_path;
}
//
void BreadcrumbsAddressBar::switch_space_mouse_up(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) // left click only
	{
		show_address_field(true);
	}
}
//
void BreadcrumbsAddressBar::show_address_field(bool show)
{
	this->setUpdatesEnabled(false);

	qDebug() << "show_address_field" << show;
	if (show)
	{
		p_crumbs_container->hide();
		p_line_address->show();
		p_line_address->setFocus();
		// Win32 / Qt interaction -> ensure the Qt window has focus
		// (explorer win32 window may already have it)
		SetFocus((HWND) p_line_address->window()->winId());
		p_line_address->selectAll();
	}
	else
	{
		p_line_address->hide();
		p_crumbs_container->show();
	}

	this->setUpdatesEnabled(true);
}
//
void BreadcrumbsAddressBar::crumb_hide_show()
{
	if (LeftHBoxLayout* pLayout = dynamic_cast<LeftHBoxLayout*>(p_crumbs_panel->layout()))
	{
		Qt::ArrowType arrow = (pLayout->count_hidden() > 0) ? Qt::LeftArrow : Qt::RightArrow;
		p_btn_root_crumb->setArrowType(arrow);
	}
}
//
QSize BreadcrumbsAddressBar::minimumSizeHint() const
{
	return QSize(150, p_line_address->height());
}
//
void BreadcrumbsAddressBar::mouse_pos_timer_event()
{
	QPoint pos = QCursor::pos();
	QToolButton* btn = dynamic_cast<QToolButton*>(qApp->widgetAt(pos));
	QWidget* active_menu = qApp->activePopupWidget();
	if (btn && btn != active_menu->parent() && 
		( (btn == p_btn_root_crumb) || (btn->parent() == p_crumbs_panel) ) )
	{
		active_menu->close();
		btn->showMenu();
	}
}
