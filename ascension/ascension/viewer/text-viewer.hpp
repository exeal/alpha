/**
 * @file text-viewer.hpp
 * This header defines several visual presentation classes.
 * @author exeal
 * @date 2003-2006 was EditView.h
 * @date 2006-2015
 */

#ifndef ASCENSION_TEXT_VIEWER_HPP
#define ASCENSION_TEXT_VIEWER_HPP
#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION, ...
#include <ascension/corelib/signals.hpp>
#include <ascension/graphics/color.hpp>
#include <ascension/kernel/document-observers.hpp>
#include <ascension/kernel/point.hpp>
#include <ascension/presentation/flow-relative-two-axes.hpp>
#include <ascension/presentation/writing-mode.hpp>
#include <ascension/viewer/mouse-input-strategy.hpp>
#include <ascension/viewer/text-viewer-component.hpp>
#include <ascension/viewer/widgetapi/event/key-input.hpp>
#include <ascension/viewer/widgetapi/event/mouse-button-input.hpp>
#include <ascension/viewer/widgetapi/event/mouse-wheel-input.hpp>
#include <ascension/viewer/widgetapi/scrollable.hpp>
#include <boost/utility/value_init.hpp>
#include <algorithm>
#include <array>
#include <set>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	if !defined(ASCENSION_PIXELFUL_SCROLL_IN_BPD)
#		include <ascension/graphics/physical-two-axes.hpp>
#	endif
#	include <glibmm/property.h>
#	include <gtkmm/container.h>
#	define ASCENSION_TEXT_VIEWER_IS_GTK_SCROLLABLE
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
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
#if !ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <ascension/corelib/timer.hpp>
#endif	// !ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)


namespace ascension {
	namespace contentassist {
		class ContentAssistant;
	}

	namespace presentation {
		struct ComputedTextToplevelStyle;
		class DeclaredTextToplevelStyle;
	}

	namespace viewer {
		class Caret;
		class CaretShaper;
		class TextArea;
		class TextViewer;
		class VirtualBox;
		class VisualPoint;

		namespace detail {
			/// @internal Implementes "Mouse Vanish" feature.
			template<typename Derived>
			class MouseVanish {
			protected:
				MouseVanish() BOOST_NOEXCEPT;
				virtual ~MouseVanish();
				void hideCursor();
				bool hidesCursor() const BOOST_NOEXCEPT;
				void restoreHiddenCursor();
			private:
				bool hidden_;
			};

#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32) && !defined(ASCENSION_NO_ACTIVE_ACCESSIBILITY)
			class AbstractAccessibleProxy : public IAccessible {
			public:
				virtual ~AbstractAccessibleProxy() BOOST_NOEXCEPT {}
				virtual void dispose() = 0;
			};
#endif

#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			std::shared_ptr<GtkIMContext> inputMethodContext(TextViewer& textViewer);
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		}

