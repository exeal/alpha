/**
 * @file viewer.hpp
 * This header defines several visual presentation classes.
 * @author exeal
 * @date 2003-2006 (was EditView.h)
 * @date 2006-2009
 */

#ifndef ASCENSION_VIEWER_HPP
#define ASCENSION_VIEWER_HPP

#include "point.hpp"
#include "presentation.hpp"
#include "content-assist.hpp"
#include <manah/win32/ui/window.hpp>
#include <manah/com/unknown-impl.hpp>
#include <set>
#include <algorithm>
#include <shlobj.h>	// IDragSourceHelper, IDropTargetHelper

#ifndef ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER
#include <dimm.h>
#endif // !ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER

#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
#include <Oleacc.h>
#include <MSAAtext.h>
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY

#ifndef ASCENSION_NO_TEXT_OBJECT_MODEL
#include <tom.h>
#endif // !ASCENSION_NO_TEXT_OBJECT_MODEL


namespace ascension {

	/**
	 * Returns default UI language.
	 * Wrapper for Win32 @c GetUserDefaultUILanguage API.
	 */
	LANGID getUserDefaultUILanguage() /*throw()*/;

	namespace texteditor {class EditorCommand;}

	namespace viewers {class TextViewer;}

	/// Provides stuffs for source code editors.
	/// @todo Need refinements.
	namespace source {
		bool getPointedIdentifier(const viewers::TextViewer& viewer,
				kernel::Position* startPosition, kernel::Position* endPosition);
		bool getNearestIdentifier(const kernel::Document& document,
				const kernel::Position& position, length_t* startColumn, length_t* endColumn);
	}

	namespace viewers {

		/**
		 * A virtual rectangle placed in the viewer.
		 * @note This feature is not fully available on bidirectional texts.
		 * @see Caret#boxForRectangleSelection
		 */
		class VirtualBox {
			MANAH_UNASSIGNABLE_TAG(VirtualBox);
		public:
			VirtualBox(const TextViewer& view, const kernel::Region& region) /*throw()*/;
			bool isPointOver(const POINT& pt) const /*throw()*/;
			bool overlappedSubline(length_t line, length_t subline, length_t& first, length_t& last) const /*throw()*/;
			void update(const kernel::Region& region) /*throw()*/;
		private:
			struct Point {
				length_t line;		// logical line number
				length_t subline;	// line 行内の折り返し行オフセット
				int x;				// distance from left side of layout
			} points_[2];
			const TextViewer& view_;
			const Point& beginning() const /*throw()*/ {
				return points_[(points_[0].line < points_[1].line
					|| (points_[0].line == points_[1].line && points_[0].subline <= points_[1].subline)) ? 0 : 1];}
			const Point& end() const /*throw()*/ {return points_[(&beginning() == &points_[0]) ? 1 : 0];}
			int left() const /*throw()*/ {return std::min(points_[0].x, points_[1].x);}
			int right() const /*throw()*/ {return std::max(points_[0].x, points_[1].x);}
		};

		/**
		 * Interface for objects which are interested in change of input status of a @c TextViewer.
		 * @see ICaretListener#overtypeModeChanged, TextViewer#addInputStatusListener, TextViewer#removeInputStatusListener
		 */
		class ITextViewerInputStatusListener {
		private:
			/// The text viewer's IME open status has been changed.
			virtual void textViewerIMEOpenStatusChanged() /*throw()*/ = 0;
			/// The text viewer's input language has been changed (@c WM_INPUTLANGCHANGE).
			virtual void textViewerInputLanguageChanged() /*throw()*/ = 0;
			friend class TextViewer;
		};

		/**
		 * @c CaretShapeUpdater updates the caret of the text viewer.
		 * @see TextViewer, ICaretShapeProvider
		 */
		class CaretShapeUpdater {
			MANAH_UNASSIGNABLE_TAG(CaretShapeUpdater);
		public:
			TextViewer& textViewer() /*throw()*/;
			void update() /*throw()*/;
		private:
			CaretShapeUpdater(TextViewer& viewer) /*throw()*/;
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
			virtual ~ICaretShapeProvider() /*throw()*/ {}
		private:
			/**
			 * Returns the bitmap or the solid size defines caret shape.
			 * @param[out] bitmap the bitmap defines caret shape. if @c null, @a solidSize is used and the shape is solid
			 * @param[out] solidSize the size of solid caret. if @a bitmap is not @c null, this parameter is ignored
			 * @param[out] orientation the orientation of the caret. this value is used for hot spot calculation
			 */
			virtual void getCaretShape(std::auto_ptr<manah::win32::gdi::Bitmap>& bitmap,
				SIZE& solidSize, layout::Orientation& orientation) /*throw()*/ = 0;
			/**
			 * Installs the provider.
			 * @param updater the caret updator which notifies the text viewer to update the caret
			 */
			virtual void install(CaretShapeUpdater& updater) /*throw()*/ = 0;
			/// Uninstalls the provider.
			virtual void uninstall() /*throw()*/ = 0;
			friend class TextViewer;
		};

