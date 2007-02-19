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
		/// [言語エンジンの選択] ダイアログ
		class SelectLanguageDialog : public manah::windows::ui::FixedIDDialog<IDD_DLG_SELECTLANGUAGE> {
		public:
			explicit SelectLanguageDialog(const std::basic_string<WCHAR>& scriptName);
			const std::wstring&	getSelectedLanguage() const throw();

		protected:
			virtual bool	onCommand(WORD wID, WORD wNotifyCode, HWND hwndCtrl);	// WM_COMMAND
			virtual bool	onInitDialog(HWND hwndFocus, LPARAM lInitParam);		// WM_INITDIALOG
			virtual void	onOK();													// IDOK
		private:
			std::basic_string<WCHAR> scriptName_;
			std::wstring selectedLanguage_;
			manah::windows::ui::ListBox languageListbox_;
			BEGIN_CONTROL_BINDING()
				BIND_CONTROL(IDC_LIST_SCRIPTENGINES, languageListbox_)
			END_CONTROL_BINDING()
		};

		/// 選択された言語を返す
		inline const std::wstring& SelectLanguageDialog::getSelectedLanguage() const throw() {return selectedLanguage_;}
	}
}

#endif /* !ALPHA_SELECT_LANGUAGE_DIALOG_HPP */
