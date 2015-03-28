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
#include <ascension/graphics/geometry/geometry.hpp>
#include <ascension/presentation/flow-relative-directions-dimensions.hpp>
#include <ascension/presentation/styles/writing-modes.hpp>
#include <boost/flyweight.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>
#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <memory>	// std.unique_ptr
#include <vector>

namespace ascension {

	namespace presentation {
		struct ComputedStyledTextRunIterator;
		struct ComputedTextLineStyle;
		struct ComputedTextToplevelStyle;
		class Presentation;
	}

	namespace graphics {

		class Paint;
		class PaintContext;

		namespace font {
			class TextLayout;

			presentation::WritingMode writingMode(const TextLayout& textLayout) BOOST_NOEXCEPT;

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

			class Font;
			class FontCollection;
			class FontRenderContext;
			class TextPaintOverride;
			template<typename Length = Scalar> class TabExpander;
			class TextRun;

			class TextLayout : private boost::noncopyable {
			public:
				/// 
				struct LineMetrics {
					/// Ascent is distance from the baseline to before-edge of the line, in user units.
					Scalar ascent;
					/// Descent is distance from the baseline to after-edge of the line, in user units.
					Scalar descent;
					/// Leading is distance from after-edge of the line to before-edge of the next line, in user units.
					Scalar leading;
//					Scalar advance;
				};

				/// 
				class LineMetricsIterator : public boost::iterators::iterator_facade<LineMetricsIterator, std::nullptr_t, boost::iterators::bidirectional_traversal_tag> {
				public:
					LineMetricsIterator() BOOST_NOEXCEPT;
					LineMetricsIterator(const TextLayout& layout, Index line);
					Scalar ascent() const;
					DominantBaseline baseline() const;
					Scalar baselineOffset() const;
					Point baselineOffsetInPhysicalCoordinates() const;
					Scalar descent() const;
					NumericRange<Scalar> extent() const;
					Scalar height() const;
					Scalar leading() const;
					Index line() const BOOST_NOEXCEPT;
				private:
					bool isDone() const BOOST_NOEXCEPT {
						return layout_ == nullptr || line() >= layout_->numberOfLines();
					}
					bool isNegativeVertical() const {
						const auto wm(writingMode(*layout_));
						if(wm.blockFlowDirection == presentation::VERTICAL_RL)
							return presentation::resolveTextOrientation(wm) == presentation::SIDEWAYS_LEFT;
						else if(wm.blockFlowDirection == presentation::VERTICAL_LR)
							return presentation::resolveTextOrientation(wm) != presentation::SIDEWAYS_LEFT;
						return false;
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
					const FontCollection& fontCollection, const FontRenderContext& fontRenderContext);
				~TextLayout() BOOST_NOEXCEPT;

				/// @name General Attributes
				/// @{
				TextAnchor anchor(Index line) const;
				std::uint8_t characterLevel() const BOOST_NOEXCEPT;
				std::uint8_t characterLevel(Index offset) const;
				bool isBidirectional() const BOOST_NOEXCEPT;
				Index numberOfCharacters() const BOOST_NOEXCEPT;
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
				const LineMetrics& lineMetrics(Index line) const;
				NumericRange<Scalar> extent() const;
				NumericRange<Scalar> extent(const boost::integer_range<Index>& lines) const;
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
				void logicalHighlightShape(const boost::integer_range<Index>& range,
					const boost::optional<Rectangle>& bounds, boost::geometry::model::multi_polygon<boost::geometry::model::polygon<Point>>& shape) const;
				void logicalRangesForVisualSelection(const boost::integer_range<TextHit<>>& visualSelection, std::vector<boost::integer_range<Index>>& ranges) const;
				void visualHighlightShape(const boost::integer_range<TextHit<>>& range,
					const boost::optional<Rectangle>& bounds, boost::geometry::model::multi_polygon<boost::geometry::model::polygon<Point>>& shape) const;
				/// @}

