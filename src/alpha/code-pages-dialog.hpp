/**
 * @file code-pages-dialog.hpp
 * @author exeal
 * @date 2003-2007
 */

#ifndef ALPHA_CODE_PAGES_DIALOG_HPP
#define ALPHA_CODE_PAGES_DIALOG_HPP
#include "resource.h"
#include "../manah/win32/ui/dialog.hpp"
#include "../manah/win32/ui/standard-controls.hpp"
#include "ascension/encoder.hpp"


namespace alpha {
	namespace ui {
		/// [コードページの選択] ダイアログ
		class CodePagesDialog : public manah::windows::ui::FixedIDDialog<IDD_DLG_CODEPAGES> {
		public:
			CodePagesDialog(ascension::encodings::CodePage codePage, bool forReading) throw();
			ascension::encodings::CodePage	getCodePage() const throw();

		private:
			ascension::encodings::CodePage codePage_;
			const bool forReading_;
			manah::windows::ui::ListBox codepageList_;
			BEGIN_CONTROL_BINDING()
				BIND_CONTROL(IDC_LIST_CODEPAGES, codepageList_)
			END_CONTROL_BINDING()
			bool	onCommand(WORD id, WORD notifyCode, HWND control);	// WM_COMMAND
			bool	onInitDialog(HWND focusWindow, LPARAM initParam);	// WM_INITDIALOG
			void	onOK();												// IDOK
		};

		/// ユーザが選択したコードページを返す
		inline ascension::encodings::CodePage CodePagesDialog::getCodePage() const throw() {return codePage_;}
	}
}

#endif /* !ALPHA_CODE_PAGES_DIALOG_HPP */
