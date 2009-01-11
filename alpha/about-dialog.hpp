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
		/// "About" dialog box.
		class AboutDialog : public manah::win32::ui::FixedIDDialog<IDD_DLG_ABOUT> {
		private:
			manah::win32::ui::LinkLabel homePageLink_;
			manah::win32::ui::LinkLabel sourceForgeLink_;
		protected:
			virtual bool	onCommand(WORD id, WORD notifyCode, HWND control);
			virtual void	onInitDialog(HWND focusWindow, bool& focusDefault);
		};
	}
}

#endif /* !ALPHA_ABOUT_DIALOG_HPP */
