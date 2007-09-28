/**
 * @file keyboard-map.cpp
 * @author exeal
 * @date 2003-2007
 */

#include "stdafx.h"
#include "keyboard-map.hpp"
#include "command.hpp"
#include "resource/messages.h"
#include "../manah/win32/file.hpp"
using namespace alpha::command;
using namespace std;


/// コンストラクタ
KeyboardMap::KeyboardMap() {
	for(KeyModifier modifiers = 0; modifiers < 8; ++modifiers) {
		for(VirtualKey key = 0; key < 0x0100; ++key) {
			firstKeyMaps_[modifiers][key].command = 0;
			firstKeyMaps_[modifiers][key].secondKeyMap = 0;
		}
	}
	clear();
	dirty_ = false;
}

/// デストラクタ
KeyboardMap::~KeyboardMap() {
	clear();
}

/**
 * 1つのコマンドを登録
 * @param command コマンド
 * @param keys キー組み合わせ
 * @return 既存の割り当てを上書きした場合は false
 */
bool KeyboardMap::assign(const KeyAssignableCommand& command, const KeyCombination& keys) {
	FirstKeyMap& firstKeyMap = firstKeyMaps_[keys.modifiers][keys.key];
	bool overridden = false;

	if(firstKeyMap.secondKeyMap != 0) {
		for(size_t i = 0; i < 8; ++i) {
			for(size_t j = 0; j < 0x100; ++j)
				delete firstKeyMap.secondKeyMap[i][j];
			delete[] firstKeyMap.secondKeyMap[i];
		}
		delete[] firstKeyMap.secondKeyMap;
		firstKeyMap.secondKeyMap = 0;
		overridden = true;
	}
	if(firstKeyMap.command != 0) {
		delete firstKeyMap.command;
		overridden = true;
	}
	command.copy(firstKeyMap.command);
	dirty_ = true;

	return !overridden;
}

/**
 * 1つのコマンドを登録
 * @param command コマンド
 * @param firstKeys 第1のキー組み合わせ
 * @param secondKeys 第2のキー組み合わせ
 */
bool KeyboardMap::assign(const KeyAssignableCommand& command, const KeyCombination& firstKeys, const KeyCombination& secondKeys) {
	FirstKeyMap& firstKeyMap = firstKeyMaps_[firstKeys.modifiers][firstKeys.key];
	bool overridden = false;

	if(firstKeyMap.command != 0 &&
			(!firstKeyMap.command->isBuiltIn() || firstKeyMap.command->getID() != CMD_SPECIAL_WAITINGFORNEXTKEYCOMBINATION))
		overridden = true;
	delete firstKeyMap.command;
	firstKeyMap.command = new BuiltInCommand(CMD_SPECIAL_WAITINGFORNEXTKEYCOMBINATION);

	if(firstKeyMap.secondKeyMap == 0) {
		firstKeyMap.secondKeyMap = new KeyAssignableCommand**[8];
		memset(firstKeyMap.secondKeyMap, 0, sizeof(Command**) * 8);
	}
	if(firstKeyMap.secondKeyMap[secondKeys.modifiers] == 0) {
		firstKeyMap.secondKeyMap[secondKeys.modifiers] = new KeyAssignableCommand*[0x0100];
		memset(firstKeyMap.secondKeyMap[secondKeys.modifiers], 0, sizeof(KeyAssignableCommand*) * 0x0100);
	}
	command.copy(firstKeyMap.secondKeyMap[secondKeys.modifiers][secondKeys.key]);
	dirty_ = true;

	return !overridden;
}

/// 登録を全て解除
void KeyboardMap::clear() {
	dirty_ = true;
	for(KeyModifier modifiers = 0; modifiers < 8; ++modifiers) {
		for(VirtualKey key = 0; key < 0x0100; ++key) {
			FirstKeyMap& firstKeyMap = firstKeyMaps_[modifiers][key];
			delete firstKeyMap.command;
			firstKeyMap.command = 0;
			if(firstKeyMap.secondKeyMap != 0) {
				for(size_t i = 0; i < 8; ++i) {
					if(firstKeyMap.secondKeyMap[i] != 0) {
						for(size_t j = 0; j < 0x100; ++j)
							delete firstKeyMap.secondKeyMap[i][j];
						delete[] firstKeyMap.secondKeyMap[i];
					}
				}
				delete[] firstKeyMap.secondKeyMap;
				firstKeyMap.secondKeyMap = 0;
			}
		}
	}
}

