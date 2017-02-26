/**
 * @file cursor-windows.cpp
 * Implements @c ascension#viewer#widgetapi#Cursor class on Win32 window system.
 * @date 2014-02-01 Created.
 */

#include <ascension/corelib/native-conversion.hpp>
#include <ascension/graphics/geometry/native-conversions.hpp>
#include <ascension/viewer/widgetapi/cursor.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)

namespace ascension {
	namespace viewer {
		namespace widgetapi {
			Cursor::Cursor(Cursor::BuiltinShape shape) {
				if(!IS_INTRESOURCE(shape))
					throw std::invalid_argument("shape");
				native_ = win32::borrowed(static_cast<HCURSOR>(::LoadImageW(nullptr, shape, IMAGE_CURSOR, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE | LR_SHARED)));
			}

			Cursor::Cursor(const Cursor& other) : native_(::CopyIcon(other.native_.get()), &::DestroyCursor) {
				// TODO: MSDN says "Do not use the CopyCursor function for animated cursor."
				if(native_.get() == nullptr)
					throw makePlatformError();
			}

			graphics::Point Cursor::position() {
				POINT p;
				if(!win32::boole(::GetCursorPos(&p)))
					throw makePlatformError();
				return fromNative<graphics::Point>(p);
			}

			graphics::Point Cursor::position(Proxy<const Window> window) {
				POINT p;
				if(!win32::boole(::GetCursorPos(&p)) || !win32::boole(::ScreenToClient(window->handle().get(), &p)))
					throw makePlatformError();
				return fromNative<graphics::Point>(p);
			}
		}
	}
}

#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
