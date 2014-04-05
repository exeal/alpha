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
#include <ascension/corelib/basic-types.hpp>		// std.uint32_t, ...
#include <ascension/graphics/geometry.hpp>
#include <functional>	// std.hash
#include <memory>		// std.unique_ptr, std.shared_ptr
#include <boost/operators.hpp>
#if ASCENSION_SELECTS_SHAPING_ENGINE(CAIRO)
#	include <cairomm.h>
#elif ASCENSION_SELECTS_SHAPING_ENGINE(CORE_GRAPHICS)
#	include <CGFont.h>
#elif ASCENSION_SELECTS_SHAPING_ENGINE(CORE_TEXT)
#	include <CTFont.h>
#elif ASCENSION_SELECTS_SHAPING_ENGINE(DIRECT_WRITE)
#	include <dwrite.h>
#elif ASCENSION_SELECTS_SHAPING_ENGINE(HARFBUZZ)
#	include <hb.h>
#	include <boost/intrusive_ptr.hpp>
namespace boost {
	inline void intrusive_ptr_add_ref(hb_font_t* p) {::hb_font_reference(p);}
	inline void intrusive_ptr_release(hb_font_t* p) {::hb_font_destroy(p);}
}
#elif ASCENSION_SELECTS_SHAPING_ENGINE(PANGO)
#	include <pangomm.h>
#elif ASCENSION_SELECTS_SHAPING_ENGINE(QT)
#	include <QFont>
#elif ASCENSION_SELECTS_SHAPING_ENGINE(UNISCRIBE) || ASCENSION_SELECTS_SHAPING_ENGINE(WIN32_GDI)
#	include <ascension/win32/handle.hpp>	// win32.Handle
#elif ASCENSION_SELECTS_SHAPING_ENGINE(WIN32_GDIPLUS)
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
					SERIF,		///< 'serif' font family.
					SANS_SERIF,	///< 'sans-serif' font family.
					CURSIVE,	///< 'cursive' font family.
					FANTASY,	///< 'fantasy' font family.
					MONOSPACE	///< 'monospace' font family.
				};
			public:
#if ASCENSION_SELECTS_SHAPING_ENGINE(DIRECT_WRITE)
				explicit FontFamily(const String& name);
				explicit FontFamily(win32::com::SmartPointer<IDWriteFontFamily> nativeObject);
				win32::com::SmartPointer<IDWriteFontFamily> asNativeObject() const;
#elif ASCENSION_SELECTS_SHAPING_ENGINE(PANGO)
				explicit FontFamily(const String& name);
				explicit FontFamily(Glib::RefPtr<Pango::FontFamily> nativeObject);
				Glib::RefPtr<Pango::FontFamily> asNativeObject();
				Glib::RefPtr<const Pango::FontFamily> asNativeObject() const;
#elif ASCENSION_SELECTS_SHAPING_ENGINE(WIN32_GDIPLUS)
				explicit FontFamily(const String& name);
				explicit FontFamily(std::unique_ptr<Gdiplus::FontFamily> nativeObject);
				explicit FontFamily(std::shared_ptr<Gdiplus::FontFamily> nativeObject);
				explicit FontFamily(Gdiplus::FontFamily& nativeObject);	// weak ref.
				std::shared_ptr<Gdiplus::FontFamily> asNativeObject() BOOST_NOEXCEPT;
				std::shared_ptr<const Gdiplus::FontFamily> asNativeObject() const BOOST_NOEXCEPT;
#else
				/**
				 * Constructor takes a family name.
				 * @param name The font family name
				 * @throw std#length_error @a name is empty
				 */
				explicit FontFamily(const String& name) : name_(name) {
					if(name.empty())
						throw std::length_error("name");
				}
#endif
				/**
				 * Constructor creates a font generic family.
				 * @param genericFamily The generic family
				 * @throw UnknownValueException @a genericFamily is invalid
				 */
				explicit FontFamily(GenericFamily genericFamily);
				/// Copy-assignment operator.
				FontFamily& operator=(const FontFamily& other);
				/**
				 * Returns the family name.
				 * @param lc The locale for which to get the font family name. If this value is
				 *           C or unsupported locale, this method returns an unlocalized name
				 * @return The family name
				 */
				String name(const std::locale& lc = std::locale::classic()) const BOOST_NOEXCEPT;
			private:
#if ASCENSION_SELECTS_SHAPING_ENGINE(DIRECT_WRITE)
				win32::com::SmartPointer<IDWriteFontFamily> nativeObject_;
#elif ASCENSION_SELECTS_SHAPING_ENGINE(PANGO)
				Glib::RefPtr<Pango::FontFamily> nativeObject_;
#elif ASCENSION_SELECTS_SHAPING_ENGINE(WIN32_GDIPLUS)
				std::shared_ptr<Gdiplus::FontFamily> nativeObject_;
#else
				const String name_;
#endif
			};

			/// Equality operator for @c FontFamily.
			inline bool operator==(const FontFamily& lhs, const FontFamily& rhs) BOOST_NOEXCEPT {
				return lhs.name() == rhs.name();
			}

			/// Less-than operator for @c FontFamily.
			inline bool operator<(const FontFamily& lhs, const FontFamily& rhs) BOOST_NOEXCEPT {
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
