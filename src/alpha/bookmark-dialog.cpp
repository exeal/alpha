/**
 * @file bookmark-dialog.cpp
 * @author exeal
 * @date 2003-2007
 */

#include "stdafx.h"
#include "bookmark-dialog.hpp"
#include "command.hpp"
#include "../manah/win32/ui/common-controls.hpp"

using alpha::ui::BookmarkDlg;
using alpha::Alpha;
using alpha::Buffer;
using namespace manah::windows::ui;
using namespace ascension;
using namespace std;


/// コンストラクタ
BookmarkDlg::BookmarkDlg(Alpha& app) : app_(app) {
}

/**
 *	指定した位置の項目情報を得る
 *	@param index 項目の位置
 *	@param[out] buffer バッファ
 *	@param[out] line 行番号 (0ベース)
 */
void BookmarkDlg::getItemInfo(int index, Buffer*& buffer, length_t& line) const {
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
void BookmarkDlg::onBtnDelete() {
	const int sel = bookmarksList_.getSelectionMark();

	if(sel == -1)	// 選択が無い
		return;

	Buffer* buffer;
	length_t line;

	getItemInfo(sel, buffer, line);
	if(bufferIndices_.find(buffer) == bufferIndices_.end())
		return;

	buffer->getBookmarker().mark(line, false);
	bookmarksList_.deleteItem(sel);
	if(bookmarksList_.getItemCount() != 0) {
		bookmarksList_.setItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	} else {
		::EnableWindow(getDlgItem(IDOK), false);
		::EnableWindow(getDlgItem(IDC_BTN_DELETE), false);
	}
}

/// ブックマークリストの更新
void BookmarkDlg::updateList() {
	const BufferList& buffers = app_.getBufferList();
	list<length_t> lines;
	Char location[300];
	int item = 0;

	bufferIndices_.clear();
	bookmarksList_.deleteAllItems();

	if(isDlgButtonChecked(IDC_CHK_SHOWALLFILES) == BST_CHECKED) {	// 全てのドキュメントを扱う場合
		for(size_t i = 0; i < buffers.getCount(); ++i) {
			const Buffer& buffer = buffers.getAt(i);
			const length_t lineOffset = buffers.getActiveView().getVerticalRulerConfiguration().lineNumbers.startValue;
			const length_t topLine = buffer.getStartPosition().line;
			const length_t bottomLine = buffer.getEndPosition().line;
			length_t line = 0;

			while((line = buffer.getBookmarker().getNext(line, FORWARD)) != -1) {
				if(line >= topLine && line <= bottomLine) {
					String s = buffer.getLine(line).substr(0, 100);

					++item;
					replace_if(s.begin(), s.end(), bind2nd(equal_to<wchar_t>(), L'\t'), L' ');
					item = bookmarksList_.insertItem(item, s.c_str());
					swprintf(location, L"%s(%lu)", buffer.getFileName(), line + lineOffset);
					bookmarksList_.setItemText(item, 1, location);
					bookmarksList_.setItemData(item, static_cast<DWORD>(reinterpret_cast<DWORD_PTR>(&buffer)));
					bufferIndices_[const_cast<Buffer*>(&buffer)] = i;
				}
			}
		}
	} else {
		// アクティブなドキュメントだけを対象にする場合
		const Buffer& activeBuffer = app_.getBufferList().getActive();
		const length_t lineOffset = app_.getBufferList().getActiveView().getVerticalRulerConfiguration().lineNumbers.startValue;
		const length_t topLine = activeBuffer.getStartPosition().line;
		const length_t bottomLine = activeBuffer.getEndPosition().line;
		length_t line = 0;

		while((line = activeBuffer.getBookmarker().getNext(line, FORWARD)) != -1) {
			if(line >= topLine && line <= bottomLine) {
				String s = activeBuffer.getLine(line).substr(0, 100);

				++item;
				replace_if(s.begin(), s.end(), bind2nd(equal_to<wchar_t>(), L'\t'), L' ');
				item = bookmarksList_.insertItem(item, s.c_str());
				swprintf(location, L"%s(%lu)", activeBuffer.getFileName(), line + lineOffset);
				bookmarksList_.setItemText(item, 1, location);
				bookmarksList_.setItemData(item, static_cast<DWORD>(reinterpret_cast<DWORD_PTR>(&activeBuffer)));
			}
		}
		bufferIndices_[const_cast<Buffer*>(&activeBuffer)] = app_.getBufferList().getActiveIndex();
	}

	if(bookmarksList_.getItemCount() != 0) {
		bookmarksList_.setItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		::EnableWindow(getDlgItem(IDOK), true);
		::EnableWindow(getDlgItem(IDC_BTN_DELETE), true);
	} else {
		::EnableWindow(getDlgItem(IDOK), false);
		::EnableWindow(getDlgItem(IDC_BTN_DELETE), false);
	}
}

/// @see Dialog::onClose
void BookmarkDlg::onClose() {
	app_.writeIntegerProfile(L"Search", L"BookmarkDialog.autoClose",
		(isDlgButtonChecked(IDC_CHK_AUTOCLOSE) == BST_CHECKED) ? 1 : 0);
	app_.writeIntegerProfile(L"Search", L"BookmarkDialog.allBuffers",
		(isDlgButtonChecked(IDC_CHK_SHOWALLFILES) == BST_CHECKED) ? 1 : 0);
	Dialog::onClose();
}

/// @see Dialog::onCommand
bool BookmarkDlg::onCommand(WORD id, WORD notifyCode, HWND control) {
	switch(id) {
	case IDC_BTN_ADD:	// [追加]
		app_.getBufferList().getActive().getBookmarker().mark(
			app_.getBufferList().getActiveView().getCaret().getLineNumber(), true);
		updateList();
		break;
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

/// @see Dialog::onInitDialog
bool BookmarkDlg::onInitDialog(HWND focusWindow, LPARAM initParam) {
	Dialog::onInitDialog(focusWindow, initParam);

	modifyStyleEx(0, WS_EX_LAYERED);
	setLayeredWindowAttributes(0, 220, LWA_ALPHA);

	bookmarksList_.modifyStyleEx(WS_EX_NOPARENTNOTIFY, 0);
	bookmarksList_.setExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
	bookmarksList_.insertColumn(0, app_.loadString(MSG_DIALOG__BOOKMARKED_LINE).c_str(), LVCFMT_LEFT, 279, -1);
	bookmarksList_.insertColumn(1, app_.loadString(MSG_DIALOG__BOOKMARKED_POSITION).c_str(), LVCFMT_LEFT, 100, -1);
	updateList();

	if(app_.readIntegerProfile(L"Search", L"BookmarkDialog.autoClose", 0) == 1)
		checkDlgButton(IDC_CHK_AUTOCLOSE, BST_CHECKED);
	if(app_.readIntegerProfile(L"Search", L"BookmarkDialog.allBuffers", 0) == 1)
		checkDlgButton(IDC_CHK_SHOWALLFILES, BST_CHECKED);

	return true;
}

/// @see Dialog::onNotify
bool BookmarkDlg::onNotify(int id, LPNMHDR nmhdr) {
	if(id== IDC_LIST_BOOKMARKS && nmhdr->code == NM_DBLCLK) {
		onOK();
		return true;
	}
	return Dialog::onNotify(id, nmhdr);
}

/// @see Dialog::OnOK
void BookmarkDlg::onOK() {
	// 一時マクロ定義中は実行できない
	if(app_.getCommandManager().getTemporaryMacro().getState() == command::TemporaryMacro::DEFINING) {
		app_.messageBox(MSG_ERROR__PROHIBITED_FOR_MACRO_DEFINING, MB_ICONEXCLAMATION);
		return;
	}

	const int sel = bookmarksList_.getSelectionMark();

	if(sel == -1)	// 選択が無い
		return;

	Buffer* buffer;
	length_t line;
	getItemInfo(sel, buffer, line);
	if(bufferIndices_.find(buffer) == bufferIndices_.end())
		return;

	app_.getBufferList().getActiveView().getCaret().moveTo(text::Position(line, 0));
	app_.getBufferList().setActive(bufferIndices_[buffer]);
	getParent().setActiveWindow();

	if(isDlgButtonChecked(IDC_CHK_AUTOCLOSE) == BST_CHECKED)	// [自動的に閉じる]
		Dialog::onOK();
}
