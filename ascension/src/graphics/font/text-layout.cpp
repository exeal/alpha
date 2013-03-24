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
#include <ascension/graphics/font/font-metrics.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/text-layout-styles.hpp>
#include <ascension/graphics/font/text-run.hpp>
//#include <ascension/graphics/special-character-renderer.hpp>
#include <ascension/corelib/range.hpp>
#include <ascension/corelib/shared-library.hpp>
#include <ascension/corelib/text/character-iterator.hpp>
#include <ascension/corelib/text/character-property.hpp>
#include <ascension/presentation/writing-mode-mappings.hpp>
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
			case PhysicalDirection::TOP:
				context
					.moveTo(geometry::topLeft(rectangle))
					.lineTo(geometry::translate(geometry::topRight(rectangle), Dimension(geometry::_dx = 1, geometry::_dy = 0)));
				break;
			case PhysicalDirection::RIGHT:
				context
					.moveTo(geometry::topRight(rectangle))
					.lineTo(geometry::translate(geometry::bottomRight(rectangle), Dimension(geometry::_dx = 0, geometry::_dy = 1)));
				break;
			case PhysicalDirection::BOTTOM:
				context
					.moveTo(geometry::bottomLeft(rectangle))
					.lineTo(geometry::translate(geometry::bottomRight(rectangle), Dimension(geometry::_dx = 1, geometry::_dy = 0)));
				break;
			case PhysicalDirection::LEFT:
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

namespace {
	// Returns distance from line-left edge of allocation-rectangle to one of content-rectangle.
	inline Scalar lineRelativeAllocationOffset(const TextRun& textRun, const WritingMode& writingMode) {
	}
}

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
 * @throw IndexOutOfBoundsException @a line &gt;= @c #numberOfLines()
 */
TextAnchor TextLayout::anchor(Index line) const {
	if(line >= numberOfLines())
		throw IndexOutOfBoundsException("line");
	return TextAnchor::START;	// TODO: Not implemented.
}

/**
 * Returns the smallest rectangle emcompasses the whole text of the line. It might not coincide
 * exactly the ascent, descent, origin or advance of the @c TextLayout.
 * @return The size of the bounds
 * @see #blackBoxBounds, #bounds(Index)
 */
FlowRelativeFourSides<Scalar> TextLayout::bounds() const BOOST_NOEXCEPT {
	if(numberOfLines() <= 1)
		return bounds(0);
	FlowRelativeFourSides<Scalar> sides;
	sides.before() = bounds(0).before();
	sides.after() = bounds(numberOfLines() - 1).after();
	sides.start() = numeric_limits<Scalar>::max();
	sides.end() = numeric_limits<Scalar>::min();
	for(Index line = 0; line < numberOfLines(); ++line) {
		const Scalar lineStart = lineStartEdge(line);
		sides.start() = min(lineStart, sides.start());
		sides.end() = max(lineStart + measure(line), sides.end());
	}
	return sides;
}

/**
 * Returns the bounds of the specified line in flow-relative coordinates. It might not coincide
 * exactly the ascent, descent, origin or advance of the @c TextLayout.
 * @param line The line number
 * @return The line bounds in user units
 * @throw IndexOutOfBoundsException @a line &gt;= @c #numberOfLines()
 * @see #basline, #lineStartEdge, #measure
 */
FlowRelativeFourSides<Scalar> TextLayout::bounds(Index line) const {
	Scalar over = numeric_limits<Scalar>::min(), under = numeric_limits<Scalar>::min();
	BOOST_FOREACH(const unique_ptr<const TextRun>& run, runsForLine(line)) {	// may throw IndexOutOfBoundsException
		const graphics::Rectangle runVisualBounds(run->visualBounds());
		over = max(-geometry::top(runVisualBounds), over);
		under = max(geometry::bottom(runVisualBounds), under);
	}

	FlowRelativeFourSides<Scalar> sides;
	sides.start() = lineStartEdge(line);
	sides.end() = sides.start() + measure(line);
	const LineMetricsIterator lm(lineMetrics(line));
	if(isHorizontal(writingMode().blockFlowDirection) || resolveTextOrientation(writingMode()) != SIDEWAYS_LEFT) {
		sides.before() = lm.baselineOffset() - over;
		sides.after() = lm.baselineOffset() + under;
	} else {
		sides.before() = lm.baselineOffset() - under;
		sides.after() = lm.baselineOffset() + over;
	}
	return sides;
}

