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
				VisualLine() /*noexcept*/ {}
				/**
				 * Constructor takes initial values.
				 * @param line The logical line number
				 * @param subline The visual offset in the logical line
				 */
				VisualLine(Index line, Index subline) /*noexcept*/ : line(line), subline(subline) {}
				Index line;		///< The logical line number.
				Index subline;	///< The visual offset in the logical line.
			};
			/// The equality operator.
			inline bool operator==(const VisualLine& lhs, const VisualLine& rhs) /*noexcept*/ {
				return lhs.line == rhs.line && lhs.subline == rhs.subline;
			}
			/// The less-than operator.
			inline bool operator<(const VisualLine& lhs, const VisualLine& rhs) /*noexcept*/ {
				return lhs.line < rhs.line || (lhs.line == rhs.line && lhs.subline < rhs.subline);
			}

			/**
			 * The @c InlineObject represents an inline object in @c TextLayout.
			 */
			class InlineObject {
			public:
				/// Destructor.
				virtual ~InlineObject() /*noexcept*/ {}
				/// Returns the advance (width) of this inline object in pixels.
				virtual Scalar advance() const /*noexcept*/ = 0;
				/// Returns the ascent of this inline object in pixels.
				virtual Scalar ascent() const /*noexcept*/ = 0;
				/// Returns the descent of this inline object in pixels.
				virtual Scalar descent() const /*noexcept*/ = 0;
				/**
				 * Renders this inline object at the specified location.
				 * @param context The graphic context
				 * @param origin The location where this inline object is rendered
				 */
				virtual void draw(PaintContext& context, const NativePoint& origin) /*noexcept*/ = 0;
				/// Returns the size of this inline object in pixels.
				NativeSize size() const /*noexcept*/ {
					return geometry::make<NativeSize>(advance(), ascent() + descent());
				}
			};

			/**
			 * @see Font#Metrics
			 */
			class LineMetrics {
			public:
				/// Destructor.
				virtual ~LineMetrics() /*noexcept*/ {}
				/// Returns the ascent of the text in pixels.
				virtual Scalar ascent() const /*noexcept*/ = 0;
				/// Returns the dominant baseline of the text.
				virtual DominantBaseline baseline() const /*noexcept*/ = 0;
				/// Returns the baseline offset od the text, relative to the baseline of the text.
				virtual Scalar baselineOffset(AlignmentBaseline baseline) const /*noexcept*/ = 0;
				/// Returns the descent of the text in pixels.
				virtual Scalar descent() const /*noexcept*/ = 0;
				/// Returns the height of the text in pixels.
				Scalar height() const /*noexcept*/ {return ascent() + descent()/* + leading()*/;}
//				/// Returns the leading of the text in pixels.
//				virtual Scalar leading() const /*noexcept*/ = 0;
			};

