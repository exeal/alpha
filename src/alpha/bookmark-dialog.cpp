/**
 * @file bookmark-dialog.cpp
 * @author exeal
 * @date 2003-2007
 */

#include "stdafx.h"
#include "bookmark-dialog.hpp"
#include "command.hpp"
#include "resource/messages.h"
#include "../manah/win32/ui/common-controls.hpp"

using alpha::ui::BookmarkDialog;
using alpha::Alpha;
using alpha::Buffer;
using namespace manah::win32::ui;
using namespace ascension;
using namespace std;


/**
 * Returns the information of the specified item.
 * @param[in] index the index of the item
 * @param[out] buffer the buffer
 * @param[out] line the line number
 */
void BookmarkDialog::getItemInfo(int index, Buffer*& buffer, length_t& line) const {
	wchar_t location[300];
	wchar_t* delimiter = 0;

	assert(index < bookmarksList_.getItemCount());
	bookmarksList_.getItemText(index, 1, location, 300);
	buffer = reinterpret_cast<Buffer*>(bookmarksList_.getItemData(index));

	delimiter = wcsrchr(location, L'(');
	assert(delimiter != 0);
	line = wcstoul(delimiter + 1, 0, 10);
	--line;
}

/// [削除] ボタンの処理
void BookmarkDialog::onBtnDelete() {
	const int sel = bookmarksList_.getSelectionMark();

	if(sel == -1)	// 選択が無い
		return;

	Buffer* buffer;
	length_t line;

	getItemInfo(sel, buffer, line);
	if(bufferIndices_.find(buffer) == bufferIndices_.end())
		return;

	buffer->bookmarker().mark(line, false);
	bookmarksList_.deleteItem(sel);
	if(bookmarksList_.getItemCount() != 0) {
		bookmarksList_.setItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	} else {
		::EnableWindow(getItem(IDOK), false);
		::EnableWindow(getItem(IDC_BTN_DELETE), false);
	}
}

/// ブックマークリストの更新
void BookmarkDialog::updateList() {
	const BufferList& buffers = Alpha::instance().bufferList();
	list<length_t> lines;
	Char location[300];
	int item = 0;

	bufferIndices_.clear();
	bookmarksList_.deleteAllItems();

	if(isButtonChecked(IDC_CHK_SHOWALLFILES) == BST_CHECKED) {	// 全てのドキュメントを扱う場合
		for(size_t i = 0; i < buffers.numberOfBuffers(); ++i) {
			const Buffer& buffer = buffers.at(i);
			const length_t lineOffset = buffers.activeView().verticalRulerConfiguration().lineNumbers.startValue;
			const length_t topLine = buffer.accessibleRegion().first.line;
			const length_t bottomLine = buffer.accessibleRegion().second.line;
			length_t line = 0;

			while((line = buffer.bookmarker().getNext(line, FORWARD)) != INVALID_INDEX) {
				if(line >= topLine && line <= bottomLine) {
					String s(buffer.line(line).substr(0, 100));
					++item;
					replace_if(s.begin(), s.end(), bind2nd(equal_to<wchar_t>(), L'\t'), L' ');
					item = bookmarksList_.insertItem(item, s.c_str());
					swprintf(location, L"%s(%lu)", buffer.name(), line + lineOffset);
					bookmarksList_.setItemText(item, 1, location);
					bookmarksList_.setItemData(item, static_cast<::DWORD>(reinterpret_cast<::DWORD_PTR>(&buffer)));
					bufferIndices_[const_cast<Buffer*>(&buffer)] = i;
				}
				if(++line > bottomLine)
					break;
			}
		}
	} else {
		// アクティブなドキュメントだけを対象にする場合
		const Buffer& activeBuffer = buffers.active();
		const length_t lineOffset = buffers.activeView().verticalRulerConfiguration().lineNumbers.startValue;
		const length_t topLine = activeBuffer.accessibleRegion().first.line;
		const length_t bottomLine = activeBuffer.accessibleRegion().second.line;
		length_t line = 0;

		while((line = activeBuffer.bookmarker().getNext(line, FORWARD)) != INVALID_INDEX) {
			if(line >= topLine && line <= bottomLine) {
				String s = activeBuffer.line(line).substr(0, 100);
				++item;
				replace_if(s.begin(), s.end(), bind2nd(equal_to<wchar_t>(), L'\t'), L' ');
				item = bookmarksList_.insertItem(item, s.c_str());
				swprintf(location, L"%s(%lu)", activeBuffer.name(), line + lineOffset);
				bookmarksList_.setItemText(item, 1, location);
				bookmarksList_.setItemData(item, static_cast<::DWORD>(reinterpret_cast<::DWORD_PTR>(&activeBuffer)));
			}
			if(++line > bottomLine)
				break;
		}
		bufferIndices_[const_cast<Buffer*>(&activeBuffer)] = buffers.activeIndex();
	}

	if(bookmarksList_.getItemCount() != 0) {
		bookmarksList_.setItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		::EnableWindow(getItem(IDOK), true);
		::EnableWindow(getItem(IDC_BTN_DELETE), true);
	} else {
		::EnableWindow(getItem(IDOK), false);
		::EnableWindow(getItem(IDC_BTN_DELETE), false);
	}
}

