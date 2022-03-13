#include <SCEP/Navigation.h>
#include <SCEP/Error.h>
//
#include <QFileInfo>
#include <QFileIconProvider>
#include <QDir>
#include <QPainter>
//
#include <shlobj_core.h>
#include <shobjidl_core.h>
//
struct KnownFolder
{
	QString						internalName;			//!< File system path (real folder) or GUID (virtual folder)
	QString						displayName;			//!< Display path
	bool						virtualFolder = false;	//!< Virtual folder or not

	inline KnownFolder(const QString& intName, const QString& dispName, bool virtFolder)
	{
		internalName = intName;
		displayName = dispName;
		virtualFolder = virtFolder;
		qDebug() << QString("new KnownFolder(%1, %2, %3)").arg(internalName).arg(displayName).arg(virtualFolder);
	}
};
//
class NavigationPathUtils
{
private:
	std::vector<KnownFolder>	m_knownFolders;			//!< Known folders
	QFileIconProvider			m_iconProvider;			//!< Icon provider

public:
	NavigationPathUtils() = default;

	ErrorPtr init()
	{
		m_knownFolders.clear();

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
								if (QString::fromWCharArray(kfd.pszName).startsWith("Music"))
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
								// Some know folders (like "Music") look like "::{GUID}\\path"
								// but are not declared as virtual.
								// However, we need to consider them this way !
								m_knownFolders.emplace_back(internalName, displayName, kfd.category == KF_CATEGORY_VIRTUAL || internalName.startsWith("::{"));
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

		return success();
	}

	inline std::vector<KnownFolder>&	knownFolders()
	{
		return m_knownFolders;
	}

	inline std::optional<KnownFolder>	knownFolder(const QString& path)
	{
		for (const KnownFolder& knownFolder : knownFolders())
		{
			if ( (path == knownFolder.internalName) || (path == knownFolder.displayName) )
			{
				return knownFolder;
			}
		}
		return std::nullopt;
	}

	inline const QFileIconProvider&		iconProvider() const
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
NavigationPath::NavigationPath(PCIDLIST_ABSOLUTE pidlFolder)
{
#if 1
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
QString NavigationPath::displayPath(bool translate) const
{
	if (m_virtualFolder)
	{
		// Test versus known folders
		if (NavigationPathUtils* pUtils = GetUtils())
		{
			std::optional<KnownFolder> knownFolder = pUtils->knownFolder(m_internalPath);
			assert(knownFolder.has_value());
			if (knownFolder.has_value())
				return knownFolder.value().displayName;
			else
				return m_internalPath; // ouch !
		}
		else
		{
			qCritical() << "NavigationPath::displayPath: No utils instance";
			return m_internalPath; // ouch !
		}
	}
	else
	{
		if (translate)
		{
			// TODO Generate translated full path !
			// TODO Return m_internalPath if translation fails
			return m_internalPath;
		}
		else
		{
			return m_internalPath;
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
			// TODO ok for virtual folders ?
			QFileInfo fi(m_internalPath);
			return pUtils->iconProvider().icon(fi);
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
QString NavigationPath::label() const
{
	if (m_virtualFolder)
	{
		return displayPath();
	}
	else
	{
		QString nativePath = m_internalPath;
		std::wstring wpath = nativePath.toStdWString();

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
		hr = SHGetNameFromIDList(idlist, SIGDN_PARENTRELATIVEFORADDRESSBAR, &pszName);
		
		QString name = pszName && SUCCEEDED(hr) ? QString::fromWCharArray(pszName) : QString();

		CoTaskMemFree(idlist);
		CoTaskMemFree(pszName);

		return name;
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
NavigationHistory::NavigationHistory(const std::vector<NavigationPath>& history, int index)
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
