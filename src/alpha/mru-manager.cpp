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
	const basic_string<WCHAR> realName = ascension::text::canonicalizePathName(fileName.c_str());

	// 同じものがあるか探す
	while(it != fileNames_.end()) {
		if(ascension::text::comparePathNames(realName.c_str(), it->fileName.c_str())) {	// 見つかった -> 先頭に出す
			MRU item = *it;
			item.codePage = cp;
			fileNames_.erase(it);
			fileNames_.push_front(item);
			updateMenu();
			return;
		}
		++it;
	}

	// 先頭に追加する。総数が上限値を超える場合は一番古いものを削除する
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

/// Loads the list from INI file.
void MRUManager::load() {
	Alpha& app = Alpha::getInstance();
	wchar_t keyName[30];
	fileNames_.clear();
	for(uint i = 0; i < limitCount_; ++i) {
		MRU file;
		swprintf(keyName, L"pathName(%u)", i);
		file.fileName = app.readStringProfile(L"MRU", keyName);
		if(file.fileName.empty())
			break;
		swprintf(keyName, L"codePage(%u)", i);
		file.codePage = app.readIntegerProfile(L"MRU", keyName, ascension::encodings::CPEX_AUTODETECT_USERLANG);
		fileNames_.push_back(file);
	}
	updateMenu();
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

/// Write the list to INI file.
void MRUManager::save() {
	Alpha& app = Alpha::getInstance();
	wchar_t keyName[30];
	list<MRU>::const_iterator it(fileNames_.begin());
	for(size_t i = 0; it != fileNames_.end(); ++i, ++it) {
		swprintf(keyName, L"pathName(%u)", i);
		app.writeStringProfile(L"MRU", keyName, it->fileName.c_str());
		swprintf(keyName, L"codePage(%u)", i);
		app.writeIntegerProfile(L"MRU", keyName, it->codePage);
	}
	app.writeStringProfile(L"MRU", keyName, L"");	// リストの終端を表す
}

/// Sets the maximum number of items.
void MRUManager::setLimit(size_t newLimit) {
	if(newLimit < 4)
		newLimit = 4;
	else if(newLimit > 16)
		newLimit = 16;
	limitCount_ = newLimit;

	// あふれた分を削除
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
	if(fileNames_.empty()) {	// 履歴が空の場合
		const wstring s = Alpha::getInstance().loadString(MSG_OTHER__EMPTY_MENU_CAPTION);
		popupMenu_ << Menu::StringItem(0, s.c_str(), MFS_GRAYED);
		return;
	}
	while(it != fileNames_.end()) {
		swprintf(caption, L"&%x  %s", item, it->fileName.c_str());
		popupMenu_ << Menu::StringItem(static_cast<UINT>(startID_ + item), caption);
		++it;
		++item;
	}
}