/**
 * Returns the bidirectional embedding level of the character at the specified offset.
 * @param offset The index of the character from which to get the bidirectional embedding level
 * @return The bidirectional embedding level of the character at the specified offset
 * @throw IndexOutOfBoundsException @a offset &gt;= @c #numberOfCharacters()
 */
uint8_t TextLayout::characterLevel(Index offset) const {
	if(offset >= numberOfCharacters())
		throw IndexOutOfBoundsException("offset");
	const auto run(runForPosition(offset));
	if(run == end(runs_))
		throw IndexOutOfBoundsException("offset");
	return (*run)->characterLevel();
}

#ifdef _DEBUG
/**
 * Dumps the all runs to the specified output stream.
 * @param out The output stream
 */
void TextLayout::dumpRuns(ostream& out) const {
	const String::const_pointer backingStore = textString_.data();
	size_t i = 0;
	BOOST_FOREACH(const unique_ptr<const TextRun>& run, runs_) {
		out << i++
			<< ": [" << static_cast<unsigned int>(run->characterRange().begin() - backingStore)
			<< "," << static_cast<unsigned int>(run->characterRange().end() - backingStore) << ")" << endl;
	}
}
#endif // _DEBUG

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
	return internalHitTestCharacter(point, nullptr, outOfBounds);
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
	return internalHitTestCharacter(point, &bounds, outOfBounds);
}

/**
 * @fn ascension::graphics::font::TextLayout::hitToPoint
 * Converts a hit to a point in abstract coordinates.
 * @param hit The hit to check. This must be a valid hit on the @c TextLayout
 * @return The returned point. The point is in abstract coordinates
 * @throw std#out_of_range @a hit is not valid for the @c TextLayout
 */

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
	BOOST_FOREACH(const unique_ptr<const TextRun>& run, runs_) {
		if(run->direction() == RIGHT_TO_LEFT)
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
 * @internal Returns 'line-left' of the specified line.
 * @param line The line number
 * @return
 */
Point TextLayout::lineLeft(Index line) const {
	if(isHorizontal(writingMode().blockFlowDirection)) {
		if(writingMode().inlineFlowDirection == LEFT_TO_RIGHT)
			return Point(geometry::_x = lineStartEdge(line), geometry::_y = 0);
		else
			return Point(geometry::_x = -lineStartEdge(line) - measure(line), geometry::_y = 0);
	} else {
		Scalar y = -lineStartEdge(line);
		if(writingMode().inlineFlowDirection == RIGHT_TO_LEFT)
			y -= measure(line);
		if(resolveTextOrientation(writingMode()) == SIDEWAYS_LEFT)
			y = -y;
		return Point(geometry::_x = 0, geometry::_y = y);
/*		if(writingMode().inlineFlowDirection == LEFT_TO_RIGHT) {
			if(resolveTextOrientation(writingMode()) != SIDEWAYS_LEFT)
				return Point(geometry::_x = 0, geometry::_y = lineStartEdge(line));
			else
				return Point(geometry::_x = 0, geometry::_y = -lineStartEdge(line));
		} else {
			if(resolveTextOrientation(writingMode()) != SIDEWAYS_LEFT)
				return Point(geometry::_x = 0, geometry::_y = -lineStartEdge(line) - measure(line));
			else
				return Point(geometry::_x = 0, geometry::_y = lineStartEdge(line) + measure(line));
		}
*/	}
}

/**
 * Returns the offset for the first character in the specified line.
 * @param line The visual line number
 * @return The offset
 * @throw IndexOutOfBoundsException @a line &gt;= @c #numberOfLines()
 * @see #lineOffsets
 * @note Designed based on @c org.eclipse.swt.graphics.TextLayout.lineOffsets method in Eclipse.
 */
Index TextLayout::lineOffset(Index line) const {
	if(line >= numberOfLines())
		throw IndexOutOfBoundsException("line");
	return (*firstRunInLine(line))->characterRange().begin() - textString_.data();
}

/**
 * Returns the line offsets. Each value in the vector is the offset for the first character in a
 * line except for the last value, which contains the length of the text.
 * @return The line offsets whose length is @c #numberOfLines(). Each element in the
 *         array is the offset for the first character in a line
 * @note Designed based on @c org.eclipse.swt.graphics.TextLayout.lineOffsets method in Eclipse.
 */
vector<Index>&& TextLayout::lineOffsets() const BOOST_NOEXCEPT {
	const String::const_pointer bol = textString_.data();
	vector<Index> offsets;
	offsets.reserve(numberOfLines() + 1);
	for(Index line = 0; line < numberOfLines(); ++line)
		offsets.push_back((*firstRunInLine(line))->characterRange().begin() - bol);
	offsets.push_back(numberOfCharacters());
	return move(offsets);
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
	if(bpd < -get<0>(lineMetrics_[0])/* - get<2>(lineMetrics_[0])*/)
		return (outside = true), 0;

	LineMetricsIterator line(lineMetrics(0));
	for(Scalar lineAfter = 0; line.line() < numberOfLines() - 1; ++line) {
		if(bpd < (lineAfter += line.ascent() + line.descent() + line.leading()))
			return (outside = false), line.line();
	}

	// beyond the after-edge
	return (outside = true), numberOfLines() - 1;
}

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
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

	const boost::iterator_range<const RunVector::const_iterator> runsInLine(firstRunInLine(line), firstRunInLine(line + 1));
	const StringPiece characterRangeInLine(static_cast<const TextRunImpl*>(runsInLine.begin()->get())->begin(), lineLength(line));
	assert(characterRangeInLine.end() == static_cast<const TextRunImpl*>((runsInLine.end() - 1)->get())->end());

	const Scalar lineStart = lineStartEdge(line);
	if(ipd < lineStart) {
		const Index offset = characterRangeInLine.begin() - textString_.data();
		return (outside = true), make_pair(offset, offset);
	}
	outside = ipd > lineStart + measure(line);	// beyond line 'end-edge'

	if(!outside) {
		Scalar x = ipd - lineStart, dx = 0;
		if(writingMode().inlineFlowDirection == RIGHT_TO_LEFT)
			x = measure(line) - x;
		for(RunVector::const_iterator run(runsInLine.begin()); run != runsInLine.end(); ++run) {
			const Scalar nextDx = dx + allocationMeasure(**run);
			if(nextDx >= x) {
				const Scalar ipdInRun = ((*run)->direction() == LEFT_TO_RIGHT) ? x - dx : nextDx - x;
				return make_pair(boost::get((*run)->characterEncompassesPosition(ipdInRun)), (*run)->characterHasClosestLeadingEdge(ipdInRun));
			}
		}
	}

	// maybe beyond line 'end-edge'
	const Index offset = characterRangeInLine.end() - textString_.data();
	return make_pair(offset, offset);
}
#endif	// ASCENSION_ABANDONED_AT_VERSION_08

