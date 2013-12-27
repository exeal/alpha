/**
 * @file viewer.hpp
 * This header defines several visual presentation classes.
 * @author exeal
 * @date 2003-2006 was EditView.h
 * @date 2006-2013
 */

#ifndef ASCENSION_VIEWER_HPP
#define ASCENSION_VIEWER_HPP

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION, ...
#include <ascension/graphics/font/text-renderer.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/kernel/point.hpp>
#include <ascension/presentation/writing-mode.hpp>
#include <ascension/viewer/caret-observers.hpp>
#include <ascension/viewer/ruler.hpp>
#include <ascension/viewer/viewer-observers.hpp>
#include <ascension/viewer/widgetapi/scrollable.hpp>
#include <algorithm>
#include <array>
#include <set>
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
#	include <glibmm/property.h>
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
#	include <ascension/win32/com/smart-pointer.hpp>
#	include <ascension/win32/com/unknown-impl.hpp>
#	include <shlobj.h>	// IDropTargetHelper

#	ifndef ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER
#		include <dimm.h>
#	endif // !ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER

#	ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
#		include <Oleacc.h>
#		include <MSAAtext.h>
#	endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY

#	ifndef ASCENSION_NO_TEXT_OBJECT_MODEL
#		include <tom.h>
#	endif // !ASCENSION_NO_TEXT_OBJECT_MODEL
#endif


namespace ascension {
	namespace viewers {
		class VirtualBox;
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

#if defined(ASCENSION_WINDOW_SYSTEM_WIN32) && !defined(ASCENSION_NO_ACTIVE_ACCESSIBILITY)
	namespace detail {
		class AbstractAccessibleProxy : public IAccessible {
		public:
			virtual ~AbstractAccessibleProxy() BOOST_NOEXCEPT {}
			virtual void dispose() = 0;
		};
	}
#endif

#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
	namespace detail {
		class TextViewerScrollableProperties : public Gtk::Widget {
		protected:
			TextViewerScrollableProperties();
		private:
			Glib::Property<Glib::RefPtr<Gtk::Adjustment>> horizontalAdjustment_, verticalAdjustment_;
			Glib::Property<Gtk::ScrollablePolicy> horizontalScrollPolicy_, verticalScrollPolicy_;
		};
	}
#endif

	namespace viewers {
		class TextViewer :
				// note:
				// Gtk.TextView inherits Gtk.Container (which inherits Gtk.Widget) and Gtk.Scrollable.
				// QPlainTextEdit and QTextEdit inherit QAbstractScrollArea.
				// NSTextView inherits NSText (which inherits NSView).
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
				public detail::TextViewerScrollableProperties, public Gtk::Scrollable,
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
				public graphics::font::TextViewportListener, public graphics::font::ComputedBlockFlowDirectionListener,
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
				boost::optional<graphics::Color> selectionForeground;
				/// Background color of active selected text. Standard setting is @c COLOR_HIGHLIGHT.
				boost::optional<graphics::Color> selectionBackground;
				/// Foreground color of inactive selected text. Standard setting is @c COLOR_INACTIVECAPTIONTEXT.
				boost::optional<graphics::Color> inactiveSelectionForeground;
				/// Background color of inactive selected text. Standard setting is @c COLOR_INACTIVECAPTION.
				boost::optional<graphics::Color> inactiveSelectionBackground;
				/// Foreground color of the inaccessible area. Standard setting is @c COLOR_GRAYTEXT.
				boost::optional<graphics::Color> restrictionForeground;
				/// Background color of the inaccessible area. Standard setting is @c color.background.
				boost::optional<graphics::Color> restrictionBackground;
				/// The reading direction of UI.
				presentation::ReadingDirection readingDirection;
				/// Set @c true to vanish the cursor when the user types. Default value depends on system setting.
				bool vanishesCursor;
				/// Set @c true to use also Rich Text Format for clipboard operations. Default value is @c false.
				bool usesRichTextClipboardFormat;

				Configuration() BOOST_NOEXCEPT;
			};

