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
			/// @defgroup css3_color CSS Color Module Level 3
			/// @see CSS Color Module Level 3 - W3C Recommendation, 07 June 2011 (http://www.w3.org/TR/css3-color/)
			/// @{
			/**
			 * Describes the foreground color of the text content. @c boost#none means 'currentColor' in CSS 3.
			 * @tparam InheritedOrNot See same name parameter of @c StyleProperty class template
			 * @see CSS Color Module Level 3, 3.1. Foreground color: the ÅecolorÅf property
			 *      (http://www.w3.org/TR/css3-color/#foreground)
			 * @see SVG 1.1 (Second Edition), 12.2 The ÅecolorÅf property
			 *      (http://www.w3.org/TR/SVG11/color.html#ColorProperty)
			 * @see XSL 1.1, 7.18.1 "color" (http://www.w3.org/TR/xsl/#color)
			 */
#ifndef BOOST_NO_TEMPLATE_ALIASES
			template<typename InheritedOrNot> using Color =
				StyleProperty<Complex<boost::optional<graphics::Color>>, InheritedOrNot>;
#else
			template<typename InheritedOrNot>
			class Color : public StyleProperty<
				Complex<
					boost::optional<graphics::Color>
				>, InheritedOrNot
			> {
			private:
				typedef StyleProperty<Complex<boost::optional<graphics::Color>>, InheritedOrNot> Base;
			public:
				Color() : Base() {}
				Color(const value_type& value) : Base(value) {}
			};
#endif

			/**
			 * Computes the specified color properties with inheritance and defaulting.
			 * @tparam InheritedOrNotForCurrentColor The template parameter for @a current
			 * @tparam InheritedOrNotForParentColor The template parameter for @a parent
			 * @tparam InheritedOrNotForAncestorColor The template parameter for @a ancestor
			 * @param current The declared color property of the current element
			 * @param parent The declared color property of the parent element
			 * @param ancestor The declared color property of the ancestor element
			 * @return A computed color value
			 */
			template<typename InheritedOrNotForCurrent,
				typename InheritedOrNotForParent, typename InheritedOrNotForAncestor>
			inline graphics::Color computeColor(
					const Color<InheritedOrNotForCurrent>* current,
					const Color<InheritedOrNotForParent>* parent,
					const Color<InheritedOrNotForAncestor>& ancestor) {
				if(current != nullptr && !current->inherits() && current->get() != boost::none)
					return *current->get();
				else if(parent != nullptr && !parent->inherits() && parent->get() != boost::none)
					return *parent->get();
				else if(!ancestor.inherits() && ancestor.get() != boost::none)
					return *ancestor.get();
				else
					return boost::get_optional_value_or(graphics::SystemColors::get(
						graphics::SystemColors::WINDOW_TEXT), graphics::Color::OPAQUE_BLACK);
			}
			/// @}
		}
	}
}

#endif // !ASCENSION_STYLES_COLOR_HPP
