/**
 * @file text-layout.cpp
 * @author exeal
 * @date 2003-2006 (was TextLayout.cpp)
 * @date 2006-2011
 * @date 2010-11-20 renamed from ascension/layout.cpp
 * @date 2012, 2014-2016
 */

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, ...
#include <ascension/corelib/numeric-range-algorithm/encompasses.hpp>
#include <ascension/corelib/numeric-range-algorithm/hull.hpp>
#include <ascension/corelib/numeric-range-algorithm/includes.hpp>
#include <ascension/corelib/numeric-range-algorithm/intersection.hpp>
#include <ascension/corelib/text/character-iterator.hpp>
#include <ascension/corelib/text/character-property.hpp>
#include <ascension/graphics/font/actual-text-styles.hpp>
#include <ascension/graphics/font/detail/inline-progression-dimension-range-iterator.hpp>
#include <ascension/graphics/font/font.hpp>
#include <ascension/graphics/font/font-metrics.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/text-run.hpp>
#include <ascension/graphics/geometry/rectangle-corners.hpp>
#include <ascension/graphics/geometry/rectangle-range.hpp>
#include <ascension/graphics/geometry/algorithms/make.hpp>
#include <ascension/graphics/geometry/algorithms/translate.hpp>
#include <ascension/graphics/paint-context.hpp>
#include <ascension/graphics/rendering-device.hpp>
#include <ascension/presentation/writing-mode-mappings.hpp>
#include <boost/foreach.hpp>
#include <boost/geometry/algorithms/intersection.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/numeric.hpp>	// boost.accumulate
#include <limits>	// std.numeric_limits
#include <numeric>	// std.accumulate
//#define ASCENSION_TRACE_LAYOUT_CACHES
//#define ASCENSION_DIAGNOSE_INHERENT_DRAWING;

namespace boost {
	template<typename T, typename U>
	inline bool operator==(boost::integer_range<T> lhs, boost::integer_range<U> rhs) {
		return boost::equal(lhs, rhs);
	}
}

namespace ascension {
	namespace graphics {
		namespace font {
			namespace detail {
				std::shared_ptr<const graphics::font::Font> findMatchingFont(const StringPiece& textRun,
						const graphics::font::FontCollection& collection, const graphics::font::ActualFontSpecification& specification) {
#if 0
					void resolveFontSpecifications(const FontCollection& fontCollection,
							std::shared_ptr<const TextRunStyle> requestedStyle, std::shared_ptr<const TextRunStyle> defaultStyle,
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
#else
					// TODO: Not implemented (but this function is referred by noone.
					return std::shared_ptr<const graphics::font::Font>();
#endif
				}

				/**
				 * @internal Paints border.
				 * @param context The graphics context
				 * @param rectangle The border box. This gives the edge surrounds the border
				 * @param border The presentative style
				 * @param writingMode The writing mode used to compute the directions and orientation of @a border
				 */
				void paintBorder(graphics::PaintContext& context, const graphics::Rectangle& rectangle,
						const graphics::PhysicalFourSides<graphics::font::ActualBorderSide>& border, const presentation::WritingMode& writingMode) {
					using namespace ::ascension::graphics;	// TODO: 'detail' namespace should spread into individual namespaces.
					using namespace ::ascension::graphics::font;
					// TODO: not implemented.
					for(PhysicalFourSides<ActualBorderSide>::const_iterator side(boost::const_begin(border)), e(boost::const_end(border)); side != e; ++side) {
						if(!side->hasVisibleStyle() || side->actualWidth() <= 0)
							continue;
						if(!boost::geometry::within(rectangle, context.boundsToPaint()))
							continue;
						const Color& color = boost::fusion::at_key<presentation::styles::BorderColor>(*side);
						if(color.isFullyTransparent())
							continue;
						context.setStrokeStyle(std::make_shared<SolidColor>(color));
						context.setLineWidth(boost::fusion::at_key<presentation::styles::BorderWidth>(*side));
//						context.setStrokeDashArray();
//						context.setStrokeDashOffset();

						Point from, to;
						auto delta(boost::geometry::make_zero<Dimension>());
						switch(boost::native_value(static_cast<PhysicalDirection>(side - boost::const_begin(border)))) {
							case PhysicalDirection::TOP:
								boost::geometry::assign(from, geometry::topLeft(rectangle));
								boost::geometry::assign(to, geometry::topRight(rectangle));
								geometry::dx(delta) = 1;
								break;
							case PhysicalDirection::RIGHT:
								boost::geometry::assign(from, geometry::topRight(rectangle));
								boost::geometry::assign(to, geometry::bottomRight(rectangle));
								geometry::dy(delta) = 1;
								break;
							case PhysicalDirection::BOTTOM:
								boost::geometry::assign(from, geometry::bottomLeft(rectangle));
								boost::geometry::assign(to, geometry::bottomRight(rectangle));
								geometry::dx(delta) = 1;
								break;
							case PhysicalDirection::LEFT:
								boost::geometry::assign(from, geometry::topLeft(rectangle));
								boost::geometry::assign(to, geometry::bottomLeft(rectangle));
								geometry::dy(delta) = 1;
								break;
							default:
								ASCENSION_ASSERT_NOT_REACHED();
						}
						geometry::translate((geometry::_from = to, geometry::_to = to), delta);
						context.beginPath().moveTo(from).lineTo(to).stroke();
					}
				}
			}	// namespace detail

			// graphics.font free functions ///////////////////////////////////////////////////////////////////////////

			void paintTextDecoration(PaintContext& context, const TextRun& run, const Point& origin, const ActualTextDecoration& decoration) {
				if(boost::fusion::at_key<presentation::styles::TextDecorationLine>(decoration) != presentation::styles::TextDecorationLineEnums::NONE
						&& !boost::fusion::at_key<presentation::styles::TextDecorationColor>(decoration).isFullyTransparent()) {
					// TODO: Not implemented.
				}
			}


			// TextLayout /////////////////////////////////////////////////////////////////////////////////////////////

			namespace {
				// Returns distance from line-left edge of allocation-rectangle to one of content-rectangle.
				inline Scalar lineRelativeAllocationOffset(const TextRun& textRun, const presentation::WritingMode& writingMode) {
				}
			}

			/**
			 * @class ascension::graphics::font::TextLayout
			 * @c TextLayout is an immutable graphical representation of styled text. Provides support for drawing,
			 * cursor navigation, hit testing, text wrapping, etc.
			 *
			 * <h3>Coordinate system</h3>
			 * All graphical information returned from a @c TextLayout object' method is relative to the origin of
			 * @c TextLayout, which is the intersection of the start edge with the baseline of the first line of
			 * @c TextLayout. The start edge is determined by the reading direction (inline progression dimension) of
			 * the line. Also, coordinates passed into a @c TextLayout object's method are assumed to be relative to
			 * the @c TextLayout object's origin.
			 *
			 * <h3>Constraints by Win32/Uniscribe</h3>
			 * <del>A long run will be split into smaller runs automatically because Uniscribe rejects too long text
			 * (especially @c ScriptShape and @c ScriptTextOut). For this reason, a combining character will be
			 * rendered incorrectly if it is presented at the boundary. The maximum length of a run is 1024.
			 *
			 * In present, this class supports only text layout horizontal against the output device.
			 *
			 * @note This class is not intended to be derived.
			 * @see TextLayoutBuffer#lineLayout, TextLayoutBuffer#lineLayoutIfCached
			 */

