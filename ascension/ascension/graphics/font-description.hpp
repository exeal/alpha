/**
 * @file font-description.hpp
 * @author exeal
 * @date 2010-11-06 created
 * @date 2010-2012 was font.hpp
 * @date 2012-08-19 separated from font.hpp
 */

#ifndef ASCENSION_FONT_DESCRIPTION_HPP
#define ASCENSION_FONT_DESCRIPTION_HPP

#include <ascension/graphics/font-family.hpp>

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
			 * @throw std#invalid_argument @a validate is @c true and any character in @a name was
			 *                             invalid
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
			 * [Copied from CSS3] The Åefont-weightÅf property specifies weight of glyphs in the
			 * font, their degree of blackness or stroke thickness.
			 * @see CSS Fonts Module Level 3, 3.2 Font weight: the font-weight property
			 *      (http://www.w3.org/TR/css3-fonts/#generic-font-families)
			 * @see Extensible Stylesheet Language (XSL) Version 1.1, 7.9.9 "font-weight"
			 *      (http://www.w3.org/TR/xsl/#font-weight)
			 * @see Fonts - SVG 1.1 (Second Edition), 20.8.3 The Åefont-faceÅf element
			 *      (http://www.w3.org/TR/SVG11/fonts.html#FontFaceElementFontWeightAttribute)
			 */
			ASCENSION_BEGIN_SCOPED_ENUM(FontWeight)
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
			ASCENSION_END_SCOPED_ENUM

			/**
			 * [Copied from CSS3] This property selects a normal, condensed, or expanded face from
			 * a font family.
			 * @see CSS Fonts Module Level 3, 3.3 Font width: the font-stretch property
			 *      (http://www.w3.org/TR/css3-fonts/#font-stretch-prop)
			 * @see Extensible Stylesheet Language (XSL) Version 1.1, 7.9.5 "font-stretch"
			 *      (http://www.w3.org/TR/xsl/#font-stretch)
			 * @see Fonts - SVG 1.1 (Second Edition), 20.8.3 The Åefont-faceÅf element
			 *      (http://www.w3.org/TR/SVG11/fonts.html#FontFaceElementFontStretchAttribute)
			 */
			ASCENSION_BEGIN_SCOPED_ENUM(FontStretch)
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
			ASCENSION_END_SCOPED_ENUM

			/**
			 * [Copied from CSS3] This property allows italic or oblique faces to be selected.
			 * @see CSS Fonts Module Level 3, 3.4 Font style: the font-style property
			 *      (http://www.w3.org/TR/css3-fonts/#font-style-prop)
			 * @see Extensible Stylesheet Language (XSL) Version 1.1, 7.9.7 "font-style"
			 *      (http://www.w3.org/TR/xsl/#font-style)
			 * @see Fonts - SVG 1.1 (Second Edition), 20.8.3 The Åefont-faceÅf element
			 *      (http://www.w3.org/TR/SVG11/fonts.html#FontFaceElementFontStyleAttribute)
			 */
			ASCENSION_BEGIN_SCOPED_ENUM(FontStyle)
				NORMAL,		///< Selects a face that is classified as 'normal'.
				ITALIC,		///< Selects a face that is labeled 'italic' or 'oblique'.
				OBLIQUE,	///< Selects a face that is labeled 'oblique'.
				BACKSLANT	///< Selects a face that is labeled 'backslant'.
			ASCENSION_END_SCOPED_ENUM

//			ASCENSION_BEGIN_SCOPED_ENUM(FontSynthesis)
//				NONE = 0,
//				WEIGHT = 1 << 0,
//				STYLE = 1 << 1
//			ASCENSION_END_SCOPED_ENUM

//			ASCENSION_BEGIN_SCOPED_ENUM(FontVariant)
//				NORMAL,
//				SMALL_CAPS
//			ASCENSION_END_SCOPED_ENUM

			ASCENSION_BEGIN_SCOPED_ENUM(FontOrientation)
				HORIZONTAL,
				VERTICAL
			ASCENSION_END_SCOPED_ENUM

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
				FontOrientation orientation;	///< The font orientation.
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
					FontStretch stretch = FontStretch::NORMAL, FontStyle style = FontStyle::NORMAL,
					FontOrientation orientation = FontOrientation::HORIZONTAL)
					: weight(weight), stretch(stretch), style(style), orientation(orientation) {}
				/// Equality operator.
				bool operator==(const FontProperties& other) const BOOST_NOEXCEPT {
					return weight == other.weight && stretch == other.stretch
						&& style == other.style && orientation == other.orientation;
				}
			};

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
				explicit FontDescription(const FontFamily& family,
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
				FontDescription& setFamilyName(const FontFamily& family) {
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
		}
	}
}

namespace std {
	/// Specialization of @c std#hash class template for @c FontProperties.
	template<>
	class hash<ascension::graphics::font::FontProperties> :
		public std::function<std::hash<void*>::result_type(const ascension::graphics::font::FontProperties&)> {
	public:
		result_type operator()(const argument_type& key) const {
			// TODO: use boost.hash_combine.
			return std::hash<ascension::graphics::font::FontWeight>()(key.weight)
				+ std::hash<ascension::graphics::font::FontStretch>()(key.stretch)
				+ std::hash<ascension::graphics::font::FontStyle>()(key.style)
//				+ std::hash<ascension::graphics::font::FontVariant>()(key.variant)
				+ std::hash<ascension::graphics::font::FontOrientation>()(key.orientation);
		}
	};

	/// Specialization of @c std#hash class template for @c FontDescription.
	template<>
	class hash<ascension::graphics::font::FontDescription> :
		public std::function<std::hash<void*>::result_type(const ascension::graphics::font::FontDescription&)> {
	public:
		result_type operator()(const argument_type& key) const {
			// TODO: use boost.hash_combine.
			return std::hash<ascension::graphics::font::FontFamily>()(key.family())
				+ std::hash<double>()(key.pointSize())
				+ std::hash<ascension::graphics::font::FontProperties>()(key.properties());
		}
	};
}

#endif // !ASCENSION_FONT_DESCRIPTION_HPP
