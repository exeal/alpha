/**
 * @file viewer.hpp
 * This header defines several visual presentation classes.
 * @author exeal
 * @date 2003-2006 (was EditView.h)
 * @date 2006-2007
 */

#ifndef ASCENSION_VIEWER_HPP
#define ASCENSION_VIEWER_HPP

#include "point.hpp"
#include "presentation.hpp"
#include "../../manah/com/text-data-object.hpp"
#include <set>
#include <algorithm>

#ifndef ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER
#include <dimm.h>
#endif /* !ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER */

#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
#include <Oleacc.h>
#include <MSAAtext.h>
#endif /* !ASCENSION_NO_ACTIVE_ACCESSIBILITY */

#ifndef ASCENSION_NO_TEXT_OBJECT_MODEL
#include <tom.h>
#endif /* !ASCENSION_NO_TEXT_OBJECT_MODEL */


namespace ascension {

	/**
	 * Returns default UI language.
	 * Wrapper for Win32 @c GetUserDefaultUILanguage API.
	 */
	LANGID getUserDefaultUILanguage() throw();

	namespace texteditor {class EditorCommand;}

	namespace viewers {

		class TextViewer;

		/**
		 * A virtual rectangle placed in the viewer.
		 * @note This feature is not fully available on bidirectional texts.
		 * @see Caret#getBoxForRectangleSelection
		 */
		class VirtualBox : private manah::Unassignable {
		public:
			VirtualBox(const TextViewer& view, const text::Region& region) throw();
			bool	getOverlappedSubline(length_t line, length_t subline, length_t& first, length_t& last) const throw();
			bool	isPointOver(const POINT& pt) const throw();
			void	update(const text::Region& region) throw();
		private:
			struct Point {
				length_t line;		// 論理行
				length_t subline;	// line 行内の折り返し行オフセット
				int x;				// テキスト左端からの位置
			} points_[2];
			const TextViewer& view_;
			const Point& getBottom() const throw() {return points_[(&getTop() == &points_[0]) ? 1 : 0];}
			int getLeft() const throw() {return std::min(points_[0].x, points_[1].x);}
			int getRight() const throw() {return std::max(points_[0].x, points_[1].x);}
			const Point& getTop() const throw() {
				return points_[(points_[0].line < points_[1].line
					|| (points_[0].line == points_[1].line && points_[0].subline <= points_[1].subline)) ? 0 : 1];}
		};

		/**
		 * Interface for objects which are interested in change of input status of a @c TextViewer.
		 * @see ICaretListener#overtypeModeChanged, TextViewer#addInputStatusListener, TextViewer#removeInputStatusListener
		 */
		class ITextViewerInputStatusListener {
		private:
			/// The text viewer's IME open status has been changed.
			virtual void textViewerIMEOpenStatusChanged() throw() = 0;
			/// The text viewer's input language has been changed (@c WM_INPUTLANGCHANGE).
			virtual void textViewerInputLanguageChanged() throw() = 0;
			friend class TextViewer;
		};

		/**
		 * Interface for objects which are interested in change of scroll positions of a @c TextViewer.
		 * @see TextViewer#addViewportListener, TextViewer#removeViewportListener
		 */
		class IViewportListener {
		private:
			/**
			 * The scroll positions of the viewer were changed.
			 * @param horizontal true if the vertical scroll position is changed
			 * @param vertical true if the vertical scroll position is changed
			 * @see TextViewer#getFirstVisibleLine
			 */
			virtual void viewportChanged(bool horizontal, bool vertical) = 0;
			friend class TextViewer;
		};

		/**
		 * @c CaretShapeUpdater updates the caret of the text viewer.
		 * @see TextViewer, ICaretShapeProvider
		 */
		class CaretShapeUpdater : private manah::Unassignable {
		public:
			TextViewer&	getTextViewer() throw();
			void		update() throw();
		private:
			CaretShapeUpdater(TextViewer& viewer) throw();
			TextViewer& viewer_;
			friend class TextViewer;
		};

		/**
		 * Interface for objects which define the shape of the text viewer's caret.
		 * @see TextViewer#setCaretShapeProvider, CaretShapeUpdater, DefaultCaretShaper, LocaleSensitiveCaretShaper
		 */
		class ICaretShapeProvider {
		public:
			/// Destructor.
			virtual ~ICaretShapeProvider() throw() {}
		private:
			/**
			 * Returns the bitmap or the solid size defines caret shape.
			 * @param[out] bitmap the bitmap defines caret shape. if @c null, @a solidSize is used and the shape is solid
			 * @param[out] solidSize the size of solid caret. if @a bitmap is not @c null, this parameter is ignored
			 * @param[out] orientation the orientation of the caret. this value is used for hot spot calculation
			 */
			virtual void getCaretShape(
				std::auto_ptr<manah::win32::gdi::Bitmap>& bitmap, ::SIZE& solidSize, Orientation& orientation) throw() = 0;
			/**
			 * Installs the provider.
			 * @param updater the caret updator which notifies the text viewer to update the caret
			 */
			virtual void install(CaretShapeUpdater& updater) throw() = 0;
			/// Uninstalls the provider.
			virtual void uninstall() throw() = 0;
			friend class TextViewer;
		};

		/**
		 * Default implementation of @c ICaretShapeProvider.
		 * @note This class is not derivable.
		 */
		class DefaultCaretShaper : virtual public ICaretShapeProvider {
		public:
			DefaultCaretShaper() throw();
		private:
			void	getCaretShape(std::auto_ptr<manah::win32::gdi::Bitmap>& bitmap, ::SIZE& solidSize, Orientation& orientation) throw();
			void	install(CaretShapeUpdater& updater) throw();
			void	uninstall() throw();
		private:
			const TextViewer* viewer_;
		};

