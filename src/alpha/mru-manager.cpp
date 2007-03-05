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
using manah::win32::ui::Menu;
using ascension::encodings::CodePage;


/**
 * Constructor.
 * @param limit
 * @param startID
 */
MRUManager::MRUManager(size_t limit, int startID) : startID_(startID), limitCount_(limit) {
	updateMenu();
}

/**
 * Adds the new file. If there is already in the list, moves to the top.
 * @param fileName the name of the file to add
 * @param cp the code page
 */
void MRUManager::add(const basic_string<WCHAR>& fileName, CodePage cp) {
	list<MRU>::iterator it = fileNames_.begin();
	WCHAR realName[MAX_PATH + 1];

	// “¯‚¶‚à‚Ì‚ª‚ ‚é‚©’T‚·
	if(!ascension::text::Document::canonicalizePathName(fileName.c_str(), realName)) {
		wcscpy(realName, fileName.c_str());
		::CharLowerW(realName);
	}
	while(it != fileNames_.end()) {
		if(ascension::text::Document::areSamePathNames(realName, it->fileName.c_str())) {	// Œ©‚Â‚©‚Á‚½ -> æ“ª‚Éo‚·
			MRU item = *it;
			item.codePage = cp;
			fileNames_.erase(it);
			fileNames_.push_front(item);
			updateMenu();
			return;
		}
		++it;
	}

	// æ“ª‚É’Ç‰Á‚·‚éB‘”‚ªãŒÀ’l‚ð’´‚¦‚éê‡‚Íˆê”ÔŒÃ‚¢‚à‚Ì‚ðíœ‚·‚é
	MRU item = {fileName, cp};
	fileNames_.push_front(item);
	if(fileNames_.size() > limitCount_)
		fileNames_.resize(limitCount_);
	updateMenu();
}

/// Returns the item.
const MRU& MRUManager::getFileInfoAt(size_t index) const {
	if(index >= fileNames_.size())
		throw out_of_range("First argument is out of range!");
	list<MRU>::const_iterator it = fileNames_.begin();
	for(size_t i = 0; i < index; ++i)
		++it;
	return *it;
}

/// Removes the specified item.
void MRUManager::remove(size_t index) {
	if(index >= fileNames_.size())
		throw out_of_range("First argument is out of range!");
	list<MRU>::iterator it = fileNames_.begin();
	for(size_t i = 0; i < index; ++i)
		++it;
	fileNames_.erase(it);
	updateMenu();
}

/// Sets the maximum number of items.
void MRUManager::setLimit(size_t newLimit) {
	if(newLimit < 4)
		newLimit = 4;
	else if(newLimit > 16)
		newLimit = 16;
	limitCount_ = newLimit;

	// ‚ ‚Ó‚ê‚½•ª‚ðíœ
	if(fileNames_.size() > newLimit) {
		fileNames_.resize(newLimit);
		for(size_t i = fileNames_.size() - 1; i > newLimit; --i)
			popupMenu_.remove<Menu::BY_POSITION>(static_cast<UINT>(i));
	}
}

/// Reconstructs the menu according to the content of @c fileNames_.
void MRUManager::updateMenu() {
	size_t item = 0;
	list<MRU>::const_iterator it = fileNames_.begin();
	wchar_t caption[MAX_PATH + 100];

	while(popupMenu_.getNumberOfItems() > 0)
		popupMenu_.erase<Menu::BY_POSITION>(0);
	if(fileNames_.empty()) {	// —š—ð‚ª‹ó‚Ìê‡
		const wstring s = Alpha::getInstance().loadString(MSG_OTHER__EMPTY_MENU_CAPTION);
		popupMenu_.append(0, s.c_str(), MFS_GRAYED);
		return;
	}
	while(it != fileNames_.end()) {
		swprintf(caption, L"&%x  %s", item, it->fileName.c_str());
		popupMenu_.append(static_cast<UINT>(startID_ + item), caption);
		++it;
		++item;
	}
}
