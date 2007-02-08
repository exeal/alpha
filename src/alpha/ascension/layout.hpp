/**
 * @file layout.hpp
 * @date 2003-2006 (was LineLayout.h)
 * @date 2006-2007
 */

#ifndef ASCENSION_LAYOUT_HPP
#define ASCENSION_LAYOUT_HPP
#include "document.hpp"
#include "../../manah/win32/dc.hpp"
#include <usp10.h>

namespace ascension {

	namespace presentation {
		struct StyledText;
		struct LineStyle;
	}

	namespace viewers {

		const COLORREF STANDARD_COLOR = 0xFFFFFFFF;		///< Standard color (This value introduces any fallback).
		const COLORREF SYSTEM_COLOR_MASK = 0x80000000;	///< Used like @c (index | SYSTEM_COLOR_MASK) to represent a system color.

		/// Alignment of the text layout.
		enum Alignment {
			ALIGN_AUTO,		///< The alignment is automatically determined. Some methods don't accept this value.
			ALIGN_LEFT,		///< The text is aligned to left.
			ALIGN_RIGHT,	///< The text is aligned to right.
			ALIGN_CENTER,	///< The text is aligned to center. Some methods don't accept this value.
//			JUSTIFY			///< The text is justified. Some methods don't accept this value.
		};

		/// Orientation of the text layout.
		enum Orientation {
			LEFT_TO_RIGHT,	///< The text is left-to-right.
			RIGHT_TO_LEFT	///< The text is right-to-left.
		};

		/// Type of the border and underline.
		enum BorderStyle {
			BS_NONE,					///< None
			BS_UNDERLINE_SOLID,			///< Solid underline
			BS_UNDERLINE_BOLD,			///< Bold solid underline
			BS_UNDERLINE_DASHED,		///< Dashed underline
			BS_UNDERLINE_BOLD_DASHED,	///< Bold dashed underline
			BS_UNDERLINE_DOTTED,		///< Dotted underline
			BS_UNDERLINE_BOLD_DOTTED,	///< Bold dotted underline
			BS_UNDERLINE_WAVED,			///< Waved underline
			BS_BORDER_SOLID,			///< Solid border
			BS_BORDER_DASHED,			///< Dashed border
			BS_BORDER_DOTTED,			///< Dotted border
		};

		/// Digit shape (substitution) types.
		enum DigitSubstitutionType {
			DST_NOMINAL,		///< Digits are not substituted.
			DST_NATIONAL,		///< Digits are substituted by the native digits of the user's locale.
			DST_CONTEXTUAL,		///< Digits are substituted using the language of the prior letter.
			DST_USER_DEFAULT	///< Follows the NLS native digit and digit substitution settings.
		};

		/// Foreground color and background.
		struct Colors {
			COLORREF foreground;	///< Color of foreground (text)
			COLORREF background;	///< Color of background
			static const Colors STANDARD;	///< Standard color. This value introduces any fallback.
			/**
			 * Constructor initializes the each colors.
			 * @param foregroundColor foreground color
			 * @param backgroundColor background color
			 */
			explicit Colors(COLORREF foregroundColor = STANDARD_COLOR,
				COLORREF backgroundColor = STANDARD_COLOR) throw() : foreground(foregroundColor), background(backgroundColor) {}
		};

		/**
		 * Configuration about line wrapping.
		 * @see Presentation#Configurations#lineWrap
		 */
		struct LineWrapConfiguration {
			/// Algorithm for wrap
			enum Alrorithm {
				NO_WRAP,			///< Does not wrap the line.
				GLYPH_BOUNDARIES,	///< Wraps any glyph boundaries.
				UNICODE_UAX_14,		///< Follows Unicode UAX #14: Line Breaking Properties
				JIS_X_4051			///< Follows JIS X 4051:2004 『日本語文書の行組版方法』
			} algorithm;	///< The algorithm
			/**
			 * The maximum line width.
			 *
			 * This value must be grater than or equal to zero.
			 * If set to zero, the lines will be wrapped at the window edge.
			 */
			int width;
			/// Default constructor.
			LineWrapConfiguration() throw() : algorithm(NO_WRAP), width(0) {};
			/// Returns true if the all members are valid.
			bool verify() const throw() {return width >= 0;}
			/// Returns true if @c algorithm is not @c NO_WRAP.
			bool wraps() const throw() {return algorithm != NO_WRAP;}
			/// Returns true if @c algorithm is not @c NO_WRAP and @c width is zero.
			bool wrapsAtWindowEdge() const throw() {return wraps() && width == 0;}
		};

