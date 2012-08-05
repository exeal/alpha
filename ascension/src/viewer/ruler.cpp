/**
 * @file ruler.cpp
 * @author exeal
 * @date 2010-10-27 created (separated code from layout.cpp)
 * @date 2010-2012
 */

#include <ascension/graphics/rendering-context.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/viewer.hpp>

using namespace ascension;
using namespace ascension::graphics;
using namespace ascension::graphics::font;
using namespace ascension::presentation;
using namespace ascension::viewers;
using namespace std;
using detail::RulerPainter;

extern const bool DIAGNOSE_INHERENT_DRAWING;


// RulerConfiguration.IndicatorMargin /////////////////////////////////////////////////////////////

/// Constructor initializes the all members to their default values.
RulerStyles::IndicatorMargin::IndicatorMargin() /*noexcept*/ : visible(false), width(Length(0)) {
}


// detail.RulerPainter ////////////////////////////////////////////////////////////////////////////

// TODO: support locale-dependent number format.

void drawLineNumber(PaintContext& context, const NativePoint& origin, Index lineNumber, const NumberSubstitution& ns) {
	// format number string
	wchar_t s[128];	// oops, is this sufficient?
	// TODO: std.swprintf may be slow.
	// TODO: use 'ns' parameter.
#if defined(_MSC_VER) && (_MSC_VER < 1400)
	const int length = swprintf(s, L"%lu", lineNumber);
#else
	const int length = swprintf(s, ASCENSION_COUNTOF(s), L"%lu", lineNumber);
#endif // _MSC_VER < 1400

	context.fillText(s, origin);
}

/**
 * Constructor.
 * @param viewer The text viewer
 * @throw std#bad_alloc
 */
RulerPainter::RulerPainter(TextViewer& viewer, shared_ptr<const RulerStyles> initialStyles /* = nullptr */) : viewer_(viewer), declaredStyles_(initialStyles) {
	if(declaredStyles_.get() == nullptr)
		declaredStyles_.reset(new RulerStyles);
	computeAllocationWidth();
}

/**
 * Computes the snap alignment of the ruler of the text viewer.
 * @param viewer The text viewer
 * @return The snap alignment of the ruler in the text viewer
 */
RulerPainter::SnapAlignment RulerPainter::alignment() const {
	presentation::TextAlignment computedAlignment;
	if(!declaredStyles().alignment.inherits())
		computedAlignment = declaredStyles().alignment.get();
	else {
		shared_ptr<const TextLineStyle> defaultLineStyle(defaultTextLineStyle(viewer_.presentation().globalTextStyle()));
		assert(defaultLineStyle.get() != nullptr);
		computedAlignment = !defaultLineStyle->textAlignment.inherits() ?
			defaultLineStyle->textAlignment.get() : declaredStyles().alignment.initialValue();
	}

	const WritingMode writingMode(viewer_.textRenderer().writingMode());
	detail::PhysicalTextAnchor anchor;
	switch(declaredStyles().alignment.getOrInitial()) {
		case presentation::TextAlignment::START:
			anchor = detail::computePhysicalTextAnchor(presentation::TextAnchor::START, writingMode.inlineFlowDirection);
			break;
		case presentation::TextAlignment::END:
			anchor = detail::computePhysicalTextAnchor(presentation::TextAnchor::END, writingMode.inlineFlowDirection);
			break;
		case presentation::TextAlignment::LEFT:
			anchor = detail::PhysicalTextAnchor::LEFT;
			break;
		case presentation::TextAlignment::RIGHT:
			anchor = detail::PhysicalTextAnchor::RIGHT;
			break;
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}
	switch(anchor) {
		case detail::PhysicalTextAnchor::LEFT:
			return isHorizontal(writingMode.blockFlowDirection) ? LEFT : TOP;
		case detail::PhysicalTextAnchor::RIGHT:
			return isHorizontal(writingMode.blockFlowDirection) ? RIGHT : BOTTOM;
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}
}

namespace {
	Scalar computeMaximumNumberGlyphsExtent(RenderingContext2D& context, shared_ptr<const Font> font,
			uint8_t digits, const WritingMode& writingMode, const NumberSubstitution& numberSubstitution) {
		shared_ptr<const Font> oldFont(context.font());
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
		Scalar maximumAdvance = 0;
		for(Char c = '0'; c <= '9'; ++c) {
			unique_ptr<const GlyphVector> glyphs(font->createGlyphVector(String(1, c)));
			shared_ptr<GlyphMetrics> gm(glyphs->metrics(0));
			const Scalar advance = isHorizontal(writingMode.blockFlowDirection) ? gm->advanceX() : gm->advanceY();
			if(advance > maximumAdvance) {
				maximumExtentCharacter = c;
				maximumAdvance = advance;
			}
		}
		const NativeSize stringExtent(context.measureText(String(digits, maximumExtentCharacter)));

		context.setFont(oldFont);
		return isHorizontal(writingMode.blockFlowDirection) ? geometry::dx(stringExtent) : geometry::dy(stringExtent);
	}
	