		/**
		 * Default implementation of @c ICaretShapeProvider.
		 * @note This class is not intended to be subclassed.
		 */
		class DefaultCaretShaper : public ICaretShapeProvider {
		public:
			DefaultCaretShaper() /*throw()*/;
		private:
			void getCaretShape(std::auto_ptr<manah::win32::gdi::Bitmap>& bitmap,
				SIZE& solidSize, layout::Orientation& orientation) /*throw()*/;
			void install(CaretShapeUpdater& updater) /*throw()*/;
			void uninstall() /*throw()*/;
		private:
			const TextViewer* viewer_;
		};

		/**
		 * @c LocaleSensitiveCaretShaper defines caret shape based on active keyboard layout.
		 * @note This class is not intended to be subclassed.
		 */
		class LocaleSensitiveCaretShaper : public ICaretShapeProvider,
			public ICaretListener, public ICaretStateListener, public ITextViewerInputStatusListener {
		public:
			explicit LocaleSensitiveCaretShaper(bool bold = false) /*throw()*/;
		private:
			// ICaretShapeProvider
			void getCaretShape(std::auto_ptr<manah::win32::gdi::Bitmap>& bitmap,
				SIZE& solidSize, layout::Orientation& orientation) /*throw()*/;
			void install(CaretShapeUpdater& updater) /*throw()*/;
			void uninstall() /*throw()*/;
			// ICaretListener
			void caretMoved(const class Caret& self, const kernel::Region& oldRegion);
			// ICaretStateListener
			void matchBracketsChanged(const Caret& self,
				const std::pair<kernel::Position, kernel::Position>& oldPair, bool outsideOfView);
			void overtypeModeChanged(const Caret& self);
			void selectionShapeChanged(const Caret& self);
			// ITextViewerInputStatusListener
			void textViewerIMEOpenStatusChanged() /*throw()*/;
			void textViewerInputLanguageChanged() /*throw()*/;
		private:
			CaretShapeUpdater* updater_;
			bool bold_;
		};

#if 0
		/**
		 * Interface of objects which define how the text editors react to the users' keyboard
		 * input. Ascension also provides the standard implementation of this interface
		 * @c DefaultKeyboardInputStrategy.
		 * @see TextViewer#set
		 */
		class IKeyboardInputStrategy {
		};
#endif

		/**
		 * Interface of objects which define how the text editors react to the users' mouse input.
		 * @note An instance of @c IMouseInputStrategy can't be shared multiple text viewers.
		 * @see TextViewer#setMouseInputStrategy
		 */
		class IMouseInputStrategy {
		public:
			/// Destructor.
			virtual ~IMouseInputStrategy() /*throw()*/ {}
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
			 * Interrupts the progressive mouse reaction.
			 * This method must be called before @c #uninstall call.
			 * @param forKeyboardInput true if the mouse reaction should interrupt because the
			 *        keyboard input was occured
			 */
			virtual void interruptMouseReaction(bool forKeyboardInput) = 0;
			/**
			 * The mouse input was occured and the viewer had focus.
			 * @param button the button of the mouse input
			 * @param action the action of the input
			 * @param position the mouse position (client coordinates)
			 * @param keyState indicates whether various key are pressed. this value is same as Win32 WM_*BUTTON*
			 * @return true if the strategy processed
			 */
			virtual bool mouseButtonInput(Button button, Action action, const POINT& position, uint keyState) = 0;
			/**
			 * The mouse was moved and the viewer had focus.
			 * @param position the mouse position (client coordinates)
			 * @param keyState indicates whether various key are pressed. this value is same as Win32 @c WM_MOSEMOVE
			 * @return true if the strategy processed
			 */
			virtual void mouseMoved(const POINT& position, uint keyState) = 0;
			/**
			 * The mouse wheel was rolated and the viewer had focus.
			 * @param delta the distance the wheel was rotated
			 * @param position the mouse position (client coordinates)
			 * @param keyState indicates whether various key are pressed. this value is same as Win32 WM_*BUTTON*
			 */
			virtual void mouseWheelRotated(short delta, const POINT& position, uint keyState) = 0;
			/**
			 * Shows a cursor on the viewer.
			 * @param position the cursor position (client coordinates)
			 * @retval true if the callee showed a cursor
			 * @retval false if the callee did not know the appropriate cursor
			 */
			virtual bool showCursor(const POINT& position) = 0;
			/// Uninstalls the strategy. The window is not destroyed yet at this time.
			virtual void uninstall() = 0;
			friend class TextViewer;
		};

