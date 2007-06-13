/**
 * @file layout.cpp
 * @author exeal
 * @date 2003-2006 (was LineLayout.cpp)
 * @date 2006-2007
 */

#include "stdafx.h"
#include "layout.hpp"
#include "viewer.hpp"
#include <limits>	// numeric_limits
using namespace ascension;
using namespace ascension::text;
using namespace ascension::viewers;
using namespace ascension::viewers::internal;
using namespace ascension::presentation;
using namespace ascension::unicode;
using namespace ascension::unicode::ucd;
using namespace manah::win32;
using namespace manah::win32::gdi;
using namespace std;

//#define TRACE_LAYOUT_CACHES
extern bool DIAGNOSE_INHERENT_DRAWING;

#pragma comment(lib, "usp10.lib")

namespace {
	ASCENSION_BEGIN_SHARED_LIB_ENTRIES(USPEntries, 14)
		ASCENSION_SHARED_LIB_ENTRY(0, "ScriptFreeCache", HRESULT(WINAPI *signature)(::SCRIPT_CACHE*))
		ASCENSION_SHARED_LIB_ENTRY(1, "ScriptItemize", HRESULT(WINAPI *signature)(const WCHAR*, int, int, const ::SCRIPT_CONTROL*, const ::SCRIPT_STATE*, ::SCRIPT_ITEM*, int*))
		ASCENSION_SHARED_LIB_ENTRY(2, "ScriptLayout", HRESULT(WINAPI *signature)(int, const ::BYTE*, int*, int*))
		ASCENSION_SHARED_LIB_ENTRY(3, "ScriptShape", HRESULT(WINAPI *signature)(HDC, ::SCRIPT_CACHE*, const WCHAR*, int, int, ::SCRIPT_ANALYSIS*, WORD*, WORD*, ::SCRIPT_VISATTR*, int*))
		ASCENSION_SHARED_LIB_ENTRY(4, "ScriptPlace", HRESULT(WINAPI *signature)(HDC, ::SCRIPT_CACHE*, const WORD*, int, const ::SCRIPT_VISATTR*, ::SCRIPT_ANALYSIS*, int*, ::GOFFSET*, ::ABC*))
		ASCENSION_SHARED_LIB_ENTRY(5, "ScriptTextOut", HRESULT(WINAPI *signature)(const HDC, ::SCRIPT_CACHE*, int, int, UINT, const ::RECT*, const ::SCRIPT_ANALYSIS*, const WCHAR*, int, const WORD*, int, const int*, const int*, const ::GOFFSET*))
		ASCENSION_SHARED_LIB_ENTRY(6, "ScriptJustify", HRESULT(WINAPI *signature)(const ::SCRIPT_VISATTR*, const int*, int, int, int, int*))
		ASCENSION_SHARED_LIB_ENTRY(7, "ScriptBreak", HRESULT(WINAPI *signature)(const WCHAR*, int, const ::SCRIPT_ANALYSIS*, ::SCRIPT_LOGATTR*))
		ASCENSION_SHARED_LIB_ENTRY(8, "ScriptCPtoX", HRESULT(WINAPI *signature)(int, BOOL, int, int, const WORD*, const ::SCRIPT_VISATTR*, const int*, const ::SCRIPT_ANALYSIS*, int*))
		ASCENSION_SHARED_LIB_ENTRY(9, "ScriptXtoCP", HRESULT(WINAPI *signature)(int, int, int, const WORD*, const ::SCRIPT_VISATTR*, const int*, const ::SCRIPT_ANALYSIS*, int*, int*))
		ASCENSION_SHARED_LIB_ENTRY(10, "ScriptGetLogicalWidths", HRESULT(WINAPI *signature)(const ::SCRIPT_ANALYSIS*, int, int, const int*, const WORD*, const ::SCRIPT_VISATTR*, int*))
		ASCENSION_SHARED_LIB_ENTRY(11, "ScriptGetProperties", HRESULT(WINAPI *signature)(const ::SCRIPT_PROPERTIES***, int*))
		ASCENSION_SHARED_LIB_ENTRY(12, "ScriptGetFontProperties", HRESULT(WINAPI *signature)(HDC, ::SCRIPT_CACHE*, ::SCRIPT_FONTPROPERTIES*))
		ASCENSION_SHARED_LIB_ENTRY(13, "ScriptRecordDigitSubstitution", HRESULT(WINAPI *signature)(::LCID, ::SCRIPT_DIGITSUBSTITUTE*))
		ASCENSION_SHARED_LIB_ENTRY(14, "ScriptApplyDigitSubstitution", HRESULT(WINAPI *signature)(const ::SCRIPT_DIGITSUBSTITUTE*, ::SCRIPT_CONTROL*, SCRIPT_STATE*))
	ASCENSION_END_SHARED_LIB_ENTRIES()

	auto_ptr<ascension::internal::SharedLibrary<USPEntries> > uspLib;

	const class ScriptProperties {
	public:
		ScriptProperties() throw() : p_(0), c_(0) {::ScriptGetProperties(&p_, &c_);}
		const ::SCRIPT_PROPERTIES& get(int script) const throw() {return *p_[script];}
	private:
		const ::SCRIPT_PROPERTIES** p_;
		int c_;
	} scriptProperties;

	class UserSettings {
	public:
		UserSettings() throw() {update();}
		LANGID getDefaultLanguage() const throw() {return langID_;}
		const ::SCRIPT_DIGITSUBSTITUTE& getDigitSubstitution() const throw() {return digitSubstitution_;}
		void update() throw() {langID_ = ::GetUserDefaultLangID(); ::ScriptRecordDigitSubstitution(LOCALE_USER_DEFAULT, &digitSubstitution_);}
	private:
		LANGID langID_;
		::SCRIPT_DIGITSUBSTITUTE digitSubstitution_;
	} userSettings;

	inline bool isC0orC1Control(CodePoint c) throw() {return c < 0x20 || c == 0x7F || (c >= 0x80 && c < 0xA0);}
	inline Orientation getLineTerminatorOrientation(const TextViewer::Configuration& c) throw() {
		switch(c.alignment) {
		case ALIGN_LEFT:	return LEFT_TO_RIGHT;
		case ALIGN_RIGHT:	return RIGHT_TO_LEFT;
		case ALIGN_CENTER:
		default:			return c.orientation;
		}
	}
} // namespace @0

namespace ascension { namespace viewers { namespace internal {	// verbose notation for VC71 class viewer's confusion
	struct Run : public StyledText {
		::SCRIPT_ANALYSIS analysis;
		::SCRIPT_CACHE cache;
		HFONT font;			// the font to draw the run
		WORD* glyphs;
		length_t length;	// the number of characters of the run
		int numberOfGlyphs;	// the number of glyphs of the run
		WORD* clusters;
		::SCRIPT_VISATTR* visualAttributes;
		int* advances;
		int* justifiedAdvances;
		::GOFFSET* glyphOffsets;
		::ABC width;
		Run(const TextStyle& textStyle) throw() :
				cache(0), font(0), glyphs(0), clusters(0), visualAttributes(0), advances(0), justifiedAdvances(0), glyphOffsets(0) {
			style = textStyle;
		}
		~Run() throw() {dispose();}
		void dispose() throw() {
			if(cache != 0) {::ScriptFreeCache(&cache); cache = 0;}
			font = 0;
			delete[] glyphs; glyphs = 0;
			delete[] clusters; clusters = 0;
			delete[] visualAttributes; visualAttributes = 0;
			delete[] advances; advances = 0;
			delete[] justifiedAdvances; justifiedAdvances = 0;
			delete[] glyphOffsets; glyphOffsets = 0;
		}
		HRESULT getLogicalWidths(int widths[]) const throw() {
			return ::ScriptGetLogicalWidths(&analysis,
				static_cast<int>(length), static_cast<int>(numberOfGlyphs), advances, clusters, visualAttributes, widths);
		}
		Orientation getOrientation() const throw() {return ((analysis.s.uBidiLevel & 0x01) == 0x00) ? LEFT_TO_RIGHT : RIGHT_TO_LEFT;}
		int getWidth() const throw() {return width.abcA + width.abcB + width.abcC;}
		HRESULT getX(size_t offset, bool trailing, int& x) const throw() {
			return ::ScriptCPtoX(static_cast<int>(offset), trailing, static_cast<int>(length),
				numberOfGlyphs, clusters, visualAttributes, (justifiedAdvances == 0) ? advances : justifiedAdvances, &analysis, &x);
		}
		bool overhangs() const throw() {return width.abcA < 0 || width.abcC < 0;}
	};
}}}	// namespace ascension.viewers.internal

void dumpRuns(const LineLayout& layout) {
#ifdef _DEBUG
	ostringstream s;
	layout.dumpRuns(s);
	::OutputDebugStringA(s.str().c_str());
#endif /* _DEBUG */
}

void ascension::updateSystemSettings() throw() {
	viewers::internal::systemColors.update();
	userSettings.update();
}


// LineLayout ///////////////////////////////////////////////////////////////

// helpers for LineLayout::draw
namespace {
	const size_t MAXIMUM_RUN_LENGTH = 1024;
	inline HPEN createPen(COLORREF color, int width, int style) {
		::LOGBRUSH brush;
		brush.lbColor = color;
		brush.lbStyle = BS_SOLID;
		switch(style) {
		case 1:	// solid
			return (width == 1) ? ::CreatePen(PS_SOLID, 1, color) : ::ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT, width, &brush, 0, 0);
		case 2:	// dashed
			return ::ExtCreatePen(PS_GEOMETRIC | PS_DASH | PS_ENDCAP_FLAT, width, &brush, 0, 0);
		case 3:	// dotted
			return ::ExtCreatePen(PS_GEOMETRIC | PS_DOT | PS_ENDCAP_FLAT, width, &brush, 0, 0);
		}
		throw invalid_argument("Unknown style value.");
	}
	inline void drawDecorationLines(DC& dc, const TextStyle& style, COLORREF foregroundColor, int x, int y, int width, int height) {
		::OUTLINETEXTMETRICW* otm = 0;
		::TEXTMETRICW tm;
		if(style.underlineStyle != NO_UNDERLINE || style.strikeout) {
			if(const UINT c = dc.getOutlineTextMetrics(0, 0)) {
				otm = static_cast<::OUTLINETEXTMETRICW*>(operator new(c));
				dc.getOutlineTextMetrics(c, otm);
			} else
				dc.getTextMetrics(tm);
		}

		// draw underline
		if(style.underlineStyle != NO_UNDERLINE) {
			HPEN oldPen = dc.selectObject(createPen((style.underlineColor != STANDARD_COLOR) ?
				style.underlineColor : foregroundColor, (otm != 0) ? otm->otmsUnderscoreSize : 1, style.underlineStyle));
			const int underlineY = y + ((otm != 0) ?
				otm->otmTextMetrics.tmAscent - otm->otmsUnderscorePosition + otm->otmsUnderscoreSize / 2 : tm.tmAscent);
			dc.moveTo(x, underlineY);
			dc.lineTo(x + width, underlineY);
			::DeleteObject(dc.selectObject(oldPen));
		}

		// draw strikeout line
		if(style.strikeout) {
			HPEN oldPen = dc.selectObject(createPen(foregroundColor, (otm != 0) ? otm->otmsStrikeoutSize : 1, 1));
			const int strikeoutY = y + ((otm != 0) ?
				otm->otmTextMetrics.tmAscent - otm->otmsStrikeoutPosition + otm->otmsStrikeoutSize / 2 : tm.tmAscent / 3);
			dc.moveTo(x, strikeoutY);
			dc.lineTo(x + width, strikeoutY);
			::DeleteObject(dc.selectObject(oldPen));
		}

		// draw border
		if(style.borderStyle != NO_BORDER) {
			HPEN oldPen = dc.selectObject(createPen((style.borderColor != STANDARD_COLOR) ?
				style.borderColor : foregroundColor, 1, style.borderStyle));
			HBRUSH oldBrush = dc.selectObject(static_cast<HBRUSH>(::GetStockObject(NULL_BRUSH)));
			dc.rectangle(x, y, x + width, y + height);
			::DeleteObject(dc.selectObject(oldPen));
			dc.selectObject(oldBrush);
		}

		delete[] otm;
	}
} // namespace @0

/**
 * @class ascension::viewers::LineLayout
 * @c LineLayout represents a layout of styled line text. Provides support for drawing, cursor
 * navigation, hit testing, text wrapping, etc.
 *
 * A long run will be split into smaller runs automatically because Uniscribe rejects too long text
 * (especially @c ScriptShape and @c ScriptTextOut). For this reason, a combining character will be
 * rendered incorrectly if it is presented at the boundary. The maximum length of a run is 1024.
 *
 * @note This class is not intended to derive.
 * @see LineLayoutBuffer#getLineLayout, LineLayoutBuffer::getLineLayoutIfCached
 */

/**
 * Constructor.
 * @param textRenderer the text renderer
 * @param line the line
 * @throw text#BadPositionException @a line is invalid
 */
LineLayout::LineLayout(const TextRenderer& textRenderer, length_t line) :
		renderer_(textRenderer), lineNumber_(line),
		runs_(0), numberOfRuns_(0), sublineOffsets_(0), sublineFirstRuns_(0), numberOfSublines_(0), longestSublineWidth_(-1) {
	if(!getText().empty()) {
		itemize(line);
		for(size_t i = 0; i < numberOfRuns_; ++i)
			shape(*runs_[i]);
		if(numberOfRuns_ == 0 || !renderer_.getTextViewer().getConfiguration().lineWrap.wraps()) {
			numberOfSublines_ = 1;
			sublineFirstRuns_ = new size_t[1];
			sublineFirstRuns_[0] = 0;
			reorder();
			expandTabsWithoutWrapping();
		} else {
			wrap();
			reorder();
			if(renderer_.getTextViewer().getConfiguration().justifiesLines)
				justify();
		}
	} else {	// a empty line
		numberOfRuns_ = 0;
		numberOfSublines_ = 1;
		longestSublineWidth_ = 0;
	}
}

/// Destructor.
LineLayout::~LineLayout() throw() {
	dispose();
}

/// Disposes the layout.
inline void LineLayout::dispose() throw() {
	for(size_t i = 0; i < numberOfRuns_; ++i)
		delete runs_[i];
	delete[] runs_;
	runs_ = 0;
	numberOfRuns_ = 0;
	delete[] sublineOffsets_;
	delete[] sublineFirstRuns_;
	sublineFirstRuns_ = 0;
	numberOfSublines_ = 0;
}

/**
 * Draws the layout to the output device. @a selectionColor and @a marginColor must be actual
 * color. Do not use @c presentation#STANDARD_COLOR or any system color using
 * @c presentation#SYSTEM_COLOR_MASK.
 * @param dc the device context
 * @param x the x-coordinate of the position to draw
 * @param y the y-coordinate of the position to draw
 * @param paintRect the region to draw
 * @param clipRect the clipping region
 * @param selectionColor the color of the selection
 */
void LineLayout::draw(DC& dc, int x, int y, const ::RECT& paintRect, const ::RECT& clipRect, const Colors& selectionColor) const throw() {
	const int linePitch = renderer_.getLinePitch();

	// empty line
	if(isDisposed()) {
		::RECT r;
		r.left = max(paintRect.left, clipRect.left);
		r.top = max(clipRect.top, max<long>(paintRect.top, y));
		r.right = min(paintRect.right, clipRect.right);
		r.bottom = min(clipRect.bottom, min<long>(paintRect.bottom, y + linePitch));
		const Colors lineColor = renderer_.getTextViewer().getPresentation().getLineColor(lineNumber_);
		dc.fillSolidRect(r, internal::systemColors.getReal((lineColor.background == STANDARD_COLOR) ?
			renderer_.getTextViewer().getConfiguration().color.background : lineColor.background, SYSTEM_COLOR_MASK | COLOR_WINDOW));
		return;
	}

	// skip to the subline needs to draw
	length_t subline = (y + linePitch >= paintRect.top) ? 0 : (paintRect.top - (y + linePitch)) / linePitch;
	if(subline >= numberOfSublines_)
		return;	// this logical line does not need to draw
	y += static_cast<int>(linePitch * subline);

	for(; subline < numberOfSublines_; ++subline) {
		draw(subline, dc, x, y, paintRect, clipRect, selectionColor);
		if((y += linePitch) >= paintRect.bottom)	// to next subline
			break;
	}
}

/**
 * Draws the specified subline layout to the output device. @a selectionColor and @a marginColor
 * must be actual color. Do not use @c presentation#STANDARD_COLOR or any system color using
 * @c presentation#SYSTEM_COLOR_MASK.
 * @param subline the visual subline
 * @param dc the device context
 * @param x the x-coordinate of the position to draw
 * @param y the y-coordinate of the position to draw
 * @param paintRect the region to draw
 * @param clipRect the clipping region
 * @param selectionColor the color of the selection
 * @throw text#BadPositionException @a subline is invalid
 */