		class TextViewer;
		class TextRenderer;

		/**
		 * @c LineLayout represents a layout of styled line text.
		 * Provides support for drawing, cursor navigation, hit testing, text wrapping, etc.
		 * @note This class is underivable.
		 * @see TextRenderer#getLineLayout
		 */
		class LineLayout : public manah::Noncopyable {
		private:
			struct Run;
		public:
			/// Edge of a character.
			enum Edge {
				LEADING,	///< Leading edge of a character
				TRAILING	///< Trailing edge of a character
			};
			/// Bidirectional iterator enumerates style runs in a line.
			class StyledSegmentIterator : public std::iterator<std::bidirectional_iterator_tag, const presentation::StyledText> {
			public:
				// constructors
				StyledSegmentIterator(const StyledSegmentIterator& rhs) throw();
				// operators
				StyledSegmentIterator&			operator=(const StyledSegmentIterator& rhs) throw();
				const presentation::StyledText&	operator*() const throw();
				const presentation::StyledText&	operator->() const throw();
				StyledSegmentIterator&			operator++() throw();
				const StyledSegmentIterator		operator++(int) throw();
				StyledSegmentIterator&			operator--() throw();
				const StyledSegmentIterator		operator--(int) throw();
				bool							operator==(const StyledSegmentIterator& rhs) const throw();
				bool							operator!=(const StyledSegmentIterator& rhs) const throw();
			private:
				explicit StyledSegmentIterator(const Run*& start) throw();
				const Run** p_;
				friend LineLayout;
			};

			// constructors
			LineLayout(const TextRenderer& textRenderer, length_t line);
			~LineLayout() throw();
			// attributes
			uchar			getBidiEmbeddingLevel(length_t column) const;
			::SIZE			getBounds() const throw();
			::RECT			getBounds(length_t first, length_t last) const;
			::POINT			getLocation(length_t column, Edge edge = LEADING) const;
			length_t		getNumberOfSublines() const throw();
			length_t		getOffset(int x, int y, Edge edge = LEADING) const throw();
			length_t		getOffset(int x, int y, length_t& trailing) const throw();
			length_t		getOffset(const ::POINT& pt, Edge edge = LEADING) const throw();
			length_t		getOffset(const ::POINT& pt, length_t& trailing) const throw();
			length_t		getSubline(length_t column) const;
			::RECT			getSublineBounds(length_t subline) const;
			int				getSublineIndent(length_t subline) const;
			length_t		getSublineLength(length_t subline) const;
			length_t		getSublineOffset(length_t subline) const;
			const length_t*	getSublineOffsets() const throw();
			int				getSublineWidth(length_t subline) const;
			int				getWidth() const throw();
			bool			isBidirectional() const throw();
			bool			isDisposed() const throw();
			// styled segments
			StyledSegmentIterator			getFirstStyledSegment() const throw();
			StyledSegmentIterator			getLastStyledSegment() const throw();
			const presentation::StyledText&	getStyledSegment(length_t column) const;
			// operations
			void	draw(manah::windows::gdi::PaintDC& dc, int x, int y, const ::RECT& clipRect, const Colors& selectionColor) const throw();
#ifdef _DEBUG
			void	dumpRuns(std::ostream& out) const;
#endif /* _DEBUG */
		private:
			void			dispose() throw();
			void			drawBorder(manah::windows::gdi::PaintDC& dc, const ::RECT& bounds, BorderStyle style, COLORREF color) const throw();
			void			expandTabsWithoutWrapping() throw();
			std::size_t		findRunForPosition(length_t column) const throw();
			int				getNextTabStop(int x, Direction direction) const throw();
			int				getNextTabStopBasedLeftEdge(int x, bool right) const throw();
			const String&	getText() const throw();
			void			itemize(length_t lineNumber) throw();
			void			merge(const ::SCRIPT_ITEM items[], std::size_t numberOfItems, const presentation::LineStyle& styles) throw();
			void			reorder() throw();
//			void			rewrap();
			bool			shape() throw();
			bool			shape(struct Run& run) throw();
			void			wrap() throw();
		private:
			const TextRenderer& renderer_;
			length_t lineNumber_;
			Run** runs_;
			std::size_t numberOfRuns_;
			length_t* sublineOffsets_;		// size is numberOfSublines_
			length_t* sublineFirstRuns_;	// size is numberOfSublines_
			length_t numberOfSublines_;
			int width_;
			friend class LineLayoutBuffer;
			friend StyledSegmentIterator;
		};