		/**
		 * @c LocaleSensitiveCaretShaper defines caret shape based on active keyboard layout.
		 * @note This class is not derivable.
		 */
		class LocaleSensitiveCaretShaper : virtual public ICaretShapeProvider,
				virtual public ICaretListener, virtual public ITextViewerInputStatusListener {
		public:
			explicit LocaleSensitiveCaretShaper(bool bold = false) throw();
		private:
			void	getCaretShape(std::auto_ptr<manah::win32::gdi::Bitmap>& bitmap, ::SIZE& solidSize, Orientation& orientation) throw();
			void	install(CaretShapeUpdater& updater) throw();
			void	uninstall() throw();
			// ICaretListener
			void	caretMoved(const class Caret& self, const text::Region& oldRegion);
			void	matchBracketsChanged(const Caret& self, const std::pair<text::Position, text::Position>& oldPair, bool outsideOfView);
			void	overtypeModeChanged(const Caret& self);
			void	selectionShapeChanged(const Caret& self);
			// ITextViewerInputStatusListener
			void	textViewerIMEOpenStatusChanged() throw();
			void	textViewerInputLanguageChanged() throw();
		private:
			CaretShapeUpdater* updater_;
			bool bold_;
		};

		/**
		 * Interface of objects which define how the text editors react to the users' mouse input.
		 * @note An instance of @c IMouseInputStrategy can't be shared multiple text viewers.
		 * @see TextViewer#setMouseInputStrategy
		 */
		class IMouseInputStrategy {
		public:
			/// Destructor.
			virtual ~IMouseInputStrategy() throw() {}
		protected:
			/// Buttons of the mouse.
			enum Button {
				LEFT_BUTTON,	///< The left button of the mouse.
				MIDDLE_BUTTON,	///< The middle button of the mouse.
				RIGHT_BUTTON,	///< The right button of the mouse.
				X1_BUTTON,		///< The first X button of the mouse.
				X2_BUTTON		///< The second X button of the mouse.
			};
			/// Actions of the mouse input.
			enum Action {
				PRESSED,		///< The button was pressed (down).
				RELEASED,		///< The button was released (up).
				DOUBLE_CLICKED	///< The button was double-clicked.
			};
		private:
			/// The viewer lost the mouse capture.
			virtual void captureChanged() = 0;
			/**
			 * Installs the strategy.
			 * @see viewer the text viewer uses the strategy. the window had been created at this time
			 */
			virtual void install(TextViewer& viewer) = 0;
			/**
			 * The mouse input was occured and the viewer had focus.
			 * @param button the button of the mouse input
			 * @param action the action of the input
			 * @param position the mouse position (client coordinates)
			 * @param keyState indicates whether various key are pressed. this value is same as Win32 WM_*BUTTON*
			 * @return true if the strategy processed
			 */
			virtual bool mouseButtonInput(Button button, Action action, const ::POINT& position, uint keyState) = 0;
			/**
			 * The mouse was moved and the viewer had focus.
			 * @param position the mouse position (client coordinates)
			 * @param keyState indicates whether various key are pressed. this value is same as Win32 @c WM_MOSEMOVE
			 * @return true if the strategy processed
			 */
			virtual void mouseMoved(const ::POINT& position, uint keyState) = 0;
			/**
			 * The mouse wheel was rolated and the viewer had focus.
			 * @param delta the distance the wheel was rotated
			 * @param position the mouse position (client coordinates)
			 * @param keyState indicates whether various key are pressed. this value is same as Win32 WM_*BUTTON*
			 */
			virtual void mouseWheelRotated(short delta, const ::POINT& position, uint keyState) = 0;
			/**
			 * Shows a cursor on the viewer.
			 * @param position the cursor position (client coordinates)
			 * @retval true if the callee showed a cursor
			 * @retval false if the callee did not know the appropriate cursor
			 */
			virtual bool showCursor(const ::POINT& position) = 0;
			/// Uninstalls the strategy. The window is not destroyed yet at this time.
			virtual void uninstall() = 0;
			friend class TextViewer;
		};

		/**
		 * Default implementation of @c IMouseOperationStrategy interface.
		 */
		class DefaultMouseInputStrategy : virtual public IMouseInputStrategy,
			virtual public ::IDropSource, virtual public ::IDropTarget, private manah::Unassignable {
		public:
			explicit DefaultMouseInputStrategy(bool enableOLEDragAndDrop);
			// IUnknown
			IMPLEMENT_UNKNOWN_NO_REF_COUNT()
			BEGIN_INTERFACE_TABLE()
				IMPLEMENTS_LEFTMOST_INTERFACE(IDropSource)
				IMPLEMENTS_INTERFACE(IDropTarget)
			END_INTERFACE_TABLE()
			// IDropSource
			STDMETHODIMP	QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState);
			STDMETHODIMP	GiveFeedback(DWORD dwEffect);
			// IDropTarget
			STDMETHODIMP	DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
			STDMETHODIMP	DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
			STDMETHODIMP	DragLeave();
			STDMETHODIMP	Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
		private:
			void					beginTimer(UINT interval);
			void					endTimer();
			void					extendSelection();
			void					handleLeftButtonPressed(const ::POINT& position, uint keyState);
			void					handleLeftButtonReleased(const ::POINT& position, uint keyState);
			static void CALLBACK	timeElapsed(HWND window, UINT message, UINT_PTR eventID, DWORD time);
			// IMouseInputStrategy
			void					captureChanged();
			void					install(TextViewer& viewer);
			virtual bool			mouseButtonInput(Button button, Action action, const ::POINT& position, uint keyState);
			virtual void			mouseMoved(const ::POINT& position, uint keyState);
			virtual void			mouseWheelRotated(short delta, const ::POINT& position, uint keyState);
			virtual bool			showCursor(const ::POINT& position);
			void					uninstall();
		private:
			TextViewer* viewer_;
			const bool oleDragAndDropEnabled_;
			bool leftButtonPressed_, oleDragging_;
			::POINT lastLeftButtonPressedPoint_;
			length_t numberOfDraggedRectangleLines_;
			static std::map<UINT_PTR, DefaultMouseInputStrategy*> timerTable_;
			static const UINT SELECTION_EXPANSION_INTERVAL, OLE_DRAGGING_TRACK_INTERVAL;
		};

