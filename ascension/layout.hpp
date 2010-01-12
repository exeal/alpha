/**
 * @file layout.hpp
 * @date 2003-2006 (was LineLayout.h)
 * @date 2006-2009
 */

#ifndef ASCENSION_LAYOUT_HPP
#define ASCENSION_LAYOUT_HPP
#include "document.hpp"
#include "unicode-property.hpp"
#include "presentation.hpp"
#include <manah/win32/dc.hpp>
#include <manah/win32/gdi-object.hpp>
#include <vector>

// forward declaration
struct tag_SCRIPT_ITEM;
typedef tag_SCRIPT_ITEM SCRIPT_ITEM;

namespace ascension {

	namespace presentation {
		struct StyledText;
		struct LineStyle;
	}

	namespace presentation {class Presentation;}
	namespace viewers {class Caret;}

	namespace layout {

		// free functions
		bool getDecorationLineMetrics(HDC dc, int* baselineOffset,
			int* underlineOffset, int* underlineThickness, int* strikethroughOffset, int* strikethroughThickness) /*throw()*/;
		bool supportsComplexScripts() /*throw()*/;
		bool supportsOpenTypeFeatures() /*throw()*/;

		/// Alignment of the text layout.
		enum Alignment {
			ALIGN_AUTO,		///< The alignment is automatically determined. Some methods don't accept this value.
			ALIGN_LEFT,		///< The text is aligned to left.
			ALIGN_RIGHT,	///< The text is aligned to right.
			ALIGN_CENTER	///< The text is aligned to center. Some methods don't accept this value.
		};

		/// Orientation of the text layout.
		enum Orientation {
			LEFT_TO_RIGHT,	///< The text is left-to-right.
			RIGHT_TO_LEFT	///< The text is right-to-left.
		};

		/// Digit shape (substitution) types.
		enum DigitSubstitutionType {
			DST_NOMINAL,		///< Digits are not substituted.
			DST_NATIONAL,		///< Digits are substituted by the native digits of the user's locale.
			DST_CONTEXTUAL,		///< Digits are substituted using the language of the prior letter.
			DST_USER_DEFAULT	///< Follows the NLS native digit and digit substitution settings.
		};

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
		 * @see ILayoutInformationProvider#getLayoutSettings, TextViewer#Configuration
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
			/// Orientation ("paragraph direction") of the lines. Default value is @c ASCENSION_DEFAULT_TEXT_ORIENTATION.
			Orientation orientation;
			/// Alignment of the lines. Default value is @c ASCENSION_DEFAULT_TEXT_ALIGNMENT.
			Alignment alignment;
			/// Line wrap configuration.
			LineWrapConfiguration lineWrap;
			/// Set @c true to justify the lines if wrapped. Default value is @c false.
			bool justifiesLines;
			/// Set @c true to inhibit any shaping. Default value is @c false.
			bool inhibitsShaping;
			/// If set to @c true, zero width control characters are shaped as representative glyphs. Default is @c false.
			bool displaysShapingControls;
			/// Set @c true to inhibit from generating mirrored glyphs. Default value is @c false.
			bool inhibitsSymmetricSwapping;
			/// Set @c true to make the deprecated format characters (NADS, NODS, ASS, and ISS) not effective. Default value is @c false.
			bool disablesDeprecatedFormatCharacters;
			/// Digits substitution type. Default value is @c DST_USER_DEFAULT.
			DigitSubstitutionType digitSubstitutionType;
			/// Constructor initializes the all members to their default values.
			LayoutSettings() /*throw()*/ : tabWidth(8), lineSpacing(0),
				orientation(ASCENSION_DEFAULT_TEXT_ORIENTATION), alignment(ASCENSION_DEFAULT_TEXT_ALIGNMENT),
				justifiesLines(false), inhibitsShaping(false), displaysShapingControls(false), inhibitsSymmetricSwapping(false),
				disablesDeprecatedFormatCharacters(false), digitSubstitutionType(DST_USER_DEFAULT) {}
			/// Returns @c true if the all mwmbers are valid.
			bool verify() const /*throw()*/ {return lineWrap.verify() && tabWidth > 0 && lineSpacing >= 0;}
		};

