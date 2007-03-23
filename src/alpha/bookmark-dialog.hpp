/**
 * @file bookmark-dialog.hpp
 * @author exeal
 * @date 2003-2007
 */

#ifndef ALPHA_BOOKMARK_DIALOG_HPP
#define ALPHA_BOOKMARK_DIALOG_HPP
#include "resource.h"
#include "ascension/common.hpp"
#include "../manah/win32/ui/dialog.hpp"
#include <map>


namespace alpha {
	class Buffer;

	namespace ui {
		///	"Bookmarks" dialog box.
		class BookmarkDialog : public manah::win32::ui::FixedIDDialog<IDD_DLG_BOOKMARKS> {
		public:
			void	updateList();

		private:
			void	getItemInfo(int index, Buffer*& buffer, ascension::length_t& line) const;
			void	onBtnDelete();
		protected:
			void	onClose(bool& continueDialog);						// WM_CLOSE
			bool	onCommand(WORD id, WORD notifyCode, HWND control);	// WM_COMMAND
			void	onInitDialog(HWND focusWindow, bool& focusDefault);	// WM_INITDIALOG
			bool	onNotify(int id, ::NMHDR& nmhdr);					// WM_NOTIFY
			void	onOK(bool& continueDialog);							// IDOK

			// データメンバ
		private:
			std::map<Buffer*, std::size_t> bufferIndices_;	// バッファ -> バッファ番号
			manah::win32::ui::ListCtrl bookmarksList_;

			MANAH_BEGIN_CONTROL_BINDING()
				MANAH_BIND_CONTROL(IDC_LIST_BOOKMARKS, bookmarksList_)
			MANAH_END_CONTROL_BINDING()
		};
	}
}

#endif /* !ALPHA_BOOKMARK_DIALOG_HPP */
