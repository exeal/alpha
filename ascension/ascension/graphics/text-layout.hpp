/**
 * @file text-layout.hpp
 * @date 2003-2006 (was LineLayout.h)
 * @date 2006-2010
 * @date 2010-11-20 renamed from ascension/layout.hpp
 */

#ifndef ASCENSION_TEXT_LAYOUT_HPP
#define ASCENSION_TEXT_LAYOUT_HPP

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION
#include <ascension/corelib/memory.hpp>	// AutoBuffer
#include <ascension/kernel/document.hpp>
#include <ascension/presentation.hpp>
#include <ascension/graphics/color.hpp>
#include <limits>	// std.numeric_limits

namespace ascension {

	namespace presentation {class Presentation;}
	namespace viewers {class Caret;}

	namespace graphics {

		class Context;
		class PaintContext;

		namespace font {

			class ISpecialCharacterRenderer;

			/**
			 * Configuration about line wrapping.
			 * @see Presentation#Configurations#lineWrap
			 */
			struct LineWrapConfiguration {
				/**
				 * Modes for text wrapping. These values are based on "text-wrap" property in
				 * <a href="http://www.w3.org/TR/2007/WD-css3-text-20070306/">CSS Text Level 3</a>
				 * working draft of W3C Cascading Style Sheet.
				 */
				enum Mode {
					NONE,			///< Lines may not break.
					NORMAL,			///< Lines may break at allowed points as determined by UAX #14.
					UNRESTRICTED,	///< Lines may break between any two grapheme clusters.
					SUPPRESS		///< Line breaking is suppressed within the run.
				} mode;	///< The mode. Default value is @c NONE.
#if 0
				/**
				 * Specifies what set of line breaking restrictions are in effect within the run.
				 * These values are based on "word-break" property in
				 * <a href="http://www.w3.org/TR/2007/WD-css3-text-20070306/">CSS Text Level 3</a>
				 * working draft of W3C Cascading Style Sheet.
				 */
				enum WordBreak {
					NORMAL,			///< Same as 'normal'.
					KEEP_ALL,		///< Same as 'keep-all'.
					LOOSE,			///< Same as 'loose'.
					BREAK_STRICT,	///< Same as 'break-strict'.
					BREAK_ALL		///< Same as 'break-all'
				} wordBreak;
#endif
				/**
				 * The maximum line width. This value must be grater than or equal to zero. If set
				 * to zero, the lines will be wrapped at the window edge.
				 */
				Scalar width;
				/// Default constructor.
				LineWrapConfiguration() /*throw()*/ : mode(NONE), width(0) {};
				/// Returns @c true if the all members are valid.
				bool verify() const /*throw()*/ {return width >= 0;}
				/// Returns @c true if @c mode is not @c NONE.
				bool wraps() const /*throw()*/ {return mode != NONE;}
				/// Returns @c true if @c algorithm is not @c NO_WRAP and @c width is zero.
				bool wrapsAtWindowEdge() const /*throw()*/ {return wraps() && width == 0;}
			};

			/**
			 * General settings for layout.
			 * @see ILayoutInformationProvider#layoutSettings, TextViewer#Configuration
			 */
			struct LayoutSettings {
//				/// Color of normal text. Standard setting is {@c COLOR_WINDOWTEXT, @c COLOR_WINDOW}.
//				presentation::Colors color;
//				/// Color of invisible controls. Standard setting is not provided.
//				presentation::Colors invisibleControlColor;
				/// Character count of a tab expansion. Default value is 8.
				Scalar tabWidth;
				/// Line spacing in pixel. Default value is 1.
				Scalar lineSpacing;
				/// Line wrap configuration.
				LineWrapConfiguration lineWrap;
				/**
				 * If set to @c true, zero width control characters are shaped as representative
				 * glyphs. Default is @c false.
				 */
				bool displaysShapingControls;
				/**
				 * Set @c true to inhibit from generating mirrored glyphs. Default value is
				 * @c false.
				 */
				bool inhibitsSymmetricSwapping;
				/**
				 * Set @c true to make the deprecated format characters (NADS, NODS, ASS, and ISS)
				 * not effective. Default value is @c false.
				 */
				bool disablesDeprecatedFormatCharacters;
				/// Constructor initializes the all members to their default values.
				LayoutSettings() /*throw()*/ : tabWidth(8), lineSpacing(0),
					displaysShapingControls(false), inhibitsSymmetricSwapping(false),
					disablesDeprecatedFormatCharacters(false) {}
				/// Returns @c true if the all mwmbers are valid.
				bool verify() const /*throw()*/ {
					return lineWrap.verify() && tabWidth > 0 && lineSpacing >= 0;}
			};