/**
 * 組み込みコマンドに割り当てられているキーの文字列表現を取得
 * @param id コマンド識別値
 * @return "Ctrl+N" などの文字列表現。登録されていなければ空文字列
 */
wstring KeyboardMap::getKeyString(CommandID id) const {
	wstring result;
	for(KeyModifier firstModifiers = 0; firstModifiers < 8; ++firstModifiers) {
		for(VirtualKey firstKey = 0; firstKey < 0x0100; ++firstKey) {
			const FirstKeyMap& firstKeyMap = firstKeyMaps_[firstModifiers][firstKey];
			if(firstKeyMap.command != 0
					&& firstKeyMap.command->isBuiltIn()
					&& firstKeyMap.command->getID() == id) {	// 1 ストローク
				const wstring buffer = getStrokeString(KeyCombination(firstKey, firstModifiers));
				if(result.empty() || buffer.length() < result.length())
					result.assign(buffer);
			} else if(firstKeyMap.secondKeyMap != 0) {
				for(KeyModifier secondModifiers = 0; secondModifiers < 8; ++secondModifiers) {
					if(firstKeyMap.secondKeyMap[secondModifiers] == 0)
						continue;
					for(VirtualKey secondKey = 0; secondKey < 0x0100; ++secondKey) {
						if(firstKeyMap.secondKeyMap[secondModifiers][secondKey] != 0
								&& firstKeyMap.secondKeyMap[secondModifiers][secondKey]->isBuiltIn()
								&& firstKeyMap.secondKeyMap[secondModifiers][secondKey]->getID() == id) {	// 2 ストローク
							const wstring buffer = getStrokeString(
								KeyCombination(firstKey, firstModifiers), KeyCombination(secondKey, secondModifiers));
							if(result.empty() || buffer.length() < result.length())
								result.assign(buffer);
						}
					}
				}
			}
		}
	}
	return result;
}

/**
 * ファイルから読み込む
 * @param fileName
 * @return
 */
bool KeyboardMap::load(const WCHAR* fileName) {
	assert(fileName != 0);

	using namespace manah::win32::io;

	clear();

	try {
		File<false> file(fileName, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL);
		CommandID id;
		VirtualKey firstKey, secondKey;
		KeyModifier firstModifiers, secondModifiers;
		DWORD readBytes;

		while(true) {
			if(!file.read(&id, sizeof(CommandID), &readBytes) || readBytes != sizeof(CommandID))
				break;
			if(!file.read(&firstKey, sizeof(VirtualKey), &readBytes) || readBytes != sizeof(VirtualKey))
				break;
			if(!file.read(&firstModifiers, sizeof(KeyModifier), &readBytes) || readBytes != sizeof(KeyModifier))
				break;
			if(id != CMD_SPECIAL_WAITINGFORNEXTKEYCOMBINATION)
				assign(BuiltInCommand(id), KeyCombination(firstKey, firstModifiers));
			else {
				if(!file.read(&id, sizeof(CommandID), &readBytes) || readBytes != sizeof(CommandID))
					break;
				if(!file.read(&secondKey, sizeof(VirtualKey), &readBytes) || readBytes != sizeof(VirtualKey))
					break;
				if(!file.read(&secondModifiers, sizeof(KeyModifier), &readBytes) || readBytes != sizeof(KeyModifier))
					break;
				assign(BuiltInCommand(id),
					KeyCombination(firstKey, firstModifiers), KeyCombination(secondKey, secondModifiers));
			}
		}
		file.close();
	} catch(FileException& /*e*/) {
		return false;
	} catch(out_of_range&) {
		return false;
	}

	dirty_ = false;
	return true;
}