		/*
		 * Interface for objects which are interested in change of a @c FontSelector.
		 * @see FontSelector#addFontListener, FontSelector#removeFontListener
		 */
		class IFontSelectorListener {
		private:
			/// The font settings was changed.
			virtual void fontChanged() = 0;
			friend class FontSelector;
		};

		/**
		 * A @c FontSelector holds a primary font, the font metrics and a font association table
		 * for text rendering.
		 * 
		 * This class supports Font Linking (limited) mechanism for CJK end users. And also supports
		 * Font Fallback mechanism for multilingual text display.
		 * @see TextRenderer
		 */
		class FontSelector {
			MANAH_UNASSIGNABLE_TAG(FontSelector);
		public:
			/// Font association table consists of pairs of a script and a font familiy name.
			typedef std::map<int, std::basic_string<WCHAR> > FontAssociations;

			// device context
			std::auto_ptr<manah::win32::gdi::DC> deviceContext() const;

			// metrics
			int ascent() const /*throw()*/;
			int averageCharacterWidth() const /*throw()*/;
			int descent() const /*throw()*/;
			int lineGap() const /*throw()*/;
			int lineHeight() const /*throw()*/;

			// primary font and alternatives
			HFONT font(int script = text::ucd::Script::COMMON, bool bold = false, bool italic = false) const;
			HFONT fontForShapingControls() const /*throw()*/;
			void setFont(const WCHAR* faceName, int height, const FontAssociations* associations);

			// default settings
			static const FontAssociations& getDefaultFontAssociations() /*throw()*/;

			// font linking
			void enableFontLinking(bool enable = true) /*throw()*/;
			bool enablesFontLinking() const /*throw()*/;
			HFONT linkedFont(std::size_t index, bool bold = false, bool italic = false) const;
			std::size_t numberOfLinkedFonts() const /*throw()*/;

			// listener
			void addFontListener(IFontSelectorListener& listener);
			void removeFontListener(IFontSelectorListener& listener);

		protected:
			FontSelector();
			FontSelector(const FontSelector& rhs);
			virtual ~FontSelector() /*throw()*/;
			virtual std::auto_ptr<manah::win32::gdi::DC> getDeviceContext() const = 0;
			virtual void fontChanged() = 0;
		private:
			struct Fontset;
			void fireFontChanged();
			HFONT fontInFontset(const Fontset& fontset, bool bold, bool italic) const /*throw()*/;
			void linkPrimaryFont() /*throw()*/;
			void resetPrimaryFont(manah::win32::gdi::DC& dc, HFONT font);
			int ascent_, descent_, internalLeading_, externalLeading_, averageCharacterWidth_;
			Fontset* primaryFont_;
			std::map<int, Fontset*> associations_;
			HFONT shapingControlsFont_;			// for shaping control characters (LRM, ZWJ, NADS, ASS, AAFS, ...)
			std::vector<Fontset*>* linkedFonts_;	// for the font linking feature
			ascension::internal::Listeners<IFontSelectorListener> listeners_;
			static FontAssociations defaultAssociations_;
		};

		class TextRenderer;

		class ISpecialCharacterRenderer {
		public:
			/// Destructor.
			virtual ~ISpecialCharacterRenderer() /*throw()*/ {}
		protected:
			/// Context of the layout.
			struct LayoutContext {
				MANAH_UNASSIGNABLE_TAG(LayoutContext);
			public:
				mutable manah::win32::gdi::DC& dc;	///< the device context.
				Orientation orientation;			///< the orientation of the character.
				/// Constructor.
				explicit LayoutContext(manah::win32::gdi::DC& deviceContext) /*throw()*/ : dc(deviceContext) {}
			};
			/// Context of the drawing.
			struct DrawingContext : public LayoutContext {
				RECT rect;	///< the bounding box to draw.
				/// Constructor.
				DrawingContext(manah::win32::gdi::DC& deviceContext) /*throw()*/ : LayoutContext(deviceContext) {}
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