			/**
			 * Defines the stuffs for layout. Clients of Ascension can implement this interface or
			 * use a higher level @c TextRenderer class.
			 * @see LineLayout, LineLayoutBuffer
			 */
			class ILayoutInformationProvider {
			public:
				/// Destructor.
				virtual ~ILayoutInformationProvider() /*throw()*/ {}
				/// Returns the font collection.
				virtual const FontCollection& fontCollection() const /*throw()*/ = 0;
				/// Returns the layout settings.
				virtual const LayoutSettings& layoutSettings() const /*throw()*/ = 0;
				/**
				 * Returns the default reading direction of UI. The value this method returns is
				 * treated as "last resort" for resolvement reading direction of text layout. If
				 * returns @c INHERIT_READING_DIRECTION, the caller should use the value defined by
				 * @c ASCENSION_DEFAULT_READING_DIRECTION symbol.
				 * @see presentation#LineStyle#readingDirection
				 * @see presentation#Presentation#defaultLineStyle
				 */
				virtual presentation::ReadingDirection defaultUIReadingDirection() const /*throw()*/ = 0;
				/// Returns the presentation object.
				virtual const presentation::Presentation& presentation() const /*throw()*/ = 0;
				/// Returns the special character renderer.
				virtual ISpecialCharacterRenderer* specialCharacterRenderer() const /*throw()*/ = 0;
				/// Returns the text metrics.
				virtual const Font::Metrics& textMetrics() const /*throw()*/ = 0;
				/// Returns the width of the rendering area in pixels.
				virtual Scalar width() const /*throw()*/ = 0;
			};

			/**
			 * The @c InlineObject represents an inline object in @c TextLayout.
			 */
			class InlineObject {
			public:
				virtual ~InlineObject() /*throw()*/ {}
				/// Returns the advance (width) of this inline object in pixels.
				virtual Scalar advance() const /*throw()*/ = 0;
				/// Returns the ascent of this inline object in pixels.
				virtual Scalar ascent() const /*throw()*/ = 0;
				/// Returns the descent of this inline object in pixels.
				virtual Scalar descent() const /*throw()*/ = 0;
				/**
				 * Renders this inline object at the specified location.
				 * @param context The graphic context
				 * @param origin The location where this inline object is rendered
				 */
				virtual void draw(const PaintContext& context, const Point<>& origin) /*throw()*/ = 0;
				/// Returns the size of this inline object in pixels.
				Dimension<> size() const /*throw()*/ {return Dimension<>(advance(), ascent() + descent());}
			};

			/**
			 * @see TextLayout#TextLayout
			 */
			class TabExpander {
			public:
				/// Destructor.
				virtual ~TabExpander() {
				}
				/**
				 * Returns the next tab stop position given a reference position.
				 * @param x The position in pixels
				 * @param tabOffset The position within the underlying text that the tab occured
				 * @return The next tab stop. Should be greater than @a x
				 */
				virtual Scalar nextTabStop(Scalar x, length_t tabOffset) const = 0;
			};

			class FixedWidthTabExpander : public TabExpander {
			public:
				explicit FixedWidthTabExpander(Scalar width) /*throw()*/;
				Scalar nextTabStop(Scalar x, length_t tabOffset) const;
			private:
				const Scalar width_;
			};

			class TextColorOverrideIterator {
			public:
				struct TextSegment {
					Range<length_t> range;	///< The character range in the line.
					Color foreground;		///< The foreground color.
					Color background;		///< The background color.
				};
			public:
				/// Destructor.
				virtual ~TextColorOverrideIterator() /*throw()*/ {}
				/**
				 * Returns the current overriden text segment.
				 * @param[out] segment
				 * @throw NoSuchElementException The iterator is end
				 */
				virtual void current(TextSegment& segment) const = 0;
				/// Returns @c false if the iterator has no more elements.
				virtual bool hasNext() const /*throw()*/ = 0;
				/**
				 * Moves the iterator to the next overriden text segment.
				 * @throw NoSuchElementException The iterator is end
				 */
				virtual void next() = 0;
			};

