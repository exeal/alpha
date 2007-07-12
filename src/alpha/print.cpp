/**
 * @file print.cpp
 * @author exeal
 * @date 2007
 */

#include "stdafx.h"
#include "print.hpp"
#include "application.hpp"
#include <commdlg.h>	// PrintDlgExW, SetupPageDlgW, ...
#include <shlwapi.h>	// PathCompactPathW
using namespace alpha;
using namespace std;


class PrintingRenderer : public ascension::layout::TextRenderer {
public:
	PrintingRenderer(ascension::presentation::Presentation& presentation, HDC deviceContext,
			const ascension::layout::LayoutSettings& layoutSettings, int width)
			: TextRenderer(presentation, false), dc_(deviceContext), layoutSettings_(layoutSettings), width_(width) {
		layoutSettings_.lineWrap.mode = ascension::layout::LineWrapConfiguration::NORMAL;
	}
private:
	// FontSelector
	auto_ptr<manah::win32::gdi::DC> doGetDeviceContext() const {
		auto_ptr<manah::win32::gdi::DC> temp(new manah::win32::gdi::DC());
		temp->attach(dc_);
		return temp;
	}
	// ILayoutInformationProvider
	const ascension::layout::LayoutSettings& getLayoutSettings() const throw() {
		return layoutSettings_;
	}
	int getWidth() const throw() {
		return width_;
	}
private:
	HDC dc_;
	ascension::layout::LayoutSettings layoutSettings_;
	int width_;
};


// Printing /////////////////////////////////////////////////////////////////

/// Private constructor.
Printing::Printing() throw() : devmode_(0), devnames_(0) {
	doSetupPages(true);
}

/// Private destructor.
Printing::~Printing() throw() {
	::GlobalFree(devmode_);
	::GlobalFree(devnames_);
}

/// Displays "Page Setup" dialog box.
bool Printing::doSetupPages(bool returnDefault) {
	::PAGESETUPDLGW psd;
	memset(&psd, 0, sizeof(::PAGESETUPDLGW));
	psd.lStructSize = sizeof(::PAGESETUPDLGW);
	psd.hwndOwner = Alpha::getInstance().getMainWindow().getHandle();
	psd.hDevMode = devmode_;
	psd.hDevNames = devnames_;
	psd.Flags = PSD_DEFAULTMINMARGINS | PSD_INHUNDREDTHSOFMILLIMETERS | PSD_SHOWHELP;
	if(returnDefault)
		psd.Flags |= PSD_RETURNDEFAULT;
	else {
		psd.Flags |= PSD_MARGINS;
		psd.ptPaperSize.x = paperSize_.cx;
		psd.ptPaperSize.y = paperSize_.cy;
		psd.rtMargin = margins_;
	}
	if(!toBoolean(::PageSetupDlgW(&psd)))
		return false;
	if(psd.hDevMode != devmode_)
		::GlobalFree(devmode_);
	devmode_ = psd.hDevMode;
	if(psd.hDevNames != devnames_)
		::GlobalFree(devnames_);
	paperSize_.cx = psd.ptPaperSize.x;
	paperSize_.cy = psd.ptPaperSize.y;
	margins_ = psd.rtMargin;
	return true;
}

/// Returns the singleton instance.
Printing& Printing::instance() throw() {
	static Printing singleton;
	return singleton;
}