		class TextViewer :
				// note:
				// Gtk.TextView inherits Gtk.Container (which inherits Gtk.Widget) and Gtk.Scrollable.
				// QPlainTextEdit and QTextEdit inherit QAbstractScrollArea.
				// NSTextView inherits NSText (which inherits NSView).
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				public Gtk::Container,
#	ifdef ASCENSION_TEXT_VIEWER_IS_GTK_SCROLLABLE
				public Gtk::Scrollable,
#	endif
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				public QAbstractScrollArea,
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
				public NSView,
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				public win32::CustomControl,
				public win32::com::IUnknownImpl<
					ASCENSION_WIN32_COM_INTERFACE(IDropTarget), win32::com::NoReferenceCounting
				>,
#endif
				public kernel::DocumentListener, public kernel::DocumentRollbackListener,
				protected TextViewerComponent::Locator, protected MouseInputStrategy::TargetLocker,
				private detail::MouseVanish<TextViewer> {
		public:
			/**
			 * A general configuration of the viewer.
			 * @see TextViewer#configurations, TextViewer#setConfigurations
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

			explicit TextViewer(presentation::Presentation& presentation);
			TextViewer(const TextViewer& other);
			virtual ~TextViewer();

			/// @name General Attributes
			/// @{
			const Configuration& configuration() const BOOST_NOEXCEPT;
			kernel::Document& document() BOOST_NOEXCEPT;
			const kernel::Document& document() const BOOST_NOEXCEPT;
			BOOST_CONSTEXPR presentation::Presentation& presentation() BOOST_NOEXCEPT;
			BOOST_CONSTEXPR const presentation::Presentation& presentation() const BOOST_NOEXCEPT;
			unsigned long scrollRate(bool horizontal) const BOOST_NOEXCEPT;
			void setConfiguration(const Configuration& newConfiguration, bool synchronizeUI);
			BOOST_CONSTEXPR TextArea& textArea() BOOST_NOEXCEPT;
			BOOST_CONSTEXPR const TextArea& textArea() const BOOST_NOEXCEPT;
			/// @}

#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32) && !defined(ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER)
			/// @name Global IME (Only Windows)
			/// @{
			void enableActiveInputMethod(bool enable = true) BOOST_NOEXCEPT;
			bool isActiveInputMethodEnabled() const BOOST_NOEXCEPT;
			/// @}

#endif
			/// @name Other User Interface
			/// @{
			void beep() BOOST_NOEXCEPT;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32) && !defined(ASCENSION_NO_ACTIVE_ACCESSIBILITY)
			HRESULT accessibleObject(IAccessible*& acc) const BOOST_NOEXCEPT;
#endif
			void hideToolTip();
			void showToolTip(const String& text, unsigned long timeToWait = -1, unsigned long timeRemainsVisible = -1);
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32) && !defined(ASCENSION_NO_TEXT_SERVICES_FRAMEWORK)
			HRESULT startTextServices();
#endif
			/// @}

			/// @name Content Assist
			/// @{
			contentassist::ContentAssistant* contentAssistant() const BOOST_NOEXCEPT;
			void setContentAssistant(std::unique_ptr<contentassist::ContentAssistant> newContentAssistant) BOOST_NOEXCEPT;
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
			/// @see TextArea
			/// @{
			TextViewerComponent* hitTest(const graphics::Point& location) BOOST_NOEXCEPT;
			virtual const TextViewerComponent* hitTest(const graphics::Point& location) const BOOST_NOEXCEPT;
			/// @}

			/// @name Signals
			/// @{
			typedef boost::signals2::signal<void(const TextViewer&)> FocusChangedSignal;
			typedef boost::signals2::signal<void(const TextViewer&)> FrozenStateChangedSignal;
			SignalConnector<FocusChangedSignal> focusChangedSignal() BOOST_NOEXCEPT;
			SignalConnector<FrozenStateChangedSignal> frozenStateChangedSignal() BOOST_NOEXCEPT;
			/// @}

		protected:
			virtual void doBeep() BOOST_NOEXCEPT;
			virtual void drawIndicatorMargin(Index line, graphics::PaintContext& context, const graphics::Rectangle& rect);
			std::shared_ptr<MouseInputStrategy> mouseInputStrategy(const graphics::Point& p);
			virtual void unfrozen();
			void updateTextAreaAllocationRectangle();
			// TextViewerComponent.Locator
			virtual graphics::Rectangle locateComponent(const TextViewerComponent& component) const override;
			// MouseInputStrategy.TargetLocker
			bool lockMouseInputTarget(std::weak_ptr<MouseInputStrategy> strategy) override;
			void unlockMouseInputTarget(MouseInputStrategy& strategy) BOOST_NOEXCEPT override;

			// helpers
		private:
			void doShowContextMenu(void* nativeEvent);
			graphics::Scalar inlineProgressionOffsetInViewport() const;
			void initialize(const TextViewer* other);
			void initializeGraphics();
			void initializeNativeObjects();
			void initializeNativeWidget();
			void updateScrollBars(
				const presentation::FlowRelativeTwoAxes<bool>& positions,
				const presentation::FlowRelativeTwoAxes<bool>& properties);

		protected:
			/// @ name Overridable Signal Slots
			/// @{
			virtual void computedTextToplevelStyleChanged(
				const presentation::Presentation& presentation,
				const presentation::DeclaredTextToplevelStyle& previouslyDeclared,
				const presentation::ComputedTextToplevelStyle& previouslyComputed);
			/// @}

		private:
			// kernel.DocumentListener
			void documentAboutToBeChanged(const kernel::Document& document) override;
			void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change) override;
			// kernel.DocumentRollbackListener
			void documentUndoSequenceStarted(const kernel::Document& document) override;
			void documentUndoSequenceStopped(const kernel::Document& document, const kernel::Position& resultPosition) override;

		protected:
			/// @name Overridable Widget Events
			/// @{
			virtual void focusAboutToBeLost(widgetapi::event::Event& event);
			virtual void focusGained(widgetapi::event::Event& event);
			virtual void keyPressed(widgetapi::event::KeyInput& input);
			virtual void keyReleased(widgetapi::event::KeyInput& input);
			virtual void mouseDoubleClicked(widgetapi::event::MouseButtonInput& input);
			virtual void mouseMoved(widgetapi::event::LocatedUserInput& input);
			virtual void mousePressed(widgetapi::event::MouseButtonInput& input);
			virtual void mouseReleased(widgetapi::event::MouseButtonInput& input);
			virtual void mouseTripleClicked(widgetapi::event::MouseButtonInput& input);
			virtual void mouseWheelChanged(widgetapi::event::MouseWheelInput& input);
			virtual void paint(graphics::PaintContext& context);
			virtual void resized(const graphics::Dimension& newSize);
			virtual void showContextMenu(const widgetapi::event::LocatedUserInput& input, void* nativeEvent);
			/// @}

			/// @name Overridable Widget Events (Platform-dependent)
			/// @{
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			virtual void get_preferred_height_for_width_vfunc(int width, int& minimumHeight, int& naturalHeight) const override;
			virtual void get_preferred_height_vfunc(int& minimumHeight, int& naturalHeight) const override;
			virtual void get_preferred_width_vfunc(int& minimumWidth, int& naturalWidth) const override;
			virtual void get_preferred_width_for_height_vfunc(int height, int& minimumWidth, int& naturalWidth) const override;
			virtual Gtk::SizeRequestMode get_request_mode_vfunc() const override;
			virtual void on_realize() override;
			virtual void on_size_allocate(Gtk::Allocation& allocation) override;
			virtual void on_unrealize() override;
			//
			virtual bool on_button_press_event(GdkEventButton* event) override;
			virtual bool on_button_release_event(GdkEventButton* event) override;
			virtual void on_drag_leave(const Glib::RefPtr<Gdk::DragContext>& context, guint time) override;
			virtual bool on_drag_motion(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time) override;
			virtual bool on_drag_drop(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time) override;
			virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& context) override;
#ifdef _DEBUG
			virtual bool on_event(GdkEvent* event) override;
#endif
			virtual bool on_focus_in_event(GdkEventFocus* event) override;
			virtual bool on_focus_out_event(GdkEventFocus* event) override;
//			virtual bool on_grab_broken_event(GdkEventGrabBroken* event) override;
			virtual void on_grab_focus() override;
			virtual bool on_key_press_event(GdkEventKey* event) override;
			virtual bool on_key_release_event(GdkEventKey* event) override;
			virtual bool on_motion_notify_event(GdkEventMotion* event) override;
			virtual bool on_scroll_event(GdkEventScroll* event) override;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
			virtual void contextMenuEvent(QContextMenuEvent* event) override;
			virtual void focusInEvent(QFocusEvent* event) override;
			virtual void focusOutEvent(QFocusEvent* event) override;
			virtual void keyPressEvent(QKeyEvent* event) override;
			virtual void keyReleaseEvent(QKeyEvent* event) override;
			virtual void mouseDoubleClickEvent(QMouseEvent* event) override;
			virtual void mouseMoveEvent(QMouseEvent* event) override;
			virtual void mousePressEvent(QMouseEvent* event) override;
			virtual void mouseReleaseEvent(QMouseEvent* event) override;
			virtual void paintEvent(QPaintEvent* event) override;
			virtual void resizeEvent(QResizeEvent* event) override;
			virtual void timerEvent(QTimerEvent* event) override;
			virtual void wheelEvent(QWheelEvent* event) override;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			virtual void onCaptureChanged(const win32::Handle<HWND>::Type& newWindow, bool& consumed);
			virtual void onCommand(WORD id, WORD notifyCode, const win32::Handle<HWND>::Type& control, bool& consumed);
			virtual void onDestroy(bool& consumed);
			virtual void onEraseBkgnd(const win32::Handle<HDC>::Type& dc, bool& consumed);
			virtual const win32::Handle<HFONT>::Type onGetFont() const;
			virtual void onHScroll(UINT sbCode, UINT pos, const win32::Handle<HWND>::Type& scrollBar);
			virtual bool onNcCreate(CREATESTRUCTW& cs);
			virtual void onNotify(int id, NMHDR& nmhdr, bool& consumed);
			virtual void onSetCursor(const win32::Handle<HWND>::Type& window, UINT hitTest, UINT message, bool& consumed);
			virtual void onStyleChanged(int type, const STYLESTRUCT& style);
			virtual void onStyleChanging(int type, STYLESTRUCT& style);
			virtual void onSysColorChange();
			virtual void onThemeChanged();
			virtual void onTimer(UINT_PTR eventId, TIMERPROC timerProc);
			virtual void onVScroll(UINT sbCode, UINT pos, const win32::Handle<HWND>::Type& scrollBar);
			virtual LRESULT processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed);
			// IDropTarget
			virtual STDMETHODIMP DragEnter(IDataObject* data, DWORD keyState, POINTL location, DWORD* effect);
			virtual STDMETHODIMP DragOver(DWORD keyState, POINTL location, DWORD* effect);
			virtual STDMETHODIMP DragLeave();
			virtual STDMETHODIMP Drop(IDataObject* data, DWORD keyState, POINTL location, DWORD* effect);
#endif
			/// @}

		private:
			void fireMouseDoubleClicked(widgetapi::event::MouseButtonInput& input);
			void fireMouseMoved(widgetapi::event::LocatedUserInput& input);
			void fireMousePressed(widgetapi::event::MouseButtonInput& input);
			void fireMouseReleased(widgetapi::event::MouseButtonInput& input);
			void fireMouseTripleClicked(widgetapi::event::MouseButtonInput& input);
			void fireMouseWheelChanged(widgetapi::event::MouseWheelInput& input);

			// window system-related private methods
		private:
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			// GtkIMContext
			static void handleInputMethodContextCommitSignal(GtkIMContext* context, gchar* text, gpointer userData);
			static gboolean handleInputMethodContextDeleteSurroundingSignal(GtkIMContext* context, gint offset, gint nchars, gpointer userData);
			static void handleInputMethodContextPreeditChangedSignal(GtkIMContext* context, gpointer userData);
			static void handleInputMethodContextPreeditEndSignal(GtkIMContext* context, gpointer userData);
			static void handleInputMethodContextPreeditStartSignal(GtkIMContext* context, gpointer userData);
			static gboolean handleInputMethodContextRetrieveSurroundingSignal(GtkIMContext* context, gpointer userData);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			// base.Widget
			void provideClassInformation(ClassInformation& classInformation) const;
			std::basic_string<WCHAR> provideClassName() const;
#endif	// ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)

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
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			Glib::RefPtr<Gdk::Window> window_;
#endif	// ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			presentation::Presentation& presentation_;
			std::unique_ptr<TextArea> textArea_;
			Configuration configuration_;
			std::weak_ptr<MouseInputStrategy> lockedMouseInputStrategy_;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			win32::Handle<HWND>::Type toolTip_;
			std::basic_string<WCHAR> tipText_;
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			// strategies and listeners
			std::unique_ptr<contentassist::ContentAssistant> contentAssistant_;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32) && !defined(ASCENSION_NO_ACTIVE_ACCESSIBILITY)
			win32::com::SmartPointer<detail::AbstractAccessibleProxy> accessibleProxy_;
#endif
			boost::signals2::scoped_connection computedTextToplevelStyleChangedConnection_,
				viewportResizedConnection_, viewportScrolledConnection_, viewportScrollPropertiesChangedConnection_;

			// modes
			struct ModeState {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32) && !defined(ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER)
				bool activeInputMethodEnabled;	// true if uses Global IME (deprecated)
#endif

				ModeState() BOOST_NOEXCEPT
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32) && !defined(ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER)
					: activeInputMethodEnabled(true)
#endif
				{}
			} modeState_;

			// scroll information
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK) && !defined(ASCENSION_PIXELFUL_SCROLL_IN_BPD)
			graphics::PhysicalTwoAxes<double> scrollPositionsBeforeChanged_;
#endif
#if 0
			struct Scrolls {
//				unsigned long horizontalRate, verticalRate;	// 最小スクロール量が何文字 (何行) に相当するか (普通は 1)
				bool changed;
				Scrolls() BOOST_NOEXCEPT : /*horizontalRate(1), verticalRate(1), */changed(false) {}
				void resetBars(const TextViewer& viewer, char bars, bool pageSizeChanged) BOOST_NOEXCEPT;
			} scrolls_;
#endif
			// freeze information
			boost::value_initialized<std::size_t> frozenCount_;

