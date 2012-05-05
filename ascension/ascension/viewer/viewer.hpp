/**
 * @file viewer.hpp
 * This header defines several visual presentation classes.
 * @author exeal
 * @date 2003-2006 (was EditView.h)
 * @date 2006-2012
 */

#ifndef ASCENSION_VIEWER_HPP
#define ASCENSION_VIEWER_HPP

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION, ...
#include <ascension/graphics/text-renderer.hpp>
#include <ascension/graphics/text-viewport.hpp>
#include <ascension/kernel/point.hpp>
#include <ascension/presentation/presentation.hpp>
#include <ascension/presentation/text-style.hpp>
#include <ascension/viewer/caret-observers.hpp>
#include <ascension/viewer/ruler.hpp>
#include <ascension/viewer/viewer-observers.hpp>
#include <ascension/viewer/widgetapi/scrollable.hpp>
#include <algorithm>
#include <array>
#include <set>
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
#	include <ascension/win32/com/unknown-impl.hpp>
#	include <shlobj.h>	// IDropTargetHelper
#endif // ASCENSION_WINDOW_SYSTEM_WIN32

#ifndef ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER
#	include <dimm.h>
#endif // !ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER

#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
#	include <Oleacc.h>
#	include <MSAAtext.h>
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY

#ifndef ASCENSION_NO_TEXT_OBJECT_MODEL
#	include <tom.h>
#endif // !ASCENSION_NO_TEXT_OBJECT_MODEL


namespace ascension {

	namespace viewers {
		class VisualPoint;
		class TextViewer;
	}

	/// Provides stuffs for source code editors.
	/// @todo Need refinements.
	namespace source {
		boost::optional<kernel::Region> getPointedIdentifier(const viewers::TextViewer& viewer);
		boost::optional<kernel::Region> getNearestIdentifier(
			const kernel::Document& document, const kernel::Position& position);
		bool getNearestIdentifier(const kernel::Document& document,
			const kernel::Position& position, Index* startOffsetInLine, Index* endOffsetInLine);
	}

	namespace contentassist {
		class ContentAssistant;
	}

	namespace viewers {

		/**
		 * A virtual rectangle placed in the viewer.
		 * @note This feature is not fully available on bidirectional texts.
		 * @see Caret#boxForRectangleSelection
		 */
		class VirtualBox {
			ASCENSION_UNASSIGNABLE_TAG(VirtualBox);
		public:
			VirtualBox(const TextViewer& viewer, const kernel::Region& region) /*throw()*/;
			bool characterRangeInVisualLine(
				const graphics::font::VisualLine& line, Range<Index>& range) const /*throw()*/;
			bool includes(const graphics::NativePoint& p) const /*throw()*/;
			void update(const kernel::Region& region) /*throw()*/;
		private:
			struct Point {
				graphics::font::VisualLine line;
				graphics::Scalar ipd;	// distance from left/top-edge of content-area
			};
			std::array<Point, 2> points_;
			const TextViewer& viewer_;
			const Point& beginning() const /*throw()*/ {return points_[(points_[0].line <= points_[1].line) ? 0 : 1];}
			const Point& end() const /*throw()*/ {return points_[(&beginning() == &points_[0]) ? 1 : 0];}
			graphics::Scalar startEdge() const /*throw()*/ {return std::min(points_[0].ipd, points_[1].ipd);}
			graphics::Scalar endEdge() const /*throw()*/ {return std::max(points_[0].ipd, points_[1].ipd);}
		};

#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
	}

	namespace viewers {
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY

		class TextViewer :
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
				public Gtk::Widget, public Gtk::Scrollable,
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
				public QAbstractScrollArea,
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
				public NSView,
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
				public win32::CustomControl,
				public win32::com::IUnknownImpl<
					ASCENSION_WIN32_COM_INTERFACE(IDropTarget), win32::com::NoReferenceCounting
				>,
#endif
				public kernel::DocumentListener, public kernel::DocumentRollbackListener,
				public graphics::font::DefaultFontListener, public graphics::font::VisualLinesListener,
				public graphics::font::TextViewportListener, public graphics::font::ComputedWritingModeListener,
				public CaretListener, public CaretStateListener, public detail::PointCollection<VisualPoint> {
		public:
			/**
			 * Result of hit test.
			 * @see #hitTest
			 */
			enum HitTestResult {
				/// The point is the indicator margin in the ruler.
				INDICATOR_MARGIN = 1 << 0,
				/// The point is the line numbers area in the ruler.
				LINE_NUMBERS = 1 << 1,
				/// The point is 'padding-start' of the text area.
				TEXT_AREA_PADDING_START = 1 << 2,
				/// The point is 'content-rectangle' of the text area.
				TEXT_AREA_CONTENT_RECTANGLE = 1 << 3,
				/// A mask for ruler.
				RULER_MASK = INDICATOR_MARGIN | LINE_NUMBERS,
				/// A mask for text area.
				TEXT_AREA_MASK = TEXT_AREA_PADDING_START | TEXT_AREA_CONTENT_RECTANGLE,
				/// The point is outside of the local bounds of the text viewer.
				OUT_OF_VIEWER = 0
			};

			/**
			 * A general configuration of the viewer.
			 * @see TextViewer#getConfigurations, TextViewer#setConfigurations
			 */
			struct Configuration /*: public graphics::LayoutSettings*/ {
				/// Foreground color of active selected text. Standard setting is @c COLOR_HIGHLIGHTTEXT.
				graphics::Color selectionForeground;
				/// Background color of active selected text. Standard setting is @c COLOR_HIGHLIGHT.
				graphics::Color selectionBackground;
				/// Foreground color of inactive selected text. Standard setting is @c COLOR_INACTIVECAPTIONTEXT.
				graphics::Color inactiveSelectionForeground;
				/// Background color of inactive selected text. Standard setting is @c COLOR_INACTIVECAPTION.
				graphics::Color inactiveSelectionBackground;
				/// Foreground color of the inaccessible area. Standard setting is @c COLOR_GRAYTEXT.
				graphics::Color restrictionForeground;
				/// Background color of the inaccessible area. Standard setting is @c color.background.
				graphics::Color restrictionBackground;
				/// The reading direction of UI.
				presentation::ReadingDirection readingDirection;
				/// Set @c true to vanish the cursor when the user types. Default value depends on system setting.
				bool vanishesCursor;
				/// Set @c true to use also Rich Text Format for clipboard operations. Default value is @c false.
				bool usesRichTextClipboardFormat;

				Configuration() /*throw()*/;
			};

			/// Implementation of @c graphics#font#TextRenderer for @c TextViewer.
			class Renderer : public graphics::font::TextRenderer {
				ASCENSION_UNASSIGNABLE_TAG(Renderer);
			public:
				explicit Renderer(TextViewer& viewer);
				Renderer(const Renderer& other, TextViewer& viewer);
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
				void rewrapAtWindowEdge();
#endif // ASCENSION_ABANDONED_AT_VERSION_08
				// TextRenderer
				std::unique_ptr<const graphics::font::TextLayout> createLineLayout(Index line) const;
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
				graphics::Scalar width() const /*throw()*/;
#endif // ASCENSION_ABANDONED_AT_VERSION_08
			private:
				TextViewer& viewer_;
			};

			// constructors
			explicit TextViewer(presentation::Presentation& presentation);
			TextViewer(const TextViewer& other);
			virtual ~TextViewer();
			// listeners and strategies
			void addDisplaySizeListener(DisplaySizeListener& listener);
			void addViewportListener(ViewportListener& listener);
			void removeDisplaySizeListener(DisplaySizeListener& listener);
			void removeViewportListener(ViewportListener& listener);
			void setMouseInputStrategy(std::shared_ptr<MouseInputStrategy> newStrategy);
			// attributes
			const Configuration& configuration() const /*throw()*/;
			kernel::Document& document();
			const kernel::Document& document() const;
			presentation::Presentation& presentation() /*throw()*/;
			const presentation::Presentation& presentation() const /*throw()*/;
			const RulerConfiguration& rulerConfiguration() const /*throw()*/;
			unsigned long scrollRate(bool horizontal) const /*throw()*/;
			void setConfiguration(const Configuration* general,
				const RulerConfiguration* ruler, bool synchronizeUI);
			Renderer& textRenderer() /*throw()*/;
			const Renderer& textRenderer() const /*throw()*/;
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
			HRESULT accessibleObject(IAccessible*& acc) const /*throw()*/;
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY
			void hideToolTip();
			void showToolTip(const String& text, unsigned long timeToWait = -1, unsigned long timeRemainsVisible = -1);
#ifndef ASCENSION_NO_TEXT_SERVICES_FRAMEWORK
			HRESULT startTextServices();
#endif // !ASCENSION_NO_TEXT_SERVICES_FRAMEWORK
			// content assist
			contentassist::ContentAssistant* contentAssistant() const /*throw()*/;
			void setContentAssistant(std::unique_ptr<contentassist::ContentAssistant> newContentAssistant) /*throw()*/;
			// redraw
			void redrawLine(Index line, bool following = false);
			void redrawLines(const Range<Index>& lines);
			// freeze
			void freeze();
			bool isFrozen() const /*throw()*/;
			void unfreeze();
			// mouse input
			bool allowsMouseInput() const /*throw()*/;
			void enableMouseInput(bool enable);
			// geometries
			HitTestResult hitTest(const graphics::NativePoint& pt) const;
			graphics::NativeRectangle textAreaAllocationRectangle() const /*throw()*/;
			graphics::NativeRectangle textAreaContentRectangle() const /*throw()*/;

		protected:
			virtual void doBeep() /*throw()*/;
			virtual void drawIndicatorMargin(Index line, graphics::Context& context, const graphics::NativeRectangle& rect);

			// helpers
		private:
			graphics::Scalar inlineProgressionOffsetInViewport() const;
			void initialize(const TextViewer* other);
			graphics::Scalar mapLineLayoutIpdToViewport(Index line, graphics::Scalar ipd) const;
//			graphics::Scalar mapLineToViewportBpd(Index line, bool fullSearch) const;
			graphics::Scalar mapViewportIpdToLineLayout(Index line, graphics::Scalar ipd) const;
			void repaintRuler();
			void updateScrollBars();

			// protected interfaces
		protected:
			// CaretListener (overridable)
			virtual void caretMoved(const Caret& self, const kernel::Region& oldRegion);
			// CaretStateListener (overridable)
			virtual void matchBracketsChanged(const Caret& self,
				const boost::optional<std::pair<kernel::Position, kernel::Position>>& oldPair,
				bool outsideOfView);
			virtual void overtypeModeChanged(const Caret& self);
			virtual void selectionShapeChanged(const Caret& self);
		private:
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32)
			// base.Widget
			void provideClassInformation(ClassInformation& classInformation) const;
			std::basic_string<WCHAR> provideClassName() const;
#endif	// defined(ASCENSION_WINDOW_SYSTEM_WIN32)
			// kernel.DocumentListener
			void documentAboutToBeChanged(const kernel::Document& document);
			void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change);
			// kernel.DocumentRollbackListener
			void documentUndoSequenceStarted(const kernel::Document& document);
			void documentUndoSequenceStopped(const kernel::Document& document, const kernel::Position& resultPosition);
			// graphics.font.DefaultFontListener
			void defaultFontChanged() /*throw()*/;
			// graphics.font.VisualLinesListener
			void visualLinesDeleted(const Range<Index>& lines, Index sublines, bool longestLineChanged) /*throw()*/;
			void visualLinesInserted(const Range<Index>& lines) /*throw()*/;
			void visualLinesModified(const Range<Index>& lines,
				SignedIndex sublinesDifference, bool documentChanged, bool longestLineChanged) /*throw()*/;
			// graphics.font.TextViewportListener
			void viewportBoundsInViewChanged(const graphics::NativeRectangle& oldBounds) /*throw()*/;
			void viewportScrollPositionChanged(
				const presentation::AbstractTwoAxes<graphics::font::TextViewport::SignedScrollOffset>& offsets,
				const graphics::font::VisualLine& oldLine,
				graphics::font::TextViewport::ScrollOffset oldInlineProgressionOffset) /*throw()*/;
			// graphics.font.ComputedWritingModeListener
			void computedWritingModeChanged(const presentation::WritingMode& used);
			// detail.PointCollection<VisualPoint>
			void addNewPoint(VisualPoint& point) {points_.insert(&point);}
			void removePoint(VisualPoint& point) {points_.erase(&point);}

