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
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(CAIRO)
#	include <gdkmm/rgba.h>					// Gdk.RGBA
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(WIN32_GDI)
#	include <ascension/win32/windows.hpp>	// COLORREF
#endif
#include <boost/math/special_functions/round.hpp>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

namespace ascension {
	namespace graphics {
		/**
		 * @c Color provides colors based on RGB values.
		 * @see "CSS Color Module Level 3" (http://www.w3.org/TR/css3-color/)
		 */
		class Color : public FastArenaObject<Color>, private boost::equality_comparable<Color> {
		public:
			static const Color TRANSPARENT_BLACK;
			static const Color OPAQUE_BLACK, OPAQUE_WHITE;
		public:
			/// Creates a color value based on RGB values.
			Color(Byte red, Byte green, Byte blue, Byte alpha = 255) BOOST_NOEXCEPT
				: red_(red * 0x0101), green_(green * 0x0101), blue_(blue * 0x0101), alpha_(alpha * 0x0101) {}
			/// Converts into a native value.
			template<typename NativeType> NativeType as() const;
			/// Creates a @c Color object from native value.
			template<typename NativeType> static Color from(const NativeType& value) BOOST_NOEXCEPT;
			/// Returns the blue color component of this color.
			Byte blue() const BOOST_NOEXCEPT {return blue_ >> 8;}
			/// Returns the green color component of this color.
			Byte green() const BOOST_NOEXCEPT {return green_ >> 8;}
			/// Returns the red color component of this color.
			Byte red() const BOOST_NOEXCEPT {return red_ >> 8;}
			/// Returns the alpha value of this color.
			Byte alpha() const BOOST_NOEXCEPT {return alpha_ >> 8;}
			/// Returns @c true if this color is fully opaque.
			bool isFullyOpaque() const BOOST_NOEXCEPT {return alpha() == 255;}
			/// Returns @c true if this color is fully transparent.
			bool isFullyTransparent() const BOOST_NOEXCEPT {return alpha() == 0;}
			/// Returns @c true if this color is transparent.
			bool isTransparent() const BOOST_NOEXCEPT {return !isFullyOpaque();}
			/// Equality operator.
			bool operator==(const Color& other) const BOOST_NOEXCEPT {
				return red() == other.red() && green() == other.green()
					&& blue() == other.blue() && alpha() == other.alpha();
			}

		private:
			std::uint16_t red_, green_, blue_, alpha_;	// pre-multiplied values
		};

#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(CAIRO)
		template<> inline Gdk::RGBA Color::as<Gdk::RGBA>() const BOOST_NOEXCEPT {
			Gdk::RGBA temp;
			temp.set_red(red_ / static_cast<double>(std::numeric_limits<uint16_t>::max()));
			temp.set_green(green_ / static_cast<double>(std::numeric_limits<uint16_t>::max()));
			temp.set_blue(blue_ / static_cast<double>(std::numeric_limits<uint16_t>::max()));
			temp.set_alpha(alpha_ / static_cast<double>(std::numeric_limits<uint16_t>::max()));
			return temp;
		}
		template<> inline Color Color::from<Gdk::RGBA>(const Gdk::RGBA& value) BOOST_NOEXCEPT {
			return Color(
				static_cast<Byte>(boost::math::iround(value.get_red() * static_cast<double>(std::numeric_limits<uint16_t>::max()))),
				static_cast<Byte>(boost::math::iround(value.get_green() * static_cast<double>(std::numeric_limits<uint16_t>::max()))),
				static_cast<Byte>(boost::math::iround(value.get_blue() * static_cast<double>(std::numeric_limits<uint16_t>::max()))),
				static_cast<Byte>(boost::math::iround(value.get_alpha() * static_cast<double>(std::numeric_limits<uint16_t>::max()))));
		}
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(CORE_GRAPHICS)
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(DIRECT2D)
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(QT)
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(WIN32_GDI)
		template<> inline COLORREF Color::as<COLORREF>() const {
			return RGB(red(), green(), blue());
		}
		template<> inline RGBQUAD Color::as<RGBQUAD>() const {
			RGBQUAD temp;
			temp.rgbRed = red();
			temp.rgbGreen = green();
			temp.rgbBlue = blue();
			temp.rgbReserved = alpha();
			return temp;
		}
		template<> inline Color Color::from<COLORREF>(const COLORREF& value) BOOST_NOEXCEPT {
			return Color(
				static_cast<Byte>(value & 0xff),
				static_cast<Byte>((value >> 8) & 0xff),
				static_cast<Byte>((value >> 16) & 0xff));
		}
		template<> inline Color Color::from<RGBQUAD>(const RGBQUAD& value) BOOST_NOEXCEPT {
			return Color(value.rgbRed, value.rgbGreen, value.rgbBlue, value.rgbReserved);
		}
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(WIN32_GDIPLUS)
#endif

		/**
		 * Provides color values defined by operating system (themes).
		 * @see "CSS Color Module Level3, 4.5. CSS system colors"
		 *      (http://www.w3.org/TR/css3-color/#css-system)
		 * @note "CSS Color Module Level 4" deprecates system colors.
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
				INACTIVE_CAPTION,
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

			/**
			 * Returns the specified system color.
			 * @param value A value
			 * @return A system color or @c #boost#none if @a value is not defined by the operating system (theme).
			 * @throw UnknownValueException @a value is not valid
			 */
			static boost::optional<Color> get(Value value);	// defined in viewer/widgetapi/widget-*.cpp
		};
	}
}

#endif // !ASCENSION_COLOR_HPP