void LineLayout::draw(length_t subline, DC& dc,
		int x, int y, const ::RECT& paintRect, const ::RECT& clipRect, const Colors& selectionColor) const {
	if(subline >= numberOfSublines_)
		throw BadPositionException();

	// クリッピングによるマスキングを利用した選択テキストの描画は以下の記事を参照
	// Catch 22 : Design and Implementation of a Win32 Text Editor
	// Part 10 - Transparent Text and Selection Highlighting (http://www.catch22.net/tuts/editor10.asp)

	const int linePitch = renderer_.getLinePitch();
	const int lineHeight = renderer_.getLineHeight();
	const Colors lineColor = renderer_.getTextViewer().getPresentation().getLineColor(lineNumber_);
	const COLORREF marginColor = internal::systemColors.getReal((lineColor.background == STANDARD_COLOR) ?
		renderer_.getTextViewer().getConfiguration().color.background : lineColor.background, SYSTEM_COLOR_MASK | COLOR_WINDOW);
	ISpecialCharacterRenderer::DrawingContext context(dc);
	ISpecialCharacterRenderer* specialCharacterRenderer = renderer_.getSpecialCharacterRenderer();

	if(specialCharacterRenderer != 0) {
		context.rect.top = y;
		context.rect.bottom = y + lineHeight;
	}

	const int savedCookie = dc.save();
	dc.setTextAlign(TA_BASELINE | TA_LEFT | TA_NOUPDATECP);
	if(isDisposed()) {	// empty line
		::RECT r;
		r.left = max(paintRect.left, clipRect.left);
		r.top = max(clipRect.top, max<long>(paintRect.top, y));
		r.right = min(paintRect.right, clipRect.right);
		r.bottom = min(clipRect.bottom, min<long>(paintRect.bottom, y + linePitch));
		dc.fillSolidRect(r, marginColor);
		if(getLineTerminatorOrientation(renderer_.getTextViewer().getConfiguration()) == RIGHT_TO_LEFT)
			x += getSublineIndent(subline);
	} else {
		const String& text = getText();
		const int originalX = x;
		HRESULT hr;
		length_t selStart, selEnd;
		const bool sel = renderer_.getTextViewer().getCaret().getSelectedRangeOnVisualLine(lineNumber_, subline, selStart, selEnd);

		// paint between sublines
		Rgn clipRegion(Rgn::createRect(clipRect.left, max<long>(y, clipRect.top), clipRect.right, min<long>(y + linePitch, clipRect.bottom)));
//		dc.selectClipRgn(clipRegion.getHandle());
		if(linePitch - lineHeight > 0)
			dc.fillSolidRect(paintRect.left, y + renderer_.getLineHeight(),
				paintRect.right - paintRect.left, linePitch - lineHeight, marginColor);

		// 1. paint background of the runs
		// 2. determine the first and the last runs need to draw
		// 3. mask selected region
		size_t firstRun = sublineFirstRuns_[subline];
		size_t lastRun = (subline < numberOfSublines_ - 1) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;
		x = originalX + getSublineIndent(subline);	// x is left edge of the subline
		// paint the left margin
		if(x > originalX && x > paintRect.left) {
			const int left = min<int>(originalX, paintRect.left);
			dc.fillSolidRect(left, y, x - left, lineHeight, marginColor);
		}
		// paint background of the runs
		int startX = x;
		for(size_t i = firstRun; i < lastRun; ++i) {
			Run& run = *runs_[i];
			if(x + run.getWidth() < paintRect.left) {	// this run does not need to draw
				++firstRun;
				startX = x + run.getWidth();
			} else {
				const COLORREF bgColor = (lineColor.background == STANDARD_COLOR) ?
					internal::systemColors.getReal(run.style.color.background, SYSTEM_COLOR_MASK | COLOR_WINDOW) : marginColor;
				if(!sel || run.column >= selEnd || run.column + run.length <= selStart)	// no selection in this run
					dc.fillSolidRect(x, y, run.getWidth(), lineHeight, bgColor);
				else if(sel && run.column >= selStart && run.column + run.length <= selEnd) {	// this run is selected entirely
					dc.fillSolidRect(x, y, run.getWidth(), lineHeight, selectionColor.background);
					dc.excludeClipRect(x, y, x + run.getWidth(), y + lineHeight);
				} else {	// selected partially
					int left, right;
					hr = run.getX(max(selStart, run.column) - run.column, false, left);
					hr = run.getX(min(selEnd, run.column + run.length) - 1 - run.column, true, right);
					if(left > right)
						swap(left, right);
					left += x;
					right += x;
					if(left > x/* && left > paintRect.left*/)
						dc.fillSolidRect(x, y, left - x, lineHeight, bgColor);
					if(right > left) {
						dc.fillSolidRect(left, y, right - left, lineHeight, selectionColor.background);
						dc.excludeClipRect(left, y, right, y + lineHeight);
					}
					if(right < x + run.getWidth())
						dc.fillSolidRect(right, y, run.getWidth() - (left - x), lineHeight, bgColor);
				}
			}
			x += run.getWidth();
			if(x >= paintRect.right) {
				lastRun = i + 1;
				break;
			}
		}
		// paint the right margin
		if(x < paintRect.right)
			dc.fillSolidRect(x, y, paintRect.right - x, linePitch, marginColor);

		// draw outside of the selection
		::RECT runRect;
		runRect.top = y;
		runRect.bottom = y + linePitch;
		runRect.left = x = startX;
		dc.setBkMode(TRANSPARENT);
		for(size_t i = firstRun; i < lastRun; ++i) {
			Run& run = *runs_[i];
			const COLORREF foregroundColor =
				internal::systemColors.getReal((lineColor.foreground == STANDARD_COLOR) ?
					run.style.color.foreground : lineColor.foreground, COLOR_WINDOWTEXT | SYSTEM_COLOR_MASK);
			if(text[run.column] != L'\t') {
				if(!sel || run.overhangs() || !(run.column >= selStart && run.column + run.length <= selEnd)) {
					dc.selectObject(run.font);
					dc.setTextColor(foregroundColor);
					runRect.left = x;
					runRect.right = runRect.left + run.getWidth();
					hr = ::ScriptTextOut(dc.getHandle(), &run.cache, x, y + renderer_.getAscent(), 0, &runRect,
						&run.analysis, 0, 0, run.glyphs, run.numberOfGlyphs, run.advances, run.justifiedAdvances, run.glyphOffsets);
				}
			}
			// decoration (underline and border)
			drawDecorationLines(dc, run.style, foregroundColor, x, y, run.getWidth(), linePitch);
			x += run.getWidth();
			runRect.left = x;
		}

		// draw selected text segment (also underline and border)
		x = startX;
		clipRegion.setRect(clipRect);
		dc.selectClipRgn(clipRegion.getHandle(), RGN_XOR);
		for(size_t i = firstRun; i < lastRun; ++i) {
			Run& run = *runs_[i];
			// text
			if(sel && text[run.column] != L'\t'
					&& (run.overhangs() || (run.column < selEnd && run.column + run.length > selStart))) {
				dc.selectObject(run.font);
				dc.setTextColor(selectionColor.foreground);
				runRect.left = x;
				runRect.right = runRect.left + run.getWidth();
				hr = ::ScriptTextOut(dc.getHandle(), &run.cache, x, y + renderer_.getAscent(), 0, &runRect,
					&run.analysis, 0, 0, run.glyphs, run.numberOfGlyphs, run.advances, run.justifiedAdvances, run.glyphOffsets);
			}
			// decoration (underline and border)
			drawDecorationLines(dc, run.style, selectionColor.foreground, x, y, run.getWidth(), linePitch);
			x += run.getWidth();
		}

		// special character substitution
		if(specialCharacterRenderer != 0) {
			// white spaces and C0/C1 control characters
			dc.selectClipRgn(clipRegion.getHandle());
			x = startX;
			int dx;
			for(size_t i = firstRun; i < lastRun; ++i) {
				Run& run = *runs_[i];
				context.orientation = run.getOrientation();
				for(length_t j = run.column; j < run.column + run.length; ++j) {
					if(BinaryProperty::is(text[j], BinaryProperty::WHITE_SPACE)) {	// IdentifierSyntax.isWhiteSpace() is preferred?
						run.getX(j - run.column, false, dx);
						context.rect.left = x + dx;
						run.getX(j - run.column, true, dx);
						context.rect.right = x + dx;
						if(context.rect.left > context.rect.right)
							swap(context.rect.left, context.rect.right);
						specialCharacterRenderer->drawWhiteSpaceCharacter(context, text[j]);
					} else if(isC0orC1Control(text[j])) {
						run.getX(j - run.column, false, dx);
						context.rect.left = x + dx;
						run.getX(j - run.column, true, dx);
						context.rect.right = x + dx;
						if(context.rect.left > context.rect.right)
							swap(context.rect.left, context.rect.right);
						specialCharacterRenderer->drawControlCharacter(context, text[j]);
					}
				}
				x += run.getWidth();
			}
		}
		if(getLineTerminatorOrientation(renderer_.getTextViewer().getConfiguration()) == RIGHT_TO_LEFT)
			x = originalX + getSublineIndent(subline);
	} // end of nonempty line case
	
	// line terminator and line wrapping mark
	const Document& document = renderer_.getTextViewer().getDocument();
	if(specialCharacterRenderer != 0) {
		context.orientation = getLineTerminatorOrientation(renderer_.getTextViewer().getConfiguration());
		if(subline < numberOfSublines_ - 1) {	// line wrapping mark
			const int markWidth = specialCharacterRenderer->getLineWrappingMarkWidth(context);
			if(context.orientation == LEFT_TO_RIGHT) {
				context.rect.right = renderer_.getWidth();
				context.rect.left = context.rect.right - markWidth;
			} else {
				context.rect.left = 0;
				context.rect.right = markWidth;
			}
			specialCharacterRenderer->drawLineWrappingMark(context);
		} else if(lineNumber_ < document.getNumberOfLines() - 1) {	// line teminator
			const text::Newline nlf = document.getLineInfo(lineNumber_).getNewline();
			const int nlfWidth = specialCharacterRenderer->getLineTerminatorWidth(context, nlf);
			if(context.orientation == LEFT_TO_RIGHT) {
				context.rect.left = x;
				context.rect.right = x + nlfWidth;
			} else {
				context.rect.left = x - nlfWidth;
				context.rect.right = x;
			}
			const Caret& caret = renderer_.getTextViewer().getCaret();
			const Position eol(lineNumber_, document.getLineLength(lineNumber_));
			if(!caret.isSelectionRectangle() && caret.getTopPoint().getPosition() <= eol && caret.getBottomPoint().getPosition() > eol)
				dc.fillSolidRect(x - (context.orientation == RIGHT_TO_LEFT ? nlfWidth : 0), y, nlfWidth, linePitch, selectionColor.background);
			dc.setBkMode(TRANSPARENT);
			specialCharacterRenderer->drawLineTerminator(context, nlf);
		}
	}

	dc.restore(savedCookie);
}

#ifdef _DEBUG
/**
 * Dumps the all runs to the specified output stream.
 * @param out the output stream
 */
void LineLayout::dumpRuns(ostream& out) const {
	for(size_t i = 0; i < numberOfRuns_; ++i) {
		const Run& run = *runs_[i];
		out << static_cast<uint>(i)
			<< ":column=" << static_cast<uint>(run.column)
			<< ",length=" << static_cast<uint>(run.length) << endl;
	}
}
#endif /* _DEBUG */

/// Expands the all tabs and resolves each width.
inline void LineLayout::expandTabsWithoutWrapping() throw() {
	const bool rtl = getLineTerminatorOrientation(renderer_.getTextViewer().getConfiguration()) == RIGHT_TO_LEFT;
	const String& text = getText();
	int x = 0;
	Run* run;
	if(!rtl) {	// expand from the left most
		for(size_t i = 0; i < numberOfRuns_; ++i) {
			run = runs_[i];
			if(run->length == 1 && text[run->column] == L'\t') {
				run->advances[0] = getNextTabStop(x, FORWARD) - x;
				run->width.abcB = run->advances[0];
				run->width.abcA = run->width.abcC = 0;
			}
			x += run->getWidth();
		}
	} else {	// expand from the right most
		for(size_t i = numberOfRuns_; i > 0; --i) {
			run = runs_[i - 1];
			if(run->length == 1 && text[run->column] == L'\t') {
				run->advances[0] = getNextTabStop(x, FORWARD) - x;
				run->width.abcB = run->advances[0];
				run->width.abcA = run->width.abcC = 0;
			}
			x += run->getWidth();
		}
	}
	longestSublineWidth_ = x;
}

/**
 * Returns the space string added to the end of the specified line to reach the specified virtual
 * point. If the end of the line is over @a virtualX, the result is an empty string.
 * @param x the x-coordinate of the virtual point
 * @return the space string consists of only white spaces (U+0020) and horizontal tabs (U+0009)
 * @throw text#BadPositionException @a line is outside of the document
 * @deprecated 0.8
 * @note This does not support line wrapping and bidirectional context.
 */
String LineLayout::fillToX(int x) const {
	int cx = getLongestSublineWidth();
	if(cx >= x)
		return L"";

	size_t numberOfTabs = 0;
	while(true) {
		const int next = getNextTabStopBasedLeftEdge(cx, true);
		if(next > x)
			break;
		++numberOfTabs;
		cx = next;
	}

	if(cx == x)
		return String(numberOfTabs, L'\t');

	ClientDC dc = const_cast<TextRenderer&>(renderer_).getTextViewer().getDC();
	HFONT oldFont = dc.selectObject(renderer_.getFont(Script::COMMON));
	int spaceWidth;
	dc.getCharWidth(L' ', L' ', &spaceWidth);
	size_t numberOfSpaces = 0;
	while(true) {
		if(cx + spaceWidth > x)
			break;
		++numberOfSpaces;
		cx += spaceWidth;
	}

	String result(numberOfTabs + numberOfSpaces, L' ');
	result.replace(0, numberOfTabs, numberOfTabs, L'\t');
	return result;
}

/**
 * Returns the index of run containing the specified column.
 * @param column the column
 * @return the index of the run
 */
inline size_t LineLayout::findRunForPosition(length_t column) const throw() {
	assert(numberOfRuns_ > 0);
	if(column == getText().length())
		return numberOfRuns_ - 1;
	const length_t subline = getSubline(column);
	const size_t lastRun = (subline + 1 < numberOfSublines_) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;
	for(size_t i = sublineFirstRuns_[subline]; i < lastRun; ++i) {
		if(runs_[i]->column <= column && runs_[i]->column + runs_[i]->length > column)
			return i;
	}
	assert(false);
	return lastRun - 1;	// ここには来ないだろうが...
}

/**
 * Returns the bidirectional embedding level at specified position.
 * @param column the column
 * @return the embedding level
 * @throw text#BadPositionException @a column is greater than the length of the line
 */
uchar LineLayout::getBidiEmbeddingLevel(length_t column) const {
	if(numberOfRuns_ == 0) {
		if(column != 0)
			throw text::BadPositionException();
		// use the default level
		return (renderer_.getTextViewer().getConfiguration().orientation == RIGHT_TO_LEFT) ? 1 : 0;
	}
	const size_t i = findRunForPosition(column);
	if(i == numberOfRuns_)
		throw text::BadPositionException();
	return static_cast<uchar>(runs_[i]->analysis.s.uBidiLevel);
}

/**
 * Returns the smallest rectangle emcompasses the whole text of the line.
 * @return the size of the bounds
 * @see #getBounds(length_t, length_t), #getSublineBounds
 */
::SIZE LineLayout::getBounds() const throw() {
	::SIZE s;
	s.cx = getLongestSublineWidth();
	s.cy = static_cast<long>(renderer_.getLinePitch() * numberOfSublines_);
	return s;
}

/**
 * Returns the smallest rectangle emcompasses all characters in the range.
 * @param first the start of the range
 * @param last the end of the range
 * @return the bounds
 * @throw text#BadPositionException @a first or @a last is greater than the length of the line
 * @throw std#invalid_argument @a first is greater than @a last
 * @see #getBounds(void), #getSublineBounds
 */
::RECT LineLayout::getBounds(length_t first, length_t last) const {
	if(first > last)
		throw invalid_argument("first is greater than last.");
	else if(last > getText().length())
		throw text::BadPositionException();
	::RECT bounds;	// the result
	int cx, x;

	// for first
	length_t subline = getSubline(first);
	size_t lastRun = (subline + 1 < numberOfSublines_) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;
	bounds.top = static_cast<long>(renderer_.getLinePitch() * subline);
	cx = getSublineIndent(subline);
	for(size_t i = sublineFirstRuns_[subline]; i < lastRun; ++i) {
		const Run& run = *runs_[i];
		if(run.column <= first && run.column + run.length > first) {
			run.getX(first - run.column, false, x);
			bounds.left = cx + x;
			break;
		}
		cx += run.getWidth();
	}

	// for last
	if(last == first) {
		bounds.bottom = bounds.top + renderer_.getLinePitch();
		bounds.right = bounds.left;
	} else {
		subline = getSubline(last);
		lastRun = (subline + 1 < numberOfSublines_) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;
		bounds.bottom = static_cast<long>(renderer_.getLinePitch() * subline);
		cx = getSublineIndent(subline);
		for(size_t i = sublineFirstRuns_[subline]; i < lastRun; ++i) {
			const Run& run = *runs_[i];
			if(run.column <= last && run.column + run.length > last) {
				run.getX(last - run.column, false, x);
				bounds.right = cx + x;
				break;
			}
			cx += run.getWidth();
		}
		if(bounds.left > bounds.right)
			swap(bounds.left, bounds.right);
		if(bounds.top > bounds.bottom)
			swap(bounds.top, bounds.bottom);
		bounds.bottom += renderer_.getLinePitch();
	}

	return bounds;
}

