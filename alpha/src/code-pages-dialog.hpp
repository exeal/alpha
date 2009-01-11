/**
 * @file code-pages-dialog.hpp
 * @author exeal
 * @date 2003-2009
 */

#ifndef ALPHA_CODE_PAGES_DIALOG_HPP
#define ALPHA_CODE_PAGES_DIALOG_HPP
#include "resource.h"
#include <manah/win32/ui/dialog.hpp>
#include <manah/win32/ui/standard-controls.hpp>
#include "ascension/encoder.hpp"


namespace alpha {
	namespace ui {
		/// "Select Encoding" dialog box.
		class EncodingsDialog : public manah::win32::ui::FixedIDDialog<IDD_DLG_CODEPAGES> {
		public:
			EncodingsDialog(const std::string& encoding, bool forReading) /*throw()*/;
			const std::string& resultEncoding() const /*throw()*/;
		private:
			std::string result_;
			const bool forReading_;
			manah::win32::ui::ListBox encodingList_;
			MANAH_BEGIN_CONTROL_BINDING()
				MANAH_BIND_CONTROL(IDC_LIST_CODEPAGES, encodingList_)
			MANAH_END_CONTROL_BINDING()
			bool onCommand(WORD id, WORD notifyCode, HWND control);		// WM_COMMAND
			void onInitDialog(HWND focusWindow, bool& focusDefault);	// WM_INITDIALOG
			void onOK(bool& continueDialog);							// IDOK
		};

		/// Returns the encoding the user selected.
		inline const std::string& EncodingsDialog::resultEncoding() const /*throw()*/ {return result_;}
	}
}

#endif // !ALPHA_CODE_PAGES_DIALOG_HPP