/// Prepares the device context for the current printer.
void Printing::prepareDeviceContext() {
	assert(devnames_ != 0);
	const ::DEVNAMES& devnames = *static_cast<::DEVNAMES*>(::GlobalLock(devnames_));
	const ::DEVMODEW* devmode = static_cast<::DEVMODE*>(::GlobalLock(devmode_));
	HDC newDC = ::CreateDCW(
		reinterpret_cast<const ::WCHAR*>(&devnames) + devnames.wDriverOffset,
		reinterpret_cast<const ::WCHAR*>(&devnames) + devnames.wDeviceOffset,
		reinterpret_cast<const ::WCHAR*>(&devnames) + devnames.wOutputOffset, devmode);
	if(newDC != 0) {
		::ResetDCW(newDC, devmode);
/*		deviceContext_.reset(newDC);
//		deviceContext_.setMapMode(MM_HIMETRIC);
		::SetMapMode(deviceContext_.getHandle(), MM_HIMETRIC);
		static const int MM100_PER_INCH = 2540;
		const int xdpi = deviceContext_.getDeviceCaps(LOGPIXELSX);	// this unit is px/in...
		const int ydpi = deviceContext_.getDeviceCaps(LOGPIXELSY);
		paperSize_.cx = ::MulDiv(deviceContext_.getDeviceCaps(PHYSICALWIDTH), MM100_PER_INCH, xdpi);
		paperSize_.cy = ::MulDiv(deviceContext_.getDeviceCaps(PHYSICALHEIGHT), MM100_PER_INCH, ydpi);
		margins_.left = ::MulDiv(deviceContext_.getDeviceCaps(PHYSICALOFFSETX), MM100_PER_INCH, xdpi);
		margins_.top = ::MulDiv(deviceContext_.getDeviceCaps(PHYSICALOFFSETY), MM100_PER_INCH, ydpi);
		margins_.right = paperSize_.cx - ::MulDiv(deviceContext_.getDeviceCaps(HORZRES), MM100_PER_INCH, xdpi) - margins_.left;
		margins_.bottom = paperSize_.cy - ::MulDiv(deviceContext_.getDeviceCaps(VERTRES), MM100_PER_INCH, ydpi) - margins_.right;
*/	}
	::GlobalUnlock(devnames_);
	::GlobalUnlock(devmode_);
}

/**
 * @param buffer the buffer to print
 */