/// Returns an iterator addresses the first styled segment.
LineLayout::StyledSegmentIterator LineLayout::getFirstStyledSegment() const throw() {
	return StyledSegmentIterator(*runs_);
}

/// Returns an iterator addresses the last styled segment.
LineLayout::StyledSegmentIterator LineLayout::getLastStyledSegment() const throw() {
	return StyledSegmentIterator(runs_[numberOfRuns_]);
}

/**
 * Return the location for the specified character offset.
 * @param column the character offset from the line start
 * @param edge the edge of the character to locate
 * @return the location. x-coordinate is distance from the left edge of the renderer, y-coordinate is relative in the visual lines
 * @throw text#BadPositionException @a column is greater than the length of the line
 */
::POINT LineLayout::getLocation(length_t column, Edge edge /* = LEADING */) const {
	::POINT location;
	if(column > getText().length())
		throw text::BadPositionException();
	else if(isDisposed()) {
		location.x = getSublineIndent(0);
		location.y = 0;
	} else {
		const length_t subline = getSubline(column);
		const length_t firstRun = sublineFirstRuns_[subline];
		const length_t lastRun = (subline + 1 < numberOfSublines_) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;
		// about x
		if(renderer_.getTextViewer().getConfiguration().orientation == LEFT_TO_RIGHT) {	// LTR
			location.x = 0;
			for(size_t i = firstRun; i < lastRun; ++i) {
				const Run& run = *runs_[i];
				if(column >= run.column && column <= run.column + run.length) {
					int offset;
					run.getX(column - run.column, edge == TRAILING, offset);
					location.x += offset;
					break;
				}
				location.x += run.getWidth();
			}
		} else {	// RTL
			location.x = getSublineWidth(subline);
			for(size_t i = lastRun - 1; ; --i) {
				const Run& run = *runs_[i];
				location.x -= run.getWidth();
				if(column >= run.column && column <= run.column + run.length) {
					int offset;
					run.getX(column - run.column, edge == TRAILING, offset);
					location.x += offset;
					break;
				}
				if(i == firstRun)
					break;
			}
		}
		location.x += getSublineIndent(subline);
		// about y
		location.y = static_cast<long>(subline * renderer_.getLinePitch());
	}
	return location;
}

/// Returns the width of the longest subline.
int LineLayout::getLongestSublineWidth() const throw() {
	if(longestSublineWidth_ == -1) {
		int width = 0;
		for(length_t subline = 0; subline < numberOfSublines_; ++subline)
			width = max<long>(getSublineWidth(subline), width);
		const_cast<LineLayout*>(this)->longestSublineWidth_ = width;
	}
	return longestSublineWidth_;
}

/**
 * Returns the next tab stop position.
 * @param x the distance from leading edge of the line (can not be negative)
 * @param direction the direction
 * @return the distance from leading edge of the line to the next tab position
 */
inline int LineLayout::getNextTabStop(int x, Direction direction) const throw() {
	assert(x >= 0);
	const int tabWidth = renderer_.getAverageCharacterWidth() * renderer_.getTextViewer().getConfiguration().tabWidth;
	return (direction == FORWARD) ? x + tabWidth - x % tabWidth : x - x % tabWidth;
}

/**
 * Returns the next tab stop.
 * @param x the distance from the left edge of the line to base position (can not be negative)
 * @param right true to find the next right position
 * @return the tab stop position in pixel
 */
int LineLayout::getNextTabStopBasedLeftEdge(int x, bool right) const throw() {
	assert(x >= 0);
	const TextViewer::Configuration& c = renderer_.getTextViewer().getConfiguration();
	const int tabWidth = renderer_.getAverageCharacterWidth() * c.tabWidth;
	if(getLineTerminatorOrientation(c) == LEFT_TO_RIGHT)
		return getNextTabStop(x, right ? FORWARD : BACKWARD);
	else
		return right ? x + (x - getLongestSublineWidth()) % tabWidth : x - (tabWidth - (x - getLongestSublineWidth()) % tabWidth);
}

/**
 * Returns the character column (offset) for the specified point.
 * @param x the x coordinate of the point. distance from the left edge of the renderer (not of the line)
 * @param y the y coordinate of the point
 * @param[out] trailing the trailing buffer
 * @return the character offset
 * @see #getLocation
 */
length_t LineLayout::getOffset(int x, int y, length_t& trailing) const throw() {
	if(getText().empty())
		return trailing = 0;

	// 折り返し行の特定
	length_t subline = 0;
	for(; subline < numberOfSublines_ - 1; ++subline) {
		if(static_cast<int>(renderer_.getLinePitch() * subline) >= y)
			break;
	}

	// 列の特定
	assert(numberOfRuns_ > 0);
	const size_t lastRun = (subline + 1 < numberOfSublines_) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;
	int cx = getSublineIndent(subline);
	if(x <= cx) {	// 行の左余白内
		trailing = 0;
		const Run& firstRun = *runs_[sublineFirstRuns_[subline]];
		return firstRun.column + ((firstRun.analysis.fRTL == 0) ? 0 : firstRun.length);
	}
	x = max(cx, x);
	for(size_t i = sublineFirstRuns_[subline]; i < lastRun; ++i) {
		const Run& run = *runs_[i];
		if(x >= cx && x <= cx + run.getWidth()) {
			int cp, t;
			::ScriptXtoCP(x - cx, static_cast<int>(run.length), static_cast<int>(run.numberOfGlyphs), run.clusters,
				run.visualAttributes, (run.justifiedAdvances == 0) ? run.advances : run.justifiedAdvances, &run.analysis, &cp, &t);
			trailing = static_cast<length_t>(t);
			return run.column + static_cast<length_t>(cp);
		}
		cx += run.getWidth();
	}
	trailing = 0;	// 行の右余白
	return runs_[lastRun - 1]->column + ((runs_[lastRun - 1]->analysis.fRTL == 0) ? runs_[lastRun - 1]->length : 0);
}

/**
 * Returns the styled segment containing the specified column.
 * @param column the column
 * @return the styled segment
 * @throw text#BadPositionException @a column is greater than the length of the line
 */
const StyledText& LineLayout::getStyledSegment(length_t column) const {
	if(column > getText().length())
		throw text::BadPositionException();
	return *runs_[findRunForPosition(column)];
}

/**
 * Returns the smallest rectangle emcompasses the specified visual line.
 * @param subline the wrapped line
 * @return the rectangle
 * @throw text#BadPositionException @a subline is greater than the number of the wrapped lines
 */
::RECT LineLayout::getSublineBounds(length_t subline) const {
	if(subline >= numberOfSublines_)
		throw text::BadPositionException();
	::RECT rc;
	rc.left = getSublineIndent(subline);
	rc.top = renderer_.getLinePitch() * static_cast<long>(subline);
	rc.right = rc.left + getSublineWidth(subline);
	rc.bottom = rc.top + renderer_.getLinePitch();
	return rc;
}

/**
 * Returns the indentation from the left most.
 * @param subline the visual line
 * @return the indentation in pixel
 * @throw text#BadPositionException @a subline is invalid
 */
int LineLayout::getSublineIndent(length_t subline) const {
	const TextViewer::Configuration& c = renderer_.getTextViewer().getConfiguration();
	if(c.alignment == ALIGN_LEFT || c.justifiesLines)
		return 0;
	else {
		int width;
		if(c.lineWrap.wraps()) {
			if(const ISpecialCharacterRenderer* scr = renderer_.getSpecialCharacterRenderer()) {
				ScreenDC dc;
				ISpecialCharacterRenderer::LayoutContext context(dc);
				context.orientation = getLineTerminatorOrientation(c);
				width = renderer_.getWrapWidth() + scr->getLineWrappingMarkWidth(context);
			} else
				width = renderer_.getWrapWidth();
		} else
			width = renderer_.getWidth();
		if(c.alignment == ALIGN_RIGHT)
			return width - getSublineWidth(subline);
		else if(c.alignment == ALIGN_CENTER)
			return (width - getSublineWidth(subline)) / 2;
		else
			throw runtime_error("");	// 無意味
	}
}

/**
 * Returns the width of the specified wrapped line.
 * @param subline the visual line
 * @return the width
 * @throw text#BadPositionException @a subline is greater than the number of visual lines
 */
int LineLayout::getSublineWidth(length_t subline) const {
	if(subline >= numberOfSublines_)
		throw text::BadPositionException();
	else if(isDisposed())
		return 0;
	else if(numberOfSublines_ == 1 && longestSublineWidth_ != -1)
		return longestSublineWidth_;
	else {
		const size_t lastRun = (subline + 1 < numberOfSublines_) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;
		int cx = 0;
		for(size_t i = sublineFirstRuns_[subline]; i < lastRun; ++i)
			cx += runs_[i]->getWidth();
		return cx;
	}
}

/// Returns the text of the line.
inline const String& LineLayout::getText() const throw() {
	return renderer_.getTextViewer().getDocument().getLine(lineNumber_);
}

/// Returns if the line contains right-to-left run.
bool LineLayout::isBidirectional() const throw() {
	if(renderer_.getTextViewer().getConfiguration().orientation == RIGHT_TO_LEFT)
		return true;
	for(size_t i = 0; i < numberOfRuns_; ++i) {
		if(runs_[i]->getOrientation() == RIGHT_TO_LEFT)
			return true;
	}
	return false;
}

/**
 * Itemizes the text into shapable runs.
 * @param lineNumber the line number of the line
 */
inline void LineLayout::itemize(length_t lineNumber) throw() {
	const String& text = getText();
	assert(!text.empty());

	HRESULT hr;
	const TextViewer::Configuration& c = renderer_.getTextViewer().getConfiguration();
	const Presentation& presentation = renderer_.getTextViewer().getPresentation();

	// configure
	manah::win32::AutoZero<::SCRIPT_CONTROL> control;
	manah::win32::AutoZero<::SCRIPT_STATE> initialState;
	initialState.uBidiLevel = (c.orientation == RIGHT_TO_LEFT) ? 1 : 0;
//	initialState.fOverrideDirection = 1;
	initialState.fInhibitSymSwap = c.inhibitsSymmetricSwapping;
	initialState.fDisplayZWG = c.displaysShapingControls;
	switch(c.digitSubstitutionType) {
	case DST_NOMINAL:
		initialState.fDigitSubstitute = 0;
		break;
	case DST_NATIONAL:
		control.uDefaultLanguage = userSettings.getDefaultLanguage();
		initialState.fDigitSubstitute = 1;
		break;
	case DST_CONTEXTUAL:
		control.uDefaultLanguage = userSettings.getDefaultLanguage();
		control.fContextDigits = 1;
		initialState.fDigitSubstitute = 1;
		break;
	case DST_USER_DEFAULT:
		hr = ::ScriptApplyDigitSubstitution(&userSettings.getDigitSubstitution(), &control, &initialState);
		break;
	}

	// itemize
	int expectedNumberOfRuns = max(static_cast<int>(text.length()) / 8, 2);
	::SCRIPT_ITEM* items;
	int numberOfItems;
	while(true) {
		items = new ::SCRIPT_ITEM[expectedNumberOfRuns];
		hr = ::ScriptItemize(text.data(), static_cast<int>(text.length()),
			expectedNumberOfRuns, &control, &initialState, items, &numberOfItems);
		if(hr != E_OUTOFMEMORY)	// expectedNumberOfRuns was enough...
			break;
		delete[] items;
		expectedNumberOfRuns *= 2;
	}
	if(c.disablesDeprecatedFormatCharacters) {
		for(int i = 0; i < numberOfItems; ++i) {
			items[i].a.s.fInhibitSymSwap = initialState.fInhibitSymSwap;
			items[i].a.s.fDigitSubstitute = initialState.fDigitSubstitute;
		}
	}

	// style
	bool mustDelete;
	const LineStyle& styles = presentation.getLineStyle(lineNumber, mustDelete);
	if(&styles != &LineStyle::NULL_STYLE) {
#ifdef _DEBUG
		// verify the given styles
		for(size_t i = 0; i < styles.count - 1; ++i)
			assert(styles.array[i].column < styles.array[i + 1].column);
#endif /* _DEBUG */
		merge(items, numberOfItems, styles);
		if(mustDelete) {
			delete[] styles.array;
			delete &styles;
		}
	} else {
		StyledText segment;
		segment.column = 0;
		LineStyle simpleStyle;
		simpleStyle.array = &segment;
		simpleStyle.count = 1;
		merge(items, numberOfItems, simpleStyle);
	}
	delete[] items;
}

/// Justifies the wrapped visual lines.
inline void LineLayout::justify() throw() {
	const int wrapWidth = renderer_.getWrapWidth();
	for(length_t subline = 0; subline < numberOfSublines_; ++subline) {
		const int lineWidth = getSublineWidth(subline);
		const size_t last = (subline + 1 < numberOfSublines_) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;
		for(size_t i = sublineFirstRuns_[subline]; i < last; ++i) {
			Run& run = *runs_[i];
			const int newRunWidth = run.getWidth() * wrapWidth / lineWidth;	// TODO: there is more precise way.
			if(newRunWidth != run.getWidth()) {
				run.justifiedAdvances = new int[run.numberOfGlyphs];
				::ScriptJustify(run.visualAttributes, run.advances,
					run.numberOfGlyphs, newRunWidth - run.getWidth(), 2, run.justifiedAdvances);
				run.width.abcB += newRunWidth - run.getWidth();
			}
		}
	}
}

/**
 * Merges the given item runs and the given style runs.
 * @param items the items itemized by @c #itemize()
 * @param numberOfItems the length of the array @a items
 * @param styles the attributed text segments in the line (style runs)
 */
inline void LineLayout::merge(const ::SCRIPT_ITEM items[], size_t numberOfItems, const LineStyle& styles) throw() {
	assert(runs_ == 0 && items != 0 && numberOfItems > 0 && styles.count > 0);
	const String& text = getText();
	vector<Run*> runs;
	Run* run = new Run(styles.array[0].style);
	run->column = 0;
	run->analysis = items[0].a;
	runs.reserve(numberOfItems + styles.count);
	runs.push_back(run);

#define SPLIT_LAST_RUN()												\
	while(runs.back()->length > MAXIMUM_RUN_LENGTH) {					\
		Run& back = *runs.back();										\
		Run* piece = new Run(back.style);								\
		length_t pieceLength = MAXIMUM_RUN_LENGTH;						\
		if(surrogates::isLowSurrogate(text[back.column + pieceLength]))	\
			--pieceLength;												\
		piece->analysis = back.analysis;								\
		piece->column = back.column + pieceLength;						\
		piece->length = back.length - pieceLength;						\
		back.length = pieceLength;										\
		runs.push_back(piece);											\
	}

	for(size_t itemIndex = 1, styleIndex = 1; itemIndex < numberOfItems || styleIndex < styles.count; ) {
		bool brokeItem = false;
		const length_t nextItem = (itemIndex < numberOfItems) ? items[itemIndex].iCharPos : text.length();
		const length_t nextStyle = (styleIndex < styles.count) ? styles.array[styleIndex].column : text.length();
		length_t column;
		if(nextItem < nextStyle)
			column = items[itemIndex++].iCharPos;
		else if(nextStyle < nextItem) {
			column = styles.array[styleIndex++].column;
			brokeItem = true;
		} else {
			++itemIndex;
			column = styles.array[styleIndex++].column;
		}
		run->length = column - run->column;
		run = new Run(styles.array[styleIndex - 1].style);
		run->column = column;
		run->analysis = items[itemIndex - 1].a;
		if(brokeItem
				&& !legacyctype::isspace(text[styles.array[styleIndex - 1].column])
				&& !legacyctype::isspace(text[styles.array[styleIndex - 1].column - 1]))
			runs[runs.size() - 1]->analysis.fLinkAfter = run->analysis.fLinkBefore = 1;
		runs.back()->length = run->column - runs.back()->column;
		SPLIT_LAST_RUN();
		runs.push_back(run);
	}
	run->length = text.length() - run->column;
	SPLIT_LAST_RUN();
	runs_ = new Run*[numberOfRuns_ = runs.size()];
	copy(runs.begin(), runs.end(), runs_);

#undef SPLIT_LAST_RUN
}

