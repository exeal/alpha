/**
 * @file system-default-font.hpp
 * @author exeal
 * @date 2016-07-17 Created.
 */

#ifndef ASCENSION_WIN32_SYSTEM_DEFAULT_FONT_HPP
#define ASCENSION_WIN32_SYSTEM_DEFAULT_FONT_HPP
#include <ascension/win32/windows.hpp>

namespace ascension {
	namespace win32 {
		inline void systemDefaultFont(LOGFONTW& out) {
			LOGFONTW lf;
			if(::GetObjectW(static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT)), sizeof(decltype(lf)), &lf) == 0) {
				win32::AutoZeroSize<NONCLIENTMETRICSW> ncm;
				if(!win32::boole(::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(decltype(ncm)), &ncm, 0)))
					throw makePlatformError();
				lf = ncm.lfMessageFont;
			}
			std::swap(lf, out);
		}
	}
}

#endif // !ASCENSION_WIN32_SYSTEM_DEFAULT_FONT_HPP
