/**
 * @file bookmarks-dialog.cpp
 * Exposes @c bookmarks_dialog function to Python.
 * @author exeal
 * @date 2003-2009
 */

#include "application.hpp"
#include "resource.h"
#include <manah/win32/ui/dialog.hpp>
#include <manah/win32/ui/common-controls.hpp>

using namespace alpha;
using namespace ascension;
using namespace std;

namespace {
	///	"Bookmarks" dialog box.
	class BookmarksDialog : public manah::win32::ui::FixedIDDialog<IDD_DLG_BOOKMARKS> {
	public:
		void updateList();

	private:
		void item(int index, Buffer*& buffer, ascension::length_t& line) const;
		void onBtnDelete();
	private:
		void onClose(bool& continueDialog);
		bool onCommand(WORD id, WORD notifyCode, HWND control);
		void onInitDialog(HWND focusWindow, bool& focusDefault);
		bool onNotify(int id, NMHDR& nmhdr);
		void onOK(bool& continueDialog);
	private:
		std::map<Buffer*, size_t> bufferIndices_;	// バッファ -> バッファ番号
		manah::win32::Borrowed<manah::win32::ui::ListCtrl> list_;
		MANAH_BEGIN_CONTROL_BINDING()
			MANAH_BIND_CONTROL(IDC_LIST_BOOKMARKS, list_)
		MANAH_END_CONTROL_BINDING()
	};
}


/**
 * Returns the information of the specified item.
 * @param[in] index the index of the item
 * @param[out] buffer the buffer
 * @param[out] line the line number
 */
void BookmarksDialog::item(int index, Buffer*& buffer, length_t& line) const {
	wchar_t location[300];
	wchar_t* delimiter = 0;

	assert(index < list_->getItemCount());
	list_->getItemText(index, 1, location, 300);
	list_->getItemData(index);

	delimiter = wcsrchr(location, L'(');
	assert(delimiter != 0);
	line = wcstoul(delimiter + 1, 0, 10);
	--line;
}

/// Handles "Delete" button.
void BookmarksDialog::onBtnDelete() {
	const int sel = list_->getSelectionMark();
	if(sel == -1)	// no selection
		return;

	Buffer* buffer;
	length_t line;
	item(sel, buffer, line);
	if(bufferIndices_.find(buffer) == bufferIndices_.end())
		return;

	buffer->bookmarker().mark(line, false);
	list_->deleteItem(sel);
	if(list_->getItemCount() != 0)
		list_->setItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	else {
		::EnableWindow(getItem(IDOK), false);
		::EnableWindow(getItem(IDC_BTN_DELETE), false);
	}
}

/// Updates the list control.
void BookmarksDialog::updateList() {
	const BufferList& buffers = BufferList::instance();
	list<length_t> lines;
	Char location[300];
	int item = 0;

	bufferIndices_.clear();
	list_->deleteAllItems();

	if(isButtonChecked(IDC_CHK_SHOWALLFILES) == BST_CHECKED) {	// 全てのドキュメントを扱う場合
		for(size_t i = 0; i < buffers.numberOfBuffers(); ++i) {
			const Buffer& buffer = buffers.at(i);
			const length_t lineOffset = buffers.activeView().verticalRulerConfiguration().lineNumbers.startValue;
			const length_t topLine = buffer.accessibleRegion().first.line;
			const length_t bottomLine = buffer.accessibleRegion().second.line;
			length_t line = 0;

			while((line = buffer.bookmarker().next(line, Direction::FORWARD)) != INVALID_INDEX) {
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

		while((line = activeBuffer.bookmarker().next(line, Direction::FORWARD)) != INVALID_INDEX) {
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
void BookmarksDialog::onClose(bool&) {
	Alpha& app = Alpha::instance();
	// TODO: use other data persistence method.
	app.writeIntegerProfile(L"Search", L"BookmarksDialog.autoClose", (isButtonChecked(IDC_CHK_AUTOCLOSE) == BST_CHECKED) ? 1 : 0);
	app.writeIntegerProfile(L"Search", L"BookmarksDialog.allBuffers", (isButtonChecked(IDC_CHK_SHOWALLFILES) == BST_CHECKED) ? 1 : 0);
}

/// @see Dialog#onCommand
bool BookmarksDialog::onCommand(WORD id, WORD notifyCode, HWND control) {
	switch(id) {
	case IDC_BTN_ADD: {	// "Add"
		const BufferList& buffers = Alpha::instance().bufferList();
		buffers.active().bookmarker().mark(buffers.activeView().caret().lineNumber(), true);
		updateList();
		break;
	}
	case IDC_BTN_DELETE:	// "Delete"
		onBtnDelete();
		break;
	case IDC_BTN_UPDATE:	// "Update"
	case IDC_CHK_SHOWALLFILES:	// "Show All Buffers"
		updateList();
		break;
	}
	return Dialog::onCommand(id, notifyCode, control);
}

/// @see Dialog#onInitDialog
void BookmarksDialog::onInitDialog(HWND focusWindow, bool&) {
	modifyStyleEx(0, WS_EX_LAYERED);
	setLayeredAttributes(0, 220, LWA_ALPHA);

	Alpha& app = Alpha::instance();
	list_->modifyStyleEx(WS_EX_NOPARENTNOTIFY, 0);
	list_->setExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
	list_->insertColumn(0, app.loadMessage(MSG_DIALOG__BOOKMARKED_LINE).c_str(), LVCFMT_LEFT, 279, -1);
	list_->insertColumn(1, app.loadMessage(MSG_DIALOG__BOOKMARKED_POSITION).c_str(), LVCFMT_LEFT, 100, -1);
	updateList();

	// TODO: use other data persistence method.
	if(app.readIntegerProfile(L"Search", L"BookmarkDialog.autoClose", 0) == 1)
		checkButton(IDC_CHK_AUTOCLOSE, BST_CHECKED);
	if(app.readIntegerProfile(L"Search", L"BookmarkDialog.allBuffers", 0) == 1)
		checkButton(IDC_CHK_SHOWALLFILES, BST_CHECKED);
}

/// @see Dialog#onNotify
bool BookmarksDialog::onNotify(int id, NMHDR& nmhdr) {
	if(id == IDC_LIST_BOOKMARKS && nmhdr.code == NM_DBLCLK) {
		postMessage(WM_COMMAND, IDOK);
		return true;
	}
	return false;
}

/// @see Dialog#OnOK
void BookmarksDialog::onOK(bool& continueDialog) {
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
