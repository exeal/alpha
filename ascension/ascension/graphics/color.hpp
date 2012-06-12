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
#include <boost/operators.hpp>

namespace ascension {
	namespace graphics {

		/**
		 * @c Color provides colors based on RGB values.
		 * @see "CSS Color Module Level 3" (http://www.w3.org/TR/css3-color/)
		 */
		class Color : public FastArenaObject<Color>, private boost::equality_comparable<Color> {
		public:
			static const Color TRANSPARENT_BLACK;
		public:
			/// Creates an invalid @c Color object.
			Color() /*noexcept*/ : valid_(false) {}
			/// Creates a color value based on RGB values.
			Color(Byte red, Byte green, Byte blue, Byte alpha = 255) /*noexcept*/
				: red_(red * 0x0101), green_(green * 0x0101), blue_(blue * 0x0101), alpha_(alpha * 0x0101), valid_(true) {}
			/// Converts into a native value.
			template<typename NativeType> NativeType as() const;
			/// Creates a @c Color object from native value.
			template<typename NativeType> static Color from(NativeType value) /*noexcept*/;
			/// Returns the blue color component of this color.
			Byte blue() const /*noexcept*/ {return blue_ >> 8;}
			/// Returns the green color component of this color.
			Byte green() const /*noexcept*/ {return green_ >> 8;}
			/// Returns the red color component of this color.
			Byte red() const /*noexcept*/ {return red_ >> 8;}
			/// Returns the alpha value of this color.
			Byte alpha() const /*noexcept*/ {return alpha_ >> 8;}
			/// Returns @c true if this color is transparent.
			bool isTransparent() const /*noexcept*/ {return alpha() == 0;}
			/// Equality operator.
			bool operator==(const Color& other) const /*noexcept*/ {
				return valid_ == other.valid_
					&& (!valid_ || (red() == other.red() && green() == other.green()
						&& blue() == other.blue() && alpha() == other.alpha()));
			}
		private:
			uint16_t red_, green_, blue_, alpha_;
			bool valid_;
		};

#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
#elif defined(ASCENSION_GRAPHICS_SYSTEM_DIRECT2D)
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
		template<> inline COLORREF Color::as<COLORREF>() const {
			return RGB(red(), green(), blue());
		}
		template<> inline Color Color::from<COLORREF>(COLORREF value) /*noexcept*/ {
			return Color(
				static_cast<Byte>(value & 0xff),
				static_cast<Byte>((value >> 8) & 0xff),
				static_cast<Byte>((value >> 16) & 0xff));
		}
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDIPLUS)
#endif

		/**
		 *
		 * @see "CSS Color Module Level3, 4.5. CSS system colors"
		 *      (http://www.w3.org/TR/css3-color/#css-system)
		 */
		class SystemColors {
		public:
			enum Value {
				ACTIVE_BORDER,
				ACTIVE_CAPTION,
				APP_WORKSPACE,
				BACKGROUND,
				BUTTON_FACE,
				BUTTON_HIGHLIGHT,
				BUTTON_SHADOW,
				BUTTON_TEXT,
				CAPTION_TEXT,
				GRAY_TEXT,
				HIGHLIGHT,
				HIGHLIGHT_TEXT,
				INACTIVE_BORDER,
				ANACTIVE_CAPTION,
				INACTIVE_CAPTION_TEXT,
				INFO_BACKGROUND,
				INFO_TEXT,
				MENU,
				MENU_TEXT,
				SCROLLBAR,
				THREE_D_DARK_SHADOW,
				THREE_D_FACE,
				THREE_D_HIGHLIGHT,
				THREE_D_LIGHT_SHADOW,
				THREE_D_SHADOW,
				WINDOW,
				WINDOW_FRAME,
				WINDOW_TEXT
			};
			static Color get(Value value);
		};

	}
}

#endif // !ASCENSION_COLOR_HPP
