/**
 * @file text-layout.hpp
 * @date 2003-2006 (was LineLayout.h)
 * @date 2006-2010
 * @date 2010-11-20 renamed from ascension/layout.hpp
 * @date 2011-2015
 */

#ifndef ASCENSION_TEXT_LAYOUT_HPP
#define ASCENSION_TEXT_LAYOUT_HPP
#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION
#include <ascension/corelib/utility.hpp>	// detail.searchBound
#include <ascension/direction.hpp>
#include <ascension/graphics/color.hpp>
#include <ascension/graphics/font/text-alignment.hpp>
#include <ascension/graphics/font/text-hit.hpp>
//#include <ascension/graphics/geometry/dimension.hpp>
#include <ascension/graphics/geometry/point.hpp>
#include <ascension/graphics/geometry/rectangle.hpp>
#include <ascension/presentation/flow-relative-directions-dimensions.hpp>
#include <ascension/presentation/styles/writing-modes.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>
#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <boost/range/irange.hpp>
#include <vector>

namespace ascension {
	namespace presentation {
		struct ComputedStyledTextRunIterator;
		struct ComputedTextLineStyle;
		struct ComputedTextRunStyle;
		struct ComputedTextToplevelStyle;
		class Presentation;
	}

	namespace graphics {
		class Paint;
		class PaintContext;

		namespace font {
			class TextLayout;

			/// @defgroup writing_modes_of_text_layout Writing Modes of @c TextLayout
			/// @{
			BOOST_CONSTEXPR bool isLeftToRight(const TextLayout& layout) BOOST_NOEXCEPT;
			bool isVertical(const TextLayout& layout) BOOST_NOEXCEPT;
			presentation::WritingMode writingMode(const TextLayout& textLayout) BOOST_NOEXCEPT;
			namespace detail {
				bool isNegativeVertical(const TextLayout& layout);
			}
			/// @}

			/**
			 * The @c InlineObject represents an inline object in @c TextLayout.
			 */
			class InlineObject {
			public:
				/// Destructor.
				virtual ~InlineObject() BOOST_NOEXCEPT {}
				/// Returns the advance (width) of this inline object in pixels.
				virtual Scalar advance() const BOOST_NOEXCEPT = 0;
				/// Returns the ascent of this inline object in pixels.
				virtual Scalar ascent() const BOOST_NOEXCEPT = 0;
				/// Returns the descent of this inline object in pixels.
				virtual Scalar descent() const BOOST_NOEXCEPT = 0;
				/**
				 * Renders this inline object at the specified location.
				 * @param context The graphic context
				 * @param origin The location where this inline object is rendered
				 */
				virtual void draw(PaintContext& context, const Point& origin) BOOST_NOEXCEPT = 0;
//				/// Returns the size of this inline object in user units.
//				Dimension size() const BOOST_NOEXCEPT {
//					return Dimension(geometry::_dx = advance(), geometry::_dy = ascent() + descent());
//				}
			};

			/**
			 * Specifies an overridden segment in the @c TextLayout.
			 * @see TextLayout#draw
			 */
			struct OverriddenSegment {
				/// The length of this segment.
				Index length;
				/// The overridden foreground or @c null if does not override.
				std::shared_ptr<const Paint> foreground;
				/// The transparency of the overridden foreground. This value should be in the range from 0.0 (fully
				/// transparent) to 1.0 (no additional transparency).
				double foregroundAlpha;
				/// The overridden background or @c null if does not override.
				std::shared_ptr<const Paint> background;
				/// The transparency of the overridden background. This value should be in the range from 0.0 (fully
				/// transparent) to 1.0 (no additional transparency).
				double backgroundAlpha;
				/// Set @c false to paint only the glyphs' bounds with @c #background. Otherwise the logical highlight
				/// bounds of characters are painted as background.
				bool usesLogicalHighlightBounds;
			};

			class Font;
			class FontCollection;
			class FontRenderContext;
			class TextPaintOverride;
			template<typename Length = Scalar> class TabExpander;
			class TextRun;

