/**
 * @file about-dialog.cpp
 * @author exeal
 * @date 2004-2006
 */

#include "StdAfx.h"
#include "about-dialog.hpp"
using alpha::ui::AboutDlg;


namespace {
	const wchar_t HOME_PAGE_URL[] = L"http://alpha.sourceforge.jp/";
	const wchar_t PROJECT_PAGE_URL[] = L"http://sourceforge.jp/projects/alpha/";
}


bool AboutDlg::onCommand(WORD id, WORD notifyCode, HWND control) {
	if(id == IDC_LINK_HOMEPAGE) {
		::ShellExecuteW(0, 0, HOME_PAGE_URL, 0, 0, SW_SHOWNORMAL);
		return true;
	} else if(id == IDC_LINK_SOURCEFORGE) {
		::ShellExecuteW(0, 0, PROJECT_PAGE_URL, 0, 0, SW_SHOWNORMAL);
		return true;
	}

	return Dialog::onCommand(id, notifyCode, control);
}

bool AboutDlg::onInitDialog(HWND focusWindow, LPARAM initParam) {
	Dialog::onInitDialog(focusWindow, initParam);

	homePageLink_.create(*this, ::GetModuleHandle(0), IDC_LINK_HOMEPAGE);
	homePageLink_.setWindowText(HOME_PAGE_URL);
	homePageLink_.setWindowPos(0, 88, 98, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
	sourceForgeLink_.create(*this, ::GetModuleHandle(0), IDC_LINK_SOURCEFORGE);
	sourceForgeLink_.setWindowText(PROJECT_PAGE_URL);
	sourceForgeLink_.setWindowPos(0, 88, 114, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);

	addToolTip(homePageLink_, L"Home page");
	addToolTip(sourceForgeLink_, L"Project page");

	return true;
}
