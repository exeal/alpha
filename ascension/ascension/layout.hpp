/**
 * @file layout.hpp
 * @date 2003-2006 (was LineLayout.h)
 * @date 2006-2010
 */

#ifndef ASCENSION_LAYOUT_HPP
#define ASCENSION_LAYOUT_HPP
#include "document.hpp"
#include "unicode-property.hpp"
#include "presentation.hpp"
#include "graphics.hpp"
#include <vector>

namespace ascension {

	namespace presentation {class Presentation;}
	namespace viewers {class Caret;}

	namespace graphics {

		// free functions
		bool getDecorationLineMetrics(const win32::Handle<HDC>& dc, int* baselineOffset,
			int* underlineOffset, int* underlineThickness, int* strikethroughOffset, int* strikethroughThickness) /*throw()*/;
		bool supportsComplexScripts() /*throw()*/;
		bool supportsOpenTypeFeatures() /*throw()*/;

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
			 * Specifies what set of line breaking restrictions are in effect within the run. These
			 * values are based on "word-break" property in
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
			 * The maximum line width. This value must be grater than or equal to zero. If set to
			 * zero, the lines will be wrapped at the window edge.
			 */
			int width;
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
			/// Color of normal text. Standard setting is {@c COLOR_WINDOWTEXT, @c COLOR_WINDOW}.
			presentation::Colors color;
			/// Color of invisible controls. Standard setting is not provided.
			presentation::Colors invisibleControlColor;
			/// Character count of a tab expansion. Default value is 8.
			int tabWidth;
			/// Line spacing in pixel. Default value is 1.
			int lineSpacing;
			/// Line wrap configuration.
			LineWrapConfiguration lineWrap;
			/// If set to @c true, zero width control characters are shaped as representative glyphs. Default is @c false.
			bool displaysShapingControls;
			/// Set @c true to inhibit from generating mirrored glyphs. Default value is @c false.
			bool inhibitsSymmetricSwapping;
			/// Set @c true to make the deprecated format characters (NADS, NODS, ASS, and ISS) not effective. Default value is @c false.
			bool disablesDeprecatedFormatCharacters;
			/// Constructor initializes the all members to their default values.
			LayoutSettings() /*throw()*/ : tabWidth(8), lineSpacing(0),
				displaysShapingControls(false), inhibitsSymmetricSwapping(false), disablesDeprecatedFormatCharacters(false) {}
			/// Returns @c true if the all mwmbers are valid.
			bool verify() const /*throw()*/ {return lineWrap.verify() && tabWidth > 0 && lineSpacing >= 0;}
		};

		class TextRenderer;

		/*
		 * Interface for objects which are interested in change of the default font of
		 * @c TextRenderer.
		 * @see TextRenderer#addDefaultFontListener, TextRenderer#removeDefaultFontListener
		 */
		class IDefaultFontListener {
		private:
			/// The font settings was changed.
			virtual void defaultFontChanged() = 0;
			friend class TextRenderer;
		};