			/// Destructor.
			TextLayout::~TextLayout() BOOST_NOEXCEPT {
			}
#if 0
			/**
			 * Returns the computed text alignment of the line. The returned value may be
			 * @c presentation#ALIGN_START or @c presentation#ALIGN_END.
			 * @see #readingDirection, presentation#resolveTextAlignment
			 */
			TextAlignment TextLayout::alignment() const BOOST_NOEXCEPT {
				if(style_.get() != nullptr && style_->readingDirection != INHERIT_TEXT_ALIGNMENT)
					style_->readingDirection;
				std::shared_ptr<const TextLineStyle> defaultStyle(lip_.presentation().defaultTextLineStyle());
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
			 * Returns the smallest rectangle emcompasses the whole text of the line.
			 * It might not coincide exactly the ascent, descent, origin or advance of the @c TextLayout.
			 * @return The size of the bounds
			 * @see #blackBoxBounds, #bounds(Index)
			 */
			presentation::FlowRelativeFourSides<Scalar> TextLayout::bounds() const BOOST_NOEXCEPT {
				if(numberOfLines() <= 1)
					return bounds(0);
				presentation::FlowRelativeFourSides<Scalar> sides;
				sides.before() = bounds(0).before();
				sides.after() = bounds(numberOfLines() - 1).after();
				sides.start() = std::numeric_limits<Scalar>::max();
				sides.end() = std::numeric_limits<Scalar>::min();
				for(Index line = 0; line < numberOfLines(); ++line) {
					const Scalar lineStart = lineStartEdge(line);
					sides.start() = std::min(lineStart, sides.start());
					sides.end() = std::max(lineStart + measure(line), sides.end());
				}
				return sides;
			}

			/**
			 * Returns the bounds of the specified line in flow-relative coordinates. It might not coincide
			 * exactly the ascent, descent, origin or advance of the @c TextLayout.
			 * @param line The line number
			 * @return The line bounds in user units
			 * @throw IndexOutOfBoundsException @a line &gt;= @c #numberOfLines()
			 * @see #baseline, #lineStartEdge, #measure
			 */
			presentation::FlowRelativeFourSides<Scalar> TextLayout::bounds(Index line) const {
				Scalar over = std::numeric_limits<Scalar>::min(), under = std::numeric_limits<Scalar>::min();
				BOOST_FOREACH(const std::unique_ptr<const TextRun>& run, runsForLine(line)) {	// may throw IndexOutOfBoundsException
					const graphics::Rectangle runVisualBounds(run->visualBounds());
					over = std::max(-geometry::top(runVisualBounds), over);
					under = std::max(geometry::bottom(runVisualBounds), under);
				}

				presentation::FlowRelativeFourSides<Scalar> sides;
				sides.start() = lineStartEdge(line);
				sides.end() = sides.start() + measure(line);
				const presentation::WritingMode wm(writingMode(*this));
				const LineMetricsIterator lm(*this, line);
				if(isHorizontal(wm.blockFlowDirection) || resolveTextOrientation(wm) != presentation::SIDEWAYS_LEFT) {
					sides.before() = lm.baselineOffset() - over;
					sides.after() = lm.baselineOffset() + under;
				} else {
					sides.before() = lm.baselineOffset() - under;
					sides.after() = lm.baselineOffset() + over;
				}
				return sides;
			}

			/**
			 * Returns the smallest rectangle emcompasses all characters in the range. It might not coincide
			 * exactly the ascent, descent or overhangs of the specified region of the text.
			 * @param characterRange The character range
			 * @return The bounds
			 * @throw IndexOutOfBoundsException @a characterRange intersects with the outside of the line
			 * @see #blackBoxBounds, #bounds(void), #lineBounds
			 */
			presentation::FlowRelativeFourSides<Scalar> TextLayout::bounds(const boost::integer_range<Index>& characterRange) const {
				const auto orderedCharacterRange(characterRange | adaptors::ordered());
				if(*boost::const_end(orderedCharacterRange) > numberOfCharacters())
					throw IndexOutOfBoundsException("characterRange");

				Scalar before, after, start, end;	// results

				if(isEmpty()) {	// empty line
					start = end = 0;
					const auto lm(lineMetrics(0));
					before = -lm.ascent();
					end = +lm.descent();
				} else if(boost::empty(orderedCharacterRange)) {	// an empty rectangle for an empty range
					const auto lm(lineMetrics(lineAt(TextHit<>::leading(orderedCharacterRange.front()))));
					const presentation::FlowRelativeTwoAxes<Scalar> leading(hitToPoint(TextHit<>::leading(orderedCharacterRange.front())));
					before = leading.bpd() - lm.ascent();
					after = leading.bpd() + lm.descent();
					start = end = leading.ipd();
				} else {
					const Index firstLine = lineAt(TextHit<>::afterOffset(*boost::const_begin(orderedCharacterRange)));
					const Index lastLine = lineAt(TextHit<>::beforeOffset(*boost::const_end(orderedCharacterRange)));	// TODO: Surrogates are OK?
					const LineMetricsIterator firstLineMetrics(*this, firstLine), lastLineMetrics(*this, lastLine);

					// calculate the block-progression-edges ('before' and 'after'; it's so easy)
					before = firstLineMetrics.baselineOffset() - firstLineMetrics.ascent();
					after = lastLineMetrics.baselineOffset() + lastLineMetrics.descent() + lastLineMetrics.leading();

					// calculate start-edge and end-edge of fully covered lines
					const bool firstLineIsFullyCovered = encompasses(orderedCharacterRange,
						boost::irange(lineOffset(firstLine), lineOffset(firstLine) + lineLength(firstLine)));
					const bool lastLineIsFullyCovered = encompasses(orderedCharacterRange,
						boost::irange(lineOffset(lastLine), lineOffset(lastLine) + lineLength(lastLine)));
					start = std::numeric_limits<Scalar>::max();
					end = std::numeric_limits<Scalar>::min();
					for(Index line = firstLine + firstLineIsFullyCovered ? 0 : 1;
							line < lastLine + lastLineIsFullyCovered ? 1 : 0; ++line) {
						const Scalar lineStart = lineStartEdge(line);
						start = std::min(lineStart, start);
						end = std::max(lineStart + measure(line), end);
					}

					// calculate start and end-edge of partially covered lines
					std::vector<Index> partiallyCoveredLines;
					if(!firstLineIsFullyCovered)
						partiallyCoveredLines.push_back(firstLine);
					if(!lastLineIsFullyCovered && (boost::empty(partiallyCoveredLines) || partiallyCoveredLines[0] != lastLine))
						partiallyCoveredLines.push_back(lastLine);
					if(!boost::empty(partiallyCoveredLines)) {
						const StringPiece effectiveCharacterRange(textString_.data() + orderedCharacterRange.front(), boost::size(orderedCharacterRange));
						BOOST_FOREACH(Index line, partiallyCoveredLines) {
							const boost::iterator_range<RunVector::const_iterator> runs(runsForLine(line));
							const presentation::ReadingDirection direction =
								isLeftToRight(*this) ? presentation::LEFT_TO_RIGHT : presentation::RIGHT_TO_LEFT;

							// find 'start-edge'
							detail::InlineProgressionDimensionRangeIterator i(
								runs, direction, effectiveCharacterRange, Direction::forward(), lineStartEdge(line));
							assert(i != detail::InlineProgressionDimensionRangeIterator());
							start = std::min(*boost::const_begin(*i), start);

							// find 'end-edge'
							i = detail::InlineProgressionDimensionRangeIterator(
								runs, direction, effectiveCharacterRange, Direction::backward(), lineStartEdge(line) + measure(line));
							assert(i != detail::InlineProgressionDimensionRangeIterator());
							end = std::max(*boost::const_end(*i), end);
						}
					}
				}

				return presentation::FlowRelativeFourSides<Scalar>(
					presentation::_blockStart = before, presentation::_blockEnd = after,
					presentation::_inlineStart = start, presentation::_inlineEnd = end);
			}

			/**
			 * Returns the bidirectional embedding level of the character at the specified offset.
			 * @param at The character from which to get the bidirectional embedding level
			 * @return The bidirectional embedding level of the character at the specified offset
			 * @throw IndexOutOfBoundsException @a offset &gt;= @c #numberOfCharacters()
			 */
			std::uint8_t TextLayout::characterLevel(const TextHit<>& at) const {
				if(at.characterIndex() >= numberOfCharacters())
					throw IndexOutOfBoundsException("at");
				const auto run(runForPosition(at));
				if(run == boost::const_end(runs_))
					throw IndexOutOfBoundsException("at");
				return (*run)->characterLevel();
			}

#ifdef _DEBUG
			/**
			 * Dumps the all runs to the specified output stream.
			 * @param out The output stream
			 */
			void TextLayout::dumpRuns(std::ostream& out) const {
				const String::const_pointer backingStore = textString_.data();
				std::size_t i = 0;
				BOOST_FOREACH(const std::unique_ptr<const TextRun>& run, runs_) {
					out << i++
						<< ": [" << static_cast<unsigned int>(std::distance(backingStore, boost::const_begin(run->characterRange())))
						<< "," << static_cast<unsigned int>(std::distance(backingStore, boost::const_end(run->characterRange()))) << ")" << std::endl;
				}
			}
#endif // _DEBUG

			/**
			 * Returns a @c TextHit corresponding to the specified point. This method is a convenience overload of
			 * @c #hitTestCharacter that uses the natural bounds of this @c TextLayout.
			 * @param point The abstract point
			 * @param[out] outOfBounds @c true if @a point is out of the logical bounds of the @c TextLayout. Can be
			 *                         @c null if not needed
			 * @return A hit describing the character and edge (leading or trailing) under the specified point
			 * @see TextRun#hitTestCharacter
			 */
			TextHit<> TextLayout::hitTestCharacter(const presentation::FlowRelativeTwoAxes<Scalar>& point, bool* outOfBounds /* = nullptr */) const {
				return internalHitTestCharacter(point, nullptr, outOfBounds);
			}

			/**
			 * Returns a @c TextHit corresponding to the specified point. Coordinates outside the bounds of the
			 * @c TextLayout map to hits on the leading edge of the first logical character, or the trailing edge of
			 * the last logical character, as appropriate, regardless of the position of that character in the line.
			 * Only the direction along the baseline is used to make this evaluation.
			 * @param point The abstract point
			 * @param bounds The bounds of the @c TextLayout
			 * @param[out] outOfBounds @c true if @a point is out of the logical bounds of the @c TextLayout. Can be
			 *                         @c null if not needed
			 * @return A hit describing the character and edge (leading or trailing) under the specified point
			 * @see TextRun#hitTestCharacter
			 */
			TextHit<> TextLayout::hitTestCharacter(const presentation::FlowRelativeTwoAxes<Scalar>& point,
					const presentation::FlowRelativeFourSides<Scalar>& bounds, bool* outOfBounds /* = nullptr */) const {
				return internalHitTestCharacter(point, &bounds, outOfBounds);
			}

			namespace {
				inline Scalar lineRelativeGlyphContentOffset(const TextRun& textRun, presentation::ReadingDirection lineInlineFlowDirection) BOOST_NOEXCEPT {
					using presentation::FlowRelativeFourSides;
					using presentation::LEFT_TO_RIGHT;
					Scalar offset = 0;
					if(const FlowRelativeFourSides<Scalar>* const margin = textRun.margin())
						offset += (lineInlineFlowDirection == LEFT_TO_RIGHT) ? margin->start() : margin->end();
					if(const FlowRelativeFourSides<ActualBorderSide>* const border = textRun.border())
						offset += ((lineInlineFlowDirection == LEFT_TO_RIGHT) ? border->start() : border->end()).actualWidth();
					if(const FlowRelativeFourSides<Scalar>* const padding = textRun.padding())
						offset += (lineInlineFlowDirection == LEFT_TO_RIGHT) ? padding->start() : padding->end();
					return offset;
				}
			}

			/**
			 * Converts a hit to a point in abstract coordinates.
			 * @param hit The hit to check. This must be a valid hit on the @c TextLayout
			 * @return The returned point. The point is in abstract coordinates
			 * @throw IndexOutOfBoundsException @a hit is not valid for the @c TextLayout
			 */
			presentation::FlowRelativeTwoAxes<Scalar> TextLayout::hitToPoint(const TextHit<>& hit) const {
				if(hit.insertionIndex() > numberOfCharacters())
					throw IndexOutOfBoundsException("hit");

				if(isEmpty())
					return presentation::FlowRelativeTwoAxes<Scalar>(presentation::_ipd = 0.0f, presentation::_bpd = LineMetricsIterator(*this, 0).baselineOffset());

				// locate line
				const Index line = lineAt(hit);

				// compute inline-progression-dimension
				const presentation::WritingMode wm(writingMode(*this));
				const StringPiece::const_iterator at(textString_.data() + hit.insertionIndex());
				Scalar x = 0;	// line-relative position
				BOOST_FOREACH(const std::unique_ptr<const TextRun>& run, runsForLine(line)) {
					if(at >= boost::const_begin(run->characterRange()) && at < boost::const_end(run->characterRange())) {
						x += lineRelativeGlyphContentOffset(*run, wm.inlineFlowDirection);
						x += run->hitToLogicalPosition(TextHit<>::leading(std::distance(boost::const_begin(run->characterRange()), at)));
						break;
					}
					x += boost::size(allocationMeasure(*run));
				}
				Scalar ipd = (wm.inlineFlowDirection == presentation::LEFT_TO_RIGHT) ? x : (measure(line) - x);
				ipd += lineStartEdge(line);

				return presentation::FlowRelativeTwoAxes<Scalar>(presentation::_ipd = ipd, presentation::_bpd = LineMetricsIterator(*this, line).baselineOffset());
			}

			/// @internal Implements @c #extent and @c #extentWithHalfLeadings methods.
			NumericRange<Scalar> TextLayout::internalExtent(const boost::integer_range<Index>& lines, bool includeHalfLeadings) const {
				if(boost::empty(lines)) {
					const Scalar baseline = LineMetricsIterator(*this, *boost::const_begin(lines)).baselineOffset();
					return nrange(baseline, baseline);
				} else if(boost::size(lines) == 1)
					return LineMetricsIterator(*this, lines.front()).extent();

				const auto orderedLines(lines | adaptors::ordered());
				if(*boost::const_end(orderedLines) > numberOfLines())
					throw IndexOutOfBoundsException("lines");

				LineMetricsIterator i(*this, lines.front());
				const NumericRange<Scalar> firstExtent(i.extent());
				std::advance(i, boost::size(lines) - 1);
				return hull(firstExtent | adaptors::ordered(), i.extent() | adaptors::ordered());
			}

			/// @internal Implements @c #hitTestCharacter methods.
			TextHit<> TextLayout::internalHitTestCharacter(const presentation::FlowRelativeTwoAxes<Scalar>& point,
					const presentation::FlowRelativeFourSides<Scalar>* bounds, bool* outOfBounds) const {
				const auto line(locateLine(point.bpd(), (bounds != nullptr) ? boost::make_optional(presentation::blockFlowRange(*bounds)) : boost::none));
				const auto runsInLine(runsForLine(std::get<0>(line)));
				const Scalar lineStart = lineStartEdge(std::get<0>(line));

				if(boost::empty(runsInLine)) {
					assert(isEmpty());
					if(outOfBounds != nullptr)
						*outOfBounds = point.ipd() == lineStart;
					return TextHit<>::leading(0);
				}

				const StringPiece characterRangeInLine(boost::const_begin((*boost::const_begin(runsInLine))->characterRange()), lineLength(std::get<0>(line)));
				assert(boost::const_end(characterRangeInLine) == boost::const_end(runsInLine.back()->characterRange()));

				if(point.ipd() < lineStart || (bounds != nullptr && point.ipd() < std::min(bounds->start(), bounds->end()))) {	// beyond 'start-edge' of line ?
					if(outOfBounds != nullptr)
						*outOfBounds = true;
					return TextHit<>::leading(boost::const_begin(characterRangeInLine) - textString_.data());
				}
				const bool outsideInIpd = point.ipd() >= lineStart + measure(std::get<0>(line))
					|| (bounds != nullptr && point.ipd() >= std::max(bounds->start(), bounds->end()));	// beyond 'end-edge' of line ?

				if(!outsideInIpd) {
					const presentation::WritingMode wm(writingMode(*this));
					Scalar x = point.ipd() - lineStart, runLineLeft = 0;
					if(wm.inlineFlowDirection == presentation::RIGHT_TO_LEFT)
						x = measure(std::get<0>(line)) - x;
					// 'x' is distance from line-left of 'line' to 'point' in inline-progression-dimension
					// 'runLineLeft' is distance from line-left of 'line' to line-left of 'run' in ...
					BOOST_FOREACH(const std::unique_ptr<const TextRun>& run, runsInLine) {
						const Scalar runLineRight = runLineLeft + boost::size(allocationMeasure(*run));
						if(runLineRight > x) {
							const TextHit<> hitInRun(run->hitTestCharacter(
								x - runLineLeft - lineRelativeGlyphContentOffset(*run, wm.inlineFlowDirection), boost::none, nullptr));
							const Index position = boost::const_begin(run->characterRange()) - textString_.data() + hitInRun.characterIndex();
							return hitInRun.isLeadingEdge() ? TextHit<>::leading(position) : TextHit<>::trailing(position);
						}
						runLineLeft = runLineRight;
					}
				}

				// maybe beyond 'end-edge' of line
				if(outOfBounds != nullptr)
					*outOfBounds = true;
				return TextHit<>::trailing((boost::const_end(characterRangeInLine) - textString_.data()) - 1);
			}

#if 0
			/// Returns an iterator addresses the first styled segment.
			TextLayout::StyledSegmentIterator TextLayout::firstStyledSegment() const BOOST_NOEXCEPT {
				const TextRun* temp = *runs_;
				return StyledSegmentIterator(temp);
			}
#endif

			/**
			 * @fn ascension::graphics::font::TextLayout::initialize
			 * @internal Initializes the layout.
			 * @param textRunStyles The computed text runs styles
			 * @param lengthContext
			 * @param percentageResolver
			 * @param fontCollection The font collection
			 * @param fontRenderContext Information about a graphics device which is needed to measure the text correctly
			 */

			/**
			 * Returns if the line contains right-to-left run.
			 * @note This method's semantics seems to be strange. Is containning RTL run means bidi?
			 */
			bool TextLayout::isBidirectional() const BOOST_NOEXCEPT {
				if(!isLeftToRight(*this))
					return true;
				BOOST_FOREACH(const std::unique_ptr<const TextRun>& run, runs_) {
					if(run->direction() == presentation::RIGHT_TO_LEFT)
						return true;
				}
				return false;
			}
#if 0
			/// Returns an iterator addresses the last styled segment.
			TextLayout::StyledSegmentIterator TextLayout::lastStyledSegment() const BOOST_NOEXCEPT {
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
				if(!isVertical(*this)) {
					if(isLeftToRight(*this))
						return geometry::make<Point>((geometry::_x = lineStartEdge(line), geometry::_y = 0.0f));
					else
						return geometry::make<Point>((geometry::_x = -lineStartEdge(line) - measure(line), geometry::_y = 0.0f));
				} else {
					const presentation::WritingMode wm(writingMode(*this));
					Scalar y = -lineStartEdge(line);
					if(wm.inlineFlowDirection == presentation::RIGHT_TO_LEFT)
						y -= measure(line);
					if(resolveTextOrientation(wm) == presentation::SIDEWAYS_LEFT)
						y = -y;
					return geometry::make<Point>((geometry::_x = 0.0f, geometry::_y = y));
/*					if(writingMode().inlineFlowDirection == presentation::LEFT_TO_RIGHT) {
						if(resolveTextOrientation(writingMode()) != presentation::SIDEWAYS_LEFT)
							return geometry::make<Point>((geometry::_x = 0.0f, geometry::_y = lineStartEdge(line)));
						else
							return geometry::make<Point>((geometry::_x = 0.0f, geometry::_y = -lineStartEdge(line)));
					} else {
						if(resolveTextOrientation(writingMode()) != presentation::SIDEWAYS_LEFT)
							return geometry::make<Point>((geometry::_x = 0.0f, geometry::_y = -lineStartEdge(line) - measure(line)));
						else
							return geometry::make<Point>((geometry::_x = 0.0f, geometry::_y = lineStartEdge(line) + measure(line)));
					}
*/				}
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
				return boost::const_begin((*firstRunInLine(line))->characterRange()) - textString_.data();
			}

			/**
			 * Returns the line offsets. Each value in the vector is the offset for the first character in a
			 * line except for the last value, which contains the length of the text.
			 * @param[out] offsets The line offsets whose length is @c #numberOfLines(). Each element in the array is
			 *                     the offset for the first character in a line
			 * @note Designed based on @c org.eclipse.swt.graphics.TextLayout.lineOffsets method in Eclipse.
			 */
			void TextLayout::lineOffsets(std::vector<Index>& offsets) const BOOST_NOEXCEPT {
				const String::const_pointer bol = textString_.data();
				std::vector<Index> result;
				result.reserve(numberOfLines() + 1);
				for(Index line = 0; line < numberOfLines(); ++line)
					result.push_back(boost::const_begin((*firstRunInLine(line))->characterRange()) - bol);
				result.push_back(numberOfCharacters());
				std::swap(result, offsets);
			}

			/**
			 * Returns the start-edge of the specified line without the start-indent in pixels.
			 * @par This is distance from the start-edge of the first line to the one of @a line in
			 * inline-progression-dimension. Therefore, returns always zero when @a line is zero or the anchor is
			 * @c TEXT_ANCHOR_START.
			 * @par A positive value means positive indentation. For example, if the start-edge of a RTL line is x =
			 * -10, this method returns +10.
			 * @param line The line number
			 * @return The start-indentation in pixels
			 * @throw IndexOutOfBoundsException @a line is invalid
			 * @see TextRenderer#lineStartEdge
			 */
			Scalar TextLayout::lineStartEdge(Index line) const {
				if(line == 0)
					return 0;
				switch(boost::native_value(anchor(line))) {
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
			 * @param bpd The position in block-progression-dimension in user units
			 * @param bounds The bounds in block-progression-dimension in user units
			 * @return The @c first member is line number. If @a bpd is outside of the line content, @c second member
			 *         is @c Direction#forward() (beyond the after-edge) or @c Direction#backward() (beyond the
			 *         before-edge), or @c boost#none otherwise
			 * @see #baseline, #lineAt, #offset
			 */
			std::tuple<Index, boost::optional<Direction>> TextLayout::locateLine(Scalar bpd, const boost::optional<NumericRange<Scalar>>& bounds) const BOOST_NOEXCEPT {
				if(bounds != boost::none) {
					const NumericRange<Scalar> orderedBounds(*bounds | adaptors::ordered());
					if(bpd < *boost::const_begin(orderedBounds))
						return std::make_tuple(0, Direction::backward());
					if(bpd >= *boost::const_end(orderedBounds))
						return std::make_tuple(numberOfLines() - 1, Direction::forward());
				}

				LineMetricsIterator line(*this, 0);

				// beyond the before-edge ?
				if(bpd < *boost::const_begin(line.extent() | adaptors::ordered()))
					return std::make_tuple(line.line(), Direction::backward());

				// locate the line includes 'bpd'
				for(; line.line() < numberOfLines(); ++line) {
					if(bpd < *boost::const_end(line.extent() | adaptors::ordered()))
						return std::make_tuple(line.line(), boost::none);
				}

				// beyond the after-edge
				return std::make_tuple(numberOfLines() - 1, Direction::forward());
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
				const StringPiece characterRangeInLine(boost::const_begin(*static_cast<const TextRunImpl*>(boost::const_begin(runsInLine)->get())), lineLength(line));
				assert(boost::const_end(characterRangeInLine) == boost::const_end(*static_cast<const TextRunImpl*>((boost::const_end(runsInLine) - 1)->get())));

				const Scalar lineStart = lineStartEdge(line);
				if(ipd < lineStart) {
					const Index offset = boost::const_begin(characterRangeInLine) - textString_.data();
					return (outside = true), make_pair(offset, offset);
				}
				outside = ipd > lineStart + measure(line);	// beyond line 'end-edge'

				if(!outside) {
					Scalar x = ipd - lineStart, dx = 0;
					if(writingMode().inlineFlowDirection == RIGHT_TO_LEFT)
						x = measure(line) - x;
					for(RunVector::const_iterator run(boost::const_begin(runsInLine)); run != boost::const_end(runsInLine); ++run) {
						const Scalar nextDx = dx + boost::size(allocationMeasure(**run));
						if(nextDx >= x) {
							const Scalar ipdInRun = ((*run)->direction() == LEFT_TO_RIGHT) ? x - dx : nextDx - x;
							return make_pair(boost::get((*run)->characterEncompassesPosition(ipdInRun)), (*run)->characterHasClosestLeadingEdge(ipdInRun));
						}
					}
				}

				// maybe beyond line 'end-edge'
				const Index offset = boost::const_end(characterRangeInLine) - textString_.data();
				return make_pair(offset, offset);
			}
#endif	// ASCENSION_ABANDONED_AT_VERSION_08

			/**
			 * Returns a multi-polygon enclosing the logical selection in the specified range, extended to the
			 * specified bounds.
			 * @param range The range of characters to select
			 * @param bounds The bounding rectangle to which to extend the selection, in user units. If this is
			 *               @c boost#none, the natural bounds is used
			 * @param includeHalfLeadings Set @c true to include the 'half-leading's of the first and the last lines
			 * @param[out] shape An area enclosing the selection, in user units
			 * @throw IndexOutOfBoundsException
			 * @see #logicalRangesForVisualSelection, #visualHighlightShape
			 */
			void TextLayout::logicalHighlightShape(const boost::integer_range<Index>& range, bool includeHalfLeadings,
					const boost::optional<graphics::Rectangle>& bounds, boost::geometry::model::multi_polygon<boost::geometry::model::polygon<Point>>& shape) const {
				const presentation::WritingMode wm(writingMode(*this));
				const bool horizontal = isHorizontal(wm.blockFlowDirection);
				boost::optional<NumericRange<Scalar>> linearBounds;
				if(bounds != boost::none)
					linearBounds = horizontal ? geometry::range<0>(*bounds) : geometry::range<1>(*bounds);
				std::remove_reference<decltype(shape)>::type result;

				const auto orderedRange(range | adaptors::ordered());
				const auto lines(boost::irange(
					lineAt(TextHit<>::afterOffset(*boost::const_begin(orderedRange))),
					lineAt(!boost::empty(orderedRange) ? TextHit<>::beforeOffset(*boost::const_end(orderedRange)) : TextHit<>::afterOffset(*boost::const_begin(orderedRange)))));
				const bool ltr = wm.inlineFlowDirection == presentation::LEFT_TO_RIGHT;
				for(LineMetricsIterator line(lineMetrics(*boost::const_begin(lines))); line.line() != *boost::const_end(lines); ++line) {
					Scalar lineOver, lineUnder;
					{
						const auto e(includeHalfLeadings ? line.extentWithHalfLeadings() : line.extent());
						lineOver = *boost::const_begin(e);
						lineUnder = *boost::const_end(e);
					}

					// skip the line if out of bounds
					if(bounds != boost::none) {
						if(horizontal) {
							if(std::max(lineOver, lineUnder) <= geometry::top(boost::get(bounds)))
								continue;
							else if(std::min(lineOver, lineUnder) >= geometry::bottom(boost::get(bounds)))
								break;
						} else {
							if(wm.blockFlowDirection == presentation::VERTICAL_RL) {
								if(std::min(lineOver, lineUnder) >= geometry::right(boost::get(bounds)))
									continue;
								else if(std::max(lineOver, lineUnder) <= geometry::left(boost::get(bounds)))
									break;
							} else {
								if(std::max(lineOver, lineUnder) <= geometry::left(boost::get(bounds)))
									continue;
								else if(std::min(lineOver, lineUnder) >= geometry::right(boost::get(bounds)))
									break;
							}
						}
					}

					Scalar x = static_cast<Scalar>(boost::geometry::distance(lineLeft(line.line()), boost::geometry::make_zero<Point>()));	// line-relative
					BOOST_FOREACH(const std::unique_ptr<const TextRun>& run, runsForLine(line.line())) {
						// 'x' is line-left edge of the run, here
						if(linearBounds != boost::none && x >= *boost::const_end(boost::get(linearBounds)))
							break;
						const Scalar runAllocationMeasure = boost::size(allocationMeasure(*run));
						if(linearBounds == boost::none || x + runAllocationMeasure > *boost::const_begin(boost::get(linearBounds))) {
							auto selectionInRun(intersection(
								boost::irange<Index>(boost::const_begin(run->characterRange()) - textString_.data(), boost::const_end(run->characterRange()) - textString_.data()), orderedRange));
							if(selectionInRun != boost::none) {
								selectionInRun->advance_begin(textString_.data() - boost::const_begin(run->characterRange()));
								selectionInRun->advance_end(textString_.data() - boost::const_begin(run->characterRange()));

								Scalar glyphsLeft = x;	// line-left edge of glyphs content of the run
								x += lineRelativeGlyphContentOffset(*run, wm.inlineFlowDirection);

								// compute leading and trailing edges highlight shape in the run
								Scalar leading = run->hitToLogicalPosition(TextHit<>::leading(*boost::const_begin(boost::get(selectionInRun))));
								Scalar trailing = run->hitToLogicalPosition(TextHit<>::leading(*boost::const_end(boost::get(selectionInRun))));
								leading = glyphsLeft + ltr ? leading : (boost::size(font::measure(*run)) - leading);
								trailing = glyphsLeft + ltr ? trailing : (boost::size(font::measure(*run)) - trailing);
								PhysicalFourSides<Scalar> temp;
								presentation::mapDimensions(wm,
									presentation::_from = LineRelativeFourSides<Scalar>(
										_lineOver = lineOver, _lineUnder = lineUnder,
										_lineLeft = std::min(leading, trailing), _lineRight = std::max(leading, trailing)),
									presentation::_to = temp);
								Rectangle rectangle(geometry::make<Rectangle>(temp));

								if(bounds != boost::none)
									boost::geometry::intersection(rectangle, boost::get(bounds), rectangle);	// clip by 'bounds'
								boost::geometry::model::polygon<Point> oneShape;
								boost::geometry::convert(rectangle, oneShape);
								result.push_back(oneShape);
							}
						}
						x += runAllocationMeasure;
					}
				}

				std::swap(result, shape);
			}

			/**
			 * Returns the logical ranges of text corresponding to a visual selection.
			 * @param visualSelection The visual range. This can be not ordered
			 * @param[out] result A vector of indices for the selected ranges
			 * @throw IndexOutOfBoundsException @a range is not valid for the @c TextLayout
			 * @see #logicalHighlightShape, #visualHighlightShape
			 */
			void TextLayout::logicalRangesForVisualSelection(const boost::integer_range<TextHit<>>& visualSelection, std::vector<boost::integer_range<Index>>& ranges) const {
				if(boost::const_begin(visualSelection)->insertionIndex() > numberOfCharacters())
					throw IndexOutOfBoundsException("range.begin()");
				if(boost::const_end(visualSelection)->insertionIndex() > numberOfCharacters())
					throw IndexOutOfBoundsException("range.end()");

				std::remove_reference<decltype(ranges)>::type result;
				if(boost::empty(visualSelection))
					return std::swap(result, ranges);

				std::list<TextHit<>> hits;
				hits.push_back(std::min(*boost::const_begin(visualSelection), *boost::const_end(visualSelection)));
				hits.push_back(std::max(*boost::const_begin(visualSelection), *boost::const_end(visualSelection)));
				if(hits.back().characterIndex() == numberOfCharacters()) {	// handle EOL
					assert(hits.back().isLeadingEdge());
					hits.pop_back();
					if(isLeftToRight(*this))
						hits.push_back(TextHit<>::leading(numberOfCharacters()));
					else {
						const std::unique_ptr<const TextRun>& firstRunInLastLine = runsForLine(numberOfLines() - 1).front();
						if(firstRunInLastLine->direction() == presentation::LEFT_TO_RIGHT)
							hits.push_back(TextHit<>::leading(boost::const_begin(firstRunInLastLine->characterRange()) - textString_.data()));
						else
							hits.push_back(TextHit<>::beforeOffset(boost::const_end(firstRunInLastLine->characterRange()) - textString_.data()));
					}
				}

				std::vector<boost::integer_range<Index>> hashedResult;
				BOOST_FOREACH(Index line, boost::irange(lineAt(hits.front()), lineAt(hits.back()) + 1)) {
					BOOST_FOREACH(const std::unique_ptr<const TextRun>& run, runsForLine(line)) {
						const auto runRange(boost::irange<Index>(boost::const_begin(run->characterRange()) - textString_.data(), boost::const_end(run->characterRange()) - textString_.data()));
						// there are four patterns
						if(boost::size(hits) == 2) {
							std::size_t foundHits = 0;
							std::list<TextHit<>>::iterator foundHit;
							for(std::list<TextHit<>>::iterator hit(std::begin(hits)), e(std::end(hits)); hit != e; ++hit) {
								if(includes(runRange, hit->characterIndex())) {
									++foundHits;
									foundHit = hit;
								}
							}
							if(foundHits == 1) {	// selection begins from here
								hits.erase(foundHit);
								if(run->direction() == presentation::LEFT_TO_RIGHT)
									hashedResult.push_back(boost::irange(hits.front().insertionIndex(), *boost::const_end(runRange)));
								else
									hashedResult.push_back(boost::irange(*boost::const_begin(runRange), hits.front().insertionIndex()));
							} else if(foundHits == 2) {	// selection is only in this run
								hashedResult.push_back(boost::irange(hits.front().insertionIndex(), hits.back().insertionIndex()) | adaptors::ordered());
								hits.clear();
							}
						} else {
							assert(boost::size(hits) == 1);
							if(includes(runRange, hits.front().characterIndex())) {	// selection is end here
								if(run->direction() == presentation::LEFT_TO_RIGHT)
									hashedResult.push_back(boost::irange(*boost::const_begin(runRange), hits.front().insertionIndex()));
								else
									hashedResult.push_back(boost::irange(hits.front().insertionIndex(), *boost::const_end(runRange)));
								hits.pop_back();
								break;
							} else
								hashedResult.push_back(runRange);
						}
					}
					if(boost::empty(hits))
						break;
				}
				assert(boost::empty(hits));

				// sort and merge
				boost::sort(hashedResult, [](const boost::integer_range<Index>& lhs, const boost::integer_range<Index>& rhs) {
					return lhs.front() < rhs.front();
				});
				BOOST_FOREACH(const boost::integer_range<Index>& subrange, hashedResult) {
					if(boost::empty(result) || *boost::const_end(result.back()) != *boost::const_begin(subrange))
						result.push_back(subrange);
					else
						result.back().advance_end(*boost::const_end(subrange) - *boost::const_end(result.back()));
				}

				std::swap(result, ranges);
			}

			/**
			 * Returns the measure of the longest line in user units.
			 * @see #measure(Index)
			 */
			Scalar TextLayout::measure() const BOOST_NOEXCEPT {
				if(maximumMeasure_ == boost::none) {
					Scalar ipd = 0;
					BOOST_FOREACH(Index line, boost::irange<Index>(0, numberOfLines()))
						ipd = std::max(measure(line), ipd);
					const_cast<TextLayout*>(this)->maximumMeasure_ = ipd;
				}
				return boost::get(maximumMeasure_);
			}

			/**
			 * Returns the measure of the specified line in user units. The measure is the measurement in the
			 * inline dimension (, or the distance from the start-edge to the end-edge of the line). The
			 * measure of a line includes margins, borders and paddings of the all text runs.
			 * @param line The line number
			 * @return The measure of the line. Must be equal to or greater than zero
			 * @throw IndexOutOfBoundsException @a line is greater than the number of lines
			 * @see #measure(void)
			 */
			Scalar TextLayout::measure(Index line) const {
				if(line >= numberOfLines())
					throw IndexOutOfBoundsException("line");
				TextLayout& self = const_cast<TextLayout&>(*this);
				if(isEmpty())
					return boost::get(self.maximumMeasure_) = 0;
				if(numberOfLines() == 1) {
					if(maximumMeasure_ != boost::none)
						return boost::get(maximumMeasure_);
				} else {
					static_assert(std::is_signed<Scalar>::value, "");
					if(lineMeasures_.get() == nullptr) {
						self.lineMeasures_.reset(new Scalar[numberOfLines()]);
						std::fill_n(self.lineMeasures_.get(), numberOfLines(), static_cast<Scalar>(-1));
					}
					if(lineMeasures_[line] >= 0)
						return lineMeasures_[line];
				}
				const Scalar ipd = boost::accumulate(runsForLine(line), 0.0f, [](Scalar ipd, const std::unique_ptr<const TextRun>& run) {
					return ipd + boost::size(allocationMeasure(*run));
				});
				assert(ipd >= 0);
				if(numberOfLines() == 1)
					self.maximumMeasure_ = ipd;
				else
					self.lineMeasures_[line] = ipd;
				return ipd;
			}
#if 0
			/**
			 * Returns the next tab stop position.
			 * @param x the distance from leading edge of the line (can not be negative)
			 * @param direction the direction
			 * @return the distance from leading edge of the line to the next tab position
			 */
			inline int TextLayout::nextTabStop(int x, Direction direction) const BOOST_NOEXCEPT {
				assert(x >= 0);
				const int tabWidth = lip_.textMetrics().averageCharacterWidth() * lip_.layoutSettings().tabWidth;
				return (direction == Direction::forward()) ? x + tabWidth - x % tabWidth : x - x % tabWidth;
			}

			/**
			 * Returns the next tab stop.
			 * @param x the distance from the left edge of the line to base position (can not be negative)
			 * @param right @c true to find the next right position
			 * @return the tab stop position in pixel
			 */
			int TextLayout::nextTabStopBasedLeftEdge(int x, bool right) const BOOST_NOEXCEPT {
				assert(x >= 0);
				const LayoutSettings& c = lip_.layoutSettings();
				const int tabWidth = lip_.textMetrics().averageCharacterWidth() * c.tabWidth;
				if(lineTerminatorOrientation(style(), lip_.presentation().defaultTextLineStyle()) == LEFT_TO_RIGHT)
					return nextTabStop(x, right ? Direction::forward() : Direction::backward());
				else
					return right ? x + (x - longestLineWidth()) % tabWidth : x - (tabWidth - (x - longestLineWidth()) % tabWidth);
			}

			/**
			 * Returns the computed reading direction of the line.
			 * @see #alignment
			 */
			ReadingDirection TextLayout::readingDirection() const BOOST_NOEXCEPT {
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
			 * @param at The character offset in this layout
			 * @return An iterator addresses the text run
			 * @note If @a offset is equal to the length of this layout, returns the last text run.
			 */
			TextLayout::RunVector::const_iterator TextLayout::runForPosition(const TextHit<>& at) const BOOST_NOEXCEPT {
				assert(!isEmpty());
				if(at.characterIndex() == numberOfCharacters())
					return boost::const_end(runs_) - 1;
				const String::const_pointer p(textString_.data() + at.characterIndex());
				const boost::iterator_range<RunVector::const_iterator> runs(runsForLine(lineAt(at)));
				for(RunVector::const_iterator run(boost::const_begin(runs)); run != boost::const_end(runs); ++run) {
					if(p >= boost::const_begin((*run)->characterRange()) && p < boost::const_end((*run)->characterRange()))
						return run;
				}
				return boost::const_end(runs_);	// unreachable
			}

			/**
			 * Stacks the line boxes and compute the line metrics.
			 * @param context
			 * @param lineHeight
			 * @param lengthContext
			 * @param lineBoxContain
			 * @param nominalFont
			 */
			void TextLayout::stackLines(const RenderingContext2D& context, const presentation::styles::Length& lineHeight,
					const presentation::styles::Length::Context& lengthContext, LineBoxContain lineBoxContain, const Font& nominalFont) {
				// TODO: this code is temporary. should rewrite later.
				assert(numberOfLines() > 0);
				std::unique_ptr<detail::Adl<Scalar>[]> newLineMetrics(new detail::Adl<Scalar>[numberOfLines()]);
				// calculate allocation-rectangle of the lines according to line-stacking-strategy
				const std::unique_ptr<const FontMetrics<Scalar>> nominalFontMetrics(context.fontMetrics(nominalFont.shared_from_this()));
				const Scalar textAltitude = nominalFontMetrics->ascent();
				const Scalar textDepth = nominalFontMetrics->descent();
				for(Index line = 0; line < numberOfLines(); ++line) {
					// calculate extent of the line in block-progression-direction
#if 0
					switch(lineBoxContain) {
						case LINE_HEIGHT: {
							// allocation-rectangle of line is per-inline-height-rectangle
							newLineMetrics[line].leading = lineHeight - (textAltitude + textDepth);
							newLineMetrics[line].ascent = textAltitude + (newLineMetrics[line].leading - newLineMetrics[line].leading / 2);
							newLineMetrics[line].descent = textDepth + newLineMetrics[line].leading / 2;
							const RunVector::const_iterator lastRun(firstRunInLine(line + 1));
							for(RunVector::const_iterator run(firstRunInLine(line)); run != lastRun; ++run) {
								newLineMetrics[line].leading = lineHeight - nominalFont.metrics()->cellHeight();
								newLineMetrics[line].ascent =
									std::max((*run)->font()->metrics()->ascent() - (newLineMetrics[line].leading - newLineMetrics[line].leading / 2), newLineMetrics[line].ascent);
								newLineMetrics[line].descent =
									std::max((*run)->font()->metrics()->descent() - newLineMetrics[line].leading / 2, newLineMetrics[line].descent);
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
								newLineMetrics[line].ascent = std::max((*run)->font()->metrics()->ascent(), newLineMetrics[line].ascent);
								newLineMetrics[line].descent = std::max((*run)->font()->metrics()->descent(), newLineMetrics[line].descent);
							}
							break;
						}
						default:
							ASCENSION_ASSERT_NOT_REACHED();
					}
#else
					newLineMetrics[line] = detail::Adl<Scalar>(textAltitude, textDepth, 0);
#endif
				}

				std::swap(newLineMetrics, lineMetrics_);
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
			 * @param includeHalfLeadings Set @c true to include the 'half-leading's of the first and the last lines
			 * @param[out] shape An area enclosing the selection, in user units
			 * @throw IndexOutOfBoundsException
			 * @see #logicalHighlightShape, #logicalRangesForVisualSelection
			 */
			void TextLayout::visualHighlightShape(const boost::integer_range<TextHit<>>& range,
					const boost::optional<graphics::Rectangle>& bounds, bool includeHalfLeadings,
					boost::geometry::model::multi_polygon<boost::geometry::model::polygon<Point>>& shape) const {
				// TODO: Not implemented.
				shape = boost::geometry::model::multi_polygon<boost::geometry::model::polygon<Point>>();
			}


			// TextLayout.LineMetricsIterator /////////////////////////////////////////////////////////////////////////

			/// Default constructor creates an iterator addresses the end.
			TextLayout::LineMetricsIterator::LineMetricsIterator() BOOST_NOEXCEPT : layout_(nullptr) {
			}

			/**
			 * Constructor creates an iterator addresses the specified line.
			 * @param layout The target text layout
			 * @param line The line number. Can be @c layout.numberOfLines()
			 * @throw IndexOutOfBoundsException @a line &gt; @c layout.numberOfLines()
			 */
			TextLayout::LineMetricsIterator::LineMetricsIterator(const TextLayout& layout, Index line) : layout_(&layout), line_(line) {
				if(line > layout.numberOfLines())
					throw IndexOutOfBoundsException("line");
				resetBaselineOffset();
			}

			/// Implements decrement operators.
			void TextLayout::LineMetricsIterator::decrement() {
				assert(layout_ != nullptr);
				assert(line() > 0);

				assert(line() <= layout_->numberOfLines());
				if(--line_ < layout_->numberOfLines()) {
					const bool negativeVertical = isNegativeVertical();
					baselineOffset_ -= negativeVertical ? layout_->lineMetrics_[line() + 1].ascent() : layout_->lineMetrics_[line() + 1].descent();
					baselineOffset_ -= layout_->lineMetrics_[line()].leading();
					baselineOffset_ -= negativeVertical ? layout_->lineMetrics_[line()].descent() : layout_->lineMetrics_[line()].ascent();
				}
			}

			/// Implements incremental operators.
			void TextLayout::LineMetricsIterator::increment() {
				assert(layout_ != nullptr);
				assert(line() < layout_->numberOfLines());
				if(++line_ < layout_->numberOfLines()) {
					const bool negativeVertical = isNegativeVertical();
					baselineOffset_ += negativeVertical ? layout_->lineMetrics_[line() - 1].descent() : layout_->lineMetrics_[line() - 1].ascent();
					baselineOffset_ += layout_->lineMetrics_[line() - 1].leading();
					baselineOffset_ += negativeVertical ? layout_->lineMetrics_[line()].ascent() : layout_->lineMetrics_[line()].descent();
				}
			}

			/// @internal
			void TextLayout::LineMetricsIterator::resetBaselineOffset() {
				Scalar newOffset = 0;
				if(line() != 0) {
					const bool negativeVertical = isNegativeVertical();
					const detail::Adl<Scalar>* current;
					const detail::Adl<Scalar>* next = &layout_->lineMetrics_[0];
					for(Index i = 0; i != line(); ++i) {
						current = next;
						next = &layout_->lineMetrics_[i + 1];
						newOffset += !negativeVertical ? current->descent() : current->ascent();
						newOffset += current->leading();
						newOffset += !negativeVertical ? next->ascent() : next->descent();
					}
				}
				baselineOffset_ = newOffset;
			}

#if 0
			// TextLayout.StyledSegmentIterator ///////////////////////////////////////////////////////////////////////

			/**
			 * Private constructor.
			 * @param start
			 */
			TextLayout::StyledSegmentIterator::StyledSegmentIterator(const TextRun*& start) BOOST_NOEXCEPT : p_(&start) {
			}

			/// Copy-constructor.
			TextLayout::StyledSegmentIterator::StyledSegmentIterator(const StyledSegmentIterator& rhs) BOOST_NOEXCEPT : p_(rhs.p_) {
			}

			/// Returns the current segment.
			StyledRun TextLayout::StyledSegmentIterator::current() const BOOST_NOEXCEPT {
				const TextRun& run = **p_;
				return StyledRun(run.offsetInLine, run.style);
			}
#endif
		}
	}
}