			// input state
			boost::value_initialized<std::size_t> mouseInputDisabledCount_;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			std::shared_ptr<GtkIMContext> inputMethodContext_;
			friend std::shared_ptr<GtkIMContext> detail::inputMethodContext(TextViewer&);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			win32::com::SmartPointer<IDropTargetHelper> dropTargetHelper_;
			win32::com::SmartPointer<IDataObject> draggingData_;
#else
//			const NativeMimeData* draggingData_;
#endif
			// signals
			FocusChangedSignal focusChangedSignal_;
			FrozenStateChangedSignal frozenStateChangedSignal_;

			friend class VisualPoint;
			friend class VirtualBox;
			friend class CaretShapeUpdater;
			friend class Renderer;
		};

		// the documentation is text-viewer.cpp
		class AutoFreeze : private boost::noncopyable {
		public:
			explicit AutoFreeze(TextViewer* textViewer);
			~AutoFreeze() BOOST_NOEXCEPT;
		private:
			TextViewer* const textViewer_;
		};


		// inline implementation //////////////////////////////////////////////////////////////////

		/**
		 * Returns @c true if the viewer allows the mouse operations.
		 * @see #enableMouseInput
		 */
		inline bool TextViewer::allowsMouseInput() const BOOST_NOEXCEPT {
			return boost::get(mouseInputDisabledCount_) == 0;
		} 

