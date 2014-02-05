/**
 * @file cursor-windows.cpp
 * Implements @c ascension#viewers#widgetapi#Cursor class on Win32 window system.
 * @date 2014-02-01 Created.
 */

#include <ascension/viewer/widgetapi/cursor.hpp>
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32

namespace ascension {
	namespace viewers {
		namespace widgetapi {
			Cursor::Cursor(Cursor::BuiltinShape shape) {
				if(!IS_INTRESOURCE(shape))
					throw std::invalid_argument("shape");
				impl_.reset(static_cast<HCURSOR>(::LoadImageW(nullptr, shape, IMAGE_CURSOR, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE | LR_SHARED)), detail::NullDeleter());
			}

			Cursor::Cursor(const Cursor& other) : impl_(::CopyCursor(other.impl_), &::DestroyCursor) {
				// TODO: MSDN says "Do not use the CopyCursor function for animated cursor."
				if(impl_.get() == nullptr)
					throw makePlatformError();
			}
		}
	}
}

#endif // ASCENSION_WINDOW_SYSTEM_WIN32