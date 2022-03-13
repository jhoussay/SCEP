#pragma once
//
#include <QString>
#include <QIcon>
//
#include <vector>
#include <map>
#include <optional>
//
#include <shtypes.h>
//
class NavigationPathUtils;
//
/**
 *	@ingroup		SCEP
 *	@brief			Structure representing a path
 */
class NavigationPath
{
public:
	/**
	 *	@brief		Indicates whether the given filesystem path is absolute or not
	 */
	static bool		IsAbsolute(const QString& path);

public:
	/**
	 *	@brief		Create navigation path from a win32 absolute identifier list
	 *	@param pidlFolder	Absolute identifier list of the folder
	 *
	 *	This identifier list may reference a virtual folder or a real filesystem folder.
	 */
	NavigationPath(PCIDLIST_ABSOLUTE pidlFolder);
	/**
	 *	@brief		Create navigation path from string description
	 *	@param path	Path
	 *
	 *	The algorithm is :
	 *	- We check whether the path corresponds to a known virtual folder (we both check the GUID style and the label style).
	 *	- If it is a virtual folder, we keep the GUID style path as internal path and we are done.
	 *	- Else, we will try to consider the path as a filesystem path (non virtual).
	 * 
	 *	Note : on non virtual path, no existence or readability check is performed.
	 *
	 *	@see		isExistingDirectory(), isReadableDirectory()
	 */
	NavigationPath(QString path = QString());

public:
	/**
	 *	@brief		Returns the full win32 path
	 *
	 *	Two cases :
	 *	- real folder : returns the filesystem path
	 *	- virtual folder : returns the win32 internal path. Example : "::{F02C1A0D-BE21-4350-88B0-7367FC96EF3C}"
	 */
	const QString&	internalPath() const;
	/**
	 *	@brief		Returns the user display path
	 *
	 *	Two cases :
	 *	- real folder : returns the path : "translated" or internel (cf. @param translate)
	 *	- virtual folder : returns the display name, such as "Network" (or "Réseau" in French)
	 */
	QString			displayPath(bool translate = true) const;
	/**
	 *	@ingroup	SCEP
	 *	@brief		Returns the "translated" leaf of the path
	 *
	 *	Three cases :
	 *	- virtual folder : same as displayPath()
	 *	- existing real folder : translated leaf of the path.
	 *	  Example in French : "Desktop" -> "Bureau"
	 *	- non existing real folder : leaf of the path
	 */
	QString			label() const;

	
	/**
	 *	@brief		Indicates whether the path is empty or not
	 */
	bool			empty() const;
	/**
	 *	@brief		Indicates whether the path is valid or not : virtual folder or ABSOLUTE path
	 */
	bool			valid() const;

	/**
	 *	@brief		Indicates whether the path corresponds to an existing directory
	 *
	 *	Two cases :
	 *	- real folder : test whether the path corresponds to a filesystem directory and whether it exists.
	 *	- virtual folder : returns `true`
	 */
	bool			isExistingDirectory() const;
	/**
	 *	@brief		Indicates whether the path corresponds to a readable directory
	 *
	 *	Two cases :
	 *	- real folder : test whether the directory can be read (should be called after isExistingDirectory())
	 *	- virtual folder : returns `true`
	 */
	bool			isReadableDirectory() const;

	/**
	 *	@brief		Indicates whether the path has a parent
	 *
	 *	Two cases :
	 *	- real folder : the path has a parent if it is not a drive root.
	 *	- virtual folder : returns `false`
	 */
	bool			hasParent() const;
	/**
	 *	@brief		Returns the parent path, if it exists.
	 *	@see		hasParent()
	 */
	std::optional<NavigationPath> parent() const;

	/**
	 *	@brief		Returns the corresponding icon, if any
	 */
	QIcon			icon() const;
	/**
	 *	@brief		Returns the corresponding pixmap, if any
	 *	@param size	Requested size
	 */
	QPixmap			pixmap(const QSize size = {32, 23}) const;

	/**
	 *	@brief		Equality operator
	 */
	bool			operator ==(const NavigationPath& other) const;
	/**
	 *	@brief		Difference operator
	 */
	bool			operator !=(const NavigationPath& other) const;

private:
	QString			m_internalPath;				//!< Internal path :
	bool			m_virtualFolder = false;	//!< Virtual folder ?

	static NavigationPathUtils* p_utils;		//!< Utils
	static NavigationPathUtils*	GetUtils();		//!< Get or creates utils
};
//
//
//
/**
 *	@ingroup		SCEP
 *	@brief			Navigation history
 *
 *	The navigation is *never* empty : it can be empty only before the first navigation finishes, in the wrapper initialization function.
 *
 *	A typical navigation history should look like this :
 *	 - [0] path0
 *	 - [1] path1
 *	 - [2] path2
 *	 - [3] path3
 *	 - [4] path4 (*)
 *
 *	The (*) denotes the current index.
 * 
 *	If the user navigates backward three times, the navigation history is :
 *	 - [0] path0
 *	 - [1] path1 (*)
 *	 - [2] path2
 *	 - [3] path3
 *	 - [4] path4
 * 
 *	If the user navigates forward once, the navigation history is :
 *	 - [0] path0
 *	 - [1] path1
 *	 - [2] path2 (*)
 *	 - [3] path3
 *	 - [4] path4
 * 
 *	If the user navigates to another place, the navigation history is :
 *	 - [0] path0
 *	 - [1] path1
 *	 - [2] path2
 *	 - [3] path5 (*)
 */
class NavigationHistory
{
public:
	/**
	 *	@brief			Constructor
	 *	@param history	Navigation history
	 *	@param index	Current index in navigation history
	 */
	NavigationHistory(const std::vector<NavigationPath>& history = {}, int index = -1);

	/**
	 *	@brief			Indicates whether the navigation history is valid or not
	 */
	bool				valid() const;

	/**
	 *	@brief			Returns the backward navigation folder (if there is one).
	 */
	std::optional<NavigationPath>	hasBackward() const;
	/**
	 *	@brief			Returns the forward navigation folder (if there is one).
	 */
	std::optional<NavigationPath>	hasForward() const;

	/**
	 *	@brief			Navigates backward (if possible) and returns the backward navigation folder (if there is one).
	 *	@see			hasBackward()
	 */
	std::optional<NavigationPath>	navigateBackward();
	/**
	 *	@brief			Navigates forward (if possible) and returns the forward navigation folder (if there is one).
	 *	@see			hasForward()
	 */
	std::optional<NavigationPath>	navigateForward();
	/**
	 *	@brief			Navigates to the selected directory
	 */
	void				navigateTo(const NavigationPath& path);

private:
	std::vector<NavigationPath>		m_history;
	int								m_index = -1;
};