		class ISpecialCharacterRenderer {
		public:
			/// Destructor.
			virtual ~ISpecialCharacterRenderer() /*throw()*/ {}
		protected:
			/// Context of the layout.
			struct LayoutContext {
				ASCENSION_UNASSIGNABLE_TAG(LayoutContext);
			public:
				mutable Context& renderingContext;					///< the rendering context.
				presentation::ReadingDirection readingDirection;	///< the orientation of the character.
				/// Constructor.
				explicit LayoutContext(Context& renderingContext) /*throw()*/ : renderingContext(renderingContext) {}
			};
			/// Context of the drawing.
			struct DrawingContext : public LayoutContext {
				Rect<> rect;	///< the bounding box to draw.
				/// Constructor.
				DrawingContext(Context& deviceContext) /*throw()*/ : LayoutContext(renderingContext) {}
			};
		private:
			/**
			 * Draws the specified C0 or C1 control character.
			 * @param context the context
			 * @param c the code point of the character to draw
			 */
			virtual void drawControlCharacter(const DrawingContext& context, CodePoint c) const = 0;
			/**
			 * Draws the specified line break indicator.
			 * @param context the context
			 * @param newline the newline to draw
			 */
			virtual void drawLineTerminator(const DrawingContext& context, kernel::Newline newline) const = 0;
			/**
			 * Draws the width of a line wrapping mark.
			 * @param context the context
			 */
			virtual void drawLineWrappingMark(const DrawingContext& context) const = 0;
			/**
			 * Draws the specified white space character.
			 * @param context the context
			 * @param c the code point of the character to draw
			 */
			virtual void drawWhiteSpaceCharacter(const DrawingContext& context, CodePoint c) const = 0;
			/**
			 * Returns the width of the specified C0 or C1 control character.
			 * @param context the context
			 * @param c the code point of the character to layout
			 * @return the width or 0 if does not render the character
			 */
			virtual int getControlCharacterWidth(const LayoutContext& context, CodePoint c) const = 0;
			/**
			 * Returns the width of the specified line break indicator.
			 * @param context the context
			 * @param newline the newline to layout
			 * @return the width or 0 if does not render the indicator
			 */
			virtual int getLineTerminatorWidth(const LayoutContext& context, kernel::Newline newline) const = 0;
			/**
			 * Returns the width of a line wrapping mark.
			 * @param context the context
			 * @return the width or 0 if does not render the mark
			 */
			virtual int getLineWrappingMarkWidth(const LayoutContext& context) const = 0;
			/**
			 * Installs the drawer.
			 * @param textRenderer the text renderer
			 */
			virtual void install(TextRenderer& textRenderer) = 0;
			/// Uninstalls the drawer.
			virtual void uninstall() = 0;
			friend class LineLayout;
			friend class TextRenderer;
		};

		class DefaultSpecialCharacterRenderer : public ISpecialCharacterRenderer, public IDefaultFontListener {
		public:
			// constructors
			DefaultSpecialCharacterRenderer() /*throw()*/;
			// attributes
			COLORREF controlCharacterColor() const /*throw()*/;
			COLORREF lineTerminatorColor() const /*throw()*/;
			COLORREF lineWrappingMarkColor() const /*throw()*/;
			void setControlCharacterColor(COLORREF color) /*throw()*/;
			void setLineTerminatorColor(COLORREF color) /*throw()*/;
			void setLineWrappingMarkColor(COLORREF color) /*throw()*/;
			void setWhiteSpaceColor(COLORREF color) /*throw()*/;
			void showLineTerminators(bool show) /*throw()*/;
			void showWhiteSpaces(bool show) /*throw()*/;
			bool showsLineTerminators() const /*throw()*/;
			bool showsWhiteSpaces() const /*throw()*/;
			COLORREF whiteSpaceColor() const /*throw()*/;
		private:
			// ISpecialCharacterRenderer
			void drawControlCharacter(const DrawingContext& context, CodePoint c) const;
			void drawLineTerminator(const DrawingContext& context, kernel::Newline newline) const;
			void drawLineWrappingMark(const DrawingContext& context) const;
			void drawWhiteSpaceCharacter(const DrawingContext& context, CodePoint c) const;
			int getControlCharacterWidth(const LayoutContext& context, CodePoint c) const;
			int getLineTerminatorWidth(const LayoutContext& context, kernel::Newline newline) const;
			int getLineWrappingMarkWidth(const LayoutContext& context) const;
			void install(TextRenderer& textRenderer);
			void uninstall();
			// IDefaultFontListener
			void defaultFontChanged();
		private:
			TextRenderer* renderer_;
			COLORREF controlColor_, eolColor_, wrapMarkColor_, whiteSpaceColor_;
			bool showsEOLs_, showsWhiteSpaces_;
			std::tr1::shared_ptr<const Font> font_;	// provides substitution glyphs
			enum {LTR_HORIZONTAL_TAB, RTL_HORIZONTAL_TAB, LINE_TERMINATOR, LTR_WRAPPING_MARK, RTL_WRAPPING_MARK, WHITE_SPACE};
			WORD glyphs_[6];
			int glyphWidths_[6];
		};

