/**
 * @file about-dialog.cpp
 * Exposes @c about_dialog function to Python.
 * @author exeal
 * @date 2004-2009
 */

#include "application.hpp"	// alpha.Alpha.getMainWindow
#include "ambient.hpp"
#include "resource.h"
#include <manah/win32/ui/dialog.hpp>
#include <manah/win32/ui/link-label.hpp>

using namespace alpha::ambient;
namespace py = boost::python;


namespace {
	const wchar_t HOME_PAGE_URL[] = L"http://alpha.sourceforge.jp/";
	const wchar_t PROJECT_PAGE_URL[] = L"http://sourceforge.jp/projects/alpha/";

	/// "About Alpha" dialog box.
	class AboutDialog : public manah::win32::ui::FixedIDDialog<IDD_DLG_ABOUT> {
	private:
		bool onCommand(WORD id, WORD notifyCode, HWND control);
		void onInitDialog(HWND, bool&);
	private:
		manah::win32::ui::LinkLabel homePageLink_, sourceForgeLink_;
	};
}

/// @see Dialog#onCommand
bool AboutDialog::onCommand(WORD id, WORD notifyCode, HWND control) {
	if(id == IDC_LINK_HOMEPAGE) {
		::ShellExecuteW(0, 0, HOME_PAGE_URL, 0, 0, SW_SHOWNORMAL);
		return true;
	} else if(id == IDC_LINK_SOURCEFORGE) {
		::ShellExecuteW(0, 0, PROJECT_PAGE_URL, 0, 0, SW_SHOWNORMAL);
		return true;
	}

	return Dialog::onCommand(id, notifyCode, control);
}

/// @see Dialog#onInitDialog
void AboutDialog::onInitDialog(HWND, bool&) {
	homePageLink_.create(use(), ::GetModuleHandle(0), IDC_LINK_HOMEPAGE);
	homePageLink_.setText(HOME_PAGE_URL);
	homePageLink_.setPosition(0, 88, 98, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
	sourceForgeLink_.create(get(), ::GetModuleHandle(0), IDC_LINK_SOURCEFORGE);
	sourceForgeLink_.setText(PROJECT_PAGE_URL);
	sourceForgeLink_.setPosition(0, 88, 114, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
	addToolTip(homePageLink_.use(), L"Home page");
	addToolTip(sourceForgeLink_.use(), L"Project page");
}

namespace {
	void aboutDialog() {
		AboutDialog dialog;
		dialog.doModal(alpha::Alpha::instance().getMainWindow());
	}
}

ALPHA_EXPOSE_PROLOGUE(Interpreter::LOWEST_INSTALLATION_ORDER)
	py::scope temp(Interpreter::instance().module("ui"));

	py::def("about_dialog", &aboutDialog);
ALPHA_EXPOSE_EPILOGUE()