			/// Implementation of @c graphics#font#TextRenderer for @c TextViewer.
			class Renderer : public graphics::font::TextRenderer {
				ASCENSION_UNASSIGNABLE_TAG(Renderer);
			public:
				explicit Renderer(TextViewer& viewer);
				Renderer(const Renderer& other, TextViewer& viewer);
				void displayShapingControls(bool display);
				bool displaysShapingControls() const BOOST_NOEXCEPT;
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
				void rewrapAtWindowEdge();
#endif // ASCENSION_ABANDONED_AT_VERSION_08
				// TextRenderer
				std::unique_ptr<const graphics::font::TextLayout> createLineLayout(Index line) const;
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
				graphics::Scalar width() const BOOST_NOEXCEPT;
#endif // ASCENSION_ABANDONED_AT_VERSION_08
			private:
				TextViewer& viewer_;
				bool displaysShapingControls_;
			};

			explicit TextViewer(presentation::Presentation& presentation);
			TextViewer(const TextViewer& other);
			virtual ~TextViewer();

			/// @name Listeners and Strategies
			/// @{
			void addDisplaySizeListener(DisplaySizeListener& listener);
			void addViewportListener(ViewportListener& listener);
			void removeDisplaySizeListener(DisplaySizeListener& listener);
			void removeViewportListener(ViewportListener& listener);
			void setMouseInputStrategy(std::shared_ptr<MouseInputStrategy> newStrategy);
			/// @}

			/// @name General Attributes
			/// @{
			const Configuration& configuration() const BOOST_NOEXCEPT;
			kernel::Document& document();
			const kernel::Document& document() const;
			presentation::Presentation& presentation() BOOST_NOEXCEPT;
			const presentation::Presentation& presentation() const BOOST_NOEXCEPT;
			const RulerStyles& declaredRulerStyles() const BOOST_NOEXCEPT;
			unsigned long scrollRate(bool horizontal) const BOOST_NOEXCEPT;
			void setConfiguration(const Configuration* general,
				std::shared_ptr<const RulerStyles> ruler, bool synchronizeUI);
			Renderer& textRenderer() BOOST_NOEXCEPT;
			const Renderer& textRenderer() const BOOST_NOEXCEPT;
			/// @}

			/// @name Caret
			/// @{
			Caret& caret() BOOST_NOEXCEPT;
			const Caret& caret() const BOOST_NOEXCEPT;
			/// @}

#if defined(ASCENSION_WINDOW_SYSTEM_WIN32) && !defined(ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER)
			/// @name Global IME (Only Windows)
			/// @{
			void enableActiveInputMethod(bool enable = true) BOOST_NOEXCEPT;
			bool isActiveInputMethodEnabled() const BOOST_NOEXCEPT;
			/// @}

#endif
			/// @name Other User Interface
			/// @{
			void beep() BOOST_NOEXCEPT;
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32) && !defined(ASCENSION_NO_ACTIVE_ACCESSIBILITY)
			HRESULT accessibleObject(IAccessible*& acc) const BOOST_NOEXCEPT;
#endif
			void hideToolTip();
			void showToolTip(const String& text, unsigned long timeToWait = -1, unsigned long timeRemainsVisible = -1);
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32) && !defined(ASCENSION_NO_TEXT_SERVICES_FRAMEWORK)
			HRESULT startTextServices();