			/**
			 * @see Font#Metrics
			 */
			class LineMetrics {
			public:
				/// Destructor.
				virtual ~LineMetrics() /*throw()*/ {}
				/// Returns the ascent of the text in pixels.
				virtual Scalar ascent() const /*throw()*/ = 0;
				/// Returns the ascent of the text in pixels.
				virtual presentation::DominantBaseline baseline() const /*throw()*/ = 0;
				/// Returns the ascent of the text in pixels.
				virtual Scalar baselineOffset(presentation::AlignmentBaseline baseline) const /*throw()*/ = 0;
				/// Returns the descent of the text in pixels.
				virtual Scalar descent() const /*throw()*/ = 0;
				/// Returns the height of the text in pixels.
				Scalar height() const {return ascent() + descent() + leading();}
				/// Returns the leading of the text in pixels.
				virtual Scalar leading() const /*throw()*/ = 0;
			};

			class TextLayout {
				ASCENSION_NONCOPYABLE_TAG(TextLayout);
			public:
				/// Edge of a character.
				enum Edge {
					LEADING,	///< Leading edge of a character.
					TRAILING	///< Trailing edge of a character.
				};
				/// Used for @c LineLayout#draw methods.
				class Selection {
					ASCENSION_UNASSIGNABLE_TAG(Selection);
				public:
					/// Constructor.
					Selection(const viewers::Caret& caret,
						const Color& foreground, const Color& background);
					/// Returns the caret object.
					const viewers::Caret& caret() const /*throw()*/ {return caret_;}
					/// Returns the background color to render.
					const Color& background() const /*throw()*/ {return background_;}
					/// Returns the foreground color to render.
					const Color& foreground() const /*throw()*/ {return foreground_;}
				private:
					const viewers::Caret& caret_;
					const Color foreground_, background_;
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

				// constructors
				TextLayout(const String& text,
					presentation::ReadingDirection readingDirection, presentation::TextAlignment alignment,
					const FontCollection& fontCollection = systemFonts(),
					std::tr1::shared_ptr<const presentation::TextRunStyle>
						defaultTextRunStyle = std::tr1::shared_ptr<const presentation::TextRunStyle>(),
					std::auto_ptr<presentation::StyledTextRunIterator>
						textRunStyles = std::auto_ptr<presentation::StyledTextRunIterator>(),
					const TabExpander* tabExpander = 0, Scalar width = std::numeric_limits<Scalar>::max(),
					const presentation::NumberSubstitution* numberSubstitution = 0,
					bool displayShapingControls = false, bool inhibitSymmetricSwapping = false,
					bool disableDeprecatedFormatCharacters = false);
				~TextLayout() /*throw()*/;
				// general attributes
				presentation::TextAlignment alignment() const /*throw()*/;
				byte bidiEmbeddingLevel(length_t column) const;
				bool isBidirectional() const /*throw()*/;
				bool isEmpty() const /*throw()*/;
				length_t lineNumber() const /*throw()*/;
				presentation::ReadingDirection readingDirection() const /*throw()*/;
				const presentation::TextLineStyle& style() const /*throw()*/;
				// visual line accesses
				length_t numberOfLines() const /*throw()*/;
				length_t lineAt(length_t column) const;
				length_t lineLength(length_t line) const;
				length_t lineOffset(length_t line) const;
				const length_t* lineOffsets() const /*throw()*/;
				// coordinates
				NativePolygon blackBoxBounds(const Range<length_t>& range) const;
				Dimension<> bounds() const /*throw()*/;
				Rect<> bounds(const Range<length_t>& range) const;
				const LineMetrics& lineMetrics(length_t line) const;
				Point<> location(length_t column, Edge edge = LEADING) const;
				std::pair<Point<>, Point<> > locations(length_t column) const;
				Scalar longestLineWidth() const /*throw()*/;
				std::pair<length_t, length_t> offset(const Point<>& p, bool* outside = 0) const /*throw()*/;
				Rect<> lineBounds(length_t line) const;
				Scalar lineIndent(length_t line) const;
				Scalar lineWidth(length_t line) const;
				// styled segments
//				StyledSegmentIterator firstStyledSegment() const /*throw()*/;
//				StyledSegmentIterator lastStyledSegment() const /*throw()*/;
				presentation::StyledTextRun styledTextRun(length_t column) const;
				// operations
				void draw(PaintContext& context,
					const Point<>& origin, const Rect<>& clipRect,
					const Color& defaultForeground, const Color& defaultBackground,
					const Selection* selection) const /*throw()*/;
				void draw(length_t line, PaintContext& context,
					const Point<>& origin, const Rect<>& clipRect,
					const Color& defaultForeground, const Color& defaultBackground,
					const Selection* selection) const;
				String fillToX(Scalar x) const;
#ifdef _DEBUG
				void dumpRuns(std::ostream& out) const;
#endif // _DEBUG