bool Printing::print(const Buffer& buffer) {
	// display "Print" dialog box
	::PRINTDLGEXW pdex;
	memset(&pdex, 0, sizeof(::PRINTDLGEXW));
	pdex.lStructSize = sizeof(::PRINTDLGEXW);
	pdex.hwndOwner = Alpha::getInstance().getMainWindow().getHandle();
	pdex.hDevMode = devmode_;
	pdex.hDevNames = devnames_;
	pdex.Flags = PD_COLLATE | PD_NOCURRENTPAGE | PD_NOPAGENUMS | PD_NOSELECTION | PD_RETURNDC;
	pdex.nStartPage = START_PAGE_GENERAL;
	if(S_OK != ::PrintDlgEx(&pdex))
		return false;
	else if(pdex.dwResultAction == PD_RESULT_CANCEL)
		return true;

	// update devmode_ and devnames_
	if(pdex.hDevMode != devmode_)
		::GlobalFree(devmode_);
	devmode_ = pdex.hDevMode;
	if(pdex.hDevNames != devnames_)
		::GlobalFree(devnames_);
	devnames_ = pdex.hDevNames;

	// update metrics
	manah::win32::gdi::DC dc;
	dc.attach(pdex.hDC);
	static const int MM100_PER_INCH = 2541;			// 1 in = 2.541 mm
	const int xdpi = dc.getDeviceCaps(LOGPIXELSX);	// this unit is px/in...
	const int ydpi = dc.getDeviceCaps(LOGPIXELSY);
	paperSize_.cx = ::MulDiv(dc.getDeviceCaps(PHYSICALWIDTH), MM100_PER_INCH, xdpi);
	paperSize_.cy = ::MulDiv(dc.getDeviceCaps(PHYSICALHEIGHT), MM100_PER_INCH, ydpi);
	margins_.left = max<long>(margins_.left, ::MulDiv(dc.getDeviceCaps(PHYSICALOFFSETX), MM100_PER_INCH, xdpi));
	margins_.top = max<long>(margins_.top, ::MulDiv(dc.getDeviceCaps(PHYSICALOFFSETY), MM100_PER_INCH, ydpi));
	margins_.right = max<long>(margins_.right,
		paperSize_.cx - ::MulDiv(dc.getDeviceCaps(HORZRES), MM100_PER_INCH, xdpi) - margins_.left);
	margins_.bottom = max<long>(margins_.bottom,
		paperSize_.cy - ::MulDiv(dc.getDeviceCaps(VERTRES), MM100_PER_INCH, ydpi) - margins_.right);

#define MM100_TO_PIXELS_X(mm100)	::MulDiv(mm100, xdpi, MM100_PER_INCH)
#define MM100_TO_PIXELS_Y(mm100)	::MulDiv(mm100, ydpi, MM100_PER_INCH)

	// reset fonts
	PrintingRenderer renderer(const_cast<ascension::presentation::Presentation&>(buffer.getPresentation()),
		dc.getHandle(), (*buffer.getPresentation().getFirstTextViewer())->getConfiguration(),
		MM100_TO_PIXELS_X(paperSize_.cx - margins_.left - margins_.right));
	::LOGFONTW lf;
	::GetObject((*buffer.getPresentation().getFirstTextViewer())->getTextRenderer().getFont(), sizeof(::LOGFONTW), &lf);
	manah::win32::gdi::ScreenDC screenDC;
	renderer.setFont(lf.lfFaceName, ::MulDiv(lf.lfHeight, ydpi, screenDC.getDeviceCaps(LOGPIXELSY)), 0);

	// calculate compacted path name
	::RECT rc = {MM100_TO_PIXELS_X(margins_.left), 0,
		MM100_TO_PIXELS_X(paperSize_.cx - margins_.right), MM100_TO_PIXELS_Y(paperSize_.cy - margins_.bottom)};
	HFONT oldFont = dc.selectObject(renderer.getFont());
	WCHAR compactedPathName[MAX_PATH];
	wcscpy(compactedPathName, buffer.isBoundToFile() ?
		buffer.getFilePathName() : Alpha::getInstance().loadString(MSG_BUFFER__UNTITLED).c_str());
	::PathCompactPathW(pdex.hDC, compactedPathName, (rc.right - rc.left) * 9 / 10);

	// create a pen draws separators
	int separatorThickness;
	if(!ascension::layout::getDecorationLineMetrics(dc.getHandle(), 0, 0, &separatorThickness, 0, 0))
		separatorThickness = 1;
	HPEN separatorPen = ::CreatePen(PS_SOLID, separatorThickness, RGB(0x00, 0x00, 0x00));
	dc.selectObject(oldFont);

	// start printing
	::DOCINFOW di;
	memset(&di, 0, sizeof(::DOCINFOW));
	di.lpszDocName = buffer.getFilePathName();
	::StartDocW(pdex.hDC, &di);

	// print lines on pages
	ulong page = 0;
	WCHAR pageNumber[128];
	const int linePitch = renderer.getLinePitch();
	rc.top= rc.bottom;
	for(ascension::length_t line = 0, lines = buffer.getNumberOfLines(); line < lines; ++line) {
		const ascension::layout::LineLayout& layout = renderer.getLineLayout(line);
		for(ascension::length_t subline = 0; subline < layout.getNumberOfSublines(); ++subline) {
			if(rc.top + linePitch > rc.bottom) {
				// go to the next page
				if(++page > 1)
					::EndPage(pdex.hDC);
				::StartPage(pdex.hDC);
				rc.top = MM100_TO_PIXELS_Y(margins_.top);
				// print a header
				dc.setTextAlign(TA_LEFT | TA_TOP | TA_NOUPDATECP);
				dc.textOut(rc.left, rc.top, compactedPathName, static_cast<int>(wcslen(compactedPathName)));
				swprintf(pageNumber, L"%lu", page);
				dc.setTextAlign(TA_RIGHT | TA_TOP | TA_NOUPDATECP);
				dc.textOut(rc.right, rc.top, pageNumber, static_cast<int>(wcslen(pageNumber)));
				HPEN oldPen = dc.selectObject(separatorPen);
				dc.moveTo(rc.left, rc.top + linePitch + separatorThickness / 2);
				dc.lineTo(rc.right, rc.top + linePitch + separatorThickness / 2);
				dc.selectObject(oldPen);
				rc.top += linePitch * 2;
			}
//			rc.bottom = rc.top + linePitch;
			layout.draw(subline, dc, rc.left, rc.top, rc, rc, 0);
			rc.top += linePitch;
		}
	}

#undef MM100_TO_PIXELS_X
#undef MM100_TO_PIXELS_Y

	::DeleteObject(separatorPen);
	::EndPage(pdex.hDC);
	::EndDoc(pdex.hDC);

	return true;
}

/// Displays "Page Setup" dialog box.
bool Printing::setupPages() {
	return doSetupPages(false);
}