			class TextLayout : private boost::noncopyable {
			public:
				/**
				 * Fetches the metrics of the lines in @c TextLayout object.
				 * @see BaselineIterator, FontMetrics, TextLayout
				 */
				class LineMetricsIterator : public boost::iterators::iterator_facade<
					LineMetricsIterator,	// Derived
					std::nullptr_t,			// value_type
					boost::iterators::bidirectional_traversal_tag> {
				public:
					LineMetricsIterator() BOOST_NOEXCEPT;
					LineMetricsIterator(const TextLayout& layout, Index line);
					Index line() const BOOST_NOEXCEPT;

					/// @name Metrics
					/// @{
					Scalar ascent() const;
					DominantBaseline baseline() const;
					Scalar baselineOffset() const;
					Point baselineOffsetInPhysicalCoordinates() const;
					Scalar descent() const;
					NumericRange<Scalar> extent() const;
					NumericRange<Scalar> extentWithHalfLeadings() const;
					Scalar height() const;
					Scalar leading() const;
					/// @}

				private:
					bool isDone() const BOOST_NOEXCEPT {
						return layout_ == nullptr || line() >= layout_->numberOfLines();
					}
					bool isNegativeVertical() const {
						return detail::isNegativeVertical(*layout_);
					}
					void resetBaselineOffset();
					// boost.iterator_facade
					void decrement();
					difference_type distance_to(const LineMetricsIterator& other) const;
					bool equal(const LineMetricsIterator& other) const;
					void increment();

				private:
					const TextLayout* layout_;
					Index line_;
					Scalar baselineOffset_;
					friend class boost::iterators::iterator_core_access;
				};

			public:
				TextLayout(const String& textString, const presentation::ComputedTextToplevelStyle& toplevelStyle,
					const presentation::ComputedTextLineStyle& lineStyle,
					std::unique_ptr<presentation::ComputedStyledTextRunIterator> textRunStyles,
					const presentation::ComputedTextRunStyle& defaultRunStyle,
					const presentation::styles::Length::Context& lengthContext,
					const Dimension& parentContentArea,
					const FontCollection& fontCollection, const FontRenderContext& fontRenderContext);
				~TextLayout() BOOST_NOEXCEPT;

				/// @name General Attributes
				/// @{
				TextAnchor anchor(Index line) const;
				std::uint8_t characterLevel() const BOOST_NOEXCEPT;
				std::uint8_t characterLevel(Index offset) const;
				bool isBidirectional() const BOOST_NOEXCEPT;
				Index numberOfCharacters() const BOOST_NOEXCEPT;
				/// @}

				/// @name Computed Styles
				/// @{
				const presentation::ComputedTextRunStyle& defaultRunStyle() const BOOST_NOEXCEPT;
				const presentation::ComputedTextToplevelStyle& parentStyle() const BOOST_NOEXCEPT;
				const presentation::ComputedTextLineStyle& style() const BOOST_NOEXCEPT;
				/// @}

				/// @name Visual Line Accesses
				/// @{
				Index lineAt(Index offset) const;
				Index lineLength(Index line) const;
				Index lineOffset(Index line) const;
				void lineOffsets(std::vector<Index>& offsets) const BOOST_NOEXCEPT;
				Index numberOfLines() const BOOST_NOEXCEPT;
				/// @}

				/// @name Metrics
				/// @{
				LineMetricsIterator lineMetrics(Index line) const;
				NumericRange<Scalar> extent() const;
				NumericRange<Scalar> extent(const boost::integer_range<Index>& lines) const;
				NumericRange<Scalar> extentWithHalfLeadings() const;
				NumericRange<Scalar> extentWithHalfLeadings(const boost::integer_range<Index>& lines) const;
				Scalar measure() const BOOST_NOEXCEPT;
				Scalar measure(Index line) const;
				/// @}

				/// @name Bounds
				/// @{
				void blackBoxBounds(const boost::integer_range<Index>& range,
					boost::geometry::model::multi_polygon<boost::geometry::model::polygon<Point>>& bounds) const;
				presentation::FlowRelativeFourSides<Scalar> bounds() const BOOST_NOEXCEPT;
				presentation::FlowRelativeFourSides<Scalar> bounds(Index line) const;
				presentation::FlowRelativeFourSides<Scalar> bounds(const boost::integer_range<Index>& characterRange) const;
				Rectangle pixelBounds(const FontRenderContext& frc, const Point& at) const;
				/// @}

