/**
 * @file graphics/color.hpp
 * Defines basic data types in @c ascension#graphics namespace.
 * @author exeal
 * @date 2010-11-06 created
 */

#ifndef ASCENSION_GRAPHICS_COLOR_HPP
#define ASCENSION_GRAPHICS_COLOR_HPP
#include <ascension/platforms.hpp>
#include <ascension/corelib/basic-types.hpp>
#include <ascension/corelib/memory.hpp>		// FastArenaObject
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(CAIRO)
#	include <gdkmm/rgba.h>					// Gdk.RGBA
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(WIN32_GDI)
#	include <ascension/win32/windows.hpp>	// COLORREF
#endif
#include <boost/functional/hash.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <functional>	// std.hash

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
			/// Creates a color value with uninitialized components.
			Color() BOOST_NOEXCEPT {}
			/// Creates a color value based on RGB values.
			BOOST_CONSTEXPR Color(Byte red, Byte green, Byte blue, Byte alpha = 255) BOOST_NOEXCEPT
				: red_(red * 0x0101), green_(green * 0x0101), blue_(blue * 0x0101), alpha_(alpha * 0x0101) {}
			/// Returns the blue color component of this color.
			BOOST_CONSTEXPR Byte blue() const BOOST_NOEXCEPT {return blue_ >> 8;}
			/// Returns the green color component of this color.
			BOOST_CONSTEXPR Byte green() const BOOST_NOEXCEPT {return green_ >> 8;}
			/// Returns the red color component of this color.
			BOOST_CONSTEXPR Byte red() const BOOST_NOEXCEPT {return red_ >> 8;}
			/// Returns the alpha value of this color.
			BOOST_CONSTEXPR Byte alpha() const BOOST_NOEXCEPT {return alpha_ >> 8;}
			/// Returns @c true if this color is fully opaque.
			BOOST_CONSTEXPR bool isFullyOpaque() const BOOST_NOEXCEPT {return alpha() == 255;}
			/// Returns @c true if this color is fully transparent.
			BOOST_CONSTEXPR bool isFullyTransparent() const BOOST_NOEXCEPT {return alpha() == 0;}
			/// Returns @c true if this color is transparent.
			BOOST_CONSTEXPR bool isTransparent() const BOOST_NOEXCEPT {return !isFullyOpaque();}
			/// Equality operator.
			BOOST_CONSTEXPR bool operator==(const Color& other) const BOOST_NOEXCEPT {
				return red() == other.red() && green() == other.green()
					&& blue() == other.blue() && alpha() == other.alpha();
			}

		private:
			std::uint16_t red_, green_, blue_, alpha_;	// pre-multiplied values
		};

		// platform-dependent conversions
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(CAIRO)
		inline Color _fromNative(const Gdk::RGBA& native, const Color* = nullptr) {
			if(native.get_red() > 1.0 || native.get_green() > 1.0 || native.get_blue() > 1.0 || native.get_alpha() > 1.0)
				throw std::overflow_error("native");
			return Color(
				static_cast<Byte>(boost::math::iround(native.get_red() * static_cast<double>(std::numeric_limits<Byte>::max()))),
				static_cast<Byte>(boost::math::iround(native.get_green() * static_cast<double>(std::numeric_limits<Byte>::max()))),
				static_cast<Byte>(boost::math::iround(native.get_blue() * static_cast<double>(std::numeric_limits<Byte>::max()))),
				static_cast<Byte>(boost::math::iround(native.get_alpha() * static_cast<double>(std::numeric_limits<Byte>::max()))));
		}
		inline Gdk::RGBA _toNative(const Color& from, const Gdk::RGBA* = nullptr) BOOST_NOEXCEPT {
			Gdk::RGBA temp;
			temp.set_rgba(
				from.red() / static_cast<double>(std::numeric_limits<Byte>::max()),
				from.green() / static_cast<double>(std::numeric_limits<Byte>::max()),
				from.blue() / static_cast<double>(std::numeric_limits<Byte>::max()),
				from.alpha() / static_cast<double>(std::numeric_limits<Byte>::max()));
			return temp;
		}
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(CORE_GRAPHICS)
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(DIRECT2D)
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(QT)
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(WIN32_GDI)
		inline Color _fromNative(const COLORREF& native, const Color* = nullptr) BOOST_NOEXCEPT {
			return Color(
				static_cast<Byte>(native & 0xff),
				static_cast<Byte>((native >> 8) & 0xff),
				static_cast<Byte>((native >> 16) & 0xff));
		}
		inline Color _fromNative(const RGBQUAD& native, const Color* = nullptr) BOOST_NOEXCEPT {
			return Color(native.rgbRed, native.rgbGreen, native.rgbBlue, native.rgbReserved);
		}
		inline COLORREF _toNative(const Color& from, const COLORREF* = nullptr) BOOST_NOEXCEPT {
			return RGB(from.red(), from.green(), from.blue());
		}
		inline RGBQUAD _toNative(const Color& from, const RGBQUAD* = nullptr) BOOST_NOEXCEPT {
			RGBQUAD temp;
			temp.rgbRed = from.red();
			temp.rgbGreen = from.green();
			temp.rgbBlue = from.blue();
			temp.rgbReserved = from.alpha();
			return temp;
		}
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(WIN32_GDIPLUS)
#endif

		/// Specialization of @c boost#hash_value function template for @c Color.
		inline std::size_t hash_value(const Color& object) BOOST_NOEXCEPT {
			std::size_t seed = 0;
			boost::hash_combine(seed, object.blue());
			boost::hash_combine(seed, object.green());
			boost::hash_combine(seed, object.red());
			boost::hash_combine(seed, object.alpha());
			return seed;
		}

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

namespace std {
	/// Specialization of @c std#hash class template for @c Color.
	template<>
	struct hash<ascension::graphics::Color> : public std::function<std::hash<void*>::result_type(const ascension::graphics::Color&)> {
	public:
		result_type operator()(const argument_type& key) const BOOST_NOEXCEPT {
			return boost::hash<argument_type>()(key);
		}
	};
}

#endif // !ASCENSION_GRAPHICS_COLOR_HPP
