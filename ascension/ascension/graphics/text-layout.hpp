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
#include <ascension/kernel/document.hpp>
#include <ascension/graphics/color.hpp>
#include <ascension/presentation/text-style.hpp>
#include <limits>	// std.numeric_limits
#include <memory>	// std.unique_ptr
#include <vector>
#include <boost/operators.hpp>

namespace ascension {

	namespace presentation {class Presentation;}
	namespace viewers {class Caret;}

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
			 * @see TextLayout#TextLayout
			 * @note This interface is designed based on @c TabExpander interface of Java.
			 */
			class TabExpander {
			public:
				/// Destructor.
				virtual ~TabExpander() /*noexcept*/ {}
				/**
				 * Returns the next tab stop position given a reference position.
				 * @param x The position in pixels
				 * @param tabOffset The position within the underlying text that the tab occured
				 * @return The next tab stop. Should be greater than @a x
				 */
				virtual Scalar nextTabStop(Scalar x, Index tabOffset) const = 0;
			};

			/// Standard implementation of @c TabExpander with fixed width tabulations.
			class FixedWidthTabExpander : public TabExpander {
			public:
				explicit FixedWidthTabExpander(Scalar width) /*noexcept*/;
				Scalar nextTabStop(Scalar x, Index tabOffset) const;
			private:
				const Scalar width_;
			};

			class TextPaintOverride {
			public:
				class Iterator {
				public:
					/// Destructor.
					virtual ~Iterator() /*noexcept*/ {}

					/**
					 * Returns the overridden foreground of the current position.
					 * @return The overridden foreground or @c null if does not override
					 * @throw NoSuchElementException The iterator is end
					 */
					virtual const Paint* foreground() const = 0;
					/**
					 * Returns the overridden transparency of the foreground of the current
					 * position.
					 * @return The transparency. This value should be in the range from 0.0 (fully
					 *         transparent) to 1.0 (no additional transparency)
					 */
					virtual double foregroundAlpha() const = 0;
					/**
					 * Returns the overridden background of the current position.
					 * @return The overridden background or @c null if does not override
					 * @throw NoSuchElementException The iterator is end
					 */
					virtual const Paint* background() const = 0;
					/**
					 * Returns the transparency of the overridden background of the current
					 * position.
					 * @return The transparency. This value should be in the range from 0.0 (fully
					 *         transparent) to 1.0 (no additional transparency)
					 */
					virtual double backgroundAlpha() const = 0;
					/**
					 * Returns @c false to paint only the glyphs' bounds with @c #background.
					 * Otherwise the logical highlight bounds of characters are painted as
					 * background.
					 * @throw NoSuchElementException The iterator is end
					 */
					virtual bool usesLogicalHighlightBounds() const = 0;

					/// Returns the length of the current text segment.
					virtual Index length() const = 0;
					/// Returns @c true if the iterator has no more elements.
					virtual bool isDone() const /*noexcept*/ = 0;
					/**
					 * Moves the iterator to the next overriden text segment.
					 * @throw NoSuchElementException The iterator is end
					 */
					virtual void next() = 0;
					/// Moves the iterator to the beginning.
					virtual void reset() /*noexcept*/ = 0;
				};
			public:
				/// Destructor.
				virtual ~TextPaintOverride() /*noexcept*/ {}
				/**
				 * Returns the iterator which overrides the paints of the specified character
				 * range in the line.
				 * @param range The character range in the line
				 * @return The iterator which generates the overridden paints
				 */
				virtual std::unique_ptr<Iterator>
					queryTextPaintOverride(const Range<Index>& range) const = 0;
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
				virtual presentation::DominantBaseline baseline() const /*noexcept*/ = 0;
				/// Returns the baseline offset od the text, relative to the baseline of the text.
				virtual Scalar baselineOffset(presentation::AlignmentBaseline baseline) const /*noexcept*/ = 0;
				/// Returns the descent of the text in pixels.
				virtual Scalar descent() const /*noexcept*/ = 0;
				/// Returns the height of the text in pixels.
				Scalar height() const /*noexcept*/ {return ascent() + descent()/* + leading()*/;}
//				/// Returns the leading of the text in pixels.
//				virtual Scalar leading() const /*noexcept*/ = 0;
			};

			struct ComputedBorderSide {
				Color color;
				presentation::sp::IntrinsicType<
					decltype(presentation::Border().sides.start().style)
				>::Type style;
				Scalar width;

				/// Returns the computed width in pixels.
				Scalar computedWidth() const /*noexcept*/ {
					return (style != presentation::Border::NONE) ? width : 0;
				}
				/// Returns @c true if this part is invisible (but may be consumes place).
				bool hasVisibleStyle() const /*noexcept*/ {
					return style != presentation::Border::NONE && style != presentation::Border::HIDDEN;
				}
			};

