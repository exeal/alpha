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
		/// "Select Code Page" dialog box.
		class CodePagesDialog : public manah::win32::ui::FixedIDDialog<IDD_DLG_CODEPAGES> {
		public:
			CodePagesDialog(ascension::encodings::CodePage codePage, bool forReading) throw();
			ascension::encodings::CodePage	getCodePage() const throw();

		private:
			ascension::encodings::CodePage codePage_;
			const bool forReading_;
			manah::win32::ui::ListBox codepageList_;
			MANAH_BEGIN_CONTROL_BINDING()
				MANAH_BIND_CONTROL(IDC_LIST_CODEPAGES, codepageList_)
			MANAH_END_CONTROL_BINDING()
			bool	onCommand(WORD id, WORD notifyCode, HWND control);	// WM_COMMAND
			void	onInitDialog(HWND focusWindow, bool& focusDefault);	// WM_INITDIALOG
			void	onOK(bool& continueDialog);							// IDOK
		};

		/// ユーザが選択したコードページを返す
		inline ascension::encodings::CodePage CodePagesDialog::getCodePage() const throw() {return codePage_;}
	}
}

#endif /* !ALPHA_CODE_PAGES_DIALOG_HPP */
