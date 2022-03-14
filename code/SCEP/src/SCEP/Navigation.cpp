﻿#include <SCEP/Navigation.h>
#include <SCEP/Error.h>
//
#include <QFileInfo>
#include <QFileIconProvider>
#include <QDir>
#include <QPainter>
#include <QStorageInfo>
#include <QStandardPaths>
//
#include <shlobj_core.h>
#include <shobjidl_core.h>
//
struct KnownFolder
{
	QString							internalName;			//!< File system path (real folder) or GUID (virtual folder)
	QString							displayName;			//!< Display path
	bool							virtualFolder = false;	//!< Virtual folder or not

	inline KnownFolder(const QString& intName = {}, const QString& dispName = {}, bool virtFolder = false)
	{
		internalName = intName;
		displayName = dispName;
		virtualFolder = virtFolder;
		qDebug() << QString("new KnownFolder(%1, %2, %3)").arg(internalName).arg(displayName).arg(virtualFolder);
	}
};
//
enum class MainFolderId
{
	OneDrive,	//!< OneDrive
	Desktop,	//!< Desktop
	Home,		//!< Home
	Libraries,	//!< Libraries
	Network,	//!< Network
	RecycleBin	//!< Recycle bin
};
//

//
struct MainFolder
{
	MainFolderId					id;		//!< Id
	QString							kfName;	//!< Known folder name
	std::optional<NavigationPath>	path;	//!< Main folder path
};
//
using MainFolders = std::vector<MainFolder>;
//
class NavigationPathUtils
{
private:
	NavigationPaths				m_drivesPaths;			//!< Drives paths

	NavigationPaths				m_networkLocationsPaths;//!< Network locations paths

	std::map<QString, KnownFolder> m_knownFolders;		//!< Known folders
	NavigationPaths				m_knownFoldersPaths;	//!< Known folders paths

	//< Main folders
	MainFolders					m_mainFolders =
	{
		{ MainFolderId::OneDrive,	"OneDrive",				std::nullopt },
		{ MainFolderId::Desktop,	"ThisPCDesktopFolder",	std::nullopt },
		{ MainFolderId::Home,		"UsersFilesFolder",		std::nullopt },
		{ MainFolderId::Libraries,	"UsersLibrariesFolder",	std::nullopt },
		{ MainFolderId::Network,	"NetworkPlacesFolder",	std::nullopt },
		{ MainFolderId::RecycleBin,	"RecycleBinFolder",		std::nullopt },
	};
	NavigationPaths				m_mainFoldersPaths;		//!< Main folders paths

	QFileIconProvider			m_iconProvider;			//!< Icon provider

public:
	NavigationPathUtils() = default;

	ErrorPtr init()
	{
		// Clear
		////////

		m_drivesPaths.clear();
		m_networkLocationsPaths.clear();
		m_mainFoldersPaths.clear();
		m_knownFolders.clear();
		m_knownFoldersPaths.clear();


		// Drives
		/////////

		for (QStorageInfo i : QStorageInfo::mountedVolumes())
		{
			m_drivesPaths.push_back({NavigationPath(i.rootPath())});
		}


		// Network locations
		////////////////////

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
				m_networkLocationsPaths.push_back(NavigationPath(path));
		}


		// Known folders
		////////////////

