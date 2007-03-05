/**
 * @file about-dialog.hpp
 * @author exeal
 * @date 2003-2007
 */

#ifndef ALPHA_ABOUT_DIALOG_HPP
#define ALPHA_ABOUT_DIALOG_HPP
#include "resource.h"
#include "../manah/win32/ui/dialog.hpp"
#include "../manah/win32/ui/link-label.hpp"


namespace alpha {
	namespace ui {
		/// 「バージョン情報」ダイアログ
		class AboutDialog : public manah::win32::ui::FixedIDDialog<IDD_DLG_ABOUT> {
			// データメンバ
		private:
			manah::win32::ui::LinkLabel homePageLink_;
			manah::win32::ui::LinkLabel sourceForgeLink_;

			// メッセージハンドラ
		protected:
			virtual bool	onCommand(WORD id, WORD notifyCode, HWND control);	// WM_COMMAND
			virtual bool	onInitDialog(HWND focusWindow, LPARAM initParam);	// WM_INITDIALOG
		};
	}
}

#endif /* !ALPHA_ABOUT_DIALOG_HPP */
