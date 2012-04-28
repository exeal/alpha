/**
 * @file viewer-observers.hpp
 * @author exeal
 * @date 2011-03-30 separated from viewer.hpp
 */

#ifndef VIEWER_OBSERVERS_HPP
#define VIEWER_OBSERVERS_HPP
#include <ascension/graphics/geometry.hpp>	// graphics.NativeSize
#include <ascension/platforms.hpp>
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32)
#	include <ascension/win32/windows.hpp>
#endif

namespace ascension {
	namespace viewers {

		class TextViewer;

		/**
		 * Interface for objects which are interested in change of scroll positions of a
		 * @c TextViewer.
		 * @see TextViewer#addViewportListener, TextViewer#removeViewportListener
		 */
		class ViewportListener {
		private:
			/**
			 * The scroll positions of the viewer were changed.
			 * @param horizontal @c true if the horizontal scroll position is changed
			 * @param vertical @c true if the vertical scroll position is changed
			 * @see TextViewer#firstVisibleLine
			 */
			virtual void viewportChanged(bool horizontal, bool vertical) = 0;
			friend class TextViewer;
		};
		
		/**
		 * Interface for objects which are interested in change of size of a @c TextViewer.
		 * @see TextViewer#addDisplaySizeListener, TextViewer#removeDisplaySizeListener
		 */
		class DisplaySizeListener {
		private:
			/// The size of the viewer was changed.
			virtual void viewerDisplaySizeChanged() = 0;
			friend class TextViewer;
		};
#if 0
		/**
		 * Interface of objects which define how the text editors react to the users' keyboard
		 * input. Ascension also provides the standard implementation of this interface
		 * @c DefaultKeyboardInputStrategy.
		 * @see TextViewer#set
		 */
		class KeyboardInputStrategy {
		};
#endif
		namespace widgetapi {
			class DropTarget;
			class LocatedUserInput;
			class MouseButtonInput;
			class MouseWheelInput;
		}

		/**
		 * Interface of objects which define how the text editors react to the users' mouse input.
		 * @note An instance of @c MouseInputStrategy can't be shared multiple text viewers.
		 * @see TextViewer#setMouseInputStrategy
		 */
		class MouseInputStrategy {
		public:
			/// Destructor.
			virtual ~MouseInputStrategy() /*throw()*/ {}
		protected:
			/// Actions of the mouse input.
			enum Action {
				PRESSED,		///< The button was pressed (down).
				RELEASED,		///< The button was released (up).
				DOUBLE_CLICKED	///< The button was double-clicked.
			};
		private:
			/// The viewer lost the mouse capture.
			virtual void captureChanged() = 0;
			/// Returns @c DropTarget if this object supports the interface, or @c null.
			virtual std::shared_ptr<base::DropTarget> handleDropTarget() const = 0;
			/**
			 * Installs the strategy.
			 * @param viewer The text viewer uses the strategy. The window had been created at this
			 *               time
			 */
			virtual void install(TextViewer& viewer) = 0;
			/**
			 * Interrupts the progressive mouse reaction.
			 * This method must be called before @c #uninstall call.
			 * @param forKeyboardInput @c true if the mouse reaction should interrupt because the
			 *                         keyboard input was occurred
			 */
			virtual void interruptMouseReaction(bool forKeyboardInput) = 0;
			/**
			 * The mouse input was occurred and the viewer had focus.
			 * @param action The action of the input
			 * @param input The input information
			 * @return @c true if the strategy processed
			 */
			virtual bool mouseButtonInput(Action action, const base::MouseButtonInput& input) = 0;
			/**
			 * The mouse was moved and the viewer had focus.
			 * @param input The input information
			 * @return @c true if the strategy processed
			 */
			virtual void mouseMoved(const base::LocatedUserInput& input) = 0;
			/**
			 * The mouse wheel was rolated and the viewer had focus.
			 * @param input The input information
			 */
			virtual void mouseWheelRotated(const base::MouseWheelInput& input) = 0;
			/**
			 * Shows a cursor on the viewer.
			 * @param position The cursor position (client coordinates)
			 * @retval @c true if the callee showed a cursor
			 * @retval @c false if the callee did not know the appropriate cursor
			 */
			virtual bool showCursor(const graphics::NativePoint& position) = 0;
			/// Uninstalls the strategy. The window is not destroyed yet at this time.
			virtual void uninstall() = 0;
			friend class TextViewer;
		};

	}

	namespace detail {
		class InputEventHandler {	// this is not an observer of caret...
		private:
			virtual void abortInput() = 0;
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32)
			virtual LRESULT handleInputEvent(UINT message, WPARAM wp, LPARAM lp, bool& consumed) = 0;
#endif
			friend class viewers::TextViewer;
		};
	}
}

#endif // !VIEWER_OBSERVERS_HPP