			private:
				void expandTabsWithoutWrapping() /*throw()*/;
				std::size_t findRunForPosition(length_t column) const /*throw()*/;
				void justify() /*throw()*/;
				void locations(length_t column, Point<>* leading, Point<>* trailing) const;
				void reorder() /*throw()*/;
//				void rewrap();
				int nextTabStopBasedLeftEdge(Scalar x, bool right) const /*throw()*/;
				void wrap(const TabExpander& tabExpander) /*throw()*/;
			private:
				const String& text_;
				const presentation::ReadingDirection readingDirection_;
				const presentation::TextAlignment alignment_;
				class TextRun;
				AutoBuffer<TextRun*> runs_;
				std::size_t numberOfRuns_;
				AutoBuffer<presentation::StyledTextRun> styledRanges_;
				std::size_t numberOfStyledRanges_;
				AutoBuffer<const length_t> lineOffsets_;	// size is numberOfLines_
				AutoBuffer<const length_t> lineFirstRuns_;	// size is numberOfLines_
				static const length_t SINGLE_LINE_OFFSETS;
				length_t numberOfLines_;
				AutoBuffer<LineMetrics*> lineMetrics_;
				Scalar longestLineWidth_;
				Scalar wrapWidth_;	// -1 if should not wrap
				friend class LineLayoutBuffer;
//				friend class StyledSegmentIterator;
			};


			/// Returns @c true if the layout is empty.
			inline bool TextLayout::isEmpty() const /*throw()*/ {return runs_.get() == 0;}

			/**
			 * Returns the wrapped line containing the specified column.
			 * @param column The column
			 * @return The wrapped line
			 * @throw kernel#BadPositionException @a column is greater than the length of the line
			 */
			inline length_t TextLayout::lineAt(length_t column) const {
				if(column > text_.length())
					throw kernel::BadPositionException(kernel::Position(INVALID_INDEX, column));
				return (numberOfLines() == 1) ? 0 :
					detail::searchBound(static_cast<length_t>(0), numberOfLines(),
						column, std::bind1st(std::mem_fun(&TextLayout::lineOffset), this));
			}

			/**
			 * Returns the metrics for the specified line.
			 * @param line The line number
			 * @return The line metrics
			 * @throw BadPositionException @a line is greater than the count of lines
			 */
			inline const LineMetrics& TextLayout::lineMetrics(length_t line) const {
				if(line >= numberOfLines())
					throw kernel::BadPositionException(kernel::Position());
				return *lineMetrics_[line];
			}

			/**
			 * Returns the length of the specified visual line.
			 * @param line The visual line
			 * @return The length of the line
			 * @throw BadPositionException @a line is greater than the count of lines
			 */
			inline length_t TextLayout::lineLength(length_t line) const {
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
			inline length_t TextLayout::lineOffset(length_t line) const {
				if(line >= numberOfLines())
					throw kernel::BadPositionException(kernel::Position());
				return lineOffsets()[line];
			}

			/**
			 * Returns the line offsets.
			 * @return The line offsets whose length is @c #numberOfLines(). Each element in the
			 *         array is the offset for the first character in a line
			 */
			inline const length_t* TextLayout::lineOffsets() const /*throw()*/ {return lineOffsets_.get();}

			/**
			 * Returns the location for the specified character offset.
			 * @param column The character offset from the beginning of the line
			 * @param edge The edge of the character to locate
			 * @return The location. X-coordinate is distance from the left edge of the renderer,
			 *         y-coordinate is relative in the visual lines
			 * @throw kernel#BadPositionException @a column is greater than the length of the line
			 */
			inline Point<> TextLayout::location(length_t column, Edge edge /* = LEADING */) const {
				Point<> result;
				locations(column, (edge == LEADING) ? &result : 0, (edge == TRAILING) ? &result : 0);
				return result;
			}

			/**
			 * Returns the locations for the specified character offset.
			 * @param column The character offset from the beginning of the line
			 * @return A pair consists of the locations. The first element means the leading, the
			 *         second element means the trailing position of the character. X-coordinates
			 *         are distances from the left edge of the renderer, y-coordinates are relative
			 *         in the visual lines
			 * @throw kernel#BadPositionException @a column is greater than the length of the line
			 */
			inline std::pair<Point<>, Point<> > TextLayout::locations(length_t column) const {
				std::pair<Point<>, Point<> > result;
				locations(column, &result.first, &result.second);
				return result;
			}

			/// Returns the number of the wrapped lines.
			inline length_t TextLayout::numberOfLines() const /*throw()*/ {return numberOfLines_;}

		}
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_TEXT_LAYOUT_HPP
