/**
 * @file widget
 * @author exeal
 * @date 2011-03-27
 */

#ifndef ASCENSION_WIDGET_HPP
#define ASCENSION_WIDGET_HPP
#include <ascension/platforms.hpp>
#if defined(ASCENSION_WINDOWS)
#	include <ascension/win32/windows.hpp>
#endif
#include <ascension/corelib/basic-exceptions.hpp>	// IllegalStateException
#include <ascension/graphics/geometry.hpp>
#include <ascension/graphics/graphics.hpp>			// graphics.Device, ...

namespace ascension {

	namespace graphics {
		class PaintContext;
	}

	namespace viewers {
		namespace base {

			/**
			 * Thrown by a window object when the method should be called after the initialization.
			 * @see Widget
			 */
			class WidgetNotInitializedException : public IllegalStateException {
			public:
				/// Default constructor.
				WidgetNotInitializedException() /*throw()*/
					: IllegalStateException("this widget is not initialized.") {}
			};

#if defined(ASCENSION_WINDOWS)
			typedef win32::Handle<HWND> NativeWidget;
#endif

			class Widget {
			public:
				virtual ~Widget();

				virtual void initialize(Widget& parent, const graphics::Rect<>& bounds) = 0;

				virtual graphics::Rect<> bounds(bool includeFrame) const = 0;
				virtual void setBounds(const graphics::Rect<>& bounds) = 0;
				virtual void setShape(const graphics::NativePolygon& shape) = 0;
 
				virtual void close() = 0;
				virtual void show() = 0;
				virtual void hide() = 0;

				virtual void forcePaint(const graphics::Rect<>& bounds) = 0;

				virtual void setOpacity(double opacity) = 0;
				virtual void setAlwaysOnTop(bool set) = 0;

				virtual bool isVisible() const = 0;
				virtual bool isActive() const = 0;

				virtual void paint(graphics::PaintContext& context) = 0;
			};

		}
	}
}

#endif // !ASCENSION_WIDGET_HPP
