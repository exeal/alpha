/**
 * @file default-text-area-mouse-input-strategy.hpp
 * Defines @c DefaultTextAreaMouseInputStrategy class.
 * @author exeal
 * @date 2003-2006 was EditView.h
 * @date 2006-2011 was viewer.hpp
 * @date 2011-10-04 separated from viewer.hpp
 * @date 2015-03-16 Renamed from default-mouse-input-strategy.hpp
 * @date 2015-03-19 Renamed from default-text-viewer-mouse-input-strategy.hpp
 */

#ifndef ASCENSION_TEXT_AREA_DEFAULT_MOUSE_INPUT_STRATEGY_HPP
#define ASCENSION_TEXT_AREA_DEFAULT_MOUSE_INPUT_STRATEGY_HPP

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION, ...
#include <ascension/platforms.hpp>
#include <ascension/corelib/timer.hpp>	// Timer
#include <ascension/viewer/text-area-mouse-input-strategy.hpp>
#include <ascension/viewer/widgetapi/widget.hpp>
#include <boost/optional.hpp>
#include <boost/range/irange.hpp>
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
	namespace kernel {
		class Position;
	}

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
		class DefaultTextAreaMouseInputStrategy :
				public AbstractMouseInputStrategy, public TextAreaMouseInputStrategy,
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				win32::com::IUnknownImpl<ASCENSION_WIN32_COM_INTERFACE(IDropSource), win32::com::NoReferenceCounting>,
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				private HasTimer<DefaultTextAreaMouseInputStrategy>, private widgetapi::DropTarget {
		public:
			struct SelectionExtender;
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			/**
			 * Defines drag-and-drop support levels.
			 * @see DefaultTextAreaMouseInputStrategy#DefaultTextAreaMouseInputStrategy
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
			DefaultTextAreaMouseInputStrategy();

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
		private:
			bool endAutoScroll();
			void continueSelectionExtension(const kernel::Position& to);
			void handleLeftButtonPressed(widgetapi::event::MouseButtonInput& input, TargetLocker& targetLocker);
			void handleLeftButtonReleased(widgetapi::event::MouseButtonInput& input);
			bool isStateNeutral() const BOOST_NOEXCEPT;
			// MouseInputStrategy
			std::shared_ptr<widgetapi::DropTarget> handleDropTarget() const override;
			void interruptMouseReaction(bool forKeyboardInput) override;
			void mouseButtonInput(Action action, widgetapi::event::MouseButtonInput& input, TargetLocker& targetLocker) override;
			void mouseInputTargetUnlocked() override;
			void mouseMoved(widgetapi::event::LocatedUserInput& input, TargetLocker& targetLocker) override;
			void mouseWheelRotated(widgetapi::event::MouseWheelInput& input, TargetLocker& targetLocker) override;
			bool showCursor(const graphics::Point& position) override;
			// TextAreaMouseInputStrategy
			void install(TextArea& viewer) override;
			void uninstall() override;
			// HasTimer<DefaultTextAreaMouseInputStrategy>
			void timeElapsed(Timer<DefaultTextAreaMouseInputStrategy>& timer) override;
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
			TextArea* textArea_;
			std::unique_ptr<SelectionExtender> selectionExtender_;	// not null only if selecting text
			struct AutoScroll {
				enum State {APPROACHING, SCROLLING_WITH_DRAG, SCROLLING_WITHOUT_DRAG} state;
				graphics::Point approachedPosition;	// in viewer-local coordinates
			};
			boost::optional<AutoScroll> autoScroll_;
			std::unique_ptr<widgetapi::Widget::value_type> autoScrollOriginMark_;
			struct DragAndDrop {
				enum State {APPROACHING, PROCESSING_AS_SOURCE, PROCESSING_AS_TARGET} state;
				graphics::Point approachedPosition;	// in viewer-local coordinates
				Index numberOfRectangleLines;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				win32::com::SmartPointer<IDragSourceHelper> dragSourceHelper;
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			};
			boost::optional<DragAndDrop> dragAndDrop_;
			Timer<DefaultTextAreaMouseInputStrategy> timer_;
			const presentation::hyperlink::Hyperlink* lastHoveredHyperlink_;
		};
	}

} // namespace ascension.viewer

#endif // !ASCENSION_DEFAULT_TEXT_AREA_MOUSE_INPUT_STRATEGY_HPP
