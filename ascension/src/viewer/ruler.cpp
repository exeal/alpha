/**
 * @file ruler.cpp
 * @author exeal
 * @date 2010-10-27 created (separated code from layout.cpp)
 */

#include <ascension/viewer/viewer.hpp>
#include <usp10.h>

using namespace ascension;
using namespace ascension::graphics;
using namespace ascension::graphics::font;
using namespace ascension::presentation;
using namespace ascension::viewers;
using namespace std;

extern const bool DIAGNOSE_INHERENT_DRAWING;


// RulerConfiguration /////////////////////////////////////////////////////////////////////////////

/// Default constructor.
RulerConfiguration::RulerConfiguration() /*throw()*/ : alignment(TEXT_ANCHOR_START) {
}


// RulerConfiguration.LineNumbers /////////////////////////////////////////////////////////////////

/// Constructor initializes the all members to their default values.
RulerConfiguration::LineNumbers::LineNumbers() /*throw()*/ : visible(false),
		anchor(TEXT_ANCHOR_END), startValue(1), minimumDigits(4), paddingStart(6), paddingEnd(1) {
}


// RulerConfiguration.IndicatorMargin /////////////////////////////////////////////////////////////

/// Constructor initializes the all members to their default values.
RulerConfiguration::IndicatorMargin::IndicatorMargin() /*throw()*/ : visible(false)/*, width(15)*/ {
}


// detail.RulerPainter ////////////////////////////////////////////////////////////////////////////

// TODO: support locale-dependent number format.

