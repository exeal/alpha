/**
 * @file font-properties.hpp
 * @author exeal
 * @date 2010-11-06 created
 * @date 2010-2012 was font.hpp
 * @date 2012-08-19 separated from font.hpp
 * @date 2015-02-07 Separated from font-description.hpp
 */

#ifndef ASCENSION_FONT_PROPERTIES_HPP
#define ASCENSION_FONT_PROPERTIES_HPP

#include <ascension/corelib/future/scoped-enum-emulation.hpp>
#include <boost/functional/hash.hpp>
#include <boost/operators.hpp>
#include <cmath>		// std.floor
#include <cstdint>
#include <functional>	// std.hash
#include <iosfwd>		// std.char_traits
#include <stdexcept>

namespace ascension {
	namespace graphics {
		namespace font {
			/// TrueType/OpenType font tag.
			typedef std::uint32_t OpenTypeFontTag;

			/**
			 * Makes an 32-bit integer represents the given TrueType/OpenType font tag.
			 * @tparam c1, c2, c3, c4 Characters consist of the tag name
			 * @see makeOpenTypeFontTag
			 */
			template<std::uint8_t c1, std::uint8_t c2 = ' ', std::uint8_t c3 = ' ', std::uint8_t c4 = ' '>
			struct MakeOpenTypeFontTag {
				static const OpenTypeFontTag value = (c1 << 24) | (c2 << 16) | (c3 << 8) | c4;
			};

			/**
			 * Returns an 32-bit integer represents the given TrueType/OpenType font tag.
			 * @tparam Character The character type of @a name
			 * @param name The TrueType tag name
			 * @param validate Set @c true to validate characters in @a name
			 * @return The 32-bit integral TrueType tag value
			 * @throw std#length_error The length of @a name is zero or greater four
			 * @throw std#invalid_argument @a validate is @c true and any character in @a name was invalid
			 * @see MakeOpenTypeFontTag
			 */
			template<typename Character>
			inline OpenTypeFontTag makeOpenTypeFontTag(const Character name[], bool validate = true) {
				const std::size_t len = std::char_traits<Character>::length(name);
				if(len == 0 || len > 4)
					throw std::length_error("name");
				OpenTypeFontTag tag = 0;
				std::size_t i = 0;
				for(; i < len; ++i) {
					if(validate && (name[i] < 32 || name[i] > 126))
						throw std::invalid_argument("name");
					tag |= name[i] << ((3 - i) * 8);
				}
				for(; i < 4; ++i)
					tag |= ' ' << ((3 - i) * 8);
				return tag;
			}

			template<typename T> inline int round(T value) {	// why is this here?
				return static_cast<int>(std::floor(value + 0.5));
			}

			/**
			 * [Copied from CSS3] The Åefont-weightÅf property specifies the weight of glyphs in the font, their degree
			 * of blackness or stroke thickness.
			 * @see CSS Fonts Module Level 3, 3.2 Font weight: the font-weight property
			 *      (http://www.w3.org/TR/css3-fonts/#font-weight-prop)
			 * @see Extensible Stylesheet Language (XSL) Version 1.1, 7.9.9 "font-weight"
			 *      (http://www.w3.org/TR/xsl/#font-weight)
			 * @see Fonts - SVG 1.1 (Second Edition), 20.8.3 The Åefont-faceÅf element
			 *      (http://www.w3.org/TR/SVG11/fonts.html#FontFaceElementFontWeightAttribute)
			 */
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(FontWeight)
				/// Same as 400.
				NORMAL = 400,
				/// Same as 700.
				BOLD = 700,
				/// Specifies the weight of the face bolder than the inherited value.
				BOLDER,
				/// Specifies the weight of the face lighter than the inherited value.
				LIGHTER,
				THIN = 100,
				EXTRA_LIGHT = 200,
				ULTRA_LIGHT = 200,
				LIGHT = 300, 
				MEDIUM = 500,
				SEMI_BOLD = 600,
				DEMI_BOLD = 600,
				EXTRA_BOLD = 800,
				ULTRA_BOLD = 800,
				BLACK = 900,
				HEAVY = 900
			ASCENSION_SCOPED_ENUM_DECLARE_END(FontWeight)

			/**
			 * [Copied from CSS3] The Åefont-stretchÅf property selects a normal, condensed, or expanded face from a font
			 * family.
			 * @see CSS Fonts Module Level 3, 3.3 Font width: the font-stretch property
			 *      (http://www.w3.org/TR/css3-fonts/#font-stretch-prop)
			 * @see Extensible Stylesheet Language (XSL) Version 1.1, 7.9.5 "font-stretch"
			 *      (http://www.w3.org/TR/xsl/#font-stretch)
			 * @see Fonts - SVG 1.1 (Second Edition), 20.8.3 The Åefont-faceÅf element
			 *      (http://www.w3.org/TR/SVG11/fonts.html#FontFaceElementFontStretchAttribute)
			 */
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(FontStretch)
				NORMAL = 1000,			///< Normal.
//				WIDER,
//				NARROWER,
				ULTRA_CONDENSED = 500,	///< Ultra Condensed.
				EXTRA_CONDENSED = 625,	///< Extra Condensed.
				CONDENSED = 750,		///< Condensed.
				SEMI_CONDENSED = 875,	///< Semi Condensed.
				SEMI_EXPANDED = 1125,	///< Semi Expanded.
				EXPANDED = 1250,		///< Expanded.
				EXTRA_EXPANDED = 1500,	///< Extra Expanded.
				ULTRA_EXPANDED = 2000	///< Ultra Expanded.
			ASCENSION_SCOPED_ENUM_DECLARE_END(FontStretch)