		/**
		 * @see TextViewer#setLinkTextStrategy
		 */
		class IViewerLinkTextStrategy {
		public:
			/// Destructor.
			virtual ~IViewerLinkTextStrategy() throw() {}
		private:
			/**
			 * Returns the description of the link and provides the hover cursor.
			 * @param[in] region the region of the link
			 * @param[in] text the label text of the link
			 * @param[out] description the text of the popup message
			 * @param[out] hoverCursor the hover cursor. the callee should manage life cycle of the cursor
			 * @return true to provide the link information. if false, nothing will be happened
			 */
			virtual bool getLinkInformation(const text::Region& region, const String& text, String& description, HCURSOR& hoverCursor) = 0;
			/**
			 * Invokes the link.
			 * @param region the region of the link
			 * @param text the label text of the link
			 */
			virtual void invokeLink(const text::Region& region, const String& text) = 0;
			friend class TextViewer;
			friend class ASCENSION_SHARED_POINTER<IViewerLinkTextStrategy>;
		};

#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
		namespace internal {class TextViewerAccessibleProxy;}
#endif /* !ASCENSION_NO_ACTIVE_ACCESSIBILITY */

		class TextViewer :
				public manah::win32::ui::CustomControl<TextViewer>,
				virtual public text::IDocumentListener,
				virtual public text::IDocumentStateListener,
				virtual public text::ISequentialEditListener,
				virtual public IVisualLinesListener,
				virtual public ICaretListener,
				virtual public ascension::text::internal::IPointCollection<VisualPoint> {
			typedef manah::win32::ui::CustomControl<TextViewer> BaseControl;
			DEFINE_WINDOW_CLASS() {
				name = L"ascension:text-viewer";
				style = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW | CS_DBLCLKS;
				bgColor = COLOR_WINDOW;
				cursor = IDC_IBEAM;
			}

		public:
			/// Result of hit test.
			enum HitTestResult {
				INDICATOR_MARGIN,	///< The point is on the indicator margin.
				LINE_NUMBERS,		///< The point is on the line numbers area.
				LEADING_MARGIN,		///< The point is on the leading margin.
				TOP_MARGIN,			///< The point is on the top margin.
				TEXT_AREA,			///< The point is on the text area.
				OUT_OF_VIEW			///< The point is outside of the client area.
			};

			/**
			 * A general configuration of the viewer.
			 * @see TextViewer#getConfigurations, TextViewer#setConfigurations
			 */
			struct Configuration {
				/// Color of normal text. Standard setting is {@c COLOR_WINDOWTEXT, @c COLOR_WINDOW}.
				Colors color;
				/// Color of active selected text. Standard setting is {@c COLOR_HIGHLIGHTTEXT, @c COLOR_HIGHLIGHT}.
				Colors selectionColor;
				/// Color of inactive selected text. Standard setting is {@c COLOR_INACTIVECAPTIONTEXT, @c COLOR_INACTIVECAPTION}.
				Colors inactiveSelectionColor;
				/// Color of the inaccessible area. Standard setting is {@c COLOR_GRAYTEXT, @c color.background}.
				Colors restrictionColor;
				/// Color of invisible controls. Standard setting is not provided.
				Colors invisibleControlColor;
				/// Character count of a tab expansion. Default value is 8.
				int tabWidth;
				/// Line spacing in pixel. default value is 1.
				int lineSpacing;
				/// The amount of the leading margin in pixels. Default value is 5. This member will be ignored if the text is center-aligned.
				int leadingMargin;
				/// The amount of the top margin in pixels. Default value is 1.
				int topMargin;
				/// Orientation ("paragraph direction") of the lines. Default value is @c ASCENSION_DEFAULT_TEXT_ORIENTATION.
				Orientation orientation;
				/// Alignment of the lines. Default value is @c ASCENSION_DEFAULT_TEXT_ALIGNMENT.
				Alignment alignment;
				/// Line wrap configuration.
				LineWrapConfiguration lineWrap;
				/// Set true to vanish the cursor when the user types. Default value depends on system setting.
				bool vanishesCursor;
				/// Set true to justify the lines if wrapped. Default value is false.
				bool justifiesLines;
				/// Set true to inhibit any shaping. Default value is false.
				bool inhibitsShaping;
				/// If set to true, zero width control characters are shaped as representative glyphs. Default is false.
				bool displaysShapingControls;
				/// Set true to inhibit from generating mirrored glyphs. Default value is false.
				bool inhibitsSymmetricSwapping;
				/// Set true to make the deprecated format characters (NADS, NODS, ASS, and ISS) not effective. Default value is false.
				bool disablesDeprecatedFormatCharacters;
				/// Digits substitution type. Default value is @c DST_USER_DEFAULT.
				DigitSubstitutionType digitSubstitutionType;
				/// Constructor.
				Configuration() throw() : tabWidth(8), lineSpacing(0), leadingMargin(5), topMargin(1),
						orientation(ASCENSION_DEFAULT_TEXT_ORIENTATION), alignment(ASCENSION_DEFAULT_TEXT_ALIGNMENT),
						justifiesLines(false), inhibitsShaping(false), displaysShapingControls(false), inhibitsSymmetricSwapping(false),
						digitSubstitutionType(DST_USER_DEFAULT), disablesDeprecatedFormatCharacters(false) {
					BOOL b;
					::SystemParametersInfo(SPI_GETMOUSEVANISH, 0, &b, 0);
					vanishesCursor = toBoolean(b);
				}
				/// Returns true if the all mwmbers are valid.
				bool verify() const throw() {return lineWrap.verify() && tabWidth > 0 && lineSpacing >= 0;}
			};

			/**
			 * A vertical ruler's configuration.
			 * @see TextViewer#getVerticalRulerConfigurations, TextViewer#setVerticalRulerConfigurations
			 */
			struct VerticalRulerConfiguration {
				/// Configuration about a line numbers area.
				struct LineNumbers {
					/// Whether the area is visible or not. Default value is false and the line numbers is invisible.
					bool visible;
					/// Alignment of the digits. Default value is @c presentation#ALIGN_AUTO.
					/// If @c presentation#ALIGN_AUTO is set, the digits are aligned to the leading edge.
					Alignment alignment;
					/// Start value of the line number. Default value is 1.
					length_t startValue;
					/// Minimum number of digits. Default value is 4.
					uchar minimumDigits;
					/// Leading margin in pixels. Default value is 6.
					int leadingMargin;
					/// Trailing margin in pixels. Default value is 1.
					int trailingMargin;
					/// Color of the text. Default value is @c presentation#Colors#STANDARD.
					/// @c presentation#Colors#STANDARD is fallbacked to the color of the system normal text.
					Colors textColor;
					/// Color of the border. Default value is @c presentation#STANDARD_COLOR.
					/// @c presentation#STANDARD_COLOR is fallbacked to the color of the system normal text.
					COLORREF borderColor;
					/// Width of the border. Default value is 1.
					uchar borderWidth;
					/// Style of the border. Default value is @c SOLID.
					enum {
						NONE,			///< No-line.
						SOLID,			///< Solid line.
						DASHED,			///< Dashed line.
						DASHED_ROUNDED,	///< Dashed and rounded line.
						DOTTED			///< Dotted line.
					} borderStyle;
					/// Digit substitution type. @c DST_CONTEXTUAL can't set. Default value is @c DST_USER_DEFAULT.
					DigitSubstitutionType digitSubstitution;
					/// Constructor.
					LineNumbers() throw() : visible(false), alignment(ALIGN_AUTO), startValue(1),
						minimumDigits(4), leadingMargin(6), trailingMargin(1), borderColor(STANDARD_COLOR),
						borderWidth(1), borderStyle(SOLID), digitSubstitution(DST_USER_DEFAULT) {}
					/// Returns true if the all members are valid.
					bool verify() const throw() {return leadingMargin >= 0 && trailingMargin >= 0;}
				} lineNumbers;	/// Configuration about the line numbers area.
				/// Configuration about an indicator margin.
				struct IndicatorMargin {
					/// Wether the indicator margin is visible or not. Default value is false and the indicator margin is invisible.
					bool visible;
					/// Width of the indicator margin. Default value is 15.
					ushort width;
					/// Background color. Default value is @c presentation#STANDARD_COLOR.
					/// @c presentation#STANDARD_COLOR is fallbacked to the system color @c COLOR_3DFACE.
					COLORREF color;
					/// Color of the border. Default value is @c presentation#STANDARD_COLOR.
					/// @c presentation#STANDARD_COLOR is fallbacked to the system color @c COLOR_3DSHADOW.
					COLORREF borderColor;
					/// Constructor.
					IndicatorMargin() throw() : visible(false), width(15), color(STANDARD_COLOR), borderColor(STANDARD_COLOR) {}
					/// Returns true if the all members are valid.
					bool verify() const throw() {return width >= 0;}
				} indicatorMargin;	/// Configuration about the indicator margin.
				/// Alignment of the vertical ruler. Can be either @c presentation#ALIGN_LEFT or
				/// @c presentation#ALIGN_RIGHT. Default value is determined based on @c ASCENSION_DEFAULT_TEXT_ORIENTATION.
				Alignment alignment;
				/// Constructor.
				VerticalRulerConfiguration() throw() :
					alignment(ASCENSION_DEFAULT_TEXT_ORIENTATION == LEFT_TO_RIGHT ? ALIGN_LEFT : ALIGN_RIGHT) {}
				/// Returns true if the all members are valid.
				bool verify() const throw() {
					return lineNumbers.verify() && indicatorMargin.verify() && (alignment == ALIGN_LEFT || alignment == ALIGN_RIGHT);}
			};

			// constructors
			explicit TextViewer(presentation::Presentation& presentation);
			TextViewer(const TextViewer& rhs);
			virtual ~TextViewer();
			// window creation
			virtual bool	create(HWND parent, const ::RECT& rect, DWORD style, DWORD exStyle);
			// listeners and strategies
			void	addDisplaySizeListener(IDisplaySizeListener& listener);
			void	addInputStatusListener(ITextViewerInputStatusListener& listener);
			void	addViewportListener(IViewportListener& listener);
			void	removeDisplaySizeListener(IDisplaySizeListener& listener);
			void	removeInputStatusListener(ITextViewerInputStatusListener& listener);
			void	removeViewportListener(IViewportListener& listener);
			void	setCaretShapeProvider(ASCENSION_SHARED_POINTER<ICaretShapeProvider> shaper) throw();
			void	setLinkTextStrategy(ASCENSION_SHARED_POINTER<IViewerLinkTextStrategy> newStrategy) throw();
			void	setMouseInputStrategy(ASCENSION_SHARED_POINTER<IMouseInputStrategy> newStrategy);
			// attributes
			const Configuration&				getConfiguration() const throw();
			text::Document&						getDocument();
			const text::Document&				getDocument() const;
			presentation::Presentation&			getPresentation() throw();
			const presentation::Presentation&	getPresentation() const throw();
			ulong								getScrollRate(bool horizontal) const throw();
			TextRenderer&						getTextRenderer() throw();
			const TextRenderer&					getTextRenderer() const throw();
			const VerticalRulerConfiguration&	getVerticalRulerConfiguration() const throw();
			void								setConfiguration(const Configuration* general,
													const VerticalRulerConfiguration* verticalRuler);
			// auto scroll
			void	beginAutoScroll();
			bool	endAutoScroll();
			bool	isAutoScrolling() const throw();
			// caret
			Caret&			getCaret() throw();
			const Caret&	getCaret() const throw();
#ifndef ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER
			// Global IME
			void	enableActiveInputMethod(bool enable = true) throw();
			bool	isActiveInputMethodEnabled() const throw();
#endif /* !ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER */
			// UI
			void	beep() throw();
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
			HRESULT	getAccessibleObject(IAccessible*& acc) const throw();
#endif /* !ASCENSION_NO_ACTIVE_ACCESSIBILITY */
			void	hideToolTip();
			void	scroll(int dx, int dy, bool redraw);
			void	scrollTo(int x, int y, bool redraw);
			void	scrollTo(length_t line, bool redraw);
			void	showToolTip(const String& text, ulong timeToWait = -1, ulong timeRemainsVisible = -1);
#ifndef ASCENSION_NO_TEXT_SERVICES_FRAMEWORK
			HRESULT	startTextServices();
#endif /* !ASCENSION_NO_TEXT_SERVICES_FRAMEWORK */
			// redraw
			void	redrawLine(length_t line, bool following = false);
			void	redrawLines(length_t first, length_t last);
			// freeze
			void	freeze(bool forAllClones = true);
			bool	isFrozen() const throw();
			void	unfreeze(bool forAllClones = true);
			// mouse input
			bool	allowsMouseInput() const throw();
			void	enableMouseInput(bool enable);
			// client coordinates vs. character position mappings
			text::Position	getCharacterForClientXY(const ::POINT& pt, bool nearestLeading) const throw();
			::POINT			getClientXYForCharacter(const text::Position& position,
								bool fullSearchY, LineLayout::Edge edge = LineLayout::LEADING) const;
			// utilities
			void			getFirstVisibleLine(length_t* logicalLine, length_t* visualLine, length_t* visualSubline) const throw();
			length_t		getNumberOfVisibleLines() const throw();
			length_t		getNumberOfVisibleColumns() const throw();
			bool			getPointedLinkText(text::Region& region, manah::AutoBuffer<Char>& text) const;
			::RECT			getTextAreaMargins() const throw();
			HitTestResult	hitTest(const ::POINT& pt) const;

		protected:
			virtual void	doBeep() throw();
			virtual void	drawIndicatorMargin(length_t line, manah::win32::gdi::DC& dc, const ::RECT& rect);
			bool			handleKeyDown(UINT key, bool controlPressed, bool shiftPressed, bool altPressed) throw();

			// helpers
		private:
			int		getDisplayXOffset() const throw();
			void	handleGUICharacterInput(CodePoint c);
			void	internalUnfreeze();
			void	mapClientYToLine(int y, length_t* logicalLine, length_t* visualSublineOffset) const throw();
			int		mapLineToClientY(length_t line, bool fullSearch) const;
			void	recreateCaret();
			void	redrawVerticalRuler();
			void	updateCaretPosition();
			void	updateIMECompositionWindowPosition();
			void	updateScrollBars();

			// protected interfaces
		protected:
			// IDocumentStateListener (overridable)
			virtual void	documentAccessibleRegionChanged(text::Document& document);
			virtual void	documentEncodingChanged(text::Document& document);
			virtual void	documentFileNameChanged(text::Document& document);
			virtual void	documentModificationSignChanged(text::Document& document);
			virtual void	documentReadOnlySignChanged(text::Document& document);
			// ICaretListener (overridable)
			virtual void	caretMoved(const Caret& self, const text::Region& oldRegion);
			virtual void	matchBracketsChanged(const Caret& self,
								const std::pair<text::Position, text::Position>& oldPair, bool outsideOfView);
			virtual void	overtypeModeChanged(const Caret& self);
			virtual void	selectionShapeChanged(const Caret& self);
		private:
			// IDocumentListener
			void	documentAboutToBeChanged(const text::Document& document);
			void	documentChanged(const text::Document& document, const text::DocumentChange& change);
			// ISequentialEditListener
			void	documentSequentialEditStarted(text::Document& document);
			void	documentSequentialEditStopped(text::Document& document);
			void	documentUndoSequenceStarted(text::Document& document);
			void	documentUndoSequenceStopped(text::Document& document, const text::Position& resultPosition);
			// IVisualLinesListener
			void	rendererFontChanged() throw();
			void	visualLinesDeleted(length_t first, length_t last, length_t sublines, bool longestLineChanged) throw();
			void	visualLinesInserted(length_t first, length_t last) throw();
			void	visualLinesModified(length_t first, length_t last,
						signed_length_t sublinesDifference, bool documentChanged, bool longestLineChanged) throw();
			// internal::IPointCollection<VisualPoint>
			void	addNewPoint(VisualPoint& point) {points_.insert(&point);}
			void	removePoint(VisualPoint& point) {points_.erase(&point);}

			// message handlers
			MANAH_DECLEAR_WINDOW_MESSAGE_MAP(TextViewer);
		protected:
			virtual LRESULT	preTranslateWindowMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled);
			void	onCaptureChanged(HWND newWindow);
			void	onChar(UINT ch, UINT flags);
			bool	onCommand(WORD id, WORD notifyCode, HWND control);
			bool	onContextMenu(HWND window, const ::POINT& pt);
			void	onDestroy();
			bool	onEraseBkgnd(HDC dc);
			HFONT	onGetFont();
			void	onHScroll(UINT sbCode, UINT pos, HWND scrollBar);
			void	onIMEComposition(WPARAM wParam, LPARAM lParam);
			void	onIMEEndComposition();
			LRESULT	onIMENotify(WPARAM command, LPARAM lParam, bool& handled);
			LRESULT	onIMERequest(WPARAM command, LPARAM lParam, bool& handled);
			void	onIMEStartComposition();
			void	onKeyDown(UINT ch, UINT flags, bool& handled);
			void	onKillFocus(HWND newWindow);
			void	onLButtonDblClk(UINT keyState, const ::POINT& pt);
			void	onLButtonDown(UINT keyState, const ::POINT& pt);
			void	onLButtonUp(UINT keyState, const ::POINT& pt);
			void	onMButtonDblClk(UINT keyState, const ::POINT& pt);
			void	onMButtonDown(UINT keyState, const ::POINT& pt);
			void	onMButtonUp(UINT keyState, const ::POINT& pt);
			void	onMouseMove(UINT keyState, const ::POINT& pt);
#ifdef WM_MOUSEWHEEL
			void	onMouseWheel(UINT flags, short zDelta, const ::POINT& pt);
#endif /* WM_MOUSEWHEEL */
			bool	onNotify(int id, ::NMHDR& nmhdr);
			void	onPaint(manah::win32::gdi::PaintDC& dc);
			void	onRButtonDblClk(UINT keyState, const ::POINT& pt);
			void	onRButtonDown(UINT keyState, const ::POINT& pt);
			void	onRButtonUp(UINT keyState, const ::POINT& pt);
			bool	onSetCursor(HWND window, UINT hitTest, UINT message);
			void	onSetFocus(HWND oldWindow);
			void	onSize(UINT type, int cx, int cy);
			void	onStyleChanged(int type, const ::STYLESTRUCT& style);
			void	onStyleChanging(int type, ::STYLESTRUCT& style);
			void	onSysChar(UINT ch, UINT flags);
			void	onSysColorChange();
			bool	onSysKeyDown(UINT ch, UINT flags);
			bool	onSysKeyUp(UINT ch, UINT flags);
#ifdef WM_THEMECHANGED
			void	onThemeChanged();
#endif /* WM_THEMECHANGED */
			void	onTimer(UINT_PTR eventId, ::TIMERPROC timerProc);
#ifdef WM_UNICHAR
			void	onUniChar(UINT ch, UINT flags);
#endif /* WM_UNICHAR */
			void	onVScroll(UINT sbCode, UINT pos, HWND scrollBar);
#ifdef WM_XBUTTONDBLCLK
			bool	onXButtonDblClk(WORD xButton, WORD keyState, const ::POINT& pt);
			bool	onXButtonDown(WORD xButton, WORD keyState, const ::POINT& pt);
			bool	onXButtonUp(WORD xButton, WORD keyState, const ::POINT& pt);
#endif /* WM_XBUTTONDBLCLK */

