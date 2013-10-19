/**
 * @file paint-gdi.cpp
 * Implements painting interface on GDI graphics system.
 * @author exeal
 * @date 2012-06-17 Created
 */

#include <ascension/graphics/paint.hpp>
#ifdef ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI

using namespace ascension::graphics;
using namespace std;

Paint::~Paint() BOOST_NOEXCEPT {
	switch(nativeObject_.lbStyle) {
		case BS_DIBPATTERN:
		case BS_DIBPATTERN8X8:
		case BS_DIBPATTERNPT:
			::GlobalFree(reinterpret_cast<HGLOBAL>(nativeObject_.lbHatch));
			break;
		case BS_PATTERN:
		case BS_PATTERN8X8:
			::DeleteObject(reinterpret_cast<HBITMAP>(nativeObject_.lbHatch));
			break;
	}
}

SolidColor::SolidColor(const Color& color) : color_(color) {
	LOGBRUSH brush;
	brush.lbStyle = BS_SOLID;
	brush.lbColor = color.as<COLORREF>();
	reset(move(brush));
}

#endif // !ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI
