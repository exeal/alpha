/**
 * @file abstract-mouse-input-strategy.cpp
 * Implements @c AbstractMouseInputStrategy class.
 * @author exeal
 * @date 2015-04-08 Created.
 */

#include <ascension/viewer/mouse-input-strategy.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/widgetapi/cursor.hpp>
#include <ascension/viewer/widgetapi/event/mouse-button-input.hpp>
#include <ascension/viewer/widgetapi/event/mouse-wheel-input.hpp>
#include <boost/optional.hpp>

namespace ascension {
	namespace viewer {
		/// Destructor.
		AbstractMouseInputStrategy::~AbstractMouseInputStrategy() BOOST_NOEXCEPT {
		}

		/// Does nothing.
		void AbstractMouseInputStrategy::captureChanged() {}
		
		/// Returns @c null.
		std::shared_ptr<widgetapi::DropTarget> AbstractMouseInputStrategy::handleDropTarget() const {
			return std::shared_ptr<widgetapi::DropTarget>();
		}
		
		/// Does nothing.
		void AbstractMouseInputStrategy::interruptMouseReaction(bool) {
		}
		
		/// Ignores the input.
		void AbstractMouseInputStrategy::mouseButtonInput(Action, widgetapi::event::MouseButtonInput& input, TargetLocker&) {
			return input.ignore();
		}
		
		/// Ignores the input.
		void AbstractMouseInputStrategy::mouseMoved(widgetapi::event::LocatedUserInput& input, TargetLocker&) {
			return input.ignore();
		}
		
		/// Ignores the input.
		void AbstractMouseInputStrategy::mouseWheelRotated(widgetapi::event::MouseWheelInput& input, TargetLocker&) {
			return input.ignore();
		}

		/**
		 * Shows the built-in arrow cursor.
		 * @param viewer The text viewer
		 * @return @c true
		 * @see #showCursor
		 */
		bool AbstractMouseInputStrategy::showArrowCursor(TextViewer& viewer) {
			boost::optional<widgetapi::Cursor::BuiltinShape> builtinShape(
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				Gdk::ARROW
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				Qt::ArrowCursor
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
				[NSCursor arrowCursor]
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				IDC_ARROW
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
			);
			const widgetapi::Cursor cursor(*builtinShape);
			showCursor(viewer, cursor);
			return true;
		}

		/**
		 * Shows the specified cursor.
		 * @param viewer The text viewer
		 * @param cursor The cursor to show
		 * @return @c true
		 */
		bool AbstractMouseInputStrategy::showCursor(TextViewer& viewer, const widgetapi::Cursor& cursor) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			viewer.get_window()->set_cursor(const_cast<widgetapi::Cursor&>(cursor).asNativeObject());
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
			QApplication::setOverrideCursor(cursor.asNativeObject());	// TODO: Restore later.
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
			[cursor set];
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			::SetCursor(cursor.asNativeObject().get());
#else
			ASCENSION_CANT_DETECT_PLATFORM();
#endif
			return true;
		}
		
		/// Returns @c false.
		bool AbstractMouseInputStrategy::showCursor(const graphics::Point&) {
			return false;
		}
	}
}