/// Reorders the runs in visual order.
inline void LineLayout::reorder() throw() {
	if(numberOfRuns_ == 0)
		return;
	Run** temp = new Run*[numberOfRuns_];
	memcpy(temp, runs_, sizeof(Run*) * numberOfRuns_);
	for(length_t subline = 0; subline < numberOfSublines_; ++subline) {
		const size_t numberOfRunsInSubline = ((subline < numberOfSublines_ - 1) ?
			sublineFirstRuns_[subline + 1] : numberOfRuns_) - sublineFirstRuns_[subline];
		::BYTE* const levels = new ::BYTE[numberOfRunsInSubline];
		for(size_t i = 0; i < numberOfRunsInSubline; ++i)
			levels[i] = static_cast<::BYTE>(runs_[i + sublineFirstRuns_[subline]]->analysis.s.uBidiLevel & 0x1F);
		int* const log2vis = new int[numberOfRunsInSubline];
		const HRESULT hr = ::ScriptLayout(static_cast<int>(numberOfRunsInSubline), levels, 0, log2vis);
		assert(SUCCEEDED(hr));
		delete[] levels;
		for(size_t i = sublineFirstRuns_[subline]; i < sublineFirstRuns_[subline] + numberOfRunsInSubline; ++i)
			runs_[sublineFirstRuns_[subline] + log2vis[i - sublineFirstRuns_[subline]]] = temp[i];
		delete[] log2vis;
	}
	delete[] temp;
}

namespace {
	/**
	 * Builds glyphs into the run structure.
	 * @param dc the device context
	 * @param text to generate glyphs
	 * @param run the run to shape. if this function failed (except @c USP_E_SCRIPT_NOT_IN_FONT),
	 * both @c glyphs and @c visualAttributes members will be null
	 * @param[in,out] expectedNumberOfGlyphs the length of @a run.glyphs
	 * @retval S_OK succeeded
	 * @retval USP_E_SCRIPT_NOT_IN_FONT the font does not support the required script
	 * @retval E_OUTOFMEMORY failed to allocate buffer for glyph indices or visual attributes array
	 * @retval E_INVALIDARG other Uniscribe error. usually, too long run was specified
	 */
	HRESULT buildGlyphs(const DC& dc, const Char* text, Run& run, size_t& expectedNumberOfGlyphs) throw() {
		while(true) {
			HRESULT hr = ::ScriptShape(dc.getHandle(), &run.cache, text, static_cast<int>(run.length),
				static_cast<int>(expectedNumberOfGlyphs), &run.analysis, run.glyphs, run.clusters,
				run.visualAttributes, &run.numberOfGlyphs);
			if(hr == S_OK || hr == USP_E_SCRIPT_NOT_IN_FONT)
				return hr;
			delete[] run.glyphs; run.glyphs = 0;
			delete[] run.visualAttributes; run.visualAttributes = 0;
			if(hr != E_OUTOFMEMORY)
				return hr;
			// repeat until a large enough buffer is provided
			expectedNumberOfGlyphs *= 2;
			if(0 == (run.glyphs = new(nothrow) WORD[expectedNumberOfGlyphs]))
				return E_OUTOFMEMORY;
			if(0 == (run.visualAttributes = new(nothrow) ::SCRIPT_VISATTR[expectedNumberOfGlyphs])) {
				delete[] run.glyphs; run.glyphs = 0;
				return E_OUTOFMEMORY;
			}
		}
	}
	/**
	 * Returns true if the given run includes missing glyphs.
	 * @param dc the device context
	 * @param run the run
	 * @return true if missing glyphs are presented
	 */
	inline bool includesMissingGlyphs(const DC& dc, Run& run) throw() {
		::SCRIPT_FONTPROPERTIES fp;
		fp.cBytes = sizeof(::SCRIPT_FONTPROPERTIES);
		if(FAILED(ScriptGetFontProperties(dc.getHandle(), &run.cache, &fp)))
			return false;
		// following is not offical way, but from Mozilla (gfxWindowsFonts.cpp)
		for(int i = 0; i < run.numberOfGlyphs; ++i) {
			const WORD glyph = run.glyphs[i];
			if(glyph == fp.wgDefault || (glyph == fp.wgInvalid && glyph != fp.wgBlank))
				return true;
			else if(run.visualAttributes[i].fZeroWidth == 1 && scriptProperties.get(run.analysis.eScript).fComplex == 0)
				return true;
		}
		return false;
	}
	inline HRESULT generateGlyphs(const DC& dc, const Char* text, Run& run, size_t& expectedNumberOfGlyphs, bool checkMissingGlyphs) {
		HRESULT hr = buildGlyphs(dc, text, run, expectedNumberOfGlyphs);
		if(SUCCEEDED(hr) && checkMissingGlyphs && includesMissingGlyphs(dc, run))
			hr = S_FALSE;
		return hr;
	}
	/**
	 * Returns a Unicode script corresponds to Win32 language identifier for digit substitution.
	 * @param id the language identifier
	 * @return the script or @c NOT_PROPERTY
	 */
	inline int convertWin32LangIDtoUnicodeScript(::LANGID id) throw() {
		switch(id) {
		case LANG_ARABIC:		return Script::ARABIC;
		case LANG_ASSAMESE:		return Script::BENGALI;
		case LANG_BENGALI:		return Script::BENGALI;
		case 0x5C:				return Script::CHEROKEE;
		case LANG_DIVEHI:		return Script::THAANA;
		case 0x5E:				return Script::ETHIOPIC;
		case LANG_FARSI:		return Script::ARABIC;	// Persian
		case LANG_GUJARATI:		return Script::GUJARATI;
		case LANG_HINDI:		return Script::DEVANAGARI;
		case LANG_KANNADA:		return Script::KANNADA;
		case 0x53:				return Script::KHMER;
		case 0x54:				return Script::LAO;
		case LANG_MALAYALAM:	return Script::MALAYALAM;
		case 0x55:				return Script::MYANMAR;
		case LANG_ORIYA:		return Script::ORIYA;
		case LANG_PUNJABI:		return Script::GURMUKHI;
		case 0x5B:				return Script::SINHALA;
		case LANG_SYRIAC:		return Script::SYRIAC;
		case LANG_TAMIL:		return Script::TAMIL;
		case 0x51:				return Script::TIBETAN;
		case LANG_TELUGU:		return Script::TELUGU;
		case LANG_THAI:			return Script::THAI;
		case LANG_URDU:			return Script::ARABIC;
		}
		return NOT_PROPERTY;
	}
} // namespace @0

/**
 * Generates the glyphs for the text.
 * @param run the run to generate glyphs
 */
void LineLayout::shape(Run& run) throw() {
	assert(run.glyphs == 0);
	HRESULT hr;
	const Char* const text = getText().data() + run.column;
	ClientDC dc = const_cast<TextRenderer&>(renderer_).getTextViewer().getDC();
	run.clusters = new WORD[run.length];
	if(renderer_.getTextViewer().getConfiguration().inhibitsShaping)
		run.analysis.eScript = SCRIPT_UNDEFINED;

	HFONT oldFont;
	size_t expectedNumberOfGlyphs;
	if(run.analysis.s.fDisplayZWG != 0 && scriptProperties.get(run.analysis.eScript).fControl != 0) {
		// bidirectional format controls
		expectedNumberOfGlyphs = run.length;
		run.glyphs = new WORD[expectedNumberOfGlyphs];
		run.visualAttributes = new ::SCRIPT_VISATTR[expectedNumberOfGlyphs];
		oldFont = dc.selectObject(run.font = renderer_.getFontForShapingControls());
		if(USP_E_SCRIPT_NOT_IN_FONT == (hr = buildGlyphs(dc, text, run, expectedNumberOfGlyphs))) {
			assert(run.analysis.eScript != SCRIPT_UNDEFINED);
			run.analysis.eScript = SCRIPT_UNDEFINED;	// hmm...
			hr = buildGlyphs(dc, text, run, expectedNumberOfGlyphs);
			assert(SUCCEEDED(hr));
		}
		dc.selectObject(oldFont);
	} else {
		// we try candidate fonts in following order:
		//
		// 1. the primary font
		// 2. the national font for digit substitution
		// 3. the linked fonts
		// 4. the fallback font
		// 5. the primary font without shaping
		// 6. the linked fonts without shaping
		// 7. the fallback font without shaping
		expectedNumberOfGlyphs = run.length * 3 / 2 + 16;
		run.glyphs = new WORD[expectedNumberOfGlyphs];
		run.visualAttributes = new ::SCRIPT_VISATTR[expectedNumberOfGlyphs];
		int script = NOT_PROPERTY;	// script of the run for fallback
		set<HFONT> failedFonts;		// fonts failed to generate glyphs

		while(true) {
			// ScriptShape may crash if the shaping is disabled (see Mozilla bug 341500).
			// Following technique is also from Mozilla (gfxWindowsFonts.cpp).
			manah::AutoBuffer<Char> safeText;
			const bool textIsDanger = run.analysis.eScript == SCRIPT_UNDEFINED
				&& find_if(text, text + run.length, surrogates::isSurrogate) != text + run.length;
			if(textIsDanger) {
				safeText.reset(new Char[run.length]);
				wmemcpy(safeText.get(), text, run.length);
				replace_if(safeText.get(), safeText.get() + run.length, surrogates::isSurrogate, REPLACEMENT_CHARACTER);
			}
			const Char* p = !textIsDanger ? text : safeText.get();
			// 1/5. the primary font
			oldFont = dc.selectObject(run.font = renderer_.getFont(Script::COMMON, run.style.bold, run.style.italic));
			if(S_OK == (hr = generateGlyphs(dc, text, run, expectedNumberOfGlyphs, run.analysis.eScript != SCRIPT_UNDEFINED)))
				break;
			::ScriptFreeCache(&run.cache);
			failedFonts.insert(run.font);

			// 2. the national font for digit substitution
			if(hr == USP_E_SCRIPT_NOT_IN_FONT && run.analysis.eScript != SCRIPT_UNDEFINED && run.analysis.s.fDigitSubstitute != 0) {
				script = convertWin32LangIDtoUnicodeScript(scriptProperties.get(run.analysis.eScript).langid);
				if(script != NOT_PROPERTY && 0 != (run.font = renderer_.getFont(script, run.style.bold, run.style.italic))) {
					if(failedFonts.find(run.font) == failedFonts.end()) {
						dc.selectObject(run.font);
						if(S_OK == (hr = generateGlyphs(dc, text, run, expectedNumberOfGlyphs, true)))
							break;
						::ScriptFreeCache(&run.cache);
						failedFonts.insert(run.font);
					}
				}
			}

			// 3/6. the linked fonts
			for(size_t i = 0; i < renderer_.getNumberOfLinkedFonts(); ++i) {
				run.font = renderer_.getLinkedFont(i);
				if(failedFonts.find(run.font) == failedFonts.end()) {
					dc.selectObject(run.font);
					if(S_OK == (hr = generateGlyphs(dc, text, run, expectedNumberOfGlyphs, run.analysis.eScript != SCRIPT_UNDEFINED)))
						break;
					::ScriptFreeCache(&run.cache);
					failedFonts.insert(run.font);
				}
			}
			if(hr == S_OK)
				break;

			// 4/7. the fallback font
			if(script == NOT_PROPERTY) {
				for(StringCharacterIterator i(text, text + run.length); i.hasNext(); ++i) {
					script = Script::of(*i);
					if(script != Script::UNKNOWN && script != Script::COMMON && script != Script::INHERITED)
						break;
				}
			}
			if(script != Script::UNKNOWN && script != Script::COMMON && script != Script::INHERITED)
				run.font = renderer_.getFont(script, run.style.bold, run.style.italic);
			else {
				run.font = 0;
				// ambiguous CJK?
				if(script == Script::COMMON && scriptProperties.get(run.analysis.eScript).fAmbiguousCharSet != 0) {
					switch(CodeBlock::of(surrogates::decodeFirst(text, text + run.length))) {
					case CodeBlock::CJK_SYMBOLS_AND_PUNCTUATION:
					case CodeBlock::ENCLOSED_CJK_LETTERS_AND_MONTHS:
					case CodeBlock::CJK_COMPATIBILITY:
					case CodeBlock::VERTICAL_FORMS:	// as of GB 18030
					case CodeBlock::CJK_COMPATIBILITY_FORMS:
					case CodeBlock::SMALL_FORM_VARIANTS:	// as of CNS-11643
					case CodeBlock::HALFWIDTH_AND_FULLWIDTH_FORMS:
						run.font = renderer_.getFont(Script::HAN, run.style.bold, run.style.italic);
						break;
					}
				}
				if(run.font == 0 && runs_[0] != &run) {
					// use the previous run setting (but this will copy the style of the font...)
					const Run& previous = *find(runs_, runs_ + numberOfRuns_, &run)[-1];
					run.analysis.eScript = previous.analysis.eScript;
					run.font = previous.font;
				}
			}
			if(run.font != 0 && failedFonts.find(run.font) == failedFonts.end()) {
				dc.selectObject(run.font);
				if(S_OK == (hr = generateGlyphs(dc, text, run, expectedNumberOfGlyphs, run.analysis.eScript != SCRIPT_UNDEFINED)))
					break;
				::ScriptFreeCache(&run.cache);
			}

			if(run.analysis.eScript != SCRIPT_UNDEFINED)
				run.analysis.eScript = SCRIPT_UNDEFINED;	// disable shaping
			else
				assert(false);	// giveup...orz
			failedFonts.clear();
		}
	}

	run.advances = new int[run.numberOfGlyphs];
	run.glyphOffsets = new ::GOFFSET[run.numberOfGlyphs];
	hr = ::ScriptPlace(dc.getHandle(), &run.cache, run.glyphs, run.numberOfGlyphs,
			run.visualAttributes, &run.analysis, run.advances, run.glyphOffsets, &run.width);

	// query widths of C0 and C1 controls in this logical line
	if(ISpecialCharacterRenderer* scr = renderer_.getSpecialCharacterRenderer()) {
		ISpecialCharacterRenderer::LayoutContext context(dc);
		context.orientation = run.getOrientation();
		::SCRIPT_FONTPROPERTIES fp;
		fp.cBytes = 0;
		for(length_t i = 0; i < run.length; ++i) {
			if(isC0orC1Control(text[i])) {
				if(const int width = scr->getControlCharacterWidth(context, text[i])) {
					// substitute the glyph
					run.width.abcB += width - run.advances[i];
					run.advances[i] = width;
					if(fp.cBytes == 0) {
						fp.cBytes = sizeof(::SCRIPT_FONTPROPERTIES);
						::ScriptGetFontProperties(dc.getHandle(), &run.cache, &fp);
					}
					run.glyphs[i] = fp.wgBlank;
				}
			}
		}
	}
	dc.selectObject(oldFont);
}