		private:
			// platform-dependent events
#if defined(ASCENSION_WINDOW_SYSTEM_GTKMM)
			bool on_button_press_event(GdkEventButton* event);
			bool on_button_release_event(GdkEventButton* event);
			bool on_configure_event(GdkEventConfigure* event);
			bool on_draw(const Cairo::RefPtr<Cairo::Context>& context);
			bool on_focus_in_event(GdkEventFocus* event);
			bool on_focus_out_event(GdkEventFocus* event);
			bool on_grab_broken_event(GdkEventGrabBroken* event);
			bool on_grab_focus();
			bool on_key_press_event(GdkEventKey* event);
			bool on_key_release(GdkEventKey* event);
			bool on_motion_notify_event(GdkEventMotion* event);
			bool on_scroll_event(GdkEventScroll* event);
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
			void contextMenuEvent(QContextMenuEvent* event);
			void focusInEvent(QFocusEvent* event);
			void focusOutEvent(QFocusEvent* event);
			void keyPressEvent(QKeyEvent* event);
			void keyReleaseEvent(QKeyEvent* event);
			void mouseDoubleClickEvent(QMouseEvent* event);
			void mouseMoveEvent(QMouseEvent* event);
			void mousePressEvent(QMouseEvent* event);
			void mouseReleaseEvent(QMouseEvent* event);
			void paintEvent(QPaintEvent* event);
			void resizeEvent(QResizeEvent* event);
			void wheelEvent(QWheelEvent* event);
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
			void onCaptureChanged(const win32::Handle<HWND>& newWindow, bool& consumed);
			void onCommand(WORD id, WORD notifyCode, const win32::Handle<HWND>& control, bool& consumed);
			void onDestroy(bool& consumed);
			void onEraseBkgnd(const win32::Handle<HDC>& dc, bool& consumed);
			const win32::Handle<HFONT>& onGetFont();
			void onHScroll(UINT sbCode, UINT pos, const win32::Handle<HWND>& scrollBar);
			bool onNcCreate(CREATESTRUCTW& cs);
			void onNotify(int id, NMHDR& nmhdr, bool& consumed);
			void onSetCursor(const win32::Handle<HWND>& window, UINT hitTest, UINT message, bool& consumed);
			void onStyleChanged(int type, const STYLESTRUCT& style);
			void onStyleChanging(int type, STYLESTRUCT& style);
			void onSysColorChange();
			void onThemeChanged();
			void onTimer(UINT_PTR eventId, TIMERPROC timerProc);
			void onVScroll(UINT sbCode, UINT pos, const win32::Handle<HWND>& scrollBar);
			LRESULT processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed);
			// IDropTarget
			STDMETHODIMP DragEnter(IDataObject* data, DWORD keyState, POINTL location, DWORD* effect);
			STDMETHODIMP DragOver(DWORD keyState, POINTL location, DWORD* effect);
			STDMETHODIMP DragLeave();
			STDMETHODIMP Drop(IDataObject* data, DWORD keyState, POINTL location, DWORD* effect);