		/**
		 * Defines the stuffs for layout. Clients of Ascension can implement this interface or use
		 * a higher level @c TextRenderer class.
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
			virtual int width() const /*throw()*/ = 0;
		};

		class LineLayout {
			ASCENSION_NONCOPYABLE_TAG(LineLayout);
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
					const graphics::Color& foreground, const graphics::Color& background);
				/// Returns the caret object.
				const viewers::Caret& caret() const /*throw()*/ {return caret_;}
				/// Returns the background color to render.
				const graphics::Color& background() const /*throw()*/ {return background_;}
				/// Returns the foreground color to render.
				const graphics::Color& foreground() const /*throw()*/ {return foreground_;}
			private:
				const viewers::Caret& caret_;
				const graphics::Color foreground_, background_;
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
			LineLayout(Context& context, const ILayoutInformationProvider& layoutInformation, length_t line);
			~LineLayout() /*throw()*/;
			// general attributes
			presentation::TextAlignment alignment() const /*throw()*/;
			byte bidiEmbeddingLevel(length_t column) const;
			bool isBidirectional() const /*throw()*/;
			bool isDisposed() const /*throw()*/;
			length_t lineNumber() const /*throw()*/;
			presentation::ReadingDirection readingDirection() const /*throw()*/;
			const presentation::LineStyle& style() const /*throw()*/;
			// subline accesses
			length_t numberOfSublines() const /*throw()*/;
			length_t subline(length_t column) const;
			length_t sublineLength(length_t subline) const;
			length_t sublineOffset(length_t subline) const;
			const length_t* sublineOffsets() const /*throw()*/;
			// coordinates
			win32::Handle<HRGN> blackBoxBounds(length_t first, length_t last) const;
			Dimension<> bounds() const /*throw()*/;
			Rect<> bounds(length_t first, length_t last) const;
			Point<> location(length_t column, Edge edge = LEADING) const;
			std::pair<Point<>, Point<> > locations(length_t column) const;
			int longestSublineWidth() const /*throw()*/;
			std::pair<length_t, length_t> offset(int x, int y, bool* outside = 0) const /*throw()*/;
			std::pair<length_t, length_t> offset(const Point<int>& pt, bool* outside = 0) const /*throw()*/;
			Rect<> sublineBounds(length_t subline) const;
			int sublineIndent(length_t subline) const;
			int sublineWidth(length_t subline) const;
			// styled segments
//			StyledSegmentIterator firstStyledSegment() const /*throw()*/;
//			StyledSegmentIterator lastStyledSegment() const /*throw()*/;
			presentation::StyledRun styledTextRun(length_t column) const;
			// operations
			void draw(Context& context, int x, int y,
				const Rect<>& paintRect, const Rect<>& clipRect, const Selection* selection) const /*throw()*/;
			void draw(length_t subline, Context& context, int x, int y,
				const Rect<>& paintRect, const Rect<>& clipRect, const Selection* selection) const;
			String fillToX(int x) const;
#ifdef _DEBUG
			void dumpRuns(std::ostream& out) const;
#endif // _DEBUG

		private:
			void dispose() /*throw()*/;
			void expandTabsWithoutWrapping() /*throw()*/;
			std::size_t findRunForPosition(length_t column) const /*throw()*/;
			void justify() /*throw()*/;
			int linePitch() const /*throw()*/;
			void locations(length_t column, Point<>* leading, Point<>* trailing) const;
			int nextTabStop(int x, Direction direction) const /*throw()*/;
			const String& text() const /*throw()*/;
			void reorder() /*throw()*/;
//			void rewrap();
			int nextTabStopBasedLeftEdge(int x, bool right) const /*throw()*/;
			void wrap(Context& context) /*throw()*/;
		private:
			const ILayoutInformationProvider& lip_;
			length_t lineNumber_;
			std::tr1::shared_ptr<const presentation::LineStyle> style_;
			class TextRun;
			TextRun** runs_;
			std::size_t numberOfRuns_;
			AutoBuffer<presentation::StyledRun> styledRanges_;
			std::size_t numberOfStyledRanges_;
			length_t* sublineOffsets_;		// size is numberOfSublines_
			length_t* sublineFirstRuns_;	// size is numberOfSublines_
			length_t numberOfSublines_;
			int longestSublineWidth_;
			int wrapWidth_;	// -1 if should not wrap
			friend class LineLayoutBuffer;
