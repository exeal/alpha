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
#include <ascension/kernel/point.hpp>
#include <ascension/presentation/presentation.hpp>
#include <ascension/presentation/text-style.hpp>
#include <ascension/viewer/caret-observers.hpp>
#include <ascension/viewer/content-assist.hpp>
#include <ascension/viewer/ruler.hpp>
#include <ascension/viewer/viewer-observers.hpp>
#include <ascension/viewer/base/scrollable.hpp>
#include <ascension/win32/com/unknown-impl.hpp>
#include <algorithm>
#include <array>
#include <set>
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
#	include <shlobj.h>	// IDragSourceHelper, IDropTargetHelper
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
		bool getPointedIdentifier(const viewers::TextViewer& viewer,
				kernel::Position* startPosition, kernel::Position* endPosition);
		bool getNearestIdentifier(const kernel::Document& document,
				const kernel::Position& position, Index* startColumn, Index* endColumn);
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
				public base::ScrollableWidget,
				public kernel::DocumentListener, public kernel::DocumentStateListener,
				public kernel::DocumentRollbackListener, public graphics::font::DefaultFontListener,
				public graphics::font::VisualLinesListener, public graphics::font::TextViewportListener,
				public CaretListener, public CaretStateListener, public detail::PointCollection<VisualPoint> {
		public:
			/// Result of hit test.
			enum HitTestResult {
				INDICATOR_MARGIN,	///< The point is on the indicator margin.
				LINE_NUMBERS,		///< The point is on the line numbers area.
				CONTENT_AREA,		///< The point is on the text content area.
				OUT_OF_VIEWPORT		///< The point is outside of the viewport.
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
				explicit Renderer(TextViewer& viewer,
					const presentation::WritingMode& writingMode = presentation::WritingMode());
				Renderer(const Renderer& other, TextViewer& viewer);
				void setDefaultWritingMode(const presentation::WritingMode& writingMode) /*throw()*/;
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
				void rewrapAtWindowEdge();
#endif // ASCENSION_ABANDONED_AT_VERSION_08
				// TextRenderer
				std::unique_ptr<const graphics::font::TextLayout> createLineLayout(Index line) const;
				const presentation::WritingMode& defaultUIWritingMode() const /*throw()*/;
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
				graphics::Scalar width() const /*throw()*/;
#endif // ASCENSION_ABANDONED_AT_VERSION_08
			private:
				TextViewer& viewer_;
				presentation::WritingMode defaultWritingMode_;
//				presentation::Inheritable<presentation::TextAnchor> overrideTextAnchor_;
			};

			// constructors
			explicit TextViewer(presentation::Presentation& presentation, Widget* parent = nullptr, Style styles = WIDGET);
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
			void scroll(int dx, int dy, bool redraw);
			void scrollTo(int x, int y, bool redraw);
			void scrollTo(Index line, bool redraw);
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
			// viewport
			void firstVisibleLine(graphics::font::VisualLine* line, Index* viewportOffset) const /*throw()*/;
			HitTestResult hitTest(const graphics::NativePoint& pt) const;
			graphics::NativeRectangle textAllocationRectangle() const /*throw()*/;

		protected:
			virtual void doBeep() /*throw()*/;
			virtual void drawIndicatorMargin(Index line, graphics::Context& context, const graphics::NativeRectangle& rect);

			// helpers
		private:
			graphics::Scalar inlineProgressionOffsetInViewport() const;
			void initialize();
			graphics::Scalar mapLineLayoutIpdToViewport(Index line, graphics::Scalar ipd) const;
//			graphics::Scalar mapLineToViewportBpd(Index line, bool fullSearch) const;
			graphics::Scalar mapViewportIpdToLineLayout(Index line, graphics::Scalar ipd) const;
			void repaintRuler();
			void updateScrollBars();

			// protected interfaces
		protected:
			// kernel.DocumentStateListener (overridable)
			virtual void documentAccessibleRegionChanged(const kernel::Document& document);
			virtual void documentModificationSignChanged(const kernel::Document& document);
			virtual void documentPropertyChanged(const kernel::Document& document, const kernel::DocumentPropertyKey& key);
			virtual void documentReadOnlySignChanged(const kernel::Document& document);
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
			// base.DropTarget
			void dragEntered(base::DragEnterInput& input);
			void dragLeft(base::DragLeaveInput& input);
			void dragMoved(base::DragMoveInput& input);
			void dropped(base::DropInput& input);
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
			void viewportPositionChanged(
				const graphics::font::VisualLine& oldLine, Index oldInlineProgressionOffset) /*throw()*/;
			void viewportSizeChanged(const graphics::NativeSize& oldSize) /*throw()*/;
			// detail.PointCollection<VisualPoint>
			void addNewPoint(VisualPoint& point) {points_.insert(&point);}
			void removePoint(VisualPoint& point) {points_.erase(&point);}

			// event handlers
		private:
			void aboutToLoseFocus();
			void focusGained();
			void keyPressed(const base::KeyInput& input);
			void keyReleased(const base::KeyInput& input);
			void mouseDoubleClicked(const base::MouseButtonInput& input);
			void mouseMoved(const base::LocatedUserInput& input);
			void mousePressed(const base::MouseButtonInput& input);
			void mouseReleased(const base::MouseButtonInput& input);
			void mouseWheelChanged(const base::MouseWheelInput& input);
			void paint(graphics::PaintContext& context);
			void resized(State state, const graphics::NativeSize& newSize);
			void showContextMenu(const base::LocatedUserInput& input, bool byKeyboard);
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32)
			LRESULT handleWindowSystemEvent(UINT message, WPARAM wp, LPARAM lp, bool& consumed);
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
#ifdef WM_THEMECHANGED
			void onThemeChanged();
#endif // WM_THEMECHANGED
			void onTimer(UINT_PTR eventId, TIMERPROC timerProc);
			void onVScroll(UINT sbCode, UINT pos, const win32::Handle<HWND>& scrollBar);
#endif

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
			std::shared_ptr<base::DropTarget> dropTargetHandler_;
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
				struct Axis {
					ScrollPosition position;
					ScrollPosition maximum;
					ScrollPosition pageSize;
//					unsigned long rate;		// 最小スクロール量が何文字 (何行) に相当するか (普通は 1)
				} horizontal, vertical;
				graphics::font::VisualLine firstVisibleLine;
				bool changed;
				Scrolls() /*throw()*/ : firstVisibleLine(0, 0), changed(false) {
					horizontal.position = vertical.position = 0;
//					horizontal.rate = vertical.rate = 1;
				}
				unsigned long x() const /*throw()*/ {return horizontal.position/* * horizontal.rate*/;}
				unsigned long y() const /*throw()*/ {return vertical.position/* * vertical.rate*/;}
				void resetBars(const TextViewer& viewer, char bars, bool pageSizeChanged) /*throw()*/;
				void updateVertical(const TextViewer& viewer) /*throw()*/;
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

/**
 * Returns the line first visible in the viewport without before-space.
 * @param[out] line The first visible logical and visual lines. Can be @c null if not needed
 * @param[out] viewportOffset The visual index of the line. Can be @c null if not needed
 */
inline void TextViewer::firstVisibleLine(graphics::font::VisualLine* line, Index* viewportOffset) const /*throw()*/ {
	if(line != 0)
		*line = scrolls_.firstVisibleLine;
	if(viewportOffset != 0)
		*viewportOffset = scrolls_.y();	// TODO: This code can't handle vertical writing-mode.
}

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
