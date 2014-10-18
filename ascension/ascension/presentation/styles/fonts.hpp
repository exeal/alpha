/**
 * @file fonts.hpp
 * @author exeal
 * @see presentation.hpp, graphics/text-alignment.hpp, graphics/text-layout-styles.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2012-07-16 reunioned with text-line-style.hpp
 * @date 2014-09-23 Separated from text-style.hpp
 */

#ifndef ASCENSION_STYLES_FONTS_HPP
#define ASCENSION_STYLES_FONTS_HPP

#include <ascension/corelib/future/scoped-enum-emulation.hpp>
#include <ascension/graphics/font/font-description.hpp>
#include <ascension/graphics/font/font-family.hpp>
#include <ascension/presentation/absolute-length.hpp>
#include <ascension/presentation/style-property.hpp>
#include <ascension/presentation/styles/length.hpp>
#include <ascension/presentation/writing-mode.hpp>
//#include <map>
#include <vector>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

namespace ascension {
	namespace presentation {
		namespace styles {
			/// @defgroup css3_fonts CSS Fonts Module Level 3
			/// @see CSS Fonts Module Level 3 - W3C Candidate Recommendation 3 October 2013
			///      (http://www.w3.org/TR/css3-fonts/)
			/// @{
			/**
			 * @see CSS Fonts Module Level 3, 3.1 Font family: the font-family property
			 *      (http://www.w3.org/TR/css3-fonts/#font-family-prop)
			 * @see SVG 1.1 (Second Edition), 10.10 Font selection properties
			 *      (http://www.w3.org/TR/SVG11/text.html#FontFamilyProperty)
			 * @see XSL 1.1, 7.9.2 "font-family" (http://www.w3.org/TR/xsl/#font-family)
			 */
			typedef StyleProperty<
				Complex<
					std::vector<graphics::font::FontFamily>
				>, Inherited<true>
			> FontFamily;

			/// @see graphics#font#FontWeight
			typedef StyleProperty<
				Enumerated<BOOST_SCOPED_ENUM_NATIVE(graphics::font::FontWeight), graphics::font::FontWeight::NORMAL>,
				Inherited<true>
			> FontWeight;

			/// @see @c graphics#font#FontStretch
			typedef StyleProperty<
				Enumerated<BOOST_SCOPED_ENUM_NATIVE(graphics::font::FontStretch), graphics::font::FontStretch::NORMAL>,
				Inherited<true>
			> FontStretch;

			/// @see @c graphics#font#FontStyle
			typedef StyleProperty<
				Enumerated<BOOST_SCOPED_ENUM_NATIVE(graphics::font::FontStyle), graphics::font::FontStyle::NORMAL>,
				Inherited<true>
			> FontStyle;

			/**
			 * [Copied from CSS3] An &lt;absolute-size&gt; keyword refers to an entry in a table of font sizes computed
			 * and kept by the user agent.
			 * @see http://www.w3.org/TR/css3-fonts/#absolute-size-value
			 */
			ASCENSION_SCOPED_ENUMS_BEGIN(AbsoluteFontSize)
				XX_SMALL, X_SMALL, SMALL, MEDIUM, LARGE, X_LARGE, XX_LARGE
			ASCENSION_SCOPED_ENUMS_END

			/**
			 * [Copied from CSS3] A &lt;relative-size&gt; keyword is interpreted relative to the table of font sizes
			 * and the computed ‘font-size’ of the parent element.
			 * @see http://www.w3.org/TR/css3-fonts/#relative-size-value
			 */
			ASCENSION_SCOPED_ENUMS_BEGIN(RelativeFontSize)
				LARGER, SMALLER
			ASCENSION_SCOPED_ENUMS_END

			/**
			 * [Copied from CSS3] This property indicates the desired height of glyphs from the font. For scalable
			 * fonts, the font-size is a scale factor applied to the EM unit of the font.
			 * @see CSS Fonts Module Level 3, 3.5 Font size: the font-size property
			 *      (http://www.w3.org/TR/css3-fonts/#font-size-prop)
			 * @see SVG 1.1 (Second Edition), 10.10 Font selection properties
			 *      (http://www.w3.org/TR/SVG11/text.html#FontSizeProperty)
			 * @see XSL 1.1, 7.9.4 "font-size" (http://www.w3.org/TR/xsl/#font-size)
			 */
			typedef StyleProperty<
				Multiple<
					boost::variant<
						BOOST_SCOPED_ENUM_NATIVE(AbsoluteFontSize),
						BOOST_SCOPED_ENUM_NATIVE(RelativeFontSize),
						Length,
						Percentage
					>,
					BOOST_SCOPED_ENUM_NATIVE(AbsoluteFontSize), AbsoluteFontSize::MEDIUM
				>,
				Inherited<true>,
				Pixels
			> FontSize;

			/**
			 * [Copied from CSS3] For any given font size, the apparent size and legibility of text varies across
			 * fonts. For scripts such as Latin or Cyrillic that distinguish between upper and lowercase letters, the
			 * relative height of lowercase letters compared to their uppercase counterparts is a determining factor of
			 * legibility. This is commonly referred to as the aspect value. Precisely defined, it is equal to the
			 * x-height of a font divided by the font size.
			 *
			 * In situations where font fallback occurs, fallback fonts may not share the same aspect value as the
			 * desired font family and will thus appear less readable. The ‘font-size-adjust’ property is a way to
			 * preserve the readability of text when font fallback occurs. It does this by adjusting the font-size so
			 * that the x-height is the same regardless of the font used.
			 * @see CSS Fonts Module Level 3, 3.6 Relative sizing: the font-size-adjust property
			 *      (http://www.w3.org/TR/css3-fonts/#font-size-adjust-prop)
			 * @see SVG 1.1 (Second Edition), 10.10 Font selection properties
			 *      (http://www.w3.org/TR/SVG11/text.html#FontSizeAdjustProperty)
			 * @see XSL 1.1, 7.9.6 "font-size-adjust" (http://www.w3.org/TR/xsl/#font-size-adjust)
			 */
			typedef StyleProperty<
				Complex<
					boost::optional<Number>
				>, Inherited<true>
			> FontSizeAdjust;

//			typedef StyleProperty<
//				Complex<
//					std::map<graphics::font::TrueTypeFontTag, std::uint32_t>
//				>, Inherited
//			> FontFeatureSettings;

//			typedef StyleProperty<
//				Complex<
//					boost::optional<String>
//				>, Inherited
//			> FontLanguageOverride;
			/// @}
		}
	}
} // namespace ascension.presentation.styles

#endif // !ASCENSION_STYLES_FONTS_HPP