				/// @name Highlight Shapes
				/// @{
				void logicalHighlightShape(const boost::integer_range<Index>& range, bool includeHalfLeadings,
					const boost::optional<Rectangle>& bounds, boost::geometry::model::multi_polygon<boost::geometry::model::polygon<Point>>& shape) const;
				void logicalRangesForVisualSelection(
					const boost::integer_range<TextHit<>>& visualSelection, std::vector<boost::integer_range<Index>>& ranges) const;
				void visualHighlightShape(const boost::integer_range<TextHit<>>& range, const boost::optional<Rectangle>& bounds,
					bool includeHalfLeadings, boost::geometry::model::multi_polygon<boost::geometry::model::polygon<Point>>& shape) const;
				/// @}

				/// @name Hit Test
				/// @{
				TextHit<> hitTestCharacter(
					const presentation::FlowRelativeTwoAxes<Scalar>& point, bool* outOfBounds = nullptr) const;
				TextHit<> hitTestCharacter(
					const presentation::FlowRelativeTwoAxes<Scalar>& point,
					const presentation::FlowRelativeFourSides<Scalar>& bounds, bool* outOfBounds = nullptr) const;
				presentation::FlowRelativeTwoAxes<Scalar> hitToPoint(const TextHit<>& hit) const;
				/// @}

				/// @name Other Hit Test
				/// @{
				boost::optional<TextHit<>> nextLeftHit(Index offset) const;
//				boost::optional<TextHit<>> nextLeftHit(Index offset, CaretPolicy policy) const;
				boost::optional<TextHit<>> nextLeftHit(const TextHit<>& hit) const;
				boost::optional<TextHit<>> nextRightHit(Index offset) const;
//				boost::optional<TextHit<>> nextRightHit(Index offset, CaretPolicy policy) const;
				boost::optional<TextHit<>> nextRightHit(const TextHit<>& hit) const;
				TextHit<> visualOtherHit(const TextHit<>& hit) const;
				/// @}

				/// @name Other Coordinates
				/// @{
				Scalar lineStartEdge(Index line) const;
				std::tuple<Index, boost::optional<Direction>> locateLine(Scalar bpd,
					const boost::optional<NumericRange<Scalar>>& bounds) const BOOST_NOEXCEPT;
				/// @}

				/// @name Painting
				/// @{
				void draw(PaintContext& context, const Point& origin,
					const std::vector<const OverriddenSegment>& overriddenSegments = std::vector<const OverriddenSegment>(),
					const InlineObject* endOfLine = nullptr,
					const InlineObject* lineWrappingMark = nullptr) const BOOST_NOEXCEPT;
				/// @}

				/// @name Miscellaneous
				/// @{
				String fillToX(Scalar x) const;
				/// @}
#ifdef _DEBUG
				// debug
				void dumpRuns(std::ostream& out) const;
#endif // _DEBUG
				// TODO: Can provide 'maximum-line-rectangle', 'nominal-requested-line-rectangle'
				//       and 'per-inline-height-rectangle' for each 'line-area'?