		/**
		 * Manages a continuous caches of layout (@c LineLayout).
		 * @see LineLayout, TextRenderer
		 */
		class LineLayoutBuffer : public manah::Noncopyable,
				virtual public text::IDocumentListener/*, virtual public presentation::IPresentationStylistListener*/ {
		public:
			// attributes
			const LineLayout&	getLineLayout(length_t line) const;
			TextViewer&			getTextViewer() throw();
			const TextViewer&	getTextViewer() const throw();
			bool				isLineCached(length_t line) const;
			// operations
			void	invalidate() throw();
			void	invalidate(length_t first, length_t last);
		protected:
			LineLayoutBuffer(TextViewer& viewer, length_t bufferSize, bool autoRepair);
			virtual ~LineLayoutBuffer() throw();
			void			documentAboutToBeChanged(const text::Document& document);
			void			documentChanged(const text::Document& document, const text::DocumentChange& change);
			length_t		getCacheFirstLine() const throw();
			length_t		getCacheLastLine() const throw();
			virtual void	layoutDeleted(length_t first, length_t last, length_t sublines) throw() = 0;
			virtual void	layoutInserted(length_t first, length_t last) throw() = 0;
			virtual void	layoutModified(length_t first, length_t last,
								length_t newSublines, length_t oldSublines, bool documentChanged) throw() = 0;
		private:
			void	clearCaches(length_t first, length_t last, bool repair);
			void	createLineLayout(length_t line) throw();
			void	deleteLineLayout(length_t line, LineLayout* newLayout = 0) throw();
			void	invalidateLineLayout(length_t line) throw();
			void	presentationStylistChanged();
			void	slideCaches(length_t first, signed_length_t offset, bool callModified) throw();
			void	updateCacheStartLine(length_t line) throw();
			void	verifyLayoutCaches() const throw();
		private:
			TextViewer& viewer_;
			LineLayout** layouts_;
			const length_t bufferSize_;	// layouts_ の要素数
			length_t startLine_;		// layouts_[0] が何行目のデータか
			const bool autoRepair_;
			enum {ABOUT_CHANGE, CHANGING, NONE} documentChangePhase_;
			struct {
				length_t first, last;
			} pendingCacheClearance_;	// ドキュメント変更中に呼び出された clearCaches の引数
		};

		/**
		 * Interface for objects which are interested in getting informed about change of visual lines of @c TextRenderer.
		 * @see TextRenderer#addVisualLinesListener, TextRenderer#removeVisualLinesListener
		 */
		class IVisualLinesListener {
		private:
			/**
			 * The font settings was changed.
			 * @note This is invoked after #visualLinesModified call.
			 */
			virtual void rendererFontChanged() throw() = 0;
			/**
			 * Several visual lines were deleted.
			 * @param first the first of created lines
			 * @param last the last of created lines (exclusive)
			 * @param sublines the total number of sublines of created lines
			 * @param true if the longest line is changed
			 */
			virtual void visualLinesDeleted(length_t first, length_t last, length_t sublines, bool longestLineChanged) throw() = 0;
			/**
			 * Several visual lines were inserted.
			 * @param first the first of inserted lines
			 * @param last the last of inserted lines (exclusive)
			 */
			virtual void visualLinesInserted(length_t first, length_t last) throw() = 0;
			/**
			 * A visual lines were modified.
			 * @param first the first of modified lines
			 * @param last the last of modified lines (exclusive)
			 * @param sublinesDifference the difference of the number of sublines between before and after the modification
			 * @param documentChanged true if the layouts were modified for the document change
			 * @param true if the longest line is changed
			 */
			virtual void visualLinesModified(length_t first, length_t last,
				signed_length_t sublinesDifference, bool documentChanged, bool longestLineChanged) throw() = 0;
			friend TextRenderer;
		};