			struct ComputedTextDecoration {
				presentation::sp::IntrinsicType<
					decltype(presentation::TextDecoration().lines)
				>::Type lines;
				presentation::sp::IntrinsicType<
					decltype(presentation::TextDecoration().style)
				>::Type style;
				presentation::sp::IntrinsicType<
					decltype(presentation::TextDecoration().skip)
				>::Type skip;
				presentation::sp::IntrinsicType<
					decltype(presentation::TextDecoration().underlinePosition)
				>::Type underlinePosition;
			};

			struct ComputedTextEmphasis {
				presentation::sp::IntrinsicType<
					decltype(presentation::TextEmphasis().style)
				>::Type style;
				Color color;
				presentation::sp::IntrinsicType<
					decltype(presentation::TextEmphasis().position)
				>::Type position;
			};

			/// Computed values of @c presentation#TextRunStyle.
			struct ComputedTextRunStyle {
				/// Computed value of @c TextRunStyle#color property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextRunStyle().color)
				> color;
				/// Computed value of @c TextRunStyle#background property.
				std::shared_ptr<const Paint> background;
				/// Computed value of @c TextRunStyle#border property.
				PhysicalFourSides<ComputedBorderSide> borders;

				/// Computed value of @c TextRunStyle#textHeight property.
				Scalar textHeight;
				/// Computed value of @c TextRunStyle#lineHeight property.
				Scalar lineHeight;
				/// Computed value of @c TextRunStyle#dominantBaseline property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextRunStyle().dominantBaseline)
				> dominantBaseline;
				/// Computed value of @c TextRunStyle#alignmentBaseline property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextRunStyle().alignmentBaseline)
				> alignmentBaseline;
				/// Computed value of @c TextRunStyle#alignmentAdjustment property.
				Scalar alignmentAdjustment;
				/// Computed value of @c TextRunStyle#baselineShift property.
				Scalar baselineShift;
				
				/// Computed value of @c TextRunStyle#textTransform property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextRunStyle().textTransform)
				> textTransform;
				/// Computed value of @c TextRunStyle#hyphens property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextRunStyle().hyphens)
				> hyphens;
				/// Computed value of @c TextRunStyle#wordSpacing property.
				presentation::SpacingLimit<Scalar> wordSpacing;
				/// Computed value of @c TextRunStyle#letterSpacing property.
				presentation::SpacingLimit<Scalar> letterSpacing;
				/// Computed value of @c TextRunStyle#textDecoration property.
				ComputedTextDecoration textDecoration;
				/// Computed value of @c TextRunStyle#textEmphasis property.
				ComputedTextEmphasis textEmphasis;
//				TextShadow textShadow;

//				RubyProperties rubyProperties;

//				Effects effects;
				/// Computed value of @c TextRunStyle#shapingEnabled.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextRunStyle().shapingEnabled)
				> shapingEnabled;
			};

			struct ComputedNumberSubstitution {
				presentation::sp::IntrinsicType<
					decltype(presentation::NumberSubstitution().method)
				>::Type method;
				presentation::sp::IntrinsicType<
					decltype(presentation::NumberSubstitution().localeName)
				>::Type localeName;
				presentation::sp::IntrinsicType<
					decltype(presentation::NumberSubstitution().ignoreUserOverride)
				>::Type ignoreUserOverride;
			};

			/// Computed values of @c presentation#TextLineStyle.
			struct ComputedTextLineStyle {
				/// Computed value of writing modes properties of @c TextToplevelStyle.
				presentation::WritingMode writingMode;

				/// Computed value of @c TextLineStyle#lineBoxContain property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextLineStyle().lineBoxContain)
				>::Type lineBoxContain;
//				/// Computed value of @c TextLineStyle#textSpaceCollapse property.
//				presentation::TextSpaceCollapse spaceCollapse;
				/// Computed value of @c TextLineStyle#tabSize property.
				std::shared_ptr<TabExpander> tabExpander;
				/// Computed value of @c TextLineStyle#lineBreak property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextLineStyle().lineBreak)
				>::Type lineBreak;
				/// Computed value of @c TextLineStyle#wordBreak property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextLineStyle().wordBreak)
				>::Type wordBreak;
//				/// Computed value of @c TextLineStyle#textWrap property.
//				presentation::sp::IntrinsicType<
//					decltype(presentation::TextLineStyle().textWrap)
//				>::Type textWrap;
				/// Computed value of @c TextLineStyle#overflowWrap property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextLineStyle().overflowWrap)
				>::Type overflowWrap;
				/// Computed value of @c TextLineStyle#textAlignment property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextLineStyle().textAlignment)
				>::Type alignment;
				/// Computed value of @c TextLineStyle#textAlignmentLast property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextLineStyle().textAlignmentLast)
				>::Type alignmentLast;
				/// Computed value of @c TextLineStyle#textJustification property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextLineStyle().textJustification)
				>::Type justification;
				/// Computed value of @c TextLineStyle#textIndent property.
				presentation::TextIndent<Scalar> indent;
				/// Computed value of @c TextLineStyle#hangingPunctuation property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextLineStyle().hangingPunctuation)
				>::Type hangingPunctuation;
				/// Computed value of @c TextLineStyle#dominantBaseline property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextLineStyle().dominantBaseline)
				>::Type dominantBaseline;
				/// Computed value of @c TextLineStyle#lineHeight property.
				Scalar lineHeight;
				/// Computed value of @c TextLineStyle#numberSubstitution property.
				ComputedNumberSubstitution numberSubstitution;

