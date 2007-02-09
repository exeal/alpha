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
 * コンストラクタ
 * @param limit
 * @param startID
 * @param ownerDrawMenu
 */
MRUManager::MRUManager(size_t limit, int startID, bool ownerDrawMenu /* = false */)
		: limitCount_(limit), startID_(startID), ownerDraw_(ownerDrawMenu) {
	updateMenu();
}

/**
 * 項目を追加する。既に同じものがあればそれを先頭に出す
 * @param fileName 新しく追加するファイルパス
 * @param cp コードページ
 */
void MRUManager::add(const basic_string<WCHAR>& fileName, CodePage cp) {
	list<MRU>::iterator it = fileNames_.begin();
	WCHAR realName[MAX_PATH + 1];

	// 同じものがあるか探す
	if(!ascension::text::Document::canonicalizePathName(fileName.c_str(), realName)) {
		wcscpy(realName, fileName.c_str());
		::CharLowerW(realName);
	}
	while(it != fileNames_.end()) {
		if(ascension::text::Document::areSamePathNames(realName, it->fileName.c_str())) {	// 見つかった -> 先頭に出す
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

/// 指定した位置のファイルパスを返す
const MRU& MRUManager::getFileInfoAt(size_t index) const {
	if(index >= fileNames_.size())
		throw out_of_range("First argument is out of range!");
	list<MRU>::const_iterator it = fileNames_.begin();
	for(size_t i = 0; i < index; ++i)
		++it;
	return *it;
}

/// 項目の削除
void MRUManager::remove(size_t index) {
	if(index >= fileNames_.size())
		throw out_of_range("First argument is out of range!");
	list<MRU>::iterator it = fileNames_.begin();
	for(size_t i = 0; i < index; ++i)
		++it;
	fileNames_.erase(it);
	updateMenu();
}

/// 項目数の上限値の設定
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
			popupMenu_.removeMenuItem<Menu::BY_POSITION>(static_cast<UINT>(i));
	}
}

/// @c fileNames_ からメニューを再構成する
void MRUManager::updateMenu() {
	size_t item = 0;
	list<MRU>::const_iterator it = fileNames_.begin();
	wchar_t caption[MAX_PATH + 100];

	while(popupMenu_.getItemCount() > 0)
		popupMenu_.deleteMenuItem<Menu::BY_POSITION>(0);
	if(fileNames_.empty()) {	// 履歴が空の場合
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
