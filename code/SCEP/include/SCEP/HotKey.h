#pragma once
//
#include <QObject>
//
#include <set>
#include <map>
//
#include <windows.h>
//
/** 
 *	@brief						Hot key structure
 */
struct HotKey
{
	Qt::Key						key;				//!< Key
	Qt::KeyboardModifiers		modifiers;			//!< Modifiers
};
//
/** 
 *	@brief						HotKey comparison operator
 */
bool operator <(const HotKey& lhs, const HotKey& rhs);
//
/**
 *	@brief						Hot keys manager
 */
class HotKeyManager : public QObject
{
	Q_OBJECT

private:
	struct Key
	{
		Qt::Key	qkey;	//!< Qt key
		DWORD	vkey;	//!< Virtual key

		bool operator <(const Key& other) const
		{
			return vkey < other.vkey;
		}
	};

	static HotKeyManager*	p_instance;				//!< Singleton instance
	std::set<int>			m_ids;					//!< Registered hot keys ids
	std::map<HotKey, int >	m_hotKeys;				//!< Registered hot keys
	HHOOK					m_hook = nullptr;		//!< Hook

	std::set<Key>			m_currentKeys;			//!< Current keys
	//Qt::KeyboardModifiers	m_currentModifiers;		//!< Current modifiers

private:
	/**
	 *	@brief					Default constructor
	 */
	HotKeyManager();
	/**
	 *	@brief					Destructorr
	 */
	virtual ~HotKeyManager();

public:
	/**
	 *	@brief					Create and return singleton
	 */
	static HotKeyManager*		createInstance();
	/**
	 *	@brief					Return singleton
	 *  @return					Return nullptr if createInstance has not been called yey
	 */
	static HotKeyManager*		instance();
	/**
	 *	@brief					Destroy singleton
	 */
	static void					deleteInstance();

	/**
	 *	@brief					Register new hot key
	 *	@param hotKey			Hot key
	 *	@param id				Hot key identifier
	 *	@return					Return true for successful operation
	 */
	bool						registerHotKey(const HotKey& hotKey, int id);
	/**
	 *	@brief					Register new hot key
	 *	@param key				Key
	 *	@param modifiers		Modifiers
	 *	@param id				Hot key identifier
	 *	@return					Return true for successful operation
	 */
	bool						registerHotKey(	Qt::Key						key,
												Qt::KeyboardModifiers		modifiers,
												int							id	);
	/**
	 *	@brief					Unregister hot key
	 *	@param id				Hot key identifier
	 *	@return					Return true for successful operation
	 */
	bool						unregisterHotKey(int id);

	/**
	 *  @brief					Start listenning hoy keys
	 */
	void						startListenning();
	/**
	 *  @brief					Stop listenning hoy keys
	 */
	void						stopListenning();

	/**
	 *	@brief					Process a keyboard event
	 *	@return					Return true si the event has been caught
	 */
	bool						processKeyBoardEvent(WPARAM wParam, LPARAM lParam);

signals:
	/**
	 *	@brief					Signal emitted when a hot key is detected
	 *	@param id				Hot key identifier
	 */
	void						hotKeyPressed(int id);
};
