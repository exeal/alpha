/**
 * @file select-language-dialog.hpp
 * @author exeal
 * @date 2003-2007
 */

#ifndef ALPHA_SELECT_LANGUAGE_DIALOG_HPP
#define ALPHA_SELECT_LANGUAGE_DIALOG_HPP
#include "resource.h"
#include "../manah/win32/ui/dialog.hpp"
#include "../manah/win32/ui/standard-controls.hpp"


namespace alpha {
	namespace ui {
		/// "Select Script Language" dialog box.
		class SelectLanguageDialog : public manah::win32::ui::FixedIDDialog<IDD_DLG_SELECTLANGUAGE> {
		public:
			explicit SelectLanguageDialog(const std::basic_string<WCHAR>& scriptName);
			const std::wstring&	resultLanguage() const throw();

		protected:
			bool	onCommand(WORD wID, WORD wNotifyCode, HWND hwndCtrl);	// WM_COMMAND
			void	onInitDialog(HWND hwndFocus, bool& focusDefault);		// WM_INITDIALOG
			void	onOK(bool& continueDialog);								// IDOK
		private:
			std::basic_string<WCHAR> scriptName_;
			std::wstring selectedLanguage_;
			manah::win32::ui::ListBox languageListbox_;
			MANAH_BEGIN_CONTROL_BINDING()
				MANAH_BIND_CONTROL(IDC_LIST_SCRIPTENGINES, languageListbox_)
			MANAH_END_CONTROL_BINDING()
		};

		/// Returns the language the user selected.
		inline const std::wstring& SelectLanguageDialog::resultLanguage() const throw() {return selectedLanguage_;}
	}
}

#endif /* !ALPHA_SELECT_LANGUAGE_DIALOG_HPP */
