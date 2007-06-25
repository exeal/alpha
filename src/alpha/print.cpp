/**
 * @file print.cpp
 * @author exeal
 * @date 2007
 */

#include "stdafx.h"
#include "print.hpp"
#include "application.hpp"
#include <commdlg.h>
using namespace alpha;
using namespace std;


// Printing /////////////////////////////////////////////////////////////////

/// Private constructor.
Printing::Printing() throw() : devmode_(0), devnames_(0) {
	paperSize_.cx = paperSize_.cy = 0;
	::SetRect(&margins_, 0, 0, 0, 0);
}

/// Private destructor.
Printing::~Printing() throw() {
}

/// Returns the singleton instance.
Printing& Printing::instance() throw() {
	static Printing singleton;
	return singleton;
}

/**
 * @param buffer the buffer to print
 */
bool Printing::print(const Buffer& buffer) {
	::PRINTDLGEXW pdex;
	memset(&pdex, 0, sizeof(::PRINTDLGEXW));
	pdex.lStructSize = sizeof(::PRINTDLGEXW);
	pdex.hwndOwner = Alpha::getInstance().getMainWindow().getHandle();
	pdex.hDevMode = devmode_;
	pdex.hDevNames = devnames_;
	pdex.Flags = PD_COLLATE | PD_NOCURRENTPAGE | PD_NOPAGENUMS | PD_NOSELECTION | PD_RETURNDC;
	pdex.nStartPage = START_PAGE_GENERAL;
	if(FAILED(::PrintDlgExW(&pdex)))
		return false;

	if(pdex.dwResultAction == PD_RESULT_CANCEL)
		return true;
	devmode_ = pdex.hDevMode;
	devnames_ = pdex.hDevNames;
	paperSize_.cx = paperSize_.cy = 0;
	::SetRect(&margins_, 0, 0, 0, 0);
//	if(pdex.dwResultAction == PD_RESULT_APPLY)
		return true;
/*
	::DOCINFOW di;
	memset(&di, 0, sizeof(::DOCINFOW));
	di.lpszDocName = buffer.getFilePathName();
	::StartDocW(pdex.hDC, &di);
	::StartPage(pdex.hDC);
	::RECT rc;
	::SetRect(&rc, 0, 0, paperSize_.cx, paperSize_.cy);
	manah::win32::gdi::DC dc;
	dc.attach(pdex.hDC);
	(*buffer.getPresentation().getFirstTextViewer())->getTextRenderer().getLineLayout(0).draw(dc, 0, 0, rc, rc, ascension::viewers::Colors());
	::EndPage(pdex.hDC);
	::EndDoc(pdex.hDC);*/
}

bool Printing::setupPages() {
	::PAGESETUPDLGW psd;
	memset(&psd, 0, sizeof(::PAGESETUPDLGW));
	psd.lStructSize = sizeof(::PAGESETUPDLGW);
	psd.hwndOwner = Alpha::getInstance().getMainWindow().getHandle();
	psd.hDevMode = devmode_;
	psd.hDevNames = devnames_;
	psd.Flags = PSD_DEFAULTMINMARGINS | PSD_INHUNDREDTHSOFMILLIMETERS | PSD_SHOWHELP;
	psd.ptPaperSize.x = paperSize_.cx;
	psd.ptPaperSize.y = paperSize_.cy;
	psd.rtMargin = margins_;
	if(!toBoolean(::PageSetupDlgW(&psd)))
		return false;
	devmode_ = psd.hDevMode;
	devnames_ = psd.hDevNames;
	paperSize_.cx = psd.ptPaperSize.x;
	paperSize_.cy = psd.ptPaperSize.y;
	margins_ = psd.rtMargin;
	return true;
}
