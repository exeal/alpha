/**
 * @file text-layout.cpp
 * @author exeal
 * @date 2003-2006 (was TextLayout.cpp)
 * @date 2006-2011
 * @date 2010-11-20 renamed from ascension/layout.cpp
 * @date 2012
 */

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, ...
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/graphics/rendering-device.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/text-layout-styles.hpp>
#include <ascension/graphics/font/text-run.hpp>
//#include <ascension/graphics/special-character-renderer.hpp>
#include <ascension/corelib/shared-library.hpp>
#include <ascension/corelib/text/character-iterator.hpp>
#include <ascension/corelib/text/character-property.hpp>
#include <limits>	// std.numeric_limits
#include <numeric>	// std.accumulate
#include <boost/foreach.hpp>

using namespace ascension;
using namespace ascension::graphics;
using namespace ascension::graphics::font;
using namespace ascension::presentation;
using namespace ascension::text;
using namespace ascension::text::ucd;
using namespace std;
namespace k = ascension::kernel;

//#define TRACE_LAYOUT_CACHES
extern bool DIAGNOSE_INHERENT_DRAWING;



// graphics.font free functions ///////////////////////////////////////////////////////////////////

void font::paintTextDecoration(PaintContext& context, const TextRun& run, const Point& origin, const ComputedTextDecoration& decoration) {
	if(decoration.lines != TextDecoration::Line::NONE && !decoration.color.isFullyTransparent()) {
		// TODO: Not implemented.
	}
}


// detail.* free function /////////////////////////////////////////////////////////////////////////

/**
 * @internal Paints border.
 * @param context The graphics context
 * @param rectangle The border box. This gives the edge surrounds the border
 * @param border The presentative style
 * @param writingMode The writing mode used to compute the directions and orientation of @a border
 */
void detail::paintBorder(PaintContext& context, const graphics::Rectangle& rectangle,
		const PhysicalFourSides<ComputedBorderSide>& border, const WritingMode& writingMode) {
	// TODO: not implemented.
	for(PhysicalFourSides<ComputedBorderSide>::const_iterator side(begin(border)), e(border.cend()); side != e; ++side) {
		if(!side->hasVisibleStyle() || side->computedWidth() <= 0)
			continue;
		if(!boost::geometry::within(rectangle, context.boundsToPaint()))
			continue;
		const Color& color = side->color;
		if(color.isFullyTransparent())
			continue;
		context.setStrokeStyle(shared_ptr<Paint>(new SolidColor(color)));
		context.setLineWidth(side->width);
//		context.setStrokeDashArray();
//		context.setStrokeDashOffset();
		context.beginPath();
		switch(static_cast<PhysicalDirection>(side - begin(border))) {
			case TOP:
				context
					.moveTo(geometry::topLeft(rectangle))
					.lineTo(geometry::translate(geometry::topRight(rectangle), Dimension(geometry::_dx = 1, geometry::_dy = 0)));
				break;
			case RIGHT:
				context
					.moveTo(geometry::topRight(rectangle))
					.lineTo(geometry::translate(geometry::bottomRight(rectangle), Dimension(geometry::_dx = 0, geometry::_dy = 1)));
				break;
			case BOTTOM:
				context
					.moveTo(geometry::bottomLeft(rectangle))
					.lineTo(geometry::translate(geometry::bottomRight(rectangle), Dimension(geometry::_dx = 1, geometry::_dy = 0)));
				break;
			case LEFT:
				context
					.moveTo(geometry::topLeft(rectangle))
					.lineTo(geometry::translate(geometry::bottomLeft(rectangle), Dimension(geometry::_dx = 0, geometry::_dy = 1)));
				break;
			default:
				ASCENSION_ASSERT_NOT_REACHED();
		}
		context.stroke();
	}
}


// FixedWidthTabExpander //////////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param width The fixed width in pixels
 */
FixedWidthTabExpander::FixedWidthTabExpander(Scalar width) BOOST_NOEXCEPT : width_(width) {
}

/// @see TabExpander#nextTabStop
Scalar FixedWidthTabExpander::nextTabStop(Scalar ipd, Index) const BOOST_NOEXCEPT {
	return ipd - static_cast<long>(ipd) % static_cast<long>(width_) + width_;
}


// TextLayout /////////////////////////////////////////////////////////////////////////////////////