			private:
				NumericRange<Scalar> internalExtent(const boost::integer_range<Index>& lines, bool includeHalfLeadings) const;
				TextHit<> internalHitTestCharacter(const presentation::FlowRelativeTwoAxes<Scalar>& point,
					const presentation::FlowRelativeFourSides<Scalar>* bounds, bool* outOfBounds) const;
//				void buildLineMetrics(Index line);
				typedef std::vector<std::unique_ptr<const TextRun>> RunVector;
				RunVector::const_iterator runForPosition(Index offset) const BOOST_NOEXCEPT;
				boost::iterator_range<RunVector::const_iterator> runsForLine(Index line) const;
				RunVector::const_iterator firstRunInLine(Index line) const BOOST_NOEXCEPT;
				void initialize(
					std::unique_ptr<presentation::ComputedStyledTextRunIterator> textRunStyles,
					const presentation::styles::Length::Context& lengthContext,
					const Dimension& parentContentArea,
					const FontCollection& fontCollection, const FontRenderContext& fontRenderContext);
				bool isEmpty() const BOOST_NOEXCEPT {return runs_.empty();}
				void justify(Scalar lineMeasure, TextJustification method) BOOST_NOEXCEPT;
				Point lineLeft(Index line) const;
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
				const LineMetrics& lineMetrics(Index line) const;
#endif // ASCENSION_ABANDONED_AT_VERSION_08
				std::pair<Index, Index> locateOffsets(
					Index line, Scalar ipd, bool& outside) const BOOST_NOEXCEPT;
				int nextTabStopBasedLeftEdge(Scalar x, bool right) const BOOST_NOEXCEPT;
				void reorder();
//				void rewrap();
				void stackLines(const RenderingContext2D& context,
					const presentation::styles::Length& lineHeight, const presentation::styles::Length::Context& lengthContext,
					LineBoxContain lineBoxContain, const Font& nominalFont);
				class TabSize;
				void wrap(const RenderingContext2D& context, const TabSize& tabSize,
					const presentation::styles::Length::Context& lengthContext, Scalar measure) BOOST_NOEXCEPT;
			private:
				const String& textString_;
				struct Styles {
					const presentation::ComputedTextToplevelStyle& forToplevel;
					const presentation::ComputedTextLineStyle& forLine;
					const presentation::ComputedTextRunStyle& forRun;
					Styles(
						const presentation::ComputedTextToplevelStyle& forToplevel,
						const presentation::ComputedTextLineStyle& forLine,
						const presentation::ComputedTextRunStyle& forRun)
						: forToplevel(forToplevel), forLine(forLine), forRun(forRun) BOOST_NOEXCEPT {}
				} styles_;
				RunVector runs_;
				Index numberOfLines_;	// TODO: The following 3 std.unique_ptr<T[]> members can be packed for compaction.
				std::unique_ptr<RunVector::const_iterator[]> firstRunsInLines_;	// size is numberOfLines_, or null if not wrapped
				struct Adl {
					Scalar ascent, descent, leading;
				};
				std::unique_ptr<Adl[]> lineMetrics_;		// size is numberOfLines_
				std::unique_ptr<Scalar[]> lineMeasures_;	// size is numberOfLines_, or null if not wrapped
				boost::optional<Scalar> maximumMeasure_;	// cached measure of the longest line
				friend class LineLayoutVector;
				friend class LineMetricsIterator;
//				friend class StyledSegmentIterator;
			};


			/**
			 * Returns extent (block-progression-dimension) of the line.
			 * @return A range of block-progression-dimension relative to the alignment-point
			 * @see #extentWithHalfLeadings, LineMetricsIterator#extent
			 */
			inline NumericRange<Scalar> TextLayout::extent() const {
				return extent(boost::irange(static_cast<Index>(0), numberOfLines()));
			}

			/**
			 * Returns extent (block-progression-dimension) of the line.
			 * @param lines A range of the lines. This can be empty
			 * @return A range of block-progression-dimension relative to the alignment-point
			 * @throw IndexOutOfBoundsException
			 * @see #extentWithHalfLeadings, LineMetricsIterator#extent
			 */
			inline NumericRange<Scalar> TextLayout::extent(const boost::integer_range<Index>& lines) const {
				return internalExtent(lines, false);
			}

			/**
			 * Returns extent (block-progression-dimension) of the specified lines with the leading. The leading is
			 * processed as 'half-leading's described by CSS 2.1 (http://www.w3.org/TR/CSS21/visudet.html#leading).
			 * @return A range of block-progression-dimension relative to the alignment-point
			 * @see #extent, LineMetricsIterator#extentWithHalfLeadings
			 */
			inline NumericRange<Scalar> TextLayout::extentWithHalfLeadings() const {
				return extentWithHalfLeadings(boost::irange(static_cast<Index>(0), numberOfLines()));
			}

			/**
			 * Returns extent (block-progression-dimension) of the specified lines with the leading. The leading is
			 * processed as 'half-leading's described by CSS 2.1 (http://www.w3.org/TR/CSS21/visudet.html#leading).
			 * @param lines A range of the lines. This can be empty
			 * @return A range of block-progression-dimension relative to the alignment-point
			 * @throw IndexOutOfBoundsException
			 * @see #extent, LineMetricsIterator#extentWithHalfLeadings
			 */
			inline NumericRange<Scalar> TextLayout::extentWithHalfLeadings(const boost::integer_range<Index>& lines) const {
				return internalExtent(lines, true);
			}

