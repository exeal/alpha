/**
 * @file paint-cairo.cpp
 * Implements painting interface on Cairo graphics system.
 * @author exeal
 * @date 2013-10-19 Created
 * @date 2014
 */

#include <ascension/graphics/paint.hpp>
#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(CAIRO)


namespace ascension {
	namespace graphics {
		Paint::~Paint() BOOST_NOEXCEPT {
			// does nothing on Cairo graphics system
		}

		SolidColor::SolidColor(const Color& color) : color_(color) {
			const GdkRGBA temp(color.as<GdkRGBA>());
			reset(Cairo::SolidPattern::create_rgba(temp.red, temp.green, temp.blue, temp.alpha));
		}

		LinearGradient::LinearGradient(const Point& p0, const Point& p1) {
			reset(Cairo::LinearGradient::create(geometry::x(p0), geometry::y(p0), geometry::x(p1), geometry::y(p1)));
		}

		RadialGradient::RadialGradient(const Point& p0, Scalar r0, const Point& p1, Scalar r1) {
			if(r0 < 0)
				throw std::out_of_range("r0");
			else if(r1 < 0)
				throw std::out_of_range("r1");
			reset(Cairo::RadialGradient::create(geometry::x(p0), geometry::y(p0), r0, geometry::x(p1), geometry::y(p1), r1));
		}
	}
}

#endif	// ASCENSION_SELECTS_GRAPHICS_SYSTEM(CAIRO)
