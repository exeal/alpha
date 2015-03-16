/**
 * @file mouse-input-strategy.hpp
 * @author exeal
 * @date 2011-03-30 separated from viewer.hpp
 * @date 2015-03-16 Separated from viewer-observers.hpp
 */

#ifndef ASCENSION_MOUSE_INPUT_STRATEGY_HPP
#define ASCENSION_MOUSE_INPUT_STRATEGY_HPP
#include <ascension/graphics/geometry/point.hpp>
#include <ascension/platforms.hpp>
#include <ascension/viewer/widgetapi/drag-and-drop.hpp>

namespace ascension {
	namespace viewer {
		namespace widgetapi {
			class DropTarget;

			namespace event {
				class LocatedUserInput;
				class MouseButtonInput;
				class MouseWheelInput;
			}
		}

		/**
		 * Interface of objects which define how the text editors react to the users' mouse input.
		 * @see TextViewerMouseInputStrategy, source#Ruler
		 */
		class MouseInputStrategy {
		public:
			/// Actions of the mouse input.
			enum Action {
				PRESSED,		///< The button was pressed (down).
				RELEASED,		///< The button was released (up).
				DOUBLE_CLICKED,	///< The button was double-clicked.
				TRIPLE_CLICKED	///< The button was triple-clicked.
			};

			/// Destructor.
			virtual ~MouseInputStrategy() BOOST_NOEXCEPT {}
			/// The viewer lost the mouse capture.
			virtual void captureChanged() = 0;
			/// Returns @c DropTarget if this object supports the interface, or @c null.
			virtual std::shared_ptr<widgetapi::DropTarget> handleDropTarget() const = 0;
			/**
			 * Interrupts the progressive mouse reaction.
			 * This method must be called before @c #uninstall call.
			 * @param forKeyboardInput @c true if the mouse reaction should interrupt because the keyboard input was
			 *                         occurred
			 */
			virtual void interruptMouseReaction(bool forKeyboardInput) = 0;
			/**
			 * The mouse input was occurred and the viewer had focus.
			 * @param action The action of the input
			 * @param input The input information
			 */
			virtual void mouseButtonInput(Action action, widgetapi::event::MouseButtonInput& input) = 0;
			/**
			 * The mouse was moved and the viewer had focus.
			 * @param input The input information
			 */
			virtual void mouseMoved(widgetapi::event::LocatedUserInput& input) = 0;
			/**
			 * The mouse wheel was rolated and the viewer had focus.
			 * @param input The input information
			 */
			virtual void mouseWheelRotated(widgetapi::event::MouseWheelInput& input) = 0;
			/**
			 * Shows a cursor on the viewer.
			 * @param position The cursor position (client coordinates)
			 * @retval @c true if the callee showed a cursor
			 * @retval @c false if the callee did not know the appropriate cursor
			 */
			virtual bool showCursor(const graphics::Point& position) = 0;
		};
	}
}

#endif // !ASCENSION_MOUSE_INPUT_STRATEGY_HPP
