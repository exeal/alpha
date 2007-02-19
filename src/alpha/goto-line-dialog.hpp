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
		/// [�w��s�ֈړ�] �_�C�A���O
		class GotoLineDialog : public manah::windows::ui::FixedIDDialog<IDD_DLG_GOTOLINE> {
		public:
			GotoLineDialog(Alpha& app);

		protected:
			bool	onInitDialog(HWND focusWindow, LPARAM initParam);	// WM_INITDIALOG
			void	onOK();												// IDOK
		private:
			Alpha& app_;
			manah::windows::ui::UpDownCtrl lineNumberSpin_;
			BEGIN_CONTROL_BINDING()
				BIND_CONTROL(IDC_SPIN_LINENUMBER, lineNumberSpin_)
			END_CONTROL_BINDING()
		};
	}
}

#endif /* !ALPHA_GOTO_LINE_DIALOG_HPP */
