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
#include <ascension/graphics/text-hit-information.hpp>
#include <ascension/graphics/text-layout-styles.hpp>
#include <ascension/kernel/position.hpp>
#include <memory>	// std.unique_ptr
#include <vector>
#include <boost/flyweight.hpp>
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
				virtual void draw(PaintContext& context, const NativePoint& origin) BOOST_NOEXCEPT = 0;
				/// Returns the size of this inline object in pixels.
				NativeSize size() const BOOST_NOEXCEPT {
					return geometry::make<NativeSize>(advance(), ascent() + descent());
				}
			};

			/**
			 * @see Font#Metrics
			 */
			class LineMetrics {
			public:
				/// Destructor.
				virtual ~LineMetrics() BOOST_NOEXCEPT {}
				/// Returns the ascent of the text in pixels.
				virtual Scalar ascent() const BOOST_NOEXCEPT = 0;
				/// Returns the dominant baseline of the text.
				virtual DominantBaseline baseline() const BOOST_NOEXCEPT = 0;
				/// Returns the baseline offset od the text, relative to the baseline of the text.
				virtual Scalar baselineOffset(AlignmentBaseline baseline) const BOOST_NOEXCEPT = 0;
				/// Returns the descent of the text in pixels.
				virtual Scalar descent() const BOOST_NOEXCEPT = 0;
				/// Returns the height of the text in pixels.
				Scalar height() const BOOST_NOEXCEPT {return ascent() + descent()/* + leading()*/;}
//				/// Returns the leading of the text in pixels.
//				virtual Scalar leading() const BOOST_NOEXCEPT = 0;
			};

