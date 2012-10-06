/**
 * @file font-family.hpp
 * @author exeal
 * @date 2010-11-06 created
 * @date 2010-2012 was font.hpp
 * @date 2012-08-19 separated from font.hpp
 */

#ifndef ASCENSION_FONT_FAMILY_HPP
#define ASCENSION_FONT_FAMILY_HPP

#include <ascension/config.hpp>	// ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
#include <ascension/platforms.hpp>
#include <ascension/corelib/basic-exceptions.hpp>	// UnknownValueException
#include <ascension/corelib/basic-types.hpp>		// uint32_t, ...
#include <ascension/graphics/geometry.hpp>
#include <functional>	// std.hash
#include <memory>		// std.unique_ptr, std.shared_ptr
#include <boost/operators.hpp>
#if defined(ASCENSION_SHAPING_ENGINE_CAIRO)
#	include <cairomm.h>
#elif defined(ASCENSION_SHAPING_ENGINE_CORE_GRAPHICS)
#	include <CGFont.h>
#elif defined(ASCENSION_SHAPING_ENGINE_CORE_TEXT)
#	include <CTFont.h>
#elif defined(ASCENSION_SHAPING_ENGINE_DIRECT_WRITE)
#	include <dwrite.h>
#elif defined(ASCENSION_SHAPING_ENGINE_HARFBUZZ)
#	include <hb.h>
#	include <boost/intrusive_ptr.hpp>
namespace boost {
	inline void intrusive_ptr_add_ref(hb_font_t* p) {::hb_font_reference(p);}
	inline void intrusive_ptr_release(hb_font_t* p) {::hb_font_destroy(p);}
}
#elif defined(ASCENSION_SHAPING_ENGINE_PANGO)
#	include <pangomm.h>
#elif defined(ASCENSION_SHAPING_ENGINE_QT)
#	include <QFont>
#elif defined(ASCENSION_SHAPING_ENGINE_UNISCRIBE) || defined(ASCENSION_SHAPING_ENGINE_WIN32_GDI)
#	include <ascension/win32/handle.hpp>	// win32.Handle
#elif defined(ASCENSION_SHAPING_ENGINE_WIN32_GDIPLUS)
#	include <GdiPlus.h>
#endif

namespace ascension {
	namespace graphics {

		/**
		 * @a font namespace is...
		 * Font properties specifications are designed based on "CSS Fonts Module Level 3"
		 * (http://dev.w3.org/csswg/css3-fonts/).
		 */
		namespace font {
			/**
			 * @c FontFamily represents a family of related font faces. A font family is a group of
			 * font faces that share a common design, but differ in styles.
			 * @see CSS Fonts Module Level 3, 3.1 Font family: the font-family property
			 *      (http://www.w3.org/TR/css3-fonts/#font-family-prop)
			 * @see CSS Fonts Module Level 3, 4.2 Font family: the font-family descriptor
			 *      (http://www.w3.org/TR/css3-fonts/#font-family-desc)
			 * @see Extensible Stylesheet Language (XSL) Version 1.1, 7.9.2 "font-family"
			 *      (http://www.w3.org/TR/xsl/#font-family)
			 * @see Fonts - SVG 1.1 (Second Edition), 20.8.3 The ‘font-face’ element
			 *      (http://www.w3.org/TR/SVG11/fonts.html#FontFaceElementFontFamilyAttribute)
			 * @see FontFace, fontFaces
			 */
			class FontFamily : private boost::totally_ordered<FontFamily> {
			public:
				/**
				 * @see CSS Fonts Module Level 3, 3.1.1 Generic font families
				 *      (http://www.w3.org/TR/css3-fonts/#generic-font-families)
				 */
				enum GenericFamily {
					SERIF, SANS_SERIF, CURSIVE, FANTASY, MONOSPACE
				};
			public:
#if defined(ASCENSION_SHAPING_ENGINE_DIRECT_WRITE)
				explicit FontFamily(const String& name);
				explicit FontFamily(win32::com::SmartPointer<IDWriteFontFamily> nativeObject);
				win32::com::SmartPointer<IDWriteFontFamily> asNativeObject() const;
#elif defined(ASCENSION_SHAPING_ENGINE_PANGO)
				explicit FontFamily(const String& name);
				explicit FontFamily(Glib::RefPtr<Pango::FontFamily> nativeObject);
				Glib::RefPtr<Pango::FontFamily> asNativeObject();
				Glib::RefPtr<const Pango::FontFamily> asNativeObject() const;
#elif defined(ASCENSION_SHAPING_ENGINE_WIN32_GDIPLUS)
				explicit FontFamily(const String& name);
				explicit FontFamily(std::unique_ptr<Gdiplus::FontFamily>&& nativeObject);
				explicit FontFamily(std::shared_ptr<Gdiplus::FontFamily> nativeObject);
				explicit FontFamily(Gdiplus::FontFamily& nativeObject);	// weak ref.
				std::shared_ptr<Gdiplus::FontFamily> asNativeObject() /*noexcept*/;
				std::shared_ptr<const Gdiplus::FontFamily> asNativeObject() const /*noexcept*/;
#else
				explicit FontFamily(const String& name) : name_(name) {
					if(name.empty())
						throw std::length_error("name");
				}
#endif
				explicit FontFamily(GenericFamily genericFamily);
				/// Copy-assignment operator.
				FontFamily& operator=(const FontFamily& other);
				/**
				 * Returns the family name.
				 * @param lc The locale for which to get the font family name. If this value is
				 *           C or unsupported locale, this method returns an unlocalized name
				 * @return The family name
				 */
				String name(const std::locale& lc = std::locale::classic()) const /*noexcept*/;
			private:
#if defined(ASCENSION_SHAPING_ENGINE_DIRECT_WRITE)
				win32::com::SmartPointer<IDWriteFontFamily> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_PANGO)
				Glib::RefPtr<Pango::FontFamily> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_WIN32_GDIPLUS)
				std::shared_ptr<Gdiplus::FontFamily> nativeObject_;
#else
				const String name_;
#endif
			};

			/// Equality operator for @c FontFamily.
			inline bool operator==(const FontFamily& lhs, const FontFamily& rhs) /*noexcept*/ {
				return lhs.name() == rhs.name();
			}

			/// Less-than operator for @c FontFamily.
			inline bool operator<(const FontFamily& lhs, const FontFamily& rhs) /*noexcept*/ {
				return lhs.name() < rhs.name();
			}
		}
	}
}

namespace std {
	/// Specialization of @c std#hash class template for @c FontFamily.
	template<>
	class hash<ascension::graphics::font::FontFamily> : public std::hash<ascension::String> {
	public:
		typedef ascension::graphics::font::FontFamily argument_type;
		result_type operator()(const argument_type& key) const {
			return std::hash<ascension::String>::operator()(key.name());
		}
	};
}

#endif // !ASCENSION_FONT_FAMILY_HPP
