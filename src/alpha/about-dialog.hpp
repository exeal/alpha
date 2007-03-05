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
		/// �u�o�[�W�������v�_�C�A���O
		class AboutDialog : public manah::win32::ui::FixedIDDialog<IDD_DLG_ABOUT> {
			// �f�[�^�����o
		private:
			manah::win32::ui::LinkLabel homePageLink_;
			manah::win32::ui::LinkLabel sourceForgeLink_;

			// ���b�Z�[�W�n���h��
		protected:
			virtual bool	onCommand(WORD id, WORD notifyCode, HWND control);	// WM_COMMAND
			virtual bool	onInitDialog(HWND focusWindow, LPARAM initParam);	// WM_INITDIALOG
		};
	}
}

#endif /* !ALPHA_ABOUT_DIALOG_HPP */