	inline Scalar platformIndicatorMarginWidthInPixels(bool horizontalLayout) {
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32)
#if 1
		const int width = ::GetSystemMetrics(horizontalLayout ? SM_CYHSCROLL : SM_CXVSCROLL);
		return (width != 0) ? width : 15;
#else
		// TODO: This code is not suitable when the indicator margin top or bottom of the viewer?
		win32::AutoZeroSize<NONCLIENTMETRICSW> ncm;
		if(win32::boole(::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &ncm, 0)))
			width = Length(ncm.iScrollWidth, Length::PIXELS);
		else
			width = Length(15, Length::PIXELS);
#endif
#endif
	}
}

/// Recomputes the total width of the ruler.
void RulerPainter::computeAllocationWidth() /*throw()*/ {
	// (ruler-total-width) = (line-numbers-width) + (indicator-margin-width)
	//   (indicator-margin-width) = (indicator-margin-border-width) + (indicator-margin-content-width)
	//   (line-numbers-width) = (line-numbers-exterior-width) + (line-numbers-interior-width) + (line-numbers-content-width)
	//     (line-numbers-exterior-width) = (line-numbers-border-width) + (line-numbers-space-width)
	//     (line-numbers-interior-width) = (line-numbers-padding-start) + (line-numbers-padding-end)
	//     (line-numbers-content-width) = max((glyphs-extent), (average-glyph-extent) * (minimum-digits-setting))

	unique_ptr<RenderingContext2D> context(widgetapi::createRenderingContext(viewer_));

	// compute the width of the line numbers
	ComputedWidths computedWidths;
	if(lineNumbers(declaredStyles())->visible) {
		const uint8_t digits = computeMaximumDigitsForLineNumbers();
		if(digits != computedLineNumberDigits_)
			boost::get(computedLineNumberDigits_) = digits;
		const Scalar glyphsExtent = computeMaximumNumberGlyphsExtent(
			*context, viewer_.textRenderer().defaultFont(), digits,
			viewer_.textRenderer().writingMode(), lineNumbers(declaredStyles())->numberSubstitution.getOrInitial());
		const Scalar minimumExtent = viewer_.textRenderer().defaultFont()->metrics()->averageCharacterWidth() * digits;
		boost::get(computedWidths.lineNumbersContent) = max(glyphsExtent, minimumExtent);

		const NativeSize referenceBox(geometry::make<NativeSize>(
			computedWidths.lineNumbersContent, computedWidths.lineNumbersContent));
		boost::get(computedWidths.lineNumbersPaddingStart) = static_cast<Scalar>(
			lineNumbers(declaredStyles())->paddingStart.getOrInitial().value(context.get(), &referenceBox));
		boost::get(computedWidths.lineNumbersPaddingEnd) = static_cast<Scalar>(
			lineNumbers(declaredStyles())->paddingEnd.getOrInitial().value(context.get(), &referenceBox));
		boost::get(computedWidths.lineNumbersBorderEnd) = static_cast<Scalar>(
			lineNumbers(declaredStyles())->borderEnd.computedWidth().value(context.get(), &referenceBox));
//		const Scalar spaceWidth = 0;
//		const Scalar exteriorWidth = borderWidth + spaceWidth;
	}

	// compute the width of the indicator margin
	if(indicatorMargin(declaredStyles())->visible) {
		boost::optional<Length> contentWidth(indicatorMargin(declaredStyles())->width.getOrInitial());
		boost::get(computedWidths.indicatorMarginContent) = (contentWidth != boost::none) ?
			static_cast<Scalar>(contentWidth->value(context.get(), nullptr))
			: platformIndicatorMarginWidthInPixels(isHorizontal(viewer_.textRenderer().writingMode().blockFlowDirection));
		boost::get(computedWidths.indicatorMarginBorderEnd) = static_cast<Scalar>(
			indicatorMargin(declaredStyles())->borderEnd.computedWidth().value(context.get(),
				&geometry::make<NativeSize>(computedWidths.indicatorMarginContent, computedWidths.indicatorMarginContent)));
	}

	// commit
	const Scalar oldWidth = allocationWidth();
	computedWidths_ = computedWidths;
	if(allocationWidth() != oldWidth) {
		widgetapi::scheduleRedraw(viewer_, false);
		viewer_.caret().updateLocation();
	}
}