//			friend class StyledSegmentIterator;
		};

		/**
		 * Interface for objects which are interested in getting informed about change of visual
		 * lines of @c TextRenderer.
		 * @see LineLayoutBuffer#addVisualLinesListener, LineLayoutBuffer#removeVisualLinesListener
		 */
		class IVisualLinesListener {
		private:
			/**
			 * Several visual lines were deleted.
			 * @param first the first of created lines
			 * @param last the last of created lines (exclusive)
			 * @param sublines the total number of sublines of created lines
			 * @param longestLineChanged set @c true if the longest line is changed
			 */
			virtual void visualLinesDeleted(length_t first, length_t last,
				length_t sublines, bool longestLineChanged) /*throw()*/ = 0;
			/**
			 * Several visual lines were inserted.
			 * @param first the first of inserted lines
			 * @param last the last of inserted lines (exclusive)
			 */
			virtual void visualLinesInserted(length_t first, length_t last) /*throw()*/ = 0;
			/**
			 * A visual lines were modified.
			 * @param first the first of modified lines
			 * @param last the last of modified lines (exclusive)
			 * @param sublinesDifference the difference of the number of sublines between before and after the modification
			 * @param documentChanged set @c true if the layouts were modified for the document change
			 * @param longestLineChanged set @c true if the longest line is changed
			 */
			virtual void visualLinesModified(length_t first, length_t last,
				signed_length_t sublinesDifference, bool documentChanged, bool longestLineChanged) /*throw()*/ = 0;
			friend class LineLayoutBuffer;
		};

		/**
		 * Manages a buffer of layout (@c LineLayout) and holds the longest line and the number of
		 * the visual lines.
		 * @see LineLayout, TextRenderer
		 */
		class LineLayoutBuffer : public kernel::IDocumentListener/*, public presentation::IPresentationStylistListener*/ {
			ASCENSION_NONCOPYABLE_TAG(LineLayoutBuffer);
		public:
			// constructors
			LineLayoutBuffer(kernel::Document& document, length_t bufferSize, bool autoRepair);
			virtual ~LineLayoutBuffer() /*throw()*/;
			// attributes
			const kernel::Document& document() const /*throw()*/;
			const LineLayout& lineLayout(length_t line) const;
			const LineLayout* lineLayoutIfCached(length_t line) const /*throw()*/;
			int longestLineWidth() const /*throw()*/;
			length_t numberOfSublinesOfLine(length_t) const;
			length_t numberOfVisualLines() const /*throw()*/;
			// listeners
			void addVisualLinesListener(IVisualLinesListener& listener);
			void removeVisualLinesListener(IVisualLinesListener& listener);
			// strategy
			void setLayoutInformation(const ILayoutInformationProvider* newProvider, bool delegateOwnership);
			// position translations
			length_t mapLogicalLineToVisualLine(length_t line) const;
			length_t mapLogicalPositionToVisualPosition(const kernel::Position& position, length_t* column) const;
//			length_t mapVisualLineToLogicalLine(length_t line, length_t* subline) const;
//			kernel::Position mapVisualPositionToLogicalPosition(const kernel::Position& position) const;
			void offsetVisualLine(length_t& line, length_t& subline,
				signed_length_t offset, bool* overflowedOrUnderflowed = 0) const /*throw()*/;
			// operations
			void invalidate() /*throw()*/;
			void invalidate(length_t first, length_t last);
		protected:
			void invalidate(length_t line);
			// enumeration
			typedef std::list<LineLayout*>::const_iterator Iterator;
			Iterator firstCachedLine() const /*throw()*/;
			Iterator lastCachedLine() const /*throw()*/;
			// abstract
			virtual std::auto_ptr<Context> renderingContext() const = 0;
		private:
			void clearCaches(length_t first, length_t last, bool repair);
			void createLineLayout(length_t line) /*throw()*/;
			void deleteLineLayout(length_t line, LineLayout* newLayout = 0) /*throw()*/;
			void fireVisualLinesDeleted(length_t first, length_t last, length_t sublines);
			void fireVisualLinesInserted(length_t first, length_t last);
			void fireVisualLinesModified(length_t first, length_t last,
				length_t newSublines, length_t oldSublines, bool documentChanged);
			void presentationStylistChanged();
			void updateLongestLine(length_t line, int width) /*throw()*/;
			// kernel.IDocumentListener
			void documentAboutToBeChanged(const kernel::Document& document);
			void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change);
		private:
			struct CachedLineComparer {
				bool operator()(const LineLayout*& lhs, length_t rhs) const /*throw()*/ {return lhs->lineNumber() < rhs;}
				bool operator()(length_t lhs, const LineLayout*& rhs) const /*throw()*/ {return lhs < rhs->lineNumber();}
			};
			kernel::Document& document_;
			ascension::internal::StrategyPointer<const ILayoutInformationProvider> lip_;
			std::list<LineLayout*> layouts_;
			const std::size_t bufferSize_;
			const bool autoRepair_;
			enum {ABOUT_CHANGE, CHANGING, NONE} documentChangePhase_;
			struct {
				length_t first, last;
			} pendingCacheClearance_;	// ドキュメント変更中に呼び出された clearCaches の引数
			int longestLineWidth_;
			length_t longestLine_, numberOfVisualLines_;
			ascension::internal::Listeners<IVisualLinesListener> listeners_;
		};

		// documentation is layout.cpp
		class TextRenderer : public LineLayoutBuffer, public ILayoutInformationProvider {
		public:
			// constructors
			TextRenderer(presentation::Presentation& presentation,
				const FontCollection& fontCollection, bool enableDoubleBuffering);
			TextRenderer(const TextRenderer& other);
			virtual ~TextRenderer() /*throw()*/;
			// text metrics
			std::tr1::shared_ptr<const Font> primaryFont() const /*throw()*/;
			int lineIndent(length_t line, length_t subline = 0) const;
			bool updateTextMetrics();
			// listener
			void addDefaultFontListener(IDefaultFontListener& listener);
			void removeDefaultFontListener(IDefaultFontListener& listener);
			// strategy
			void setSpecialCharacterRenderer(ISpecialCharacterRenderer* newRenderer, bool delegateOwnership);
			// operation
			void renderLine(length_t line, Context& context, int x, int y,
				const Rect<>& paintRect, const Rect<>& clipRect,
				const LineLayout::Selection* selection) const /*throw()*/;
			// ILayoutInformationProvider
			const FontCollection& fontCollection() const /*throw()*/;
			const presentation::Presentation& presentation() const /*throw()*/;
			ISpecialCharacterRenderer* specialCharacterRenderer() const /*throw()*/;
			const Font::Metrics& textMetrics() const /*throw()*/;
		private:
			void fireDefaultFontChanged();
		private:
			presentation::Presentation& presentation_;
			const FontCollection& fontCollection_;
			const bool enablesDoubleBuffering_;
			mutable win32::Handle<HDC> memoryDC_;
			mutable win32::Handle<HBITMAP> memoryBitmap_;
			std::tr1::shared_ptr<const Font> primaryFont_;
			ascension::internal::StrategyPointer<ISpecialCharacterRenderer> specialCharacterRenderer_;
			ascension::internal::Listeners<IDefaultFontListener> listeners_;
		};


