/**
 * @file print.cpp
 * @author exeal
 * @date 2007
 */

#include "stdafx.h"
#include "print.hpp"
#include "application.hpp"
#include "resource/messages.h"
#include "../manah/win32/ui/dialog.hpp"
#include <commdlg.h>	// PrintDlgExW, SetupPageDlgW, ...
#include <shlwapi.h>	// PathCompactPathW
using namespace alpha;
using namespace std;


namespace {
	class PrintingRenderer : public ascension::layout::TextRenderer {
	public:
		PrintingRenderer(ascension::presentation::Presentation& presentation, HDC deviceContext,
				const ascension::layout::LayoutSettings& layoutSettings, int width)
				: TextRenderer(presentation, false), dc_(deviceContext), layoutSettings_(layoutSettings), width_(width) {
			layoutSettings_.lineWrap.mode = ascension::layout::LineWrapConfiguration::NORMAL;
		}
	private:
		// FontSelector
		auto_ptr<manah::win32::gdi::DC> getDeviceContext() const {
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

	class PrintingPrompt : public manah::win32::ui::FixedIDDialog<IDD_DLG_PRINTING> {
	public:
		explicit PrintingPrompt(const basic_string<::WCHAR>& bufferName) : bufferName_(bufferName) {}
		void setPageNumber(ulong page) {
			::WCHAR s[32];
			swprintf(s, L"%lu", page);
			setItemText(IDC_STATIC_2, s);
		}
	private:
		void onCancel(bool& continueDialog) {
			Printing::instance().abort();
			continueDialog = false;
		}
		void onInitDialog(HWND /* focusedWindow */, bool& /* focusDefault */) {
			setItemText(IDC_STATIC_1, bufferName_.c_str());
			setItemText(IDC_STATIC_2, L"0");
		}
	private:
		const basic_string<::WCHAR> bufferName_;
	};
} // namespace @0


// Printing /////////////////////////////////////////////////////////////////

/// Private constructor.
Printing::Printing() throw() : devmode_(0), devnames_(0),
		printsLineNumbers_(Alpha::instance().readIntegerProfile(L"Printing", L"printsLineNumbers", 1)),
		printsHeader_(Alpha::instance().readIntegerProfile(L"Printing", L"printsHeader", 1)),
		printing_(false), userAborted_(false) {
	doSetupPages(true);
}

/// Private destructor.
Printing::~Printing() throw() {
	::GlobalFree(devmode_);
	::GlobalFree(devnames_);
}

/// Aborts the active printing.
void Printing::abort() {
	if(printing_)
		userAborted_ = true;
}

/// A callback procedure for @c SetAbortDoc.
BOOL CALLBACK Printing::abortProcedure(HDC dc, int error) {
	if(error != 0 && error != SP_OUTOFDISK)
		return false;
	::MSG message;
	while(!Printing::instance().userAborted_ && ::PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
		if(message.message == WM_QUIT) {
			::PostQuitMessage(0);
			return false;
		}
		::TranslateMessage(&message);
		::DispatchMessage(&message);
	}
	return !Printing::instance().userAborted_;
}

/// Displays "Page Setup" dialog box.
bool Printing::doSetupPages(bool returnDefault) {
	::PAGESETUPDLGW psd;
	memset(&psd, 0, sizeof(::PAGESETUPDLGW));
	psd.lStructSize = sizeof(::PAGESETUPDLGW);
	psd.hwndOwner = Alpha::instance().getMainWindow().getHandle();
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

/**
 * Prints the specified buffer.
 * @param buffer the buffer to print
 * @param showDialog
 */
bool Printing::print(const Buffer& buffer, bool showDialog) {
	if(printing_)
		return false;
	printing_ = true;

	// display "Print" dialog box
	::PRINTDLGEXW pdex;
	memset(&pdex, 0, sizeof(::PRINTDLGEXW));
	pdex.lStructSize = sizeof(::PRINTDLGEXW);
	pdex.hwndOwner = Alpha::instance().getMainWindow().getHandle();
	pdex.hDevMode = devmode_;
	pdex.hDevNames = devnames_;
	pdex.Flags = (showDialog ? (PD_COLLATE | PD_NOCURRENTPAGE | PD_NOPAGENUMS | PD_NOSELECTION) : PD_RETURNDEFAULT) | PD_RETURNDC;
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
	static const int MM100_PER_INCH = 2540;			// 1 in = 2.540 mm
	const int xdpi = dc.getDeviceCaps(LOGPIXELSX);	// resolutions in px/in
	const int ydpi = dc.getDeviceCaps(LOGPIXELSY);
	const ::POINT physicalOffsetInMM = {dc.getDeviceCaps(PHYSICALOFFSETX), dc.getDeviceCaps(PHYSICALOFFSETY)};
	const ::POINT physicalOffset = {	// physical offsets in mm
		::MulDiv(physicalOffsetInMM.x, MM100_PER_INCH, xdpi), ::MulDiv(physicalOffsetInMM.y, MM100_PER_INCH, ydpi)};
	paperSize_.cx = ::MulDiv(dc.getDeviceCaps(PHYSICALWIDTH), MM100_PER_INCH, xdpi);
	paperSize_.cy = ::MulDiv(dc.getDeviceCaps(PHYSICALHEIGHT), MM100_PER_INCH, ydpi);
	margins_.left = max<long>(margins_.left, physicalOffset.x);
	margins_.top = max<long>(margins_.top, physicalOffset.y);
	margins_.right = max<long>(margins_.right,
		paperSize_.cx - ::MulDiv(dc.getDeviceCaps(HORZRES), MM100_PER_INCH, xdpi) - margins_.left);
	margins_.bottom = max<long>(margins_.bottom,
		paperSize_.cy - ::MulDiv(dc.getDeviceCaps(VERTRES), MM100_PER_INCH, ydpi) - margins_.right);

#define MM100_TO_PIXELS_X(mm100)	::MulDiv(mm100, xdpi, MM100_PER_INCH)
#define MM100_TO_PIXELS_Y(mm100)	::MulDiv(mm100, ydpi, MM100_PER_INCH)

	// reset fonts
	PrintingRenderer renderer(const_cast<ascension::presentation::Presentation&>(buffer.presentation()),
		dc.getHandle(), (*buffer.presentation().firstTextViewer())->configuration(),
		MM100_TO_PIXELS_X(paperSize_.cx - margins_.left - margins_.right));
	::LOGFONTW lf;
	::GetObject((*buffer.presentation().firstTextViewer())->textRenderer().font(), sizeof(::LOGFONTW), &lf);
	manah::win32::gdi::ScreenDC screenDC;
	renderer.setFont(lf.lfFaceName, ::MulDiv(lf.lfHeight, ydpi, screenDC.getDeviceCaps(LOGPIXELSY)), 0);

	// start printing
	dc.setAbortProc(abortProcedure);
	const basic_string<::WCHAR> bufferName(buffer.textFile().isOpen() ? buffer.textFile().location() : buffer.name());
	::DOCINFOW di;
	memset(&di, 0, sizeof(::DOCINFOW));
	di.lpszDocName = bufferName.c_str();
	if(dc.startDoc(di) == SP_ERROR) {
		printing_ = false;
		return false;
	}

	PrintingPrompt prompt(bufferName);
	Alpha::instance().getMainWindow().enable(false);
	prompt.doModeless(Alpha::instance().getMainWindow());

	// calculate compacted path name
	::RECT rc = {MM100_TO_PIXELS_X(margins_.left), 0,
		MM100_TO_PIXELS_X(paperSize_.cx - margins_.right), MM100_TO_PIXELS_Y(paperSize_.cy - margins_.top - margins_.bottom)};
	::HFONT oldFont = dc.selectObject(renderer.font());
	::WCHAR compactedPathName[MAX_PATH];
	wcscpy(compactedPathName, bufferName.c_str());
	::PathCompactPathW(pdex.hDC, compactedPathName, (rc.right - rc.left) * 9 / 10);

	// create a pen draws separators
	int separatorThickness;
	if(!ascension::layout::getDecorationLineMetrics(dc.getHandle(), 0, 0, &separatorThickness, 0, 0))
		separatorThickness = 1;
	HPEN separatorPen = ::CreatePen(PS_SOLID, separatorThickness, RGB(0x00, 0x00, 0x00));
	dc.selectObject(oldFont);

	// print lines on pages
	bool error = false;
	ulong page = 0;
	::WCHAR pageNumber[128];
	const int linePitch = renderer.linePitch();
	const ascension::layout::Alignment alignment = (*buffer.presentation().firstTextViewer())->configuration().alignment;
	rc.top = rc.bottom;
	for(ascension::length_t line = 0, lines = buffer.numberOfLines(); !error && line < lines; ++line) {
		const ascension::layout::LineLayout& layout = renderer.lineLayout(line);
		for(ascension::length_t subline = 0; !error && subline < layout.numberOfSublines(); ++subline) {
			if(rc.top + linePitch > rc.bottom) {
				// go to the next page
				if(++page > 1) {
					if(dc.endPage() == SP_ERROR) {
						error = true;
						break;
					}
				}
				if(!toBoolean(abortProcedure(dc.getHandle(), 0)) || dc.startPage() == SP_ERROR) {
					error = true;
					break;
				}
				prompt.setPageNumber(page);
				dc.setViewportOrg(-physicalOffsetInMM.x, -physicalOffsetInMM.y);
				// print a header
				::HFONT oldFont = dc.selectObject(renderer.font());
				dc.setTextAlign(TA_LEFT | TA_TOP | TA_NOUPDATECP);
				dc.textOut(rc.left, rc.top = MM100_TO_PIXELS_Y(margins_.top), compactedPathName, static_cast<int>(wcslen(compactedPathName)));
				swprintf(pageNumber, L"%lu", page);
				dc.setTextAlign(TA_RIGHT | TA_TOP | TA_NOUPDATECP);
				dc.textOut(rc.right, rc.top, pageNumber, static_cast<int>(wcslen(pageNumber)));
				dc.selectObject(oldFont);
				::HPEN oldPen = dc.selectObject(separatorPen);
				dc.moveTo(rc.left, rc.top + linePitch + separatorThickness / 2);
				dc.lineTo(rc.right, rc.top + linePitch + separatorThickness / 2);
				dc.selectObject(oldPen);
				rc.top += linePitch * 2;
			}
			switch(alignment) {
			case ascension::layout::ALIGN_LEFT:
				layout.draw(subline, dc, rc.left, rc.top, rc, rc, 0);
				break;
			case ascension::layout::ALIGN_RIGHT:
				layout.draw(subline, dc, rc.right - layout.sublineWidth(0), rc.top, rc, rc, 0);
				break;
			case ascension::layout::ALIGN_CENTER:
				layout.draw(subline, dc, (rc.right - layout.sublineWidth(0)) / 2, rc.top, rc, rc, 0);
				break;
			}
			rc.top += linePitch;
		}
	}

#undef MM100_TO_PIXELS_X
#undef MM100_TO_PIXELS_Y

	if(!error && !userAborted_) {
		dc.endPage();
		dc.endDoc();
		prompt.end(IDOK);
	} else
		dc.abortDoc();
	::DeleteObject(separatorPen);
	Alpha::instance().getMainWindow().enable(true);

	printing_ = userAborted_ = false;
	return !error;
}

/// Displays "Page Setup" dialog box.
bool Printing::setupPages() {
	return doSetupPages(false);
}