/// Computes the maximum number of digits of line numbers.
uint8_t RulerPainter::computeMaximumDigitsForLineNumbers() const /*noexcept*/ {
	uint8_t n = 1;
	const Index startValue = lineNumbers(declaredStyles())->startValue.getOrInitial();
	Index lines = viewer_.document().numberOfLines() + startValue - 1;
	while(lines >= 10) {
		lines /= 10;
		++n;
	}
	return static_cast<uint8_t>(n);	// hmm...
}

/// Returns the 'allocation-rectangle' of the indicator margin in the viewer-local coordinates.
NativeRectangle RulerPainter::indicatorMarginAllocationRectangle() const /*throw()*/ {
	const NativeRectangle localBounds(widgetapi::bounds(viewer_, false));
	switch(alignment()) {
		case LEFT:
			return geometry::make<NativeRectangle>(
				geometry::topLeft(localBounds),
				geometry::make<NativeSize>(indicatorMarginAllocationWidth(), geometry::dy(localBounds)));
		case TOP:
			return geometry::make<NativeRectangle>(
				geometry::topLeft(localBounds),
				geometry::make<NativeSize>(geometry::dx(localBounds), indicatorMarginAllocationWidth()));
		case RIGHT:
			return geometry::normalize(
				geometry::make<NativeRectangle>(
					geometry::topRight(localBounds),
					geometry::make<NativeSize>(-indicatorMarginAllocationWidth(), geometry::dy(localBounds))));
		case BOTTOM:
			return geometry::normalize(
				geometry::make<NativeRectangle>(
					geometry::bottomLeft(localBounds),
					geometry::make<NativeSize>(geometry::dx(localBounds), -indicatorMarginAllocationWidth())));
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}
}

/// Returns the 'allocation-rectangle' of the line numbers in the viewer-local coordinates.
NativeRectangle RulerPainter::lineNumbersAllocationRectangle() const /*throw()*/ {
	const NativeRectangle localBounds(widgetapi::bounds(viewer_, false));
	switch(alignment()) {
		case LEFT:
			return geometry::make<NativeRectangle>(
				geometry::translate(
					geometry::topLeft(localBounds), geometry::make<NativeSize>(indicatorMarginAllocationWidth(), 0)),
				geometry::make<NativeSize>(lineNumbersAllocationWidth(), geometry::dy(localBounds)));
		case TOP:
			return geometry::make<NativeRectangle>(
				geometry::translate(
					geometry::topLeft(localBounds), geometry::make<NativeSize>(0, indicatorMarginAllocationWidth())),
				geometry::make<NativeSize>(geometry::dx(localBounds), lineNumbersAllocationWidth()));
		case RIGHT:
			return geometry::normalize(
				geometry::make<NativeRectangle>(
					geometry::translate(
						geometry::topRight(localBounds), geometry::make<NativeSize>(-indicatorMarginAllocationWidth(), 0)),
					geometry::make<NativeSize>(-lineNumbersAllocationWidth(), geometry::dy(localBounds))));
		case BOTTOM:
			return geometry::normalize(
				geometry::make<NativeRectangle>(
					geometry::translate(
						geometry::bottomLeft(localBounds), geometry::make<NativeSize>(0, -indicatorMarginAllocationWidth())),				
					geometry::make<NativeSize>(geometry::dx(localBounds), -lineNumbersAllocationWidth())));
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}
}

/**
 * Paints the ruler.
 * @param context The graphics context
 */