		// the documentation is user-input.cpp
		class DefaultMouseInputStrategy : public IMouseInputStrategy,
				manah::com::IUnknownImpl<
					manah::typelist::Cat<
						MANAH_INTERFACE_SIGNATURE(IDropSource), manah::typelist::Cat<
							MANAH_INTERFACE_SIGNATURE(IDropTarget)
						>
					>, manah::com::NoReferenceCounting
				> {
			MANAH_UNASSIGNABLE_TAG(DefaultMouseInputStrategy);
		public:
			DefaultMouseInputStrategy(bool enableOLEDragAndDrop, bool showDraggingImage);
		private:
			virtual bool handleLeftButtonDoubleClick(const POINT& position, uint keyState);
			virtual bool handleRightButton(Action action, const POINT& position, uint keyState);
			virtual bool handleX1Button(Action action, const POINT& position, uint keyState);
			virtual bool handleX2Button(Action action, const POINT& position, uint keyState);
		private:
			void beginTimer(UINT interval);
			void doDragAndDrop();
			bool endAutoScroll();
			void endTimer();
			void extendSelection(const kernel::Position* to = 0);
			void handleLeftButtonPressed(const POINT& position, uint keyState);
			void handleLeftButtonReleased(const POINT& position, uint keyState);
			static void CALLBACK timeElapsed(HWND window, UINT message, UINT_PTR eventID, DWORD time);
			// IMouseInputStrategy
			void captureChanged();
			void install(TextViewer& viewer);
			void interruptMouseReaction(bool forKeyboardInput);
			bool mouseButtonInput(Button button, Action action, const POINT& position, uint keyState);
			void mouseMoved(const POINT& position, uint keyState);
			void mouseWheelRotated(short delta, const POINT& position, uint keyState);
			bool showCursor(const POINT& position);
			void uninstall();
			// IDropSource
			STDMETHODIMP QueryContinueDrag(BOOL escapePressed, DWORD keyState);
			STDMETHODIMP GiveFeedback(DWORD effect);
			// IDropTarget
			STDMETHODIMP DragEnter(IDataObject* data, DWORD keyState, POINTL pt, DWORD* effect);
			STDMETHODIMP DragOver(DWORD keyState, POINTL pt, DWORD* effect);
			STDMETHODIMP DragLeave();
			STDMETHODIMP Drop(IDataObject* data, DWORD keyState, POINTL pt, DWORD* effect);
		private:
			TextViewer* viewer_;
			enum {
				NONE = 0x00,
				SELECTION_EXTENDING_MASK = 0x10, EXTENDING_CHARACTER_SELECTION, EXTENDING_WORD_SELECTION, EXTENDING_LINE_SELECTION,
				AUTO_SCROLL_MASK = 0x20, APPROACHING_AUTO_SCROLL, AUTO_SCROLL_DRAGGING, AUTO_SCROLL,
				OLE_DND_MASK = 0x40, APPROACHING_OLE_DND, OLE_DND_SOURCE, OLE_DND_TARGET
			} state_;
			POINT dragApproachedPosition_;
			struct Selection {
				length_t initialLine;	// line of the anchor when entered the selection extending
				std::pair<length_t, length_t> initialWordColumns;
			} selection_;
			struct DragAndDrop {
				bool enabled;
				length_t numberOfRectangleLines;
				manah::com::ComPtr<IDragSourceHelper> dragSourceHelper;
				manah::com::ComPtr<IDropTargetHelper> dropTargetHelper;
			} dnd_;
			std::auto_ptr<manah::win32::ui::Window> autoScrollOriginMark_;
			const presentation::hyperlink::IHyperlink* lastHoveredHyperlink_;
			static std::map<UINT_PTR, DefaultMouseInputStrategy*> timerTable_;
			static const UINT SELECTION_EXPANSION_INTERVAL, OLE_DRAGGING_TRACK_INTERVAL;
		};

		/**
		 * Interface for objects which are interested in change of size of a @c TextViewer.
		 * @see TextViewer#addDisplaySizeListener, TextViewer#removeDisplaySizeListener
		 */
		class IDisplaySizeListener {
		private:
			/// The size of the viewer was changed.
			virtual void viewerDisplaySizeChanged() = 0;
			friend class TextViewer;
		};

#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
		namespace internal {class TextViewerAccessibleProxy;}
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY

