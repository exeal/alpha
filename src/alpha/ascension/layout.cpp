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
using namespace ascension::presentation;
using namespace ascension::unicode;
using namespace manah::win32::gdi;
using namespace std;

//#define TRACE_LAYOUT_CACHES

#pragma comment(lib, "usp10.lib")

namespace {
	ASCENSION_BEGIN_SHARED_LIB_ENTRIES(USPEntries, 14)
		ASCENSION_SHARED_LIB_ENTRY(0, "ScriptFreeCache", HRESULT(WINAPI *signature)(::SCRIPT_CACHE*))
		ASCENSION_SHARED_LIB_ENTRY(1, "ScriptItemize", HRESULT(WINAPI *signature)(const WCHAR*, int, int, const ::SCRIPT_CONTROL*, const ::SCRIPT_STATE*, ::SCRIPT_ITEM*, int*))
		ASCENSION_SHARED_LIB_ENTRY(2, "ScriptLayout", HRESULT(WINAPI *signature)(int, const ::BYTE*, int*, int*))
		ASCENSION_SHARED_LIB_ENTRY(3, "ScriptShape", HRESULT(WINAPI *signature)(HDC, ::SCRIPT_CACHE*, const WCHAR*, int, int, ::SCRIPT_ANALYSIS*, WORD*, WORD*, ::SCRIPT_VISATTR*, int*))
		ASCENSION_SHARED_LIB_ENTRY(4, "ScriptPlace", HRESULT(WINAPI *signature)(HDC, ::SCRIPT_CACHE*, const WORD*, int, const ::SCRIPT_VISATTR*, ::SCRIPT_ANALYSIS*, int*, ::GOFFSET*, ::ABC*))
		ASCENSION_SHARED_LIB_ENTRY(5, "ScriptTextOut", HRESULT(WINAPI *signature)(const HDC, ::SCRIPT_CACHE*, int, int, UINT, const ::RECT*, const ::SCRIPT_ANALYSIS*, const WCHAR*, int, const WORD*, int, const int*, const int*, const ::GOFFSET*))
		ASCENSION_SHARED_LIB_ENTRY(6, "ScriptBreak", HRESULT(WINAPI *signature)(const WCHAR*, int, const ::SCRIPT_ANALYSIS*, ::SCRIPT_LOGATTR*))
		ASCENSION_SHARED_LIB_ENTRY(7, "ScriptCPtoX", HRESULT(WINAPI *signature)(int, BOOL, int, int, const WORD*, const ::SCRIPT_VISATTR*, const int*, const ::SCRIPT_ANALYSIS*, int*))
		ASCENSION_SHARED_LIB_ENTRY(8, "ScriptXtoCP", HRESULT(WINAPI *signature)(int, int, int, const WORD*, const ::SCRIPT_VISATTR*, const int*, const ::SCRIPT_ANALYSIS*, int*, int*))
		ASCENSION_SHARED_LIB_ENTRY(9, "ScriptGetLogicalWidths", HRESULT(WINAPI *signature)(const ::SCRIPT_ANALYSIS*, int, int, const int*, const WORD*, const ::SCRIPT_VISATTR*, int*))
		ASCENSION_SHARED_LIB_ENTRY(10, "ScriptGetProperties", HRESULT(WINAPI *signature)(const ::SCRIPT_PROPERTIES***, int*))
		ASCENSION_SHARED_LIB_ENTRY(11, "ScriptGetFontProperties", HRESULT(WINAPI *signature)(HDC, ::SCRIPT_CACHE*, ::SCRIPT_FONTPROPERTIES*))
		ASCENSION_SHARED_LIB_ENTRY(12, "ScriptRecordDigitSubstitution", HRESULT(WINAPI *signature)(::LCID, ::SCRIPT_DIGITSUBSTITUTE*))
		ASCENSION_SHARED_LIB_ENTRY(13, "ScriptApplyDigitSubstitution", HRESULT(WINAPI *signature)(const ::SCRIPT_DIGITSUBSTITUTE*, ::SCRIPT_CONTROL*, SCRIPT_STATE*))
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
} // namespace @0

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

struct LineLayout::Run : public StyledText {
	::SCRIPT_ANALYSIS analysis;
	::SCRIPT_CACHE cache;
	HFONT font;			// ランを描画するフォント
	WORD* glyphs;
	length_t length;	// ランの文字数
	int numberOfGlyphs;	// glyphs の要素数
	WORD* clusters;
	::SCRIPT_VISATTR* visualAttributes;
	int* advances;
	::GOFFSET* glyphOffsets;
	::ABC width;
	Run(const TextStyle& textStyle) throw() :
			cache(0), font(0), glyphs(0), clusters(0), visualAttributes(0), advances(0), glyphOffsets(0) {
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
		delete[] glyphOffsets; glyphOffsets = 0;
	}
	HRESULT getLogicalWidths(int widths[]) const throw() {
		return ::ScriptGetLogicalWidths(&analysis,
			static_cast<int>(length), static_cast<int>(numberOfGlyphs), advances, clusters, visualAttributes, widths);
	}
	int getWidth() const throw() {return width.abcA + width.abcB + width.abcC;}
	HRESULT getX(size_t offset, bool trailing, int& x) const throw() {
		return ::ScriptCPtoX(static_cast<int>(offset), trailing,
			static_cast<int>(length), numberOfGlyphs, clusters, visualAttributes, advances, &analysis, &x);
	}
	bool overhangs() const throw() {return width.abcA < 0 || width.abcC < 0;}
};

// helpers for LineLayout::draw
namespace {
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
	inline void drawUnderline(PaintDC& dc, int x, int y, int width, COLORREF color, UnderlineStyle style) {
		if(style == NO_UNDERLINE)
			return;

		// get underline metrics of TrueType font
		const UINT c = dc.getOutlineTextMetrics(0, 0);
		if(c == 0)
			return;
		::OUTLINETEXTMETRIC* otm = reinterpret_cast<::OUTLINETEXTMETRIC*>(new char[c]);
		dc.getOutlineTextMetrics(c, otm);

		// draw
		HPEN oldPen = dc.selectObject(createPen(color, otm->otmsUnderscoreSize, style));
		y += otm->otmTextMetrics.tmAscent - otm->otmsUnderscorePosition + otm->otmsUnderscoreSize / 2;
		dc.moveTo(x, y);
		dc.lineTo(x + width, y);
		::DeleteObject(dc.selectObject(oldPen));
	}
	inline void drawBorder(PaintDC& dc, int x, int y, int width, int height, COLORREF color, BorderStyle style) {
		if(style == NO_BORDER)
			return;
		HPEN oldPen = dc.selectObject(createPen(color, 1, style));
		HBRUSH oldBrush = dc.selectObject(static_cast<HBRUSH>(::GetStockObject(NULL_BRUSH)));
		dc.rectangle(x, y, x + width, y + height);
		::DeleteObject(dc.selectObject(oldPen));
		dc.selectObject(oldBrush);
	}
} // namespace @0