/// Locates the wrap points and resolves tab expansions.
void LineLayout::wrap() throw() {
	assert(numberOfRuns_ != 0 && renderer_.getTextViewer().getConfiguration().lineWrap.wraps());
	assert(numberOfSublines_ == 0 && sublineOffsets_ == 0 && sublineFirstRuns_ == 0);

	const String& text = getText();
	vector<length_t> sublineFirstRuns;
	sublineFirstRuns.push_back(0);
	ClientDC dc = const_cast<TextRenderer&>(renderer_).getTextViewer().getDC();
	const int cookie = dc.save();
	const int wrapWidth = renderer_.getWrapWidth();
	int cx = 0;
	vector<Run*> newRuns;
	newRuns.reserve(numberOfRuns_ * 3 / 2);
	// for each runs... (at this time, runs_ is in logical order)
	for(size_t i = 0; i < numberOfRuns_; ++i) {
		Run& run = *runs_[i];

		// if the run is a tab, expand and calculate actual width
		if(text[run.column] == L'\t') {
			assert(run.length == 1);
			if(cx == wrapWidth) {
				cx = run.width.abcB = getNextTabStop(0, FORWARD);
				run.width.abcA = run.width.abcC = 0;
				newRuns.push_back(&run);
				sublineFirstRuns.push_back(newRuns.size());
			} else {
				run.width.abcB = min(getNextTabStop(cx, FORWARD), wrapWidth) - cx;
				run.width.abcA = run.width.abcC = 0;
				cx += run.getWidth();
				newRuns.push_back(&run);
			}
			run.advances[0] = run.getWidth();
			continue;
		}

		// obtain logical widths and attributes for all characters in this run to determine line break positions
		int* logicalWidths = new int[run.length];
		::SCRIPT_LOGATTR* logicalAttributes = new ::SCRIPT_LOGATTR[run.length];
		dc.selectObject(run.font);
		HRESULT hr = run.getLogicalWidths(logicalWidths);
		hr = ::ScriptBreak(text.data() + run.column, static_cast<int>(run.length), &run.analysis, logicalAttributes);
		const length_t originalRunPosition = run.column;
		int widthInThisRun = 0;
		length_t lastBreakable = run.column, lastGlyphEnd = run.column;
		int lastBreakableCx = cx, lastGlyphEndCx = cx;
		// for each characters in the run...
		for(length_t j = run.column; j < run.column + run.length; ) {	// j is position in the LOGICAL line
			const int x = cx + widthInThisRun;
			// remember this opportunity
			if(logicalAttributes[j - originalRunPosition].fCharStop != 0) {
				lastGlyphEnd = j;
				lastGlyphEndCx = x;
				if(logicalAttributes[j - originalRunPosition].fSoftBreak != 0
						|| logicalAttributes[j - originalRunPosition].fWhiteSpace != 0) {
					lastBreakable = j;
					lastBreakableCx = x;
				}
			}
			// break if the width of the visual line overs the wrap width
			if(x + logicalWidths[j - originalRunPosition] > wrapWidth) {
				// the opportunity is the start of this run
				if(lastBreakable == run.column) {
					// break at the last glyph boundary if no opportunities
					if(sublineFirstRuns.empty() || sublineFirstRuns.back() == newRuns.size()) {
						if(lastGlyphEnd == run.column) {	// break here if no glyph boundaries
							lastBreakable = j;
							lastBreakableCx = x;
						} else {
							lastBreakable = lastGlyphEnd;
							lastBreakableCx = lastGlyphEndCx;
						}
					}
				}

				// case 1: break at the start of the run
				if(lastBreakable == run.column) {
					assert(sublineFirstRuns.empty() || newRuns.size() != sublineFirstRuns.back());
					sublineFirstRuns.push_back(newRuns.size());
//dout << L"broke the line at " << lastBreakable << L" where the run start.\n";
				}
				// case 2: break at the end of the run
				else if(lastBreakable == run.column + run.length) {
					if(lastBreakable < text.length()) {
						assert(sublineFirstRuns.empty() || newRuns.size() != sublineFirstRuns.back());
						sublineFirstRuns.push_back(newRuns.size() + 1);
//dout << L"broke the line at " << lastBreakable << L" where the run end.\n";
					}
					break;
				}
				// case 3: break at the middle of the run -> split the run (run -> newRun + run)
				else {
					run.dispose();
					auto_ptr<Run> newRun(new Run(run));
					run.column = lastBreakable;
					newRun->length = run.column - newRun->column;
					assert(newRun->length != 0);
					run.length -= newRun->length;
					assert(run.length != 0);
//					newRun->analysis.fLinkAfter = run.analysis.fLinkBefore = 0;	// break link of glyphs
					newRuns.push_back(newRun.get());
					assert(sublineFirstRuns.empty() || newRuns.size() != sublineFirstRuns.back());
					sublineFirstRuns.push_back(newRuns.size());
//dout << L"broke the line at " << lastBreakable << L" where the run meddle.\n";
					shape(*newRun.release());
					shape(run);
				}
				widthInThisRun = cx + widthInThisRun - lastBreakableCx;
				lastBreakableCx -= cx;
				lastGlyphEndCx -= cx;
				cx = 0;
				j = max(lastBreakable, j);
			} else
				widthInThisRun += logicalWidths[j++ - originalRunPosition];
		}
		newRuns.push_back(&run);
		delete[] logicalWidths;
		delete[] logicalAttributes;
		cx += widthInThisRun;
	}
//dout << L"...broke the all lines.\n";
	dc.restore(cookie);
	if(newRuns.empty())
		newRuns.push_back(0);
	delete[] runs_;
	runs_ = new Run*[numberOfRuns_ = newRuns.size()];
	copy(newRuns.begin(), newRuns.end(), runs_);
	sublineFirstRuns_ = new length_t[numberOfSublines_ = sublineFirstRuns.size()];
	copy(sublineFirstRuns.begin(), sublineFirstRuns.end(), sublineFirstRuns_);
	sublineOffsets_ = new length_t[numberOfSublines_];
	for(size_t i = 0; i < numberOfSublines_; ++i)
		sublineOffsets_[i] = runs_[sublineFirstRuns_[i]]->column;
}


// LineLayout::StyledSegmentIterator ////////////////////////////////////////

/**
 * Private constructor.
 * @param start
 */
LineLayout::StyledSegmentIterator::StyledSegmentIterator(const Run*& start) throw() : p_(&start) {
}

/// Copy-constructor.
LineLayout::StyledSegmentIterator::StyledSegmentIterator(const StyledSegmentIterator& rhs) throw() : p_(rhs.p_) {
}

LineLayout::StyledSegmentIterator::reference LineLayout::StyledSegmentIterator::dereference() const throw() {
	return **p_;
}


// LineLayoutBuffer /////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param viewer the text viewer
 * @param bufferSize the maximum number of lines cached
 * @param autoRepair true to repair disposed layout automatically if the line number of its line was not changed
 * @throw std#invalid_argument @a bufferSize is zero
 */
LineLayoutBuffer::LineLayoutBuffer(TextViewer& viewer, length_t bufferSize, bool autoRepair) :
		viewer_(viewer), bufferSize_(bufferSize), autoRepair_(autoRepair), documentChangePhase_(NONE) {
	pendingCacheClearance_.first = pendingCacheClearance_.last = INVALID_INDEX;
	if(bufferSize == 0)
		throw invalid_argument("size of the buffer can't be zero.");
	viewer.getDocument().addPrenotifiedListener(*this);
}

/// Destructor.
LineLayoutBuffer::~LineLayoutBuffer() throw() {
//	clearCaches(startLine_, startLine_ + bufferSize_, false);
	for(list<LineLayout*>::iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i)
		delete *i;
	viewer_.getDocument().removePrenotifiedListener(*this);
}

/**
 * Clears the layout caches of the specified lines. This method calls @c #layoutModified.
 * @param first the start of lines
 * @param last the end of lines (exclusive. this line will not be cleared)
 * @param repair set true to recreate layouts for the lines. if true, this method calls
 * @c #layoutModified. otherwise calls @c #layoutDeleted
 * @throw std#invalid_argument @a first and/or @a last are invalid
 */
void LineLayoutBuffer::clearCaches(length_t first, length_t last, bool repair) {
	if(first > last /*|| last > viewer_.getDocument().getNumberOfLines()*/)
		throw invalid_argument("either line number is invalid.");
	if(documentChangePhase_ == ABOUT_CHANGE) {
		pendingCacheClearance_.first = (pendingCacheClearance_.first == INVALID_INDEX) ? first : min(first, pendingCacheClearance_.first);
		pendingCacheClearance_.last = (pendingCacheClearance_.last == INVALID_INDEX) ? last : max(last, pendingCacheClearance_.last);
		return;
	}
	if(first == last)
		return;

	const size_t originalSize = layouts_.size();
	length_t oldSublines = 0, cachedLines = 0;
	if(repair) {
		length_t newSublines = 0, actualFirst = last, actualLast = first;
		for(list<LineLayout*>::iterator i(layouts_.begin()); i != layouts_.end(); ++i) {
			LineLayout*& layout = *i;
			const length_t lineNumber = layout->getLineNumber();
			if(lineNumber >= first && lineNumber < last) {
				oldSublines += layout->getNumberOfSublines();
				delete layout;
				layout = new LineLayout(viewer_.getTextRenderer(), lineNumber);
				newSublines += layout->getNumberOfSublines();
				++cachedLines;
				actualFirst = min(actualFirst, lineNumber);
				actualLast = max(actualLast, lineNumber);
			}
		}
		if(actualFirst == last)	// no lines cleared
			return;
		++actualLast;
		layoutModified(actualFirst, actualLast, newSublines += actualLast - actualFirst - cachedLines,
			oldSublines += actualLast - actualFirst - cachedLines, documentChangePhase_ == CHANGING);
	} else {
		for(list<LineLayout*>::iterator i(layouts_.begin()); i != layouts_.end(); ++i) {
			if((*i)->getLineNumber() >= first && (*i)->getLineNumber() < last) {
				oldSublines += (*i)->getNumberOfSublines();
				delete *i;
				i = layouts_.erase(i);
				++cachedLines;
			}
		}
		layoutDeleted(first, last, oldSublines += last - first - cachedLines);
	}
}

/// @see text#IDocumentListener#documentAboutToBeChanged
void LineLayoutBuffer::documentAboutToBeChanged(const text::Document&) {
	documentChangePhase_ = ABOUT_CHANGE;
}

/// @see text#IDocumentListener#documentChanged
void LineLayoutBuffer::documentChanged(const text::Document&, const text::DocumentChange& change) {
	const length_t top = change.getRegion().getTop().line, bottom = change.getRegion().getBottom().line;
	documentChangePhase_ = CHANGING;
	if(top != bottom) {
		if(change.isDeletion()) {	// 改行を含む範囲の削除
			clearCaches(top + 1, bottom + 1, false);
			for(list<LineLayout*>::iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
				if((*i)->getLineNumber() > top)
					(*i)->lineNumber_ -= bottom - top;	// $friendly-access
			}
		} else {	// 改行を含む範囲の挿入
			for(list<LineLayout*>::iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
				if((*i)->getLineNumber() > top)
					(*i)->lineNumber_ += bottom - top;	// $friendly-access
			}
			layoutInserted(top + 1, bottom + 1);
		}
	}
	if(pendingCacheClearance_.first == INVALID_INDEX
			|| top < pendingCacheClearance_.first || top >= pendingCacheClearance_.last)
		invalidate(top);
	documentChangePhase_ = NONE;
	if(pendingCacheClearance_.first != INVALID_INDEX) {
		clearCaches(pendingCacheClearance_.first, pendingCacheClearance_.last, autoRepair_);
		pendingCacheClearance_.first = pendingCacheClearance_.last = INVALID_INDEX;
	}
}

/**
 * Returns the layout of the specified line.
 * @param line the line
 * @return the layout
 * @throw text#BadPositionException @a line is greater than the number of the lines
 */
const LineLayout& LineLayoutBuffer::getLineLayout(length_t line) const {
#ifdef TRACE_LAYOUT_CACHES
	dout << "finding layout for line " << line;
#endif
	if(line > viewer_.getDocument().getNumberOfLines())
		throw text::BadPositionException();
	LineLayoutBuffer& self = *const_cast<LineLayoutBuffer*>(this);
	list<LineLayout*>::iterator i(self.layouts_.begin());
	for(const list<LineLayout*>::iterator e(self.layouts_.end()); i != e; ++i) {
		if((*i)->lineNumber_ == line)
			break;
	}

	if(i != layouts_.end()) {
#ifdef TRACE_LAYOUT_CACHES
		dout << "... cache found\n";
#endif
		LineLayout* layout = *i;
		if(layout != layouts_.front()) {
			// bring to the top
			self.layouts_.erase(i);
			self.layouts_.push_front(layout);
		}
		return *layout;
	} else {
#ifdef TRACE_LAYOUT_CACHES
		dout << "... cache not found\n";
#endif
		if(layouts_.size() == bufferSize_) {
			// delete the last
			LineLayout* p = layouts_.back();
			self.layouts_.pop_back();
			self.layoutModified(p->getLineNumber(), p->getLineNumber() + 1,
				1, p->getNumberOfSublines(), documentChangePhase_ == CHANGING);
			delete p;
		}
		LineLayout* const layout = new LineLayout(viewer_.getTextRenderer(), line);
		self.layouts_.push_front(layout);
		self.layoutModified(line, line + 1, layout->getNumberOfSublines(), 1, documentChangePhase_ == CHANGING);
		return *layout;
	}
}

/// Invalidates all layouts.
void LineLayoutBuffer::invalidate() {
	clearCaches(0, viewer_.getDocument().getNumberOfLines(), autoRepair_);
}

/**
 * Invalidates the layouts of the specified lines.
 * @param first the start of the lines
 * @param last the end of the lines (exclusive. this line will not be cleared)
 * @throw std#invalid_argument @a first &gt;= @a last
 */
void LineLayoutBuffer::invalidate(length_t first, length_t last) {
	if(first >= last)
		throw invalid_argument("Any line number is invalid.");
	clearCaches(first, last, autoRepair_);
}

/**
 * Resets the cached layout of the specified line and repairs if necessary.
 * @param line the line to invalidate layout
 */
inline void LineLayoutBuffer::invalidate(length_t line) throw() {
	for(list<LineLayout*>::iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
		LineLayout*& p = *i;
		if(p->getLineNumber() == line) {
			const length_t oldSublines = p->getNumberOfSublines();
			delete p;
			if(autoRepair_) {
				p = new LineLayout(viewer_.getTextRenderer(), line);
				layoutModified(line, line + 1, p->getNumberOfSublines(), oldSublines, documentChangePhase_ == CHANGING);
			} else {
				layouts_.erase(i);
				layoutModified(line, line + 1, 1, oldSublines, documentChangePhase_ == CHANGING);
			}
			break;
		}
	}
}

/**
 * @fn void ascension::viewers::LineLayoutBuffer::layoutDeleted(length_t, length_t, length_t)
 * The layouts of lines were deleted.
 * @param first the start of lines to be deleted
 * @param last the end of lines (exclusive) to be deleted
 * @param sublines the number of sublines of the deleted lines
 */

/**
 * @fn void ascension::viewers::LineLayoutBuffer::layoutInserted(length_t, length_t)
 * The new layouts of lines were inserted.
 * @param first the start of lines to be inserted
 * @param last the end of lines (exclusive) to be inserted
 */

/**
 * @fn void ascension::viewers::LineLayoutBuffer::layoutModified(length_t, length_t, length_t, length_t, bool)
 * The layouts of lines were modified.
 * @param first the start of the lines to be modified
 * @param last the end of the lines (exclusive) to be modified
 * @param newSublines the number of sublines of the modified lines after the modification
 * @param oldSublines the number of sublines of the modified lines before the modification
 * @param documentChanged true if the modification was occured by the document change
 */

/**
 * Returns the first visual line number of the specified logical line.
 * @param line the logical line
 * @return the first visual line of @a line
 * @throw text#BadPositionException @a line is outside of the document
 * @see #mapLogicalPositionToVisualPosition, #mapVisualLineToLogicalLine
 */
length_t LineLayoutBuffer::mapLogicalLineToVisualLine(length_t line) const {
	if(line >= getTextViewer().getDocument().getNumberOfLines())
		throw text::BadPositionException();
	else if(!getTextViewer().getConfiguration().lineWrap.wraps())
		return line;
	length_t result = 0, cachedLines = 0;
	for(Iterator i(getFirstCachedLine()), e(getLastCachedLine()); i != e; ++i) {
		if((*i)->getLineNumber() < line) {
			result += (*i)->getNumberOfSublines();
			++cachedLines;
		}
	}
	return result + line - cachedLines;
}

/**
 * Returns the visual line number and the visual column number of the specified logical position.
 * @param position the logical coordinates of the position to be mapped
 * @param[out] column the visual column of @a position. can be @c null if not needed
 * @return the visual line of @a position
 * @throw text#BadPositionException @a position is outside of the document
 * @see #mapLogicalLineToVisualLine, #mapVisualPositionToLogicalPosition
 */
length_t LineLayoutBuffer::mapLogicalPositionToVisualPosition(const Position& position, length_t* column) const {
	if(!getTextViewer().getConfiguration().lineWrap.wraps()) {
		if(column != 0)
			*column = position.column;
		return position.line;
	}
	const LineLayout& layout = getLineLayout(position.line);
	const length_t subline = layout.getSubline(position.column);
	if(column != 0)
		*column = position.column - layout.getSublineOffset(subline);
	return mapLogicalLineToVisualLine(position.line) + subline;
}

#if 0
/**
 * Returns the logical line number and the visual subline number of the specified visual line.
 * @param line the visual line
 * @param[out] subline the visual subline of @a line. can be @c null if not needed
 * @return the logical line
 * @throw text#BadPositionException @a line is outside of the document
 * @see #mapLogicalLineToVisualLine, #mapVisualPositionToLogicalPosition
 */
length_t LineLayoutBuffer::mapVisualLineToLogicalLine(length_t line, length_t* subline) const {
	if(!getTextViewer().getConfiguration().lineWrap.wraps()) {
		if(subline != 0)
			*subline = 0;
		return line;
	}
	length_t c = getCacheFirstLine();
	for(length_t i = getCacheFirstLine(); ; ++i) {
		if(c + getNumberOfSublinesOfLine(i) > line) {
			if(subline != 0)
				*subline = line - c;
			return i;
		}
		c += getNumberOfSublinesOfLine(i);
	}
	assert(false);
	return getCacheLastLine();	// ここには来ない
}

/**
 * Returns the logical line number and the logical column number of the specified visual position.
 * @param position the visual coordinates of the position to be mapped
 * @return the logical coordinates of @a position
 * @throw text#BadPositionException @a position is outside of the document
 * @see #mapLogicalPositionToVisualPosition, #mapVisualLineToLogicalLine
 */
Position LineLayoutBuffer::mapVisualPositionToLogicalPosition(const Position& position) const {
	if(!getTextViewer().getConfiguration().lineWrap.wraps())
		return position;
	Position result;
	length_t subline;
	result.line = mapVisualLineToLogicalLine(position.line, &subline);
	result.column = getLineLayout(result.line).getSublineOffset(subline) + position.column;
	return result;
}
#endif /* 0 */

/// @see presentation#IPresentationStylistListener
void LineLayoutBuffer::presentationStylistChanged() {
	invalidate();
}


// ISpecialCharacterRenderer //////////////////////////////////////////////////

/**
 * @class ascension::viewers::ISpecialCharacterRenderer
 * Interface for objects which draw special characters.
 *
 * @c ISpecialCharacterRenderer hooks shaping and drawing processes of @c LineLayout about some
 * special characters. These include:
 * - C0 controls
 * - C1 controls
 * - End of line (Line terminators)
 * - White space characters
 * - Line wrapping marks
 *
 * <h2>Characters @c ISpecialCharacterRenderer can render</h2>
 *
 * <em>C0 controls</em> include characters whose code point is U+0000..001F or U+007F. But U+0009,
 * U+000A, and U+000D are excluded. These characters can be found in "White space characters" and
 * "End of line".
 *
 * <em>C1 controls</em> include characters whose code point is U+0080..009F. But only U+0085 is
 * excluded. This is one of "End of line" character.
 *
 * <em>End of line</em> includes any NLFs in Unicode. Identified by @c text#Newline enumeration.
 *
 * <em>White space characters</em> include all Unicode white spaces and horizontal tab (U+0009). An
 * instance of @c ISpecialCharacterRenderer can't set the width of these glyphs.
 *
 * <em>Line wrapping marks</em> indicate a logical is wrapped visually. Note that this is not an
 * actual character.
 *
 * <h2>Process</h2>
 *
 * @c ISpecialCharacterRenderer will be invoked at the following two stages.
 * -# To layout a special character.
 * -# To draw a special character.
 *
 * (1) When layout of a line is needed, @c TextRenderer creates and initializes a @c LineLayout.
 * In this process, the widths of the all characters in the line are calculated by Unicode script
 * processor (Uniscribe). For the above special characters, @c LineLayout queries the widths to
 * @c ISpecialCharacterRenderer (However, for white spaces, this query is not performed).
 *
 * (2) When a line is drawn, @c LineLayout#draw calls @c ISpecialCharacterRenderer::drawXxxx
 * methods to draw special characters with the device context, the orientation, and the rectangle
 * to paint.
 *
 * @see TextRenderer, TextRenderer#setSpecialCharacterRenderer
 */


// DefaultSpecialCharacterRenderer //////////////////////////////////////////

namespace {
	inline void getControlPresentationString(CodePoint c, Char* buffer) {
		buffer[0] = L'^';
		buffer[1] = (c != 0x7F) ? static_cast<Char>(c) + 0x40 : L'?';
	}
}

/**
 * @class ascension::viewers::DefaultSpecialCharacterRenderer
 *
 * Default implementation of @c ISpecialCharacterRenderer interface. This renders special
 * characters with the glyphs provided by the standard international font "Lucida Sans Unicode".
 * The mapping special characters to characters provide glyphs are as follows:
 * - Horizontal tab (LTR) : U+2192 Rightwards Arrow (&#x2192)
 * - Horizontal tab (RTL) : U+2190 Leftwards Arrow (&#x2190)
 * - Line terminator : U+2193 Downwards Arrow (&#x2193)
 * - Line wrapping mark (LTR) : U+21A9 Leftwards Arrow With Hook (&#x21A9)
 * - Line wrapping mark (RTL) : U+21AA Rightwards Arrow With Hook (&#x21AA)
 * - White space : U+00B7 Middle Dot (&#x00B7)
 *
 * Default foreground colors of glyphs are as follows:
 * - Control characters : RGB(0x80, 0x80, 0x00)
 * - Line terminators : RGB(0x00, 0x80, 0x80)
 * - Line wrapping markers: RGB(0x00, 0x80, 0x80)
 * - White space characters : RGB(0x00, 0x80, 0x80)
 */

/// Default constructor.
DefaultSpecialCharacterRenderer::DefaultSpecialCharacterRenderer() throw() : renderer_(0),
		controlColor_(RGB(0x80, 0x80, 0x00)), eolColor_(RGB(0x00, 0x80, 0x80)), wrapMarkColor_(RGB(0x00, 0x80, 0x80)),
		whiteSpaceColor_(RGB(0x00, 0x80, 0x80)), showsEOLs_(true), showsWhiteSpaces_(true), font_(0) {
}

/// Destructor.
DefaultSpecialCharacterRenderer::~DefaultSpecialCharacterRenderer() throw() {
	::DeleteObject(font_);
	font_ = 0;
}

/// @see ISpecialCharacterRenderer#drawControlCharacter
void DefaultSpecialCharacterRenderer::drawControlCharacter(const DrawingContext& context, CodePoint c) const {
	HFONT oldFont = context.dc.selectObject(renderer_->getFont());
	context.dc.setTextColor(controlColor_);
	Char buffer[2];
	getControlPresentationString(c, buffer);
	context.dc.extTextOut(context.rect.left, context.rect.top + renderer_->getAscent(), 0, 0, buffer, 2, 0);
	context.dc.selectObject(oldFont);
}

/// @see ISpecialCharacterRenderer#drawLineTerminator
void DefaultSpecialCharacterRenderer::drawLineTerminator(const DrawingContext& context, text::Newline) const {
	if(showsEOLs_ && glyphs_[LINE_TERMINATOR] != 0xFFFF) {
		HFONT oldFont = context.dc.selectObject(toBoolean(glyphWidths_[LINE_TERMINATOR] & 0x80000000) ? font_ : renderer_->getFont());
		context.dc.setTextColor(eolColor_);
		context.dc.extTextOut(context.rect.left,
			context.rect.top + renderer_->getAscent(), ETO_GLYPH_INDEX, 0, reinterpret_cast<const WCHAR*>(&glyphs_[LINE_TERMINATOR]), 1, 0);
		context.dc.selectObject(oldFont);
	}
}

/// @see ISpecialCharacterRenderer#drawLineWrappingMark
void DefaultSpecialCharacterRenderer::drawLineWrappingMark(const DrawingContext& context) const {
	const int id = (context.orientation == LEFT_TO_RIGHT) ? LTR_WRAPPING_MARK : RTL_WRAPPING_MARK;
	const ::WCHAR glyph = glyphs_[id];
	if(glyph != 0xFFFF) {
		HFONT oldFont = context.dc.selectObject(toBoolean(glyphWidths_[id] & 0x80000000) ? font_ : renderer_->getFont());
		context.dc.setTextColor(wrapMarkColor_);
		context.dc.extTextOut(context.rect.left, context.rect.top + renderer_->getAscent(), ETO_GLYPH_INDEX, 0, &glyph, 1, 0);
		context.dc.selectObject(oldFont);
	}
}

/// @see ISpecialCharacterRenderer#drawWhiteSpaceCharacter
void DefaultSpecialCharacterRenderer::drawWhiteSpaceCharacter(const DrawingContext& context, CodePoint c) const {
	if(!showsWhiteSpaces_)
		return;
	else if(c == 0x0009) {
		const int id = (context.orientation == LEFT_TO_RIGHT) ? LTR_HORIZONTAL_TAB : RTL_HORIZONTAL_TAB;
		const ::WCHAR glyph = glyphs_[id];
		if(glyph != 0xFFFF) {
			HFONT oldFont = context.dc.selectObject(toBoolean(glyphWidths_[id] & 0x80000000) ? font_ : renderer_->getFont());
			const int glyphWidth = glyphWidths_[id] & 0x7FFFFFFF;
			const int x =
				((context.orientation == LEFT_TO_RIGHT && glyphWidth < context.rect.right - context.rect.left)
					|| (context.orientation == RIGHT_TO_LEFT && glyphWidth > context.rect.right - context.rect.left)) ?
				context.rect.left : context.rect.right - glyphWidth;
			context.dc.setTextColor(whiteSpaceColor_);
			context.dc.extTextOut(x, context.rect.top + renderer_->getAscent(), ETO_CLIPPED | ETO_GLYPH_INDEX, &context.rect, &glyph, 1, 0);
			context.dc.selectObject(oldFont);
		}
	} else if(glyphs_[WHITE_SPACE] != 0xFFFF) {
		HFONT oldFont = context.dc.selectObject(toBoolean(glyphWidths_[WHITE_SPACE] & 0x80000000) ? font_ : renderer_->getFont());
		context.dc.setTextColor(whiteSpaceColor_);
		context.dc.extTextOut((context.rect.left + context.rect.right - (glyphWidths_[WHITE_SPACE] & 0x7FFFFFFF)) / 2,
			context.rect.top + renderer_->getAscent(), ETO_CLIPPED | ETO_GLYPH_INDEX, &context.rect,
			reinterpret_cast<const WCHAR*>(&glyphs_[WHITE_SPACE]), 1, 0);
		context.dc.selectObject(oldFont);
	}
}

/// @see IFontSelectorListener#fontChanged
void DefaultSpecialCharacterRenderer::fontChanged() {
	static const Char codes[] = {0x2192, 0x2190, 0x2193, 0x21A9, 0x21AA, 0x00B7};

	// using the primary font
	ScreenDC dc;
	HFONT oldFont = dc.selectObject(renderer_->getFont());
	dc.getGlyphIndices(codes, countof(codes), glyphs_, GGI_MARK_NONEXISTING_GLYPHS);
	dc.getCharWidthI(glyphs_, countof(codes), glyphWidths_);

	// using the fallback font
	::DeleteObject(font_);
	font_ = 0;
	if(find(glyphs_, endof(glyphs_), 0xFFFF) != endof(glyphs_)) {
		::LOGFONTW lf;
		::GetObject(renderer_->getFont(), sizeof(::LOGFONTW), &lf);
		lf.lfWeight = FW_REGULAR;
		lf.lfItalic = lf.lfUnderline = lf.lfStrikeOut = false;
		wcscpy(lf.lfFaceName, L"Lucida Sans Unicode");
		dc.selectObject(font_ = ::CreateFontIndirectW(&lf));
		::WORD g[countof(glyphs_)];
		int w[countof(glyphWidths_)];
		dc.getGlyphIndices(codes, countof(codes), g, GGI_MARK_NONEXISTING_GLYPHS);
		dc.getCharWidthI(g, countof(codes), w);
		for(int i = 0; i < countof(glyphs_); ++i) {
			if(glyphs_[i] == 0xFFFF) {
				if(g[i] != 0xFFFF) {
					glyphs_[i] = g[i];
					glyphWidths_[i] = w[i] | 0x80000000;
				} else
					glyphWidths_[i] = 0;	// missing
			}
		}
	}

	dc.selectObject(oldFont);
}

/// @see ISpecialCharacterRenderer#getControlCharacterWidth
int DefaultSpecialCharacterRenderer::getControlCharacterWidth(const LayoutContext& context, CodePoint c) const {
	Char buffer[2];
	getControlPresentationString(c, buffer);
	HFONT oldFont = context.dc.selectObject(renderer_->getFont());
	const int result = context.dc.getTextExtent(buffer, 2).cx;
	context.dc.selectObject(oldFont);
	return result;
}

/// @see ISpecialCharacterRenderer#getLineTerminatorWidth
int DefaultSpecialCharacterRenderer::getLineTerminatorWidth(const LayoutContext&, text::Newline) const {
	return showsEOLs_ ? (glyphWidths_[LINE_TERMINATOR] & 0x7FFFFFFF) : 0;
}

/// @see ISpecialCharacterRenderer#getLineWrappingMarkWidth
int DefaultSpecialCharacterRenderer::getLineWrappingMarkWidth(const LayoutContext& context) const {
	return glyphWidths_[(context.orientation == LEFT_TO_RIGHT) ? LTR_WRAPPING_MARK : RTL_WRAPPING_MARK] & 0x7FFFFFFF;
}

/// @see ISpecialCharacterRenderer#install
void DefaultSpecialCharacterRenderer::install(TextRenderer& renderer) {
	(renderer_ = &renderer)->addFontListener(*this);
	fontChanged();
}

/// @see ISpecialCharacterRenderer#uninstall
void DefaultSpecialCharacterRenderer::uninstall() {
	renderer_->removeFontListener(*this);
	renderer_ = 0;
}


// FontSelector /////////////////////////////////////////////////////////////

namespace {
	inline HFONT copyFont(HFONT src) throw() {
		::LOGFONTW lf;
		::GetObjectW(src, sizeof(::LOGFONTW), &lf);
		return ::CreateFontIndirectW(&lf);
	}
}

struct FontSelector::Fontset : private manah::Noncopyable {
	WCHAR faceName[LF_FACESIZE];
	HFONT regular, bold, italic, boldItalic;
	explicit Fontset(const WCHAR* name) throw() : regular(0), bold(0), italic(0), boldItalic(0) {
		wcscpy(faceName, name);
	}
	Fontset(const Fontset& rhs) throw() : regular(0), bold(0), italic(0), boldItalic(0) {
		wcscpy(faceName, rhs.faceName);
	}
	~Fontset() throw() {
		clear(L"");
	}
	void clear(const WCHAR* newName = 0) throw() {
		::DeleteObject(regular);
		::DeleteObject(bold);
		::DeleteObject(italic);
		::DeleteObject(boldItalic);
		regular = bold = italic = boldItalic = 0;
		if(newName != 0)
			wcscpy(faceName, newName);
	}
};

FontSelector::FontAssociations FontSelector::defaultAssociations_;

/// Constructor.
FontSelector::FontSelector() : primaryFont_(new Fontset(L"")), shapingControlsFont_(0), linkedFonts_(0) {
	resetPrimaryFont(ScreenDC(), copyFont(static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT))));
}

/// Copy-constructor.
FontSelector::FontSelector(const FontSelector& rhs) : primaryFont_(new Fontset(L"")), shapingControlsFont_(0), linkedFonts_(0) {
	resetPrimaryFont(ScreenDC(), copyFont(rhs.primaryFont_->regular));
	for(map<int, Fontset*>::const_iterator i(rhs.associations_.begin()), e(rhs.associations_.end()); i != e; ++i)
		associations_.insert(make_pair(i->first, new Fontset(i->second->faceName)));
	if(rhs.linkedFonts_ != 0) {
		linkedFonts_ = new vector<Fontset*>;
		for(vector<Fontset*>::const_iterator i(rhs.linkedFonts_->begin()), e(rhs.linkedFonts_->end()); i != e; ++i)
			linkedFonts_->push_back(new Fontset(**i));
	}
}

/// Destructor.
FontSelector::~FontSelector() throw() {
	::DeleteObject(shapingControlsFont_);
	delete primaryFont_;
	for(map<int, Fontset*>::iterator i = associations_.begin(); i != associations_.end(); ++i)
		delete i->second;
	if(linkedFonts_ != 0) {
		for(vector<Fontset*>::iterator i(linkedFonts_->begin()); i != linkedFonts_->end(); ++i)
			delete *i;
		delete linkedFonts_;
	}
}

/**
 * Enables or disables the font linking feature for CJK. When this method is called, #fontChanged
 * method of the derived class will be called.
 * @param enable set true to enable
 */
void FontSelector::enableFontLinking(bool enable /* = true */) throw() {
	if(enable) {
		if(linkedFonts_ == 0)
			linkedFonts_ = new std::vector<Fontset*>;
		linkPrimaryFont();
	} else if(linkedFonts_ != 0) {
		delete linkedFonts_;
		linkedFonts_ = 0;
	}
}

inline void FontSelector::fireFontChanged() {
	fontChanged();
	listeners_.notify(IFontSelectorListener::fontChanged);
}

namespace {
	int CALLBACK checkFontInstalled(::ENUMLOGFONTEXW*, ::NEWTEXTMETRICEXW*, DWORD, LPARAM param) {
		*reinterpret_cast<bool*>(param) = true;
		return 0;
	}
}

/// Returns the default font association (fallback) map.
const FontSelector::FontAssociations& FontSelector::getDefaultFontAssociations() throw() {
	if(defaultAssociations_.empty()) {
		defaultAssociations_[Script::ARABIC] = L"Microsoft Sans Serif";
		defaultAssociations_[Script::ARMENIAN] = L"Sylfaen";
		defaultAssociations_[Script::BENGALI] = L"Vrinda";
		defaultAssociations_[Script::CYRILLIC] = L"Microsoft Sans Serif";	// Segoe UI is an alternative
		defaultAssociations_[Script::DEVANAGARI] = L"Mangal";
		defaultAssociations_[Script::GEORGIAN] = L"Sylfaen";	// partial support
		defaultAssociations_[Script::GREEK] = L"Microsoft Sans Serif";
		defaultAssociations_[Script::GUJARATI] = L"Shruti";
		defaultAssociations_[Script::GURMUKHI] = L"Raavi";
		defaultAssociations_[Script::HANGUL] = L"Gulim";
		defaultAssociations_[Script::HEBREW] = L"Microsoft Sans Serif";
//		defaultAssociations_[Script::HIRAGANA] = L"MS P Gothic";
		defaultAssociations_[Script::KANNADA] = L"Tunga";
//		defaultAssociations_[Script::KATAKANA] = L"MS P Gothic";
		defaultAssociations_[Script::LATIN] = L"Tahoma";
		defaultAssociations_[Script::MALAYALAM] = L"Kartika";
    defaultAssociations_[Script::SYRIAC] = L"Estrangelo Edessa";
		defaultAssociations_[Script::TAMIL] = L"Latha";
		defaultAssociations_[Script::TELUGU] = L"Gautami";
		defaultAssociations_[Script::THAI] = L"Tahoma";
    defaultAssociations_[Script::THAANA] = L"MV Boli";
		const LANGID uiLang = getUserDefaultUILanguage();
		switch(PRIMARYLANGID(uiLang)) {	// yes, this is not enough...
		case LANG_CHINESE:
			defaultAssociations_[Script::HAN] = (SUBLANGID(uiLang) == SUBLANG_CHINESE_TRADITIONAL
				&& SUBLANGID(uiLang) == SUBLANG_CHINESE_HONGKONG) ? L"PMingLiu" : L"SimSun"; break;
		case LANG_JAPANESE:
			defaultAssociations_[Script::HAN] = L"MS P Gothic"; break;
		case LANG_KOREAN:
			defaultAssociations_[Script::HAN] = L"Gulim"; break;
		default:
			{
				ScreenDC dc;
				bool installed = false;
				::LOGFONTW lf;
				memset(&lf, 0, sizeof(::LOGFONTW));
#define ASCENSION_SELECT_INSTALLED_FONT(charset, fontname)		\
	lf.lfCharSet = charset;										\
	wcscpy(lf.lfFaceName, fontname);							\
	::EnumFontFamiliesExW(dc.getHandle(), &lf,					\
		reinterpret_cast<::FONTENUMPROCW>(checkFontInstalled),	\
		reinterpret_cast<LPARAM>(&installed), 0);				\
	if(installed) {												\
		defaultAssociations_[Script::HAN] = lf.lfFaceName;		\
		break;													\
	}
				ASCENSION_SELECT_INSTALLED_FONT(GB2312_CHARSET, L"SimSun")
				ASCENSION_SELECT_INSTALLED_FONT(SHIFTJIS_CHARSET, L"\xFF2D\xFF33 \xFF30\x30B4\x30B7\x30C3\x30AF")	// "ＭＳ Ｐゴシック"
				ASCENSION_SELECT_INSTALLED_FONT(HANGUL_CHARSET, L"Gulim")
				ASCENSION_SELECT_INSTALLED_FONT(CHINESEBIG5_CHARSET, L"PMingLiu")
#undef ASCENSION_SELECT_INSTALLED_FONT
			}
			break;
		}
		if(defaultAssociations_.find(Script::HAN) != defaultAssociations_.end())
			defaultAssociations_[Script::HIRAGANA] = defaultAssociations_[Script::KATAKANA] = defaultAssociations_[Script::HAN];
	}
	return defaultAssociations_;
}

/**
 * Returns the font associated to the specified script.
 * @param script the language. set to @c Script#COMMON to get the primary font. other special
 * script values @c Script#UNKNOWN, @c Script#INHERITED and @c Script#KATAKANA_OR_HIRAGANA can't set
 * @param bold true to get the bold variant
 * @param italic true to get the italic variant
 * @return the primary font if @p script is @c Script#COMMON. otherwise, a fallbacked font or @c null
 * @throw std#invalid_argument @a script is invalid
 * @see #getLinkedFont, #setFont
 */
HFONT FontSelector::getFont(int script /* = Script::COMMON */, bool bold /* = false */, bool italic /* = false */) const {
	if(script <= Script::FIRST_VALUE || script == Script::INHERITED
			|| script == Script::KATAKANA_OR_HIRAGANA || script >= Script::LAST_VALUE)
		throw invalid_argument("invalid script value.");
	if(script == Script::COMMON)
		return getFontInFontset(*const_cast<FontSelector*>(this)->primaryFont_, bold, italic);
	else {
		map<int, Fontset*>::iterator i = const_cast<FontSelector*>(this)->associations_.find(script);
		return (i != associations_.end()) ? getFontInFontset(*i->second, bold, italic) : 0;
	}
}

/// Returns the font to render shaping control characters.
HFONT FontSelector::getFontForShapingControls() const throw() {
	if(shapingControlsFont_ == 0)
		const_cast<FontSelector*>(this)->shapingControlsFont_ = ::CreateFontW(
			ascent_ + descent_, 0, 0, 0, FW_REGULAR, false, false, false,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
	return shapingControlsFont_;
}

/**
 * Returns the font in the given font set.
 * @param fontset the font set
 * @param bold set true to get a bold font
 * @param italic set true to get an italic font
 * @return the font
 */
HFONT FontSelector::getFontInFontset(const Fontset& fontset, bool bold, bool italic) const throw() {
	Fontset& fs = const_cast<Fontset&>(fontset);
	HFONT& font = bold ? (italic ? fs.boldItalic : fs.bold) : (italic ? fs.italic : fs.regular);
	if(font == 0) {
		font = ::CreateFontW(-(ascent_ + descent_),
			0, 0, 0, bold ? FW_BOLD : FW_REGULAR, italic, 0, 0, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fs.faceName);
		auto_ptr<DC> dc = getDC();
		HFONT oldFont = dc->selectObject(font);
		::TEXTMETRICW tm;
		dc->getTextMetrics(tm);
		dc->selectObject(oldFont);
		// adjust to the primary ascent and descent
		if(tm.tmAscent > ascent_ && tm.tmAscent > 0) {	// we don't consider the descents...
			::DeleteObject(font);
			const double ratio = static_cast<double>(ascent_) / static_cast<double>(tm.tmAscent);
			font = ::CreateFontW(static_cast<int>(-static_cast<double>(ascent_ + descent_) * ratio),
				0, 0, 0, bold ? FW_BOLD : FW_REGULAR, italic, 0, 0, DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fs.faceName);
		}
	}
	return font;
}

/**
 * Returns the font linking to the primary font.
 * @param index the index in the link fonts chain
 * @param bold true to get the bold variant
 * @param italic true to get the italic variant
 * @return the font
 * @throw std#out_of_range @a index is invalid
 * @see #getFont, #getNumberOfLinkedFonts
 */
HFONT FontSelector::getLinkedFont(size_t index, bool bold /* = false */, bool italic /* = false */) const {
	if(linkedFonts_ == 0)
		throw out_of_range("the specified index is invalid.");
	return getFontInFontset(*linkedFonts_->at(index), bold, italic);
}

///
void FontSelector::linkPrimaryFont() throw() {
	assert(linkedFonts_ != 0);
	for(vector<Fontset*>::iterator i(linkedFonts_->begin()); i != linkedFonts_->end(); ++i)
		delete *i;
	linkedFonts_->clear();

	// レジストリからフォントリンクの設定を読み込む
	::HKEY key;
	long e = ::RegOpenKeyExW(HKEY_CURRENT_USER,
		L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontLink\\SystemLink", 0, KEY_READ, &key);
	if(e != ERROR_SUCCESS)
		e = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE,
			L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontLink\\SystemLink", 0, KEY_READ, &key);
	if(e == ERROR_SUCCESS) {
		DWORD type, bytes;
		if(ERROR_SUCCESS == ::RegQueryValueExW(key, primaryFont_->faceName, 0, &type, 0, &bytes)) {
			manah::AutoBuffer<::BYTE> data(new ::BYTE[bytes]);
			if(ERROR_SUCCESS == ::RegQueryValueExW(key, primaryFont_->faceName, 0, &type, data.get(), &bytes)) {
				const wchar_t* p = reinterpret_cast<wchar_t*>(data.get());
				const wchar_t* const last = p + bytes / sizeof(wchar_t);
				while(true) {
					p = find(p, last, L',');
					if(p == last)
						break;
					linkedFonts_->push_back(new Fontset(++p));
					p = find(p, last, 0);
				}
			}
		}
		::RegCloseKey(key);
	}
	fireFontChanged();
}

void FontSelector::resetPrimaryFont(DC& dc, HFONT font) {
	assert(font != 0);
	::TEXTMETRICW tm;
	HFONT oldFont = dc.selectObject(primaryFont_->regular = font);
	dc.getTextMetrics(tm);
	ascent_ = tm.tmAscent;
	descent_ = tm.tmDescent;
	internalLeading_ = tm.tmInternalLeading;
	externalLeading_ = tm.tmExternalLeading;
	averageCharacterWidth_ = tm.tmAveCharWidth;
	dc.selectObject(oldFont);
	// real name is needed for font linking
	::LOGFONTW lf;
	::GetObjectW(primaryFont_->regular, sizeof(::LOGFONTW), &lf);
	wcscpy(primaryFont_->faceName, lf.lfFaceName);
}

/**
 * Sets the primary font and the association table.
 * @param faceName the typeface name of the primary font. if this is @c null, the primary font
 * will not change
 * @param height the height of the font in logical units. if @a faceName is @c null, this value
 * will be ignored
 * @param associations the association table. script values @c Script#COMMON, @c Script#UNKNOWN,
 * @c Script#INHERITED and @c Script#KATAKANA_OR_HIRAGANA can't set. if this value is @c null,
 * the current associations will not be changed
 * @throw std#invalid_argument @a faceName is @c null, any script of @a associations is invalid,
 * or any typeface name of @a associations is @c null
 * @throw std#length_error the length of @a faceName or any typeface name of @a associations
 * exceeds @c LF_FACESIZE
 */
void FontSelector::setFont(const WCHAR* faceName, int height, const FontAssociations* associations) {
	// check the arguments
	if(faceName != 0 && wcslen(faceName) >= LF_FACESIZE)
		throw length_error("the primary typeface name is too long.");
	else if(associations != 0) {
		for(FontAssociations::const_iterator i = associations->begin(); i != associations->end(); ++i) {
			if(i->first == Script::COMMON || i->first == Script::UNKNOWN
					|| i->first == Script::INHERITED || i->first == Script::KATAKANA_OR_HIRAGANA)
				throw invalid_argument("the association language is invalid.");
			else if(i->second.length() >= LF_FACESIZE)
				throw length_error("the association font name is too long.");
		}
	}
	// reset the association table
	if(associations != 0) {
		for(map<int, Fontset*>::iterator i = associations_.begin(); i != associations_.end(); ++i)
			delete i->second;
		associations_.clear();
		for(FontAssociations::const_iterator i = associations->begin(); i != associations->end(); ++i)
			associations_.insert(make_pair(i->first, new Fontset(i->second.c_str())));
	}
	// reset the primary font
	bool notified = false;
	if(faceName != 0) {
		primaryFont_->clear(faceName);
		for(map<int, Fontset*>::iterator i = associations_.begin(); i != associations_.end(); ++i)
			i->second->clear();
		// create the primary font and reset the text metrics
		resetPrimaryFont(*getDC(), ::CreateFontW(height, 0, 0, 0, FW_REGULAR, false, false, false,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, faceName));
		// reset the fontset for rendering shaping control characters
		if(shapingControlsFont_ != 0) {
			::DeleteObject(shapingControlsFont_);
			shapingControlsFont_ = 0;
		}
		if(linkedFonts_ != 0) {
			linkPrimaryFont();	// this calls fireFontChanged()
			notified = true;
		}
	}
	if(!notified)
		fireFontChanged();
}


// TextRenderer /////////////////////////////////////////////////////////////

#ifndef ASCENSION_NO_DOUBLE_BUFFERING
namespace {
	inline int calculateMemoryBitmapSize(int src) throw() {
		const int UNIT = 32;
		return (src % UNIT != 0) ? src + UNIT - src % UNIT : src;
	}
}
#endif /* !ASCENSION_NO_DOUBLE_BUFFERING */

/**
 * Constructor.
 * @param viewer the text viewer
 */
TextRenderer::TextRenderer(TextViewer& viewer) :
		LineLayoutBuffer(viewer, ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, true),
		longestLineWidth_(0), longestLine_(INVALID_INDEX), numberOfVisualLines_(0) {
	setFont(0, 0, &getDefaultFontAssociations());
	switch(PRIMARYLANGID(getUserDefaultUILanguage())) {
	case LANG_CHINESE:
	case LANG_JAPANESE:
	case LANG_KOREAN:
		enableFontLinking();
		break;
	}
//	updateViewerSize(); ???
	const length_t logicalLines = viewer.getDocument().getNumberOfLines();
	if(logicalLines > 1)
		layoutInserted(1, logicalLines);
	viewer.addDisplaySizeListener(*this);

#if 1
	specialCharacterRenderer_.reset(new DefaultSpecialCharacterRenderer);
	specialCharacterRenderer_->install(*this);
#endif
}

/// Constructor.
TextRenderer::TextRenderer(TextViewer& viewer, const TextRenderer& source) :
		LineLayoutBuffer(viewer, ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, true),
		FontSelector(source), longestLineWidth_(0), longestLine_(INVALID_INDEX), numberOfVisualLines_(0) {
//	updateViewerSize(); ???
	const length_t logicalLines = getTextViewer().getDocument().getNumberOfLines();
	if(logicalLines > 1)
		layoutInserted(1, logicalLines);
	viewer.addDisplaySizeListener(*this);
}

/// Destructor.
TextRenderer::~TextRenderer() throw() {
//	getTextViewer().removeDisplaySizeListener(*this);
}

/// @see FontSelector#fontChanged
void TextRenderer::fontChanged() {
	const TextViewer::Configuration& c = getTextViewer().getConfiguration();
	invalidate();
	visualLinesListeners_.notify(IVisualLinesListener::rendererFontChanged);

#ifndef ASCENSION_NO_DOUBLE_BUFFERING
	if(memoryBitmap_.getHandle() != 0) {
		::BITMAP b;
		memoryBitmap_.getBitmap(b);
		if(b.bmHeight != calculateMemoryBitmapSize(getLinePitch()))
			memoryBitmap_.reset();
	}
#endif /* !ASCENSION_NO_DOUBLE_BUFFERING */
}

/// @see FontSelector#getDC
auto_ptr<DC> TextRenderer::getDC() const {
	return auto_ptr<DC>(getTextViewer().isWindow() ? new ClientDC(const_cast<TextViewer&>(getTextViewer()).getDC()) : new ScreenDC());
}

/**
 * Returns the pitch of each lines (height + space).
 * @see FontSelector#getLineHeight
 */
int TextRenderer::getLinePitch() const throw() {
	return getLineHeight() + getTextViewer().getConfiguration().lineSpacing;
}

/**
 * Returns the width of the rendering area.
 * @see #getWrapWidth
 */
int TextRenderer::getWidth() const throw() {
	if(canvasWidth_ > longestLineWidth_ || getTextViewer().getConfiguration().lineWrap.wrapsAtWindowEdge())
		return canvasWidth_;
	manah::win32::AutoZeroCB<::SCROLLINFO> si;
	si.fMask = SIF_RANGE;
	getTextViewer().getScrollInformation(SB_HORZ, si);
	return (si.nMax + 1) * getAverageCharacterWidth();
}

/**
 * Returns the actual wrap width, or the result of @c std#numeric_limits<int>#max() if wrapping is
 * not occured.
 * @see #getWidth
 */
int TextRenderer::getWrapWidth() const throw() {
	const LineWrapConfiguration& c = getTextViewer().getConfiguration().lineWrap;
	if(c.wrapsAtWindowEdge()) {
		auto_ptr<DC> dc(getDC());
		ISpecialCharacterRenderer::LayoutContext context(*dc);
		context.orientation = getLineTerminatorOrientation(getTextViewer().getConfiguration());
		return canvasWidth_ - ((specialCharacterRenderer_.get() != 0) ? specialCharacterRenderer_->getLineWrappingMarkWidth(context) : 0);
	} else if(c.wraps())
		return c.width;
	else
		return numeric_limits<int>::max();
}

/// @see LineLayoutBuffer#layoutDeleted
void TextRenderer::layoutDeleted(length_t first, length_t last, length_t sublines) throw() {
	numberOfVisualLines_ -= sublines;
	const bool widthChanged = longestLine_ >= first && longestLine_ < last;
	if(widthChanged)
		updateLongestLine(-1, 0);
	visualLinesListeners_.notify<length_t, length_t, length_t>(IVisualLinesListener::visualLinesDeleted, first, last, sublines, widthChanged);
}

/// @see LineLayoutBuffer#layoutInserted
void TextRenderer::layoutInserted(length_t first, length_t last) throw() {
	numberOfVisualLines_ += last - first;
	visualLinesListeners_.notify<length_t, length_t>(IVisualLinesListener::visualLinesInserted, first, last);
}

/// @see LineLayoutBuffer#layoutModified
void TextRenderer::layoutModified(length_t first, length_t last, length_t newSublines, length_t oldSublines, bool documentChanged) throw() {
	numberOfVisualLines_ += newSublines;
	numberOfVisualLines_ -= oldSublines;

	// 最長行の更新
	bool longestLineChanged = false;;
	if(longestLine_ >= first && longestLine_ < last) {
		updateLongestLine(-1, 0);
		longestLineChanged = true;
	} else {
		length_t newLongestLine = longestLine_;
		int newLongestLineWidth = longestLineWidth_;
		for(Iterator i(getFirstCachedLine()), e(getLastCachedLine()); i != e; ++i) {
			const LineLayout& layout = **i;
			if(layout.getLongestSublineWidth() > newLongestLineWidth) {
				newLongestLine = (*i)->getLineNumber();
				newLongestLineWidth = layout.getLongestSublineWidth();
			}
		}
		if(longestLineChanged = (newLongestLine != longestLine_))
			updateLongestLine(newLongestLine, newLongestLineWidth);
	}

	visualLinesListeners_.notify<length_t, length_t, signed_length_t>(
		IVisualLinesListener::visualLinesModified, first, last,
		static_cast<signed_length_t>(newSublines) - static_cast<signed_length_t>(oldSublines), documentChanged, longestLineChanged);

}

/**
 * Offsets visual line.
 * @param[in,out] line the logical line
 * @param[in,out] subline the visual subline
 * @param offset the offset
 */
void TextRenderer::offsetVisualLine(length_t& line, length_t& subline, signed_length_t offset) const throw() {
	if(offset > 0) {
		if(subline + offset < getNumberOfSublinesOfLine(line))
			subline += offset;
		else {
			const length_t lines = getTextViewer().getDocument().getNumberOfLines();
			offset -= static_cast<signed_length_t>(getNumberOfSublinesOfLine(line) - subline) - 1;
			while(offset > 0 && line < lines - 1)
				offset -= static_cast<signed_length_t>(getNumberOfSublinesOfLine(++line));
			subline = getNumberOfSublinesOfLine(line) - 1;
			if(offset < 0)
				subline += offset;
		}
	} else if(offset < 0) {
		if(static_cast<length_t>(-offset) <= subline)
			subline += offset;
		else {
			offset += static_cast<signed_length_t>(subline);
			while(offset < 0 && line > 0)
				offset += static_cast<signed_length_t>(getNumberOfSublinesOfLine(--line));
			subline = (offset > 0) ? offset : 0;
		}
	}
}

/**
 * Renders the specified logical line to the output device. @a selectionColor and @a marginColor
 * must be actual color. Do not use @c presentation#STANDARD_COLOR or any system color using
 * @c presentation#SYSTEM_COLOR_MASK.
 * @param line the line number
 * @param dc the device context
 * @param x the x-coordinate of the position to draw
 * @param y the y-coordinate of the position to draw
 * @param clipRect the clipping region
 * @param selectionColor the color of the selection
 */
void TextRenderer::renderLine(length_t line, PaintDC& dc, int x, int y, const ::RECT& clipRect, const Colors& selectionColor) const throw() {
#ifndef ASCENSION_NO_DOUBLE_BUFFERING
	const LineLayout& layout = getLineLayout(line);
	const int dy = getLinePitch();
	manah::win32::Rect paintRect(dc.getPaintStruct().rcPaint);

	// skip to the subline needs to draw
	const int top = max(paintRect.top, clipRect.top);
	length_t subline = (y + dy >= top) ? 0 : (top - (y + dy)) / dy;
	if(subline >= layout.getNumberOfSublines())
		return;	// this logical line does not need to draw
	y += static_cast<int>(dy * subline);

	TextRenderer& self = *const_cast<TextRenderer*>(this);
	if(memoryDC_.get() == 0)		
		self.memoryDC_ = self.getTextViewer().getDC().createCompatibleDC();
	if(memoryBitmap_.getHandle() == 0)
		self.memoryBitmap_ = Bitmap::createCompatibleBitmap(
			self.getTextViewer().getDC(), calculateMemoryBitmapSize(canvasWidth_), calculateMemoryBitmapSize(dy));
	memoryDC_->selectObject(memoryBitmap_.getHandle());

	const long left = max(paintRect.left, clipRect.left), right = min(paintRect.right, clipRect.right);
	x -= left;
	manah::win32::Rect offsetedClipRect(clipRect);
	paintRect.offset(-left, -y);
	offsetedClipRect.offset(-left, -y);
	for(; subline < layout.getNumberOfSublines() && paintRect.bottom >= 0;
			++subline, y += dy, paintRect.offset(0, -dy), offsetedClipRect.offset(0, -dy)) {
		layout.draw(subline, *memoryDC_, x, 0, paintRect, offsetedClipRect, selectionColor);
		dc.bitBlt(left, y, right - left, dy, memoryDC_->getHandle(), 0, 0, SRCCOPY);
	}
#else
	getLineLayout(line).draw(dc, x, y, dc.getPaintStruct().rcPaint, clipRect, selectionColor);
#endif /* !ASCENSION_NO_DOUBLE_BUFFERING */
}

/**
 * Sets the special character renderer.
 * @param newRenderer the new renderer or @c null
 * @throw std#invalid_argument @a newRenderer is already registered
 */
void TextRenderer::setSpecialCharacterRenderer(ASCENSION_SHARED_POINTER<ISpecialCharacterRenderer> newRenderer) {
	if(newRenderer.get() != 0 && newRenderer.get() == specialCharacterRenderer_.get())
		throw invalid_argument("the specified renderer is already registered.");
	if(specialCharacterRenderer_.get() != 0)
		specialCharacterRenderer_->uninstall();
	specialCharacterRenderer_ = newRenderer;
	newRenderer->install(*this);
	invalidate();
}

/// Returns true if complex scripts are supported.
bool TextRenderer::supportsComplexScripts() throw() {
	return true;
}

/// Returns true if OpenType features are supported.
bool TextRenderer::supportsOpenTypeFeatures() throw() {
	return false;
}

/**
 * Updates the longest line and invokes @c ILongestLineListener#longestLineChanged.
 * @param line the new longest line. set -1 to recalculate
 * @param width the width of the longest line. if @a line is -1, this value is ignored
 */
void TextRenderer::updateLongestLine(length_t line, int width) throw() {
	if(line != -1) {
		longestLine_ = line;
		longestLineWidth_ = width;
	} else {
		longestLine_ = -1;
		longestLineWidth_ = 0;
		for(Iterator i(getFirstCachedLine()), e(getLastCachedLine()); i != e; ++i) {
			if((*i)->getLongestSublineWidth() > longestLineWidth_) {
				longestLine_ = (*i)->getLineNumber();
				longestLineWidth_ = (*i)->getLongestSublineWidth();
			}
		}
	}
}

/// Informs about the change of the viewer's size.
void TextRenderer::viewerDisplaySizeChanged() throw() {
	const TextViewer& viewer = getTextViewer();
	if(!viewer.isWindow())
		return;
	::RECT viewerRect;
	getTextViewer().getClientRect(viewerRect);
	if(viewerRect.right - viewerRect.left > getAverageCharacterWidth()) {
		const ::RECT margins = getTextViewer().getTextAreaMargins();
		const int newWidth = viewerRect.right - viewerRect.left - margins.left - margins.right;
		if(newWidth != canvasWidth_) {
			canvasWidth_ = newWidth;
#ifndef ASCENSION_NO_DOUBLE_BUFFERING
			if(memoryBitmap_.getHandle() != 0) {
				::BITMAP b;
				memoryBitmap_.getBitmap(b);
				if(b.bmWidth != calculateMemoryBitmapSize(canvasWidth_))
					memoryBitmap_.reset();
			}
#endif /* !ASCENSION_NO_DOUBLE_BUFFERING */
			// ウィンドウ幅で折り返す場合は再計算
			if(getTextViewer().getConfiguration().lineWrap.wrapsAtWindowEdge()) {
				for(Iterator i(getFirstCachedLine()), e(getLastCachedLine()); i != e; ) {
					const LineLayout& layout = **i;
					++i;	// invalidate() may break iterator
					if(layout.getNumberOfSublines() != 1
							|| getTextViewer().getConfiguration().justifiesLines || layout.getLongestSublineWidth() > newWidth)
//						layout.rewrap();
						invalidate(layout.getLineNumber(), layout.getLineNumber() + 1);
				}
			}
		}
	}
}


// TextViewer::VerticalRulerDrawer //////////////////////////////////////////

/**
 * Draws the vertical ruler.
 * @param dc the device context
 */
void TextViewer::VerticalRulerDrawer::draw(PaintDC& dc) {
	if(getWidth() == 0)
		return;

	const ::RECT& paintRect = dc.getPaintStruct().rcPaint;
	const Presentation& presentation = viewer_.getPresentation();
	const TextRenderer& renderer = viewer_.getTextRenderer();
	::RECT clientRect;
	viewer_.getClientRect(clientRect);
	if((configuration_.alignment == ALIGN_LEFT && paintRect.left >= clientRect.left + getWidth())
			|| (configuration_.alignment == ALIGN_RIGHT && paintRect.right < clientRect.right - getWidth()))
		return;

#ifdef _DEBUG
	if(DIAGNOSE_INHERENT_DRAWING)
		manah::win32::DumpContext() << L"ruler rect : " << paintRect.top << L" ... " << paintRect.bottom << L"\n";
#endif /* _DEBUG */

	const int savedCookie = dc.save();
	const bool alignLeft = configuration_.alignment == ALIGN_LEFT;
	const int imWidth = configuration_.indicatorMargin.visible ? configuration_.indicatorMargin.width : 0;

#ifndef ASCENSION_NO_DOUBLE_BUFFERING
	if(memoryDC_.get() == 0)
		memoryDC_ = viewer_.getDC().createCompatibleDC();
	if(memoryBitmap_.getHandle() == 0)
		memoryBitmap_ = Bitmap::createCompatibleBitmap(dc,
			getWidth(), clientRect.bottom - clientRect.top + ::GetSystemMetrics(SM_CYHSCROLL));
	memoryDC_->selectObject(memoryBitmap_.getHandle());
	DC& dcex = *memoryDC_;
	const int left = 0;
#else
	DC& dcex = dc;
	const int left = alignLeft ? clientRect.left : clientRect.right - getWidth();
#endif /* !ASCENSION_NO_DOUBLE_BUFFERING */
	const int right = left + getWidth();

	// まず、描画領域全体を描いておく
	if(configuration_.indicatorMargin.visible) {
		// インジケータマージンの境界線と内側
		const int borderX = alignLeft ? left + imWidth - 1 : right - imWidth;
		HPEN oldPen = dcex.selectObject(indicatorMarginPen_.getHandle());
		HBRUSH oldBrush = dcex.selectObject(indicatorMarginBrush_.getHandle());
		dcex.patBlt(alignLeft ? left : borderX + 1, paintRect.top, imWidth, paintRect.bottom - paintRect.top, PATCOPY);
		dcex.moveTo(borderX, paintRect.top);
		dcex.lineTo(borderX, paintRect.bottom);
		dcex.selectObject(oldPen);
		dcex.selectObject(oldBrush);
	}
	if(configuration_.lineNumbers.visible) {
		// background of the line numbers
		HBRUSH oldBrush = dcex.selectObject(lineNumbersBrush_.getHandle());
		dcex.patBlt(alignLeft ? left + imWidth : left, paintRect.top, right - imWidth, paintRect.bottom, PATCOPY);
		// border of the line numbers
		if(configuration_.lineNumbers.borderStyle != VerticalRulerConfiguration::LineNumbers::NONE) {
			HPEN oldPen = dcex.selectObject(lineNumbersPen_.getHandle());
			const int x = (alignLeft ? right : left + 1) - configuration_.lineNumbers.borderWidth;
			dcex.moveTo(x, 0/*paintRect.top*/);
			dcex.lineTo(x, paintRect.bottom);
			dcex.selectObject(oldPen);
		}
		dcex.selectObject(oldBrush);

		// 次の準備...
		dcex.setBkMode(TRANSPARENT);
		dcex.setTextColor(configuration_.lineNumbers.textColor.foreground);
		dcex.setTextCharacterExtra(0);	// 行番号表示は文字間隔の設定を無視
		dcex.selectObject(viewer_.getTextRenderer().getFont());
	}

	// 行番号描画の準備
	Alignment lineNumbersAlignment;
	int lineNumbersX;
	if(configuration_.lineNumbers.visible) {
		lineNumbersAlignment = configuration_.lineNumbers.alignment;
		if(lineNumbersAlignment == ALIGN_AUTO)
			lineNumbersAlignment = alignLeft ? ALIGN_RIGHT : ALIGN_LEFT;
		switch(lineNumbersAlignment) {
		case ALIGN_LEFT:
			lineNumbersX = alignLeft ?
				left + imWidth + configuration_.lineNumbers.leadingMargin : left + configuration_.lineNumbers.trailingMargin + 1;
			dcex.setTextAlign(TA_LEFT | TA_TOP | TA_NOUPDATECP);
			break;
		case ALIGN_RIGHT:
			lineNumbersX = alignLeft ?
				right - configuration_.lineNumbers.trailingMargin - 1 : right - imWidth - configuration_.lineNumbers.leadingMargin;
			dcex.setTextAlign(TA_RIGHT | TA_TOP | TA_NOUPDATECP);
			break;
		case ALIGN_CENTER:	// 中央揃えなんて誰も使わんと思うけど...
			lineNumbersX = alignLeft ?
				left + (imWidth + configuration_.lineNumbers.leadingMargin + getWidth() - configuration_.lineNumbers.trailingMargin) / 2
				: right - (getWidth() - configuration_.lineNumbers.trailingMargin + imWidth + configuration_.lineNumbers.leadingMargin) / 2;
			dcex.setTextAlign(TA_CENTER | TA_TOP | TA_NOUPDATECP);
			break;
		}
	}

	// 1 行ずつ細かい描画
	length_t line, visualSublineOffset;
	const length_t lines = viewer_.getDocument().getNumberOfLines();
	viewer_.mapClientYToLine(paintRect.top, &line, &visualSublineOffset);	// $friendly-access
	if(visualSublineOffset > 0)	// 描画開始は次の論理行から...
		++line;
	int y = viewer_.mapLineToClientY(line, false);
	if(y != 32767 && y != -32768) {
		while(y < paintRect.bottom && line < lines) {
			const LineLayout& layout = renderer.getLineLayout(line);
			const int nextY = y + static_cast<int>(layout.getNumberOfSublines() * renderer.getLinePitch());
			if(nextY >= paintRect.top) {
				// 派生クラスにインジケータマージンの描画機会を与える
				if(configuration_.indicatorMargin.visible) {
					::RECT rect = {
						alignLeft ? left : right - configuration_.indicatorMargin.width,
						y, alignLeft ? left + configuration_.indicatorMargin.width : right,
						y + renderer.getLinePitch()};
					viewer_.drawIndicatorMargin(line, dcex, rect);
				}

				// draw line number digits
				if(configuration_.lineNumbers.visible) {
					wchar_t buffer[32];
					swprintf(buffer, L"%lu", line + configuration_.lineNumbers.startValue);
					UINT option;
					switch(configuration_.lineNumbers.digitSubstitution) {
					case DST_CONTEXTUAL:
					case DST_NOMINAL:		option = ETO_NUMERICSLATIN; break;
					case DST_NATIONAL:		option = ETO_NUMERICSLOCAL; break;
					case DST_USER_DEFAULT:	option = 0; break;
					}
					dcex.extTextOut(lineNumbersX, y, option, 0, buffer, static_cast<int>(wcslen(buffer)), 0);
				}
			}
			++line;
			y = nextY;
		}
	}

#ifndef ASCENSION_NO_DOUBLE_BUFFERING
	dc.bitBlt(alignLeft ? clientRect.left : clientRect.right - getWidth(), paintRect.top,
		right - left, paintRect.bottom - paintRect.top, memoryDC_->getHandle(), 0, paintRect.top, SRCCOPY);
#endif /* !ASCENSION_NO_DOUBLE_BUFFERING */

	dc.restore(savedCookie);
}

/// Recalculates the width of the vertical ruler.
void TextViewer::VerticalRulerDrawer::recalculateWidth() throw() {
	int newWidth = 0;
	if(configuration_.lineNumbers.visible) {
		const uchar newLineNumberDigits = getLineNumberMaxDigits();
		if(newLineNumberDigits != lineNumberDigitsCache_) {
			// the width of the line numbers area is determined by the maximum width of glyphs of 0..9
			ClientDC dc = viewer_.getDC();
			HFONT oldFont = dc.selectObject(viewer_.getTextRenderer().getFont());
			::SCRIPT_STRING_ANALYSIS ssa;
			AutoZero<::SCRIPT_CONTROL> sc;
			AutoZero<::SCRIPT_STATE> ss;
			HRESULT hr;
			switch(configuration_.lineNumbers.digitSubstitution) {
			case DST_CONTEXTUAL:
			case DST_NOMINAL:
				break;
			case DST_NATIONAL:
				ss.fDigitSubstitute = 1;
				break;
			case DST_USER_DEFAULT:
				hr = ::ScriptApplyDigitSubstitution(&userSettings.getDigitSubstitution(), &sc, &ss);
				break;
			}
			dc.setTextCharacterExtra(0);
			hr = ::ScriptStringAnalyse(dc.getHandle(), L"0123456789", 10,
				10 * 3 / 2 + 16, -1, SSA_FALLBACK | SSA_GLYPHS | SSA_LINK, 0, &sc, &ss, 0, 0, 0, &ssa);
			dc.selectObject(oldFont);
			int glyphWidths[10];
			hr = ::ScriptStringGetLogicalWidths(ssa, glyphWidths);
			int maxGlyphWidth = *max_element(glyphWidths, endof(glyphWidths));
			lineNumberDigitsCache_ = newLineNumberDigits;
			if(maxGlyphWidth != 0) {
				newWidth += max<uchar>(newLineNumberDigits, configuration_.lineNumbers.minimumDigits) * maxGlyphWidth;
				newWidth += configuration_.lineNumbers.leadingMargin + configuration_.lineNumbers.trailingMargin;
				if(configuration_.lineNumbers.borderStyle != VerticalRulerConfiguration::LineNumbers::NONE)
					newWidth += configuration_.lineNumbers.borderWidth;
			}
		}
	}
	if(configuration_.indicatorMargin.visible)
		newWidth += configuration_.indicatorMargin.width;
	if(newWidth != width_) {
		width_ = newWidth;
		viewer_.invalidateRect(0, false);
		viewer_.updateCaretPosition();
	}
}
