/**
 * @file font-description.hpp
 * @author exeal
 * @date 2010-11-06 created
 * @date 2010-2012 was font.hpp
 * @date 2012-08-19 separated from font.hpp
 */

#ifndef ASCENSION_FONT_DESCRIPTION_HPP
#define ASCENSION_FONT_DESCRIPTION_HPP

#include <ascension/graphics/font/font-family.hpp>
#include <ascension/graphics/font/font-properties.hpp>
#if ASCENSION_SUPPORTS_SHAPING_ENGINE(PANGO)
#	include <pangomm/fontdescription.h>
#endif
#if ASCENSION_SUPPORTS_SHAPING_ENGINE(UNISCRIBE) || ASCENSION_SUPPORTS_SHAPING_ENGINE(WIN32_GDI) || ASCENSION_SUPPORTS_SHAPING_ENGINE(WIN32_GDIPLUS)
#	include <ascension/win32/windows.hpp>
#endif

namespace ascension {
	namespace graphics {
		namespace font {
			/**
			 * @see FontProperties
			 */
			class FontDescription : private boost::equality_comparable<FontDescription> {
			public:
				/**
				 * Constructor.
				 * @param family The font family
				 * @param pointSize The size in points
				 * @param properties The other properties
				 * @throw std#underflow_error @a pointSize is negative
				 */
				FontDescription(const FontFamily& family,
						double pointSize, const FontProperties& properties = FontProperties())
						: family_(family), pointSize_(pointSize), properties_(properties) {
					if(pointSize < 0.0)
						throw std::underflow_error("pointSize");
				}
				/// Equality operator.
				bool operator==(const FontDescription& other) const BOOST_NOEXCEPT {
					return family_ == other.family_
						&& pointSize_ == other.pointSize_	// TODO: use epsilon.
						&& properties_ == other.properties_;
				}
				/// Returns the font family.
				const FontFamily& family() const BOOST_NOEXCEPT {return family_;}
				/// Returns the size in points.
				double pointSize() const BOOST_NOEXCEPT {return pointSize_;}
				/// Returns the other properties.
				FontProperties& properties() BOOST_NOEXCEPT {return properties_;}
				/// Returns the other properties.
				const FontProperties& properties() const BOOST_NOEXCEPT {return properties_;}
				/**
				 * Sets the family name.
				 * @param family The new font family
				 */
				FontDescription& setFamily(const FontFamily& family) {
					return (family_ = family), *this;
				}
				/**
				 * Sets the size in points.
				 * @param newValue The new size
				 * @return This object
				 * @throw std#underflow_error @a newValue is negative
				 */
				FontDescription& setPointSize(double newValue) {
					if(newValue < 0.0)
						throw std::underflow_error("newValue");
					return (pointSize_ = newValue), *this;
				}

			private:
				FontFamily family_;
				double pointSize_;
				FontProperties properties_;
			};

			/// Specialization of @c boost#hash_value function template for @c FontDescription.
			inline std::size_t hash_value(const FontDescription& object) BOOST_NOEXCEPT {
				std::size_t seed = 0;
				boost::hash_combine(seed, object.family());
				boost::hash_combine(seed, object.pointSize());
				boost::hash_combine(seed, object.properties());
				return seed;
			}
		}

		namespace detail {
#if ASCENSION_SUPPORTS_SHAPING_ENGINE(CORE_GRAPHICS)
#endif
#if ASCENSION_SUPPORTS_SHAPING_ENGINE(CORE_TEXT)
			template<typename T> T fromNative(const CTFontDescriptor& object);
			template font::FontDescription fromNative<font::FontDescription>(const CTFontDescriptor& object);
			CTFontDescriptor toNative(const font::FontDescription& object, const QFontInfo* = nullptr);
#endif
#if ASCENSION_SUPPORTS_SHAPING_ENGINE(DIRECT_WRITE)
#endif
#if ASCENSION_SUPPORTS_SHAPING_ENGINE(HARFBUZZ)
#endif
#if ASCENSION_SUPPORTS_SHAPING_ENGINE(PANGO)
			template<typename T> T fromNative(const Pango::FontDescription& object);
//			template font::FontDescription fromNative<font::FontDescription>(const Pango::FontDescription& object);
			Pango::FontDescription toNative(const font::FontDescription& object, const Pango::FontDescription* = nullptr);
#endif
#if ASCENSION_SUPPORTS_SHAPING_ENGINE(QT)
			template<typename T> T fromNative(const QFontInfo& object);
			template font::FontDescription fromNative<font::FontDescription>(const QFontInfo& object);
			QFontInfo toNative(const font::FontDescription& object, const QFontInfo* = nullptr);
#endif
#if ASCENSION_SUPPORTS_SHAPING_ENGINE(UNISCRIBE) || ASCENSION_SUPPORTS_SHAPING_ENGINE(WIN32_GDI) || ASCENSION_SUPPORTS_SHAPING_ENGINE(WIN32_GDIPLUS)
			template<typename T> T fromNative(const LOGFONTW& object);
//			template font::FontDescription fromNative<font::FontDescription>(const LOGFONTW& object);
			LOGFONTW toNative(const font::FontDescription& object, const LOGFONTW* = nullptr);
#endif
		}
	}
}

namespace std {
	/// Specialization of @c std#hash class template for @c FontDescription.
	template<>
	class hash<ascension::graphics::font::FontDescription> :
		public std::function<std::hash<void*>::result_type(const ascension::graphics::font::FontDescription&)> {
	public:
		result_type operator()(const argument_type& key) const BOOST_NOEXCEPT {
			return boost::hash<argument_type>()(key);
		}
	};
}

#endif // !ASCENSION_FONT_DESCRIPTION_HPP
