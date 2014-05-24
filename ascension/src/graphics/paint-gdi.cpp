/**
 * @file paint-gdi.cpp
 * Implements painting interface on GDI graphics system.
 * @author exeal
 * @date 2012-06-17 Created
 * @date 2012, 2014
 */

#include <ascension/graphics/paint.hpp>
#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI)

namespace ascension {
	namespace graphics {
		namespace {
			void releaseBrush(LOGBRUSH& brush) BOOST_NOEXCEPT {
				switch(brush.lbStyle) {
					case BS_DIBPATTERN:
					case BS_DIBPATTERN8X8:
					case BS_DIBPATTERNPT:
						::GlobalFree(reinterpret_cast<HGLOBAL>(brush.lbHatch));
						break;
					case BS_PATTERN:
					case BS_PATTERN8X8:
						::DeleteObject(reinterpret_cast<HBITMAP>(brush.lbHatch));
						break;
				}
			}
		}

		Paint::~Paint() BOOST_NOEXCEPT {
			releaseBrush(nativeObject_);
		}

		void Paint::reset(LOGBRUSH&& nativeObject) BOOST_NOEXCEPT {
			std::swap(nativeObject_, nativeObject);
			releaseBrush(nativeObject);
		}

		SolidColor::SolidColor(const Color& color) : color_(color) {
			LOGBRUSH brush;
			brush.lbStyle = BS_SOLID;
			brush.lbColor = color.as<COLORREF>();
			reset(std::move(brush));
		}
	}
}

#endif // !ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI)