#endif
			// event handlers
		private:
			void aboutToLoseFocus();
			void focusGained();
			void keyPressed(const widgetapi::KeyInput& input);
			void keyReleased(const widgetapi::KeyInput& input);
			void mouseDoubleClicked(const widgetapi::MouseButtonInput& input);
			void mouseMoved(const widgetapi::LocatedUserInput& input);
			void mousePressed(const widgetapi::MouseButtonInput& input);
			void mouseReleased(const widgetapi::MouseButtonInput& input);
			void mouseWheelChanged(const widgetapi::MouseWheelInput& input);
			void paint(graphics::PaintContext& context);
			void resized(State state, const graphics::NativeSize& newSize);
			void showContextMenu(const widgetapi::LocatedUserInput& input, bool byKeyboard);

			// internal classes
		private:
			class CursorVanisher {
			public:
				CursorVanisher() /*throw()*/;
				~CursorVanisher();
				void install(TextViewer& viewer);
				void restore();
				void vanish();
				bool vanished() const;
			private:
				TextViewer* viewer_;
				bool vanished_;
			} cursorVanisher_;

			// enumerations
		private:
			// timer identifiers
			enum {
				TIMERID_CALLTIP,	// interval for tooltip
//				TIMERID_LINEPARSE
			};

			// data members
		private:
			// big stars
			presentation::Presentation& presentation_;
			std::unique_ptr<Caret> caret_;
			std::unique_ptr<Renderer> renderer_;
			Configuration configuration_;
			std::set<VisualPoint*> points_;
			HWND toolTip_;
			Char* tipText_;
			// strategies and listeners
			std::shared_ptr<MouseInputStrategy> mouseInputStrategy_;
			std::shared_ptr<widgetapi::DropTarget> dropTargetHandler_;
			detail::Listeners<DisplaySizeListener> displaySizeListeners_;
			detail::Listeners<ViewportListener> viewportListeners_;
			std::unique_ptr<detail::RulerPainter> rulerPainter_;
			std::unique_ptr<contentassist::ContentAssistant> contentAssistant_;
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
			class AccessibleProxy;
			AccessibleProxy* accessibleProxy_;
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY

			// modes
			struct ModeState {
#ifndef ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER
				bool activeInputMethodEnabled;	// true if uses Global IME (deprecated)
#endif // !ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER

				ModeState() /*throw()*/
#ifndef ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER
					: activeInputMethodEnabled(true)
#endif // !ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER
				{}
			} modeState_;

			// scroll information
			struct Scrolls {
//				unsigned long horizontalRate, verticalRate;	// 最小スクロール量が何文字 (何行) に相当するか (普通は 1)
				bool changed;
				Scrolls() /*throw()*/ : /*horizontalRate(1), verticalRate(1), */changed(false) {}
				void resetBars(const TextViewer& viewer, char bars, bool pageSizeChanged) /*throw()*/;
			} scrolls_;

			// freeze information
			class FreezeRegister {
			public:
				FreezeRegister() /*throw()*/ : count_(0) {
					freeze();
					unfreeze();
				}
				void freeze() /*throw()*/ {++count_;}
				void addLinesToRedraw(const Range<Index>& lines) {
					assert(isFrozen());
					linesToRedraw_ = merged(linesToRedraw_, lines);
				}
				bool isFrozen() const /*throw()*/ {return count_ != 0;}
				const Range<Index>& linesToRedraw() const /*throw()*/ {return linesToRedraw_;}
				void resetLinesToRedraw(const Range<Index>& lines) {
					assert(isFrozen());
					linesToRedraw_ = lines;
				}
				Range<Index> unfreeze() {
					assert(isFrozen());
					const Range<Index> temp(linesToRedraw());
					--count_;
					linesToRedraw_ = Range<Index>(0, 0);
					return temp;
				}
			private:
				unsigned long count_;
				Range<Index> linesToRedraw_;
			} freezeRegister_;

			// input state
			unsigned long mouseInputDisabledCount_;
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
			win32::com::SmartPointer<IDropTargetHelper> dropTargetHelper_;
			win32::com::SmartPointer<IDataObject> draggingData_;