		/**
		 * Interface for objects which draw special characters.
		 * @see TextRenderer, TextRenderer#setSpecialCharacterDrawer
		 */
		class ISpecialCharacterDrawer {
		protected:
			/// Context of the rendering.
			struct Context {
				mutable manah::windows::gdi::PaintDC& dc;	///< the device context
				Colors color;								///< the color
				Orientation orientation;					///< the orientation of the character
				/// Constructor.
				explicit Context(manah::windows::gdi::PaintDC& deviceContext) : dc(deviceContext) {}
			};
			/// Destructor.
			virtual ~ISpecialCharacterDrawer() throw() {}
		private:
			/**
			 * Draws an ASCII control character.
			 * @param context the context
			 * @param rect the rectangle to be drawn
			 * @param ch the ASCII control character to be drawn
			 */
			virtual void drawASCIIControl(const Context& context, const ::RECT& rect, uchar ch) = 0;
			/**
			 * Draws a line terminator symbol.
			 * @param context the context
			 * @param x the x-coordinate of the position
			 * @param y the y-coordinate of the position
			 * @param lineBreak the line break to be drawn
			 */
			virtual void drawLineTerminator(const Context& context, int x, int y, text::LineBreak lineBreak) = 0;
			/**
			 * Draws a line terminator symbol.
			 * @param context the context
			 * @param x the x-coordinate of the position
			 * @param y the y-coordinate of the position
			 */
			virtual void drawLineWrappingMark(const Context& context, int x, int y) = 0;
			/**
			 * Draws a substitution glyph for the specified white space character.
			 * @param context the context
			 * @param rect the rectangle to be drawn
			 * @param cp the code point of the white space character to be drawn
			 */
			virtual void drawWhiteSpace(const Context& context, const ::RECT& rect, CodePoint cp) = 0;
			/**
			 * Returns the width of a line wrapping mark.
			 * @param orientation the orientation
			 * @return the width
			 */
			virtual int getLineWrappingMarkWidth(Orientation orientation) const throw() = 0;
			/**
			 * Installs the drawer.
			 * @param textRenderer the text renderer
			 */
			virtual void install(TextRenderer& textRenderer) = 0;
			/// Uninstalls the drawer.
			virtual void uninstall() = 0;
			friend LineLayout;
			friend TextRenderer;
			friend ascension::internal::StrategyPointer<ISpecialCharacterDrawer>;
		};

		/**
		 * Default implementation of @c ISpecialCharacterDrawer interface.
		 * @c SpecialCharacterSubstitutionGlyphDrawer uses the glyphs of the specified character and renderer's primary font.
		 */
		class SpecialCharacterSubstitutionGlyphDrawer : virtual public ISpecialCharacterDrawer {
		public:
			struct SubstitutionGlyphs {
				Char horizontalTab;			///< Horizontal tab (U+0009). Default value is '^' (U+005E)
				Char generalWhiteSpace;		///< General white spaces (except U+1680: Ogham Space Mark). Default value is '_' (U+005F)
				Char ideographicSpace;		///< Ideographic space (U+3000). Default value is '□' (U+25A1)
				Char unixEOL;				///< LF (U+000A). Default value is '/' (U+002F) for LTR or '\' (U+005C) for RTL
				Char macintoshEOL;			///< CR (U+000D). Default value is '/' (U+002F) for LTR or '\' (U+005C) for RTL
				Char windowsEOL;			///< CRLF (U+000D+000A). Default value is '/' (U+002F) for LTR or '\' (U+005C) for RTL
				Char ebcdicEOL;				///< NEL (U+0085). Default value is '/' (U+002F) for LTR or '\' (U+005C) for RTL
				Char lineSeparator;			///< LS (U+2028). Default value is '/' (U+002F) for LTR or '\' (U+005C) for RTL
				Char paragraphSeparator;	///< PS (U+2029). Default value is '/' (U+002F) for LTR or '\' (U+005C) for RTL
				Char lineWrappingMarker;	///< Sign indicates the line is wrapped. Default value is '<' (U+003C) for LTR or '>' (U+003E) for LTR
//				String endOfFile;			///< End of file. Default is "[EOF]"
			};
		private:
			void	drawASCIIControl(const Context& context, const ::RECT& rect, uchar ch);
			void	drawLineTerminator(const Context& context, int x, int y, text::LineBreak lineBreak);
			void	drawLineWrappingMark(const Context& context, int x, int y);
			void	drawWhiteSpace(const Context& context, const ::RECT& rect, CodePoint cp);
			int		getLineWrappingMarkWidth(Orientation orientation) const throw();
			void	install(TextRenderer& textRenderer);
			void	uninstall();
		private:
			SubstitutionGlyphs glyphsForLTR_, glyphsForRTL_;
		};

