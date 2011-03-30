/**
 * @file window-windows.hpp
 * @date 2002-2010
 * @date 2010-10-27 renamed from "window.hpp"
 */

#ifndef ASCENSION_WINDOW_WINDOWS_HPP
#define ASCENSION_WINDOW_WINDOWS_HPP
#include <ascension/viewer/base/window.hpp>
#include <ascension/viewer/base/widget-windows.hpp>

namespace ascension {
	namespace win32 {

		template<typename Derived>
		class Window : public viewers::base::Window, public Widget<Derived> {
		};

	}
} // namespace ascension.win32

#endif // !ASCENSION_WINDOW_WINDOWS_HPP
