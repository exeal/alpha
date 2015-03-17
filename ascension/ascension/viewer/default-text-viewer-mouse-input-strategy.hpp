/**
 * @file default-text-viewer-mouse-input-strategy.hpp
 * Defines @c DefaultTextViewerMouseInputStrategy class.
 * @author exeal
 * @date 2003-2006 was EditView.h
 * @date 2006-2011 was viewer.hpp
 * @date 2011-10-04 separated from viewer.hpp
 * @date 2015-03-16 Renamed from default-mouse-input-strategy.hpp
 */

#ifndef ASCENSION_TEXT_VIEWER_DEFAULT_MOUSE_INPUT_STRATEGY_HPP
#define ASCENSION_TEXT_VIEWER_DEFAULT_MOUSE_INPUT_STRATEGY_HPP

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION, ...
#include <ascension/platforms.hpp>
#include <ascension/corelib/timer.hpp>	// Timer
#include <ascension/kernel/position.hpp>	// kernel.Position
#include <ascension/viewer/widgetapi/widget.hpp>
#include <memory>	// std.unique_ptr
#include <utility>	// std.pair
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <ascension/win32/com/smart-pointer.hpp>
#	include <ascension/win32/com/unknown-impl.hpp>
#	include <shlobj.h>	// IDragSourceHelper
#endif


namespace ascension {

	namespace presentation {
		namespace hyperlink {
			class Hyperlink;
		}
	}

	namespace viewer {
		namespace widgetapi {
			class Cursor;
		}

		// the documentation is user-input.cpp
		class DefaultMouseInputStrategy : public TextViewerMouseInputStrategy,
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				win32::com::IUnknownImpl<ASCENSION_WIN32_COM_INTERFACE(IDropSource), win32::com::NoReferenceCounting>,
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				private HasTimer, private widgetapi::DropTarget {
		public:
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			/**
			 * Defines drag-and-drop support levels.
			 * @see DefaultMouseInputStrategy#DefaultMouseInputStrategy
			 */
			enum DragAndDropSupport {
				/// Disables drag-and-drop.
				DONT_SUPPORT_DND,
				/// Enables drag-and-drop.
				SUPPORT_DND,
				/// Enables drag-and-drop and shows a drag-image.
				SUPPORT_DND_WITH_DRAG_IMAGE,
				/// Enables drag-and-drop and shows a selection-highlighted drag-image.
				SUPPORT_DND_WITH_SELECTED_DRAG_IMAGE
			};
#endif // ASCENSION_ABANDONED_AT_VERSION_08
		public:
			DefaultTextViewerMouseInputStrategy();

		protected:
			/// @name Overridable @c widgetapi#DropTarget Implementation
			/// @{
			virtual void dragEntered(widgetapi::DragEnterInput& input) override;
			virtual void dragLeft(widgetapi::DragLeaveInput& input) override;
			virtual void dragMoved(widgetapi::DragMoveInput& input) override;
			virtual void dropped(widgetapi::DropInput& input) override;
			/// @}

		private:
			void beginDragAndDrop(const widgetapi::event::LocatedUserInput& input);
			virtual void handleLeftButtonDoubleClick(widgetapi::event::MouseButtonInput& input);
			virtual void handleRightButton(Action action, widgetapi::event::MouseButtonInput& input);
			virtual void handleX1Button(Action action, widgetapi::event::MouseButtonInput& input);
			virtual void handleX2Button(Action action, widgetapi::event::MouseButtonInput& input);
			static void showCursor(TextViewer& viewer, const widgetapi::Cursor& cursor);
		private:
			bool endAutoScroll();
			void extendSelectionTo(const kernel::Position* to = nullptr);
			void handleLeftButtonPressed(widgetapi::event::MouseButtonInput& input);
			void handleLeftButtonReleased(widgetapi::event::MouseButtonInput& input);
			// MouseInputStrategy
			void captureChanged() override;
			std::shared_ptr<widgetapi::DropTarget> handleDropTarget() const override;
			void interruptMouseReaction(bool forKeyboardInput) override;
			void mouseButtonInput(Action action, widgetapi::event::MouseButtonInput& input) override;
			void mouseMoved(widgetapi::event::LocatedUserInput& input) override;
			void mouseWheelRotated(widgetapi::event::MouseWheelInput& input) override;
			bool showCursor(const graphics::Point& position) override;
			// TextViewerMouseInputStrategy
			void install(TextViewer& viewer) override;
			void uninstall() override;
			// HasTimer
			void timeElapsed(Timer& timer) override;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			// IDropSource
			STDMETHODIMP QueryContinueDrag(BOOL escapePressed, DWORD keyState) override;
			STDMETHODIMP GiveFeedback(DWORD effect) override;
			// IDropTarget
			STDMETHODIMP DragEnter(IDataObject* data, DWORD keyState, POINTL pt, DWORD* effect) override;
			STDMETHODIMP DragOver(DWORD keyState, POINTL pt, DWORD* effect) override;
			STDMETHODIMP DragLeave() override;
			STDMETHODIMP Drop(IDataObject* data, DWORD keyState, POINTL pt, DWORD* effect) override;
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		private:
			TextViewer* viewer_;
			enum {
				NONE = 0x00,
				SELECTION_EXTENDING_MASK = 0x10, EXTENDING_CHARACTER_SELECTION, EXTENDING_WORD_SELECTION, EXTENDING_LINE_SELECTION,
				AUTO_SCROLL_MASK = 0x20, APPROACHING_AUTO_SCROLL, AUTO_SCROLL_DRAGGING, AUTO_SCROLL,
				DND_MASK = 0x40, APPROACHING_DND, DND_SOURCE, DND_TARGET
			} state_;
			graphics::Point dragApproachedPosition_;	// in client coordinates
			struct Selection {
				Index initialLine;	// line of the anchor when entered the selection extending
				std::pair<Index, Index> initialWordColumns;
			} selection_;
			struct DragAndDrop {
				Index numberOfRectangleLines;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				win32::com::SmartPointer<IDragSourceHelper> dragSourceHelper;
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			} dnd_;
			std::unique_ptr<widgetapi::Widget::value_type> autoScrollOriginMark_;
			const presentation::hyperlink::Hyperlink* lastHoveredHyperlink_;
			Timer timer_;
			static const unsigned int SELECTION_EXPANSION_INTERVAL, DRAGGING_TRACK_INTERVAL;
		};
	}

} // namespace ascension.viewer

#endif // !ASCENSION_DEFAULT_TEXT_VIEWER_MOUSE_INPUT_STRATEGY_HPP