/**
 * Returns a multi-polygon enclosing the logical selection in the specified range, extended to the
 * specified bounds.
 * @param range The range of characters to select
 * @param bounds The bounding rectangle to which to extend the selection, in user units. If this is
 *               @c boost#none, the natural bounds is used
 * @return An area enclosing the selection, in user units
 * @throw IndexOutOfBoundsException
 * @see #logicalRangesForVisualSelection, #visualHighlightShape
 */
boost::geometry::model::multi_polygon<boost::geometry::model::polygon<Point>>&& TextLayout::logicalHighlightShape(
		const boost::integer_range<Index>& range, const boost::optional<graphics::Rectangle>& bounds) const {
	boost::geometry::model::multi_polygon<boost::geometry::model::polygon<Point>> results;
	const bool horizontal = isHorizontal(writingMode().blockFlowDirection);
	boost::optional<boost::integer_range<Scalar>> linearBounds;
	if(bounds != boost::none)
		linearBounds = horizontal ? geometry::range<0>(*bounds) : geometry::range<1>(*bounds);

	const boost::integer_range<Index> orderedRange(ordered(range));
	const boost::integer_range<Index> lines(boost::irange(lineAt(*orderedRange.begin()), lineAt(*orderedRange.end())));
	for(LineMetricsIterator line(*this, *lines.begin()); line.line() != *lines.end(); ++line) {
		const Point baseline(line.baselineOffsetInPhysicalCoordinates());
		Scalar lineOver, lineUnder;
		if(horizontal) {
			lineOver = geometry::y(baseline) - line.ascent();
			lineUnder = geometry::y(baseline) + line.descent() + line.leading();
		} else {
			const bool sidewaysLeft = resolveTextOrientation(writingMode()) == SIDEWAYS_LEFT;
			lineOver = geometry::x(baseline) + (!sidewaysLeft ? line.ascent() : -line.ascent());
			lineUnder = geometry::x(baseline) - (!sidewaysLeft ? (line.descent() + line.leading()) : -(line.descent() + line.leading()));
		}

		// skip the line if out of bounds
		if(bounds != boost::none) {
			if(horizontal) {
				if(max(lineOver, lineUnder) <= geometry::top(*bounds))
					continue;
				else if(min(lineOver, lineUnder) >= geometry::bottom(*bounds))
					break;
			} else {
				if(writingMode().blockFlowDirection == VERTICAL_RL) {
					if(min(lineOver, lineUnder) >= geometry::right(*bounds))
						continue;
					else if(max(lineOver, lineUnder) <= geometry::left(*bounds))
						break;
				} else {
					if(max(lineOver, lineUnder) <= geometry::left(*bounds))
						continue;
					else if(min(lineOver, lineUnder) >= geometry::right(*bounds))
						break;
				}
			}
		}

		const boost::iterator_range<RunVector::const_iterator> runs(runsForLine(line.line()));
		Scalar x = static_cast<Scalar>(boost::geometry::distance(lineLeft(line.line()), boost::geometry::make_zero<Point>()));	// line-relative
		BOOST_FOREACH(const unique_ptr<const TextRun>& run, runs) {
			// 'x' is line-left edge of the run, here
			if(linearBounds != boost::none && x >= *linearBounds->end())
				break;
			const Scalar runAllocationMeasure = allocationMeasure(*run);
			if(linearBounds == boost::none || x + runAllocationMeasure > *linearBounds->begin()) {
				auto selectionInRun(intersection(
					boost::irange<Index>(run->characterRange().begin() - textString_.data(), run->characterRange().end() - textString_.data()), orderedRange));
				if(!boost::empty(selectionInRun)) {
					Scalar glyphsLeft = x;	// line-left edge of glyphs content of the run
					if(const FlowRelativeFourSides<Scalar>* const margin = run->margin())
						glyphsLeft += margin->start();
					if(const FlowRelativeFourSides<ComputedBorderSide>* const border = run->border())
						glyphsLeft += border->start().computedWidth();
					if(const FlowRelativeFourSides<Scalar>* const padding = run->padding())
						glyphsLeft += padding->start();

					// compute leading and trailing edges highlight shape in the run
					Scalar leading = run->leadingEdge(*selectionInRun.begin()), trailing = run->leadingEdge(*selectionInRun.end());
					leading = glyphsLeft + (writingMode().inlineFlowDirection == LEFT_TO_RIGHT) ? leading : (font::measure(*run) - leading);
					trailing = glyphsLeft + (writingMode().inlineFlowDirection == LEFT_TO_RIGHT) ? trailing : (font::measure(*run) - trailing);
					Rectangle rectangle(mapLineRelativeToPhysical(writingMode(),
						LineRelativeFourSides<Scalar>(_over = lineOver, _under = lineUnder, _lineLeft = min(leading, trailing), _lineRight = max(leading, trailing))));

					if(bounds != boost::none)
						rectangle = boost::geometry::intersection(rectangle, *bounds, rectangle);	// clip by 'bounds'
					boost::geometry::model::polygon<Point> oneShape;
					boost::geometry::convert(rectangle, oneShape);
					results.push_back(oneShape);
				}
			}
			x += runAllocationMeasure;
		}
	}

	return move(results);
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
		Scalar ipd = 0;
		BOOST_FOREACH(const unique_ptr<const TextRun>& run, runsForLine(line))
			ipd += allocationMeasure(*run);
		assert(ipd >= 0);
		if(numberOfLines() == 1)
			self.maximumMeasure_ = ipd;
		else
			self.lineMeasures_[line] = ipd;
		return ipd;
	}
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