#endif
			/// @}

			/// @name Content Assist
			/// @{
			contentassist::ContentAssistant* contentAssistant() const BOOST_NOEXCEPT;
			void setContentAssistant(std::unique_ptr<contentassist::ContentAssistant> newContentAssistant) BOOST_NOEXCEPT;
			/// @}

			/// @name Redraw
			/// @{
			void redrawLine(Index line, bool following = false);
			void redrawLines(const boost::integer_range<Index>& lines);
			/// @}

			/// @name Freeze
			/// @{
			void freeze();
			bool isFrozen() const BOOST_NOEXCEPT;
			void unfreeze();
			/// @}

			/// @name Mouse Input
			/// @{
			bool allowsMouseInput() const BOOST_NOEXCEPT;
			void enableMouseInput(bool enable);
			/// @}

			/// @name Geometries
			/// @{
			HitTestResult hitTest(const graphics::Point& pt) const;
			graphics::Rectangle textAreaAllocationRectangle() const BOOST_NOEXCEPT;
			graphics::Rectangle textAreaContentRectangle() const BOOST_NOEXCEPT;
			/// @}

		protected:
			virtual void doBeep() BOOST_NOEXCEPT;
			virtual void drawIndicatorMargin(Index line, graphics::PaintContext& context, const graphics::Rectangle& rect);

			// helpers
		private:
			graphics::Scalar inlineProgressionOffsetInViewport() const;
			void initialize(const TextViewer* other);
			void initializeNativeObjects(const TextViewer* other);
			void repaintRuler();
			void updateScrollBars(
				const presentation::AbstractTwoAxes<bool>& positions,
				const presentation::AbstractTwoAxes<bool>& properties);

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
			void defaultFontChanged() BOOST_NOEXCEPT;
			// graphics.font.VisualLinesListener
			void visualLinesDeleted(const boost::integer_range<Index>& lines, Index sublines, bool longestLineChanged) BOOST_NOEXCEPT;
			void visualLinesInserted(const boost::integer_range<Index>& lines) BOOST_NOEXCEPT;
			void visualLinesModified(const boost::integer_range<Index>& lines,
				SignedIndex sublinesDifference, bool documentChanged, bool longestLineChanged) BOOST_NOEXCEPT;
			// graphics.font.TextViewportListener
			void viewportBoundsInViewChanged(const graphics::Rectangle& oldBounds) BOOST_NOEXCEPT;
			void viewportScrollPositionChanged(
				const presentation::AbstractTwoAxes<graphics::font::TextViewport::ScrollOffset>& positionsBeforeScroll,
				const graphics::font::VisualLine& firstVisibleLineBeforeScroll) BOOST_NOEXCEPT;
			void viewportScrollPropertiesChanged(
				const presentation::AbstractTwoAxes<bool>& changedDimensions) BOOST_NOEXCEPT;
			// graphics.font.ComputedBlockFlowDirectionListener
			void computedBlockFlowDirectionChanged(presentation::BlockFlowDirection used);
			// detail.PointCollection<VisualPoint>
			void addNewPoint(VisualPoint& point) {points_.insert(&point);}
			void removePoint(VisualPoint& point) {points_.erase(&point);}

			// platform-dependent events and other overrides
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
		protected:
			virtual void get_preferred_height_for_width_vfunc(int width, int& minimumHeight, int& naturalHeight) const;
			virtual void get_preferred_height_vfunc(int& minimumHeight, int& naturalHeight) const;
			virtual void get_preferred_width_vfunc(int& minimumWidth, int& naturalWidth) const;
			virtual void get_preferred_width_for_height_vfunc(int height, int& minimumWidth, int& naturalWidth) const;
			virtual Gtk::SizeRequestMode get_request_mode_vfunc() const;
			virtual void on_realize();
			virtual void on_size_allocate(Gtk::Allocation& allocation);
			virtual void on_unrealize();
		private:
			bool on_button_press_event(GdkEventButton* event);
			bool on_button_release_event(GdkEventButton* event);
			bool on_configure_event(GdkEventConfigure* event);
			void on_drag_leave(const Glib::RefPtr<Gdk::DragContext>& context, guint time);
			bool on_drag_motion(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time);
			bool on_drag_drop(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time);
			bool on_draw(const Cairo::RefPtr<Cairo::Context>& context);
			bool on_focus_in_event(GdkEventFocus* event);
			bool on_focus_out_event(GdkEventFocus* event);
			bool on_grab_broken_event(GdkEventGrabBroken* event);
			void on_grab_focus();
			bool on_key_press_event(GdkEventKey* event);
			bool on_key_release(GdkEventKey* event);
			bool on_motion_notify_event(GdkEventMotion* event);
			bool on_scroll_event(GdkEventScroll* event);
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
		private:
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
			void timerEvent(QTimerEvent* event);
			void wheelEvent(QWheelEvent* event);
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
		private:
			void onCaptureChanged(const win32::Handle<HWND>::Type& newWindow, bool& consumed);
			void onCommand(WORD id, WORD notifyCode, const win32::Handle<HWND>::Type& control, bool& consumed);
			void onDestroy(bool& consumed);
			void onEraseBkgnd(const win32::Handle<HDC>::Type& dc, bool& consumed);
			const win32::Handle<HFONT>::Type onGetFont() const;
			void onHScroll(UINT sbCode, UINT pos, const win32::Handle<HWND>::Type& scrollBar);
			bool onNcCreate(CREATESTRUCTW& cs);
			void onNotify(int id, NMHDR& nmhdr, bool& consumed);
			void onSetCursor(const win32::Handle<HWND>::Type& window, UINT hitTest, UINT message, bool& consumed);
			void onStyleChanged(int type, const STYLESTRUCT& style);
			void onStyleChanging(int type, STYLESTRUCT& style);
			void onSysColorChange();
			void onThemeChanged();
			void onTimer(UINT_PTR eventId, TIMERPROC timerProc);
			void onVScroll(UINT sbCode, UINT pos, const win32::Handle<HWND>::Type& scrollBar);
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
			void resized(const graphics::Dimension& newSize);
			void showContextMenu(const widgetapi::LocatedUserInput& input, bool byKeyboard);

			// internal classes
		private:
			class CursorVanisher {
			public:
				CursorVanisher() BOOST_NOEXCEPT;
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
#ifdef ASCENSION_WINDOW_SYSTEM_GTK
			Glib::RefPtr<Gdk::Window> textAreaWindow_, rulerWindow_;
#endif	// ASCENSION_WINDOW_SYSTEM_GTK
			presentation::Presentation& presentation_;
			std::unique_ptr<Caret> caret_;
			std::unique_ptr<Renderer> renderer_;
			Configuration configuration_;
			std::set<VisualPoint*> points_;
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
			win32::Handle<HWND>::Type toolTip_;
			std::basic_string<WCHAR> tipText_;
#endif // ASCENSION_WINDOW_SYSTEM_WIN32
			// strategies and listeners
			std::shared_ptr<MouseInputStrategy> mouseInputStrategy_;
			std::shared_ptr<widgetapi::DropTarget> dropTargetHandler_;
			detail::Listeners<DisplaySizeListener> displaySizeListeners_;
			detail::Listeners<ViewportListener> viewportListeners_;
			std::unique_ptr<detail::RulerPainter> rulerPainter_;
			std::unique_ptr<contentassist::ContentAssistant> contentAssistant_;
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32) && !defined(ASCENSION_NO_ACTIVE_ACCESSIBILITY)
			win32::com::SmartPointer<detail::AbstractAccessibleProxy> accessibleProxy_;