//			class ComputedStyledTextRunIterator;
//			struct ComputedTextLineStyle;
//			class TextPaintOverride;
//			class TabExpander;
			class TextRun;

			class TextLayout {
				ASCENSION_NONCOPYABLE_TAG(TextLayout);
			public:
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
				// constructors
				TextLayout(const String& textString, const ComputedTextLineStyle& lineStyle,
					std::unique_ptr<ComputedStyledTextRunIterator> textRunStyles);
				~TextLayout() /*throw()*/;
				// general attributes
				presentation::TextAnchor anchor(Index line) const;
				std::uint8_t characterLevel(Index offset) const;
				bool isBidirectional() const BOOST_NOEXCEPT;
				const presentation::TextLineStyle& style() const /*throw()*/;
				const presentation::WritingMode& writingMode() const /*throw()*/;
				// visual line accesses
				Index numberOfLines() const BOOST_NOEXCEPT;
				Index lineAt(Index offset) const;
				Index lineLength(Index line) const;
				Index lineOffset(Index line) const;
				std::vector<Index>&& lineOffsets() const BOOST_NOEXCEPT;
				// bounds, extents and measures
				NativeRegion blackBoxBounds(const Range<Index>& range) const;
				presentation::FlowRelativeFourSides<Scalar> bounds() const BOOST_NOEXCEPT;
				presentation::FlowRelativeFourSides<Scalar> bounds(const Range<Index>& characterRange) const;
				Range<Scalar> extent() BOOST_NOEXCEPT const;
				Range<Scalar> extent(const Range<Index>& lines) const;
				presentation::FlowRelativeFourSides<Scalar> lineBounds(Index line) const;
				Scalar measure() const BOOST_NOEXCEPT;
				Scalar measure(Index line) const;
				// other coordinates
				Scalar baseline(Index line) const;
				const LineMetrics& lineMetrics(Index line) const;
				Scalar lineStartEdge(Index line) const;
				Index locateLine(Scalar bpd, bool& outside) const /*throw()*/;
				presentation::AbstractTwoAxes<Scalar> location(const TextHitInformation& hit) const;
				std::pair<presentation::AbstractTwoAxes<Scalar>,
					presentation::AbstractTwoAxes<Scalar>> locations(Index offset) const;
				std::pair<Index, Index> offset(const NativePoint& p, bool* outside = nullptr) const /*throw()*/;
				// styled segments
//				StyledSegmentIterator firstStyledSegment() const /*throw()*/;
//				StyledSegmentIterator lastStyledSegment() const /*throw()*/;
//				presentation::StyledTextRun styledTextRun(Index offset) const;
				// painting
				void draw(PaintContext& context, const NativePoint& origin,
					const TextPaintOverride* paintOverride = nullptr,
					const InlineObject* endOfLine = nullptr,
					const InlineObject* lineWrappingMark = nullptr) const /*throw()*/;
				// miscellaneous
				String fillToX(Scalar x) const;
#ifdef _DEBUG
				// debug
				void dumpRuns(std::ostream& out) const;
#endif // _DEBUG

			private:
				void expandTabsWithoutWrapping() /*throw()*/;
				typedef std::vector<std::unique_ptr<const TextRun>> RunVector;
				RunVector::const_iterator runForPosition(Index offset) const /*throw()*/;
				RunVector::const_iterator firstRunInLine(Index line) const BOOST_NOEXCEPT;
				bool isEmpty() const BOOST_NOEXCEPT {return runs_.empty();}
				void justify(Scalar lineMeasure, presentation::TextJustification method) /*throw()*/;
				std::pair<Index, Index> locateOffsets(
					Index line, Scalar ipd, bool& outside) const /*throw()*/;
				void locations(Index offset,
					presentation::AbstractTwoAxes<Scalar>* leading,
					presentation::AbstractTwoAxes<Scalar>* trailing) const;
				int nextTabStopBasedLeftEdge(Scalar x, bool right) const /*throw()*/;
				void reorder() /*throw()*/;
//				void rewrap();
				void stackLines(boost::optional<Scalar> lineHeight,
					presentation::LineBoxContain lineBoxContain, const Font& nominalFont);
				void wrap(Scalar measure, const TabExpander& tabExpander) BOOST_NOEXCEPT;
			private:
				const String& textString_;
				boost::flyweight<ComputedTextLineStyle> lineStyle_;
				RunVector runs_;
				class LineArea;
				std::unique_ptr<RunVector::const_iterator[]> firstRunsInLines_;	// size is runs_.size(), or null if not wrapped
				Index numberOfLines_;
				std::unique_ptr<LineMetrics*[]> lineMetrics_;
				std::unique_ptr<Scalar[]> measures_;
				boost::optional<Scalar> maximumMeasure_;	// cached measure of the longest line
				friend class LineLayoutVector;
//				friend class StyledSegmentIterator;
			};


			/**
			 * Returns extent (block-progression-dimension) of the line.
			 * @return A range of block-progression-dimension relative to the alignment-point
			 */
			inline Range<Scalar> TextLayout::extent() const BOOST_NOEXCEPT {
				return makeRange(
					baseline(0) - lineMetrics_[0]->ascent(),
					baseline(numberOfLines() - 1) + lineMetrics_[numberOfLines() - 1]->descent());
			}

			/**
			 * Returns extent (block-progression-dimension) of the specified lines.
			 * @param lines A range of the lines. @a lines.end() is exclusive
			 * @return A range of block-progression-dimension relative to the alignment-point
			 * @throw kernel#BadRegionException
			 */
			inline Range<Scalar> TextLayout::extent(const Range<Index>& lines) const {
				if(lines.end() >= numberOfLines())
					throw kernel::BadRegionException(kernel::Region(
						kernel::Position(lines.beginning(), 0), kernel::Position(lines.end(), 0)));
				return makeRange(
					baseline(lines.beginning()) - lineMetrics_[lines.beginning()]->ascent(),
					baseline(lines.end() - 1) + lineMetrics_[lines.end() - 1]->descent());
			}

			/**
			 * Returns the wrapped line containing the specified offset in the logical line.
			 * @param offset The offset in this layout
			 * @return The wrapped line
			 * @throw kernel#BadPositionException @a offset is greater than the length of the layout
			 */
			inline Index TextLayout::lineAt(Index offset) const {
				if(offset > textString_.length())
					throw kernel::BadPositionException(kernel::Position(0, offset));
				if(numberOfLines() == 1)
					return 0;
				const std::vector<Index> offsets(lineOffsets());
				return *detail::searchBound(std::begin(offsets), std::end(offsets) - 1, offset);
			}

			/**
			 * Returns the metrics for the specified line.
			 * @param line The line number
			 * @return The line metrics
			 * @throw BadPositionException @a line is greater than the count of lines
			 */
			inline const LineMetrics& TextLayout::lineMetrics(Index line) const {
				if(line >= numberOfLines())
					throw kernel::BadPositionException(kernel::Position(line, 0));
				return *lineMetrics_[line];
			}

			/**
			 * Returns the length of the specified visual line.
			 * @param line The visual line
			 * @return The length of the line
			 * @throw BadPositionException @a line is greater than the count of lines
			 */
			inline Index TextLayout::lineLength(Index line) const {
				return (line < numberOfLines_ - 1 ?
					lineOffset(line + 1) : textString_.length()) - lineOffset(line);
			}

			/**
			 * Returns the location for the specified character hit.
			 * @param hit The character hit in this layout
			 * @return The location of the character
			 * @throw kernel#BadPositionException @a hit is outside of the layout
			 */
			inline presentation::AbstractTwoAxes<Scalar> TextLayout::location(const TextHitInformation& hit) const {
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

			/// Returns the number of the wrapped lines.
			inline Index TextLayout::numberOfLines() const BOOST_NOEXCEPT {return numberOfLines_;}

		}
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_TEXT_LAYOUT_HPP