			// 内部クラス
		private:
			/// Circled window displayed at which the auto scroll started.
			class AutoScrollOriginMark : public manah::win32::ui::CustomControl<AutoScrollOriginMark>, private manah::Noncopyable {
				DEFINE_WINDOW_CLASS() {
					name = L"AutoScrollOriginMark";
					style = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW;
					bgColor = COLOR_WINDOW;
					cursor = IDC_IBEAM;
				}
			public:
				bool	create(const TextViewer& view);
			protected:
				void	onPaint(manah::win32::gdi::PaintDC& dc);
			private:
				static const long WINDOW_WIDTH;
			};
			/// @c VerticalRulerDrawer draws the vertical ruler of the @c TextViewer.
			class VerticalRulerDrawer : public manah::Noncopyable {
			public:
				explicit VerticalRulerDrawer(TextViewer& viewer) throw();
				void								draw(manah::win32::gdi::PaintDC& dc);
				const VerticalRulerConfiguration&	getConfiguration() const throw();
				int									getWidth() const throw();
				void								setConfiguration(const VerticalRulerConfiguration& configuration);
				void								update() throw();
			private:
				uchar	getLineNumberMaxDigits() const throw();
				void	recalculateWidth() throw();
				void	updateGDIObjects() throw();
				TextViewer& viewer_;
				VerticalRulerConfiguration configuration_;
				int width_;
				uchar lineNumberDigitsCache_;
				manah::win32::gdi::Pen indicatorMarginPen_, lineNumbersPen_;
				manah::win32::gdi::Brush indicatorMarginBrush_, lineNumbersBrush_;
#ifndef ASCENSION_NO_DOUBLE_BUFFERING
				std::auto_ptr<manah::win32::gdi::DC> memoryDC_;
				manah::win32::gdi::Bitmap memoryBitmap_;
#endif /* !ASCENSION_NO_DOUBLE_BUFFERING */
			};