			/**
			 * @internal Returns the first text run in the specified visual line.
			 * @param line The visual line
			 * @return An iterator addresses the text run
			 */
			inline TextLayout::RunVector::const_iterator TextLayout::firstRunInLine(Index line) const BOOST_NOEXCEPT {
				assert(line <= numberOfLines());
				if(firstRunsInLines_.get() == nullptr) {
					assert(numberOfLines() == 1);
					return std::begin(runs_);
				}
				return (line < numberOfLines()) ? firstRunsInLines_[line] : std::end(runs_);
			}

			/**
			 * Returns the wrapped line containing the specified offset in the logical line.
			 * @param offset The offset in this layout
			 * @return The wrapped line
			 * @throw IndexOutOfBoundsException @a offset is greater than the length of the layout
			 */
			inline Index TextLayout::lineAt(Index offset) const {
				if(offset > numberOfCharacters())
					throw IndexOutOfBoundsException("offset");
				if(numberOfLines() == 1)
					return 0;
				std::vector<Index> offsets;
				lineOffsets(offsets);
				return ascension::detail::searchBound(std::begin(offsets), std::end(offsets) - 1, offset) - std::begin(offsets);
			}
			/**
			 * Returns the length of the specified visual line.
			 * @param line The visual line
			 * @return The length of the line
			 * @throw BadPositionException @a line is greater than the count of lines
			 */
			inline Index TextLayout::lineLength(Index line) const {
				return (line < numberOfLines() - 1 ?
					lineOffset(line + 1) : numberOfCharacters()) - lineOffset(line);
			}

			/**
			 * Returns the metrics for the specified line.
			 * @param line The line number
			 * @return The line metrics
			 * @throw IndexOutOfBoundsException @a line &gt;= numberOfLines()
			 */
			inline TextLayout::LineMetricsIterator TextLayout::lineMetrics(Index line) const {
				if(line >= numberOfLines())
					throw IndexOutOfBoundsException("line");
				assert(lineMetrics_.get() != nullptr);
				return LineMetricsIterator(*this, line);
			}

			/// Returns the number of characters represented by this @c TextLayout.
			inline Index TextLayout::numberOfCharacters() const BOOST_NOEXCEPT {
				return textString_.length();
			}

			/// Returns the number of the wrapped lines.
			inline Index TextLayout::numberOfLines() const BOOST_NOEXCEPT {
				return numberOfLines_;
			}

			/**
			 * @internal Returns iterator range addresses the all text runs belong to the specified visual line.
			 * @param line The visual line
			 * @return An iterator range addresses the all text runs belong to @a line
			 */
			inline boost::iterator_range<TextLayout::RunVector::const_iterator> TextLayout::runsForLine(Index line) const BOOST_NOEXCEPT {
				assert(line < numberOfLines());
				if(firstRunsInLines_.get() == nullptr) {
					assert(numberOfLines() == 1);
					return boost::make_iterator_range(runs_);
				}
				return boost::make_iterator_range(firstRunsInLines_[line],
					(line + 1 < numberOfLines()) ? firstRunsInLines_[line + 1] : std::end(runs_));
			}

			/**
			 * Returns the ascent of the current line in user units.
			 * Ascent is distance from the baseline to 'before-edge' of the line
			 * @return The ascent in user units
			 * @see #baselineOffset, #descent, #leading
			 * @throw NoSuchElementException The iterator is done
			 */
			inline Scalar TextLayout::LineMetricsIterator::ascent() const {
				if(isDone())
					throw NoSuchElementException();
				return layout_->lineMetrics_[line()].ascent;
			}

			/**
			 * Returns the distance from the baseline of the fitst line to the one of the current line.
			 * @return The baseline distance in user units
			 * @see #baselineOffset, #descent, #leading
			 * @throw NoSuchElementException The iterator is done
			 */
			inline Scalar TextLayout::LineMetricsIterator::baselineOffset() const {
				if(isDone())
					throw NoSuchElementException();
				return baselineOffset_;
			}

			/**
			 * Returns the descent of the current line, in user units.
			 * Descent is distance from the baseline to 'after-edge' of the line
			 * @return The descent in user units
			 * @see #ascent, #baselineOffset, #leading
			 * @throw NoSuchElementException The iterator is done
			 */
			inline Scalar TextLayout::LineMetricsIterator::descent() const {
				if(isDone())
					throw NoSuchElementException();
				return layout_->lineMetrics_[line()].descent;
			}