		// Create an IKnownFolderManager instance
		IKnownFolderManager* pkfm = nullptr;
		HRESULT hr = CoCreateInstance(CLSID_KnownFolderManager, nullptr,  CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pkfm));
		if (SUCCEEDED(hr))
		{
			KNOWNFOLDERID *rgKFIDs = nullptr;
			UINT cKFIDs = 0;
			// Get the IDs of all known folders
			hr = pkfm->GetFolderIds(&rgKFIDs, &cKFIDs);
			if (SUCCEEDED(hr))
			{
				IKnownFolder *pkfCurrent = nullptr;
				// Enumerate the known folders. rgKFIDs[i] has the KNOWNFOLDERID
				for (UINT i = 0; i < cKFIDs; ++i)
				{
					hr = pkfm->GetFolder(rgKFIDs[i], &pkfCurrent);
					if (SUCCEEDED(hr))
					{
						// Get the non-localized, canonical name for the known 
						// folder from KNOWNFOLDER_DEFINITION
						KNOWNFOLDER_DEFINITION kfd;
						hr = pkfCurrent->GetFolderDefinition(&kfd);
						if (SUCCEEDED(hr))
						{
							// Known folder name
							QString kfName = QString::fromWCharArray(kfd.pszName);

							// Get the translated name and the win32 path
							QString displayName, internalName;
							PIDLIST_ABSOLUTE pidl;
							hr = pkfCurrent->GetIDList(0, &pidl);
							if (SUCCEEDED(hr))
							{
								PWSTR pszName = nullptr;
								hr = SHGetNameFromIDList(pidl, SIGDN_NORMALDISPLAY, &pszName);
								if (SUCCEEDED(hr))
								{
									displayName = QString::fromWCharArray(pszName);
									CoTaskMemFree(pszName);
								}

								hr = SHGetNameFromIDList(pidl, SIGDN_DESKTOPABSOLUTEPARSING, &pszName);
								if (SUCCEEDED(hr))
								{
									internalName = QString::fromWCharArray(pszName);
									CoTaskMemFree(pszName);
								}

#if 0
								if (kfName.startsWith("Music"))
								{
									std::map<SIGDN, QString> sigdns = 
									{
										{ SIGDN_NORMALDISPLAY, "SIGDN_NORMALDISPLAY" },
										{ SIGDN_PARENTRELATIVEPARSING, "SIGDN_PARENTRELATIVEPARSING" },
										{ SIGDN_DESKTOPABSOLUTEPARSING, "SIGDN_DESKTOPABSOLUTEPARSING" },
										{ SIGDN_PARENTRELATIVEEDITING, "SIGDN_PARENTRELATIVEEDITING" },
										{ SIGDN_DESKTOPABSOLUTEEDITING, "SIGDN_DESKTOPABSOLUTEEDITING" },
										{ SIGDN_FILESYSPATH, "SIGDN_FILESYSPATH" },
										{ SIGDN_URL, "SIGDN_URL" },
										{ SIGDN_PARENTRELATIVEFORADDRESSBAR, "SIGDN_PARENTRELATIVEFORADDRESSBAR" },
										{ SIGDN_PARENTRELATIVE, "SIGDN_PARENTRELATIVE" },
										{ SIGDN_PARENTRELATIVEFORUI, "SIGDN_PARENTRELATIVEFORUI" }
									};
									qDebug() << "SHGetNameFromIDList :";
									for (auto [sigdn, sigdn_str] : sigdns)
									{
										PWSTR pszName = nullptr;
										HRESULT hr = SHGetNameFromIDList(pidl, sigdn, &pszName);
										if (SUCCEEDED(hr))
										{
											qDebug() << "	" << sigdn_str << " -> " << QString::fromWCharArray(pszName);
											CoTaskMemFree(pszName);
										}
										else
										{
											qDebug() << "	" << sigdn_str << " -> FAILED !";
										}
									}
								}
#endif

								CoTaskMemFree(pidl);
							}

							// If ok, add to known folders
							if ( (! internalName.isEmpty()) && (! displayName.isEmpty()) )
							{
								qDebug() << kfName;

								// Some know folders (like "Music") look like "::{GUID}\\path"
								// but are not declared as virtual.
								// However, we need to consider them this way !
								m_knownFolders[kfName] = {internalName, displayName, kfd.category == KF_CATEGORY_VIRTUAL || internalName.startsWith("::{")};

								m_knownFoldersPaths.emplace_back(internalName);
							}

							FreeKnownFolderDefinitionFields(&kfd);
						}
					}
					pkfCurrent->Release();
				}
				CoTaskMemFree(rgKFIDs);
			}
			pkfm->Release();
		}


		// Main folders
		///////////////

		for (MainFolder& mainFolder : m_mainFolders)
		{
			auto ite = m_knownFolders.find(mainFolder.kfName);
			if (ite != m_knownFolders.end())
			{
				NavigationPath path(ite->second.internalName);

				mainFolder.path = path;
				m_mainFoldersPaths.push_back(path);
			}
			else
			{
				qWarning() << "Could not get known folder " << mainFolder.kfName;
			}
		}


		return success();
	}

	inline const NavigationPaths&			drivesPaths() const
	{
		return m_drivesPaths;
	}

	inline const NavigationPaths&			networkLocationsPaths() const
	{
		return m_networkLocationsPaths;
	}

	inline const NavigationPaths&			knownFoldersPaths() const
	{
		return m_knownFoldersPaths;
	}

	inline std::optional<KnownFolder>		knownFolder(const QString& path)
	{
		for (const auto& [kfName, knownFolder]  : m_knownFolders)
		{
			if ( (path.toLower() == knownFolder.internalName.toLower()) || (path.toLower() == knownFolder.displayName.toLower()) )
			{
				return knownFolder;
			}
		}
		return std::nullopt;
	}

	inline const NavigationPaths&			mainFoldersPaths() const
	{
		return m_mainFoldersPaths;
	}

	inline const QFileIconProvider&			iconProvider() const
	{
		return m_iconProvider;
	}
};
//
//
//
NavigationPathUtils* NavigationPath::p_utils = nullptr;
//
NavigationPathUtils* NavigationPath::GetUtils()
{
	if (p_utils == nullptr)
	{
		p_utils = new NavigationPathUtils();
		if (ErrorPtr pError = p_utils->init())
		{
			qCritical() << "Unable to initialize NavigationPathUtils : " << *pError;
			delete p_utils;
			p_utils = nullptr;
		}
	}
	return p_utils;
}
//
bool NavigationPath::IsAbsolute(const QString& path)
{
	QFileInfo fi(path);
	// QFileInfo::isAbsolute returns true for some invalid pathes
	return (path.contains("/") || path.contains("\\")) && (! path.startsWith("::{")) && fi.isAbsolute();
}
//
const NavigationPaths& NavigationPath::Drives()
{
	if (NavigationPathUtils* pUtils = GetUtils())
	{
		return pUtils->drivesPaths();
	}
	else
	{
		qCritical() << "NavigationPath::Drives: No utils instance";

		static NavigationPaths InvalidNavigationPaths;
		return InvalidNavigationPaths;
	}
}
//
const NavigationPaths& NavigationPath::NetworkLocations()
{
	if (NavigationPathUtils* pUtils = GetUtils())
	{
		return pUtils->networkLocationsPaths();
	}
	else
	{
		qCritical() << "NavigationPath::NetworkLocations: No utils instance";

		static NavigationPaths InvalidNavigationPaths;
		return InvalidNavigationPaths;
	}
}
//
const NavigationPaths& NavigationPath::KnownFolders()
{
	if (NavigationPathUtils* pUtils = GetUtils())
	{
		return pUtils->knownFoldersPaths();
	}
	else
	{
		qCritical() << "NavigationPath::KnownFolders: No utils instance";

		static NavigationPaths InvalidNavigationPaths;
		return InvalidNavigationPaths;
	}
}
//
const NavigationPaths& NavigationPath::MainFolders()
{
	if (NavigationPathUtils* pUtils = GetUtils())
	{
		return pUtils->mainFoldersPaths();
	}
	else
	{
		qCritical() << "NavigationPath::MainFolders: No utils instance";

		static NavigationPaths InvalidNavigationPaths;
		return InvalidNavigationPaths;
	}
}
//
NavigationPath::NavigationPath(PCIDLIST_ABSOLUTE pidlFolder)
{
#if 0
	// DEBUG
	std::map<SIGDN, QString> sigdns = 
	{
		{ SIGDN_NORMALDISPLAY, "SIGDN_NORMALDISPLAY" },
		{ SIGDN_PARENTRELATIVEPARSING, "SIGDN_PARENTRELATIVEPARSING" },
		{ SIGDN_DESKTOPABSOLUTEPARSING, "SIGDN_DESKTOPABSOLUTEPARSING" },
		{ SIGDN_PARENTRELATIVEEDITING, "SIGDN_PARENTRELATIVEEDITING" },
		{ SIGDN_DESKTOPABSOLUTEEDITING, "SIGDN_DESKTOPABSOLUTEEDITING" },
		{ SIGDN_FILESYSPATH, "SIGDN_FILESYSPATH" },
		{ SIGDN_URL, "SIGDN_URL" },
		{ SIGDN_PARENTRELATIVEFORADDRESSBAR, "SIGDN_PARENTRELATIVEFORADDRESSBAR" },
		{ SIGDN_PARENTRELATIVE, "SIGDN_PARENTRELATIVE" },
		{ SIGDN_PARENTRELATIVEFORUI, "SIGDN_PARENTRELATIVEFORUI" }
	};
	qDebug() << "SHGetNameFromIDList :";
	for (auto [sigdn, sigdn_str] : sigdns)
	{
		PWSTR pszName = nullptr;
		HRESULT hr = SHGetNameFromIDList(pidlFolder, sigdn, &pszName);
		if (SUCCEEDED(hr))
		{
			qDebug() << "	" << sigdn_str << "	" << " -> " << QString::fromWCharArray(pszName);
			CoTaskMemFree(pszName);
		}
		else
		{
			qDebug() << "	" << sigdn_str << " -> FAILED !";
		}
	}
#endif

	PWSTR pszName = nullptr;

	// Trying to get the file path
	HRESULT hr = SHGetNameFromIDList(pidlFolder, SIGDN_FILESYSPATH, &pszName);
	if (SUCCEEDED(hr))
	{
		m_virtualFolder = false;
		m_internalPath = QString::fromWCharArray(pszName);
		CoTaskMemFree(pszName);
	}
	// Defaulting to the normal display
	// example : "Network" virtual folder
	else
	{
		hr = SHGetNameFromIDList(pidlFolder, SIGDN_DESKTOPABSOLUTEPARSING/*SIGDN_NORMALDISPLAY*/, &pszName);
		if (SUCCEEDED(hr))
		{
			if (NavigationPathUtils* pUtils = GetUtils())
			{
				std::optional<KnownFolder> knownFolder = pUtils->knownFolder(QString::fromWCharArray(pszName));
				CoTaskMemFree(pszName);
				if (knownFolder.has_value()) // Found it ?
				{
					m_internalPath = knownFolder.value().internalName;
					m_virtualFolder = knownFolder.value().virtualFolder;
					return;
				}
				else
				{
					qCritical() << "Could not create NavigationPath from absolute item list (1).";
				}
			}
			else
			{
				qCritical() << "NavigationPath::NavigationPath: No utils instance";
			}
		}
		else
		{
			qCritical() << "Could not create NavigationPath from absolute item list (2).";
		}
	}
}
//
NavigationPath::NavigationPath(QString path)
{
	path.replace("/", "\\");

	if (! path.isEmpty())
	{
		// Absolute path ?
		if (IsAbsolute(path))
		{
			m_internalPath = path;
			m_virtualFolder = false;
			return;
		}

		// Test versus known folders
		if (NavigationPathUtils* pUtils = GetUtils())
		{
			std::optional<KnownFolder> knownFolder = pUtils->knownFolder(path);
			if (knownFolder.has_value()) // Found it ?
			{
				m_internalPath = knownFolder.value().internalName;
				m_virtualFolder = knownFolder.value().virtualFolder;
				return;
			}
		}
		else
		{
			qWarning() << "NavigationPath : could not check virtual folders";
		}

		// Fallback : relative (and thus invalid !) filesystem path
		m_internalPath = path;
		m_virtualFolder = false;
	}
}
//
const QString& NavigationPath::internalPath() const
{
	return m_internalPath;
}
//
QString NavigationPath::displayPath() const
{
	if (m_virtualFolder)
	{
		// Test versus known folders
		if (NavigationPathUtils* pUtils = GetUtils())
		{
			std::optional<KnownFolder> knownFolder = pUtils->knownFolder(m_internalPath);
			assert(knownFolder.has_value());
			if (knownFolder.has_value())
			{
				return knownFolder.value().displayName;
			}
			else
			{
				qCritical() << "NavigationPath::displayPath: unknown virtual folder";
				return m_internalPath; // ouch !
			}
		}
		else
		{
			qCritical() << "NavigationPath::displayPath: No utils instance";
			return m_internalPath; // ouch !
		}
	}
	else
	{
		return m_internalPath;
	}
}
//
QString NavigationPath::label() const
{
	if (m_virtualFolder)
	{
		return displayPath();
	}
	else
	{
		std::wstring wpath = m_internalPath.toStdWString();

		ITEMIDLIST* idlist;
		HRESULT hr = SHParseDisplayName(wpath.c_str(), 0, &idlist, 0, 0);
		if (! SUCCEEDED(hr))
		{
			QString label = QFileInfo(m_internalPath).fileName();
			qDebug() << "Could not get path label for " << m_internalPath << ", defaulting to file name : " << label;
			return label;
		}

#if 0
		std::map<SIGDN, QString> sigdns = 
		{
			{ SIGDN_NORMALDISPLAY, "SIGDN_NORMALDISPLAY" },
			{ SIGDN_PARENTRELATIVEPARSING, "SIGDN_PARENTRELATIVEPARSING" },
			{ SIGDN_DESKTOPABSOLUTEPARSING, "SIGDN_DESKTOPABSOLUTEPARSING" },
			{ SIGDN_PARENTRELATIVEEDITING, "SIGDN_PARENTRELATIVEEDITING" },
			{ SIGDN_DESKTOPABSOLUTEEDITING, "SIGDN_DESKTOPABSOLUTEEDITING" },
			{ SIGDN_FILESYSPATH, "SIGDN_FILESYSPATH" },
			{ SIGDN_URL, "SIGDN_URL" },
			{ SIGDN_PARENTRELATIVEFORADDRESSBAR, "SIGDN_PARENTRELATIVEFORADDRESSBAR" },
			{ SIGDN_PARENTRELATIVE, "SIGDN_PARENTRELATIVE" },
			{ SIGDN_PARENTRELATIVEFORUI, "SIGDN_PARENTRELATIVEFORUI" }
		};
		qDebug() << "SHGetNameFromIDList :";
		for (auto [sigdn, sigdn_str] : sigdns)
		{
			PWSTR pszName = nullptr;
			HRESULT hr = SHGetNameFromIDList(idlist, sigdn, &pszName);
			if (SUCCEEDED(hr))
			{
				qDebug() << "	" << sigdn_str << " -> " << QString::fromWCharArray(pszName);
				CoTaskMemFree(pszName);
			}
			else
			{
				qDebug() << "	" << sigdn_str << " -> FAILED !";
			}
		}
#endif

		PWSTR pszName;
		hr = SHGetNameFromIDList(idlist, SIGDN_PARENTRELATIVEEDITING, &pszName);
		
		QString name = pszName && SUCCEEDED(hr) ? QString::fromWCharArray(pszName) : QString();

		CoTaskMemFree(idlist);
		CoTaskMemFree(pszName);

		// Special case : drive !
		if ( (m_internalPath.size() == 3) && (m_internalPath.endsWith(":\\")) )
		{
			return QString("%1 (%2)").arg(name).arg(m_internalPath.mid(0, 2));
		}
		// All other cases
		else
		{
			return name;
		}
	}
}
//
bool NavigationPath::empty() const
{
	return m_internalPath.isEmpty();
}
//
bool NavigationPath::valid() const
{
	if (m_virtualFolder)
		return true;
	else
	{
		return IsAbsolute(m_internalPath);
	}
}
//
bool NavigationPath::isExistingDirectory() const
{
	if (m_virtualFolder)
	{
		return true;
	}
	else
	{
		QFileInfo fi(m_internalPath);
		return (! m_internalPath.isEmpty()) && fi.exists() && fi.isDir();
	}
}
//
bool NavigationPath::isReadableDirectory() const
{
	if (m_virtualFolder)
	{
		return true;
	}
	else
	{
		return QDir(m_internalPath).isReadable();
	}
}
//
bool NavigationPath::hasParent() const
{
	// TODO Handle parent folder for virtual folder ?
	return (! m_virtualFolder) && (! QFileInfo(m_internalPath).isRoot());
}
//
std::optional<NavigationPath> NavigationPath::parent() const
{
	if (hasParent())
	{
		// TODO Handle parent folder for virtual folder ?
		return NavigationPath(QFileInfo(m_internalPath).absolutePath().replace("/", "\\"));
	}
	else
	{
		return {};
	}
}
//
QIcon NavigationPath::icon() const
{
	if (NavigationPathUtils* pUtils = GetUtils())
	{
		if (m_virtualFolder)
		{
			std::wstring wpath = m_internalPath.toStdWString();

			PIDLIST_ABSOLUTE pidl = nullptr;
			HRESULT hr = SHParseDisplayName(wpath.c_str(), 0, &pidl, 0, 0);
			if (SUCCEEDED(hr))
			{
				SHFILEINFO info;
				unsigned int flags = SHGFI_ICON | SHGFI_SMALLICON | SHGFI_SYSICONINDEX | SHGFI_ADDOVERLAYS | SHGFI_OVERLAYINDEX /*| SHGFI_USEFILEATTRIBUTES*/;
				hr = SHGetFileInfo((LPCWSTR) pidl, -1, &info, sizeof(SHFILEINFO), SHGFI_PIDL | flags);
				CoTaskMemFree(pidl);
				if (SUCCEEDED(hr))
				{
					QPixmap pixmap = QPixmap::fromImage(QImage::fromHICON(info.hIcon));
					DestroyIcon(info.hIcon);
					return QIcon(pixmap);
				}
			}
			return {};
		}
		else
		{
			static QSize TranspIconSize = {40, 40}; // px, size of generated semi-transparent icons

			QFileInfo fi(m_internalPath);
			QIcon ico = pUtils->iconProvider().icon(fi);
			if (fi.isHidden())
			{
				QPixmap pmap(TranspIconSize);
				pmap.fill(Qt::transparent);
				QPainter painter(&pmap);
				painter.setOpacity(0.5);
				ico.paint(&painter, 0, 0, TranspIconSize.width(), TranspIconSize.height());
				painter.end();
				ico = QIcon(pmap);
			}
			return ico;
		}
	}
	else
	{
		qCritical() << "NavigationPath::icon() : No utils instance";
		return {};
	}
}
//
QPixmap NavigationPath::pixmap(const QSize size) const
{
	return icon().pixmap(size);
}
//
NavigationPath NavigationPath::childPath(const QString& childName) const
{
	if (! empty())
	{
		if (m_virtualFolder)
		{
			// TODO Can we handle this ?
			return m_internalPath + "\\" + childName;
		}
		else
		{
			QString path = m_internalPath;
			if (! path.endsWith("\\"))
				path += "\\";
			path += childName;
			return path;
		}
	}
	else
	{
		return {};
	}
}
//
bool NavigationPath::operator ==(const NavigationPath& other) const
{
	return (m_internalPath == other.m_internalPath) && (m_virtualFolder == other.m_virtualFolder);
}
//
bool NavigationPath::operator !=(const NavigationPath& other) const
{
	return ! operator ==(other);
}
//
//
//
NavigationHistory::NavigationHistory(const NavigationPaths& history, int index)
	:	m_history(history)
	,	m_index(index)
{
	if (! history.empty())
	{
		assert(valid());
	}
}
//
bool NavigationHistory::valid() const
{
	return (! m_history.empty()) && (m_index >= 0) && (m_index < (int) m_history.size());
}
//
std::optional<NavigationPath> NavigationHistory::hasBackward() const
{
	if (valid() && (m_index > 0))
	{
		return m_history[m_index-1];
	}
	else
	{
		return std::nullopt;
	}
}
//
std::optional<NavigationPath> NavigationHistory::hasForward() const
{
	if (valid() && (m_index < (int) m_history.size()-1))
	{
		return m_history[m_index+1];
	}
	else
	{
		return std::nullopt;
	}
}
//
std::optional<NavigationPath> NavigationHistory::navigateBackward()
{
	if (valid() && (m_index > 0))
	{
		m_index--;
		return m_history[m_index];
	}
	else
	{
		return std::nullopt;
	}
}
//
std::optional<NavigationPath> NavigationHistory::navigateForward()
{
	if (valid() && (m_index < (int) m_history.size()-1))
	{
		m_index++;
		return m_history[m_index];
	}
	else
	{
		return std::nullopt;
	}
}
//
void NavigationHistory::navigateTo(const NavigationPath& path)
{
	m_index = m_history.empty() ? 0 : m_index + 1;
	m_history.resize(m_index+1);
	m_history[m_index] = path;
}
//