/**
 * @class ascension::graphics::font::TextLayout
 * @c TextLayout is an immutable graphical representation of styled text. Provides support for
 * drawing, cursor navigation, hit testing, text wrapping, etc.
 *
 * <h3>Coordinate system</h3>
 * All graphical information returned from a @c TextLayout object' method is relative to the origin
 * of @c TextLayout, which is the intersection of the start edge with the baseline of the first
 * line of @c TextLayout. The start edge is determined by the reading direction (inline progression
 * dimension) of the line. Also, coordinates passed into a @c TextLayout object's method are
 * assumed to be relative to the @c TextLayout object's origin.
 *
 * <h3>Constraints by Win32/Uniscribe</h3>
 * <del>A long run will be split into smaller runs automatically because Uniscribe rejects too long
 * text (especially @c ScriptShape and @c ScriptTextOut). For this reason, a combining character
 * will be rendered incorrectly if it is presented at the boundary. The maximum length of a run is
 * 1024.
 *
 * In present, this class supports only text layout horizontal against the output device.
 *
 * @note This class is not intended to be derived.
 * @see TextLayoutBuffer#lineLayout, TextLayoutBuffer#lineLayoutIfCached
 */

/// Destructor.
TextLayout::~TextLayout() BOOST_NOEXCEPT {
//	for(size_t i = 0; i < numberOfRuns_; ++i)
//		delete runs_[i];
//	for(vector<const InlineArea*>::const_iterator i(inlineAreas_.begin()), e(inlineAreas_.end()); i != e; ++i)
//		delete *i;
	assert(numberOfLines() != 1 || firstRunsInLines_.get() == nullptr);
//	for(size_t i = 0; i < numberOfLines(); ++i)
//		delete lineMetrics_[i];
}
#if 0
/**
 * Returns the computed text alignment of the line. The returned value may be
 * @c presentation#ALIGN_START or @c presentation#ALIGN_END.
 * @see #readingDirection, presentation#resolveTextAlignment
 */
TextAlignment TextLayout::alignment() const /*throw()*/ {
	if(style_.get() != nullptr && style_->readingDirection != INHERIT_TEXT_ALIGNMENT)
		style_->readingDirection;
	shared_ptr<const TextLineStyle> defaultStyle(lip_.presentation().defaultTextLineStyle());
	return (defaultStyle.get() != nullptr
		&& defaultStyle->alignment != INHERIT_TEXT_ALIGNMENT) ? defaultStyle->alignment : ASCENSION_DEFAULT_TEXT_ALIGNMENT;
}
#endif
/**
 * Returns the computed text anchor of the specified visual line.
 * @param line The visual line number
 * @return The computed text anchor value
 * @throw kernel#BadPositionException @a line is greater than the number of lines
 */
TextAnchor TextLayout::anchor(Index line) const {
	if(line >= numberOfLines())
		throw kernel::BadPositionException(kernel::Position(line, 0));
	return TextAnchor::START;	// TODO: Not implemented.
}

/**
 * Returns distance from the baseline of the first line to the baseline of the
 * specified line in pixels.
 * @param line The line number
 * @return The baseline position 
 * @throw kernel#BadPositionException @a line is greater than the number of lines
 */
Scalar TextLayout::baseline(Index line) const {
	if(line >= numberOfLines())
		throw kernel::BadPositionException(kernel::Position(line, 0));
	else if(line == 0)
		return 0;
	Scalar result = 0;
	for(Index i = 1; i <= line; ++i) {
		const LineMetrics& preceding = lineMetrics(i - 1);
		result += preceding.descent + preceding.leading;
		result += lineMetrics(i).ascent;
	}
	return result;
}

/**
 * Returns the smallest rectangle emcompasses the whole text of the line. It might not coincide
 * exactly the ascent, descent or overhangs of the text.
 * @return The size of the bounds
 * @see #blackBoxBounds, #bounds(const Range&lt;Index&gt;&amp;), #lineBounds
 */
