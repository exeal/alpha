/**
 * @file text-layout.hpp
 * @date 2003-2006 (was LineLayout.h)
 * @date 2006-2010
 * @date 2010-11-20 renamed from ascension/layout.hpp
 * @date 2011-2012
 */

#ifndef ASCENSION_TEXT_LAYOUT_HPP
#define ASCENSION_TEXT_LAYOUT_HPP

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION
#include <ascension/corelib/utility.hpp>	// detail.searchBound
#include <ascension/graphics/color.hpp>
#include <ascension/graphics/font/text-hit.hpp>
#include <ascension/graphics/font/text-layout-styles.hpp>
#include <memory>	// std.unique_ptr
#include <vector>
#include <boost/flyweight.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>
#include <boost/operators.hpp>

namespace ascension {

	namespace presentation {
		class Presentation;
	}

	namespace graphics {

		class Paint;
		class PaintContext;

		namespace font {

			struct VisualLine : private boost::totally_ordered<VisualLine> {
				/// Default constructor.
				VisualLine() BOOST_NOEXCEPT {}
				/**
				 * Constructor takes initial values.
				 * @param line The logical line number
				 * @param subline The visual offset in the logical line
				 */
				VisualLine(Index line, Index subline) BOOST_NOEXCEPT : line(line), subline(subline) {}
				Index line;		///< The logical line number.
				Index subline;	///< The visual offset in the logical line.
			};
			/// The equality operator.
			inline bool operator==(const VisualLine& lhs, const VisualLine& rhs) BOOST_NOEXCEPT {
				return lhs.line == rhs.line && lhs.subline == rhs.subline;
			}
			/// The less-than operator.
			inline bool operator<(const VisualLine& lhs, const VisualLine& rhs) BOOST_NOEXCEPT {
				return lhs.line < rhs.line || (lhs.line == rhs.line && lhs.subline < rhs.subline);
			}

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

//			class ComputedStyledTextRunIterator;
//			struct ComputedTextLineStyle;
//			class TextPaintOverride;
//			class TabExpander;
			class TextRun;

			class TextLayout {
				ASCENSION_NONCOPYABLE_TAG(TextLayout);
			public:
				/// 
				class LineMetricsIterator : public boost::iterator_facade<LineMetricsIterator, std::nullptr_t, boost::bidirectional_traversal_tag> {
				public:
					LineMetricsIterator(const TextLayout& layout, Index line);
					Scalar ascent() const;
					DominantBaseline baseline() const;
					Scalar baselineOffset() const;
					Point baselineOffsetInPhysicalCoordinates() const;
					Scalar descent() const;
					Scalar leading() const;
					Index line() const BOOST_NOEXCEPT;
				private:
					void decrement();
					difference_type distance_to(const LineMetricsIterator& other) const;
					bool equal(const LineMetricsIterator& other) const;
					void increment();
				private:
					const TextLayout& layout_;
					Index line_;
					Scalar baselineOffset_;
					friend class boost::iterator_core_access;
				};
#if 0
				/// Bidirectional iterator enumerates style runs in a line.
				class StyledSegmentIterator {
				public:
					// constructors
					StyledSegmentIterator(const StyledSegmentIterator& rhs) /*throw()*/;
					// operators
					StyledSegmentIterator& operator=(const StyledSegmentIterator& rhs) /*throw()*/;
					// methods
					presentation::StyledRun current() const /*throw()*/;
					bool equals(const StyledSegmentIterator& rhs) const /*throw()*/;
					StyledSegmentIterator& next() /*throw()*/;
					StyledSegmentIterator& previous() /*throw()*/;
				private:
					explicit StyledSegmentIterator(const internal::Run*& start) /*throw()*/;
					const internal::Run** p_;
					friend class LineLayout;
				};
#endif
			public:
				TextLayout(const String& textString,
					const ComputedTextLineStyle& lineStyle,
					std::unique_ptr<ComputedStyledTextRunIterator> textRunStyles,
					const FontCollection& fontCollection,
					const FontRenderContext& fontRenderContext);
				~TextLayout() BOOST_NOEXCEPT;

