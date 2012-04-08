/**
 * @file viewer.hpp
 * This header defines several visual presentation classes.
 * @author exeal
 * @date 2003-2006 was EditView.h
 * @date 2006-2011 was viewer.hpp
 * @date 2011-10-04 separated from viewer.hpp
 * @date 2012
 */

#ifndef ASCENSION_DEFAULT_MOUSE_INPUT_STRATEGY_HPP
#define ASCENSION_DEFAULT_MOUSE_INPUT_STRATEGY_HPP

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION, ...
#include <ascension/platforms.hpp>
#include <ascension/corelib/timer.hpp>	// Timer
#include <ascension/kernel/position.hpp>	// kernel.Position
#include <ascension/viewer/caret-observers.hpp>
#include <ascension/viewer/base/widget.hpp>
#include <memory>	// std.unique_ptr
#include <utility>	// std.pair
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
#	include <ascension/win32/com/unknown-impl.hpp>
#	include <shlobj.h>	// IDragSourceHelper, IDropTargetHelper
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
				win32::com::IUnknownImpl<
					typelist::Cat<
						ASCENSION_WIN32_COM_INTERFACE_SIGNATURE(IDropSource), typelist::Cat<
							ASCENSION_WIN32_COM_INTERFACE_SIGNATURE(IDropTarget)
						>
					>, win32::com::NoReferenceCounting
				>,
#endif // ASCENSION_WINDOW_SYSTEM_WIN32
				private HasTimer, private base::DropTarget {
			ASCENSION_UNASSIGNABLE_TAG(DefaultMouseInputStrategy);
		public:
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
		public:
			explicit DefaultMouseInputStrategy(
				DragAndDropSupport dragAndDropSupportLevel = SUPPORT_DND_WITH_SELECTED_DRAG_IMAGE);
		private:
			virtual bool handleLeftButtonDoubleClick(const graphics::NativePoint& position, int modifiers);
			virtual bool handleRightButton(Action action, const graphics::NativePoint& position, int modifiers);
			virtual bool handleX1Button(Action action, const graphics::NativePoint& position, int modifiers);
			virtual bool handleX2Button(Action action, const graphics::NativePoint& position, int modifiers);
		private:
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
			HRESULT doDragAndDrop();
#endif // ASCENSION_WINDOW_SYSTEM_WIN32
			bool endAutoScroll();
			void extendSelectionTo(const kernel::Position* to = nullptr);
			void handleLeftButtonPressed(const graphics::NativePoint& position, int modifiers);
			void handleLeftButtonReleased(const graphics::NativePoint& position, int modifiers);
			// MouseInputStrategy
			void captureChanged();
			std::shared_ptr<base::DropTarget> handleDropTarget() const;
			void install(TextViewer& viewer);
			void interruptMouseReaction(bool forKeyboardInput);
			bool mouseButtonInput(Action action, const base::MouseButtonInput& input);
			void mouseMoved(const base::LocatedUserInput& input);
			void mouseWheelRotated(const base::MouseWheelInput& input);
			bool showCursor(const graphics::NativePoint& position);
			void uninstall();
			// HasTimer
			void timeElapsed(Timer& timer);
			// base.DropTarget
			void dragEntered(base::DragEnterInput& input);
			void dragLeft(base::DragLeaveInput& input);
			void dragMoved(base::DragMoveInput& input);
			void dropped(base::DropInput& input);
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
			graphics::NativePoint dragApproachedPosition_;	// in client coordinates
			struct Selection {
				Index initialLine;	// line of the anchor when entered the selection extending
				std::pair<Index, Index> initialWordColumns;
			} selection_;
			struct DragAndDrop {
				DragAndDropSupport supportLevel;
				Index numberOfRectangleLines;
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
				win32::com::ComPtr<IDragSourceHelper> dragSourceHelper;
				win32::com::ComPtr<IDropTargetHelper> dropTargetHelper;
#endif // ASCENSION_WINDOW_SYSTEM_WIN32
			} dnd_;
			std::unique_ptr<base::Widget> autoScrollOriginMark_;
			const presentation::hyperlink::Hyperlink* lastHoveredHyperlink_;
			Timer timer_;
			static const unsigned int SELECTION_EXPANSION_INTERVAL, DRAGGING_TRACK_INTERVAL;
		};
	}

} // namespace ascension.viewers

#endif // !ASCENSION_DEFAULT_MOUSE_INPUT_STRATEGY_HPP