			/// @internal Implements relational and subtract operators.
			inline TextLayout::LineMetricsIterator::difference_type TextLayout::LineMetricsIterator::distance_to(const LineMetricsIterator& other) const {
				const bool done = isDone(), otherIsDone = other.isDone();
				if(done != otherIsDone)
					return done ? (other.layout_->numberOfLines() - other.line()) : (line() - layout_->numberOfLines());
				if(done)
					return 0;
				if(layout_ != other.layout_)
					throw std::invalid_argument("other");
				return line() - other.line();
			}

			/// @internal Implements equality-operators.
			inline bool TextLayout::LineMetricsIterator::equal(const LineMetricsIterator& other) const {
				const bool done = isDone(), otherIsDone = other.isDone();
				if(done != otherIsDone)
					return false;
				if(done)
					return true;
				if(layout_ != other.layout_)
					throw std::invalid_argument("other");
				return line() == other.line();
			}

			/**
			 * Returns the extent of the current line in block-progression-dimension.
			 * @return The extent range by the distance from the baseline of the fitst line, in user units
			 * @see #ascent, #descent, #extentWithHalfLeadings, #height, #leading, TextLayout#extent
			 * @throw NoSuchElementException The iterator is done
			 */
			inline NumericRange<Scalar> TextLayout::LineMetricsIterator::extent() const {
				const Scalar bsln = baselineOffset();	// may throw NoSuchElementException
				return !isNegativeVertical() ?
					nrange(bsln - layout_->lineMetrics_[line()].ascent, bsln + layout_->lineMetrics_[line()].descent)
					: nrange(bsln - layout_->lineMetrics_[line()].descent, bsln + layout_->lineMetrics_[line()].ascent);
			}

			/**
			 * Returns the extent of the current line in block-progression-dimension with leading. The leading is
			 * processed as 'half-leading's described by CSS 2.1 (http://www.w3.org/TR/CSS21/visudet.html#leading).
			 * @return The extent range by the distance from the baseline of the fitst line, in user units
			 * @see #ascent, #descent, #extent, #height, #leading, TextLayout#extentWithHalfLeadings
			 * @throw NoSuchElementException The iterator is done
			 */
			inline NumericRange<Scalar> TextLayout::LineMetricsIterator::extentWithHalfLeadings() const {
				Scalar lineUnder = *boost::const_end(extent());	// may throw NoSuchElementException
				lineUnder += layout_->lineMetrics_[line()].leading / 2;
				return nrange(lineUnder - height(), lineUnder);
			}

			/**
			 * Returns the height of the current line. Height is sum of 'ascent', 'descent' and 'leading'.
			 * @return The height in user units
			 * @see #extentWithHalfLeadings
			 * @throw NoSuchElementException The iterator is done
			 */
			inline Scalar TextLayout::LineMetricsIterator::height() const {
				if(isDone())
					throw NoSuchElementException();
				return layout_->lineMetrics_[line()].ascent + layout_->lineMetrics_[line()].descent + layout_->lineMetrics_[line()].leading;
			}

			/**
			 * Returns the leading of the current line in user units.
			 * Leading is distance from after-edge of the line to 'before-edge' of the next line.
			 * @return The leading in user units
			 * @see #ascent, #baselineOffset, #descent
			 * @throw NoSuchElementException The iterator is done
			 */
			inline Scalar TextLayout::LineMetricsIterator::leading() const {
				if(isDone())
					throw NoSuchElementException();
				return layout_->lineMetrics_[line()].leading;
			}

			/// Returns the line number of the current line.
			inline Index TextLayout::LineMetricsIterator::line() const BOOST_NOEXCEPT {
				return line_;
			}

			/**
			 * Returns @c true if the given @c TextLayout has a left-to-right base direction or @c false if it has a
			 * right-to-left base direction.
			 * @param layout The @c TextLayout object
			 * @return @c true if the base direction of @a layout is left-to-right; @c false otherwise
			 */
			inline BOOST_CONSTEXPR bool isLeftToRight(const TextLayout& layout) BOOST_NOEXCEPT {
				return layout.characterLevel() % 2 == 0;
			}
		}
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_TEXT_LAYOUT_HPP
