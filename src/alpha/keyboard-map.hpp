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

		/// �R�}���h�̎��ʎq (����)
		typedef WORD CommandID;	// typeof(ACCEL::cmd)

		/// ���z�L�[�R�[�h�̌^
		typedef WORD VirtualKey;	// typeof(ACCEL::key)
		const VirtualKey VK_NULL = 0;	///< �����ȃL�[

		/// ���z�L�[�̏C���L�[
		typedef uchar KeyModifier;
		const KeyModifier KM_SHIFT	= 0x01;	///< Shift �L�[
		const KeyModifier KM_CTRL	= 0x02;	///< Ctrl �L�[
		const KeyModifier KM_ALT	= 0x04;	///< Alt �L�[

		/// 1�X�g���[�N�̃L�[�g�ݍ��킹
		struct KeyCombination {
			VirtualKey key;			///< �L�[
			KeyModifier modifiers;	///< �C���L�[

			/// �R���X�g���N�^
			KeyCombination(VirtualKey k = VK_NULL, KeyModifier m = 0) : key(k), modifiers(m) {
				assert(key < 0x100);
				assert(modifiers <= (KM_SHIFT | KM_CTRL | KM_ALT));
			}
		};

		/// �L�[�o�C���h�̊Ǘ�
		class KeyboardMap {
		public:
			// �R���X�g���N�^
			KeyboardMap();
			~KeyboardMap();
			// ����
			KeyAssignableCommand*	getCommand(const KeyCombination& keys) const;
			KeyAssignableCommand*	getCommand(const KeyCombination& firstKeys, const KeyCombination& secondKeys) const;
			static const wchar_t*	getKeyName(VirtualKey key, bool shortName);
			std::wstring			getKeyString(CommandID id, bool shortName) const;
			static std::wstring		getStrokeString(const KeyCombination& keys, bool shortName);
			static std::wstring		getStrokeString(const KeyCombination& firstKeys, const KeyCombination& secondKeys, bool shortName);
			bool					isDirty() const;
			// ����
			bool	assign(const KeyAssignableCommand& command, const KeyCombination& keys);
			bool	assign(const KeyAssignableCommand& command, const KeyCombination& firstKeys, const KeyCombination& secondKeys);
			void	clear();
			bool	load(const WCHAR* fileName);
			void	unassign(const KeyCombination& keys);
			void	unassign(const KeyCombination& firstKeys, const KeyCombination& secondKeys);
			bool	save(const WCHAR* fileName);

		private:
			struct FirstKeyMap {
				KeyAssignableCommand*	command;		// �L�[�V�[�P���X��1�X�g���[�N�ŏI���ꍇ�̓R�}���h
				KeyAssignableCommand***	secondKeyMap;	// 2�X�g���[�N�g���ꍇ�� 8 * 0x0100 �̔z��
														// �R�}���h�����蓖�Ă��Ă��Ȃ��ꍇ�͂�����̃����o�� null
			};
			FirstKeyMap firstKeyMaps_[8][0x0100];	// (�C���L�[) * (���z�L�[�R�[�h) �ŃR�}���h�����͑�2�L�[�g�ݍ��킹���i�[�B
													// 0�ł���Ή����o�^����Ă��Ȃ�
			bool dirty_;
		};

		namespace {
			// �L�[�̖��O (���蓖�ĕs�\�ȃL�[�� null)
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
		 * �L�[�g�ݍ��킹����R�}���h���擾
		 * @param keys �L�[�g�ݍ��킹
		 * @return ���蓖�Ă��Ă���R�}���h�B���蓖�Ă��Ă��Ȃ��Ƃ��� @c null�B
		 * 2�X�g���[�N��1�X�g���[�N�ڂł���� @c CMD_SPECIAL_WAITFOR2NDKEYS �ɑ������� @c BuiltInCommand
		 */
		inline KeyAssignableCommand* KeyboardMap::getCommand(const KeyCombination& keys) const {
			return firstKeyMaps_[keys.modifiers][keys.key].command;}

		/**
		 * 2�X�g���[�N�̃L�[�g�ݍ��킹����R�}���h���擾
		 * @param firstKeys ��1�̃L�[�g�ݍ��킹
		 * @param secondKeys ��2�̃L�[�g�ݍ��킹
		 * @return ���蓖�Ă��Ă���R�}���h�B���蓖�Ă��Ă��Ȃ��Ƃ��� @c null
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
		 * �L�[�̖��O��Ԃ�
		 * @param key ���z�L�[
		 * @param shortName �Z���`���̖��O���擾����ꍇ true
		 * @return �L�[���B�Ή�����L�[�������ꍇ�� @c null
		 * @throw std::out_of_range @a key ���s���ȂƂ��X���[
		 */
		inline const wchar_t* KeyboardMap::getKeyName(VirtualKey key, bool shortName) {
			if(key >= 0x100)
				throw std::out_of_range("The first argument is invalid as virutal key code.");
			return shortName ? SHORT_KEY_NAMES[key] : LONG_KEY_NAMES[key];
		}

		/// �L�[�X�g���[�N��\�����镶�����Ԃ�
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

		/// �L�[�X�g���[�N��\�����镶�����Ԃ�
		inline std::wstring KeyboardMap::getStrokeString(
				const KeyCombination& firstKeys, const KeyCombination& secondKeys, bool shortName) {
			return getStrokeString(KeyCombination(firstKeys.key, firstKeys.modifiers), shortName)
				+ L" " + getStrokeString(KeyCombination(secondKeys.key, secondKeys.modifiers), shortName);
		}

		/// ���ۑ���Ԃ���Ԃ�
		inline bool KeyboardMap::isDirty() const {return dirty_;}

}} // namespace alpha::command

#endif /* !ALPHA_KEYBOARD_MAP_HPP */