			// 列挙
		private:
			// timer identifiers
			enum {
				TIMERID_CALLTIP,	// interval for tooltip
				TIMERID_AUTOSCROLL,	// the viewer is auto scrolling
//				TIMERID_LINEPARSE
			};

			// identifiers of GUI commands
			enum {
				WM_REDO	= WM_APP + 1,		// Undo
				WM_SELECTALL,				// Select All
				ID_DISPLAYSHAPINGCONTROLS,	// Show Unicode control characters
				ID_RTLREADING,				// Right to left Reading order
				ID_TOGGLEIMESTATUS,			// Open/Close IME
				ID_TOGGLESOFTKEYBOARD,		// Open/Close soft keyboard
				ID_RECONVERT,				// Reconvert

				ID_INSERT_LRM,		// LRM (Left-to-right mark)
				ID_INSERT_RLM,		// RLM (Right-to-left mark)
				ID_INSERT_ZWJ,		// ZWJ (Zero width joiner)
				ID_INSERT_ZWNJ,		// ZWNJ (Zero width non-joiner)
				ID_INSERT_LRE,		// LRE (Left-to-right embedding)
				ID_INSERT_RLE,		// RLE (Right-to-left embedding)
				ID_INSERT_LRO,		// LRO (Left-to-right override)
				ID_INSERT_RLO,		// RLO (Right-to-left override)
				ID_INSERT_PDF,		// PDF (Pop directional formatting)
				ID_INSERT_WJ,		// WJ (Word Joiner)
				ID_INSERT_NADS,		// NADS (National digit shapes)	<- the following six are deprecated code points (Unicode 4.0)
				ID_INSERT_NODS,		// NODS (Nominal digit shapes)
				ID_INSERT_ASS,		// ASS (Activate symmetric swapping)
				ID_INSERT_ISS,		// ISS (Inhibit symmetric swapping)
				ID_INSERT_AAFS,		// AAFS (Activate Arabic form shaping)
				ID_INSERT_IAFS,		// IAFS (Inhibit Arabic form shaping)
				ID_INSERT_RS,		// RS (Record Separator)
				ID_INSERT_US,		// US (Unit Separator)
				ID_INSERT_IAA,		// Interlinear Annotation Anchor
				ID_INSERT_IAS,		// Interlinear Annotation Separator
				ID_INSERT_IAT,		// Interlinear Annotation Terminator