				/**
				 * Set @c true to shape zero width control characters as representative glyphs.
				 * The default value is @c false.
				 */
				bool displayShapingControls;
				/**
				 * Set @c true to make the deprecated format characters (NADS, NODS, ASS and ISS)
				 * not effective. The default value is @c false.
				 */
				bool disableDeprecatedFormatCharacters;
				/**
				 * Set @c true to inhibit from generating mirrored glyphs. The default value is
				 * @c false.
				 */
				bool inhibitSymmetricSwapping;
			};

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
				class TextRun;
				class InlineArea;

			public:
				// constructors
				TextLayout(const String& text, const ComputedTextLineStyle& lineStyle,
					std::unique_ptr<presentation::StyledTextRunIterator> textRunStyles);
				~TextLayout() /*throw()*/;
				// general attributes
				presentation::TextAnchor anchor() const /*throw()*/;
				uint8_t bidiEmbeddingLevel(Index offsetInLine) const;
				bool isBidirectional() const /*throw()*/;
				bool isEmpty() const /*throw()*/;
				const presentation::TextLineStyle& style() const /*throw()*/;
				const presentation::WritingMode& writingMode() const /*throw()*/;
				// visual line accesses
				Index numberOfLines() const /*throw()*/;
				Index lineAt(Index offsetInLine) const;
				Index lineLength(Index line) const;
				Index lineOffset(Index line) const;
				const Index* lineOffsets() const /*throw()*/;
				// bounds, extents and measures
				NativeRegion blackBoxBounds(const Range<Index>& range) const;
				NativeRectangle bounds() const /*throw()*/;
				NativeRectangle bounds(const Range<Index>& range) const;
				Range<Scalar> extent() /*throw()*/ const;
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
				presentation::StyledTextRun styledTextRun(Index offsetInLine) const;
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
				const String& text_;
				const presentation::WritingMode writingMode_;
				const presentation::TextAnchor anchor_;
				const presentation::DominantBaseline dominantBaseline_;
				std::unique_ptr<TextRun*[]> runs_;
				std::size_t numberOfRuns_;
				class LineArea;
				std::vector<const InlineArea*> inlineAreas_;
				std::unique_ptr<const Index[]> lineOffsets_;	// size is numberOfLines_
				std::unique_ptr<const Index[]> lineFirstRuns_;	// size is numberOfLines_
				static const Index SINGLE_LINE_OFFSETS;
				Index numberOfLines_;
				std::unique_ptr<LineMetrics*[]> lineMetrics_;
				std::unique_ptr<Scalar[]> measures_;
				Scalar maximumMeasure_;
				Scalar wrappingMeasure_;	// -1 if should not wrap
				friend class LineLayoutVector;
//				friend class StyledSegmentIterator;
			};


			/**
			 * Returns extent (block-progression-dimension) of the line.
			 * @return A range of block-progression-dimension relative to the alignment-point
			 */
			inline Range<Scalar> TextLayout::extent() const /*throw()*/ {
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

			/// Returns @c true if the layout is empty.
			inline bool TextLayout::isEmpty() const /*throw()*/ {return runs_.get() == nullptr;}

			/**
			 * Returns the wrapped line containing the specified offset in the logical line.
			 * @param offsetInLine The offset in the line
			 * @return The wrapped line
			 * @throw kernel#BadPositionException @a offsetInLine is greater than the length of the line
			 */
			inline Index TextLayout::lineAt(Index offsetInLine) const {
				if(offsetInLine > text_.length())
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
					lineOffset(line + 1) : text_.length()) - lineOffset(line);
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
			inline const Index* TextLayout::lineOffsets() const /*throw()*/ {return lineOffsets_.get();}

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
			inline Index TextLayout::numberOfLines() const /*throw()*/ {return numberOfLines_;}

		}
	}

	namespace detail {
		void paintBorder(graphics::PaintContext& context, const graphics::NativeRectangle& rectangle,
			const graphics::PhysicalFourSides<graphics::font::ComputedBorderSide>& style,
			const graphics::Color& currentColor, const presentation::WritingMode& writingMode);
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_TEXT_LAYOUT_HPP
