/**
 * @file about-dialog.cpp
 * @author exeal
 * @date 2004-2007
 */

#include "stdafx.h"
#include "about-dialog.hpp"
using alpha::ui::AboutDialog;


namespace {
	const wchar_t HOME_PAGE_URL[] = L"http://alpha.sourceforge.jp/";
	const wchar_t PROJECT_PAGE_URL[] = L"http://sourceforge.jp/projects/alpha/";
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
	homePageLink_.create(get(), ::GetModuleHandle(0), IDC_LINK_HOMEPAGE);
	homePageLink_.setText(HOME_PAGE_URL);
	homePageLink_.setPosition(0, 88, 98, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
	sourceForgeLink_.create(get(), ::GetModuleHandle(0), IDC_LINK_SOURCEFORGE);
	sourceForgeLink_.setText(PROJECT_PAGE_URL);
	sourceForgeLink_.setPosition(0, 88, 114, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
	addToolTip(homePageLink_.get(), L"Home page");
	addToolTip(sourceForgeLink_.get(), L"Project page");
}