		class TextViewer :
				public manah::win32::ui::CustomControl<TextViewer>,
				public kernel::IDocumentListener, public kernel::IDocumentStateListener,
				public kernel::IDocumentRollbackListener, public layout::IFontSelectorListener,
				public layout::IVisualLinesListener, public ICaretListener, public ICaretStateListener,
				public ascension::kernel::internal::IPointCollection<VisualPoint> {
			typedef manah::win32::ui::CustomControl<TextViewer> BaseControl;
			DEFINE_WINDOW_CLASS() {
				name = L"ascension:text-viewer";
				style = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW | CS_DBLCLKS;
				bgColor = COLOR_WINDOW;
				cursor = MAKEINTRESOURCEW(32513);	// IDC_IBEAM
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
			struct Configuration : public layout::LayoutSettings {
				/// Color of active selected text. Standard setting is {@c COLOR_HIGHLIGHTTEXT, @c COLOR_HIGHLIGHT}.
				presentation::Colors selectionColor;
				/// Color of inactive selected text. Standard setting is {@c COLOR_INACTIVECAPTIONTEXT, @c COLOR_INACTIVECAPTION}.
				presentation::Colors inactiveSelectionColor;
				/// Color of the inaccessible area. Standard setting is {@c COLOR_GRAYTEXT, @c color.background}.
				presentation::Colors restrictionColor;
				/// The amount of the leading margin in pixels. Default value is 5. This member will be ignored if the text is center-aligned.
				int leadingMargin;
				/// The amount of the top margin in pixels. Default value is 1.
				int topMargin;
				/// Set true to vanish the cursor when the user types. Default value depends on system setting.
				bool vanishesCursor;
				/// Set true to use also Rich Text Format for clipboard operations. Default value is false.
				bool usesRichTextClipboardFormat;
				/// Constructor.
				Configuration() /*throw()*/ : leadingMargin(5), topMargin(1), usesRichTextClipboardFormat(false) {
#if(_WIN32_WINNT >= 0x0501)
					BOOL b;
					::SystemParametersInfo(SPI_GETMOUSEVANISH, 0, &b, 0);
					vanishesCursor = manah::toBoolean(b);
#else
					vanishesCursor = false;
#endif // _WIN32_WINNT >= 0x0501
				}
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
					layout::Alignment alignment;
					/// Start value of the line number. Default value is 1.
					length_t startValue;
					/// Minimum number of digits. Default value is 4.
					uchar minimumDigits;
					/// Leading margin in pixels. Default value is 6.
					int leadingMargin;
					/// Trailing margin in pixels. Default value is 1.
					int trailingMargin;
					/// Color of the text. Default value is invalid color which is fallbacked to
					/// the color of the system normal text.
					presentation::Colors textColor;
					/// Color of the border. Default value is invalid color which is fallbacked to
					/// the color of the system normal text.
					presentation::Color borderColor;
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
					layout::DigitSubstitutionType digitSubstitution;
					/// Constructor initializes the all members to their default values.
					LineNumbers() /*throw()*/ : visible(false), alignment(layout::ALIGN_AUTO), startValue(1),
						minimumDigits(4), leadingMargin(6), trailingMargin(1), borderColor(presentation::Color()),
						borderWidth(1), borderStyle(SOLID), digitSubstitution(layout::DST_USER_DEFAULT) {}
					/// Returns true if one of the all members are valid.
					bool verify() const /*throw()*/ {return leadingMargin >= 0 && trailingMargin >= 0;}
				} lineNumbers;	/// Configuration about the line numbers area.
				/// Configuration about an indicator margin.
				struct IndicatorMargin {
					/// Wether the indicator margin is visible or not. Default value is false and the indicator margin is invisible.
					bool visible;
					/// Width of the indicator margin. Default value is 15.
					ushort width;
					/// Background color. Default value is invalid color which is fallbacked to the
					/// system color @c COLOR_3DFACE.
					presentation::Color color;
					/// Color of the border. Default value is invalid color which is fallbacked to
					/// the system color @c COLOR_3DSHADOW.
					presentation::Color borderColor;
					/// Constructor initializes the all members to their default values.
					IndicatorMargin() /*throw()*/ : visible(false), width(15) {}
					/// Returns true if one of the all members are valid.
					bool verify() const /*throw()*/ {return width >= 0;}
				} indicatorMargin;	/// Configuration about the indicator margin.
				/// Alignment of the vertical ruler. Can be either @c layout#ALIGN_LEFT or
				/// @c layout#ALIGN_RIGHT. Default value is determined based on @c ASCENSION_DEFAULT_TEXT_ORIENTATION.
				layout::Alignment alignment;
				/// Constructor.
				VerticalRulerConfiguration() /*throw()*/ :
					alignment(ASCENSION_DEFAULT_TEXT_ORIENTATION == layout::LEFT_TO_RIGHT ? layout::ALIGN_LEFT : layout::ALIGN_RIGHT) {}
				/// Returns true if one of the all members are valid.
				bool verify() const /*throw()*/ {return lineNumbers.verify()
					&& indicatorMargin.verify() && (alignment == layout::ALIGN_LEFT || alignment == layout::ALIGN_RIGHT);}
			};

			// constructors
			explicit TextViewer(presentation::Presentation& presentation);
			TextViewer(const TextViewer& rhs);
			virtual ~TextViewer();
			// window creation
			virtual bool create(HWND parent, const RECT& rect, DWORD style, DWORD exStyle);
			// listeners and strategies
			void addDisplaySizeListener(IDisplaySizeListener& listener);
			void addInputStatusListener(ITextViewerInputStatusListener& listener);
			void addViewportListener(IViewportListener& listener);
			void removeDisplaySizeListener(IDisplaySizeListener& listener);
			void removeInputStatusListener(ITextViewerInputStatusListener& listener);
			void removeViewportListener(IViewportListener& listener);
			void setCaretShapeProvider(ASCENSION_SHARED_POINTER<ICaretShapeProvider> shaper) /*throw()*/;
			void setMouseInputStrategy(IMouseInputStrategy* newStrategy, bool delegateOwnership);
			// attributes
			const Configuration& configuration() const /*throw()*/;
			kernel::Document& document();
			const kernel::Document& document() const;
			presentation::Presentation& presentation() /*throw()*/;
			const presentation::Presentation& presentation() const /*throw()*/;
			ulong scrollRate(bool horizontal) const /*throw()*/;
			layout::TextRenderer& textRenderer() /*throw()*/;
			const layout::TextRenderer& textRenderer() const /*throw()*/;
			void setConfiguration(const Configuration* general, const VerticalRulerConfiguration* verticalRuler);
			const VerticalRulerConfiguration& verticalRulerConfiguration() const /*throw()*/;
			// caret
			Caret& caret() /*throw()*/;
			const Caret& caret() const /*throw()*/;
#ifndef ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER
			// Global IME
			void enableActiveInputMethod(bool enable = true) /*throw()*/;
			bool isActiveInputMethodEnabled() const /*throw()*/;
#endif // !ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER
			// UI
			void beep() /*throw()*/;
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
			HRESULT accessibleObject(::IAccessible*& acc) const /*throw()*/;
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY
			void hideToolTip();
			void lockScroll(bool unlock = false);
			void scroll(int dx, int dy, bool redraw);
			void scrollTo(int x, int y, bool redraw);
			void scrollTo(length_t line, bool redraw);
			void showToolTip(const String& text, ulong timeToWait = -1, ulong timeRemainsVisible = -1);
#ifndef ASCENSION_NO_TEXT_SERVICES_FRAMEWORK
			HRESULT startTextServices();
#endif // !ASCENSION_NO_TEXT_SERVICES_FRAMEWORK
			// content assist
			contentassist::IContentAssistant* contentAssistant() const /*throw()*/;
			void setContentAssistant(std::auto_ptr<contentassist::IContentAssistant> newContentAssistant) /*throw()*/;
			// redraw
			void redrawLine(length_t line, bool following = false);
			void redrawLines(length_t first, length_t last);
			// freeze
			void freeze(bool forAllClones = true);
			bool isFrozen() const /*throw()*/;
			void unfreeze(bool forAllClones = true);
			// mouse input
			bool allowsMouseInput() const /*throw()*/;
			void enableMouseInput(bool enable);
			// client coordinates vs. character position mappings
			kernel::Position characterForClientXY(const POINT& pt,
				layout::LineLayout::Edge, bool abortNoCharacter = false,
				kernel::EditPoint::CharacterUnit snapPolicy = kernel::EditPoint::DEFAULT_UNIT) const;
			POINT clientXYForCharacter(const kernel::Position& position,
				bool fullSearchY, layout::LineLayout::Edge edge = layout::LineLayout::LEADING) const;
			// utilities
			void firstVisibleLine(length_t* logicalLine, length_t* visualLine, length_t* visualSubline) const /*throw()*/;
			HitTestResult hitTest(const POINT& pt) const;
			length_t numberOfVisibleLines() const /*throw()*/;
			length_t numberOfVisibleColumns() const /*throw()*/;
			RECT textAreaMargins() const /*throw()*/;

		protected:
			virtual void doBeep() /*throw()*/;
			virtual void drawIndicatorMargin(length_t line, manah::win32::gdi::DC& dc, const RECT& rect);
			bool handleKeyDown(UINT key, bool controlPressed, bool shiftPressed, bool altPressed) /*throw()*/;

			// helpers
		private:
			int getDisplayXOffset(length_t line) const;
			void handleGUICharacterInput(CodePoint c);
			void internalUnfreeze();
			void mapClientYToLine(int y, length_t* logicalLine, length_t* visualSublineOffset, bool* snapped = 0) const /*throw()*/;
			int mapLineToClientY(length_t line, bool fullSearch) const;
			void recreateCaret();
			void redrawVerticalRuler();
			void updateCaretPosition();
			void updateIMECompositionWindowPosition();
			void updateScrollBars();

			// protected interfaces
		protected:
			// kernel.IDocumentStateListener (overridable)
			virtual void documentAccessibleRegionChanged(const kernel::Document& document);
			virtual void documentModificationSignChanged(const kernel::Document& document);
			virtual void documentPropertyChanged(const kernel::Document& document, const kernel::DocumentPropertyKey& key);
			virtual void documentReadOnlySignChanged(const kernel::Document& document);
			// ICaretListener (overridable)
			virtual void caretMoved(const Caret& self, const kernel::Region& oldRegion);
			// ICaretStateListener (overridable)
			virtual void matchBracketsChanged(const Caret& self,
				const std::pair<kernel::Position, kernel::Position>& oldPair, bool outsideOfView);
			virtual void overtypeModeChanged(const Caret& self);
			virtual void selectionShapeChanged(const Caret& self);
		private:
			// kernel.IDocumentListener
			void documentAboutToBeChanged(const kernel::Document& document, const kernel::DocumentChange& change);
			void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change);
			// kernel.IDocumentRollbackListener
			void documentUndoSequenceStarted(const kernel::Document& document);
			void documentUndoSequenceStopped(const kernel::Document& document, const kernel::Position& resultPosition);
			// layout.IFontSelectorListener
			void fontChanged() /*throw()*/;
			// layout.IVisualLinesListener
			void visualLinesDeleted(length_t first, length_t last, length_t sublines, bool longestLineChanged) /*throw()*/;
			void visualLinesInserted(length_t first, length_t last) /*throw()*/;
			void visualLinesModified(length_t first, length_t last,
				signed_length_t sublinesDifference, bool documentChanged, bool longestLineChanged) /*throw()*/;
			// internal.IPointCollection<VisualPoint>
			void addNewPoint(VisualPoint& point) {points_.insert(&point);}
			void removePoint(VisualPoint& point) {points_.erase(&point);}