		/**
		 * @c TextRenderer renders styled text to the display or to a printer.
		 * @note This class is underivable.
		 * @see LineLayout, LineLayoutBuffer, Presentation, TextViewer
		 */
		class TextRenderer : public LineLayoutBuffer {
		public:
			// constructors
			explicit TextRenderer(TextViewer& viewer);
			~TextRenderer() throw();
			// attributes
			int				getAscent() const throw();
			int				getAverageCharacterWidth() const throw();
			int				getDescent() const throw();
			HFONT			getFont(uchar language = LANG_NEUTRAL, bool bold = false, bool italic = false) const throw();
			HFONT			getFontForShapingControls() const throw();
			int				getLineHeight() const throw();
			int				getLinePitch() const throw();
			int				getLongestLineWidth() const throw();
			length_t		getNumberOfSublinesOfLine(length_t) const;
			length_t		getNumberOfVisualLines() const throw();
			int				getWidth() const throw();
			int				getWrapWidth() const throw();
			length_t		mapLogicalLineToVisualLine(length_t line) const;
			length_t		mapLogicalPositionToVisualPosition(const text::Position& position, length_t* column) const;
			length_t		mapVisualLineToLogicalLine(length_t line, length_t* subline) const;
			text::Position	mapVisualPositionToLogicalPosition(const text::Position& position) const;
			void			setFont(const WCHAR* faceName, int height, const std::map<uchar, const WCHAR*>* associations);
			// class sttributes
			static const std::map<uchar, const WCHAR*>&	getDefaultFontAssociations() throw();
			static bool									supportsComplexScript() throw();
			// listeners and strategies
			void						addVisualLinesListener(IVisualLinesListener& listener);
			ISpecialCharacterDrawer*	getSpecialCharacterDrawer() const throw();
			void						removeVisualLinesListener(IVisualLinesListener& listener);
			void						setSpecialCharacterDrawer(ISpecialCharacterDrawer* newDrawer, bool delegateOwnership) throw();
			// operations
			void	updateViewerSize() throw();
			// utilities
			void	offsetVisualLine(length_t& line, length_t& subline, signed_length_t offset) const throw();
		private:
			void	layoutDeleted(length_t first, length_t last, length_t sublines) throw();
			void	layoutInserted(length_t first, length_t last) throw();
			void	layoutModified(length_t first, length_t last, length_t newSublines, length_t oldSublines, bool documentChanged) throw();
			void	updateLongestLine(length_t line, int width) throw();
		private:
			int ascent_, descent_, averageCharacterWidth_, viewerWidth_, longestLineWidth_, lineWrappingMarkWidth_;
			length_t longestLine_, numberOfVisualLines_;
			struct Fontset {
				WCHAR faceName[LF_FACESIZE];
				HFONT regular, bold, italic, boldItalic;
				explicit Fontset(const WCHAR* name) throw() : regular(0), bold(0), italic(0), boldItalic(0) {std::wcscpy(faceName, name);}
				~Fontset() throw() {clear(L"");}
				void clear(const WCHAR* newName) throw() {
					::DeleteObject(regular); ::DeleteObject(bold); ::DeleteObject(italic); ::DeleteObject(boldItalic);
					regular = bold = italic = boldItalic; std::wcscpy(faceName, newName);}
			};
			Fontset primaryFont_;
			std::map<uchar, Fontset*> fontAssociations_;
			HFONT shapingControlsFont_;	// for shaping control characters (LRM, ZWJ, NADS, ASS, AAFS, ...)
			static std::map<uchar, const WCHAR*> defaultFontAssociations_;
			ascension::internal::Listeners<IVisualLinesListener> visualLinesListeners_;
			ascension::internal::StrategyPointer<ISpecialCharacterDrawer> specialCharacterDrawer_;
		};

