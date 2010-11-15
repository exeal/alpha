/**
 * @file window.hpp
 * @author exeal
 * @date 2010-10-27 created
 */

#include <ascension/corelib/basic-exceptions.hpp>	// IllegalStateException
#include <ascension/graphics/graphics.hpp>			// graphics.Device, ...
#include <ascension/viewer/base/user-input.hpp>

namespace ascension {

	namespace graphics {class Context;}

	namespace viewers {
		namespace base {

			/**
			 * Thrown by a window object when the method should be called after the initialization.
			 * @see Window
			 */
			class WindowNotInitializedException : public IllegalStateException {
			public:
				/// Default constructor.
				WindowNotInitializedException() /*throw()*/
					: IllegalStateException("this window is not initialized.") {}
			};

			class Window : public graphics::Device {
			public:
				virtual ~Window() /*throw()*/;
			public:
				virtual graphics::Rect<> bounds(bool includeFrame) const = 0;
	//			virtual void forceRedraw(const graphics::Rect<>& rect) = 0;
				virtual bool hasFocus() const = 0;
				/// Hides the window without deactivation.
				virtual void hide() = 0;
				virtual bool isVisible() const = 0;
				virtual void paint(graphics::Context& context) = 0;
				virtual void setBounds(const graphics::Rect<>& bounds) = 0;
				/// Shows the window without activation.
				virtual void show() = 0;
			protected:
				virtual bool aboutToLoseFocus() {return false;}
				virtual bool focusGained() {return false;}
				virtual bool keyPressed(const KeyInput& input) {return false;}
				virtual bool keyReleased(const KeyInput& input) {return false;}
				virtual bool mouseDoubleClicked(const MouseButtonInput& input) {return false;}
				virtual bool mouseMoved(const LocatedUserInput& input) {return false;}
				virtual bool mousePressed(const MouseButtonInput& input) {return false;}
				virtual bool mouseReleased(const MouseButtonInput& input) {return false;}
				virtual bool mouseWheelChanged(const MouseWheelInput& input) {return false;}
				virtual void paint(const graphics::PaintContext& context) = 0;
			};

		}
	}
}