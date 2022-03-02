/**
 *	Ported to C++ and adapted from Andrey Makarov's breadcrumbsaddressbar, 2019, MIT license
 *	https://github.com/Winand/breadcrumbsaddressbar
 */

#include <SCEP/BreadcrumbAdressBar/Layouts.h>
//
#include <QWidget>
#include <QTimer>
//
LeftHBoxLayout::LeftHBoxLayout(QWidget* pParent, double minimal_space)
	:	QHBoxLayout(pParent)
	,	m_first_visible(0)
{
	set_space_widget();
	set_minimal_space(minimal_space);
}
//
void LeftHBoxLayout::set_space_widget(QWidget* pWidget, int stretch)
{
	QHBoxLayout::takeAt(count());
	if (pWidget)
		QHBoxLayout::addWidget(pWidget, stretch);
	else
		addStretch(stretch);
}
//
QWidget* LeftHBoxLayout::space_widget()
{
	return (*this)[count()]->widget();
}
//
void LeftHBoxLayout::setGeometry(const QRect& rc)
{
	QHBoxLayout::setGeometry(rc); // perform the layout
	int min_sp = minimal_space();
	if (min_sp < 1) // percent
		min_sp *= rc.width();
	int free_space = (*this)[count()]->geometry().width() - min_sp;
	if ( (free_space <= 0) && (count_visible() > 1) ) // hide more items
	{
		QWidget* widget = (*this)[m_first_visible]->widget();
		widget->hide();
		m_first_visible += 1;
		emit widget_state_changed();
	}
	else if ( (free_space > 0) && (count_hidden()) ) // show more items
	{
		QWidget* widget = (*this)[m_first_visible-1]->widget();
		int w_width = widget->width() + spacing();
		if (w_width <= free_space) // enough space to show next item
		{
			// setGeometry is called after show
			QTimer::singleShot(0, widget, SLOT(show()));
			m_first_visible -= 1;
			emit widget_state_changed();
		}
	}
}
//
int LeftHBoxLayout::count_visible() const
{
	return count(Visibility::Visible);
}
//
int LeftHBoxLayout::count_hidden() const
{
	return count(Visibility::Hidden);
}
//
QSize LeftHBoxLayout::minimumSize() const
{
	QMargins margins = contentsMargins();
	return QSize(margins.left() + margins.right(),
		margins.top() + 24 + margins.bottom());
}
//
void LeftHBoxLayout::addWidget(QWidget *widget, int stretch, Qt::Alignment alignment)
{
	// widget->setMinimumSize(widget->minimumSizeHint())  # FIXME:
	QHBoxLayout::insertWidget(count(), widget, stretch, alignment);
}
//
int LeftHBoxLayout::count(Visibility visibility) const
{
	int cnt = QHBoxLayout::count() - 1; // w/o last stretchable item
	switch (visibility)
	{
	case Visibility::All:
		return cnt;
	case Visibility::Visible:
		return cnt - m_first_visible;
	case Visibility::Hidden:
		return m_first_visible; // hidden items
	default:
		return 0;
	}
}
//
std::vector<QWidget*> LeftHBoxLayout::widgets(Visibility visibility)
{
	int begin = (visibility == Visibility::Visible) ? m_first_visible : 0;
	int end = (visibility == Visibility::Hidden) ? m_first_visible : count();

	std::vector<QWidget*> rslt;
	rslt.reserve(end-begin);

	for (int i = begin; i < end; i++)
		rslt.push_back((*this)[i]->widget());
	return rslt;
}
//
void LeftHBoxLayout::set_minimal_space(int value)
{
	m_minimal_space = value;
	invalidate();
}
//
int LeftHBoxLayout::minimal_space() const
{
	return m_minimal_space;
}
//
QLayoutItem* LeftHBoxLayout::operator [](int index)
{
	if (index < 0)
		index = count() + index;
	return itemAt(index);
}
//
QLayoutItem* LeftHBoxLayout::takeAt(int index)
{
	if (index < m_first_visible)
		m_first_visible -= 1;
	QLayoutItem* item = QHBoxLayout::takeAt(index);
	emit widget_state_changed();
	return item;
}
//
