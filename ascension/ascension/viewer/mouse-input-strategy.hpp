/**
 * @file mouse-input-strategy.hpp
 * @author exeal
 * @date 2011-03-30 separated from viewer.hpp
 * @date 2015-03-16 Separated from viewer-observers.hpp
 */

#ifndef ASCENSION_MOUSE_INPUT_STRATEGY_HPP
#define ASCENSION_MOUSE_INPUT_STRATEGY_HPP
#include <ascension/corelib/timer.hpp>
#include <ascension/graphics/geometry/point.hpp>
#include <boost/core/noncopyable.hpp>
#include <memory>

namespace ascension {
	namespace viewer {
		namespace widgetapi {
			class Cursor;
			class DropTarget;

			namespace event {
				class LocatedUserInput;
				class MouseButtonInput;
				class MouseWheelInput;
			}
		}

		/**
		 * Interface of objects which define how the text editors react to the users' mouse input.
		 * If the implementation of this interface other than @c TextArea didn't handle an event, the @c TextArea
		 * handles that event.
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

			/**
			 * Interface of objects which lock the target of mouse input.
			 * Implementation of @c MouseInputStrategy should not call @c widgetapi#grabInput function directly. Use
			 * this interface instead.
			 */
			class TargetLocker {
			public:
				/// Destructor.
				virtual ~TargetLocker() BOOST_NOEXCEPT {}
				/**
				 * Locks the target of mouse input. After the call, all mouse input are sent to this
				 * @c MouseInputStrategy object.
				 * @param The target which want to lock
				 * @return @c true if the lock succeeded. @c false if failed
				 * @throw std#bad_weak_ptr @a strategy is @c null
				 * @see #unlockMouseInputTarget
				 */
				virtual bool lockMouseInputTarget(std::weak_ptr<MouseInputStrategy> strategy) = 0;
				/**
				 * Unlocks the target of mouse input.
				 * @param The target to unlock
				 * @see #lockMouseInputTarget, MouseInputStrategy#interruptMouseReaction
				 */
				virtual void unlockMouseInputTarget(MouseInputStrategy& strategy) BOOST_NOEXCEPT = 0;
			};

			/// Destructor.
			virtual ~MouseInputStrategy() BOOST_NOEXCEPT {}
			/**
			 * Returns @c DropTarget if this object supports the interface, or @c null.
			 * Default implementation returns @c null.
			 */
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
			 * @param targetLocker The @c TargetLocker object
			 */
			virtual void mouseButtonInput(Action action, widgetapi::event::MouseButtonInput& input, TargetLocker& targetLocker) = 0;
			/**
			 * The lock by @c TargetLocker#lockMouseInputTarget method was revoked.
			 * @note The lock may be revoked regardless of call of @c TargetLocker#lockMouseInputTarget method. For
			 *       example, if the text viewer lost the input grab in the window system.
			 */
			virtual void mouseInputTargetUnlocked() = 0;
			/**
			 * The mouse was moved and the viewer had focus.
			 * @param input The input information
			 * @param targetLocker The @c TargetLocker object
			 */
			virtual void mouseMoved(widgetapi::event::LocatedUserInput& input, TargetLocker& targetLocker) = 0;
			/**
			 * The mouse wheel was rolated and the viewer had focus.
			 * @param input The input information
			 * @param targetLocker The @c TargetLocker object
			 */
			virtual void mouseWheelRotated(widgetapi::event::MouseWheelInput& input, TargetLocker& targetLocker) = 0;
			/**
			 * Shows a cursor on the viewer.
			 * @param position The cursor position (client coordinates)
			 * @retval @c true if the callee showed a cursor
			 * @retval @c false if the callee did not know the appropriate cursor
			 */
			virtual bool showCursor(const graphics::Point& position) = 0;
		};

		class TextViewer;

		/// Default implementation of @c MouseInputStrategy interface.
		class AbstractMouseInputStrategy : public MouseInputStrategy, private HasTimer, private boost::noncopyable {
		public:
			AbstractMouseInputStrategy() BOOST_NOEXCEPT;
			virtual ~AbstractMouseInputStrategy() BOOST_NOEXCEPT;
			virtual std::shared_ptr<widgetapi::DropTarget> handleDropTarget() const override;
			virtual void interruptMouseReaction(bool) override;
			virtual void mouseButtonInput(Action, widgetapi::event::MouseButtonInput& input, TargetLocker&) override;
			virtual void mouseInputTargetUnlocked() override;
			virtual void mouseMoved(widgetapi::event::LocatedUserInput& input, TargetLocker&) override;
			virtual void mouseWheelRotated(widgetapi::event::MouseWheelInput& input, TargetLocker&) override;
			virtual bool showCursor(const graphics::Point&) override;

		protected:
			/// @name
			/// @{
			void beginLocationTracking(TextViewer& viewer, TargetLocker& targetLocker, bool autoScroll, bool locateCursor);
			void endLocationTracking();
			bool isTrackingLocation() const BOOST_NOEXCEPT;
			virtual void trackedLocationChanged(const kernel::Position& position);
			/// @}

			/// @name
			/// @{
			bool showArrowCursor(TextViewer& viewer);
			bool showCursor(TextViewer& viewer, const widgetapi::Cursor& cursor);
			/// @}
			static const unsigned int SELECTION_EXPANSION_INTERVAL_IN_MILLISECONDS;	// TODO: Use std.chrono.
			static const unsigned int DRAGGING_TRACK_INTERVAL_IN_MILLISECONDS;	// TODO: Use std.chrono.

		private:
			void timeElapsed(Timer& timer) override;
			struct Tracking;
			std::unique_ptr<Tracking> tracking_;
		};
	}
}

#endif // !ASCENSION_MOUSE_INPUT_STRATEGY_HPP