FlowRelativeFourSides<Scalar> TextLayout::bounds() const BOOST_NOEXCEPT {
	// TODO: this implementation can't handle vertical text.
	FlowRelativeFourSides<Scalar> result;
	result.before() = /*-lineMetrics(0).leading()*/ - lineMetrics(0).ascent;
	result.after() = result.before();
	result.start() = numeric_limits<Scalar>::max();
	result.end() = numeric_limits<Scalar>::min();
	for(Index line = 0; line < numberOfLines(); ++line) {
		const LineMetrics& lm = lineMetrics(line);
		result.after() += lm.ascent + lm.descent + lm.leading;
		const Scalar lineStart = lineStartEdge(line);
		result.start() = min(lineStart, result.start());
		result.end() = max(lineStart + measure(line), result.end());
	}
	return result;
}

/**
 * Returns the bidirectional embedding level at specified position.
 * @param offset The offset in this layout
 * @return The embedding level
 * @throw kernel#BadPositionException @a offset is greater than the length of the layout
 */
uint8_t TextLayout::characterLevel(Index offset) const {
	if(isEmpty()) {
		if(offset != 0)
			throw kernel::BadPositionException(kernel::Position(0, offset));
		// use the default level
		return (writingMode().inlineFlowDirection == RIGHT_TO_LEFT) ? 1 : 0;
	}
	const auto run(runForPosition(offset));
	if(run == end(runs_))
		throw kernel::BadPositionException(kernel::Position(0, offset));
	return (*run)->characterLevel();
}

shared_ptr<const Font> TextLayout::findMatchingFont(const StringPiece& textRun,
		const FontCollection& collection, const ComputedFontSpecification& specification) {
#if 0
	void resolveFontSpecifications(const FontCollection& fontCollection,
			shared_ptr<const TextRunStyle> requestedStyle, shared_ptr<const TextRunStyle> defaultStyle,
			FontDescription* computedDescription, double* computedSizeAdjust) {
		// family name
		if(computedDescription != nullptr) {
			String familyName = (requestedStyle.get() != nullptr) ? requestedStyle->fontFamily.getOrInitial() : String();
			if(familyName.empty()) {
				if(defaultStyle.get() != nullptr)
					familyName = defaultStyle->fontFamily.getOrInitial();
				if(computedFamilyName->empty())
					*computedFamilyName = fontCollection.lastResortFallback(FontDescription())->familyName();
			}
			computedDescription->setFamilyName();
		}
		// size
		if(computedPixelSize != nullptr) {
			requestedStyle->fontProperties
		}
		// properties
		if(computedProperties != 0) {
			FontProperties<Inheritable> result;
			if(requestedStyle.get() != nullptr)
				result = requestedStyle->fontProperties;
			Inheritable<double> computedSize(computedProperties->pixelSize());
			if(computedSize.inherits()) {
				if(defaultStyle.get() != nullptr)
					computedSize = defaultStyle->fontProperties.pixelSize();
				if(computedSize.inherits())
					computedSize = fontCollection.lastResortFallback(FontProperties<>())->metrics().emHeight();
			}
			*computedProperties = FontProperties<>(
				!result.weight().inherits() ? result.weight()
					: ((defaultStyle.get() != nullptr) ? defaultStyle->fontProperties.weight() : FontPropertiesBase::NORMAL_WEIGHT),
				!result.stretch().inherits() ? result.stretch()
					: ((defaultStyle.get() != nullptr) ? defaultStyle->fontProperties.stretch() : FontPropertiesBase::NORMAL_STRETCH),
				!result.style().inherits() ? result.style()
					: ((defaultStyle.get() != nullptr) ? defaultStyle->fontProperties.style() : FontPropertiesBase::NORMAL_STYLE),
				!result.variant().inherits() ? result.variant()
					: ((defaultStyle.get() != nullptr) ? defaultStyle->fontProperties.variant() : FontPropertiesBase::NORMAL_VARIANT),
				!result.orientation().inherits() ? result.orientation()
					: ((defaultStyle.get() != nullptr) ? defaultStyle->fontProperties.orientation() : FontPropertiesBase::HORIZONTAL),
				computedSize);
		}
		// size-adjust
		if(computedSizeAdjust != nullptr) {
			*computedSizeAdjust = (requestedStyle.get() != nullptr) ? requestedStyle->fontSizeAdjust : -1.0;
			if(*computedSizeAdjust < 0.0)
				*computedSizeAdjust = (defaultStyle.get() != nullptr) ? defaultStyle->fontSizeAdjust : 0.0;
		}
	}
#endif
}

