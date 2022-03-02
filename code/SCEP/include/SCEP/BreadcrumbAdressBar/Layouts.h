/**
 *	Ported to C++ and adapted from Andrey Makarov's breadcrumbsaddressbar, 2019, MIT license
 *	https://github.com/Winand/breadcrumbsaddressbar
 */

#pragma once

#include <QHBoxLayout>

/**
 *	@ingroup				SCEP
 *
 *	From BreadcrumbsAddressBar
 *
 *	Left aligned horizontal layout.
	Hides items similar to Windows Explorer address bar.
 */
class LeftHBoxLayout : public QHBoxLayout
{
	Q_OBJECT

public:
	/**
	*	@brief				Enumeration on visibility mode
	*/
	enum Visibility
	{
		Visible,
		Hidden,
		All
	};

public:
	LeftHBoxLayout(QWidget* pParent = nullptr, double minimal_space = 0.1);

	/**
	 *	@brief				Set widget to be used to fill empty space to the right
	 *						If `widget`=None the stretch item is used (by default)
	 */
	void					set_space_widget(QWidget* pWidget = nullptr, int stretch=1);
	/**
	 *	@brief				Widget used to fill free space
	 */
	QWidget*				space_widget();
	
	/**
	 *	@brief				
	 *	@param rc			Layout's rectangle w/o margins
	 */
	void					setGeometry(const QRect& rc) override;

	/**
	*	@brief				Count of visible widgets
	*/
	int						count_visible() const;
	/**
	 *	@brief				Count of hidden widgets
	 */
	int						count_hidden() const;

	/**
	 *	@brief
	 */
	QSize					minimumSize() const override;

	/**
	 *	@brief				Append widget to layout, make its width fixed
	 *
	 *	CAUTION : has the same name in the super class but the method is NOT declared virtual !
	 */
	void					addWidget(QWidget *widget, int stretch = 0, Qt::Alignment alignment = Qt::Alignment());

	/**
	 *	@brief				Count of items in layout: `visible`=True|False(hidden)|None(all)
	 *
	 *	CAUTION : has the same name (but no arguments) in the super class but the method is NOT declared virtual !
	 */
	int						count(Visibility visibility = Visibility::All) const;

	/**
	 *	@brief				Iterate over child widgets
	 */
	std::vector<QWidget*>	widgets(Visibility visibility = Visibility::All);

	/**
	 *	@brief				Set minimal size of space area to the right:
	 *						 - [0.0, 1.0) - % of the full width
	*						 - [1, ...) - size in pixels
	 */
	void					set_minimal_space(int value);
	/**
	 *	@see				set_minimal_space
	 */
	int						minimal_space() const;

	/**
	 *	@brief				`itemAt` slices wrapper
	 */
	QLayoutItem*			operator [](int index);

	/**
	 *	@brief				Return an item at the specified `index` and remove it from layout
	 */
	QLayoutItem*			takeAt(int index) override;

signals:
	/**
	*	@brief				Signal is emitted when an item is hidden/shown or removed with `takeAt`
	*/
	void					widget_state_changed();

private:
	int						m_first_visible = 0;
	int						m_minimal_space = 0.1;
};
