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


/// �R���X�g���N�^
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

/// �f�X�g���N�^
KeyboardMap::~KeyboardMap() {
	clear();
}

/**
 * 1�̃R�}���h��o�^
 * @param command �R�}���h
 * @param keys �L�[�g�ݍ��킹
 * @return �����̊��蓖�Ă��㏑�������ꍇ�� false
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
 * 1�̃R�}���h��o�^
 * @param command �R�}���h
 * @param firstKeys ��1�̃L�[�g�ݍ��킹
 * @param secondKeys ��2�̃L�[�g�ݍ��킹
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

/// �o�^��S�ĉ���
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
 * �g�ݍ��݃R�}���h�Ɋ��蓖�Ă��Ă���L�[�̕�����\�����擾
 * @param id �R�}���h���ʒl
 * @return "Ctrl+N" �Ȃǂ̕�����\���B�o�^����Ă��Ȃ���΋󕶎���
 */
wstring KeyboardMap::getKeyString(CommandID id) const {
	wstring result;
	for(KeyModifier firstModifiers = 0; firstModifiers < 8; ++firstModifiers) {
		for(VirtualKey firstKey = 0; firstKey < 0x0100; ++firstKey) {
			const FirstKeyMap& firstKeyMap = firstKeyMaps_[firstModifiers][firstKey];
			if(firstKeyMap.command != 0
					&& firstKeyMap.command->isBuiltIn()
					&& firstKeyMap.command->getID() == id) {	// 1 �X�g���[�N
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
								&& firstKeyMap.secondKeyMap[secondModifiers][secondKey]->getID() == id) {	// 2 �X�g���[�N
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
 * �t�@�C������ǂݍ���
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
 * �t�@�C���ɕۑ�����
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
				if(firstKeyMap.command == 0)	// ���蓖�Ă��Ă��Ȃ�
					continue;
				else if(firstKeyMap.secondKeyMap == 0 && firstKeyMap.command->isBuiltIn()) {	// 1�X�g���[�N
					const CommandID id = firstKeyMap.command->getID();
					file.write(&id, sizeof(CommandID));
					file.write(&firstKey, sizeof(VirtualKey));
					file.write(&firstModifiers, sizeof(KeyModifier));
				} else {	// 2�X�g���[�N
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
 * �L�[���蓖�Ă�1�����B�L�[�g�ݍ��킹��2�X�g���[�N�L�[�̑�1�L�[�g�ݍ��킹�ł���΂��̑S�Ă̊��蓖�Ă����������
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
 * �L�[���蓖�Ă�1����
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
