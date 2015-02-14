/**
 * @file color.hpp
 * @author exeal
 * @see ascension/graphics/color.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2012-07-16 reunioned with text-line-style.hpp
 * @date 2014-09-21 Separated from text-style.hpp
 */

#ifndef ASCENSION_STYLES_COLOR_HPP
#define ASCENSION_STYLES_COLOR_HPP

#include <ascension/graphics/color.hpp>	// graphics.Color
#include <ascension/presentation/style-property.hpp>
#include <boost/optional.hpp>

namespace ascension {
	namespace presentation {
		namespace styles {
			/// @defgroup css_color_3 CSS Color Module Level 3
			/// @see CSS Color Module Level 3 - W3C Recommendation, 07 June 2011 (http://www.w3.org/TR/css-color-3/)
			/// @{
			/**
			 * Base type of @c Color and other color-related properties. @c boost#none means 'currentColor' in CSS 3.
			 * @tparam TypeSpec The tag type which has the two members: A boolean constant named @c INHERITED specifies
			 *                  if this is "Inherited Property". And a static function named @c initialValue returns
			 *                  the initial value as a @c boost#optional&lt;graphics#Color&gt;. Note that
			 *                  @c TypedColor&lt;...&gt; should be unique in a @c boost#fusion#vector
			 */
#ifndef BOOST_NO_TEMPLATE_ALIASES
			template<typename TypeSpec> using TypedColor =
				StyleProperty<Complex<boost::optional<graphics::Color>>, InheritedOrNot, graphics::Color>;
#else
			template<typename TypeSpec>
			class TypedColor : public StyleProperty<
				Complex<
					boost::optional<graphics::Color>,
					&TypeSpec::initialValue
				>,
				Inherited<TypeSpec::INHERITED>,
				graphics::Color
			> {
			private:
				typedef StyleProperty<
					Complex<boost::optional<graphics::Color>, &TypeSpec::initialValue>, Inherited<TypeSpec::INHERITED>, graphics::Color
				> Base;
			public:
				TypedColor() : Base() {}
				TypedColor(const value_type& value) : Base(value) {}
				TypedColor(const InitialTag&) : Base(INITIAL) {}
				TypedColor(const InheritTag&) : Base(INHERIT) {}
				TypedColor(const UnsetTag&) : Base(UNSET) {}
			};
#endif
			/**
			 * The common base type of types for template parameter of @c TypedColor.
			 * @tparam InheritedOrNot Specifies if this is "Inherited Property" or not. See @c StyleProperty.
			 */
			template<typename InheritedOrNot>
			struct BasicColorSpec {
				static const bool INHERITED = InheritedOrNot::value;
				static boost::optional<graphics::Color> initialValue() BOOST_NOEXCEPT {
					return boost::make_optional(graphics::Color::OPAQUE_BLACK);	// the initial value "depends on user agent"
				}
			};

			/**
			 * [Copied from CSS3] This property describes the foreground color of an element's text content.
			 * @see CSS Color Module Level 3, 3.1. Foreground color: the ÅecolorÅf property
			 *      (http://www.w3.org/TR/css-color-3/#foreground)
			 * @see SVG 1.1 (Second Edition), 12.2 The ÅecolorÅf property
			 *      (http://www.w3.org/TR/SVG11/color.html#ColorProperty)
			 * @see XSL 1.1, 7.18.1 "color" (http://www.w3.org/TR/xsl/#color)
			 */
			typedef TypedColor<BasicColorSpec<Inherited<true>>> Color;

			/**
			 * Returns "Specified Value" of 'currentColor' in CSS3.
			 * @see CSS Color Module Level 3, 4.4. ÅecurrentColorÅf color keyword
			 *      (http://www.w3.org/TR/css3-color/#currentcolor)
			 */
			BOOST_CONSTEXPR inline SpecifiedValue<Color>::type currentColor() BOOST_NOEXCEPT {
				return boost::none;
			}
			/// @}
		}
	}
}

#endif // !ASCENSION_STYLES_COLOR_HPP