#endif

			// modes
			struct ModeState {
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32) && !defined(ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER)
				bool activeInputMethodEnabled;	// true if uses Global IME (deprecated)
#endif

				ModeState() BOOST_NOEXCEPT
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32) && !defined(ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER)
					: activeInputMethodEnabled(true)
#endif
				{}
			} modeState_;
#if 0
			// scroll information
			struct Scrolls {
//				unsigned long horizontalRate, verticalRate;	// 最小スクロール量が何文字 (何行) に相当するか (普通は 1)
				bool changed;
				Scrolls() BOOST_NOEXCEPT : /*horizontalRate(1), verticalRate(1), */changed(false) {}
				void resetBars(const TextViewer& viewer, char bars, bool pageSizeChanged) BOOST_NOEXCEPT;
			} scrolls_;
#endif
			// freeze information
			class FreezeRegister {
			public:
				FreezeRegister() BOOST_NOEXCEPT;
				void freeze();
				void addLinesToRedraw(const boost::integer_range<Index>& lines);
				bool isFrozen() const BOOST_NOEXCEPT {return count_ != 0;}
				const boost::integer_range<Index>& linesToRedraw() const BOOST_NOEXCEPT {return linesToRedraw_;}
				void resetLinesToRedraw(const boost::integer_range<Index>& lines);
				boost::integer_range<Index> thaw();
			private:
				boost::value_initialized<std::size_t> count_;
				boost::integer_range<Index> linesToRedraw_;
			} freezeRegister_;

			// input state
#ifndef ASCENSION_WINDOW_SYSTEM_WIN32
			bool blinkingCaretIsVisible_;
			Timer caretBlinkingTimer_;
#endif	// !ASCENSION_WINDOW_SYSTEM_WIN32
			unsigned long mouseInputDisabledCount_;
#ifdef ASCENSION_WINDOW_SYSTEM_GTK
			std::shared_ptr<GtkIMContext> inputMethodContext_;
#endif
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
			win32::com::SmartPointer<IDropTargetHelper> dropTargetHelper_;
			win32::com::SmartPointer<IDataObject> draggingData_;