				/// @name General Attributes
				/// @{
				presentation::TextAnchor anchor(Index line) const;
				std::uint8_t characterLevel() const BOOST_NOEXCEPT;
				std::uint8_t characterLevel(Index offset) const;
				bool isBidirectional() const BOOST_NOEXCEPT;
				Index numberOfCharacters() const BOOST_NOEXCEPT;
				const presentation::TextLineStyle& style() const /*throw()*/;
				const presentation::WritingMode& writingMode() const BOOST_NOEXCEPT;
				/// @}

				/// @name Visual Line Accesses
				/// @{
				Index lineAt(Index offset) const;
				Index lineLength(Index line) const;
				Index lineOffset(Index line) const;
				std::vector<Index>&& lineOffsets() const BOOST_NOEXCEPT;
				Index numberOfLines() const BOOST_NOEXCEPT;
				/// @}

				/// @name Metrics
				/// @{
				LineMetricsIterator&& lineMetrics(Index line) const;
				boost::integer_range<Scalar> extent() const;
				boost::integer_range<Scalar> extent(const boost::integer_range<Index>& lines) const;
				Scalar measure() const BOOST_NOEXCEPT;
				Scalar measure(Index line) const;
				/// @}

				/// @name Bounds
				/// @{
				boost::geometry::model::multi_polygon<
					boost::geometry::model::polygon<Point>
				>&& blackBoxBounds(const boost::integer_range<Index>& range) const;
				presentation::FlowRelativeFourSides<Scalar> bounds() const BOOST_NOEXCEPT;
				presentation::FlowRelativeFourSides<Scalar> bounds(Index line) const;
				presentation::FlowRelativeFourSides<Scalar> bounds(const boost::integer_range<Index>& characterRange) const;
				Rectangle pixelBounds(const FontRenderContext& frc, const Point& at) const;
				/// @}

				/// @name Highlight Shapes
				/// @{
				boost::geometry::model::multi_polygon<boost::geometry::model::polygon<Point>>&&
					logicalHighlightShape(const boost::integer_range<Index>& range, const boost::optional<Rectangle>& bounds) const;
				std::vector<boost::integer_range<Index>>&& logicalRangesForVisualSelection(const boost::integer_range<TextHit>& range) const;
				boost::geometry::model::multi_polygon<boost::geometry::model::polygon<Point>>&&
					visualHighlightShape(const boost::integer_range<TextHit>& range, const boost::optional<Rectangle>& bounds) const;
				/// @}

				/// @name Hit Test
				/// @{
				TextHit&& hitTestCharacter(const presentation::AbstractTwoAxes<Scalar>& point, bool* outOfBounds = nullptr) const;
				TextHit&& hitTestCharacter(const presentation::AbstractTwoAxes<Scalar>& point, const presentation::FlowRelativeFourSides<Scalar>& bounds, bool* outOfBounds = nullptr) const;
				presentation::AbstractTwoAxes<Scalar> hitToPoint(const TextHit& hit) const;
				/// @}

				/// @name Other Hit Test
				/// @{
				TextHit visualOtherHit(const TextHit& hit) const;
				/// @}

				/// @name Other Coordinates
				/// @{
				Scalar lineStartEdge(Index line) const;
				Index locateLine(Scalar bpd, bool& outside) const /*throw()*/;
				presentation::AbstractTwoAxes<Scalar> location(const TextHit& hit) const;
				std::pair<presentation::AbstractTwoAxes<Scalar>,
					presentation::AbstractTwoAxes<Scalar>> locations(Index offset) const;
				/// @}

				// styled segments
//				StyledSegmentIterator firstStyledSegment() const /*throw()*/;
//				StyledSegmentIterator lastStyledSegment() const /*throw()*/;
//				presentation::StyledTextRun styledTextRun(Index offset) const;