/**
 * Private constructor.
 * @param textRenderer the text renderer
 * @param line the line
 * @throw text#BadPositionException @a line is invalid
 */
LineLayout::LineLayout(const TextRenderer& textRenderer, length_t line) :
		renderer_(textRenderer), lineNumber_(line),
		runs_(0), numberOfRuns_(0), sublineOffsets_(0), sublineFirstRuns_(0), numberOfSublines_(0), width_(-1) {
	if(!getText().empty()) {
		itemize(line);
		if(!shape())
			dispose();
		if(numberOfRuns_ == 0 || !renderer_.getTextViewer().getConfiguration().lineWrap.wraps()) {
			numberOfSublines_ = 1;
			sublineFirstRuns_ = new size_t[1];
			sublineFirstRuns_[0] = 0;
			reorder();
			expandTabsWithoutWrapping();
		} else {
			wrap();
			reorder();
		}
	} else {	// 空行の場合
		numberOfRuns_ = 0;
		numberOfSublines_ = 1;
		width_ = 0;
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
 * Builds glyphs into the run structure.
 * @param dc the device context
 * @param line the line text
 * @param run the run to shape
 * @param[in,out] expectedNumberOfGlyphs the length of @a run.glyphs
 * @return the result of @c ScriptShape call
 */
inline HRESULT LineLayout::buildGlyphs(HDC dc, const wchar_t* line, Run& run, size_t& expectedNumberOfGlyphs) {
	while(true) {
		// グリフを格納するのに十分な run.glyphs が得られるまで繰り返す
		HRESULT hr = ::ScriptShape(dc, &run.cache, line + run.column, static_cast<int>(run.length),
			static_cast<int>(expectedNumberOfGlyphs), &run.analysis, run.glyphs, run.clusters,
			run.visualAttributes, &run.numberOfGlyphs);
		if(hr != E_OUTOFMEMORY)
			return hr;
		delete[] run.glyphs;
		delete[] run.visualAttributes;
		expectedNumberOfGlyphs *= 2;
		run.glyphs = new WORD[expectedNumberOfGlyphs];
		run.visualAttributes = new ::SCRIPT_VISATTR[expectedNumberOfGlyphs];
	}
}

/**
 * Draws the layout to the output device.
 * @a selectionColor and @a marginColor must be actual color.
 * do not use @c presentation#STANDARD_COLOR or any system color using @c presentation#SYSTEM_COLOR_MASK
 * @param dc the device context
 * @param x the x-coordinate of the position to draw
 * @param y the y-coordinate of the position to draw
 * @param clipRect the clipping region
 * @param selectionColor the color of the selection
 */
void LineLayout::draw(PaintDC& dc, int x, int y, const ::RECT& clipRect, const Colors& selectionColor) const throw() {
	// TODO: call ISpecialCharacterDrawer.

	// クリッピングによるマスキングを利用した選択テキストの描画は以下の記事を参照
	// Catch 22 : Design and Implementation of a Win32 Text Editor
	// Part 10 - Transparent Text and Selection Highlighting (http://www.catch22.net/tuts/editor10.asp)

	const ::RECT& paintRect = dc.getPaintStruct().rcPaint;
	const int linePitch = renderer_.getLinePitch();
	const int lineHeight = renderer_.getLineHeight();
	const Colors lineColor = renderer_.getTextViewer().getPresentation().getLineColor(lineNumber_);
	const COLORREF marginColor = internal::systemColors.getReal((lineColor.background == STANDARD_COLOR) ?
		renderer_.getTextViewer().getConfiguration().color.background : lineColor.background, SYSTEM_COLOR_MASK | COLOR_WINDOW);
	ISpecialCharacterDrawer::Context context(dc);
	ISpecialCharacterDrawer* specialCharacterDrawer = renderer_.getSpecialCharacterDrawer();

	// 空行などの場合
	if(isDisposed()) {
		::RECT r;
		r.left = max(paintRect.left, clipRect.left);
		r.top = max(clipRect.top, max<long>(paintRect.top, y));
		r.right = min(paintRect.right, clipRect.right);
		r.bottom = min(clipRect.bottom, min<long>(paintRect.bottom, y + linePitch));
		dc.fillSolidRect(r, marginColor);
		return;
	}

	// 描画が必要な折り返し行までスキップ
	length_t subline = (y + linePitch >= paintRect.top) ? 0 : (paintRect.top - (y + linePitch)) / linePitch;
	if(subline >= numberOfSublines_)
		return;	// 描画の必要なし
	y += static_cast<int>(linePitch * subline);

	const int originalX = x;
	const int savedCookie = dc.save();
	HRESULT hr;
	dc.setTextAlign(TA_BASELINE | TA_LEFT | TA_NOUPDATECP);

	// 折り返し行ごとのループ
	for(; subline < numberOfSublines_; ++subline) {
		length_t selStart, selEnd;
		const bool sel = renderer_.getTextViewer().getCaret().getSelectedRangeOnVisualLine(lineNumber_, subline, selStart, selEnd);

		// 行間を塗る
		Rgn clipRegion;
		clipRegion.createRectRgn(clipRect.left, max<long>(y, clipRect.top), clipRect.right, min<long>(y + linePitch, clipRect.bottom));
		dc.selectClipRgn(clipRegion);
		if(linePitch - lineHeight > 0)
			dc.fillSolidRect(paintRect.left, y + renderer_.getLineHeight(),
				paintRect.right - paintRect.left, linePitch - lineHeight, marginColor);

		// 背景を先に塗って、描画が必要なランを調べて、選択領域をマスキングする
		size_t firstRun = sublineFirstRuns_[subline];
		size_t lastRun = (subline < numberOfSublines_ - 1) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;
		x = originalX + getSublineIndent(subline);
		// 左余白を塗る
		if(x > originalX && x > paintRect.left) {
			const int left = max<int>(originalX, paintRect.left);
			dc.fillSolidRect(left, y, x - left, lineHeight, marginColor);
		}
		// ランの背景を塗る
		int startX = x;
		for(size_t i = firstRun; i < lastRun; ++i) {
			Run& run = *runs_[i];
			if(x + static_cast<int>(run.width.abcB) + run.width.abcA < paintRect.left) {	// 描画不要
				++firstRun;
				startX = x + run.getWidth();
			} else {
				const COLORREF bgColor = (lineColor.background == STANDARD_COLOR) ?
					internal::systemColors.getReal(run.style.color.background, SYSTEM_COLOR_MASK | COLOR_WINDOW) : marginColor;
				if(!sel || run.column >= selEnd || run.column + run.length <= selStart)	// 選択がランに突入していない
					dc.fillSolidRect(x, y, run.getWidth(), lineHeight, bgColor);
				else if(sel && run.column >= selStart && run.column + run.length <= selEnd) {	// ランが丸ごと選択されている
					dc.fillSolidRect(x, y, run.getWidth(), lineHeight, selectionColor.background);
					dc.excludeClipRect(x, y, x + run.getWidth(), y + lineHeight);
				} else {	// 部分的に選択されている
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
		// 右余白を塗る
		if(x < paintRect.right)
			dc.fillSolidRect(x, y, paintRect.right - x, linePitch, marginColor);

		// 選択範囲外のテキストを描画
		x = startX;
		dc.setBkMode(TRANSPARENT);
		for(size_t i = firstRun; i < lastRun; ++i) {
			Run& run = *runs_[i];
			if(getText()[run.column] != L'\t') {
				if(!sel || run.overhangs() || !(run.column >= selStart && run.column + run.length <= selEnd)) {
					dc.selectObject(run.font);
					dc.setTextColor(internal::systemColors.getReal((lineColor.foreground == STANDARD_COLOR) ?
						run.style.color.foreground : lineColor.foreground, COLOR_WINDOWTEXT | SYSTEM_COLOR_MASK));
					hr = ::ScriptTextOut(dc.get(), &run.cache, x, y + renderer_.getAscent(), 0, 0,
						&run.analysis, 0, 0, run.glyphs, run.numberOfGlyphs, run.advances, 0, run.glyphOffsets);
				}
			}
			x += run.getWidth();
		}

		// 選択範囲内のテキストを描画 (下線と境界線もついでに)
		x = startX;
		clipRegion.setRectRgn(clipRect);
		dc.selectClipRgn(clipRegion, RGN_XOR);
		for(size_t i = firstRun; i < lastRun; ++i) {
			Run& run = *runs_[i];
			if(sel && getText()[run.column] != L'\t'
					&& (run.overhangs() || (run.column < selEnd && run.column + run.length > selStart))) {
				dc.selectObject(run.font);
				dc.setTextColor(selectionColor.foreground);
				hr = ::ScriptTextOut(dc.get(), &run.cache, x, y + renderer_.getAscent(), 0, 0,
					&run.analysis, 0, 0, run.glyphs, run.numberOfGlyphs, run.advances, 0, run.glyphOffsets);
			}
			drawUnderline(dc, x, y, run.getWidth(), run.style.underlineColor, run.style.underlineStyle);
			drawBorder(dc, x, y, run.getWidth(), linePitch, run.style.borderColor, run.style.borderStyle);
			x += run.getWidth();
		}

		// 次の折り返し行へ進む
		if((y += linePitch) >= paintRect.bottom)
			break;
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
	bool rtl;
	switch(renderer_.getTextViewer().getConfiguration().alignment) {
	case ALIGN_LEFT:	rtl = false; break;
	case ALIGN_RIGHT:	rtl = true; break;
	default:			rtl = renderer_.getTextViewer().getConfiguration().orientation == RIGHT_TO_LEFT;
	}
	const String& text = getText();
	int x = 0;
	Run* run;
	if(!rtl) {	// 左端からタブを展開する
		for(size_t i = 0; i < numberOfRuns_; ++i) {
			run = runs_[i];
			if(run->length == 1 && text[run->column] == L'\t') {
				run->advances[0] = getNextTabStop(x, FORWARD) - x;
				run->width.abcB = run->advances[0];
				run->width.abcA = run->width.abcC = 0;
			}
			x += run->getWidth();
		}
	} else {	// 右端からタブを展開する
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
	width_ = x;
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
	int cx = getWidth();
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
	if(numberOfRuns_ == 0 && column == 0)	// 既定のレベルを使う
		return (renderer_.getTextViewer().getConfiguration().orientation == RIGHT_TO_LEFT) ? 1 : 0;
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
	s.cx = getWidth();
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
	::RECT bounds;	// 結果
	int cx, x;

	// first について
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

	// last について
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
		const length_t lastRun = (subline + 1 < numberOfSublines_) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;
		location.x = 0;
		for(size_t i = sublineFirstRuns_[subline]; i < lastRun; ++i) {
			const Run& run = *runs_[i];
			if(run.column > column || run.column + run.length <= column) {
				location.x += run.getWidth();
				continue;
			}
			int offset;
			run.getX(column - run.column, edge == TRAILING, offset);
			location.x += offset;
			break;
		}
		location.x += getSublineIndent(subline);
		location.y = static_cast<long>(subline * renderer_.getLinePitch());
	}
	return location;
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
	bool rtl;
	switch(c.alignment) {
	case ALIGN_LEFT:	rtl = false; break;
	case ALIGN_RIGHT:	rtl = true; break;
	default:			rtl = c.orientation == RIGHT_TO_LEFT; break;
	}
	if(!rtl)
		return getNextTabStop(x, right ? FORWARD : BACKWARD);
	else
		return right ? x + (x - getWidth()) % tabWidth : x - (tabWidth - (x - getWidth()) % tabWidth);
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
			::ScriptXtoCP(x - cx, static_cast<int>(run.length),
				static_cast<int>(run.numberOfGlyphs), run.clusters, run.visualAttributes, run.advances, &run.analysis, &cp, &t);
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
	const int width = c.lineWrap.wraps() ? renderer_.getWrapWidth() : renderer_.getWidth();
	switch(c.alignment) {
	case ALIGN_LEFT:	return 0;
	case ALIGN_RIGHT:	return width - getSublineWidth(subline);
	case ALIGN_CENTER:	return (width - getSublineWidth(subline)) / 2;
//	case JUSTIFY:		return 0;
	default:			assert(false);
	}
	return 0;	// 無意味
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
	if(runs_ != 0) {
		const size_t lastRun = (subline + 1 < numberOfSublines_) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;
		int cx = 0;
		for(size_t i = sublineFirstRuns_[subline]; i < lastRun; ++i)
			cx += runs_[i]->getWidth();
		return cx;
	} else
		return 0;
}

/// Returns the text of the line.
inline const String& LineLayout::getText() const throw() {
	return renderer_.getTextViewer().getDocument().getLine(lineNumber_);
}

/// Returns the width of the line (maximum width of sublines).
int LineLayout::getWidth() const throw() {
	if(width_ == -1) {
		int& width = const_cast<LineLayout*>(this)->width_;
		width = 0;
		for(length_t subline = 0; subline < numberOfSublines_; ++subline)
			width = max<long>(getSublineWidth(subline), width);
	}
	return width_;
}

/// Returns if the line contains right-to-left run.
bool LineLayout::isBidirectional() const throw() {
	if(renderer_.getTextViewer().getConfiguration().orientation == RIGHT_TO_LEFT)
		return true;
	for(size_t i = 0; i < numberOfRuns_; ++i) {
		if(runs_[i]->analysis.s.uBidiLevel % 2 == 1)
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
	::SCRIPT_ITEM* items = new ::SCRIPT_ITEM[text.length() + 1];
	int numberOfItems;
	manah::win32::AutoZero<::SCRIPT_CONTROL> control;
	manah::win32::AutoZero<::SCRIPT_STATE> initialState;
	initialState.uBidiLevel = (c.orientation == RIGHT_TO_LEFT) ? 1 : 0;
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
	hr = ::ScriptItemize(text.data(), static_cast<int>(text.length()),
		static_cast<int>(text.length() + 1), &control, &initialState, items, &numberOfItems);
	if(c.disablesDeprecatedFormatCharacters) {
		for(int i = 0; i < numberOfItems; ++i) {
			items[i].a.s.fInhibitSymSwap = initialState.fInhibitSymSwap;
			items[i].a.s.fDigitSubstitute = initialState.fDigitSubstitute;
		}
	}

	bool mustDelete;
	const LineStyle& styles = presentation.getLineStyle(lineNumber, mustDelete);
	if(&styles != &LineStyle::NULL_STYLE) {
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
				&& !iswspace(text[styles.array[styleIndex - 1].column])
				&& !iswspace(text[styles.array[styleIndex - 1].column - 1]))
			runs[runs.size() - 1]->analysis.fLinkAfter = run->analysis.fLinkBefore = 1;
		runs.push_back(run);
		runs[runs.size() - 2]->length = run->column - runs[runs.size() - 2]->column;
	}
	run->length = text.length() - run->column;
	runs_ = new Run*[numberOfRuns_ = runs.size()];
	copy(runs.begin(), runs.end(), runs_);
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

/**
 * Shapes all runs.
 * @return succeeded or not
 */
inline bool LineLayout::shape() throw() {
	for(size_t i = 0; i < numberOfRuns_; ++i) {
		if(!shape(*runs_[i])) {
			dispose();
			return false;
		}
	}
	return true;
}

/**
 * Shapes the text.
 * @param run the run to be performed
 * @return succeeded or not
 */
bool LineLayout::shape(Run& run) throw() {
	// TODO: call ISpecialCharacterDrawer.

	assert(run.glyphs == 0);
	HRESULT hr;
	const String& line = getText();
	ClientDC dc = const_cast<TextRenderer&>(renderer_).getTextViewer().getDC();
	run.clusters = new WORD[run.length];
	if(renderer_.getTextViewer().getConfiguration().inhibitsShaping)
		run.analysis.eScript = SCRIPT_UNDEFINED;

	// フォントを選択する
	if(run.analysis.s.fDisplayZWG != 0 && scriptProperties.get(run.analysis.eScript).fControl != 0)
		run.font = renderer_.getFontForShapingControls();
	else
		run.font = renderer_.getFont(Script::COMMON, run.style.bold, run.style.italic);
	HFONT oldFont = dc.selectObject(run.font);

	// 1 回目のシェーピング
	size_t expectedNumberOfGlyphs = run.length * 3 / 2 + 16;
	run.glyphs = new WORD[expectedNumberOfGlyphs];
	run.visualAttributes = new ::SCRIPT_VISATTR[expectedNumberOfGlyphs];
	hr = buildGlyphs(dc.get(), line.data(), run, expectedNumberOfGlyphs);

	// 既定のグリフが返った場合は欠損 (判定が厳しすぎると思うが...)
	if(SUCCEEDED(hr)) {
		::SCRIPT_FONTPROPERTIES fp;
		fp.cBytes = sizeof(::SCRIPT_FONTPROPERTIES);
		if(SUCCEEDED(ScriptGetFontProperties(dc.get(), &run.cache, &fp))
				&& find(run.glyphs, run.glyphs + run.numberOfGlyphs, fp.wgDefault) != run.glyphs + run.numberOfGlyphs)
			hr = USP_E_SCRIPT_NOT_IN_FONT;
	}

	// 欠損グリフがある場合はフォールバックする
	if(hr == USP_E_SCRIPT_NOT_IN_FONT) {
		::ScriptFreeCache(&run.cache);
		// TODO: tries about the font linking.
		// ランの書記体系を調べる
		int script;
		for(StringCharacterIterator i(line.data() + run.column, line.data() + run.column + run.length); !i.isLast(); i.next()) {
			script = Script::of(i.current());
			if(script != Script::UNKNOWN && script != Script::COMMON && script != Script::INHERITED)
				break;
		}
		HFONT fallbackFont;
		if(script != Script::UNKNOWN && script != Script::COMMON && script != Script::INHERITED)
			fallbackFont = renderer_.getFont(script, run.style.bold, run.style.italic);
		else if(runs_[0] != &run)
			fallbackFont = find(runs_, runs_ + numberOfRuns_, &run)[-1]->font;
		else
			fallbackFont = run.font;	// うーむ
		if(fallbackFont != run.font) {
			dc.selectObject(run.font = fallbackFont);
			hr = buildGlyphs(dc.get(), line.data(), run, expectedNumberOfGlyphs);
		}
		if(FAILED(hr)) {	// シェーピング無し
			run.analysis.eScript = SCRIPT_UNDEFINED;
			hr = buildGlyphs(dc.get(), line.data(), run, expectedNumberOfGlyphs);
		}
		if(FAILED(hr))
			return false;
	}
	run.advances = new int[run.numberOfGlyphs];
	run.glyphOffsets = new ::GOFFSET[run.numberOfGlyphs];
	hr = ::ScriptPlace(dc.get(), &run.cache, run.glyphs, run.numberOfGlyphs,
			run.visualAttributes, &run.analysis, run.advances, run.glyphOffsets, &run.width);
	dc.selectObject(oldFont);
	return true;
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
	// ランごとのループ (この時点では runs_ は論理順になっている)
	for(size_t i = 0; i < numberOfRuns_; ++i) {
		Run& run = *runs_[i];

		// タブの場合は展開して幅を算出する
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

		// ランの全文字について論理幅と論理属性を求め、折り返し位置を決める
		int* logicalWidths = new int[run.length];
		::SCRIPT_LOGATTR* logicalAttributes = new ::SCRIPT_LOGATTR[run.length];
		dc.selectObject(run.font);
		HRESULT hr = run.getLogicalWidths(logicalWidths);
		hr = ::ScriptBreak(text.data() + run.column, static_cast<int>(run.length), &run.analysis, logicalAttributes);
		const length_t originalRunPosition = run.column;
		int widthInThisRun = 0;
		length_t lastBreakable = run.column, lastGlyphEnd = run.column;
		int lastBreakableCx = cx, lastGlyphEndCx = cx;
		// ラン内の文字ごとのループ
		for(length_t j = run.column; j < run.column + run.length; ) {	// j は論理行頭からの位置
			const int x = cx + widthInThisRun;
			// この位置で折り返し可能か覚えておく
			if(logicalAttributes[j - originalRunPosition].fCharStop != 0) {
				lastGlyphEnd = j;
				lastGlyphEndCx = x;
//				if(logicalAttributes[j - originalRunPosition].fSoftBreak != 0 || logicalAttributes[j - originalRunPosition].fWhiteSpace != 0) {
					lastBreakable = j;
					lastBreakableCx = x;
//				}
			}
			// 物理行の幅が折り返し幅を超えたら折り返す
			if(x + logicalWidths[j - originalRunPosition] > wrapWidth) {
				// 折り返し可能な位置がランの先頭の場合
				if(lastBreakable == run.column) {
					// 折り返し可能な位置が無い場合は最後のグリフ境界で折り返す
					if(sublineFirstRuns.empty() || sublineFirstRuns.back() == newRuns.size()) {
						if(lastGlyphEnd == run.column) {	// グリフ境界も見つからない場合はここで折り返す
							lastBreakable = j;
							lastBreakableCx = x;
						} else {
							lastBreakable = lastGlyphEnd;
							lastBreakableCx = lastGlyphEndCx;
						}
					}
				}

				// ランの先頭で折り返す場合
				if(lastBreakable == run.column) {
					assert(sublineFirstRuns.empty() || newRuns.size() != sublineFirstRuns.back());
					sublineFirstRuns.push_back(newRuns.size());
//dout << L"broke the line at " << lastBreakable << L" where the run start.\n";
				}
				// ランの終端で折り返す場合
				else if(lastBreakable == run.column + run.length) {
					if(lastBreakable < text.length()) {
						assert(sublineFirstRuns.empty() || newRuns.size() != sublineFirstRuns.back());
						sublineFirstRuns.push_back(newRuns.size() + 1);
//dout << L"broke the line at " << lastBreakable << L" where the run end.\n";
					}
					break;
				}
				// ランの途中で折り返す場合はランを分割する (run -> newRun + run)
				else {
					run.dispose();
					auto_ptr<Run> newRun(new Run(run));
					run.column = lastBreakable;
					newRun->length = run.column - newRun->column;
					assert(newRun->length != 0);
					run.length -= newRun->length;
					assert(run.length != 0);
//					newRun->analysis.fLinkAfter = run.analysis.fLinkBefore = 0;	// グリフの結合を切る
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

LineLayout::StyledSegmentIterator::reference LineLayout::StyledSegmentIterator::dereference() const throw() {
	return **p_;
}


// LineLayoutBuffer /////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param viewer the text viewer
 * @param bufferSize the size of the buffer for caches in lines
 * @param autoRepair true to repair disposed layout automatically if the line number of its line was not changed
 * @throw std#invalid_argument @a bufferSize is zero
 */
LineLayoutBuffer::LineLayoutBuffer(TextViewer& viewer, length_t bufferSize, bool autoRepair) :
		viewer_(viewer), layouts_(new LineLayout*[bufferSize]), bufferSize_(bufferSize),
		startLine_(0), autoRepair_(autoRepair), documentChangePhase_(NONE) {
	pendingCacheClearance_.first = pendingCacheClearance_.last = INVALID_INDEX;
	if(bufferSize == 0) {
		delete[] layouts_;
		throw invalid_argument("size of the buffer can't be zero.");
	}
	fill(layouts_, layouts_ + bufferSize_, static_cast<LineLayout*>(0));
	viewer.getDocument().addPrenotifiedListener(*this);
}

/// Destructor.
LineLayoutBuffer::~LineLayoutBuffer() throw() {
//	clearCaches(startLine_, startLine_ + bufferSize_, false);
	for(size_t i = 0; i < bufferSize_; ++i)
		delete layouts_[i];
	delete[] layouts_;
	viewer_.getDocument().removePrenotifiedListener(*this);
}

/**
 * Clears the layout caches of the specified lines.
 * This method calls @c #layoutModified.
 * @param first the start of lines
 * @param last the end of lines (exclusive. this line will not be cleared)
 * @param repair true to recreate layouts for the lines
 */
void LineLayoutBuffer::clearCaches(length_t first, length_t last, bool repair) {
	if(documentChangePhase_ == ABOUT_CHANGE) {
		pendingCacheClearance_.first = (pendingCacheClearance_.first == INVALID_INDEX) ? first : min(first, pendingCacheClearance_.first);
		pendingCacheClearance_.last = (pendingCacheClearance_.last == INVALID_INDEX) ? last : max(last, pendingCacheClearance_.last);
		return;
	}
	first = min(max(first, startLine_), startLine_ + bufferSize_);
	last = min(max(last, startLine_), startLine_ + bufferSize_);
	if(first == last)
		return;
	length_t oldSublines = 0, newSublines = 0;
	for(length_t i = first; i < last; ++i) {
		LineLayout*& layout = layouts_[i - startLine_];
		oldSublines += (layout != 0) ? layout->getNumberOfSublines() : 1;
		delete layout;
		layout = (repair && layout != 0) ? new LineLayout(viewer_.getTextRenderer(), i) : 0;
		newSublines += (layout != 0) ? layout->getNumberOfSublines() : 1;
	}
//	last = min(last, viewer_.getDocument().getNumberOfLines());
	layoutModified(first, last, newSublines, oldSublines, documentChangePhase_ == CHANGING);
}

/// @see text#IDocumentListener#documentAboutToBeChanged
void LineLayoutBuffer::documentAboutToBeChanged(const text::Document&) {
	documentChangePhase_ = ABOUT_CHANGE;
}

/// @see text#IDocumentListener#documentChanged
void LineLayoutBuffer::documentChanged(const text::Document&, const text::DocumentChange& change) {
	const length_t top = change.getRegion().getTop().line, bottom = change.getRegion().getBottom().line;
	documentChangePhase_ = CHANGING;
	invalidateLineLayout(top);
	if(top != bottom) {
		if(change.isDeletion()) {	// 改行を含む範囲の削除
			// 削除される行の表示行数を数える (キャッシュ窓の内部と前後)
			length_t sublines = 0;
			if(top + 1 < startLine_)
				sublines += min(startLine_, bottom + 1) - (top + 1);
			for(length_t i = max(top + 1, startLine_); i < min(bottom + 1, startLine_ + bufferSize_); ++i)
				sublines += (layouts_[i - startLine_] != 0) ? layouts_[i - startLine_]->getNumberOfSublines() : 1;
			if(bottom > startLine_ + bufferSize_ - 1)
				sublines += bottom - (startLine_ + bufferSize_ - 1);
			slideCaches(top + 1, static_cast<signed_length_t>(top) - static_cast<signed_length_t>(bottom), false);
			layoutDeleted(top + 1, bottom + 1, sublines);
		} else {	// 改行を含む範囲の挿入
			slideCaches(top + 1, static_cast<signed_length_t>(bottom - top), true);
			layoutInserted(top + 1, bottom + 1);
		}
	}
	verifyLayoutCaches();
	documentChangePhase_ = NONE;
	if(pendingCacheClearance_.first != INVALID_INDEX) {
		clearCaches(pendingCacheClearance_.first, pendingCacheClearance_.last, autoRepair_);
		pendingCacheClearance_.first = pendingCacheClearance_.last = INVALID_INDEX;
	}
}

/// Returns the last line of the cached layouts.
length_t LineLayoutBuffer::getCacheLastLine() const throw() {
	return min(startLine_ + bufferSize_, viewer_.getDocument().getNumberOfLines());
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
	verifyLayoutCaches();
	LineLayoutBuffer& self = *const_cast<LineLayoutBuffer*>(this);
	if(line > viewer_.getDocument().getNumberOfLines())
		throw text::BadPositionException();
	else if(line < startLine_) {
#ifdef TRACE_LAYOUT_CACHES
		dout << "... cache not found\n";
#endif
		self.updateCacheStartLine(line);
	} else if(line >= startLine_ + bufferSize_) {
#ifdef TRACE_LAYOUT_CACHES
		dout << "... cache not found\n";
#endif
		self.updateCacheStartLine(line - bufferSize_ + 1);
	} else {
#ifdef TRACE_LAYOUT_CACHES
		dout << "... cache found\n";
#endif
	}
	if(layouts_[line - startLine_] == 0) {
		layouts_[line - startLine_] = new LineLayout(viewer_.getTextRenderer(), line);
		self.layoutModified(line, line + 1, layouts_[line - startLine_]->getNumberOfSublines(), 1, documentChangePhase_ == CHANGING);
	}
	verifyLayoutCaches();
#ifdef TRACE_LAYOUT_CACHES
		dout << "  ok. line " << line << " was returned.\n";
#endif
	return *layouts_[line - startLine_];
}

/// Invalidates all layouts.
void LineLayoutBuffer::invalidate() {
	clearCaches(startLine_, startLine_ + bufferSize_, autoRepair_);
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
 * Resets the layout cache of the specified line, and repairs if necessary.
 * @param line the logical line number
 */
inline void LineLayoutBuffer::invalidateLineLayout(length_t line) throw() {
	if(line >= startLine_ && line < startLine_ + bufferSize_) {
		if(LineLayout*& layout = layouts_[line - startLine_]) {
			const length_t oldSublines = layout->getNumberOfSublines();
			delete layout;
			if(autoRepair_) {
				layout = new LineLayout(viewer_.getTextRenderer(), line);
				layoutModified(line, line + 1, layout->getNumberOfSublines(), oldSublines, documentChangePhase_ == CHANGING);
			} else {
				layout = 0;
				layoutModified(line, line + 1, 1, oldSublines, documentChangePhase_ == CHANGING);
			}
		}
	}
}

/**
 * @fn void LineLayoutBuffer#layoutDeleted(length_t first, length_t last, length_t sublines)
 * The layouts of lines were deleted.
 * @param first the start of lines to be deleted
 * @param last the end of lines (exclusive) to be deleted
 * @param sublines the number of sublines of the deleted lines
 */

/**
 * @fn void LineLayoutBuffer#layoutInserted(length_t first, length_t last)
 * The new layouts of lines were inserted.
 * @param first the start of lines to be inserted
 * @param last the end of lines (exclusive) to be inserted
 */

/**
 * @fn void LineLayoutBuffer#layoutModified(length_t first, length_t last, length_t newSublines, length_t oldSublines, bool documentChanged)
 * The layouts of lines were modified.
 * @param first the start of the lines to be modified
 * @param last the end of the lines (exclusive) to be modified
 * @param newSublines the number of sublines of the modified lines after the modification
 * @param oldSublines the number of sublines of the modified lines before the modification
 * @param documentChanged true if the modification was occured by the document change
 */

/// @see presentation#IPresentationStylistListener
void LineLayoutBuffer::presentationStylistChanged() {
	invalidate();
}

/**
 * Slides cached layouts. This method does not change @c startLine_ member.
 * @param first the first line of range to slide
 * @param offset the amount to slide
 * @param callModified set true to invoke the concrete's #layoutModified method
 */
void LineLayoutBuffer::slideCaches(length_t first, signed_length_t offset, bool callModified) throw() {
	if(offset == 0 || first >= startLine_ + bufferSize_)
		return;
	length_t clearedFirst, clearedLast, clearedSublines = 0;
	first = max(startLine_, first) - startLine_;	// layouts_ 内の相対位置に変換
	if(offset > 0) {
		const length_t clearedLines = min(static_cast<length_t>(offset), bufferSize_ - first);
		for(length_t i = bufferSize_ - 1; ; --i) {
			if(i >= bufferSize_ - clearedLines) {
				if(layouts_[i] != 0) {
					clearedSublines += layouts_[i]->getNumberOfSublines();
					delete layouts_[i];
				} else
					++clearedSublines;
			}
			const signed_length_t src = static_cast<signed_length_t>(i) - offset;
			if((layouts_[i] = (src >= 0 && static_cast<length_t>(src) >= first) ? layouts_[src] : 0) != 0)
				layouts_[i]->lineNumber_ += offset;
			if(i == first)
				break;
		}
		clearedLast = startLine_ + bufferSize_;
		clearedFirst = clearedLast - clearedLines;
	} else {	// offset < 0
		const length_t clearedLines = min(static_cast<length_t>(-offset), bufferSize_ - first);
		for(length_t i = first; i < bufferSize_; ++i) {
			if(i < first + clearedLines) {
				if(layouts_[i] != 0) {
					clearedSublines += layouts_[i]->getNumberOfSublines();
					delete layouts_[i];
				} else
					++clearedSublines;
			}
			const signed_length_t src = static_cast<signed_length_t>(i) - offset;
			if((layouts_[i] = (src >= 0 && static_cast<length_t>(src) < bufferSize_) ? layouts_[src] : 0) != 0)
				layouts_[i]->lineNumber_ += offset;
		}
		clearedFirst = first + startLine_;
		clearedLast = clearedFirst + clearedLines;
	}
	if(clearedFirst < viewer_.getDocument().getNumberOfLines()) {
//		clearedLast = min(clearedLast, viewer_.getDocument().getNumberOfLines());
		if(callModified && clearedFirst != clearedLast)
			layoutModified(clearedFirst, clearedLast, clearedLast - clearedFirst, clearedSublines, documentChangePhase_ == CHANGING);
	}
}

/**
 * @param line the line to set new start of the caches.
 */
inline void LineLayoutBuffer::updateCacheStartLine(length_t line) throw() {
//	if(line + bufferSize_ > viewer_.getDocument().getNumberOfLines())
//		line -= min(line + bufferSize_ - viewer_.getDocument().getNumberOfLines(), line);
	if(line == startLine_)
		return;
	verifyLayoutCaches();
	slideCaches(startLine_, static_cast<signed_length_t>(startLine_) - static_cast<signed_length_t>(line), true);
	startLine_ = line;
	for(length_t i = 0; i < bufferSize_; ++i) {	// すぐ上で設定したばかりなんだが...
		if(layouts_[i] != 0)
			layouts_[i]->lineNumber_ = i + startLine_;
	}
	verifyLayoutCaches();
#ifdef TRACE_LAYOUT_CACHES
	dout << "cache window moved. new window is " << startLine_ << ".." << startLine_ + bufferSize_ << "\n";
#endif
}

/// Verifies whether the layout caches connect corresponding lines. This method does nothing in release mode.
inline void LineLayoutBuffer::verifyLayoutCaches() const throw() {
#ifdef _DEBUG
	const size_t last = min(startLine_ + bufferSize_, viewer_.getDocument().getNumberOfLines());
	for(size_t i = startLine_; i < last; ++i) {
		if(layouts_[i - startLine_] != 0 && layouts_[i - startLine_]->lineNumber_ != i)
			::DebugBreak();
	}
#endif /* _DEBUG */
}


// FontSelector /////////////////////////////////////////////////////////////

FontSelector::FontAssociations FontSelector::defaultAssociations_;

/// Constructor.
FontSelector::FontSelector() : primaryFont_(L""), shapingControlsFont_(0), linkedFonts_(0) {
}

/// Destructor.
FontSelector::~FontSelector() throw() {
	::DeleteObject(shapingControlsFont_);
	for(map<int, Fontset*>::iterator i = associations_.begin(); i != associations_.end(); ++i)
		delete i->second;
	if(linkedFonts_ != 0) {
		for(list<Fontset*>::iterator i(linkedFonts_->begin()); i != linkedFonts_->end(); ++i)
			delete *i;
		delete linkedFonts_;
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
		switch(PRIMARYLANGID(getUserDefaultUILanguage())) {	// yes, this is not enough...
		case LANG_CHINESE:
			defaultAssociations_[Script::HAN] = L"PMingLiu"; break;
		case LANG_JAPANESE:
			defaultAssociations_[Script::HAN] = L"MS P Gothic"; break;
		case LANG_KOREAN:
			defaultAssociations_[Script::HAN] = L"Gulim"; break;
		}
		defaultAssociations_[Script::HANGUL] = L"Gulim";
		defaultAssociations_[Script::HEBREW] = L"Microsoft Sans Serif";
		defaultAssociations_[Script::HIRAGANA] = L"MS P Gothic";
		defaultAssociations_[Script::KANNADA] = L"Tunga";
		defaultAssociations_[Script::KATAKANA] = L"MS P Gothic";
		defaultAssociations_[Script::LATIN] = L"Tahoma";
		defaultAssociations_[Script::MALAYALAM] = L"Kartika";
		defaultAssociations_[Script::TAMIL] = L"Latha";
		defaultAssociations_[Script::TELUGU] = L"Gautami";
		defaultAssociations_[Script::THAI] = L"Tahoma";
	}
	return defaultAssociations_;
}

/**
 * Returns the font associated to the specified script.
 * @param script the language. set to @c Script#COMMON to get the primary font. other special
 * script values @c Script#UNKNOWN, @c Script#INHERITED and @c Script#KATAKANA_OR_HIRAGANA can't set
 * @param bold true to get the bold variant
 * @param italic true to get the italic variant
 * @return the font or a fallbacked font if the association was failed
 * @throw std#invalid_argument @a script is invalid
 */
HFONT FontSelector::getFont(int script /* = Script::COMMON */, bool bold /* = false */, bool italic /* = false */) const {
	if(script <= Script::UNKNOWN || script == Script::INHERITED
			|| script == Script::KATAKANA_OR_HIRAGANA || script >= Script::COUNT)
		throw invalid_argument("invalid script value.");
	Fontset* fontset;
	if(script != Script::COMMON) {
		map<int, Fontset*>::iterator i = const_cast<FontSelector*>(this)->associations_.find(script);
		if(i == associations_.end())	// フォールバック
			script = Script::COMMON;
		else
			fontset = i->second;
	}
	if(script == Script::COMMON)
		fontset = &const_cast<FontSelector*>(this)->primaryFont_;
	HFONT& font = bold ? (italic ? fontset->boldItalic : fontset->bold) : (italic ? fontset->italic : fontset->regular);
	if(font == 0)
		font = ::CreateFontW(ascent_ + descent_, 0, 0, 0, bold ? FW_BOLD : FW_REGULAR, italic, 0, 0,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontset->faceName);
	return font;
}

/**
 * Sets the primary font and the association table.
 * @param faceName the typeface name of the font
 * @param height the height of the font in logical units
 * @param associations the association table. script values @c Script#COMMON, @c Script#UNKNOWN,
 * @c Script#INHERITED and @c Script#KATAKANA_OR_HIRAGANA can't set. if this value is @c null,
 * the current associations will not be changed
 * @throw std#invalid_argument @a faceName is @c null, any script of @a associations is invalid,
 * or any typeface name of @a associations is @c null
 * @throw std#length_error the length of @a faceName or any typeface name of @a associations
 * exceeds @c LF_FACESIZE
 */
void FontSelector::setFont(const WCHAR* faceName, int height, const FontAssociations* associations) {
	// 引数をチェック
	if(faceName == 0)
		throw invalid_argument("the primary typeface name is null.");
	else if(wcslen(faceName) >= LF_FACESIZE)
		throw length_error("the primary typeface name is too long.");
	else if(associations != 0) {
		for(FontAssociations::const_iterator i = associations->begin(); i != associations->end(); ++i) {
			if(i->first == Script::COMMON || i->first == Script::UNKNOWN
					|| i->first == Script::INHERITED || i->first == Script::KATAKANA_OR_HIRAGANA)
				throw invalid_argument("the association language is invalid.");
			else if(i->second == 0)
				throw invalid_argument("the association font name is invalid.");
			else if(wcslen(i->second) >= LF_FACESIZE)
				throw length_error("the association font name is too long.");
		}
	}
	// 古いのを破棄
	primaryFont_.clear(faceName);
	if(associations != 0) {
		for(map<int, Fontset*>::iterator i = associations_.begin(); i != associations_.end(); ++i)
			delete i->second;
		associations_.clear();
	} else {
		for(map<int, Fontset*>::iterator i = associations_.begin(); i != associations_.end(); ++i)
			i->second->clear();
	}
	// 第一位のフォントを取得して、メトリクスを初期化
	primaryFont_.regular = ::CreateFontW(height, 0, 0, 0, FW_REGULAR, false, false, false,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, faceName);
	::TEXTMETRICW tm;
	auto_ptr<DC> dc(getDC());
	HFONT oldFont = dc->selectObject(primaryFont_.regular);
	dc->getTextMetrics(tm);
	ascent_ = tm.tmAscent;
	descent_ = tm.tmDescent;
	averageCharacterWidth_ = tm.tmAveCharWidth;
	dc->selectObject(oldFont);
	// フォントリンクのために実際の名前が必要
	::LOGFONTW lf;
	::GetObject(primaryFont_.regular, sizeof(::LOGFONTW), &lf);
	wcscpy(primaryFont_.faceName, lf.lfFaceName);
	if(associations != 0) {
		for(FontAssociations::const_iterator i = associations->begin(); i != associations->end(); ++i)
			associations_.insert(make_pair(i->first, new Fontset(i->second)));
	}
	if(shapingControlsFont_ != 0) {
		::DeleteObject(shapingControlsFont_);
		shapingControlsFont_ = 0;
	}
	if(linkedFonts_ != 0) {
		for(list<Fontset*>::iterator i(linkedFonts_->begin()); i != linkedFonts_->end(); ++i)
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
			if(ERROR_SUCCESS != ::RegQueryValueExW(key, primaryFont_.faceName, 0, &type, 0, &bytes)) {
				manah::AutoBuffer<::BYTE> data(new ::BYTE[bytes]);
				if(ERROR_SUCCESS != ::RegQueryValueExW(key, L"Tahoma", 0, &type, data.get(), &bytes)) {
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
	}
	fontChanged();
}


// TextRenderer /////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param viewer the text viewer
 * @throw std#invalid_argument @a viewer is not a window
 */
TextRenderer::TextRenderer(TextViewer& viewer)
		: LineLayoutBuffer(viewer, ASCENSION_TEXT_RENDERER_CACHE_LINES, true),
		longestLineWidth_(0), longestLine_(INVALID_INDEX), numberOfVisualLines_(0) {
	if(!viewer.isWindow())
		throw invalid_argument("The specified viewer is not a window.");
	::LOGFONTW lf;
	HFONT f = static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
	::GetObject(f, sizeof(::LOGFONTW), &lf);
	setFont(lf.lfFaceName, lf.lfHeight, &getDefaultFontAssociations());
	updateViewerSize();
}

/// Destructor.
TextRenderer::~TextRenderer() throw() {
}

/// @see FontSelector#fontChanged
void TextRenderer::fontChanged() {
	const TextViewer::Configuration& c = getTextViewer().getConfiguration();
	if(c.lineWrap.wraps() && specialCharacterDrawer_.get() != 0) {
		lineWrappingMarkWidth_ = specialCharacterDrawer_->getLineWrappingMarkWidth(
			(c.alignment == ALIGN_LEFT || (c.alignment == ALIGN_CENTER && c.orientation == LEFT_TO_RIGHT)) ? LEFT_TO_RIGHT : RIGHT_TO_LEFT);
	} else
		lineWrappingMarkWidth_ = 0;
	invalidate();
	visualLinesListeners_.notify(IVisualLinesListener::rendererFontChanged);
}

/// @see FontSelector#getDC
auto_ptr<DC> TextRenderer::getDC() {
	return auto_ptr<DC>(getTextViewer().isWindow() ? new ClientDC(getTextViewer().getDC()) : new ScreenDC());
}

/**
 * Returns the pitch of each lines (height + space).
 * @see FontSelector#getLineHeight
 */
int TextRenderer::getLinePitch() const throw() {
	return getLineHeight() + getTextViewer().getConfiguration().lineSpacing;
}

/**
 * Returns the actual wrap width, or the result of @c std#numeric_limits<int>#max() if wrapping is not occured
 * @see #getWrapWidth
 */
int TextRenderer::getWrapWidth() const throw() {
	const LineWrapConfiguration& c = getTextViewer().getConfiguration().lineWrap;
	if(c.wrapsAtWindowEdge())
		return viewerWidth_ - lineWrappingMarkWidth_;
	else if(c.wraps())
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
		for(length_t i = first; i < last; ++i) {
			if(isLineCached(i)) {
				const LineLayout& layout = getLineLayout(i);
				if(layout.getWidth() > newLongestLineWidth) {
					newLongestLine = i;
					newLongestLineWidth = layout.getWidth();
				}
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
 * Returns the first visual line number of the specified logical line.
 * @param line the logical line
 * @return the first visual line of @a line
 * @throw text#BadPositionException @a line is outside of the document
 * @see #mapLogicalPositionToVisualPosition, #mapVisualLineToLogicalLine
 */
length_t TextRenderer::mapLogicalLineToVisualLine(length_t line) const {
	if(line >= getTextViewer().getDocument().getNumberOfLines())
		throw text::BadPositionException();
	else if(!getTextViewer().getConfiguration().lineWrap.wraps() || line < getCacheFirstLine())
		return line;
	else if(line >= getCacheLastLine())
		return line + numberOfVisualLines_ - getTextViewer().getDocument().getNumberOfLines();
	length_t result = getCacheFirstLine();
	for(length_t i = getCacheFirstLine(); i < line; ++i)
		result += getNumberOfSublinesOfLine(i);
	return result;
}

/**
 * Returns the visual line number and the visual column number of the specified logical position.
 * @param position the logical coordinates of the position to be mapped
 * @param[out] column the visual column of @a position. can be @c null if not needed
 * @return the visual line of @a position
 * @throw text#BadPositionException @a position is outside of the document
 * @see #mapLogicalLineToVisualLine, #mapVisualPositionToLogicalPosition
 */
length_t TextRenderer::mapLogicalPositionToVisualPosition(const Position& position, length_t* column) const {
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

/**
 * Returns the logical line number and the visual subline number of the specified visual line.
 * @param line the visual line
 * @param[out] subline the visual subline of @a line. can be @c null if not needed
 * @return the logical line
 * @throw text#BadPositionException @a line is outside of the document
 * @see #mapLogicalLineToVisualLine, #mapVisualPositionToLogicalPosition
 */
length_t TextRenderer::mapVisualLineToLogicalLine(length_t line, length_t* subline) const {
	if(!getTextViewer().getConfiguration().lineWrap.wraps() || line < getCacheFirstLine()) {
		if(subline != 0)
			*subline = 0;
		return line;
	} else if(line >= getCacheLastLine() + numberOfVisualLines_ - getTextViewer().getDocument().getNumberOfLines()) {
		if(subline != 0)
			*subline = 0;
		return line + numberOfVisualLines_ - getTextViewer().getDocument().getNumberOfLines();
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
Position TextRenderer::mapVisualPositionToLogicalPosition(const Position& position) const {
	if(!getTextViewer().getConfiguration().lineWrap.wraps())
		return position;
	Position result;
	length_t subline;
	result.line = mapVisualLineToLogicalLine(position.line, &subline);
	result.column = getLineLayout(result.line).getSublineOffset(subline) + position.column;
	return result;
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

/// Returns if the complex script features supported.
bool TextRenderer::supportsComplexScript() throw() {
//	return uspLib.isAvailable();
	return true;
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
		for(size_t i = getCacheFirstLine(); i < getCacheLastLine(); ++i) {
			if(isLineCached(i)) {
				const LineLayout& layout = getLineLayout(i);
				if(layout.getWidth() > longestLineWidth_) {
					longestLine_ = i;
					longestLineWidth_ = layout.getWidth();
				}
			}
		}
	}
}

/// Informs about the change of the viewer's size.
void TextRenderer::updateViewerSize() throw() {
	const TextViewer& viewer = getTextViewer();
	if(!viewer.isWindow())
		return;
	::RECT viewerRect;
	getTextViewer().getClientRect(viewerRect);
	if(viewerRect.right - viewerRect.left > getAverageCharacterWidth()) {
		const ::RECT margins = getTextViewer().getTextAreaMargins();
		const int newWidth = viewerRect.right - viewerRect.left - margins.left - margins.right;
		if(newWidth != viewerWidth_) {
			viewerWidth_ = newWidth;
			// ウィンドウ幅で折り返す場合は再計算
			if(getTextViewer().getConfiguration().lineWrap.wrapsAtWindowEdge()) {
				for(length_t i = getCacheFirstLine(); i < getCacheLastLine(); ++i) {
					if(!isLineCached(i))
						continue;
					const LineLayout& layout = getLineLayout(i);
					if(layout.getNumberOfSublines() != 1 || layout.getWidth() > newWidth)
//						layout.rewrap();
						invalidate(i, i + 1);
				}
			}
		}
	}
}
