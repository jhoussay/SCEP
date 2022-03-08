/**
 *	Ported to C++ and adapted from Andrey Makarov's breadcrumbsaddressbar, 2019, MIT license
 *	https://github.com/Winand/breadcrumbsaddressbar
 */

#include <SCEP/BreadcrumbsAddressBar/ModelViews.h>
#include <SCEP/win32_utils.h>
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
FilenameModel::FilenameModel(QWidget* parent, Filter filter, IconProviderFn icon_provider)
	:	QStringListModel(parent)
{
	m_current_path = std::nullopt;
	m_filter = filter;
	m_icon_provider = icon_provider;
}
//
QVariant FilenameModel::data(const QModelIndex &index, int role) const
{
	QVariant rslt = QStringListModel::data(index, role);
	if ( (role == Qt::DecorationRole) && m_icon_provider)
	{
		// self.setData(index, dat, role)
		return m_icon_provider(QStringListModel::data(index, Qt::DisplayRole).toString());
	}
	if (role == Qt::DisplayRole)
	{
		//return getPathLabel(rslt.toString());//QFileInfo(rslt.toString()).fileName();
		return QFileInfo(rslt.toString()).fileName();
	}
	return rslt;
}
//
QStringList FilenameModel::get_file_list(const QString& path) const
{
	QDir qdir(path);
	QDir::Filters filters = QDir::NoDotAndDotDot /*| QDir::Hidden*/ |
		(m_filter == Filter::Dirs ? QDir::Dirs : QDir::AllEntries);
	QDir::SortFlags sortFlags = QDir::DirsFirst | QDir::LocaleAware;
	QStringList names = qdir.entryList(filters, sortFlags);

	// Pathes do not have a final separator ("C:\Windows"), except drives ("C:\")
	QString separator = "\\";
	if (path.endsWith("/") || path.endsWith("\\"))
		separator = "";

	//// Translate and sort again
	//for (QString& name : names)
	//{
	//	name = getPathLabel(path + separator + name);
	//}
	//names.sort();

	QStringList lst;
	lst.reserve(names.size());
	for (const QString& name : names)
	{
		lst.push_back(path + separator + name);
	}
	return lst;
}
//
void FilenameModel::setPathPrefix(const QString& prefix)
{
	QFileInfo fi(prefix);
	QString path = fi.canonicalPath().replace("/", "\\");
	if ( path.isEmpty() || (! fi.isAbsolute()) || (! fi.exists()) || (! fi.isDir()) )
	{
		qDebug() << "Reject " << prefix;

		// invalid, wrong path or no drive letter
		setStringList({});
		return;
	}
	else if (path == m_current_path.value_or(""))
	{
		// already listed
		return;
	}
	setStringList(get_file_list(path));
	m_current_path = path;
}
//
//
//
ListView::ListView(MenuListView* pMenuListView)
	:	QListView(pMenuListView)
	,	ptr_menuListView(pMenuListView)
{
	assert(ptr_menuListView);

	setFrameShape(QFrame::NoFrame);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	QPalette pal = palette();
	// TODO JHO self.palette().color(pal.Window)) with self = MenuListView
	pal.setColor(QPalette::Base, pal.color(pal.Window));
	setPalette(pal);

	setMouseTracking(true); // receive mouse move events
	setFocusPolicy(Qt::NoFocus); // no focus rect
}
//
QSize ListView::sizeHint() const
{
	return ptr_menuListView ? ptr_menuListView->size_hint() : QSize();
}
//
QSize ListView::minimumSizeHint() const
{
	return ptr_menuListView ? ptr_menuListView->size_hint() : QSize();
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
MenuListView::MenuListView(QWidget* parent)
	:	QMenu(parent)
{
	p_listview = new ListView(this);

	QWidgetAction* act_wgt = new QWidgetAction(this);
	act_wgt->setDefaultWidget(p_listview);
	addAction(act_wgt);

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