// inlines //////////////////////////////////////////////////////////////////

/// Returns @c true if the layout has been disposed.
inline bool LineLayout::isDisposed() const /*throw()*/ {return runs_ == 0;}

/// Returns the line number.
inline length_t LineLayout::lineNumber() const /*throw()*/ {return lineNumber_;}

/**
 * Returns the location for the specified character offset.
 * @param column the character offset from the beginning of the line
 * @param edge the edge of the character to locate
 * @return the location. x-coordinate is distance from the left edge of the renderer, y-coordinate
 *         is relative in the visual lines
 * @throw kernel#BadPositionException @a column is greater than the length of the line
 */
inline Point<> LineLayout::location(length_t column, Edge edge /* = LEADING */) const {
	Point<> result;
	locations(column, (edge == LEADING) ? &result : 0, (edge == TRAILING) ? &result : 0);
	return result;
}

/**
 * Returns the locations for the specified character offset.
 * @param column the character offset from the beginning of the line
 * @return a pair consists of the locations. the first element means the leading, the second
 *         element means the trailing position of the character. x-coordinates are distances from
 *         the left edge of the renderer, y-coordinates are relative in the visual lines
 * @throw kernel#BadPositionException @a column is greater than the length of the line
 */
inline std::pair<Point<>, Point<> > LineLayout::locations(length_t column) const {
	std::pair<Point<>, Point<> > result;
	locations(column, &result.first, &result.second);
	return result;
}

