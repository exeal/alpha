/**
 * @file ruler.cpp
 * @author exeal
 * @date 2010-10-27 created (separated code from layout.cpp)
 */

#include <ascension/viewer/viewer.hpp>
#include <usp10.h>

using namespace ascension;
using namespace ascension::graphics;
using namespace ascension::presentation;
using namespace ascension::viewers;
using namespace std;

extern const bool DIAGNOSE_INHERENT_DRAWING;


// TextViewer.RulerPainter ////////////////////////////////////////////////////////////////////////

// TODO: support locale-dependent number format.

HRESULT drawLineNumber(Context& context, int x, int y, length_t lineNumber/*, const SCRIPT_CONTROL& control, const SCRIPT_STATE& initialState*/) {
	// format number string
	wchar_t s[128];	// oops, is this sufficient?
#if(_MSC_VER < 1400)
	const int length = swprintf(s, L"%lu", lineNumber);
#else
	const int length = swprintf(s, ASCENSION_COUNTOF(s), L"%lu", lineNumber);
#endif // _MSC_VER < 1400

	UINT option;
#if 0
	if(!ignoreUserOverride)
		option = 0;
	else {
		switch(???) {
			case CONTEXTUAL:
			case NONE:
				option = ETO_NUMERICSLATIN;
				break;
			case FROM_LOCALE:
			case NATIONAL:
			case TRADITIONAL:
				option = ETO_NUMERICSLOCAL;
				break;
		}
	}
#else
	option = 0;
#endif
	context.drawText(x, y, s);
	::ExtTextOutW(context.engine()->nativeHandle().get(), x, y, option, 0, s, length, 0);
	return S_OK;
}

/**
 * Paints the ruler.
 * @param context the graphics context
 */