void RulerPainter::paint(PaintContext& context) {
	if(allocationWidth() == 0)
		return;

	const NativeRectangle paintBounds(context.boundsToPaint());
	const TextRenderer& renderer = viewer_.textRenderer();
	const SnapAlignment location = alignment();
	FlowRelativeFourSides<Border::Part>::reference (FlowRelativeFourSides<Border::Part>::*borderPart)();

	const NativeRectangle indicatorMarginRectangle(indicatorMarginAllocationRectangle());
	const NativeRectangle lineNumbersRectangle(lineNumbersAllocationRectangle());
	switch(location) {
		case LEFT:
			borderPart = &FlowRelativeFourSides<Border::Part>::end;
			break;
		case TOP:
			borderPart = &FlowRelativeFourSides<Border::Part>::after;
			break;
		case RIGHT:
			borderPart = &FlowRelativeFourSides<Border::Part>::start;
			break;
		case BOTTOM:
			borderPart = &FlowRelativeFourSides<Border::Part>::before;
			break;
	}

	const bool indicatorMarginToPaint = indicatorMargin(declaredStyles())->visible
		&& !geometry::isEmpty(indicatorMarginRectangle) && geometry::intersects(indicatorMarginRectangle, paintBounds);
	const bool lineNumbersToPaint = lineNumbers(declaredStyles())->visible
		&& !geometry::isEmpty(lineNumbersRectangle) && geometry::intersects(lineNumbersRectangle, paintBounds);
	if(!indicatorMarginToPaint && !lineNumbersToPaint)
		return;

#ifdef _DEBUG
	if(DIAGNOSE_INHERENT_DRAWING)
		win32::DumpContext() << L"@RulerPainter.paint draws y = "
			<< geometry::top(paintBounds) << L" ~ " << geometry::bottom(paintBounds) << L"\n";
#endif // _DEBUG

	context.save();

	// paint the indicator margin
	if(indicatorMarginToPaint) {
		// background
		const shared_ptr<Paint> paint(indicatorMargin(declaredStyles())->paint.getOr(
			shared_ptr<Paint>(new SolidColor(SystemColors::get(SystemColors::THREE_D_FACE)))));
		context.setFillStyle(paint);
		context.fillRectangle(indicatorMarginRectangle);

		// border
		const Color currentColor(computeColor(nullptr, shared_ptr<const TextLineStyle>(), viewer_.presentation().globalTextStyle()));
		Border borderStyle;
		(borderStyle.sides.*borderPart)() = indicatorMargin(declaredStyles())->borderEnd;
		if(!(borderStyle.sides.*borderPart)().color)
			(borderStyle.sides.*borderPart)().color = SystemColors::get(SystemColors::THREE_D_SHADOW);
		detail::paintBorder(context, indicatorMarginRectangle, borderStyle, currentColor, viewer_.textRenderer().writingMode());
	}

	// paint the line numbers
	if(lineNumbersToPaint) {
		// compute foreground
		const Color currentColor(computeColor(&lineNumbers(declaredStyles())->color, shared_ptr<const TextLineStyle>(), viewer_.presentation().globalTextStyle()));

		// background and border
		shared_ptr<Paint> background(move(computeBackground(
			&lineNumbers(declaredStyles())->background, nullptr, viewer_.presentation().globalTextStyle())));
		context.setFillStyle(background);
		context.fillRectangle(lineNumbersRectangle);
		Border borderStyle;
		(borderStyle.sides.*borderPart)() = lineNumbers(declaredStyles())->borderEnd;
		detail::paintBorder(context, lineNumbersRectangle, borderStyle, currentColor, viewer_.textRenderer().writingMode());

		// text
		context.setFillStyle(shared_ptr<Paint>(new SolidColor(currentColor)));
		context.setFont(viewer_.textRenderer().defaultFont());
//		context.setTextAlign();
//		context.setTextBaseline();

		// TODO: paint glyphs.
	}

#if defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI) && 0
	Scalar left;
	HDC dcex;
	if(enablesDoubleBuffering_) {
		if(memoryDC_.get() == nullptr)
			memoryDC_.reset(::CreateCompatibleDC(win32::ClientAreaGraphicsContext(viewer_.identifier()).nativeHandle().get()), &::DeleteDC);
		if(memoryBitmap_.get() == nullptr)
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
		::MoveToEx(dcex, borderX, geometry::top(paintBounds), nullptr);
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
			::MoveToEx(dcex, x, 0/*paintRect.top*/, nullptr);
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
			const shared_ptr<const TextLineStyle> defaultLineStyle(viewer_.presentation().globalTextStyle()->defaultLineStyle);
			if(defaultLineStyle.get() != nullptr)
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
	Index line, visualSublineOffset;
	const Index lines = viewer_.document().numberOfLines();
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
#endif

	context.restore();
}

void RulerPainter::scroll(const VisualLine& from) {
}

void RulerPainter::setStyles(shared_ptr<const RulerStyles> styles) {
	if(styles.get() != nullptr) {
		if(!styles->alignment.inherits()) {
			const presentation::TextAlignment alignment = styles->alignment.get();
			if(alignment != presentation::TextAlignment::START
					&& alignment != presentation::TextAlignment::END
					&& alignment != presentation::TextAlignment::LEFT
					&& alignment != presentation::TextAlignment::RIGHT)
				throw UnknownValueException("styles->alignment");
		}
	} else
		declaredStyles_.reset(new RulerStyles);
	update();
}

void RulerPainter::update() /*throw()*/ {
	// TODO: Is this method need?
	computedLineNumberDigits_ = boost::value_initialized<uint8_t>();
	computeAllocationWidth();
#if defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI) && 0
	updateGDIObjects();
	if(enablesDoubleBuffering_ && memoryBitmap_.get() != nullptr)
		memoryBitmap_.reset();
#endif
}

#if defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI) && 0
///
void RulerPainter::updateGDIObjects() /*throw()*/ {
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
#endif
