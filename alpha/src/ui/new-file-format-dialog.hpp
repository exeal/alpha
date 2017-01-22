/**
 * @file new-file-format-dialog.hpp
 * @author exeal
 * @date 2003-2009
 */

#ifndef ALPHA_NEW_FILE_FORMAT_DIALOG_HPP
#define ALPHA_NEW_FILE_FORMAT_DIALOG_HPP
#include "resource.h"
#include <manah/win32/ui/dialog.hpp>
#include <manah/win32/ui/standard-controls.hpp>
#include <ascension/document.hpp>	// ascension.kernel.Newline


namespace alpha {
	namespace ui {
		/// "New with Format" dialog box.
		class NewFileFormatDialog : public manah::win32::ui::FixedIDDialog<IDD_DLG_FILENEWWITHFORMAT> {
		public:
			NewFileFormatDialog(const std::string& encoding, ascension::kernel::Newline newline) throw();
			const std::string& encoding() const /*throw()*/;
			ascension::kernel::Newline newline() const /*throw()*/;
		private:
			bool onCommand(WORD id, WORD notifyCode, HWND control);
			void onInitDialog(HWND focusWindow, bool& focusDefault);
			void onOK(bool& continueDialog);
			std::string encoding_;
			ascension::kernel::Newline newline_;
			manah::win32::ui::ComboBox encodingCombobox_;
			manah::win32::ui::ComboBox newlineCombobox_;
			MANAH_BEGIN_CONTROL_BINDING()
				MANAH_BIND_CONTROL(IDC_COMBO_ENCODING, encodingCombobox_)
				MANAH_BIND_CONTROL(IDC_COMBO_NEWLINE, newlineCombobox_)
			MANAH_END_CONTROL_BINDING()
		};

		/// Returns the encoding the user selected.
		inline const std::string& NewFileFormatDialog::encoding() const /*throw()*/ {return encoding_;}

		/// Returns the newline the user selected.
		inline ascension::kernel::Newline NewFileFormatDialog::newline() const /*throw()*/ {return newline_;}

	} // namespace ui
} // namespace alpha

#endif // !ALPHA_NEW_FILE_FORMAT_DIALOG_HPP