		class DefaultSpecialCharacterRenderer : public ISpecialCharacterRenderer, public IFontSelectorListener {
		public:
			// constructors
			DefaultSpecialCharacterRenderer() /*throw()*/;
			~DefaultSpecialCharacterRenderer() /*throw()*/;
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
			// IFontSelectorListener
			void fontChanged();
		private:
			TextRenderer* renderer_;
			COLORREF controlColor_, eolColor_, wrapMarkColor_, whiteSpaceColor_;
			bool showsEOLs_, showsWhiteSpaces_;
			HFONT font_;	// provides substitution glyphs
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
			/// Returns the font selector.
			virtual const FontSelector& getFontSelector() const /*throw()*/ = 0;
			/// Returns the layout settings.
			virtual const LayoutSettings& getLayoutSettings() const /*throw()*/ = 0;
			/// Returns the presentation object.
			virtual const presentation::Presentation& getPresentation() const /*throw()*/ = 0;
			/// Returns the special character renderer.
			virtual ISpecialCharacterRenderer* getSpecialCharacterRenderer() const /*throw()*/ = 0;
			/// Returns the width of the rendering area in pixels.
			virtual int getWidth() const /*throw()*/ = 0;
		};

		namespace internal {struct Run;}

		class LineLayout {
			MANAH_NONCOPYABLE_TAG(LineLayout);
		public:
			/// Edge of a character.
			enum Edge {
				LEADING,	///< Leading edge of a character.
				TRAILING	///< Trailing edge of a character.
			};
			/// Used for @c LineLayout#draw methods.
			class Selection {
				MANAH_UNASSIGNABLE_TAG(Selection);
			public:
				/// Constructor.
				explicit Selection(const viewers::Caret& caret) /*throw()*/;
				/// Constructor.
				Selection(const viewers::Caret& caret, const presentation::Colors& color);
				/// Returns the caret object.
				const viewers::Caret& caret() const /*throw()*/ {return caret_;}
				/// Returns the color to render.
				const presentation::Colors& color() const /*throw()*/ {return color_;}
			private:
				const viewers::Caret& caret_;
				const presentation::Colors color_;
			};
			/// Bidirectional iterator enumerates style runs in a line.
			class StyledSegmentIterator {
			public:
				// constructors
				StyledSegmentIterator(const StyledSegmentIterator& rhs) /*throw()*/;
				// operators
				StyledSegmentIterator& operator=(const StyledSegmentIterator& rhs) /*throw()*/;
				// methods
				const presentation::StyledText& current() const /*throw()*/;
				bool equals(const StyledSegmentIterator& rhs) const /*throw()*/;
				StyledSegmentIterator& next() /*throw()*/;
				StyledSegmentIterator& previous() /*throw()*/;
			private:
				explicit StyledSegmentIterator(const internal::Run*& start) /*throw()*/;
				const internal::Run** p_;
				friend class LineLayout;
			};

