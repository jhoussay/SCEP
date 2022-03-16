/**
 *	Ported to C++ and adapted from Andrey Makarov's breadcrumbsaddressbar, 2019, MIT license
 *	https://github.com/Winand/breadcrumbsaddressbar
 */

#include <SCEP/BreadcrumbsAddressBar/ModelViews.h>
#include <SCEP/win32_utils.h>
#include <SCEP/Theme.h>
#include <SCEP/Navigation.h>
//
#include <QFileInfo>
#include <QDir>
#include <QWidgetAction>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QToolButton>
#include <QtDebug>
//
FilenameModel::FilenameModel(QWidget* parent)
	:	QAbstractListModel(parent)
{
	m_currentPath = {};
	m_fullPath = {};
}
//
int FilenameModel::rowCount(const QModelIndex& /*parent*/) const
{
	return (int) m_items.size();
}
//
QVariant FilenameModel::data(const QModelIndex &index, int role) const
{
	if ( (index.row() >= 0) && (index.row() < (int) m_items.size()) )
	{
		const Item& item = m_items[index.row()];
		if (role == Qt::DisplayRole)
			return item.label;
		else if (role == Qt::EditRole)
		{
			QString rslt;
			if (! m_currentPath.empty())
			{
				// Completer : We need to return the input path to match what the
				// user typed in the address line edit
				rslt = m_currentPath.inputPath();
				if (! rslt.endsWith("\\"))
					rslt += "\\";
			}
			rslt += item.label;
			return rslt;
		}
		else if (role == PathRole)
			return item.path.internalPath();
		else if (role == Qt::DecorationRole)
			return item.path.pixmap(QSize(16, 16));
		else if (role == Qt::FontRole)
		{
			if (! m_fullPath.empty())
			{
				QFont font;
				if (m_fullPath.internalPath().startsWith(item.path.internalPath()))
				{
					font.setBold(true);
				}
				return font;
			}
		}
	}
	return {};
}
//
void FilenameModel::setCurrentPath(NavigationPath currentPath, const NavigationPath& fullPath, Mode mode)
{
	if (! NavigationPath::IsAbsolute(currentPath.internalPath()))
	{
		//if ( (! m_currentPath.empty()) || (m_items.size() == 0) )
		{
			beginResetModel();
			m_currentPath = {};
			m_fullPath = {};
			m_items = rootItems();
			endResetModel();
		}
	}
	else if (currentPath.isExistingDirectory())
	{
		if (currentPath != m_currentPath)
		{
			beginResetModel();
			m_currentPath = currentPath;
			m_fullPath = fullPath;
			m_items = GetItems(m_currentPath, mode);
			endResetModel();
		}
	}
}
//
bool lowerItem(const FilenameModel::Item& lhs, const FilenameModel::Item& rhs)
{
	return lhs.label.toLower() < rhs.label.toLower();
}
//
FilenameModel::Items FilenameModel::GetItems(const NavigationPath& path, Mode mode)
{
	bool dirsOnly = (mode == Mode::Lister);

	// Enumerate path entries
	// TODO Propose hidden items ?
	QDir dir(path.internalPath() + "\\");
	QDir::Filters filters = QDir::NoDotAndDotDot /*| QDir::Hidden*/ |
		(dirsOnly ? QDir::Dirs : QDir::AllEntries);
	QDir::SortFlags sortFlags = QDir::DirsFirst | QDir::LocaleAware;
	QStringList names = dir.entryList(filters, sortFlags);

	// Create items
	Items items;
	items.reserve(names.size());
	for (const QString& name : names)
	{
		NavigationPath child_path = path.childPath(name);
		items.push_back({child_path, child_path.label()});
	}

	// Sort items per label
	std::sort(items.begin(), items.end(), lowerItem);

	return items;
}
//
FilenameModel::Items FilenameModel::rootItems() const
{
	const NavigationPaths& drives = NavigationPath::Drives();
	const NavigationPaths& mainFolders = NavigationPath::MainFolders();

	Items items;
	items.reserve(drives.size() + mainFolders.size());
	for (const NavigationPath& drive : drives)
		items.push_back({drive, drive.internalPath()}); // raw drive names
	for (const NavigationPath& mainFolder : mainFolders)
		items.push_back({mainFolder, mainFolder.label()});

	// Sort items per label
	std::sort(items.begin(), items.end(), lowerItem);

	return items;
}
//
//
//
ListView::ListView(MenuListView* pMenuListView, Theme* ptrTheme)
	:	QListView(pMenuListView)
	,	ptr_menuListView(pMenuListView)
{
	assert(ptr_menuListView);

	setFrameShape(QFrame::NoFrame);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	setMouseTracking(true); // receive mouse move events
	setFocusPolicy(Qt::NoFocus); // no focus rect
}
//
QSize ListView::sizeHint() const
{
	if (ptr_menuListView)
	{
		static constexpr int BorderWidth = 0;

		QSize size = ptr_menuListView->size_hint();
		return QSize(size.width()+BorderWidth, size.height()+BorderWidth);
	}
	else
	{
		return QSize();
	}
}
//
QSize ListView::minimumSizeHint() const
{
	return sizeHint();
}
//
void ListView::mousePressEvent(QMouseEvent *event)
{
	if (ptr_menuListView)
		ptr_menuListView->mouse_press_event(event);
}
//
void ListView::mouseMoveEvent(QMouseEvent *event)
{
	if (ptr_menuListView)
		ptr_menuListView->update_current_index(event);
}
//
void ListView::leaveEvent(QEvent *event)
{
	if (ptr_menuListView)
		ptr_menuListView->clear_selection(event);
}
//
void ListView::mouseReleaseEvent(QMouseEvent *event)
{
	if (ptr_menuListView)
		ptr_menuListView->mouse_release_event(event);
}
//
void ListView::keyPressEvent(QKeyEvent *event)
{
	if (ptr_menuListView)
		ptr_menuListView->key_press_event(event);
}
//
//
//
MenuListView::MenuListView(QWidget* parent, Theme* ptrTheme)
	:	QMenu(parent)
{
	p_listview = new ListView(this, ptrTheme);

	QWidgetAction* act_wgt = new QWidgetAction(this);
	act_wgt->setDefaultWidget(p_listview);
	addAction(act_wgt);
	// Resolving some dark style problems
	setStyleSheet("QMenu { background: palette(base); }");

	connect(p_listview, &ListView::activated, this, &MenuListView::activated);
	connect(p_listview, &ListView::clicked, this, &MenuListView::clicked);

	p_listview->setFocus();

	m_last_index = QModelIndex(); // selected index
	m_flag_mouse_l_pressed = false;
}
//
void MenuListView::setModel(QAbstractItemModel *model)
{
	if (p_listview)
		p_listview->setModel(model);
}
//
void MenuListView::key_press_event(QKeyEvent *event)
{
	int key = event->key();
	if ( (key == Qt::Key_Return) || ( key == Qt::Key_Enter) )
	{
		if (m_last_index.isValid())
			emit activated(m_last_index);
		close();
	}
	else if (key == Qt::Key_Escape)
	{
		close();
	}
	else if ( (key == Qt::Key_Down) || ( key == Qt::Key_Up) )
	{
		if (p_listview)
		{
			QAbstractItemModel* model = p_listview->model();
			int row_from = 0;
			int row_to = model->rowCount()-1;
			if (key == Qt::Key_Down)
			{
				std::swap(row_from, row_to);
			}
			QModelIndex index;
			if ( (m_last_index.row() == -1) || (m_last_index.row() == row_from) ) // no index=-1
			{
				index = model->index(row_to, 0);
			}
			else
			{
				int shift = (key == Qt::Key_Down) ? 1 : -1;
				index = model->index(m_last_index.row() + shift, 0);
			}
			p_listview->setCurrentIndex(index);
			m_last_index = index;
		}
	}
}
//
void MenuListView::update_current_index(QMouseEvent* event)
{
	if (p_listview)
	{
		m_last_index = p_listview->indexAt(event->pos());
		p_listview->setCurrentIndex(m_last_index);
	}
}
//
void MenuListView::clear_selection(QEvent* /*event*/)
{
	if (p_listview)
	{
		p_listview->clearSelection();
		// selectionModel()->clear() leaves selected item in Fusion theme
		p_listview->setCurrentIndex(QModelIndex());
		m_last_index = QModelIndex();
	}
}
//
void MenuListView::mouse_press_event(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_flag_mouse_l_pressed = true;
		update_current_index(event);
	}
}
//
void MenuListView::mouse_release_event(QMouseEvent *event)
{
	if ( (event->button() == Qt::LeftButton) && m_flag_mouse_l_pressed )
	{
		m_flag_mouse_l_pressed = false;
		if (m_last_index.isValid())
			emit clicked(m_last_index);
		close();
	}
}
//
QSize MenuListView::size_hint() const
{
	if (p_listview)
	{
		int width = p_listview->sizeHintForColumn(0);
		width += p_listview->verticalScrollBar()->sizeHint().width();
		if (dynamic_cast<QToolButton*>(parentWidget()))
			width = std::max(width, parentWidget()->width());
		int visible_rows = std::min(Max_visible_items, p_listview->model()->rowCount());
		return QSize(width, visible_rows * p_listview->sizeHintForRow(0));
	}
	else
	{
		return {};
	}
}
//
bool MenuListView::event(QEvent *e)
{
	// TODO move menu
	//if (e->type() == QEvent::Show)
	//	move(parent()->mapToGlobal(QtCore.QPoint(0,0))-QtCore.QPoint(0,self.height()))
	return QMenu::event(e);
}
//
