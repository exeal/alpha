/**
 * @file mru-manager.cpp
 * @author exeal
 * @date 2003-2007
 */

#include "stdafx.h"
#include "mru-manager.hpp"
#include "application.hpp"
#include "resource/messages.h"
using namespace alpha;
using namespace std;
using manah::win32::ui::Menu;


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
 */
void MRUManager::add(const basic_string<::WCHAR>& fileName) {
	const basic_string<WCHAR> realName(ascension::kernel::fileio::canonicalizePathName(fileName.c_str()));

	// 同じものがあるか探す
	for(list<basic_string<WCHAR> >::iterator i(fileNames_.begin()), e(fileNames_.end()); i != e; ++i) {
		if(ascension::kernel::fileio::comparePathNames(realName.c_str(), i->c_str())) {	// 見つかった -> 先頭に出す
			const basic_string<WCHAR> item(*i);
			fileNames_.erase(i);
			fileNames_.push_front(item);
			updateMenu();
			return;
		}
	}

	// 先頭に追加する。総数が上限値を超える場合は一番古いものを削除する
	fileNames_.push_front(realName);
	if(fileNames_.size() > limitCount_)
		fileNames_.resize(limitCount_);
	updateMenu();
}

/// Returns the item.
const basic_string<WCHAR>& MRUManager::at(size_t index) const {
	if(index >= fileNames_.size())
		throw out_of_range("First argument is out of range!");
	list<basic_string<WCHAR> >::const_iterator it = fileNames_.begin();
	for(size_t i = 0; i < index; ++i)
		++it;
	return *it;
}

/// Loads the list from INI file.
void MRUManager::load() {
	Alpha& app = Alpha::instance();
	wchar_t keyName[30];
	fileNames_.clear();
	for(uint i = 0; i < limitCount_; ++i) {
		swprintf(keyName, L"pathName(%u)", i);
		const basic_string<WCHAR> fileName(app.readStringProfile(L"MRU", keyName));
		if(fileName.empty())
			break;
		fileNames_.push_back(fileName);
	}
	updateMenu();
}

/// Removes the specified item.
void MRUManager::remove(size_t index) {
	if(index >= fileNames_.size())
		throw out_of_range("First argument is out of range!");
	list<basic_string<WCHAR> >::iterator it = fileNames_.begin();
	for(size_t i = 0; i < index; ++i)
		++it;
	fileNames_.erase(it);
	updateMenu();
}

/// Write the list to INI file.
void MRUManager::save() {
	Alpha& app = Alpha::instance();
	wchar_t keyName[30];
	list<basic_string<WCHAR> >::const_iterator i(fileNames_.begin());
	for(size_t index = 0; i != fileNames_.end(); ++index, ++i) {
		swprintf(keyName, L"pathName(%u)", index);
		app.writeStringProfile(L"MRU", keyName, i->c_str());
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
	list<basic_string<WCHAR> >::const_iterator i(fileNames_.begin());
	wchar_t caption[MAX_PATH + 100];

	while(popupMenu_.getNumberOfItems() > 0)
		popupMenu_.erase<Menu::BY_POSITION>(0);
	if(fileNames_.empty()) {	// 履歴が空の場合
		const wstring s(Alpha::instance().loadMessage(MSG_OTHER__EMPTY_MENU_CAPTION));
		popupMenu_ << Menu::StringItem(0, s.c_str(), MFS_GRAYED);
		return;
	}
	while(i != fileNames_.end()) {
		swprintf(caption, L"&%x  %s", item, i->c_str());
		popupMenu_ << Menu::StringItem(static_cast<UINT>(startID_ + item), caption);
		++i;
		++item;
	}
}