void TextViewer::RulerPainter::paint(PaintContext& context) {
	if(width() == 0)
		return;

	const Rect<> paintBounds(context.boundsToPaint());
	const TextRenderer& renderer = viewer_.textRenderer();
	const Rect<> clientBounds(viewer_.bounds(false));
	const TextAlignment alignment = utils::computeRulerAlignment(viewer_);
	if((alignment == ALIGN_LEFT && paintBounds.left() >= clientBounds.left() + width())
			|| (alignment == ALIGN_RIGHT && paintBounds.right() < clientBounds.right() - width()))
		return;

#ifdef _DEBUG
	if(DIAGNOSE_INHERENT_DRAWING)
		win32::DumpContext() << L"@VerticalRulerDrawer.draw draws y = "
			<< paintBounds.top() << L" ~ " << paintBounds.bottom() << L"\n";
#endif // _DEBUG

	context.save();
	const bool alignLeft = alignment == ALIGN_LEFT;
	const int imWidth = configuration_.indicatorMargin.visible ? configuration_.indicatorMargin.width : 0;

	int left;
	HDC dcex;
	if(enablesDoubleBuffering_) {
		if(memoryDC_.get() == 0)
			memoryDC_.reset(::CreateCompatibleDC(win32::ClientAreaGraphicsContext(viewer_.handle()).nativeHandle().get()), &::DeleteDC);
		if(memoryBitmap_.get() == 0)
			memoryBitmap_.reset(::CreateCompatibleBitmap(context.nativeHandle().get(),
				width(), clientBounds.height() + ::GetSystemMetrics(SM_CYHSCROLL)), &::DeleteObject);
		::SelectObject(memoryDC_.get(), memoryBitmap_.get());
		dcex = memoryDC_.get();
		left = 0;
	} else {
		dcex = context.nativeHandle().get();
		left = alignLeft ? clientBounds.left() : clientBounds.right() - width();
	}
	const int right = left + width();

	// first of all, paint the drawing area
	if(configuration_.indicatorMargin.visible) {
		// border and inside of the indicator margin
		const int borderX = alignLeft ? left + imWidth - 1 : right - imWidth;
		HPEN oldPen = static_cast<HPEN>(::SelectObject(dcex, indicatorMarginPen_.get()));
		HBRUSH oldBrush = static_cast<HBRUSH>(::SelectObject(dcex, indicatorMarginBrush_.get()));
		::PatBlt(dcex, alignLeft ? left : borderX + 1, paintBounds.top(), imWidth, paintBounds.height(), PATCOPY);
		::MoveToEx(dcex, borderX, paintBounds.top(), 0);
		::LineTo(dcex, borderX, paintBounds.bottom());
		::SelectObject(dcex, oldPen);
		::SelectObject(dcex, oldBrush);
	}
	if(configuration_.lineNumbers.visible) {
		// background of the line numbers
		HBRUSH oldBrush = static_cast<HBRUSH>(::SelectObject(dcex, lineNumbersBrush_.get()));
		::PatBlt(dcex, alignLeft ? left + imWidth : left, paintBounds.top(), right - imWidth, paintBounds.bottom(), PATCOPY);
		// border of the line numbers
		if(configuration_.lineNumbers.borderStyle != VerticalRulerConfiguration::LineNumbers::NONE) {
			HPEN oldPen = static_cast<HPEN>(::SelectObject(dcex, lineNumbersPen_.get()));
			const int x = (alignLeft ? right : left + 1) - configuration_.lineNumbers.borderWidth;
			::MoveToEx(dcex, x, 0/*paintRect.top*/, 0);
			::LineTo(dcex, x, paintBounds.bottom());
			::SelectObject(dcex, oldPen);
		}
		::SelectObject(dcex, oldBrush);

		// for next...
		::SetBkMode(dcex, TRANSPARENT);
		::SetTextColor(dcex, systemColors.serve(configuration_.lineNumbers.textColor.foreground, COLOR_WINDOWTEXT));
		::SetTextCharacterExtra(dcex, 0);	// line numbers ignore character extra
		::SelectObject(dcex, renderer.primaryFont()->nativeHandle().get());
	}

	// prepare to draw the line numbers
	ReadingDirection lineNumbersReadingDirection;
	TextAlignment lineNumbersAlignment;
	int lineNumbersX;
	if(configuration_.lineNumbers.visible) {
		// compute reading direction of the line numbers from 'configuration_.lineNumbers.readingDirection'
		switch(configuration_.lineNumbers.readingDirection) {
			case LEFT_TO_RIGHT:
			case RIGHT_TO_LEFT:
				lineNumbersReadingDirection = configuration_.lineNumbers.readingDirection;
				break;
			case INHERIT_READING_DIRECTION: {
				tr1::shared_ptr<const LineStyle> defaultLineStyle(viewer_.presentation().defaultLineStyle());
				if(defaultLineStyle.get() != 0)
					lineNumbersReadingDirection = defaultLineStyle->readingDirection;
				if(lineNumbersReadingDirection == INHERIT_READING_DIRECTION)
					lineNumbersReadingDirection = renderer.defaultUIReadingDirection();
				if(lineNumbersReadingDirection == INHERIT_READING_DIRECTION)
					lineNumbersReadingDirection = ASCENSION_DEFAULT_TEXT_READING_DIRECTION;
				assert(lineNumbersReadingDirection == LEFT_TO_RIGHT || lineNumbersReadingDirection == RIGHT_TO_LEFT);
				break;
			}
			default:
				throw runtime_error("can't resolve reading direction of line numbers in vertical ruler.");
		}
		// compute alignment of the line numbers from 'configuration_.lineNumbers.alignment'
		switch(configuration_.lineNumbers.alignment) {
			case ALIGN_LEFT:
			case ALIGN_RIGHT:
			case ALIGN_CENTER:
			case JUSTIFY:
				lineNumbersAlignment = configuration_.lineNumbers.alignment;
				break;
			case ALIGN_START:
			case ALIGN_END:
				lineNumbersAlignment = resolveTextAlignment(configuration_.lineNumbers.alignment, lineNumbersReadingDirection);
				break;
			default:
				throw runtime_error("can't resolve alignment of line numbers in vertical ruler.");
		}
		switch(lineNumbersAlignment) {
		case ALIGN_LEFT:
			lineNumbersX = alignLeft ?
				left + imWidth + configuration_.lineNumbers.leadingMargin : left + configuration_.lineNumbers.trailingMargin + 1;
			::SetTextAlign(dcex, TA_LEFT | TA_TOP | TA_NOUPDATECP);
			break;
		case ALIGN_RIGHT:
			lineNumbersX = alignLeft ?
				right - configuration_.lineNumbers.trailingMargin - 1 : right - imWidth - configuration_.lineNumbers.leadingMargin;
			::SetTextAlign(dcex, TA_RIGHT | TA_TOP | TA_NOUPDATECP);
			break;
		case ALIGN_CENTER:
			lineNumbersX = alignLeft ?
				left + (imWidth + configuration_.lineNumbers.leadingMargin + width() - configuration_.lineNumbers.trailingMargin) / 2
				: right - (width() - configuration_.lineNumbers.trailingMargin + imWidth + configuration_.lineNumbers.leadingMargin) / 2;
			::SetTextAlign(dcex, TA_CENTER | TA_TOP | TA_NOUPDATECP);
			break;
		}
	}

	// 1 行ずつ細かい描画
	length_t line, visualSublineOffset;
	const length_t lines = viewer_.document().numberOfLines();
	viewer_.mapClientYToLine(paintBounds.top(), &line, &visualSublineOffset);	// $friendly-access
	if(visualSublineOffset > 0)	// 描画開始は次の論理行から...
		++line;
	int y = viewer_.mapLineToClientY(line, false);
	if(y != 32767 && y != -32768) {
		const int dy = renderer.textMetrics().linePitch();
		while(y < paintBounds.bottom() && line < lines) {
			const LineLayout& layout = renderer.lineLayout(line);
			const int nextY = y + static_cast<int>(layout.numberOfSublines() * dy);
			if(nextY >= paintBounds.top()) {
				// 派生クラスにインジケータマージンの描画機会を与える
				if(configuration_.indicatorMargin.visible) {
					Rect<int> rect;
					rect.setX(makeRange(
						alignLeft ? left : right - configuration_.indicatorMargin.width,
						alignLeft ? left + configuration_.indicatorMargin.width : right));
					rect.setY(makeRange(y, y + dy));
						viewer_.drawIndicatorMargin(line, *dcex, rect);
				}

				// draw line number digits
				if(configuration_.lineNumbers.visible)
					drawLineNumber(*dcex, lineNumbersX, y, line + configuration_.lineNumbers.startValue);
			}
			++line;
			y = nextY;
		}
	}

	if(enablesDoubleBuffering_)
		::BitBlt(context.engine()->nativeHandle().get(),
			alignLeft ? clientBounds.left() : clientBounds.right() - width(), paintBounds.top(),
			right - left, paintBounds.height(), memoryDC_.get(), 0, paintBounds.top(), SRCCOPY);
	context.restore();
}