#endif // ASCENSION_WINDOW_SYSTEM_WIN32

			friend class VisualPoint;
			friend class VirtualBox;
			friend class detail::RulerPainter;
			friend class CaretShapeUpdater;
			friend class Renderer;
		};

		// the documentation is viewer.cpp
		class AutoFreeze {
			ASCENSION_NONCOPYABLE_TAG(AutoFreeze);
		public:
			explicit AutoFreeze(TextViewer* textViewer);
			~AutoFreeze() /*throw()*/;
		private:
			TextViewer* const textViewer_;
		};

		/// Highlights the line on which the caret is put.
		class CurrentLineHighlighter : public presentation::TextLineColorDirector,
				public CaretListener, public CaretStateListener, public kernel::PointLifeCycleListener {
			ASCENSION_NONCOPYABLE_TAG(CurrentLineHighlighter);
		public:
			// constant
			static const TextLineColorDirector::Priority LINE_COLOR_PRIORITY;
			// constructors
			CurrentLineHighlighter(Caret& caret,
				const graphics::Color& foreground, const graphics::Color& background);
			~CurrentLineHighlighter() /*throw()*/;
			// attributes
			const graphics::Color& background() const /*throw()*/;
			const graphics::Color& foreground() const /*throw()*/;
			void setBackground(const graphics::Color& color) /*throw()*/;
			void setForeground(const graphics::Color& color) /*throw()*/;
		private:
			// presentation.TextLineColorDirector
			TextLineColorDirector::Priority queryLineColors(Index line,
				graphics::Color& foreground, graphics::Color& background) const;
			// CaretListener
			void caretMoved(const Caret& self, const kernel::Region& oldRegion);
			// CaretStateListener
			void matchBracketsChanged(const Caret& self,
				const boost::optional<std::pair<kernel::Position, kernel::Position>>& oldPair,
				bool outsideOfView);
			void overtypeModeChanged(const Caret& self);
			void selectionShapeChanged(const Caret& self);
			// kernel.PointLifeCycleListener
			void pointDestroyed();
		private:
			Caret* caret_;
			graphics::Color foreground_, background_;
		};

		/// Provides the utility stuffs for viewers.
		namespace utils {
			void closeCompletionProposalsPopup(TextViewer& viewer) /*throw()*/;
			const presentation::hyperlink::Hyperlink* getPointedHyperlink(const TextViewer& viewer, const kernel::Position& at);
			bool isRulerLeftAligned(const TextViewer& viewer);
			void toggleOrientation(TextViewer& viewer) /*throw()*/;
		} // namespace utils