				ID_INSERT_U0020,	// U+0020 (Space)
				ID_INSERT_NBSP,		// NBSP (No-Break Space)
				ID_INSERT_U1680,	// U+1680 (Ogham Space Mark)
				ID_INSERT_MVS,		// MVS (Mongolian Vowel Separator)
				ID_INSERT_U2000,	// U+2000 (En Quad)
				ID_INSERT_U2001,	// U+2001 (Em Quad)
				ID_INSERT_U2002,	// U+2002 (En Space)
				ID_INSERT_U2003,	// U+2003 (Em Space)
				ID_INSERT_U2004,	// U+2004 (Three-Per-Em Space)
				ID_INSERT_U2005,	// U+2005 (Four-Per-Em Space)
				ID_INSERT_U2006,	// U+2006 (Six-Per-Em Space)
				ID_INSERT_U2007,	// U+2007 (Figure Space)
				ID_INSERT_U2008,	// U+2008 (Punctuation Space)
				ID_INSERT_U2009,	// U+2009 (Thin Space)
				ID_INSERT_U200A,	// U+200A (Hair Space)
				ID_INSERT_ZWSP,		// ZWSP (Zero Width Space)
				ID_INSERT_NNBSP,	// NNSBP (Narrwow No-Break Space)
				ID_INSERT_MMSP,		// MMSP (Medium Mathematical Space)
				ID_INSERT_U3000,	// U+3000 (Ideographic Space)
				ID_INSERT_NEL,		// NEL (Next Line)
				ID_INSERT_LS,		// LS (Line Separator)
				ID_INSERT_PS,		// PS (Paragraph Separator)
			};