				/// @name Painting
				/// @{
				void draw(PaintContext& context, const Point& origin,
					const TextPaintOverride* paintOverride = nullptr,
					const InlineObject* endOfLine = nullptr,
					const InlineObject* lineWrappingMark = nullptr) const /*throw()*/;
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
				TextHit&& internalHitTestCharacter(const presentation::AbstractTwoAxes<Scalar>& point,
					const presentation::FlowRelativeFourSides<Scalar>* bounds, bool* outOfBounds) const;
//				void buildLineMetrics(Index line);
				void expandTabsWithoutWrapping() /*throw()*/;
				typedef std::vector<std::unique_ptr<const TextRun>> RunVector;
				RunVector::const_iterator runForPosition(Index offset) const /*throw()*/;
				boost::iterator_range<RunVector::const_iterator> runsForLine(Index line) const;
				RunVector::const_iterator firstRunInLine(Index line) const BOOST_NOEXCEPT;
				bool isEmpty() const BOOST_NOEXCEPT {return runs_.empty();}
				void justify(Scalar lineMeasure, presentation::TextJustification method) /*throw()*/;
				Point lineLeft(Index line) const;
				typedef std::tuple<Scalar, Scalar, Scalar/*, Scalar*/> LineMetrics;	// ascent, descent, leading/*, advance*/
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
				const LineMetrics& lineMetrics(Index line) const;
#endif // ASCENSION_ABANDONED_AT_VERSION_08
				std::pair<Index, Index> locateOffsets(
					Index line, Scalar ipd, bool& outside) const /*throw()*/;
				void locations(Index offset,
					presentation::AbstractTwoAxes<Scalar>* leading,
					presentation::AbstractTwoAxes<Scalar>* trailing) const;
				int nextTabStopBasedLeftEdge(Scalar x, bool right) const /*throw()*/;
				void reorder();
//				void rewrap();
				void stackLines(
					const RenderingContext2D& context, boost::optional<Scalar> lineHeight,
					presentation::LineBoxContain lineBoxContain, const Font& nominalFont);
				void wrap(Scalar measure, const TabExpander& tabExpander) BOOST_NOEXCEPT;
			private:
				const String& textString_;
				boost::flyweight<ComputedTextLineStyle> lineStyle_;
				RunVector runs_;
				Index numberOfLines_;	// TODO: The following 3 std.unique_ptr<T[]> members can be packed for compaction.
				std::unique_ptr<RunVector::const_iterator[]> firstRunsInLines_;	// size is numberOfLines_, or null if not wrapped
				std::unique_ptr<LineMetrics[]> lineMetrics_;	// size is numberOfLines_
				std::unique_ptr<Scalar[]> lineMeasures_;	// size is numberOfLines_, or null if not wrapped
				boost::optional<Scalar> maximumMeasure_;	// cached measure of the longest line
				friend class LineLayoutVector;
//				friend class StyledSegmentIterator;
			};



			/// Returns the base bidirectional embedding level of this @c TextLayout.
			inline std::uint8_t TextLayout::characterLevel() const BOOST_NOEXCEPT {
				return (writingMode().inlineFlowDirection == presentation::RIGHT_TO_LEFT) ? 1 : 0;
			}

			/**
			 * Returns extent (block-progression-dimension) of the line.
			 * @return A range of block-progression-dimension relative to the alignment-point
			 */
			inline boost::integer_range<Scalar> TextLayout::extent() const {
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
					return begin(runs_);
				}
				return (line < numberOfLines()) ? firstRunsInLines_[line] : end(runs_);
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
				const std::vector<Index> offsets(lineOffsets());
				return *detail::searchBound(std::begin(offsets), std::end(offsets) - 1, offset);
			}
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			/**
			 * Returns the metrics for the specified line.
			 * @param line The line number
			 * @return The line metrics
			 * @throw BadPositionException @a line is greater than the count of lines
			 */
			inline const TextLayout::LineMetrics& TextLayout::lineMetrics(Index line) const {
				if(line >= numberOfLines())
					throw kernel::BadPositionException(kernel::Position(line, 0));
				if(lineMetrics_.get() == nullptr)
					const_cast<TextLayout*>(this)->lineMetrics_.reset(new std::unique_ptr<LineMetrics>[numberOfLines()]);
				if(lineMetrics_[line] == nullptr)
					const_cast<TextLayout*>(this)->buildLineMetrics(line);
				return *lineMetrics_[line];
			}
#endif
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
			 * Returns the location for the specified character hit.
			 * @param hit The character hit in this layout
			 * @return The location of the character
			 * @throw kernel#BadPositionException @a hit is outside of the layout
			 */
			inline presentation::AbstractTwoAxes<Scalar> TextLayout::location(const TextHit& hit) const {
				presentation::AbstractTwoAxes<Scalar> result;
				locations(hit.characterIndex(), hit.isLeadingEdge() ? &result : nullptr, !hit.isLeadingEdge() ? &result : nullptr);
				return result;
			}

