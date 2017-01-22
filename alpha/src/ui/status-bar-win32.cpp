/**
 * @file ui/status-bar-win32.cpp
 * @author exeal
 * @date 2003-2009, 2014 (was application.cpp)
 * @date 2014-03-29 Separated from application.cpp
 * @date 2017-01-21 Renamed from status-bar.cpp.
 */

#include "ui/status-bar.hpp"
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include "ui/main-window.hpp"
#	include <CommCtrl.h>

namespace alpha {
	namespace ui {
		StatusBar::StatusBar() : ascension::win32::Window(STATUSCLASSNAMEW, ascension::win32::Window::WIDGET) {
			const auto styles = ascension::win32::getWindowLong(handle().get(), GWL_STYLE);
			ascension::win32::setWindowLong(handle().get(), GWL_STYLE, styles | WS_VISIBLE | CCS_BOTTOM | SBARS_SIZEGRIP);
		}

		bool StatusBar::isSimple() const BOOST_NOEXCEPT {
			return ascension::win32::boole(::SendMessageW(handle().get(), SB_ISSIMPLE, 0, 0L));
		}

		void StatusBar::setSimple(bool simple) {
			::SendMessageW(handle().get(), SB_SIMPLE, simple ? TRUE : FALSE, 0L);
		}
	}
}

#endif
