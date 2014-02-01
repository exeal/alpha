/**
 * @file viewer.hpp
 * This header defines several visual presentation classes.
 * @author exeal
 * @date 2003-2006 was EditView.h
 * @date 2006-2011 was viewer.hpp
 * @date 2011-10-04 separated from viewer.hpp
 * @date 2012, 2014
 */

#ifndef ASCENSION_DEFAULT_MOUSE_INPUT_STRATEGY_HPP
#define ASCENSION_DEFAULT_MOUSE_INPUT_STRATEGY_HPP

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION, ...
#include <ascension/platforms.hpp>
#include <ascension/corelib/timer.hpp>	// Timer
#include <ascension/kernel/position.hpp>	// kernel.Position
#include <ascension/viewer/widgetapi/widget.hpp>
#include <memory>	// std.unique_ptr
#include <utility>	// std.pair
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
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

	namespace viewers {

		// the documentation is user-input.cpp
		class DefaultMouseInputStrategy : public MouseInputStrategy,
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
				win32::com::IUnknownImpl<ASCENSION_WIN32_COM_INTERFACE(IDropSource), win32::com::NoReferenceCounting>,
#endif // ASCENSION_WINDOW_SYSTEM_WIN32
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
			DefaultMouseInputStrategy();
		protected:
			// widgetapi.DropTarget
			virtual void dragEntered(widgetapi::DragEnterInput& input);
			virtual void dragLeft(widgetapi::DragLeaveInput& input);
			virtual void dragMoved(widgetapi::DragMoveInput& input);
			virtual void dropped(widgetapi::DropInput& input);
		private:
			void beginDragAndDrop(const widgetapi::LocatedUserInput& input);
			virtual bool handleLeftButtonDoubleClick(const graphics::Point& position, int modifiers);
			virtual bool handleRightButton(Action action, const graphics::Point& position, int modifiers);
			virtual bool handleX1Button(Action action, const graphics::Point& position, int modifiers);
			virtual bool handleX2Button(Action action, const graphics::Point& position, int modifiers);
		private:
			bool endAutoScroll();
			void extendSelectionTo(const kernel::Position* to = nullptr);
			void handleLeftButtonPressed(const graphics::Point& position, int modifiers);
			void handleLeftButtonReleased(const graphics::Point& position, int modifiers);
			// MouseInputStrategy
			void captureChanged();
			std::shared_ptr<widgetapi::DropTarget> handleDropTarget() const;
			void install(TextViewer& viewer);
			void interruptMouseReaction(bool forKeyboardInput);
			bool mouseButtonInput(Action action, const widgetapi::MouseButtonInput& input);
			void mouseMoved(const widgetapi::LocatedUserInput& input);
			void mouseWheelRotated(const widgetapi::MouseWheelInput& input);
			bool showCursor(const graphics::Point& position);
			void uninstall();
			// HasTimer
			void timeElapsed(Timer& timer);
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
			// IDropSource
			STDMETHODIMP QueryContinueDrag(BOOL escapePressed, DWORD keyState);
			STDMETHODIMP GiveFeedback(DWORD effect);
			// IDropTarget
			STDMETHODIMP DragEnter(IDataObject* data, DWORD keyState, POINTL pt, DWORD* effect);
			STDMETHODIMP DragOver(DWORD keyState, POINTL pt, DWORD* effect);
			STDMETHODIMP DragLeave();
			STDMETHODIMP Drop(IDataObject* data, DWORD keyState, POINTL pt, DWORD* effect);
#endif // ASCENSION_WINDOW_SYSTEM_WIN32
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
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
				win32::com::SmartPointer<IDragSourceHelper> dragSourceHelper;
#endif // ASCENSION_WINDOW_SYSTEM_WIN32
			} dnd_;
			std::unique_ptr<widgetapi::Widget::value_type> autoScrollOriginMark_;
			const presentation::hyperlink::Hyperlink* lastHoveredHyperlink_;
			Timer timer_;
			static const unsigned int SELECTION_EXPANSION_INTERVAL, DRAGGING_TRACK_INTERVAL;
		};
	}

} // namespace ascension.viewers

#endif // !ASCENSION_DEFAULT_MOUSE_INPUT_STRATEGY_HPP
