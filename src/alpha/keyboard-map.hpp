/**
 * @file keyboard-map.hpp
 * @author exeal
 * @date 2003-2007
 */

#ifndef ALPHA_KEYBOARD_MAP_HPP
#define ALPHA_KEYBOARD_MAP_HPP

#include "resource.h"
#include "../manah/win32/windows.hpp"
#include <sstream>


namespace alpha {
	namespace command {
		class KeyAssignableCommand;

		/// An numeric identifier of a command.
		typedef DWORD CommandID;

		/// A virtual key code.
		typedef WORD VirtualKey;	// typeof(ACCEL::key)
		const VirtualKey VK_NULL = 0;	///< An invalid key.

		/// A modifier key.
		typedef uchar KeyModifier;
		const KeyModifier KM_SHIFT	= 0x01;	///< Shift key.
		const KeyModifier KM_CTRL	= 0x02;	///< Ctrl key.
		const KeyModifier KM_ALT	= 0x04;	///< Alt key.

		/// A key combination.
		struct KeyCombination {
			VirtualKey key;			///< The main key.
			KeyModifier modifiers;	///< The modifier key.
			/// Constructor.
			KeyCombination(VirtualKey k = VK_NULL, KeyModifier m = 0) : key(k), modifiers(m) {
				assert(key < 0x100);
				assert(modifiers <= (KM_SHIFT | KM_CTRL | KM_ALT));
			}
		};

		/// Manages keyboard assignments.
		class KeyboardMap {
		public:
			// constructors
			KeyboardMap();
			~KeyboardMap();
			// attributes
			KeyAssignableCommand*	getCommand(const KeyCombination& keys) const;
			KeyAssignableCommand*	getCommand(const KeyCombination& firstKeys, const KeyCombination& secondKeys) const;
			static std::wstring		getKeyName(VirtualKey key);
			std::wstring			getKeyString(CommandID id) const;
			static std::wstring		getStrokeString(const KeyCombination& keys);
			static std::wstring		getStrokeString(const KeyCombination& firstKeys, const KeyCombination& secondKeys);
			bool					isDirty() const;
			// operations
			bool	assign(const KeyAssignableCommand& command, const KeyCombination& keys);
			bool	assign(const KeyAssignableCommand& command, const KeyCombination& firstKeys, const KeyCombination& secondKeys);
			void	clear();
			bool	load(const WCHAR* fileName);
			void	unassign(const KeyCombination& keys);
			void	unassign(const KeyCombination& firstKeys, const KeyCombination& secondKeys);
			bool	save(const WCHAR* fileName);

		private:
			struct FirstKeyMap {
				KeyAssignableCommand* command;			// キーシーケンスが1ストロークで終わる場合はコマンド
				KeyAssignableCommand*** secondKeyMap;	// 2 ストローク使う場合は 8 * 0x0100 の配列
														// コマンドが割り当てられていない場合はいずれのメンバも null
			};
			FirstKeyMap firstKeyMaps_[8][0x0100];	// (修飾キー) * (仮想キーコード) でコマンド或いは第 2 キー組み合わせを格納。
													// 0 であれば何も登録されていない
			bool dirty_;
		};


		/**
		 * Returns the command assigned to the specified key combination.
		 * @param keys the keycombinations
		 * @return the command or @c null if no command is assigned. if the first stroke, the 
		 * @c BuiltInCommand corresponding to @c CMD_SPECIAL_WAITFOR2NDKEYS
		 */
		inline KeyAssignableCommand* KeyboardMap::getCommand(const KeyCombination& keys) const {
			return firstKeyMaps_[keys.modifiers][keys.key].command;}

		/**
		 * Returns the command assigned to the specified key combinations.
		 * @param firstKeys the first key combination
		 * @param secondKeys the second key combination
		 * @return the command or @c null if no command is assigned
		 */
		inline KeyAssignableCommand* KeyboardMap::getCommand(
				const KeyCombination& firstKeys, const KeyCombination& secondKeys) const {
			const FirstKeyMap& firstKeyMap = firstKeyMaps_[firstKeys.modifiers][firstKeys.key];
			if(firstKeyMap.secondKeyMap == 0)
				return 0;
			return (firstKeyMap.secondKeyMap[secondKeys.modifiers] == 0) ?
				0 : firstKeyMap.secondKeyMap[secondKeys.modifiers][secondKeys.key];
		}

		/**
		 * Returns the name of the virtual key.
		 * @param key the virtual key code
		 * @return the key name or an empty string if @a key is invalid
		 */
		inline std::wstring KeyboardMap::getKeyName(VirtualKey key) {
			UINT c = ::MapVirtualKeyW(key, 0) << 16;
			switch(key) {
			case VK_INSERT:	case VK_DELETE:	case VK_HOME:	case VK_END:
			case VK_NEXT:	case VK_PRIOR:	case VK_LEFT:	case VK_RIGHT:
			case VK_UP:		case VK_DOWN:
				c |= (1 << 24);
			}
			wchar_t buffer[256];
			return (::GetKeyNameTextW(c | (1 << 25), buffer, countof(buffer)) != 0) ? buffer : L"";
		}

		/// Returns the string expresses the specified key combinations.
		inline std::wstring KeyboardMap::getStrokeString(const KeyCombination& keys) {
			std::wostringstream	s;
			if(toBoolean(keys.modifiers & KM_CTRL))		s << L"Ctrl+";
			if(toBoolean(keys.modifiers & KM_SHIFT))	s << L"Shift+";
			if(toBoolean(keys.modifiers & KM_ALT))		s << L"Alt+";
			s << getKeyName(keys.key);
			return s.str();
		}

		/// Returns the string expresses the specified key combinations.
		inline std::wstring KeyboardMap::getStrokeString(
				const KeyCombination& firstKeys, const KeyCombination& secondKeys) {
			return getStrokeString(KeyCombination(firstKeys.key, firstKeys.modifiers))
				+ L" " + getStrokeString(KeyCombination(secondKeys.key, secondKeys.modifiers));
		}

		/// Returns true if the keyboard map is not saved.
		inline bool KeyboardMap::isDirty() const {return dirty_;}

}} // namespace alpha::command

#endif /* !ALPHA_KEYBOARD_MAP_HPP */