/**
 * Returns a @c TextHit corresponding to the specified point. This method is a convenience overload
 * of @c #hitTestCharacter that uses the natural bounds of this @c TextLayout.
 * @param point The abstract point
 * @param[out] outOfBounds @c true if @a point is out of the logical bounds of the @c TextLayout.
 *                         Can be @c null if not needed
 * @return A hit describing the character and edge (leading or trailing) under the specified point
 */
TextHit&& TextLayout::hitTestCharacter(const AbstractTwoAxes<Scalar>& point, bool* outOfBounds /* = nullptr */) const {
}

/**
 * Returns a @c TextHit corresponding to the specified point. Coordinates outside the bounds of the
 * @c TextLayout map to hits on the leading edge of the first logical character, or the trailing
 * edge of the last logical character, as appropriate, regardless of the position of that character
 * in the line. Only the direction along the baseline is used to make this evaluation.
 * @param point The abstract point
 * @param bounds The bounds of the @c TextLayout
 * @param[out] outOfBounds @c true if @a point is out of the logical bounds of the @c TextLayout.
 *                         Can be @c null if not needed
 * @return A hit describing the character and edge (leading or trailing) under the specified point
 */
TextHit&& TextLayout::hitTestCharacter(const AbstractTwoAxes<Scalar>& point, const FlowRelativeFourSides<Scalar>& bounds, bool* outOfBounds /* = nullptr */) const {
}

/**
 * Converts a hit to a point in abstract coordinates.
 * @param hit The hit to check. This must be a valid hit on the @c TextLayout
 * @return The returned point. The point is in abstract coordinates
 * @throw std#invalid_argument @a hit is not valid for the @c TextLayout
 */
AbstractTwoAxes<Scalar> TextLayout::hitToPoint(const TextHit& hit) const {
}
#if 0
/// Returns an iterator addresses the first styled segment.
TextLayout::StyledSegmentIterator TextLayout::firstStyledSegment() const /*throw()*/ {
	const TextRun* temp = *runs_;
	return StyledSegmentIterator(temp);
}
#endif
/**
 * Returns if the line contains right-to-left run.
 * @note This method's semantics seems to be strange. Is containning RTL run means bidi?
 */
bool TextLayout::isBidirectional() const BOOST_NOEXCEPT {
	if(writingMode().inlineFlowDirection == RIGHT_TO_LEFT)
		return true;
	BOOST_FOREACH(const RunVector::const_iterator i, runs_) {
		if((*i)->direction() == RIGHT_TO_LEFT)
			return true;
	}
	return false;
}
#if 0
/// Returns an iterator addresses the last styled segment.
TextLayout::StyledSegmentIterator TextLayout::lastStyledSegment() const /*throw()*/ {
	const TextRun* temp = runs_[numberOfRuns_];
	return StyledSegmentIterator(temp);
}
#endif
/**
 * Returns the smallest rectangle emcompasses the specified line. It might not coincide exactly the
 * ascent, descent or overhangs of the specified line.
 * @param line The line number
 * @return The line bounds in pixels
 * @throw IndexOutOfBoundsException @a line is greater than the number of the lines
 * @see #basline, #lineStartEdge, #measure
 */
FlowRelativeFourSides<Scalar> TextLayout::lineBounds(Index line) const {
	if(line >= numberOfLines())
		throw IndexOutOfBoundsException("line");

	FlowRelativeFourSides<Scalar> sides;
	sides.start() = lineStartEdge(line);
	sides.end() = sides.start() + measure(line);
	const LineMetrics& lm = lineMetrics(line);
	const float bsln = baseline(line);
	sides.before() = bsln - lm.ascent/* - lm.leading*/;
	sides.after() = bsln + lm.descent + lm.leading;
	return sides;
/*
	// TODO: this implementation can't handle vertical text.
	const NativeSize size(geometry::make<NativeSize>(end - start, after - before));
	const NativePoint origin(geometry::make<NativePoint>(
		(writingMode().inlineFlowDirection == LEFT_TO_RIGHT) ? start : start - geometry::dx(size), before));
	return geometry::make<NativeRectangle>(origin, size);
*/
}