// inlines ////////////////////////////////////////////////////////////////////////////////////////

/**
 * Returns @c true if the viewer allows the mouse operations.
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
 * @see #rulerConfiguration, #setConfiguration
 */
inline const TextViewer::Configuration& TextViewer::configuration() const /*throw()*/ {return configuration_;}

/// Returns the content assistant or @c null if not registered.
inline contentassist::ContentAssistant* TextViewer::contentAssistant() const /*throw()*/ {return contentAssistant_.get();}

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
 * @param enable Set @c false to increment the disabled count, @c true to decrement
 * @see #allowsMouseInput
 */
inline void TextViewer::enableMouseInput(bool enable) {
	if(mouseInputDisabledCount_ != 0 || !enable) mouseInputDisabledCount_ += !enable ? 1 : -1;}

#ifndef ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER
/// Returns @c true if Global IME is enabled.
inline bool TextViewer::isActiveInputMethodEnabled() const /*throw()*/ {return modeState_.activeInputMethodEnabled;}
#endif // !ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER

/// Returns @c true if the viewer is frozen.
inline bool TextViewer::isFrozen() const /*throw()*/ {return freezeRegister_.isFrozen();}

/// Returns the presentation object. 
inline presentation::Presentation& TextViewer::presentation() /*throw()*/ {return presentation_;}

/// Returns the presentation object. 
inline const presentation::Presentation& TextViewer::presentation() const /*throw()*/ {return presentation_;}

/**
 * Returns the ratio to vertical/horizontal scroll amount of line/column numbers.
 * @param horizontal Set @c true for horizontal, @c false for vertical
 * @return The rate
 */
inline unsigned long TextViewer::scrollRate(bool horizontal) const /*throw()*/ {
	return 1/*horizontal ? scrollInfo_.horizontal.rate : scrollInfo_.vertical.rate*/;}

/// Returns the text renderer.
inline TextViewer::Renderer& TextViewer::textRenderer() /*throw()*/ {return *renderer_;}

/// Returns the text renderer.
inline const TextViewer::Renderer& TextViewer::textRenderer() const /*throw()*/ {return *renderer_;}

/**
 * Returns the ruler's configuration.
 * @see #configuration, #setConfiguration
 */
inline const RulerConfiguration& TextViewer::rulerConfiguration() const /*throw()*/ {return rulerPainter_->configuration();}

}} // namespace ascension.viewers

#endif // !ASCENSION_VIEWER_HPP