/// Returns the number of the wrapped lines.
inline length_t LineLayout::numberOfSublines() const /*throw()*/ {return numberOfSublines_;}

/**
 * Returns the character column (offset) for the specified point.
 * @param pt the point. pt.x is distance from the left edge of the renderer (not of the line)
 * @param[out] outside @c true if the specified point is outside of the layout. optional
 * @return the character offset
 * @see #location
 */
inline std::pair<length_t, length_t> LineLayout::offset(
	const Point<>& pt, bool* outside /* = 0 */) const /*throw()*/ {return offset(pt.x, pt.y);}

/// Returns the text line style.
inline const presentation::LineStyle& LineLayout::style() const /*throw()*/ {return *style_;}

/**
 * Returns the wrapped line containing the specified column.
 * @param column the column
 * @return the wrapped line
 * @throw kernel#BadPositionException @a column is greater than the length of the line
 */
inline length_t LineLayout::subline(length_t column) const {
	if(column > text().length())
		throw kernel::BadPositionException(kernel::Position(lineNumber_, column));
	return (numberOfSublines_ == 1) ? 0 :
		ascension::internal::searchBound(static_cast<length_t>(0),
			numberOfSublines_, column, std::bind1st(std::mem_fun(&LineLayout::sublineOffset), this));
}

/**
 * Returns the length of the specified visual subline.
 * @param subline the visual subline
 * @return the length of the subline
 * @throw BadPositionException @a subline is greater than the count of visual lines
 */
inline length_t LineLayout::sublineLength(length_t subline) const {
	return (subline < numberOfSublines_ - 1 ? sublineOffset(subline + 1) : text().length()) - sublineOffset(subline);}

/**
 * Returns the offset of the start of the specified visual subline from the start of the logical line.
 * @param subline the visual subline
 * @return the offset
 * @throw BadPositionException @a subline is greater than the count of visual lines
 */
inline length_t LineLayout::sublineOffset(length_t subline) const {
	if(subline >= numberOfSublines_)
		throw kernel::BadPositionException(kernel::Position());
	return (sublineOffsets_ != 0) ? sublineOffsets_[subline] : 0;
}

/**
 * Returns the line offsets.
 * Each value in the array is the offset for the first character in a line.
 * @return the line offsets whose length is #numberOfSublines(), or @c null if the line is empty
 */
inline const length_t* LineLayout::sublineOffsets() const /*throw()*/ {return sublineOffsets_;}
#if 0
/// Asignment operator.
inline LineLayout::StyledSegmentIterator&
	LineLayout::StyledSegmentIterator::operator=(const StyledSegmentIterator& rhs) /*throw()*/ {p_ = rhs.p_; return *this;}

/// Returns @c true if the two iterators address the same segment.
inline bool LineLayout::StyledSegmentIterator::equals(const StyledSegmentIterator& rhs) const /*throw()*/ {return p_ == rhs.p_;}

/// Moves to the next.
inline LineLayout::StyledSegmentIterator& LineLayout::StyledSegmentIterator::next() /*throw()*/ {++p_; return *this;}

/// Moves to the previous.
inline LineLayout::StyledSegmentIterator& LineLayout::StyledSegmentIterator::previous() /*throw()*/ {--p_; return *this;}
#endif
/// Returns the document.
inline const kernel::Document& LineLayoutBuffer::document() const /*throw()*/ {return document_;}

/// Returns the first cached line layout.
inline LineLayoutBuffer::Iterator LineLayoutBuffer::firstCachedLine() const /*throw()*/ {return layouts_.begin();}

/// Returns the last cached line layout.
inline LineLayoutBuffer::Iterator LineLayoutBuffer::lastCachedLine() const /*throw()*/ {return layouts_.end();}

/**
 * Returns the layout of the specified line.
 * @param line the line
 * @return the layout or @c null if the layout is not cached
 */