#else
//			const NativeMimeData* draggingData_;
#endif

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
			~AutoFreeze() BOOST_NOEXCEPT;
		private:
			TextViewer* const textViewer_;
		};

		/// Provides the utility stuffs for viewers.
		namespace utils {
			void closeCompletionProposalsPopup(TextViewer& viewer) BOOST_NOEXCEPT;
			const presentation::hyperlink::Hyperlink* getPointedHyperlink(const TextViewer& viewer, const kernel::Position& at);
			bool isRulerLeftAligned(const TextViewer& viewer);
			void toggleOrientation(TextViewer& viewer) BOOST_NOEXCEPT;
		} // namespace utils


		// inline implementation //////////////////////////////////////////////////////////////////

		/**
		 * Returns @c true if the viewer allows the mouse operations.
		 * @see #enableMouseInput
		 */
		inline bool TextViewer::allowsMouseInput() const BOOST_NOEXCEPT {return mouseInputDisabledCount_ == 0;} 

		/// Informs the end user of <strong>safe</strong> error.
		inline void TextViewer::beep() BOOST_NOEXCEPT {doBeep();}
		
		/// Returns the caret.
		inline Caret& TextViewer::caret() BOOST_NOEXCEPT {return *caret_;}
		
		/// Returns the caret.
		inline const Caret& TextViewer::caret() const BOOST_NOEXCEPT {return *caret_;}

		/**
		 * Returns the general configuration.
		 * @see #rulerConfiguration, #setConfiguration
		 */
		inline const TextViewer::Configuration& TextViewer::configuration() const BOOST_NOEXCEPT {
			return configuration_;
		}
		
		/// Returns the content assistant or @c null if not registered.
		inline contentassist::ContentAssistant* TextViewer::contentAssistant() const BOOST_NOEXCEPT {
			return contentAssistant_.get();
		}
		
		/**
		 * Returns the ruler's declared styles.
		 * @see #configuration, #setConfiguration
		 */
		inline const RulerStyles& TextViewer::declaredRulerStyles() const BOOST_NOEXCEPT {
			return rulerPainter_->declaredStyles();
		}
		
		/// Returns the document.
		inline kernel::Document& TextViewer::document() {return presentation_.document();}
		
		/// Returns the document.
		inline const kernel::Document& TextViewer::document() const {return presentation_.document();}
		
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32) && !defined(ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER)
		/**
		 * Enables Global IME. This setting effects under only Windows NT 4.0. Otherwise, Ascension
		 * does not use Global IME.
		 * @deprecated 0.8
		 */
		inline void TextViewer::enableActiveInputMethod(bool enable /* = true */) BOOST_NOEXCEPT {
			modeState_.activeInputMethodEnabled = enable;
		}
#endif

		/**
		 * Enables/disables the mouse operations.
		 *
		 * A @c TextViewer has a disabled count for the mouse input. If this value is not zero, any
		 * mouse inputs are not allowed.
		 *
		 * These is no way to disable the scroll bars.
		 * @param enable Set @c false to increment the disabled count, @c true to decrement
		 * @see #allowsMouseInput
		 */
		inline void TextViewer::enableMouseInput(bool enable) {
			if(mouseInputDisabledCount_ != 0 || !enable)
				mouseInputDisabledCount_ += !enable ? 1 : -1;
		}
		
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32) && !defined(ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER)
		/// Returns @c true if Global IME is enabled.
		inline bool TextViewer::isActiveInputMethodEnabled() const BOOST_NOEXCEPT {
			return modeState_.activeInputMethodEnabled;
		}
#endif
		
		/// Returns @c true if the viewer is frozen.
		inline bool TextViewer::isFrozen() const BOOST_NOEXCEPT {
			return freezeRegister_.isFrozen();
		}
		
		/// Returns the presentation object. 
		inline presentation::Presentation& TextViewer::presentation() BOOST_NOEXCEPT {
			return presentation_;
		}
		
		/// Returns the presentation object. 
		inline const presentation::Presentation& TextViewer::presentation() const BOOST_NOEXCEPT {
			return presentation_;
		}
		
		/**
		 * Returns the ratio to vertical/horizontal scroll amount of line/column numbers.
		 * @param horizontal Set @c true for horizontal, @c false for vertical
		 * @return The rate
		 */
		inline unsigned long TextViewer::scrollRate(bool horizontal) const BOOST_NOEXCEPT {
			return 1/*horizontal ? scrollInfo_.horizontal.rate : scrollInfo_.vertical.rate*/;
		}
		
		/// Returns the text renderer.
		inline TextViewer::Renderer& TextViewer::textRenderer() BOOST_NOEXCEPT {
			return *renderer_;
		}
		
		/// Returns the text renderer.
		inline const TextViewer::Renderer& TextViewer::textRenderer() const BOOST_NOEXCEPT {
			return *renderer_;
		}
	}
} // namespace ascension.viewers

#endif // !ASCENSION_VIEWER_HPP
