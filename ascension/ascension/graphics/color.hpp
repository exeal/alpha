/**
 * @file color.hpp
 * Defines basic data types in @c ascension#graphics namespace.
 * @author exeal
 * @date 2010-11-06 created
 */

#ifndef ASCENSION_COLOR_HPP
#define ASCENSION_COLOR_HPP

#include <ascension/platforms.hpp>
#include <ascension/corelib/memory.hpp>		// FastArenaObject
#if defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
#	include <ascension/win32/windows.hpp>	// COLORREF
#endif

namespace ascension {
	namespace graphics {

		/**
		 * @c Color provides colors based on RGB values.
		 * @see "CSS Color Module Level 3" (http://www.w3.org/TR/2010/PR-css3-color-20101028)
		 */
		class Color : public FastArenaObject<Color> {
		public:
			static const Color TRANSPARENT_COLOR;
		public:
			/// Creates an invalid @c Color object.
			Color() /*throw()*/ : valid_(false) {}
			/// Creates a color value based on RGB values.
			Color(byte red, byte green, byte blue, byte alpha = 255) /*throw()*/
				: red_(red << 8), green_(green << 8), blue_(blue << 8), alpha_(alpha << 8), valid_(true) {}
#ifdef ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI
			/// Creates an object from Win32 @c COLORREF value.
			static Color fromCOLORREF(COLORREF value) /*throw()*/ {
				return Color(
					static_cast<byte>(value & 0xff),
					static_cast<byte>((value >> 8) & 0xff),
					static_cast<byte>((value >> 16) & 0xff));
			}
			/// Returns a win32 @c COLORREF value represents this color.
			COLORREF asCOLORREF() const /*throw()*/ {return RGB(red(), green(), blue());}
#endif // ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI
			/// Returns the blue color component of this color.
			byte blue() const /*throw()*/ {return blue_ >> 8;}
			/// Returns the green color component of this color.
			byte green() const /*throw()*/ {return green_ >> 8;}
			/// Returns the red color component of this color.
			byte red() const /*throw()*/ {return red_ >> 8;}
			/// Returns the alpha value of this color.
			byte alpha() const /*throw()*/ {return alpha_ >> 8;}
			/// Returns @c true if this color is transparent.
			bool isTransparent() const /**/ {return alpha() == 0;}
			/// Equality operator.
			bool operator==(const Color& other) const /*throw()*/ {
				return valid_ == other.valid_
					&& (!valid_ || (red() == other.red() && green() == other.green()
						&& blue() == other.blue() && alpha() == other.alpha()));
			}
			/// Inequality operator.
			bool operator!=(const Color& other) const /*throw()*/ {return !(*this == other);}
		private:
			uint16_t red_, green_, blue_, alpha_;
			bool valid_;
		};

	}
}

#endif // !ASCENSION_COLOR_HPP
