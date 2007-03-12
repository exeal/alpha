/**
 * @file goto-line-dialog.hpp
 * @author exeal
 * @date 2003-2007
 */

#ifndef ALPHA_GOTO_LINE_DIALOG_HPP
#define ALPHA_GOTO_LINE_DIALOG_HPP
#include "resource.h"
#include "../manah/win32/ui/dialog.hpp"


namespace alpha {
	class Alpha;

	namespace ui {
		/// "Go to Line" Dialog.
		class GotoLineDialog : public manah::win32::ui::FixedIDDialog<IDD_DLG_GOTOLINE> {
		protected:
			void	onInitDialog(HWND focusWindow, bool& focusDefault);	// WM_INITDIALOG
			void	onOK(bool& continueDialog);							// IDOK
		private:
			manah::win32::ui::UpDownCtrl lineNumberSpin_;
			MANAH_BEGIN_CONTROL_BINDING()
				MANAH_BIND_CONTROL(IDC_SPIN_LINENUMBER, lineNumberSpin_)
			MANAH_END_CONTROL_BINDING()
		};
	}
}

#endif /* !ALPHA_GOTO_LINE_DIALOG_HPP */
