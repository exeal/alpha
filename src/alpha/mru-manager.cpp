/**
 * @file mru-manager.cpp
 * @author exeal
 * @date 2003-2007
 */

#include "stdafx.h"
#include "mru-manager.hpp"
#include "application.hpp"
using namespace alpha;
using namespace std;
using manah::windows::ui::Menu;
using ascension::encodings::CodePage;


/**
 * �R���X�g���N�^
 * @param limit
 * @param startID
 * @param ownerDrawMenu
 */
MRUManager::MRUManager(size_t limit, int startID, bool ownerDrawMenu /* = false */)
		: limitCount_(limit), startID_(startID), ownerDraw_(ownerDrawMenu) {
	updateMenu();
}

/**
 * ���ڂ�ǉ�����B���ɓ������̂�����΂����擪�ɏo��
 * @param fileName �V�����ǉ�����t�@�C���p�X
 * @param cp �R�[�h�y�[�W
 */
void MRUManager::add(const basic_string<WCHAR>& fileName, CodePage cp) {
	list<MRU>::iterator it = fileNames_.begin();
	WCHAR realName[MAX_PATH + 1];

	// �������̂����邩�T��
	if(!ascension::text::Document::canonicalizePathName(fileName.c_str(), realName)) {
		wcscpy(realName, fileName.c_str());
		::CharLowerW(realName);
	}
	while(it != fileNames_.end()) {
		if(ascension::text::Document::areSamePathNames(realName, it->fileName.c_str())) {	// �������� -> �擪�ɏo��
			MRU item = *it;
			item.codePage = cp;
			fileNames_.erase(it);
			fileNames_.push_front(item);
			updateMenu();
			return;
		}
		++it;
	}

	// �擪�ɒǉ�����B����������l�𒴂���ꍇ�͈�ԌÂ����̂��폜����
	MRU item = {fileName, cp};
	fileNames_.push_front(item);
	if(fileNames_.size() > limitCount_)
		fileNames_.resize(limitCount_);
	updateMenu();
}

/// �w�肵���ʒu�̃t�@�C���p�X��Ԃ�
const MRU& MRUManager::getFileInfoAt(size_t index) const {
	if(index >= fileNames_.size())
		throw out_of_range("First argument is out of range!");
	list<MRU>::const_iterator it = fileNames_.begin();
	for(size_t i = 0; i < index; ++i)
		++it;
	return *it;
}

/// ���ڂ̍폜
void MRUManager::remove(size_t index) {
	if(index >= fileNames_.size())
		throw out_of_range("First argument is out of range!");
	list<MRU>::iterator it = fileNames_.begin();
	for(size_t i = 0; i < index; ++i)
		++it;
	fileNames_.erase(it);
	updateMenu();
}

/// ���ڐ��̏���l�̐ݒ�
void MRUManager::setLimit(size_t newLimit) {
	if(newLimit < 4)
		newLimit = 4;
	else if(newLimit > 16)
		newLimit = 16;
	limitCount_ = newLimit;

	// ���ӂꂽ�����폜
	if(fileNames_.size() > newLimit) {
		fileNames_.resize(newLimit);
		for(size_t i = fileNames_.size() - 1; i > newLimit; --i)
			popupMenu_.removeMenuItem<Menu::BY_POSITION>(static_cast<UINT>(i));
	}
}

/// @c fileNames_ ���烁�j���[���č\������
void MRUManager::updateMenu() {
	size_t item = 0;
	list<MRU>::const_iterator it = fileNames_.begin();
	wchar_t caption[MAX_PATH + 100];

	while(popupMenu_.getItemCount() > 0)
		popupMenu_.deleteMenuItem<Menu::BY_POSITION>(0);
	if(fileNames_.empty()) {	// ��������̏ꍇ
		const wstring s = Alpha::getInstance().loadString(MSG_OTHER__EMPTY_MENU_CAPTION);
		popupMenu_.appendMenuItem(0, s.c_str(), MFS_GRAYED);
		return;
	}
	while(it != fileNames_.end()) {
		swprintf(caption, L"&%x  %s", item, it->fileName.c_str());
		if(ownerDraw_)
			popupMenu_.appendMenuItem(static_cast<UINT>(startID_ + item), 0U);
		else
			popupMenu_.appendMenuItem(static_cast<UINT>(startID_ + item), caption);
		++it;
		++item;
	}
}