			// message handlers
			MANAH_DECLEAR_WINDOW_MESSAGE_MAP(TextViewer);
		protected:
			virtual LRESULT preTranslateWindowMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled);
			void onCaptureChanged(HWND newWindow);
			void onChar(UINT ch, UINT flags);
			bool onCommand(WORD id, WORD notifyCode, HWND control);
			bool onContextMenu(HWND window, const POINT& pt);
			void onDestroy();
			bool onEraseBkgnd(HDC dc);
			HFONT onGetFont();
			void onHScroll(UINT sbCode, UINT pos, HWND scrollBar);
			void onIMEComposition(WPARAM wParam, LPARAM lParam, bool& handled);
			void onIMEEndComposition();
			LRESULT onIMENotify(WPARAM command, LPARAM lParam, bool& handled);
			LRESULT onIMERequest(WPARAM command, LPARAM lParam, bool& handled);
			void onIMEStartComposition();
			void onKeyDown(UINT ch, UINT flags, bool& handled);
			void onKillFocus(HWND newWindow);
			void onLButtonDblClk(UINT keyState, const POINT& pt, bool& handled);
			void onLButtonDown(UINT keyState, const POINT& pt, bool& handled);
			void onLButtonUp(UINT keyState, const POINT& pt, bool& handled);
			void onMButtonDblClk(UINT keyState, const POINT& pt, bool& handled);
			void onMButtonDown(UINT keyState, const POINT& pt, bool& handled);
			void onMButtonUp(UINT keyState, const POINT& pt, bool& handled);
			void onMouseMove(UINT keyState, const POINT& pt);
#ifdef WM_MOUSEWHEEL
			void onMouseWheel(UINT flags, short zDelta, const POINT& pt);
#endif // WM_MOUSEWHEEL
			bool onNcCreate(CREATESTRUCTW& cs);
			bool onNotify(int id, NMHDR& nmhdr);
			void onPaint(manah::win32::gdi::PaintDC& dc);
			void onRButtonDblClk(UINT keyState, const POINT& pt, bool& handled);
			void onRButtonDown(UINT keyState, const POINT& pt, bool& handled);
			void onRButtonUp(UINT keyState, const POINT& pt, bool& handled);
			bool onSetCursor(HWND window, UINT hitTest, UINT message);
			void onSetFocus(HWND oldWindow);
			void onSize(UINT type, int cx, int cy);
			void onStyleChanged(int type, const STYLESTRUCT& style);
			void onStyleChanging(int type, STYLESTRUCT& style);
			void onSysChar(UINT ch, UINT flags);
			void onSysColorChange();
			bool onSysKeyDown(UINT ch, UINT flags);
			bool onSysKeyUp(UINT ch, UINT flags);
#ifdef WM_THEMECHANGED
			void onThemeChanged();
#endif // WM_THEMECHANGED
			void onTimer(UINT_PTR eventId, TIMERPROC timerProc);
#ifdef WM_UNICHAR
			void onUniChar(UINT ch, UINT flags);
#endif // WM_UNICHAR
			void onVScroll(UINT sbCode, UINT pos, HWND scrollBar);
#ifdef WM_XBUTTONDBLCLK
			bool onXButtonDblClk(WORD xButton, WORD keyState, const POINT& pt);
			bool onXButtonDown(WORD xButton, WORD keyState, const POINT& pt);
			bool onXButtonUp(WORD xButton, WORD keyState, const POINT& pt);
#endif // WM_XBUTTONDBLCLK