			// データメンバ
		private:
			// 大物
			presentation::Presentation& presentation_;
			std::auto_ptr<Caret> caret_;
			std::auto_ptr<TextRenderer> renderer_;
			Configuration configuration_;
			std::set<VisualPoint*> points_;
			HWND toolTip_;	// ツールチップ
			Char* tipText_;	// ツールチップのテキスト
			ASCENSION_SHARED_POINTER<IMouseInputStrategy> mouseInputStrategy_;
			ASCENSION_SHARED_POINTER<IViewerLinkTextStrategy> linkTextStrategy_;
			ascension::internal::Listeners<IDisplaySizeListener> displaySizeListeners_;
			ascension::internal::Listeners<ITextViewerInputStatusListener> inputStatusListeners_;
			ascension::internal::Listeners<IViewportListener> viewportListeners_;
			std::auto_ptr<AutoScrollOriginMark> autoScrollOriginMark_;
			std::auto_ptr<VerticalRulerDrawer> verticalRulerDrawer_;
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
			internal::TextViewerAccessibleProxy* accessibleProxy_;
#endif /* !ASCENSION_NO_ACTIVE_ACCESSIBILITY */

			// モード
			struct ModeState {
				bool cursorVanished;		// ユーザが文字の入力を開始したのでカーソルが非表示
#ifndef ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER
				bool activeInputMethodEnabled;	// Global IME を使うか
#endif /* !ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER */

				ModeState() throw() : cursorVanished(false)
#ifndef ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER
					, activeInputMethodEnabled(true)
#endif /* !ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER */
				{}
			} modeState_;

			// スクロール情報
			struct ScrollInfo {
				struct {
					int position;	// SCROLLINFO.nPos
					int maximum;	// SCROLLINFO.nMax
					UINT pageSize;	// SCROLLINFO.nPage
//					ulong rate;		// 最小スクロール量が何文字 (何行) に相当するか (普通は 1)
				} horizontal, vertical;
				length_t firstVisibleLine, firstVisibleSubline;
				bool changed;
				ScrollInfo() throw() : firstVisibleLine(0), firstVisibleSubline(0), changed(false) {
					horizontal.position = vertical.position = 0;
//					horizontal.rate = vertical.rate = 1;
				}
				ulong getX() const throw() {return horizontal.position/* * horizontal.rate*/;}
				ulong getY() const throw() {return vertical.position/* * vertical.rate*/;}
				void resetBars(const TextViewer& viewer, int bars, bool pageSizeChanged) throw();
				void updateVertical(const TextViewer& viewer) throw();
			} scrollInfo_;

			// 凍結
			struct FreezeInfo {
				ulong count;								// zero for not frozen
				std::pair<length_t, length_t> invalidLines;	// 凍結中に再描画を要求された行。要求が無ければ first == second
				FreezeInfo() throw() : count(0) {invalidLines.first = invalidLines.second = INVALID_INDEX;}
			} freezeInfo_;

			// キャレットのビットマップ
			struct CaretShape {
				ASCENSION_SHARED_POINTER<ICaretShapeProvider> shaper;
				Orientation orientation;
				int width;
				std::auto_ptr<manah::win32::gdi::Bitmap> bitmap;
				CaretShape() throw() : orientation(LEFT_TO_RIGHT), width(0) {}
			} caretShape_;

			// 状態
			bool imeCompositionActivated_;	// true if the user is inputing by using IME
			ulong mouseInputDisabledCount_;

			// 自動スクロール
			struct AutoScroll {
				::POINT	indicatorPosition;	// position of the indicator margin (in client coodinates)
				bool	scrolling;			// true if the viewer is scrolling
				AutoScroll() throw() : scrolling(false) {}
			} autoScroll_;

			friend class VisualPoint;
			friend class VirtualBox;
			friend class VerticalRulerDrawer;
			friend class CaretShapeUpdater;
		};

		/// Extension of @c TextViewer for code editor.
		class SourceViewer : public TextViewer {
		public:
			// constructors
			explicit SourceViewer(presentation::Presentation& presentation) throw();
			// attributes
			// operations
			// utility
			bool	getPointedIdentifier(text::Position* startPosition, text::Position* endPosition, String* identifier) const;
		protected:
			bool	getNearestIdentifier(const text::Position& position, length_t* startChar, length_t* endChar, String* identifier) const;
		private:
		};

		class CurrentLineHighlighter : public manah::Noncopyable,
			virtual public presentation::ILineColorDirector, virtual public ICaretListener {
		public:
			// constant
			static const ILineColorDirector::Priority LINE_COLOR_PRIORITY;
			// constructors
			CurrentLineHighlighter(Caret& caret, const Colors& color = Colors(STANDARD_COLOR, COLOR_INFOBK | SYSTEM_COLOR_MASK));
			~CurrentLineHighlighter() throw();
			// attributes
			const Colors&	getColor() const throw();
			void			setColor(const Colors& color) throw();
		private:
			// ILineColorDirector
			ILineColorDirector::Priority	queryLineColor(length_t line, Colors& color) const;
			// ICaretListener
			void	caretMoved(const Caret& self, const text::Region& oldRegion);
			void	matchBracketsChanged(const Caret& self, const std::pair<text::Position, text::Position>& oldPair, bool outsideOfView);
			void	overtypeModeChanged(const Caret& self);
			void	selectionShapeChanged(const Caret& self);
		private:
			Caret& caret_;
			Colors color_;
		};


// inlines //////////////////////////////////////////////////////////////////

/**
 * Registers the display size listener.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
inline void TextViewer::addDisplaySizeListener(IDisplaySizeListener& listener) {displaySizeListeners_.add(listener);}

/**
 * Registers the input status listener.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
inline void TextViewer::addInputStatusListener(ITextViewerInputStatusListener& listener) {inputStatusListeners_.add(listener);}

/**
 * Registers the viewport listener.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
inline void TextViewer::addViewportListener(IViewportListener& listener) {viewportListeners_.add(listener);}

/**
 * Returns true if the viewer allows the mouse operations.
 * @see #enableMouseInput
 */
inline bool TextViewer::allowsMouseInput() const throw() {return mouseInputDisabledCount_ == 0;} 