			/**
			 * [Copied from CSS3] The Åefont-styleÅf property allows italic or oblique faces to be selected. Italic forms
			 * are generally cursive in nature while oblique faces are typically sloped versions of the regular face.
			 * @see CSS Fonts Module Level 3, 3.4 Font style: the font-style property
			 *      (http://www.w3.org/TR/css3-fonts/#font-style-prop)
			 * @see Extensible Stylesheet Language (XSL) Version 1.1, 7.9.7 "font-style"
			 *      (http://www.w3.org/TR/xsl/#font-style)
			 * @see Fonts - SVG 1.1 (Second Edition), 20.8.3 The Åefont-faceÅf element
			 *      (http://www.w3.org/TR/SVG11/fonts.html#FontFaceElementFontStyleAttribute)
			 */
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(FontStyle)
				/// selects a face that is classified as a normal face, one that is neither italic or obliqued
				NORMAL,
				/// selects a font that is labeled as an italic face, or an oblique face if one is not
				ITALIC,
				/// selects a font that is labeled as an oblique face, or an italic face if one is not
				OBLIQUE,
				/// Selects a face that is labeled 'backslant'. This is not part of CSS level 3.
				BACKSLANT
			ASCENSION_SCOPED_ENUM_DECLARE_END(FontStyle)

//			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(FontSynthesis)
//				NONE = 0,
//				WEIGHT = 1 << 0,
//				STYLE = 1 << 1
//			ASCENSION_SCOPED_ENUM_DECLARE_END(FontSynthesis)

//			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(FontVariant)
//				NORMAL,
//				SMALL_CAPS
//			ASCENSION_SCOPED_ENUM_DECLARE_END(FontVariant)
#if 0
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(FontOrientation)
				HORIZONTAL,
				VERTICAL
			ASCENSION_SCOPED_ENUM_DECLARE_END(FontOrientation)
#endif
			struct FontFeatureSetting {
				OpenTypeFontTag name;
				std::uint32_t value;
				/// Default constructor does not initialize the data members.
				FontFeatureSetting() BOOST_NOEXCEPT {}
				FontFeatureSetting(OpenTypeFontTag name, std::uint32_t value) BOOST_NOEXCEPT : name(name), value(value) {}
			};

			/**
			 * Set of font properties without the family name.
			 * @see FontDescription
			 */
			struct FontProperties : private boost::equality_comparable<FontProperties> {
				FontWeight weight;				///< The font weight.
				FontStretch stretch;			///< The font stretch.
				FontStyle style;				///< The font style.
//				FontVariant variant;			///< The font variant.
//				FontSynthesis synthesis;		///< The font synthesis.
#if 0
				FontOrientation orientation;	///< The font orientation.
#endif
				// TODO: 'font-feature-settings' property should be implemented as TextRunStyle?
//				std::vector<FontFeatureSetting> featureSettings;

				/**
				 * Constructor.
				 * @param weight The initial weight value
				 * @param stretch The initial stretch value
				 * @param style The initial style value
				 * @param orientation The initial orientation value
				 */
				explicit FontProperties(FontWeight weight = FontWeight::NORMAL,
					FontStretch stretch = FontStretch::NORMAL, FontStyle style = FontStyle::NORMAL)
					: weight(weight), stretch(stretch), style(style) {}
				/// Equality operator.
				bool operator==(const FontProperties& other) const BOOST_NOEXCEPT {
					return weight == other.weight && stretch == other.stretch && style == other.style;
				}
			};

			/// Specialization of @c boost#hash_value function template for @c FontProperties.
			inline std::size_t hash_value(const FontProperties& object) BOOST_NOEXCEPT {
				std::size_t seed = 0;
				boost::hash_combine(seed, boost::native_value(object.weight));
				boost::hash_combine(seed, boost::native_value(object.stretch));
				boost::hash_combine(seed, boost::native_value(object.style));
//				boost::hash_combine(seed, boost::native_value(object.variant));
//				boost::hash_combine(seed, boost::native_value(object.synthesis));
//				boost::hash_combine(seed, boost::native_value(object.orientation));
				return seed;
			}
		}
	}
}

namespace std {
	/// Specialization of @c std#hash class template for @c FontProperties.
	template<>
	class hash<ascension::graphics::font::FontProperties> :
		public std::function<std::hash<void*>::result_type(const ascension::graphics::font::FontProperties&)> {
	public:
		result_type operator()(const argument_type& key) const BOOST_NOEXCEPT {
			return boost::hash<argument_type>()(key);
		}
	};
}

#endif // !ASCENSION_FONT_PROPERTIES_HPP
