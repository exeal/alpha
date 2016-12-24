/**
 * @file cursor-windows.cpp
 * Implements @c ascension#viewer#widgetapi#Cursor class on Win32 window system.
 * @date 2014-02-01 Created.
 */

#include <ascension/viewer/widgetapi/cursor.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)

namespace ascension {
	namespace viewer {
		namespace widgetapi {
			Cursor::Cursor(Cursor::BuiltinShape shape) {
				if(!IS_INTRESOURCE(shape))
					throw std::invalid_argument("shape");
				impl_ = win32::borrowed(static_cast<HCURSOR>(::LoadImageW(nullptr, shape, IMAGE_CURSOR, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE | LR_SHARED)));
			}

			Cursor::Cursor(const Cursor& other) : impl_(::CopyIcon(other.impl_.get()), &::DestroyCursor) {
				// TODO: MSDN says "Do not use the CopyCursor function for animated cursor."
				if(impl_.get() == nullptr)
					throw makePlatformError();
			}
		}
	}
}

#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