		/// Informs the end user of <strong>safe</strong> error.
		inline void TextViewer::beep() BOOST_NOEXCEPT {doBeep();}

		/**
		 * Returns the general configuration.
		 * @see #setConfiguration
		 */
		inline const TextViewer::Configuration& TextViewer::configuration() const BOOST_NOEXCEPT {
			return configuration_;
		}
		
		/// Returns the content assistant or @c null if not registered.
		inline contentassist::ContentAssistant* TextViewer::contentAssistant() const BOOST_NOEXCEPT {
			return contentAssistant_.get();
		}
		
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32) && !defined(ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER)
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
			if(boost::get(mouseInputDisabledCount_) != 0 || !enable)
				mouseInputDisabledCount_ += !enable ? 1 : -1;
		}
		
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32) && !defined(ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER)
		/// Returns @c true if Global IME is enabled.
		inline bool TextViewer::isActiveInputMethodEnabled() const BOOST_NOEXCEPT {
			return modeState_.activeInputMethodEnabled;
		}
#endif
		
		/// Returns @c true if the viewer is frozen.
		inline bool TextViewer::isFrozen() const BOOST_NOEXCEPT {
			return frozenCount_ != 0;
		}
		
		/// Returns the presentation object. 
		inline BOOST_CONSTEXPR presentation::Presentation& TextViewer::presentation() BOOST_NOEXCEPT {
			return presentation_;
		}
		
		/// Returns the presentation object. 
		inline BOOST_CONSTEXPR const presentation::Presentation& TextViewer::presentation() const BOOST_NOEXCEPT {
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

		/// Returns the @c TextArea of this text viewer.
		inline BOOST_CONSTEXPR TextArea& TextViewer::textArea() BOOST_NOEXCEPT {
			return *textArea_;
		}

		/// Returns the @c TextArea of this text viewer.
		inline BOOST_CONSTEXPR const TextArea& TextViewer::textArea() const BOOST_NOEXCEPT {
			return *textArea_;
		}
	}
} // namespace ascension.viewer

#endif // !ASCENSION_VIEWER_HPP