/// Recalculates the width of the ruler.
void TextViewer::RulerDrawer::recalculateWidth() /*throw()*/ {
	int newWidth = 0;
	if(configuration_.lineNumbers.visible) {
		const uchar newLineNumberDigits = getLineNumberMaxDigits();
		if(newLineNumberDigits != lineNumberDigitsCache_) {
			// the width of the line numbers area is determined by the maximum width of glyphs of 0..9
			win32::Handle<HDC> dc(viewer_.getDC());
			HFONT oldFont = static_cast<HFONT>(::SelectObject(dc.use(), viewer_.textRenderer().primaryFont()->nativeHandle().get()));
			SCRIPT_STRING_ANALYSIS ssa;
			win32::AutoZero<SCRIPT_CONTROL> sc;
			win32::AutoZero<SCRIPT_STATE> ss;
			HRESULT hr;
/*			switch(configuration_.lineNumbers.digitSubstitution) {
			case DST_CONTEXTUAL:
			case DST_NOMINAL:
				break;
			case DST_NATIONAL:
				ss.fDigitSubstitute = 1;
				break;
			case DST_USER_DEFAULT:
*/				hr = ::ScriptApplyDigitSubstitution(&userSettings.digitSubstitution(false), &sc, &ss);
/*				break;
			}
*/			::SetTextCharacterExtra(dc.get(), 0);
			hr = ::ScriptStringAnalyse(dc.get(), L"0123456789", 10,
				estimateNumberOfGlyphs(10), -1, SSA_FALLBACK | SSA_GLYPHS | SSA_LINK, 0, &sc, &ss, 0, 0, 0, &ssa);
			::SelectObject(dc.get(), oldFont);
			int glyphWidths[10];
			hr = ::ScriptStringGetLogicalWidths(ssa, glyphWidths);
			int maxGlyphWidth = *max_element(glyphWidths, ASCENSION_ENDOF(glyphWidths));
			lineNumberDigitsCache_ = newLineNumberDigits;
			if(maxGlyphWidth != 0) {
				newWidth += max<uchar>(newLineNumberDigits, configuration_.lineNumbers.minimumDigits) * maxGlyphWidth;
				newWidth += configuration_.lineNumbers.leadingMargin + configuration_.lineNumbers.trailingMargin;
				if(configuration_.lineNumbers.borderStyle != RulerConfiguration::LineNumbers::NONE)
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

///
void TextViewer::RulerPainter::updateGDIObjects() /*throw()*/ {
	indicatorMarginPen_.reset();
	indicatorMarginBrush_.reset();
	if(configuration_.indicatorMargin.visible) {
		indicatorMarginPen_.reset(::CreatePen(PS_SOLID, 1,
			systemColors.serve(configuration_.indicatorMargin.borderColor, COLOR_3DSHADOW)), &::DeleteObject);
		indicatorMarginBrush_.reset(::CreateSolidBrush(
			systemColors.serve(configuration_.indicatorMargin.color, COLOR_3DFACE)), &::DeleteObject);
	}

	lineNumbersPen_.reset();
	lineNumbersBrush_.reset();
	if(configuration_.lineNumbers.visible) {
		if(configuration_.lineNumbers.borderStyle == RulerConfiguration::LineNumbers::SOLID)	// 実線
			lineNumbersPen_.reset(::CreatePen(PS_SOLID, configuration_.lineNumbers.borderWidth,
				systemColors.serve(configuration_.lineNumbers.borderColor, COLOR_WINDOWTEXT)), &::DeleteObject);
		else if(configuration_.lineNumbers.borderStyle != RulerConfiguration::LineNumbers::NONE) {
			LOGBRUSH brush;
			brush.lbColor = systemColors.serve(configuration_.lineNumbers.borderColor, COLOR_WINDOWTEXT);
			brush.lbStyle = BS_SOLID;
			if(configuration_.lineNumbers.borderStyle == RulerConfiguration::LineNumbers::DASHED)	// 破線
				lineNumbersPen_.reset(::ExtCreatePen(
					PS_GEOMETRIC | PS_DASH | PS_ENDCAP_FLAT, configuration_.lineNumbers.borderWidth, &brush, 0, 0), &::DeleteObject);
			else if(configuration_.lineNumbers.borderStyle == RulerConfiguration::LineNumbers::DASHED_ROUNDED)	// 丸破線
				lineNumbersPen_.reset(::ExtCreatePen(
					PS_GEOMETRIC | PS_DASH | PS_ENDCAP_ROUND, configuration_.lineNumbers.borderWidth, &brush, 0, 0), &::DeleteObject);
			else if(configuration_.lineNumbers.borderStyle == RulerConfiguration::LineNumbers::DOTTED)	// 点線
				lineNumbersPen_.reset(::ExtCreatePen(
					PS_GEOMETRIC | PS_DOT, configuration_.lineNumbers.borderWidth, &brush, 0, 0), &::DeleteObject);
		}
		lineNumbersBrush_.reset(::CreateSolidBrush(
			systemColors.serve(configuration_.lineNumbers.textColor.background, COLOR_WINDOW)), &::DeleteObject);
	}
}
