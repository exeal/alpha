/**
 * @file new-file-format-dialog.hpp
 * @author exeal
 * @date 2003-2007
 */

#ifndef ALPHA_NEW_FILE_FORMAT_DIALOG_HPP
#define ALPHA_NEW_FILE_FORMAT_DIALOG_HPP
#include "resource.h"
#include "../manah/win32/ui/dialog.hpp"
#include "../manah/win32/ui/standard-controls.hpp"
#include "ascension/document.hpp"	// ascension::text::LineBreak


namespace alpha {
	namespace ui {
		/// "New with Format" dialog box.
		class NewFileFormatDialog : public manah::win32::ui::FixedIDDialog<IDD_DLG_FILENEWWITHFORMAT> {
		public:
			NewFileFormatDialog(ascension::encodings::CodePage encoding, ascension::text::Newline newline) throw();
			ascension::encodings::CodePage	getEncoding() const throw();
			ascension::text::Newline		getNewline() const throw();
		private:
			bool	onCommand(WORD id, WORD notifyCode, HWND control);
			void	onInitDialog(HWND focusWindow, bool& focusDefault);
			void	onOK(bool& continueDialog);
			ascension::encodings::CodePage encoding_;
			ascension::text::Newline newline_;
			manah::win32::ui::ComboBox codePageCombobox_;
			manah::win32::ui::ComboBox newlineCombobox_;
			MANAH_BEGIN_CONTROL_BINDING()
				MANAH_BIND_CONTROL(IDC_COMBO_ENCODING, codePageCombobox_)
				MANAH_BIND_CONTROL(IDC_COMBO_NEWLINE, newlineCombobox_)
			MANAH_END_CONTROL_BINDING()
		};

		/// Returns the encoding the user selected.
		inline ascension::encodings::CodePage NewFileFormatDialog::getEncoding() const throw() {return encoding_;}

		/// Returns the newline the user selected.
		inline ascension::text::Newline NewFileFormatDialog::getNewline() const throw() {return newline_;}

	} // namespace ui
} // namespace alpha

#endif /* !ALPHA_NEW_FILE_FORMAT_DIALOG_HPP */