		namespace internal {
			/// @c SystemColor caches the system colors.
			static class SystemColors {
			public:
				SystemColors() throw() {update();}
				COLORREF get(int index) const {assert(index >= 0 && index < countof(c_)); return c_[index];}
				COLORREF getReal(COLORREF color, COLORREF defaultColor) const {
					assert(defaultColor != viewers::STANDARD_COLOR);
					if(color == viewers::STANDARD_COLOR) color = defaultColor;
					return toBoolean(color & viewers::SYSTEM_COLOR_MASK) ? get(color & ~viewers::SYSTEM_COLOR_MASK) : color;}
				void update() throw() {for(int i = 0; i < countof(c_); ++i) c_[i] = ::GetSysColor(i);}
			private:
				COLORREF c_[COLOR_MENUBAR + 1];
			} systemColors;
		}


// inlines //////////////////////////////////////////////////////////////////

/// Returns the number of the wrapped lines.
inline length_t LineLayout::getNumberOfSublines() const throw() {return numberOfSublines_;}

/**
 * Returns the character column (offset) for the specified point.
 * @param x the x coordinate of the point. distance from the left edge of the renderer (not of the line)
 * @param y the y coordinate of the point
 * @param edge the edge of the column
 * @return the character offset
 * @see #getLocation
 */
inline length_t LineLayout::getOffset(int x, int y, Edge edge /* = LEADING */) const throw() {
	length_t trailing;
	const length_t offset = getOffset(x, y, trailing);
	return (edge == LEADING) ? offset : offset + trailing;
}

/**
 * Returns the character column (offset) for the specified point.
 * @param pt the point. pt.x is distance from the left edge of the renderer (not of the line)
 * @param edge the edge of the column
 * @return the character offset
 * @see #getLocation
 */
inline length_t LineLayout::getOffset(const ::POINT& pt, Edge edge /* = LEADING */) const throw() {return getOffset(pt.x, pt.y, edge);}

/**
 * Returns the character column (offset) for the specified point.
 * @param pt the point. pt.x is distance from the left edge of the renderer (not of the line)
 * @param[out] trailing the trailing buffer
 * @return the character offset
 * @see #getLocation
 */
inline length_t LineLayout::getOffset(const ::POINT& pt, length_t& trailing) const throw() {return getOffset(pt.x, pt.y, trailing);}

/**
 * Returns the wrapped line containing the specified column.
 * @param column the column
 * @return the wrapped line
 * @throw BadPositionException @p column is greater than the length of the line
 */
inline length_t LineLayout::getSubline(length_t column) const {
	if(column > getText().length())
		throw text::BadPositionException();
	return (numberOfSublines_ == 1) ? 0 :
		ascension::internal::searchBound(static_cast<length_t>(0), numberOfSublines_, column, std::bind1st(std::mem_fun(getSublineOffset), this));
}

/**
 * Returns the length of the specified visual subline.
 * @param subline the visual subline
 * @return the length of the subline
 * @throw BadPositionException @p subline is greater than the count of visual lines
 */
inline length_t LineLayout::getSublineLength(length_t subline) const {
	return (subline < numberOfSublines_ - 1 ? getSublineOffset(subline + 1) : getText().length()) - getSublineOffset(subline);}

/**
 * Returns the offset of the start of the specified visual subline from the start of the logical line.
 * @param subline the visual subline
 * @return the offset
 * @throw BadPositionException @p subline is greater than the count of visual lines
 */
inline length_t LineLayout::getSublineOffset(length_t subline) const {
	if(subline >= numberOfSublines_)
		throw text::BadPositionException();
	return (sublineOffsets_ != 0) ? sublineOffsets_[subline] : 0;
}

/**
 * Returns the line offsets.
 * Each value in the array is the offset for the first character in a line.
 * @return the line offsets whose length is #getNumberOfSublines(), or @c null if the line is empty
 */
inline const length_t* LineLayout::getSublineOffsets() const throw() {return sublineOffsets_;}

/// Returns if the layout has been disposed.
inline bool LineLayout::isDisposed() const throw() {return runs_ == 0;}

/**
 * Private constructor.
 * @param start
 */
inline LineLayout::StyledSegmentIterator::StyledSegmentIterator(const Run*& start) throw() : p_(&start) {}

/// Copy-constructor.
inline LineLayout::StyledSegmentIterator::StyledSegmentIterator(const StyledSegmentIterator& rhs) throw() : p_(rhs.p_) {}

/// Asignment operator.
inline LineLayout::StyledSegmentIterator& LineLayout::StyledSegmentIterator::operator=(const StyledSegmentIterator& rhs) throw() {p_ = rhs.p_;}

/// Prefix increment operator.
inline LineLayout::StyledSegmentIterator& LineLayout::StyledSegmentIterator::operator++() throw() {++p_; return *this;}

/// Postfix increment operator.
inline const LineLayout::StyledSegmentIterator LineLayout::StyledSegmentIterator::operator++(int) throw() {StyledSegmentIterator temp(*this); ++*this; return temp;}

/// Prefix decrement operator.
inline LineLayout::StyledSegmentIterator& LineLayout::StyledSegmentIterator::operator--() throw() {--p_; return *this;}

/// Postfix decrement operator.
inline const LineLayout::StyledSegmentIterator LineLayout::StyledSegmentIterator::operator--(int) throw() {StyledSegmentIterator temp(*this); --*this; return temp;}

/// Equality operator.
inline bool LineLayout::StyledSegmentIterator::operator==(const StyledSegmentIterator& rhs) const throw() {return p_ == rhs.p_;}

/// Inequality operator.
inline bool LineLayout::StyledSegmentIterator::operator!=(const StyledSegmentIterator& rhs) const throw() {return !(*this == rhs);}

/// Returns the first line of the cached layouts.
inline length_t LineLayoutBuffer::getCacheFirstLine() const throw() {return startLine_;}

/// Returns the text viewer.
inline TextViewer& LineLayoutBuffer::getTextViewer() throw() {return viewer_;}

/// Returns the text viewer.
inline const TextViewer& LineLayoutBuffer::getTextViewer() const throw() {return viewer_;}

/**
 * Returns true if the layout of the specified line is cached.
 * @param line the line
 * @return true if @p line is cached
 */
inline bool LineLayoutBuffer::isLineCached(length_t line) const throw() {
	return line >= startLine_ && line < startLine_ + bufferSize_ && layouts_[line - startLine_] != 0;}

/**
 * Registers the visual lines listener.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @p listener is already registered
 */
inline void TextRenderer::addVisualLinesListener(IVisualLinesListener& listener) {visualLinesListeners_.add(listener);}

/// Returns the ascent of the text.
inline int TextRenderer::getAscent() const throw() {return ascent_;}

/// Returns average width of a character.
inline int TextRenderer::getAverageCharacterWidth() const throw() {return averageCharacterWidth_;}

/// Returns the descent of the text.
inline int TextRenderer::getDescent() const throw() {return descent_;}

/// Returns the font to render shaping control characters.
inline HFONT TextRenderer::getFontForShapingControls() const throw() {
	if(shapingControlsFont_ == 0)
		const_cast<TextRenderer*>(this)->shapingControlsFont_ = ::CreateFontW(
			ascent_ + descent_, 0, 0, 0, FW_REGULAR, false, false, false,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
	return shapingControlsFont_;
}

/// Returns the height of the lines.
inline int TextRenderer::getLineHeight() const throw() {return ascent_ + descent_;}

/// Returns the width of the longest line.
inline int TextRenderer::getLongestLineWidth() const throw() {return longestLineWidth_;}

/**
 * Returns the number of sublines of the specified line.
 * If the layout of the line is not calculated, this method returns 1.
 * @param line the line
 * @return the count of the sublines
 * @throw BadPositionException @p line is outside of the document
 * @see #getLineLayout, LineLayout#getNumberOfSublines
 */
inline length_t TextRenderer::getNumberOfSublinesOfLine(length_t line) const {return isLineCached(line) ? getLineLayout(line).getNumberOfSublines() : 1;}

/// Returns the number of the visual lines.
inline length_t TextRenderer::getNumberOfVisualLines() const throw() {return numberOfVisualLines_;}

/// Returns the special character drawer.
inline ISpecialCharacterDrawer* TextRenderer::getSpecialCharacterDrawer() const throw() {
	return const_cast<TextRenderer*>(this)->specialCharacterDrawer_.get();}

/// Returns the width of the rendering area.
inline int TextRenderer::getWidth() const throw() {return std::max(longestLineWidth_, viewerWidth_);}

/**
 * Removes the visual lines listener.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @p listener is not registered
 */
inline void TextRenderer::removeVisualLinesListener(IVisualLinesListener& listener) {visualLinesListeners_.remove(listener);}

/**
 * Sets the special character drawer.
 * @param newDrawer the new drawer or @c null
 * @param set true to transfer the ownership to the callee
 */
inline void TextRenderer::setSpecialCharacterDrawer(ISpecialCharacterDrawer* newDrawer, bool delegateOwnership) throw() {specialCharacterDrawer_.reset(newDrawer, delegateOwnership);}

}} // namespace ascension::viewers

#endif /* !ASCENSION_LAYOUT_HPP */