/**
 * @internal Returns the text run containing the specified offset in this layout.
 * @param offset The offset in this layout
 * @return An iterator addresses the text run
 * @note If @a offset is equal to the length of this layout, returns the last text run.
 */
TextLayout::RunVector::const_iterator TextLayout::runForPosition(Index offset) const BOOST_NOEXCEPT {
	assert(!isEmpty());
	if(offset == numberOfCharacters())
		return end(runs_) - 1;
	const String::const_pointer p(textString_.data() + offset);
	const boost::iterator_range<RunVector::const_iterator> runs(runsForLine(lineAt(offset)));
	for(RunVector::const_iterator run(runs.begin()); run != runs.end(); ++run) {
		if(includes((*run)->characterRange(), p))
			return run;
	}
	ASCENSION_ASSERT_NOT_REACHED();
}

/**
 * Stacks the line boxes and compute the line metrics.
 * @param context
 * @param lineHeight
 * @param lineStackingStrategy
 * @param nominalFont
 */
void TextLayout::stackLines(const RenderingContext2D& context, boost::optional<Scalar> lineHeight, LineBoxContain lineBoxContain, const Font& nominalFont) {
	// TODO: this code is temporary. should rewrite later.
	assert(numberOfLines() > 1);
	unique_ptr<LineMetrics[]> newLineMetrics(new LineMetrics[numberOfLines()]);
	// calculate allocation-rectangle of the lines according to line-stacking-strategy
	const unique_ptr<const FontMetrics<Scalar>> nominalFontMetrics(context.fontMetrics(nominalFont.shared_from_this()));
	const Scalar textAltitude = nominalFontMetrics->ascent();
	const Scalar textDepth = nominalFontMetrics->descent();
	for(Index line = 0; line < numberOfLines(); ++line) {
		// calculate extent of the line in block-progression-direction
		Scalar ascent, descent;
#if 0
		switch(lineBoxContain) {
			case LINE_HEIGHT: {
				// allocation-rectangle of line is per-inline-height-rectangle
				Scalar leading = lineHeight - (textAltitude + textDepth);
				ascent = textAltitude + (leading - leading / 2);
				descent = textDepth + leading / 2;
				const RunVector::const_iterator lastRun(firstRunInLine(line + 1));
				for(RunVector::const_iterator run(firstRunInLine(line)); run != lastRun; ++run) {
					leading = lineHeight - nominalFont.metrics()->cellHeight();
					ascent = max((*run)->font()->metrics()->ascent() - (leading - leading / 2), ascent);
					descent = max((*run)->font()->metrics()->descent() - leading / 2, descent);
				}
				break;
			}
			case FONT_HEIGHT:
				// allocation-rectangle of line is nominal-requested-line-rectangle
				ascent = textAltitude;
				descent = textDepth;
				break;
			case MAX_HEIGHT: {
				// allocation-rectangle of line is maximum-line-rectangle
				ascent = textAltitude;
				descent = textDepth;
				const RunVector::const_iterator lastRun(firstRunInLine(line + 1));
				for(RunVector::const_iterator run(firstRunInLine(line)); run != lastRun; ++run) {
					ascent = max((*run)->font()->metrics()->ascent(), ascent);
					descent = max((*run)->font()->metrics()->descent(), descent);
				}
				break;
			}
			default:
				ASCENSION_ASSERT_NOT_REACHED();
		}
#else
		ascent = textAltitude;
		descent = textDepth;
#endif
		get<0>(newLineMetrics[line]) = ascent;
		get<1>(newLineMetrics[line]) = descent;
		get<2>(newLineMetrics[line]) = 0;
	}
}

#if 0
/**
 * Returns the styled text run containing the specified offset in the line.
 * @param offsetInLine The offset in the line
 * @return the styled segment
 * @throw IndexOutOfBoundsException @a offsetInLine is greater than the length of the line
 */
StyledRun TextLayout::styledTextRun(Index offsetInLine) const {
	if(offsetInLine > text().length())
		throw IndexOutOfBoundsException("offsetInLine");
	const TextRun& run = *runs_[findRunForPosition(offsetInLine)];
	return StyledRun(run.offsetInLine(), run.requestedStyle());
}
#endif

/**
 * Returns a multi-polygon enclosing the visual selection in the specified range, extended to the
 * specified bounds.
 * @param range The visual selection
 * @param bounds The bounding rectangle to which to extend the selection, in user units. If this is
 *               @c boost#none, the natural bounds is used
 * @return An area enclosing the selection, in user units
 * @throw IndexOutOfBoundsException
 * @see #logicalHighlightShape, #logicalRangesForVisualSelection
 */
boost::geometry::model::multi_polygon<boost::geometry::model::polygon<Point>>&&
		TextLayout::visualHighlightShape(const boost::iterator_range<TextHit>& range, const boost::optional<Rectangle>& bounds) const {
}

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