HRESULT drawLineNumber(Context& context, int x, int y, length_t lineNumber/*, const SCRIPT_CONTROL& control, const SCRIPT_STATE& initialState*/) {
	// format number string
	wchar_t s[128];	// oops, is this sufficient?
#if defined(_MSC_VER) && (_MSC_VER < 1400)
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
 * Constructor.
 * @param viewer The text viewer
 * @param enableDoubleBuffering Set @c true to use double-buffering for non-flicker drawing
 */
detail::RulerPainter::RulerPainter(TextViewer& viewer, bool enableDoubleBuffering)
		: viewer_(viewer), width_(0), lineNumberDigitsCache_(0), enablesDoubleBuffering_(enableDoubleBuffering) {
	recalculateWidth();
}

/// Returns the maximum number of digits of line numbers.
uint8_t detail::RulerPainter::maximumDigitsForLineNumbers() const /*throw()*/ {
	uint8_t n = 1;
	length_t lines = viewer_.document().numberOfLines() + configuration_.lineNumbers.startValue - 1;
	while(lines >= 10) {
		lines /= 10;
		++n;
	}
	return static_cast<uint8_t>(n);	// hmm...
}

/**
 * Paints the ruler.
 * @param context The graphics context
 */
void detail::RulerPainter::paint(PaintContext& context) {
	if(width() == 0)
		return;

	const NativeRectangle paintBounds(context.boundsToPaint());
	const TextRenderer& renderer = viewer_.textRenderer();
	const NativeRectangle clientBounds(viewer_.bounds(false));
	const bool leftAligned = utils::isRulerLeftAligned(viewer_);

	if()
	if((leftAligned && geometry::left(paintBounds) >= geometry::left(clientBounds) + width())
			|| (!leftAligned && geometry::right(paintBounds) < geometry::right(clientBounds) - width()))
		return;

#ifdef _DEBUG
	if(DIAGNOSE_INHERENT_DRAWING)
		win32::DumpContext() << L"@VerticalRulerDrawer.draw draws y = "
			<< geometry::top(paintBounds) << L" ~ " << geometry::bottom(paintBounds) << L"\n";
#endif // _DEBUG

	context.save();
	const Scalar imWidth = configuration_.indicatorMargin.visible ? configuration_.indicatorMargin.width : 0;

	Scalar left;
	HDC dcex;
	if(enablesDoubleBuffering_) {
		if(memoryDC_.get() == 0)
			memoryDC_.reset(::CreateCompatibleDC(win32::ClientAreaGraphicsContext(viewer_.identifier()).nativeHandle().get()), &::DeleteDC);
		if(memoryBitmap_.get() == 0)
			memoryBitmap_.reset(::CreateCompatibleBitmap(context.nativeHandle().get(),
				width(), geometry::dy(clientBounds) + ::GetSystemMetrics(SM_CYHSCROLL)), &::DeleteObject);
		::SelectObject(memoryDC_.get(), memoryBitmap_.get());
		dcex = memoryDC_.get();
		left = 0;
	} else {
		dcex = context.nativeHandle().get();
		left = leftAligned ? geometry::left(clientBounds) : geometry::right(clientBounds) - width();
	}
	const Scalar right = left + width();

	// first of all, paint the drawing area
	if(configuration_.indicatorMargin.visible) {
		// border and inside of the indicator margin
		const Scalar borderX = leftAligned ? left + imWidth - 1 : right - imWidth;
		HPEN oldPen = static_cast<HPEN>(::SelectObject(dcex, indicatorMarginPen_.get()));
		HBRUSH oldBrush = static_cast<HBRUSH>(::SelectObject(dcex, indicatorMarginBrush_.get()));
		::PatBlt(dcex, leftAligned ? left : borderX + 1, geometry::top(paintBounds), imWidth, geometry::dy(paintBounds), PATCOPY);
		::MoveToEx(dcex, borderX, geometry::top(paintBounds), 0);
		::LineTo(dcex, borderX, geometry::bottom(paintBounds));
		::SelectObject(dcex, oldPen);
		::SelectObject(dcex, oldBrush);
	}
	if(configuration_.lineNumbers.visible) {
		// background of the line numbers
		HBRUSH oldBrush = static_cast<HBRUSH>(::SelectObject(dcex, lineNumbersBrush_.get()));
		::PatBlt(dcex, leftAligned ? left + imWidth : left, geometry::top(paintBounds), right - imWidth, geometry::bottom(paintBounds), PATCOPY);
		// border of the line numbers
		if(configuration_.lineNumbers.borderStyle != RulerConfiguration::LineNumbers::NONE) {
			HPEN oldPen = static_cast<HPEN>(::SelectObject(dcex, lineNumbersPen_.get()));
			const Scalar x = (leftAligned ? right : left + 1) - configuration_.lineNumbers.borderWidth;
			::MoveToEx(dcex, x, 0/*paintRect.top*/, 0);
			::LineTo(dcex, x, geometry::bottom(paintBounds));
			::SelectObject(dcex, oldPen);
		}
		::SelectObject(dcex, oldBrush);

		// for next...
		::SetBkMode(dcex, TRANSPARENT);
		::SetTextColor(dcex, systemColors.serve(configuration_.lineNumbers.textColor.foreground, COLOR_WINDOWTEXT));
		::SetTextCharacterExtra(dcex, 0);	// line numbers ignore character extra
		::SelectObject(dcex, renderer.defaultFont()->nativeHandle().get());
	}

	// prepare to draw the line numbers
	Inheritable<ReadingDirection> lineNumbersReadingDirection;
	TextAnchor lineNumbersAlignment;
	Scalar lineNumbersX;
	if(configuration_.lineNumbers.visible) {
		// compute reading direction of the line numbers from 'configuration_.lineNumbers.readingDirection'
		if(configuration_.lineNumbers.readingDirection.inherits()) {
			const tr1::shared_ptr<const TextLineStyle> defaultLineStyle(viewer_.presentation().globalTextStyle()->defaultLineStyle);
			if(defaultLineStyle.get() != 0)
				lineNumbersReadingDirection = defaultLineStyle->readingDirection;
			if(lineNumbersReadingDirection.inherits())
				lineNumbersReadingDirection = renderer.defaultUIWritingMode().inlineFlowDirection;
			if(lineNumbersReadingDirection.inherits())
				lineNumbersReadingDirection = ASCENSION_DEFAULT_TEXT_READING_DIRECTION;
			assert(lineNumbersReadingDirection == LEFT_TO_RIGHT || lineNumbersReadingDirection == RIGHT_TO_LEFT);
		} else if(configuration_.lineNumbers.readingDirection == LEFT_TO_RIGHT
				|| configuration_.lineNumbers.readingDirection == RIGHT_TO_LEFT)
			lineNumbersReadingDirection = configuration_.lineNumbers.readingDirection;
		else
			throw runtime_error("can't resolve reading direction of line numbers in vertical ruler.");

		// compute alignment of the line numbers from 'configuration_.lineNumbers.anchor'
		switch(detail::computePhysicalTextAnchor(configuration_.lineNumbers.anchor, lineNumbersReadingDirection)) {
		case detail::LEFT:
			lineNumbersX = leftAligned ?
				left + imWidth + configuration_.lineNumbers.leadingMargin : left + configuration_.lineNumbers.trailingMargin + 1;
			::SetTextAlign(dcex, TA_LEFT | TA_TOP | TA_NOUPDATECP);
			break;
		case detail::RIGHT:
			lineNumbersX = leftAligned ?
				right - configuration_.lineNumbers.trailingMargin - 1 : right - imWidth - configuration_.lineNumbers.leadingMargin;
			::SetTextAlign(dcex, TA_RIGHT | TA_TOP | TA_NOUPDATECP);
			break;
		case detail::MIDDLE:
			lineNumbersX = leftAligned ?
				left + (imWidth + configuration_.lineNumbers.leadingMargin + width() - configuration_.lineNumbers.trailingMargin) / 2
				: right - (width() - configuration_.lineNumbers.trailingMargin + imWidth + configuration_.lineNumbers.leadingMargin) / 2;
			::SetTextAlign(dcex, TA_CENTER | TA_TOP | TA_NOUPDATECP);
			break;
		}
	}

	// 1 行ずつ細かい描画
	length_t line, visualSublineOffset;
	const length_t lines = viewer_.document().numberOfLines();
	viewer_.mapClientYToLine(geometry::top(paintBounds), &line, &visualSublineOffset);	// $friendly-access
	if(visualSublineOffset > 0)	// 描画開始は次の論理行から...
		++line;
	int y = viewer_.mapLineToClientY(line, false);
	if(y != 32767 && y != -32768) {
		const int dy = renderer.defaultFont()->metrics().linePitch();
		while(y < geometry::bottom(paintBounds) && line < lines) {
			const TextLayout& layout = renderer.layouts().at(line);
			const int nextY = y + static_cast<int>(layout.numberOfLines() * dy);
			if(nextY >= geometry::top(paintBounds)) {
				// 派生クラスにインジケータマージンの描画機会を与える
				if(configuration_.indicatorMargin.visible) {
					NativeRectangle rect;
					geometry::range<geometry::X_COORDINATE>(rect) = makeRange(
						leftAligned ? left : right - configuration_.indicatorMargin.width,
						leftAligned ? left + configuration_.indicatorMargin.width : right);
					geometry::range<geometry::Y_COORDINATE>(rect) = makeRange(y, y + dy);
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
			leftAligned ? geometry::left(clientBounds) : geometry::right(clientBounds) - width(), geometry::top(paintBounds),
			right - left, geometry::dy(paintBounds), memoryDC_.get(), 0, geometry::top(paintBounds), SRCCOPY);
	context.restore();
}

namespace {
	Scalar computeMaximumNumberGlyphsExtent(Context& context, tr1::shared_ptr<const Font> font,
			uint8_t digits, const WritingMode& writingMode, const NumberSubstitution& numberSubstitution) {
		tr1::shared_ptr<const Font> oldFont(context.font());
		context.setFont(font);
/*
#if defined(ASCENSION_SHAPING_ENGINE_UNISCRIBE)
		SCRIPT_STRING_ANALYSIS ssa;
		win32::AutoZero<SCRIPT_CONTROL> sc;
		win32::AutoZero<SCRIPT_STATE> ss;
		HRESULT hr;
//		switch(configuration_.lineNumbers.digitSubstitution) {
//			case DST_CONTEXTUAL:
//			case DST_NOMINAL:
//				break;
//			case DST_NATIONAL:
//				ss.fDigitSubstitute = 1;
//				break;
//			case DST_USER_DEFAULT:
				hr = ::ScriptApplyDigitSubstitution(&numberSubstitution.asUniscribe(), &sc, &ss);
//				break;
//		}
		::SetTextCharacterExtra(dc.get(), 0);
		hr = ::ScriptStringAnalyse(dc.get(), L"0123456789", 10,
			estimateNumberOfGlyphs(10), -1, SSA_FALLBACK | SSA_GLYPHS | SSA_LINK, 0, &sc, &ss, 0, 0, 0, &ssa);
		int glyphWidths[10];
		hr = ::ScriptStringGetLogicalWidths(ssa, glyphWidths);
		int maxGlyphWidth = *max_element(glyphWidths, ASCENSION_ENDOF(glyphWidths));
#else
#endif
*/
		Char maximumExtentCharacter;
		font->missing();
		const NativeSize stringExtent(context.measureText(String(digits, maximumExtentCharacter)));

		context.setFont(oldFont);
		return (writingMode.blockFlowDirection == WritingMode::HORIZONTAL_TB) ? geometry::dx(stringExtent) : geometry::dy(stringExtent);
	}
}

/// Recalculates the total width of the ruler.
void detail::RulerPainter::recalculateWidth() /*throw()*/ {
	// (ruler-total-width) = (line-numbers-width) + (indicator-margin-width)
	//   (indicator-margin-width) = (indicator-margin-border-width) + (indicator-margin-body-width)
	//   (line-numbers-width) = (line-numbers-exterior-width) + (line-numbers-interior-width) + (line-numbers-body-width)
	//     (line-numbers-exterior-width) = (line-numbers-border-width) + (line-numbers-space-width)
	//     (line-numbers-interior-width) = (line-numbers-padding-start) + (line-numbers-padding-end)
	//     (line-numbers-body-width) = max((glyphs-extent), (average-glyph-extent) * (minimum-digits-setting))

	Scalar totalWidth = 0;
	if(configuration_.lineNumbers.visible) {
		const uint8_t digits = maximumDigitsForLineNumbers();
		if(digits != lineNumberDigitsCache_) {
			lineNumberDigitsCache_ = digits;
		}
		const Scalar glyphsExtent = computeMaximumNumberGlyphsExtent(
			viewer_.graphicsContext(), viewer_.textRenderer().defaultFont(), digits,
			viewer_.presentation().globalTextStyle()->writingMode, configuration().lineNumbers.numberSubstitution);
		const Scalar minimumExtent = viewer_.textRenderer().defaultFont()->metrics().averageCharacterWidth() * digits;
		totalWidth += max(glyphsExtent, minimumExtent);

		const Scalar borderWidth = configuration_.lineNumbers.border.hasVisibleStyle() ?
			configuration_.lineNumbers.border.width.compute() : 0;
		const Scalar spaceWidth = 0;
		const Scalar exteriorWidth = borderWidth + spaceWidth;
		totalWidth += exteriorWidth;

		const Scalar interiorWidth =
			configuration().lineNumbers.paddingStart.compute() + configuration().lineNumbers.paddingEnd.compute();
		totalWidth += interiorWidth;
	}

	// compute the width of the indicator margin
	if(configuration_.indicatorMargin.visible)
		totalWidth_ += configuration().indicatorMargin.width.inherits() ?
			platformIndicatorMargin() : configuration().indicatorMargin.width.inherits().compute();

	if(totalWidth != width_) {
		width_ = totalWidth;
		viewer_.scheduleRedraw(false);
		viewer_.updateCaretPosition();
	}
}

void detail::RulerPainter::setConfiguration(const RulerConfiguration& configuration) {
	if(configuration.alignment != TEXT_ANCHOR_START && configuration.alignment != TEXT_ANCHOR_END)
		throw UnknownValueException("configuration.alignment");
	if(configuration.lineNumbers.anchor != TEXT_ANCHOR_START
			&& configuration.lineNumbers.anchor != TEXT_ANCHOR_MIDDLE
			&& configuration.lineNumbers.anchor != TEXT_ANCHOR_END)
		throw UnknownValueException("configuration.lineNumbers.anchor");
//	if(configuration.lineNumbers.justification != ...)
//		throw UnknownValueException("");
	if(!configuration.lineNumbers.readingDirection.inherits()
			&& configuration.lineNumbers.readingDirection != LEFT_TO_RIGHT
			&& configuration.lineNumbers.readingDirection != RIGHT_TO_LEFT)
		throw UnknownValueException("configuration.lineNumbers.readingDirection");
	configuration_ = configuration;
	update();
}

void detail::RulerPainter::update() /*throw()*/ {
	lineNumberDigitsCache_ = 0;
	recalculateWidth();
	updateGDIObjects();
	if(enablesDoubleBuffering_ && memoryBitmap_.get() != 0)
		memoryBitmap_.reset();
}

///
void detail::RulerPainter::updateGDIObjects() /*throw()*/ {
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
