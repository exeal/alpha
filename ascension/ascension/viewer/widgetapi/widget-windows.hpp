/**
 * @file widget-windows.hpp
 * @author exeal
 * @date 2012-04-29 created
 */

#ifndef ASCENSION_WIDGET_WINDOWS_HPP
#define ASCENSION_WIDGET_WINDOWS_HPP

namespace ascension {
	namespace win32 {
		inline Handle<HIMC>::Type inputMethod(const viewer::widgetapi::NativeWidget& widget) {
			return Handle<HIMC>::Type(::ImmGetContext(widget.handle().get()),
				std::bind(&::ImmReleaseContext, widget.handle().get(), std::placeholders::_1));
		}
	}

	namespace viewer {
		namespace widgetapi {
		}
	}
}

#endif // !ASCENSION_WIDGET_WINDOWS_HPP