			// internal classes
		private:
			/// Internal extension of @c layout#TextRenderer.
			class Renderer : public layout::TextRenderer {
				MANAH_UNASSIGNABLE_TAG(Renderer);
			public:
				explicit Renderer(TextViewer& viewer);
				Renderer(const Renderer& rhs, TextViewer& viewer);
				void rewrapAtWindowEdge();
			private:
				// FontSelector
				std::auto_ptr<manah::win32::gdi::DC> getDeviceContext() const;
				// ILayoutInformationProvider
				const layout::LayoutSettings& getLayoutSettings() const /*throw()*/;
				int getWidth() const /*throw()*/;
			private:
				TextViewer& viewer_;
			};
			/// @c VerticalRulerDrawer draws the vertical ruler of the @c TextViewer.
			class VerticalRulerDrawer {
				MANAH_NONCOPYABLE_TAG(VerticalRulerDrawer);
			public:
				VerticalRulerDrawer(TextViewer& viewer, bool enableDoubleBuffering) /*throw()*/;
				const VerticalRulerConfiguration& configuration() const /*throw()*/;
				void draw(manah::win32::gdi::PaintDC& dc);
				void setConfiguration(const VerticalRulerConfiguration& configuration);
				void update() /*throw()*/;
				int width() const /*throw()*/;
			private:
				uchar getLineNumberMaxDigits() const /*throw()*/;
				void recalculateWidth() /*throw()*/;
				void updateGDIObjects() /*throw()*/;
				TextViewer& viewer_;
				VerticalRulerConfiguration configuration_;
				int width_;
				uchar lineNumberDigitsCache_;
				manah::win32::gdi::Pen indicatorMarginPen_, lineNumbersPen_;
				manah::win32::gdi::Brush indicatorMarginBrush_, lineNumbersBrush_;
				const bool enablesDoubleBuffering_;
				std::auto_ptr<manah::win32::gdi::DC> memoryDC_;
				manah::win32::gdi::Bitmap memoryBitmap_;
			};

			// enumerations
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