/// Informs the end user of <strong>safe</strong> error.
inline void TextViewer::beep() throw() {doBeep();}

#ifndef ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER
/**
 * Enables Global IME.
 *
 * This setting effects under only Windows NT 4.0.
 * Under other platforms, Ascension does not use Global IME.
 */
inline void TextViewer::enableActiveInputMethod(bool enable /* = true */) throw() {modeState_.activeInputMethodEnabled = enable;}
#endif /* !ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER */

/**
 * Enables/disables the mouse operations.
 *
 * A @c TextViewer has a disabled count for the mouse input. If this value is not zero, any mouse
 * inputs are not allowed.
 *
 * These is no way to disable the scroll bars.
 * @param enable set false to increment the disabled count, true to decrement
 * @see #allowMouseInput
 */
inline void TextViewer::enableMouseInput(bool enable) {
	if(mouseInputDisabledCount_ != 0 || !enable) mouseInputDisabledCount_ += !enable ? 1 : -1;}

/// Returns the caret.
inline Caret& TextViewer::getCaret() throw() {return *caret_;}

/// Returns the caret.
inline const Caret& TextViewer::getCaret() const throw() {return *caret_;}

/**
 * Returns the general configuration.
 * @see #getVerticalRulerConfiguration, #setConfiguration
 */
inline const TextViewer::Configuration& TextViewer::getConfiguration() const throw() {return configuration_;}

/// Returns the document.
inline text::Document& TextViewer::getDocument() {return presentation_.getDocument();}

/// Returns the document.
inline const text::Document& TextViewer::getDocument() const {return presentation_.getDocument();}

/**
 * Returns the information about the uppermost visible line in the viewer.
 * @param[out] logicalLine the logical index of the line. can be @c null if not needed
 * @param[out] visualLine the visual index of the line. can be @c null if not needed
 * @param[out] visualSubline the offset of @a visualLine from the first line in @a logicalLine. can be @c null if not needed
 */
inline void TextViewer::getFirstVisibleLine(length_t* logicalLine, length_t* visualLine, length_t* visualSubline) const throw() {
	if(logicalLine != 0)
		*logicalLine = scrollInfo_.firstVisibleLine;
	if(visualSubline != 0)
		*visualSubline = scrollInfo_.firstVisibleSubline;
	if(visualLine != 0)
		*visualLine = scrollInfo_.getY();
}

/**
 * Returns the number of the drawable columns in the window.
 * @return the number of columns
 */
inline length_t TextViewer::getNumberOfVisibleColumns() const throw() {
	::RECT r;
	getClientRect(r);
	return (r.left == r.right) ? 0 :
		(r.right - r.left - configuration_.leadingMargin - verticalRulerDrawer_->getWidth()) / renderer_->getAverageCharacterWidth();
}

/**
 * Returns the number of the drawable lines in the window.
 * @return the number of lines
 */
inline length_t TextViewer::getNumberOfVisibleLines() const throw() {
	::RECT r;
	getClientRect(r);
	return (r.top == r.bottom) ? 0 : (r.bottom - r.top - configuration_.topMargin) / renderer_->getLineHeight();
}

/// Returns the presentation object. 
inline presentation::Presentation& TextViewer::getPresentation() throw() {return presentation_;}

/// Returns the presentation object. 
inline const presentation::Presentation& TextViewer::getPresentation() const throw() {return presentation_;}

/**
 * スクロール量の対する行数、文字数の比率を返す
 * @param horizontal 水平スクロールバーについて調べる場合 true。垂直スクロールバーの場合 false
 * @return 比率
 */
inline ulong TextViewer::getScrollRate(bool horizontal) const throw() {
	assertValidAsWindow(); return 1/*horizontal ? scrollInfo_.horizontal.rate : scrollInfo_.vertical.rate*/;}

/// Returns the text renderer.
inline TextRenderer& TextViewer::getTextRenderer() throw() {return *renderer_;}

/// Returns the text renderer.
inline const TextRenderer& TextViewer::getTextRenderer() const throw() {return *renderer_;}

/**
 * Returns the vertical ruler's configuration.
 * @see #getConfiguration, #setConfiguration
 */
inline const TextViewer::VerticalRulerConfiguration& TextViewer::getVerticalRulerConfiguration() const throw() {return verticalRulerDrawer_->getConfiguration();}

#ifndef ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER
/// Returns true if Global IME is enabled.
inline bool TextViewer::isActiveInputMethodEnabled() const throw() {return modeState_.activeInputMethodEnabled;}
#endif /* !ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER */

/// Returns true if the viewer is auto-scrolling.
inline bool TextViewer::isAutoScrolling() const throw() {return autoScroll_.scrolling;}

/// Returns true if the viewer is frozen.
inline bool TextViewer::isFrozen() const throw() {return freezeInfo_.count != 0;}

/**
 * Removes the display size listener.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
inline void TextViewer::removeDisplaySizeListener(IDisplaySizeListener& listener) {displaySizeListeners_.remove(listener);}

/**
 * Removes the input status listener.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
inline void TextViewer::removeInputStatusListener(ITextViewerInputStatusListener& listener) {inputStatusListeners_.remove(listener);}

/**
 * Removes the viewport listener.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
inline void TextViewer::removeViewportListener(IViewportListener& listener) {viewportListeners_.remove(listener);}

/**
 * Sets the caret shape provider.
 * @param shaper the new caret shaper
 */
inline void TextViewer::setCaretShapeProvider(ASCENSION_SHARED_POINTER<ICaretShapeProvider> shaper) {caretShape_.shaper = shaper;}

/**
 * Sets the link text strategy.
 * @param newStrategy the new strategy or @c null
 */
inline void TextViewer::setLinkTextStrategy(ASCENSION_SHARED_POINTER<IViewerLinkTextStrategy> newStrategy) {linkTextStrategy_ = newStrategy;}

/// Returns the vertical ruler's configurations.
inline const TextViewer::VerticalRulerConfiguration& TextViewer::VerticalRulerDrawer::getConfiguration() const throw() {return configuration_;}

/// Returns the width of the vertical ruler.
inline int TextViewer::VerticalRulerDrawer::getWidth() const throw() {return width_;}

}} // namespace ascension::viewers

#endif /* ASCENSION_VIEWER_HPP */