/**
 * Returns the start-edge of the specified line without the start-indent in pixels.
 * @par This is distance from the start-edge of the first line to the one of @a line in
 * inline-progression-dimension. Therefore, returns always zero when @a line is zero or the anchor
 * is @c TEXT_ANCHOR_START.
 * @par A positive value means positive indentation. For example, if the start-edge of a RTL line
 * is x = -10, this method returns +10.
 * @param line The line number
 * @return The start-indentation in pixels
 * @throw IndexOutOfBoundsException @a line is invalid
 * @see TextRenderer#lineStartEdge
 */
Scalar TextLayout::lineStartEdge(Index line) const {
	if(line == 0)
		return 0;
	switch(anchor(line)) {
	case TextAnchor::START:
		return 0;
	case TextAnchor::MIDDLE:
		return (measure(0) - measure(line)) / 2;
	case TextAnchor::END:
		return measure(0) - measure(line);
	default:
		ASCENSION_ASSERT_NOT_REACHED();
	}
}

/**
 * Converts a position in the block-progression-direction into the corresponding line.
 * @param bpd The position in block-progression-dimension in pixels
 * @param[out] outside @c true if @a bpd is outside of the line content
 * @return The line number
 * @see #basline, #lineAt, #offset
 */
Index TextLayout::locateLine(Scalar bpd, bool& outside) const BOOST_NOEXCEPT {
	// TODO: This implementation can't handle tricky 'text-orientation'.

	// beyond the before-edge ?
	if(bpd < -lineMetrics(0).ascent/* - lineMetrics(0).leading*/)
		return (outside = true), 0;

	Index line = 0;
	for(Scalar lineAfter = 0; line < numberOfLines() - 1; ++line) {
		const LineMetrics& lm = lineMetrics(line);
		if(bpd < (lineAfter += lm.ascent + lm.descent + lm.leading))
			return (outside = false), line;
	}

	// beyond the after-edge
	return (outside = true), numberOfLines() - 1;
}

/**
 * @internal Converts an inline-progression-dimension into character offset(s) in the line.
 * @param line The line number
 * @param ipd The inline-progression-dimension
 * @param[out] outside @c true if @a ipd is outside of the line content
 * @return See the documentation of @c #offset method
 * @throw IndexOutOfBoundsException @a line is invalid
 */