				ID_INVOKE_HYPERLINK	// Open <hyperlink>
			};

			// data members
		private:
			// 大物
			presentation::Presentation& presentation_;
			std::auto_ptr<Caret> caret_;
			std::auto_ptr<Renderer> renderer_;
			Configuration configuration_;
			std::set<VisualPoint*> points_;
			HWND toolTip_;
			Char* tipText_;
			ascension::internal::StrategyPointer<IMouseInputStrategy> mouseInputStrategy_;
			ascension::internal::Listeners<IDisplaySizeListener> displaySizeListeners_;
			ascension::internal::Listeners<ITextViewerInputStatusListener> inputStatusListeners_;
			ascension::internal::Listeners<IViewportListener> viewportListeners_;
			std::auto_ptr<VerticalRulerDrawer> verticalRulerDrawer_;
			std::auto_ptr<contentassist::IContentAssistant> contentAssistant_;
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
			internal::TextViewerAccessibleProxy* accessibleProxy_;
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY

			// modes
			struct ModeState {
				bool cursorVanished;			// the cursor is vanished for user is inputting
#ifndef ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER
				bool activeInputMethodEnabled;	// true if uses Global IME (deprecated)
#endif // !ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER

				ModeState() /*throw()*/ : cursorVanished(false)
#ifndef ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER
					, activeInputMethodEnabled(true)
#endif // !ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER
				{}
			} modeState_;

			// scroll information
			struct ScrollInfo {
				struct {
					int position;		// SCROLLINFO.nPos
					int maximum;		// SCROLLINFO.nMax
					UINT pageSize;	// SCROLLINFO.nPage
//					ulong rate;			// 最小スクロール量が何文字 (何行) に相当するか (普通は 1)
				} horizontal, vertical;
				length_t firstVisibleLine, firstVisibleSubline;
				bool changed;
				std::size_t lockCount;	// see TextViewer.lockScroll
				ScrollInfo() /*throw()*/ : firstVisibleLine(0), firstVisibleSubline(0), changed(false), lockCount(0) {
					horizontal.position = vertical.position = 0;
//					horizontal.rate = vertical.rate = 1;
				}
				ulong x() const /*throw()*/ {return horizontal.position/* * horizontal.rate*/;}
				ulong y() const /*throw()*/ {return vertical.position/* * vertical.rate*/;}
				void resetBars(const TextViewer& viewer, int bars, bool pageSizeChanged) /*throw()*/;
				void updateVertical(const TextViewer& viewer) /*throw()*/;
			} scrollInfo_;

			// freeze information
			struct FreezeInfo {
				ulong count;								// zero for not frozen
				std::pair<length_t, length_t> invalidLines;	// 凍結中に再描画を要求された行。要求が無ければ first == second
				FreezeInfo() /*throw()*/ : count(0) {invalidLines.first = invalidLines.second = INVALID_INDEX;}
			} freezeInfo_;

			// a bitmap for caret presentation
			struct CaretShape {
				ASCENSION_SHARED_POINTER<ICaretShapeProvider> shaper;
				layout::Orientation orientation;
				int width;
				std::auto_ptr<manah::win32::gdi::Bitmap> bitmap;
				CaretShape() /*throw()*/ : orientation(layout::LEFT_TO_RIGHT), width(0) {}
			} caretShape_;

			// input state
			bool imeCompositionActivated_, imeComposingCharacter_;
			ulong mouseInputDisabledCount_;

			friend class VisualPoint;
			friend class VirtualBox;
			friend class VerticalRulerDrawer;
			friend class CaretShapeUpdater;
			friend class Renderer;
		};

		/// Highlights the line on which the caret is put.
		class CurrentLineHighlighter : public presentation::ILineColorDirector,
				public ICaretListener, public ICaretStateListener, public kernel::IPointLifeCycleListener {
			MANAH_NONCOPYABLE_TAG(CurrentLineHighlighter);
		public:
			// constant
			static const ILineColorDirector::Priority LINE_COLOR_PRIORITY;
			// constructors
			CurrentLineHighlighter(Caret& caret, const presentation::Colors& color);
			~CurrentLineHighlighter() /*throw()*/;
			// attributes
			const presentation::Colors& color() const /*throw()*/;
			void setColor(const presentation::Colors& color) /*throw()*/;
		private:
			// presentation.ILineColorDirector
			ILineColorDirector::Priority queryLineColor(length_t line, presentation::Colors& color) const;
			// ICaretListener
			void caretMoved(const Caret& self, const kernel::Region& oldRegion);
			// ICaretStateListener
			void matchBracketsChanged(const Caret& self, const std::pair<kernel::Position, kernel::Position>& oldPair, bool outsideOfView);
			void overtypeModeChanged(const Caret& self);
			void selectionShapeChanged(const Caret& self);
			// kernel.IPointLifeCycleListener
			void pointDestroyed();
		private:
			Caret* caret_;
			presentation::Colors color_;
		};

		/// Provides the utility stuffs for viewers.
		namespace utils {
			void closeCompletionProposalsPopup(TextViewer& viewer) /*throw()*/;
		} // namespace utils


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
inline bool TextViewer::allowsMouseInput() const /*throw()*/ {return mouseInputDisabledCount_ == 0;} 

/// Informs the end user of <strong>safe</strong> error.
inline void TextViewer::beep() /*throw()*/ {doBeep();}

/// Returns the caret.
inline Caret& TextViewer::caret() /*throw()*/ {return *caret_;}

/// Returns the caret.
inline const Caret& TextViewer::caret() const /*throw()*/ {return *caret_;}