			// constructors
			LineLayout(const ILayoutInformationProvider& layoutInformation, length_t line);
			~LineLayout() /*throw()*/;
			// general attributes
			byte bidiEmbeddingLevel(length_t column) const;
			bool isBidirectional() const /*throw()*/;
			bool isDisposed() const /*throw()*/;
			length_t lineNumber() const /*throw()*/;
			// subline accesses
			length_t numberOfSublines() const /*throw()*/;
			length_t subline(length_t column) const;
			length_t sublineLength(length_t subline) const;
			length_t sublineOffset(length_t subline) const;
			const length_t* sublineOffsets() const /*throw()*/;
			// coordinates
			manah::win32::gdi::Rgn blackBoxBounds(length_t first, length_t last) const;
			SIZE bounds() const /*throw()*/;
			RECT bounds(length_t first, length_t last) const;
			POINT location(length_t column, Edge edge = LEADING) const;
			std::pair<POINT, POINT> locations(length_t column) const;
			int longestSublineWidth() const /*throw()*/;
			std::pair<length_t, length_t> offset(int x, int y, bool* outside = 0) const /*throw()*/;
			std::pair<length_t, length_t> offset(const POINT& pt, bool* outside = 0) const /*throw()*/;
			RECT sublineBounds(length_t subline) const;
			int sublineIndent(length_t subline) const;
			int sublineWidth(length_t subline) const;
			// styled segments
			StyledSegmentIterator firstStyledSegment() const /*throw()*/;
			StyledSegmentIterator lastStyledSegment() const /*throw()*/;
			const presentation::StyledText& styledSegment(length_t column) const;
			// operations
			void draw(manah::win32::gdi::DC& dc, int x, int y,
				const RECT& paintRect, const RECT& clipRect, const Selection* selection) const /*throw()*/;
			void draw(length_t subline, manah::win32::gdi::DC& dc, int x, int y,
				const RECT& paintRect, const RECT& clipRect, const Selection* selection) const;
			String fillToX(int x) const;
#ifdef _DEBUG
			void dumpRuns(std::ostream& out) const;
#endif // _DEBUG

		private:
			void dispose() /*throw()*/;
			void expandTabsWithoutWrapping() /*throw()*/;
			std::size_t findRunForPosition(length_t column) const /*throw()*/;
			void itemize(length_t lineNumber) /*throw()*/;
			void justify() /*throw()*/;
			int linePitch() const /*throw()*/;
			void locations(length_t column, POINT* leading, POINT* trailing) const;
			void merge(const SCRIPT_ITEM items[], std::size_t numberOfItems, const presentation::LineStyle& styles) /*throw()*/;
			int nextTabStop(int x, Direction direction) const /*throw()*/;
			const String& text() const /*throw()*/;
			void reorder() /*throw()*/;
//			void rewrap();
			void shape() /*throw()*/;
			int nextTabStopBasedLeftEdge(int x, bool right) const /*throw()*/;
			void wrap() /*throw()*/;
		private:
			const ILayoutInformationProvider& lip_;
			length_t lineNumber_;
			internal::Run** runs_;
			std::size_t numberOfRuns_;
			length_t* sublineOffsets_;		// size is numberOfSublines_
			length_t* sublineFirstRuns_;	// size is numberOfSublines_
			length_t numberOfSublines_;
			int longestSublineWidth_;
			int wrapWidth_;	// -1 if should not wrap
			friend class LineLayoutBuffer;
			friend class StyledSegmentIterator;
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
			MANAH_NONCOPYABLE_TAG(LineLayoutBuffer);
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
			void	setLayoutInformation(const ILayoutInformationProvider* newProvider, bool delegateOwnership);
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