inline const LineLayout* LineLayoutBuffer::lineLayoutIfCached(length_t line) const /*throw()*/ {
	if(pendingCacheClearance_.first != INVALID_INDEX && line >= pendingCacheClearance_.first && line < pendingCacheClearance_.last)
		return 0;
	for(std::list<LineLayout*>::const_iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
		if((*i)->lineNumber_ == line)
			return *i;
	}
	return 0;
}

/// Returns the width of the longest line.
inline int LineLayoutBuffer::longestLineWidth() const /*throw()*/ {return longestLineWidth_;}

/**
 * Returns the number of sublines of the specified line.
 * If the layout of the line is not calculated, this method returns 1.
 * @param line the line
 * @return the count of the sublines
 * @throw BadPositionException @a line is outside of the document
 * @see #getLineLayout, LineLayout#getNumberOfSublines
 */
inline length_t LineLayoutBuffer::numberOfSublinesOfLine(length_t line) const /*throw()*/ {
	const LineLayout* layout = lineLayoutIfCached(line); return (layout != 0) ? layout->numberOfSublines() : 1;}

/// Returns the number of the visual lines.
inline length_t LineLayoutBuffer::numberOfVisualLines() const /*throw()*/ {return numberOfVisualLines_;}

/**
 * Removes the visual lines listener.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
inline void LineLayoutBuffer::removeVisualLinesListener(IVisualLinesListener& listener) {listeners_.remove(listener);}

/// Returns the color of glyphs for control characters.
inline COLORREF DefaultSpecialCharacterRenderer::controlCharacterColor() const /*throw()*/ {return controlColor_;}

/// Returns the color of line terminators.
inline COLORREF DefaultSpecialCharacterRenderer::lineTerminatorColor() const /*throw()*/ {return eolColor_;}

/// Returns the color of line wrapping marks.
inline COLORREF DefaultSpecialCharacterRenderer::lineWrappingMarkColor() const /*throw()*/ {return wrapMarkColor_;}

/// Returns the color of glyphs for white space characters.
inline COLORREF DefaultSpecialCharacterRenderer::whiteSpaceColor() const /*throw()*/ {return whiteSpaceColor_;}

/**
 * Sets the color of glyphs for control characters.
 * @param color the new color
 */
inline void DefaultSpecialCharacterRenderer::setControlCharacterColor(COLORREF color) /*throw()*/ {controlColor_ = color;}

/**
 * Sets the color of line terminators.
 * @param color the new color
 */
inline void DefaultSpecialCharacterRenderer::setLineTerminatorColor(COLORREF color) /*throw()*/ {eolColor_ = color;}

/**
 * Sets the color of line wrapping marks.
 * @param color the new color
 */
inline void DefaultSpecialCharacterRenderer::setLineWrappingMarkColor(COLORREF color) /*throw()*/ {wrapMarkColor_ = color;}

/**
 * Sets the color of glyphs for white space characters.
 * @param color the new color
 */
inline void DefaultSpecialCharacterRenderer::setWhiteSpaceColor(COLORREF color) /*throw()*/ {whiteSpaceColor_ = color;}

/**
 * Sets the appearances of line terminators.
 * @param show set @c true to show
 */
inline void DefaultSpecialCharacterRenderer::showLineTerminators(bool show) /*throw()*/ {showsEOLs_ = show;}

/**
 * Sets the appearances of white space characters.
 * @param show set @c true to show
 */
inline void DefaultSpecialCharacterRenderer::showWhiteSpaces(bool show) /*throw()*/ {showsWhiteSpaces_ = show;}

/// Returns @c true if line terminators are visible.
inline bool DefaultSpecialCharacterRenderer::showsLineTerminators() const /*throw()*/ {return showsEOLs_;}

/// Returns @c true if white space characters are visible.
inline bool DefaultSpecialCharacterRenderer::showsWhiteSpaces() const /*throw()*/ {return showsWhiteSpaces_;}

/// Returns the primary font.
inline std::tr1::shared_ptr<const Font> TextRenderer::primaryFont() const /*throw()*/ {return primaryFont_;}

/// @see ILayoutInformationProvider#textMetrics
inline const Font::Metrics& TextRenderer::textMetrics() const /*throw()*/ {return primaryFont()->metrics();}

}} // namespace ascension.graphics

#endif // !ASCENSION_LAYOUT_HPP