/**
 * Returns the general configuration.
 * @see #verticalRulerConfiguration, #setConfiguration
 */
inline const TextViewer::Configuration& TextViewer::configuration() const /*throw()*/ {return configuration_;}

/// Returns the content assistant or @c null if not registered.
inline contentassist::IContentAssistant* TextViewer::contentAssistant() const /*throw()*/ {return contentAssistant_.get();}

/// Returns the document.
inline kernel::Document& TextViewer::document() {return presentation_.document();}

/// Returns the document.
inline const kernel::Document& TextViewer::document() const {return presentation_.document();}

#ifndef ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER
/**
 * Enables Global IME.
 * This setting effects under only Windows NT 4.0. Otherwise, Ascension does not use Global IME.
 * @deprecated 0.8
 */
inline void TextViewer::enableActiveInputMethod(bool enable /* = true */) /*throw()*/ {modeState_.activeInputMethodEnabled = enable;}
#endif // !ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER

/**
 * Enables/disables the mouse operations.
 *
 * A @c TextViewer has a disabled count for the mouse input. If this value is not zero, any mouse
 * inputs are not allowed.
 *
 * These is no way to disable the scroll bars.
 * @param enable set false to increment the disabled count, true to decrement
 * @see #allowsMouseInput
 */
inline void TextViewer::enableMouseInput(bool enable) {
	if(mouseInputDisabledCount_ != 0 || !enable) mouseInputDisabledCount_ += !enable ? 1 : -1;}

/**
 * Returns the information about the uppermost visible line in the viewer.
 * @param[out] logicalLine the logical index of the line. can be @c null if not needed
 * @param[out] visualLine the visual index of the line. can be @c null if not needed
 * @param[out] visualSubline the offset of @a visualLine from the first line in @a logicalLine. can be @c null if not needed
 */
inline void TextViewer::firstVisibleLine(length_t* logicalLine, length_t* visualLine, length_t* visualSubline) const /*throw()*/ {
	if(logicalLine != 0)
		*logicalLine = scrollInfo_.firstVisibleLine;
	if(visualSubline != 0)
		*visualSubline = scrollInfo_.firstVisibleSubline;
	if(visualLine != 0)
		*visualLine = scrollInfo_.y();
}

#ifndef ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER
/// Returns true if Global IME is enabled.
inline bool TextViewer::isActiveInputMethodEnabled() const /*throw()*/ {return modeState_.activeInputMethodEnabled;}
#endif // !ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER

/// Returns true if the viewer is frozen.
inline bool TextViewer::isFrozen() const /*throw()*/ {return freezeInfo_.count != 0;}

/**
 * Returns the number of the drawable columns in the window.
 * @return the number of columns
 */
inline length_t TextViewer::numberOfVisibleColumns() const /*throw()*/ {
	RECT r;
	getClientRect(r);
	return (r.left == r.right) ? 0 :
		(r.right - r.left - configuration_.leadingMargin - verticalRulerDrawer_->width()) / renderer_->averageCharacterWidth();
}

/**
 * Returns the number of the drawable lines in the window.
 * @return the number of lines
 */
inline length_t TextViewer::numberOfVisibleLines() const /*throw()*/ {
	RECT r;
	getClientRect(r);
	return (r.top == r.bottom) ? 0 : (r.bottom - r.top - configuration_.topMargin) / renderer_->lineHeight();
}

/// Returns the presentation object. 
inline presentation::Presentation& TextViewer::presentation() /*throw()*/ {return presentation_;}

/// Returns the presentation object. 
inline const presentation::Presentation& TextViewer::presentation() const /*throw()*/ {return presentation_;}

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
 * Returns the ratio to vertical/horizontal scroll amount of line/column numbers.
 * @param horizontal set true for horizontal, false for vertical
 * @return the rate
 */
inline ulong TextViewer::scrollRate(bool horizontal) const /*throw()*/ {
	return 1/*horizontal ? scrollInfo_.horizontal.rate : scrollInfo_.vertical.rate*/;}

/**
 * Sets the caret shape provider.
 * @param shaper the new caret shaper
 */
inline void TextViewer::setCaretShapeProvider(ASCENSION_SHARED_POINTER<ICaretShapeProvider> shaper) {caretShape_.shaper = shaper;}

/// Returns the text renderer.
inline layout::TextRenderer& TextViewer::textRenderer() /*throw()*/ {return *renderer_;}

/// Returns the text renderer.
inline const layout::TextRenderer& TextViewer::textRenderer() const /*throw()*/ {return *renderer_;}

/**
 * Returns the vertical ruler's configuration.
 * @see #configuration, #setConfiguration
 */
inline const TextViewer::VerticalRulerConfiguration&
	TextViewer::verticalRulerConfiguration() const /*throw()*/ {return verticalRulerDrawer_->configuration();}

/// Returns the vertical ruler's configurations.
inline const TextViewer::VerticalRulerConfiguration&
	TextViewer::VerticalRulerDrawer::configuration() const /*throw()*/ {return configuration_;}

/// Returns the width of the vertical ruler.
inline int TextViewer::VerticalRulerDrawer::width() const /*throw()*/ {return width_;}

}} // namespace ascension.viewers

#endif // !ASCENSION_VIEWER_HPP