pair<Index, Index> TextLayout::locateOffsets(Index line, Scalar ipd, bool& outside) const {
	if(isEmpty())
		return (outside = true), make_pair(static_cast<Index>(0), static_cast<Index>(0));
	const size_t lastRun = (line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_;

	if(writingMode().inlineFlowDirection == LEFT_TO_RIGHT) {
		Scalar x = lineStartEdge(line);
		if(ipd < x) {	// beyond the left-edge => the start of the first run
			const Index offsetInLine = runs_[lineFirstRuns_[line]]->beginning();
			return (outside = true), make_pair(offsetInLine, offsetInLine);
		}
		for(size_t i = lineFirstRuns_[line]; i < lastRun; ++i) {	// scan left to right
			const TextRun& run = *runs_[i];
			if(ipd >= x && ipd <= x + run.totalWidth()) {
				int cp, trailing;
				run.hitTest(ipd - x, cp, trailing);	// TODO: check the returned value.
				const Index temp = run.beginning() + static_cast<Index>(cp);
				return (outside = false), make_pair(temp, temp + static_cast<Index>(trailing));
			}
			x += run.totalWidth();
		}
		// beyond the right-edge => the end of last run
		const Index offsetInLine = runs_[lastRun - 1]->end();
		return (outside = true), make_pair(offsetInLine, offsetInLine);
	} else {
		Scalar x = -lineStartEdge(line);
		if(ipd > x) {	// beyond the right-edge => the start of the last run
			const Index offsetInLine = runs_[lastRun - 1]->beginning();
			return (outside = true), make_pair(offsetInLine, offsetInLine);
		}
		// beyond the left-edge => the end of the first run
		const Index offsetInLine = runs_[lineFirstRuns_[line]]->end();
		return (outside = true), make_pair(offsetInLine, offsetInLine);
	}
}

// implements public location methods
void TextLayout::locations(Index offsetInLine, NativePoint* leading, NativePoint* trailing) const {
	assert(leading != nullptr || trailing != nullptr);
	if(offsetInLine > text_.length())
		throw kernel::BadPositionException(kernel::Position(0, offsetInLine));

	Scalar leadingIpd, trailingIpd, bpd = lineMetrics_[0]->ascent()/* + lineMetrics_[0]->leading()*/;
	if(isEmpty())
		leadingIpd = trailingIpd = 0;
	else {
		// inline-progression-dimension
		const Index line = lineAt(offsetInLine);
		const Index firstRun = lineFirstRuns_[line];
		const Index lastRun = (line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_;
		if(writingMode().inlineFlowDirection == LEFT_TO_RIGHT) {	// LTR
			Scalar x = lineStartEdge(line);
			for(size_t i = firstRun; i < lastRun; ++i) {
				const TextRun& run = *runs_[i];
				if(offsetInLine >= run.beginning() && offsetInLine <= run.end()) {
					if(leading != nullptr)
						leadingIpd = x + run.x(offsetInLine, false);
					if(trailing != nullptr)
						trailingIpd = x + run.x(offsetInLine, true);
					break;
				}
				x += run.totalWidth();
			}
		} else {	// RTL
			Scalar x = -lineStartEdge(line);
			for(size_t i = lastRun - 1; ; --i) {
				const TextRun& run = *runs_[i];
				x -= run.totalWidth();
				if(offsetInLine >= run.beginning() && offsetInLine <= run.end()) {
					if(leading != nullptr)
						leadingIpd = -(x + run.x(offsetInLine, false));
					if(trailing != nullptr)
						trailingIpd = -(x + run.x(offsetInLine, true));
					break;
				}
				if(i == firstRun) {
					ASCENSION_ASSERT_NOT_REACHED();
					break;
				}
			}
		}

		// block-progression-dimension
		bpd += baseline(line);
	}
		
	// TODO: this implementation can't handle vertical text.
	if(leading != nullptr) {
		leading->x = leadingIpd;
		leading->y = bpd;
	}
	if(trailing != nullptr) {
		trailing->x = trailingIpd;
		trailing->y = bpd;
	}
}

/**
 * Returns the inline-progression-dimension of the longest line.
 * @see #measure(Index)
 */
Scalar TextLayout::measure() const BOOST_NOEXCEPT {
	if(!maximumMeasure_) {
		Scalar ipd = 0;
		for(Index line = 0; line < numberOfLines(); ++line)
			ipd = max(measure(line), ipd);
		const_cast<TextLayout*>(this)->maximumMeasure_ = ipd;
	}
	return boost::get(maximumMeasure_);
}

/**
 * Returns the length in inline-progression-dimension without the indentations (the distance from
 * the start-edge to the end-edge) of the specified line in pixels.
 * @param line The line number
 * @return The width. Must be equal to or greater than zero
 * @throw IndexOutOfBoundsException @a line is greater than the number of lines
 * @see #measure(void)
 */
Scalar TextLayout::measure(Index line) const {
	if(line >= numberOfLines())
		throw IndexOutOfBoundsException("line");
	else if(isEmpty())
		return boost::get(const_cast<TextLayout*>(this)->maximumMeasure_ = 0);
	else {
		TextLayout& self = const_cast<TextLayout&>(*this);
		if(numberOfLines() == 1) {
			if(maximumMeasure_)
				return boost::get(maximumMeasure_);
		} else {
			static_assert(is_signed<Scalar>::value, "");
			if(lineMeasures_.get() == nullptr) {
				self.lineMeasures_.reset(new Scalar[numberOfLines()]);
				fill_n(self.lineMeasures_.get(), numberOfLines(), -1);
			}
			if(lineMeasures_[line] >= 0)
				return lineMeasures_[line];
		}
		const RunVector::const_iterator lastRun(firstRunInLine(line + 1));
		Scalar ipd = 0;
		for(RunVector::const_iterator run(firstRunInLine(line)); run != lastRun; ++run)
			ipd += allocationMeasure(**run);
		assert(ipd >= 0);
		if(numberOfLines() == 1)
			self.maximumMeasure_ = ipd;
		else
			self.lineMeasures_[line] = ipd;
		return ipd;
	}
}

/**
 * Returns the hit test information corresponding to the specified point.
 * @param p The point
 * @param[out] outside @c true if the specified point is outside of the layout
 * @return A pair of the character offsets. The first element addresses the character whose black
 *         box (bounding box) encompasses the specified point. The second element addresses the
 *         character whose leading point is the closest to the specified point in the line
 * @see #locateLine, #location
 */
pair<Index, Index> TextLayout::offset(const NativePoint& p, bool* outside /* = nullptr */) const /*throw()*/ {
	const bool vertical = isVertical(writingMode().blockFlowDirection);
	bool outsides[2];
	const std::pair<Index, Index> result(locateOffsets(locateLine(
		vertical ? geometry::x(p) : geometry::y(p), outsides[0]), vertical ? geometry::y(p) : geometry::x(p), outsides[1]));
	if(outside != nullptr)
		*outside = outsides[0] | outsides[1];
	return result;
}
#if 0
/**
 * Returns the next tab stop position.
 * @param x the distance from leading edge of the line (can not be negative)
 * @param direction the direction
 * @return the distance from leading edge of the line to the next tab position
 */
inline int TextLayout::nextTabStop(int x, Direction direction) const /*throw()*/ {
	assert(x >= 0);
	const int tabWidth = lip_.textMetrics().averageCharacterWidth() * lip_.layoutSettings().tabWidth;
	return (direction == Direction::FORWARD) ? x + tabWidth - x % tabWidth : x - x % tabWidth;
}

/**
 * Returns the next tab stop.
 * @param x the distance from the left edge of the line to base position (can not be negative)
 * @param right @c true to find the next right position
 * @return the tab stop position in pixel
 */
int TextLayout::nextTabStopBasedLeftEdge(int x, bool right) const /*throw()*/ {
	assert(x >= 0);
	const LayoutSettings& c = lip_.layoutSettings();
	const int tabWidth = lip_.textMetrics().averageCharacterWidth() * c.tabWidth;
	if(lineTerminatorOrientation(style(), lip_.presentation().defaultTextLineStyle()) == LEFT_TO_RIGHT)
		return nextTabStop(x, right ? Direction::FORWARD : Direction::BACKWARD);
	else
		return right ? x + (x - longestLineWidth()) % tabWidth : x - (tabWidth - (x - longestLineWidth()) % tabWidth);
}
#endif

#if 0
/**
 * Returns the computed reading direction of the line.
 * @see #alignment
 */
ReadingDirection TextLayout::readingDirection() const /*throw()*/ {
	ReadingDirection result = INHERIT_READING_DIRECTION;
	// try the requested line style
	if(style_.get() != nullptr)
		result = style_->readingDirection;
	// try the default line style
	if(result == INHERIT_READING_DIRECTION) {
		shared_ptr<const TextLineStyle> defaultLineStyle(lip_.presentation().defaultTextLineStyle());
		if(defaultLineStyle.get() != nullptr)
			result = defaultLineStyle->readingDirection;
	}
	// try the default UI style
	if(result == INHERIT_READING_DIRECTION)
		result = lip_.defaultUIReadingDirection();
	// use user default
	if(result == INHERIT_READING_DIRECTION)
		result = ASCENSION_DEFAULT_TEXT_READING_DIRECTION;
	assert(result == LEFT_TO_RIGHT || result == RIGHT_TO_LEFT);
	return result;
}
#endif

#if 0
/**
 * Returns the styled text run containing the specified offset in the line.
 * @param offsetInLine The offset in the line
 * @return the styled segment
 * @throw kernel#BadPositionException @a offsetInLine is greater than the length of the line
 */
StyledRun TextLayout::styledTextRun(Index offsetInLine) const {
	if(offsetInLine > text().length())
		throw kernel::BadPositionException(kernel::Position(INVALID_INDEX, offsetInLine));
	const TextRun& run = *runs_[findRunForPosition(offsetInLine)];
	return StyledRun(run.offsetInLine(), run.requestedStyle());
}
#endif

#if 0
// TextLayout.StyledSegmentIterator ///////////////////////////////////////////////////////////////

/**
 * Private constructor.
 * @param start
 */
TextLayout::StyledSegmentIterator::StyledSegmentIterator(const TextRun*& start) /*throw()*/ : p_(&start) {
}

/// Copy-constructor.
TextLayout::StyledSegmentIterator::StyledSegmentIterator(const StyledSegmentIterator& rhs) /*throw()*/ : p_(rhs.p_) {
}

/// Returns the current segment.
StyledRun TextLayout::StyledSegmentIterator::current() const /*throw()*/ {
	const TextRun& run = **p_;
	return StyledRun(run.offsetInLine, run.style);
}
#endif