				/// @name Hit Test
				/// @{
				TextHit<> hitTestCharacter(const presentation::FlowRelativeTwoAxes<Scalar>& point, bool* outOfBounds = nullptr) const;
				TextHit<> hitTestCharacter(const presentation::FlowRelativeTwoAxes<Scalar>& point, const presentation::FlowRelativeFourSides<Scalar>& bounds, bool* outOfBounds = nullptr) const;
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
					const TextPaintOverride* paintOverride = nullptr,
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
				TextHit<> internalHitTestCharacter(const presentation::FlowRelativeTwoAxes<Scalar>& point,
					const presentation::FlowRelativeFourSides<Scalar>* bounds, bool* outOfBounds) const;
//				void buildLineMetrics(Index line);
				void expandTabsWithoutWrapping(const TabExpander<>& tabExpander) BOOST_NOEXCEPT;
				typedef std::vector<std::unique_ptr<const TextRun>> RunVector;
				RunVector::const_iterator runForPosition(Index offset) const BOOST_NOEXCEPT;
				boost::iterator_range<RunVector::const_iterator> runsForLine(Index line) const;
				RunVector::const_iterator firstRunInLine(Index line) const BOOST_NOEXCEPT;
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
					boost::optional<Scalar> lineHeight, LineBoxContain lineBoxContain, const Font& nominalFont);
				void wrap(Scalar measure, const TabExpander<>& tabExpander) BOOST_NOEXCEPT;
			private:
				const String& textString_;
				struct Styles;
				std::unique_ptr<Styles> styles_;
				RunVector runs_;
				Index numberOfLines_;	// TODO: The following 3 std.unique_ptr<T[]> members can be packed for compaction.
				std::unique_ptr<RunVector::const_iterator[]> firstRunsInLines_;	// size is numberOfLines_, or null if not wrapped
				std::unique_ptr<LineMetrics[]> lineMetrics_;	// size is numberOfLines_
				std::unique_ptr<Scalar[]> lineMeasures_;	// size is numberOfLines_, or null if not wrapped
				boost::optional<Scalar> maximumMeasure_;	// cached measure of the longest line
				friend class LineLayoutVector;
//				friend class StyledSegmentIterator;
			};


			/**
			 * Returns extent (block-progression-dimension) of the line.
			 * @return A range of block-progression-dimension relative to the alignment-point
			 */
			inline NumericRange<Scalar> TextLayout::extent() const {
				return extent(boost::irange(static_cast<Index>(0), numberOfLines()));
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
				return *ascension::detail::searchBound(std::begin(offsets), std::end(offsets) - 1, offset);
			}
			/**
			 * Returns the length of the specified visual line.
			 * @param line The visual line
			 * @return The length of the line
			 * @throw BadPositionException @a line is greater than the count of lines
			 */
			inline Index TextLayout::lineLength(Index line) const {
				return (line < numberOfLines_ - 1 ?
					lineOffset(line + 1) : numberOfCharacters()) - lineOffset(line);
			}

			/**
			 * Returns the metrics for the specified line.
			 * @param line The line number
			 * @return The line metrics
			 * @throw IndexOutOfBoundsException @a line &gt;= numberOfLines()
			 */
			inline const TextLayout::LineMetrics& TextLayout::lineMetrics(Index line) const {
				if(line >= numberOfLines())
					throw IndexOutOfBoundsException("line");
				assert(lineMetrics_.get() != nullptr);
				return lineMetrics_[line];
			}

			/// Returns the number of characters represented by this @c TextLayout.
			inline Index TextLayout::numberOfCharacters() const BOOST_NOEXCEPT {return textString_.length();}

			/// Returns the number of the wrapped lines.
			inline Index TextLayout::numberOfLines() const BOOST_NOEXCEPT {return numberOfLines_;}

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
			 * Returns the ascent of the current line.
			 * @return The ascent in user units
			 * @see #baselineOffset, #descent, #leading
			 * @throw NoSuchElementException The iterator is done
			 */
			inline Scalar TextLayout::LineMetricsIterator::ascent() const {
				if(isDone())
					throw NoSuchElementException();
				return layout_->lineMetrics(line_).ascent;
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
			 * Returns the descent of the current line.
			 * @return The descent in user units
			 * @see #ascent, #baselineOffset, #leading
			 * @throw NoSuchElementException The iterator is done
			 */
			inline Scalar TextLayout::LineMetricsIterator::descent() const {
				if(isDone())
					throw NoSuchElementException();
				return layout_->lineMetrics(line_).descent;
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
			 * @see #ascent, #descent, #height, #leading
			 * @throw NoSuchElementException The iterator is done
			 * @see TextLayout#extent
			 */
			inline NumericRange<Scalar> TextLayout::LineMetricsIterator::extent() const {
				const Scalar bsln = baselineOffset();	// may throw NoSuchElementException
				return !isNegativeVertical() ?
					nrange(bsln - ascent(), bsln + descent() + leading())
			    	: nrange(bsln - descent(), bsln + ascent() + leading());	// TODO: leading is there?
			}

			/**
			 * Returns the height of the current line.
			 * @return The height in user units
			 * @see #extent
			 * @throw NoSuchElementException The iterator is done
			 */
			inline Scalar TextLayout::LineMetricsIterator::height() const {
//				const NumericRange<Scalar> e(extent() | adaptors::ordered());
				const NumericRange<Scalar> e(extent());	// may throw NoSuchElementException
				return *e.end() - *e.begin();
			}

			/**
			 * Returns the leading of the current line.
			 * @return The leading in user units
			 * @see #ascent, #baselineOffset, #descent
			 * @throw NoSuchElementException The iterator is done
			 */
			inline Scalar TextLayout::LineMetricsIterator::leading() const {
				if(isDone())
					throw NoSuchElementException();
				return layout_->lineMetrics(line_).leading;
			}

			/// Returns the line number of the current line.
			inline Index TextLayout::LineMetricsIterator::line() const BOOST_NOEXCEPT {
				return line_;
			}
		}
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_TEXT_LAYOUT_HPP