/**
 * ファイルに保存する
 * @param fileName
 * @return
 */
bool KeyboardMap::save(const WCHAR* fileName) {
	assert(fileName != 0);

	using namespace manah::win32::io;

	try {
		File<false> file(fileName, GENERIC_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL);

		for(KeyModifier firstModifiers = 0; firstModifiers < 8; ++firstModifiers) {
			for(VirtualKey firstKey = 0; firstKey < 0x0100; ++firstKey) {
				const FirstKeyMap& firstKeyMap = firstKeyMaps_[firstModifiers][firstKey];
				if(firstKeyMap.command == 0)	// 割り当てられていない
					continue;
				else if(firstKeyMap.secondKeyMap == 0 && firstKeyMap.command->isBuiltIn()) {	// 1ストローク
					const CommandID id = firstKeyMap.command->getID();
					file.write(&id, sizeof(CommandID));
					file.write(&firstKey, sizeof(VirtualKey));
					file.write(&firstModifiers, sizeof(KeyModifier));
				} else {	// 2ストローク
					assert(firstKeyMap.secondKeyMap != 0);
					for(KeyModifier secondModifiers = 0; secondModifiers < 8; ++secondModifiers) {
						for(VirtualKey secondKey = 0; secondKey < 0x0100; ++secondKey) {
							if(firstKeyMap.secondKeyMap[secondModifiers] == 0
									|| firstKeyMap.secondKeyMap[secondModifiers][secondKey] == 0
									|| !firstKeyMap.secondKeyMap[secondModifiers][secondKey]->isBuiltIn())
								continue;
							CommandID id = firstKeyMap.command->getID();
							file.write(&id, sizeof(CommandID));
							file.write(&firstKey, sizeof(VirtualKey));
							file.write(&firstModifiers, sizeof(KeyModifier));
							id = firstKeyMap.secondKeyMap[secondModifiers][secondKey]->getID();
							file.write(&id, sizeof(CommandID));
							file.write(&secondKey, sizeof(VirtualKey));
							file.write(&secondModifiers, sizeof(KeyModifier));
						}
					}
				}
			}
		}
		file.close();
	} catch(FileException& /*e*/) {
		return false;
	}

	dirty_ = false;
	return true;
}

/**
 * キー割り当てを1つ解除。キー組み合わせが2ストロークキーの第1キー組み合わせであればその全ての割り当てが解除される
 * @param keys
 */
void KeyboardMap::unassign(const KeyCombination& keys) {
	FirstKeyMap& firstKeyMap = firstKeyMaps_[keys.modifiers][keys.key];

	if(firstKeyMap.command == 0)
		return;
	dirty_ = true;
	delete firstKeyMap.command;
	firstKeyMap.command = 0;
	if(firstKeyMap.secondKeyMap != 0) {
		for(size_t i = 0; i < 8; ++i) {
			if(firstKeyMap.secondKeyMap[i] != 0) {
				for(size_t j = 0; j < 0x100; ++j)
					delete firstKeyMap.secondKeyMap[i][j];
				delete[] firstKeyMap.secondKeyMap[i];
			}
		}
		delete[] firstKeyMap.secondKeyMap;
		firstKeyMap.secondKeyMap = 0;
	}
}

/**
 * キー割り当てを1つ解除
 * @param firstKeys
 * @param secondKeys
 */
void KeyboardMap::unassign(const KeyCombination& firstKeys, const KeyCombination& secondKeys) {
	const FirstKeyMap& firstKeyMap = firstKeyMaps_[firstKeys.modifiers][firstKeys.key];
	if(firstKeyMap.secondKeyMap != 0) {
		if(firstKeyMap.secondKeyMap[secondKeys.modifiers] != 0) {
			delete firstKeyMap.secondKeyMap[secondKeys.modifiers][secondKeys.key];
			firstKeyMap.secondKeyMap[secondKeys.modifiers][secondKeys.key] = 0;
			dirty_ = true;
		}
	}
}
