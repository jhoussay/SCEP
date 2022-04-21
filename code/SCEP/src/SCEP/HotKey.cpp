#include <SCEP/HotKey.h>
#include <SCEP/win32_utils.h>
//
#include <QtDebug>
//
extern "C"
LRESULT CALLBACK KeyboardProc(	int		code,
								WPARAM	wParam,
								LPARAM	lParam	)
{
	bool ok = HotKeyManager::instance()->processKeyBoardEvent(wParam, lParam);

	if ( (! ok) || (code < 0) )
	{
		return CallNextHookEx(	HHOOK(),
								code,
								wParam,
								lParam	);
	}
	else
	{
		// On interrompt la chaine
		return 1;
	}
}
//
bool operator <(const HotKey& key1, const HotKey& key2)
{
	if (key1.key < key2.key)
		return true;
	else if (key1.key > key2.key)
		return false;
	else
		return (key1.modifiers < key2.modifiers);
}
//
//
//
HotKeyManager* HotKeyManager::p_instance = NULL;
//
HotKeyManager::HotKeyManager()
	:	QObject()
{}
//
HotKeyManager::~HotKeyManager()
{
	stopListenning();
}
//
HotKeyManager* HotKeyManager::createInstance()
{
	deleteInstance();
	p_instance = new HotKeyManager();
	return p_instance;
}
//
HotKeyManager* HotKeyManager::instance()
{
	return p_instance;
}
//
void HotKeyManager::deleteInstance()
{
	delete  p_instance;
	p_instance  =   NULL;
}
//
bool HotKeyManager::registerHotKey(const HotKey& hotKey, int id)
{
	if (m_ids.find(id) != m_ids.end())
	{
		qWarning() << "Already registered id " << id;
		return false;
	}
	else
	{
		m_ids.insert(id);
		m_hotKeys[hotKey] = id;

		return true;
	}
}
//
bool HotKeyManager::registerHotKey(	Qt::Key					key,
									Qt::KeyboardModifiers	modifiers,
									int						id	)
{
	return registerHotKey({key, modifiers}, id);
}
//
bool HotKeyManager::unregisterHotKey(int id)
{
	if (m_ids.find(id) == m_ids.end())
	{
		qWarning() << "Unregistered id " << id;
		return false;
	}
	else
	{
		std::map<HotKey, int>::iterator iteHotKeys;
		for (iteHotKeys = m_hotKeys.begin(); iteHotKeys != m_hotKeys.end(); iteHotKeys++)
		{
			if (iteHotKeys->second == id)
				break;
		}
		if (iteHotKeys != m_hotKeys.end())
			m_hotKeys.erase(iteHotKeys);

		m_ids.erase(id);

		return true;
	}
}
//
void HotKeyManager::startListenning()
{
	stopListenning();

	// Trying to handle shortcuts
	m_hook = SetWindowsHookEx(WH_KEYBOARD, &KeyboardProc, nullptr, GetCurrentThreadId());
	if (m_hook == nullptr)
	{
		qCritical() << "Error calling SetWindowsHookEx: " << GetLastErrorAsString();
	}
	
}
void HotKeyManager::stopListenning()
{
	if (m_hook != nullptr)
	{
		UnhookWindowsHookEx(m_hook);
		m_hook = nullptr;
	}
}
//
inline bool isDown(DWORD vkey)
{
	//static constexpr SHORT KEYSTATE_MASK = 0x01;
	static constexpr SHORT KEYSTATE_MASK = (SHORT) (USHORT) 0x8000;
	return GetAsyncKeyState(vkey) & KEYSTATE_MASK;
}
//
bool HotKeyManager::processKeyBoardEvent(WPARAM wParam, LPARAM lParam)
{
	// Current key
	//////////////

	bool keyPressed = (HIWORD(lParam) & KF_UP) == 0;

	bool repeatFlag = (HIWORD(lParam) & KF_REPEAT) == KF_REPEAT;
	if (keyPressed && repeatFlag)
		return false;

	DWORD vkey = wParam;
	Qt::Key qkey;
	switch (vkey)
	{
	case 0x30:
		qkey = Qt::Key_0;
		break;
	case 0x31:
		qkey = Qt::Key_1;
		break;
	case 0x32:
		qkey = Qt::Key_2;
		break;
	case 0x33:
		qkey = Qt::Key_3;
		break;
	case 0x34:
		qkey = Qt::Key_4;
		break;
	case 0x35:
		qkey = Qt::Key_5;
		break;
	case 0x36:
		qkey = Qt::Key_6;
		break;
	case 0x37:
		qkey = Qt::Key_7;
		break;
	case 0x38:
		qkey = Qt::Key_8;
		break;
	case 0x39:
		qkey = Qt::Key_9;
		break;
	case 0x41:
		qkey = Qt::Key_A;
		break;
	case 0x42:
		qkey = Qt::Key_B;
		break;
	case 0x43:
		qkey = Qt::Key_C;
		break;
	case 0x44:
		qkey = Qt::Key_D;
		break;
	case 0x45:
		qkey = Qt::Key_E;
		break;
	case 0x46:
		qkey = Qt::Key_F;
		break;
	case 0x47:
		qkey = Qt::Key_G;
		break;
	case 0x48:
		qkey = Qt::Key_H;
		break;
	case 0x49:
		qkey = Qt::Key_I;
		break;
	case 0x4A:
		qkey = Qt::Key_J;
		break;
	case 0x4B:
		qkey = Qt::Key_K;
		break;
	case 0x4C:
		qkey = Qt::Key_L;
		break;
	case 0x4D:
		qkey = Qt::Key_M;
		break;
	case 0x4E:
		qkey = Qt::Key_N;
		break;
	case 0x4F:
		qkey = Qt::Key_O;
		break;
	case 0x50:
		qkey = Qt::Key_P;
		break;
	case 0x51:
		qkey = Qt::Key_Q;
		break;
	case 0x52:
		qkey = Qt::Key_R;
		break;
	case 0x53:
		qkey = Qt::Key_S;
		break;
	case 0x54:
		qkey = Qt::Key_T;
		break;
	case 0x55:
		qkey = Qt::Key_U;
		break;
	case 0x56:
		qkey = Qt::Key_V;
		break;
	case 0x57:
		qkey = Qt::Key_W;
		break;
	case 0x58:
		qkey = Qt::Key_X;
		break;
	case 0x59:
		qkey = Qt::Key_Y;
		break;
	case 0x5A:
		qkey = Qt::Key_Z;
		break;
	case VK_F1:
		qkey = Qt::Key_F1;
		break;
	case VK_F2:
		qkey = Qt::Key_F2;
		break;
	case VK_F3:
		qkey = Qt::Key_F3;
		break;
	case VK_F4:
		qkey = Qt::Key_F4;
		break;
	case VK_F5:
		qkey = Qt::Key_F5;
		break;
	case VK_F6:
		qkey = Qt::Key_F6;
		break;
	case VK_F7:
		qkey = Qt::Key_F7;
		break;
	case VK_F8:
		qkey = Qt::Key_F8;
		break;
	case VK_F9:
		qkey = Qt::Key_F9;
		break;
	case VK_F10:
		qkey = Qt::Key_F10;
		break;
	case VK_F11:
		qkey = Qt::Key_F11;
		break;
	case VK_F12:
		qkey = Qt::Key_F12;
		break;
	//case VK_SHIFT:
	//case VK_LSHIFT:
	//case VK_RSHIFT:
	//	qkey = Qt::Key_Shift;
	//	break;
	//case VK_CONTROL:
	//case VK_LCONTROL:
	//case VK_RCONTROL:
	//	qkey = Qt::Key_Control;
	//	break;
	//case VK_MENU:
	//	qkey = Qt::Key_Alt;
	//	break;
	case VK_LEFT:
		qkey = Qt::Key_Left;
		break;
	case VK_RIGHT:
		qkey = Qt::Key_Right;
		break;
	case VK_UP:
		qkey = Qt::Key_Up;
		break;
	case VK_DOWN:
		qkey = Qt::Key_Down;
		break;
	case VK_TAB:
		qkey = Qt::Key_Tab;
		break;
	case VK_DELETE:
		qkey = Qt::Key_Delete;
		break;
	case VK_BACK:
		qkey = Qt::Key_Backspace;
		break;
	default:
		return false;
	}
	//qDebug().noquote() << "key " << qkey << " is " << (keyPressed ? "down" : "up");

	// Update current key status
	Key key = {qkey, vkey};
	if (keyPressed)
	{
		if (m_currentKeys.find(key) == m_currentKeys.end())
			m_currentKeys.insert(key);
	}
	else
	{
		m_currentKeys.erase(key);
	}


	// Key up ?
	// Done !!
	///////////
	
	if (! keyPressed)
		return false;


	// Previous keys
	////////////////

	// Update previous keys status
	std::set<Key> keysToRemove;
	for (const Key& key : m_currentKeys)
	{
		if (! isDown(key.vkey))
		{
			keysToRemove.insert(key);
		}
	}
	for (const Key& key : keysToRemove)
	{
		//qDebug().noquote() << "key " << key.qkey << " is not down any more !";
		m_currentKeys.erase(key);
	}


	// Hot key
	//////////

	if (m_currentKeys.size() == 1)
	{
		// Current modifiers
		Qt::KeyboardModifiers currentModifiers;
		// Alt
		bool altDown = isDown(VK_MENU);
#ifdef _DEBUG
		bool altDown2 = (HIWORD(lParam) & KF_ALTDOWN) == KF_ALTDOWN;
		assert(altDown == altDown2);
#endif
		if (altDown)
			currentModifiers |= Qt::AltModifier;
		// Shift
		bool shiftDown = isDown(VK_SHIFT) || isDown(VK_LSHIFT) || isDown(VK_RSHIFT);
		if (shiftDown)
			currentModifiers |= Qt::ShiftModifier;
		// Control
		bool controlDown = isDown(VK_CONTROL) || isDown(VK_LCONTROL) || isDown(VK_RCONTROL);
		if (controlDown)
			currentModifiers |= Qt::ControlModifier;
		//qDebug().noquote() << "	modifiers are " << currentModifiers;


		// HotKey
		/////////

		HotKey qtKey;
		qtKey.key		= m_currentKeys.begin()->qkey;
		//qtKey.modifiers	= m_currentModifiers;
		qtKey.modifiers	= currentModifiers;

		auto ite = m_hotKeys.find(qtKey);
		if (ite != m_hotKeys.end())
		{
			int shortcut = ite->second;
			emit hotKeyPressed(shortcut);

			// We do not return true because we do not want to break the hook chain
		}
	}

	return false;
}
//