			/**
			 * Returns the locations for the specified character offset.
			 * @param offset The character offset in this layout
			 * @return A pair consists of the locations. The first element means the leading, the
			 *         second element means the trailing position of the character
			 * @throw kernel#BadPositionException @a offset is greater than the length of the layout
			 */
			inline std::pair<presentation::AbstractTwoAxes<Scalar>,
					presentation::AbstractTwoAxes<Scalar>> TextLayout::locations(Index offset) const {
				std::pair<presentation::AbstractTwoAxes<Scalar>, presentation::AbstractTwoAxes<Scalar>> result;
				locations(offset, &result.first, &result.second);
				return result;
			}

			/// Returns the number of characters represented by this @c TextLayout.
			inline Index TextLayout::numberOfCharacters() const BOOST_NOEXCEPT {return textString_.length();}

			/// Returns the number of the wrapped lines.
			inline Index TextLayout::numberOfLines() const BOOST_NOEXCEPT {return numberOfLines_;}

			/// Returns the writing mode for this @c TextLayout.
			inline const presentation::WritingMode& TextLayout::writingMode() const BOOST_NOEXCEPT {
				return lineStyle_.get().writingMode;
			}

			/**
			 * Returns the ascent of the current line.
			 * @return The ascent in user units
			 * @see #baselineOffset, #descent, #leading
			 * @throw NoSuchElementException The iterator is done
			 */
			inline Scalar TextLayout::LineMetricsIterator::ascent() const {
				if(line_ >= layout_.numberOfLines())
					throw NoSuchElementException();
				return std::get<0>(layout_.lineMetrics_[line_]);	// $friendly-access$
			}

			/**
			 * Returns the distance from the baseline of the fitst line to the one of the current line.
			 * @return The baseline distance in user units
			 * @see #baselineOffset, #descent, #leading
			 * @throw NoSuchElementException The iterator is done
			 */
			inline Scalar TextLayout::LineMetricsIterator::baselineOffset() const {
				if(line_ >= layout_.numberOfLines())
					throw NoSuchElementException();
				return baselineOffset_;
			}

			/**
			 */
			inline Point TextLayout::LineMetricsIterator::baselineOffsetInPhysicalCoordinates() const {
				switch(layout_.writingMode().blockFlowDirection) {
					case presentation::HORIZONTAL_TB:
						return Point(geometry::_x = static_cast<Scalar>(0), geometry::_y = baselineOffset());
					case presentation::VERTICAL_RL:
			 			return Point(geometry::_x = -baselineOffset(), geometry::_y = static_cast<Scalar>(0));
					case presentation::VERTICAL_LR:
			 			return Point(geometry::_x = +baselineOffset(), geometry::_y = static_cast<Scalar>(0));
					default:
						ASCENSION_ASSERT_NOT_REACHED();
				}
			}

			/**
			 * Returns the descent of the current line.
			 * @return The descent in user units
			 * @see #ascent, #baselineOffset, #leading
			 * @throw NoSuchElementException The iterator is done
			 */
			inline Scalar TextLayout::LineMetricsIterator::descent() const {
				if(line_ >= layout_.numberOfLines())
					throw NoSuchElementException();
				return std::get<1>(layout_.lineMetrics_[line_]);	// $friendly-access$
			}

			/**
			 * Returns the leading of the current line.
			 * @return The leading in user units
			 * @see #ascent, #baselineOffset, #descent
			 * @throw NoSuchElementException The iterator is done
			 */
			inline Scalar TextLayout::LineMetricsIterator::leading() const {
				if(line_ >= layout_.numberOfLines())
					throw NoSuchElementException();
				return std::get<2>(layout_.lineMetrics_[line_]);	// $friendly-access$
			}

		}
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_TEXT_LAYOUT_HPP