		/**
		 * @c TextRenderer renders styled text to the display or to a printer. Although this class
		 * extends @c FontSelector class and implements @c ILayoutInformationProvider interface,
		 * @c FontSelector#doGetDeviceContext, @c ILayoutInformationProvider#getLayoutSettings, and
		 * @c ILayoutInformationProvider#getWidth methods are not defined (An internal extension
		 * @c TextViewer#Renderer class implements these).
		 * @see LineLayout, LineLayoutBuffer, FontSelector, Presentation
		 */
		class TextRenderer : public LineLayoutBuffer, public FontSelector, public ILayoutInformationProvider {
		public:
			// constructors
			TextRenderer(presentation::Presentation& presentation, bool enableDoubleBuffering);
			TextRenderer(const TextRenderer& rhs);
			virtual ~TextRenderer() /*throw()*/;
			// attributes
			int linePitch() const /*throw()*/;
			int lineIndent(length_t line, length_t subline = 0) const;
			// strategy
			void setSpecialCharacterRenderer(ISpecialCharacterRenderer* newRenderer, bool delegateOwnership);
			// operation
			void renderLine(length_t line, manah::win32::gdi::DC& dc, int x, int y,
				const RECT& paintRect, const RECT& clipRect, const LineLayout::Selection* selection) const /*throw()*/;
		private:
			// FontSelector
//			std::auto_ptr<manah::win32::gdi::DC> getDeviceContext() const;
			void fontChanged();
			// ILayoutInformation
			const FontSelector& getFontSelector() const /*throw()*/;
//			const LayoutSettings& getLayoutSettings() const /*throw()*/;
			const presentation::Presentation& getPresentation() const /*throw()*/;
			ISpecialCharacterRenderer* getSpecialCharacterRenderer() const /*throw()*/;
//			int getWidth() const /*throw()*/;
		private:
			presentation::Presentation& presentation_;
			const bool enablesDoubleBuffering_;
			mutable manah::win32::gdi::DC memoryDC_;
			mutable manah::win32::gdi::Bitmap memoryBitmap_;
			ascension::internal::StrategyPointer<ISpecialCharacterRenderer> specialCharacterRenderer_;
		};


// inlines //////////////////////////////////////////////////////////////////

/// Returns if the layout has been disposed.
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
inline POINT LineLayout::location(length_t column, Edge edge /* = LEADING */) const {
	POINT result;
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
inline std::pair<POINT, POINT> LineLayout::locations(length_t column) const {
	std::pair<POINT, POINT> result;
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
	const POINT& pt, bool* outside /* = 0 */) const /*throw()*/ {return offset(pt.x, pt.y);}

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

/// Asignment operator.
inline LineLayout::StyledSegmentIterator&
	LineLayout::StyledSegmentIterator::operator=(const StyledSegmentIterator& rhs) /*throw()*/ {p_ = rhs.p_; return *this;}

/// Returns @c true if the two iterators address the same segment.
inline bool LineLayout::StyledSegmentIterator::equals(const StyledSegmentIterator& rhs) const /*throw()*/ {return p_ == rhs.p_;}

/// Moves to the next.
inline LineLayout::StyledSegmentIterator& LineLayout::StyledSegmentIterator::next() /*throw()*/ {++p_; return *this;}

/// Moves to the previous.
inline LineLayout::StyledSegmentIterator& LineLayout::StyledSegmentIterator::previous() /*throw()*/ {--p_; return *this;}

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

/**
 * Registers the font selector listener.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
inline void FontSelector::addFontListener(IFontSelectorListener& listener) {listeners_.add(listener);}

/// Returns the ascent of the text.
inline int FontSelector::ascent() const /*throw()*/ {return ascent_;}

/// Returns average width of a character.
inline int FontSelector::averageCharacterWidth() const /*throw()*/ {return averageCharacterWidth_;}

/// Returns the descent of the text.
inline int FontSelector::descent() const /*throw()*/ {return descent_;}

/// Returns the device context.
inline std::auto_ptr<manah::win32::gdi::DC> FontSelector::deviceContext() const {return getDeviceContext();}

/// Returns the font linking is enabled.
inline bool FontSelector::enablesFontLinking() const /*throw()*/ {return linkedFonts_ != 0;}

/**
 * Returns the ideal line gap (external leading of the primary font).
 * @see LayoutSettings#lineSpacing
 */
inline int FontSelector::lineGap() const /*throw()*/ {return externalLeading_;}

/**
 * Returns the height of the lines.
 * @see TextRenderer#getLinePitch
 */
inline int FontSelector::lineHeight() const /*throw()*/ {return ascent_ + descent_;}

/// Returns the number of the linked fonts.
inline std::size_t FontSelector::numberOfLinkedFonts() const /*throw()*/ {return (linkedFonts_ != 0) ? linkedFonts_->size() : 0;}

/**
 * Removes the font selector listener.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
inline void FontSelector::removeFontListener(IFontSelectorListener& listener) {listeners_.remove(listener);}

}} // namespace ascension.viewers

#endif // !ASCENSION_LAYOUT_HPP
