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

		/// コマンドの識別子 (整数)
		typedef WORD CommandID;	// typeof(ACCEL::cmd)

		/// 仮想キーコードの型
		typedef WORD VirtualKey;	// typeof(ACCEL::key)
		const VirtualKey VK_NULL = 0;	///< 無効なキー

		/// 仮想キーの修飾キー
		typedef uchar KeyModifier;
		const KeyModifier KM_SHIFT	= 0x01;	///< Shift キー
		const KeyModifier KM_CTRL	= 0x02;	///< Ctrl キー
		const KeyModifier KM_ALT	= 0x04;	///< Alt キー

		/// 1ストロークのキー組み合わせ
		struct KeyCombination {
			VirtualKey key;			///< キー
			KeyModifier modifiers;	///< 修飾キー

			/// コンストラクタ
			KeyCombination(VirtualKey k = VK_NULL, KeyModifier m = 0) : key(k), modifiers(m) {
				assert(key < 0x100);
				assert(modifiers <= (KM_SHIFT | KM_CTRL | KM_ALT));
			}
		};

		/// キーバインドの管理
		class KeyboardMap {
		public:
			// コンストラクタ
			KeyboardMap();
			~KeyboardMap();
			// 属性
			KeyAssignableCommand*	getCommand(const KeyCombination& keys) const;
			KeyAssignableCommand*	getCommand(const KeyCombination& firstKeys, const KeyCombination& secondKeys) const;
			static const wchar_t*	getKeyName(VirtualKey key, bool shortName);
			std::wstring			getKeyString(CommandID id, bool shortName) const;
			static std::wstring		getStrokeString(const KeyCombination& keys, bool shortName);
			static std::wstring		getStrokeString(const KeyCombination& firstKeys, const KeyCombination& secondKeys, bool shortName);
			bool					isDirty() const;
			// 操作
			bool	assign(const KeyAssignableCommand& command, const KeyCombination& keys);
			bool	assign(const KeyAssignableCommand& command, const KeyCombination& firstKeys, const KeyCombination& secondKeys);
			void	clear();
			bool	load(const WCHAR* fileName);
			void	unassign(const KeyCombination& keys);
			void	unassign(const KeyCombination& firstKeys, const KeyCombination& secondKeys);
			bool	save(const WCHAR* fileName);

		private:
			struct FirstKeyMap {
				KeyAssignableCommand*	command;		// キーシーケンスが1ストロークで終わる場合はコマンド
				KeyAssignableCommand***	secondKeyMap;	// 2ストローク使う場合は 8 * 0x0100 の配列
														// コマンドが割り当てられていない場合はいずれのメンバも null
			};
			FirstKeyMap firstKeyMaps_[8][0x0100];	// (修飾キー) * (仮想キーコード) でコマンド或いは第2キー組み合わせを格納。
													// 0であれば何も登録されていない
			bool dirty_;
		};

		namespace {
			// キーの名前 (割り当て不能なキーは null)
			const wchar_t* const LONG_KEY_NAMES[0x0100] = {
			/* 0x00 */	0, 0, 0, L"Break", 0, 0, 0,	0,
						L"Backspace", L"Tab", 0, 0, L"Delete", L"Enter", 0, 0,
						L"Shift", L"Control", L"Alt", 0, 0, 0, 0, 0,
						0, 0, 0, L"Esc", 0, 0, 0, 0,
			/* 0x20 */	L"Space", L"Page Up", L"Page Down", L"End", L"Home", L"Left", L"Up", L"Right",
						L"Down", 0, 0, 0, 0, L"Insert", L"Delete", 0,
						L"0", L"1", L"2", L"3", L"4", L"5", L"6", L"7",
						L"8", L"9", 0, 0, 0, 0, 0, 0,
			/* 0x40 */	0, L"A", L"B", L"C", L"D", L"E", L"F", L"G",
						L"H", L"I", L"J", L"K", L"L", L"M", L"N", L"O",
						L"P", L"Q", L"R", L"S", L"T", L"U", L"V", L"W",
						L"X", L"Y", L"Z", L"Left Windows", L"Right Windows", L"Application", 0, 0,
			/* 0x60 */	L"Num 0", L"Num 1", L"Num 2", L"Num 3", L"Num 4", L"Num 5", L"Num 6", L"Num 7",
						L"Num 8", L"Num 9", L"Num *", L"Num +", L"|", L"Num -", L"Num .", L"Num /",
						L"F1", L"F2", L"F3", L"F4", L"F5", L"F6", L"F7", L"F8",
						L"F9", L"F10", L"F11", L"F12", L"F13", L"F14", L"F15", L"F16",
			/* 0x80 */	L"F17", L"F18", L"F19", L"F20", L"F21", L"F22", L"F23", L"F24",
						0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0,
			/* 0xA0 */	L"Left Shift", L"RightShift", L"Left Control", L"Right Control", L"Left Alt", L"Right Alt", 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, L":", L";", L",", L"-", L".", L"/",
			/* 0xC0 */	L"@", 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0,	0, 0, 0, 0,
						0, 0, 0, 0,	0, 0, 0, 0,
						0, 0, 0, L"[", L"\\", L"]", L"^", 0,
			/* 0xE0 */	0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0
			};
			const wchar_t* const SHORT_KEY_NAMES[0x0100] = {
			/* 0x00 */	0, 0, 0, L"Break", 0, 0, 0, 0,
						L"BS", L"TAB", 0, 0, L"Delete", L"RET", 0, 0,
						L"Shift", L"Ctrl", L"Alt", 0, 0, 0, 0, 0,
						0, 0, 0, L"ESC", 0, 0, 0, 0,
			/* 0x20 */	L"SPC", L"PageUp", L"PageDown", L"End", L"Home", L"Left", L"Up", L"Right",
						L"Down", 0, 0, 0, 0, L"Insert", L"Delete", 0,
						L"0", L"1", L"2", L"3", L"4", L"5", L"6", L"7",
						L"8", L"9", 0, 0, 0, 0, 0, 0,
			/* 0x40 */	0, L"a", L"b", L"c", L"d", L"e", L"f", L"g",
						L"h", L"i", L"j", L"k", L"l", L"m", L"n", L"o",
						L"p", L"q", L"r", L"s", L"t", L"u", L"v", L"w",
						L"x", L"y", L"z", L"LWIN", L"RWIN",	L"APP", 0, 0,
			/* 0x60 */	L"NUMPAD 0", L"NUMPAD 1", L"NUMPAD 2", L"NUMPAD 3", L"NUMPAD 4", L"NUMPAD 5", L"NUMPAD 6", L"NUMPAD 7",
						L"NUMPAD 8", L"NUMPAD 9", L"NUMPAD *", L"NUMPAD +", L"|", L"NUMPAD -", L"NUMPAD .", L"NUMPAD /",
						L"F1", L"F2", L"F3", L"F4", L"F5", L"F6", L"F7", L"F8",
						L"F9", L"F10", L"F11", L"F12", L"F13", L"F14", L"F15", L"F16",
			/* 0x80 */	L"F17", L"F18", L"F19", L"F20", L"F21", L"F22", L"F23", L"F24",
						0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0,
			/* 0xA0 */	L"LShift", L"RShift", L"LCtrl", L"RCtrl", L"LAlt", L"RAlt", 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, L":", L";", L",", L"-", L".", L"/",
			/* 0xC0 */	L"@", 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0,	0, 0, 0, 0,
						0, 0, 0, 0,	0, 0, 0, 0,
						0, 0, 0, L"[", L"\\", L"]", L"^", 0,
			/* 0xE0 */	0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0
			};
		}


		/**
		 * キー組み合わせからコマンドを取得
		 * @param keys キー組み合わせ
		 * @return 割り当てられているコマンド。割り当てられていないときは @c null。
		 * 2ストロークの1ストローク目であれば @c CMD_SPECIAL_WAITFOR2NDKEYS に相当する @c BuiltInCommand
		 */
		inline KeyAssignableCommand* KeyboardMap::getCommand(const KeyCombination& keys) const {
			return firstKeyMaps_[keys.modifiers][keys.key].command;}

		/**
		 * 2ストロークのキー組み合わせからコマンドを取得
		 * @param firstKeys 第1のキー組み合わせ
		 * @param secondKeys 第2のキー組み合わせ
		 * @return 割り当てられているコマンド。割り当てられていないときは @c null
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
		 * キーの名前を返す
		 * @param key 仮想キー
		 * @param shortName 短い形式の名前を取得する場合 true
		 * @return キー名。対応するキーが無い場合は @c null
		 * @throw std::out_of_range @a key が不正なときスロー
		 */
		inline const wchar_t* KeyboardMap::getKeyName(VirtualKey key, bool shortName) {
			if(key >= 0x100)
				throw std::out_of_range("The first argument is invalid as virutal key code.");
			return shortName ? SHORT_KEY_NAMES[key] : LONG_KEY_NAMES[key];
		}

		/// キーストロークを表現する文字列を返す
		inline std::wstring KeyboardMap::getStrokeString(const KeyCombination& keys, bool shortName) {
			if(keys.modifiers == 0)
				return shortName ? SHORT_KEY_NAMES[keys.key] : LONG_KEY_NAMES[keys.key];
			std::wostringstream	ss;

			if(toBoolean(keys.modifiers & KM_CTRL))		ss << (shortName ? L"C-" : L"Ctrl+");
			if(toBoolean(keys.modifiers & KM_SHIFT))	ss << (shortName ? L"S-" : L"Shift+");
			if(toBoolean(keys.modifiers & KM_ALT))		ss << (shortName ? L"M-" : L"Alt+");
			ss << (shortName ? SHORT_KEY_NAMES[keys.key] : LONG_KEY_NAMES[keys.key]);
			return ss.str();
		}

		/// キーストロークを表現する文字列を返す
		inline std::wstring KeyboardMap::getStrokeString(
				const KeyCombination& firstKeys, const KeyCombination& secondKeys, bool shortName) {
			return getStrokeString(KeyCombination(firstKeys.key, firstKeys.modifiers), shortName)
				+ L" " + getStrokeString(KeyCombination(secondKeys.key, secondKeys.modifiers), shortName);
		}

		/// 未保存状態かを返す
		inline bool KeyboardMap::isDirty() const {return dirty_;}

}} // namespace alpha::command

#endif /* !ALPHA_KEYBOARD_MAP_HPP */