/// @see Dialog#onClose
void BookmarkDialog::onClose(bool&) {
	Alpha& app = Alpha::instance();
	app.writeIntegerProfile(L"Search", L"BookmarkDialog.autoClose", (isButtonChecked(IDC_CHK_AUTOCLOSE) == BST_CHECKED) ? 1 : 0);
	app.writeIntegerProfile(L"Search", L"BookmarkDialog.allBuffers", (isButtonChecked(IDC_CHK_SHOWALLFILES) == BST_CHECKED) ? 1 : 0);
}

/// @see Dialog#onCommand
bool BookmarkDialog::onCommand(WORD id, WORD notifyCode, HWND control) {
	switch(id) {
	case IDC_BTN_ADD: {	// [追加]
		const BufferList& buffers = Alpha::instance().bufferList();
		buffers.active().bookmarker().mark(buffers.activeView().caret().lineNumber(), true);
		updateList();
		break;
	}
	case IDC_BTN_DELETE:	// [削除]
		onBtnDelete();
		break;
	case IDC_BTN_UPDATE:	// [更新]
	case IDC_CHK_SHOWALLFILES:	// [開いている全てのドキュメントを表示]
		updateList();
		break;
	}
	return Dialog::onCommand(id, notifyCode, control);
}

/// @see Dialog#onInitDialog
void BookmarkDialog::onInitDialog(HWND focusWindow, bool&) {
	modifyStyleEx(0, WS_EX_LAYERED);
	setLayeredAttributes(0, 220, LWA_ALPHA);

	Alpha& app = Alpha::instance();
	bookmarksList_.modifyStyleEx(WS_EX_NOPARENTNOTIFY, 0);
	bookmarksList_.setExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
	bookmarksList_.insertColumn(0, app.loadMessage(MSG_DIALOG__BOOKMARKED_LINE).c_str(), LVCFMT_LEFT, 279, -1);
	bookmarksList_.insertColumn(1, app.loadMessage(MSG_DIALOG__BOOKMARKED_POSITION).c_str(), LVCFMT_LEFT, 100, -1);
	updateList();

	if(app.readIntegerProfile(L"Search", L"BookmarkDialog.autoClose", 0) == 1)
		checkButton(IDC_CHK_AUTOCLOSE, BST_CHECKED);
	if(app.readIntegerProfile(L"Search", L"BookmarkDialog.allBuffers", 0) == 1)
		checkButton(IDC_CHK_SHOWALLFILES, BST_CHECKED);
}

/// @see Dialog#onNotify
bool BookmarkDialog::onNotify(int id, ::NMHDR& nmhdr) {
	if(id== IDC_LIST_BOOKMARKS && nmhdr.code == NM_DBLCLK) {
		postMessage(WM_COMMAND, IDOK);
		return true;
	}
	return false;
}

/// @see Dialog#OnOK
void BookmarkDialog::onOK(bool& continueDialog) {
	// 一時マクロ定義中は実行できない
	Alpha& app = Alpha::instance();
	if(app.commandManager().temporaryMacro().state() == command::TemporaryMacro::DEFINING) {
		app.messageBox(MSG_ERROR__PROHIBITED_FOR_MACRO_DEFINING, MB_ICONEXCLAMATION);
		continueDialog = true;
		return;
	}

	const int sel = bookmarksList_.getSelectionMark();
	if(sel == -1)	// 選択が無い
		return;

	Buffer* buffer;
	length_t line;
	getItemInfo(sel, buffer, line);
	if(bufferIndices_.find(buffer) == bufferIndices_.end()) {
		continueDialog = true;
		return;
	}

	app.bufferList().activeView().caret().moveTo(kernel::Position(line, 0));
	app.bufferList().setActive(bufferIndices_[buffer]);
	getParent().setActive();

	if(isButtonChecked(IDC_CHK_AUTOCLOSE) != BST_CHECKED)	// [自動的に閉じる]
		continueDialog = true;
}