//			class ComputedStyledTextRunIterator;
//			struct ComputedTextLineStyle;
//			class TextPaintOverride;
//			class TabExpander;
			class TextRun;

			class TextLayout {
				ASCENSION_NONCOPYABLE_TAG(TextLayout);
			public:
				/**
				 * Edge of a character.
				 * @see #location
				 */
				enum Edge {
					LEADING,	///< Leading edge of a character.
					TRAILING	///< Trailing edge of a character.
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
				// constructors
				TextLayout(const String& textString, const ComputedTextLineStyle& lineStyle,
					std::unique_ptr<ComputedStyledTextRunIterator> textRunStyles);
				~TextLayout() /*throw()*/;
				// general attributes
				presentation::TextAnchor anchor() const /*throw()*/;
				uint8_t bidiEmbeddingLevel(Index offsetInLine) const;
				bool isBidirectional() const /*throw()*/;
				const presentation::TextLineStyle& style() const /*throw()*/;
				const presentation::WritingMode& writingMode() const /*throw()*/;
				// visual line accesses
				Index numberOfLines() const /*noexcept*/;
				Index lineAt(Index offsetInLine) const;
				Index lineLength(Index line) const;
				Index lineOffset(Index line) const;
				const Index* lineOffsets() const /*noexcept*/;
				// bounds, extents and measures
				NativeRegion blackBoxBounds(const Range<Index>& range) const;
				presentation::FlowRelativeFourSides<Scalar> bounds() const /*noexcept*/;
				presentation::FlowRelativeFourSides<Scalar> bounds(const Range<Index>& characterRange) const;
				Range<Scalar> extent() /*noexcept*/ const;
				Range<Scalar> extent(const Range<Index>& lines) const;
				NativeRectangle lineBounds(Index line) const;
				Scalar measure() const /*throw()*/;
				Scalar measure(Index line) const;
				// other coordinates
				Scalar baseline(Index line) const;
				const LineMetrics& lineMetrics(Index line) const;
				Scalar lineStartEdge(Index line) const;
				Index locateLine(Scalar bpd, bool& outside) const /*throw()*/;
				NativePoint location(Index offsetInLine, Edge edge = LEADING) const;
				std::pair<NativePoint, NativePoint> locations(Index offsetInLine) const;
				std::pair<Index, Index> offset(const NativePoint& p, bool* outside = nullptr) const /*throw()*/;
				// styled segments
//				StyledSegmentIterator firstStyledSegment() const /*throw()*/;
//				StyledSegmentIterator lastStyledSegment() const /*throw()*/;
//				presentation::StyledTextRun styledTextRun(Index offsetInLine) const;
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
				std::size_t findRunForPosition(Index offsetInLine) const /*throw()*/;
				bool isEmpty() const /*noexcept*/ {return runs_.empty();}
				void justify(presentation::TextJustification method) /*throw()*/;
				std::pair<Index, Index> locateOffsets(
					Index line, Scalar ipd, bool& outside) const /*throw()*/;
				void locations(Index offsetInLine, NativePoint* leading, NativePoint* trailing) const;
				int nextTabStopBasedLeftEdge(Scalar x, bool right) const /*throw()*/;
				void reorder() /*throw()*/;
//				void rewrap();
				void stackLines(
					presentation::LineBoxContain lineBoxContain,
					const Font& nominalFont, Scalar lineHeight);
				void wrap(const TabExpander& tabExpander) /*throw()*/;
			private:
				const String& textString_;
				boost::flyweight<ComputedTextLineStyle> lineStyle_;
				std::vector<std::unique_ptr<TextRun>> runs_;
				class LineArea;
				std::unique_ptr<const Index[]> lineOffsets_;	// size is numberOfLines_
				std::unique_ptr<const Index[]> lineFirstRuns_;	// size is numberOfLines_
				static const Index SINGLE_LINE_OFFSETS;
				Index numberOfLines_;
				std::unique_ptr<LineMetrics*[]> lineMetrics_;
				std::unique_ptr<Scalar[]> measures_;
				boost::optional<Scalar> maximumMeasure_;
				friend class LineLayoutVector;
//				friend class StyledSegmentIterator;
			};


			/**
			 * Returns extent (block-progression-dimension) of the line.
			 * @return A range of block-progression-dimension relative to the alignment-point
			 */
			inline Range<Scalar> TextLayout::extent() const /*noexcept*/ {
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
			 * @param offsetInLine The offset in the line
			 * @return The wrapped line
			 * @throw kernel#BadPositionException @a offsetInLine is greater than the length of the line
			 */
			inline Index TextLayout::lineAt(Index offsetInLine) const {
				if(offsetInLine > textString_.length())
					throw kernel::BadPositionException(kernel::Position(0, offsetInLine));
				return (numberOfLines() == 1) ? 0 :
					*detail::searchBound(lineOffsets(), lineOffsets() + numberOfLines(), offsetInLine);
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
			 * Returns the offset of the start of the specified visual line from the start of the
			 * logical line.
			 * @param line The visual line
			 * @return The offset
			 * @throw BadPositionException @a line is greater than the count of lines
			 */
			inline Index TextLayout::lineOffset(Index line) const {
				if(line >= numberOfLines())
					throw kernel::BadPositionException(kernel::Position(line, 0));
				return lineOffsets()[line];
			}

			/**
			 * Returns the line offsets.
			 * @return The line offsets whose length is @c #numberOfLines(). Each element in the
			 *         array is the offset for the first character in a line
			 */
			inline const Index* TextLayout::lineOffsets() const /*noexcept*/ {
				return lineOffsets_.get();
			}

			/**
			 * Returns the location for the specified character offset.
			 * @param offsetInLine The character offset in the line
			 * @param edge The edge of the character to locate
			 * @return The location. X-coordinate is distance from the left edge of the renderer,
			 *         y-coordinate is relative in the visual lines
			 * @throw kernel#BadPositionException @a offsetInLine is greater than the length of the
			 *                                    line
			 */
			inline NativePoint TextLayout::location(Index offsetInLine, Edge edge /* = LEADING */) const {
				NativePoint result;
				locations(offsetInLine, (edge == LEADING) ? &result : nullptr, (edge == TRAILING) ? &result : nullptr);
				return result;
			}

			/**
			 * Returns the locations for the specified character offset.
			 * @param offsetInLine The character offset in the line
			 * @return A pair consists of the locations. The first element means the leading, the
			 *         second element means the trailing position of the character. X-coordinates
			 *         are distances from the left edge of the renderer, y-coordinates are relative
			 *         in the visual lines
			 * @throw kernel#BadPositionException @a offsetInLine is greater than the length of the
			 *                                    line
			 */
			inline std::pair<NativePoint, NativePoint> TextLayout::locations(Index offsetInLine) const {
				std::pair<NativePoint, NativePoint> result;
				locations(offsetInLine, &result.first, &result.second);
				return result;
			}

			/// Returns the number of the wrapped lines.
			inline Index TextLayout::numberOfLines() const /*noexcept*/ {return numberOfLines_;}

		}
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_TEXT_LAYOUT_HPP
