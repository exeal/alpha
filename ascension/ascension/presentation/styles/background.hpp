/**
 * @file background.hpp
 * @author exeal
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2012-07-16 reunioned with text-line-style.hpp
 * @date 2014-09-21 Separated from text-style.hpp
 */

#ifndef ASCENSION_STYLES_BACKGROUND_HPP
#define ASCENSION_STYLES_BACKGROUND_HPP

#include <ascension/presentation/styles/color.hpp>
#include <ascension/graphics/paint.hpp>	// graphics.Paint
#include <ascension/presentation/style-property.hpp>
#include <memory>						// std.unique_ptr
#include <boost/mpl/identity.hpp>
#include <boost/optional.hpp>

namespace ascension {
	namespace presentation {
		namespace styles {
			/// @defgroup css3_background CSS Backgrounds and Borders Module Level 3
			/// @see CSS Backgrounds and Borders Module Level 3 - W3C Candidate Recommendation, 9 September 2014
			///      (http://www.w3.org/TR/css3-background/)
			/// @{
			/**
			 * @c null also means 'transparent'.
			 * @see CSS Backgrounds and Borders Module Level 3, 3.10. Backgrounds Shorthand: the
			 *      ‘background’ property (http://www.w3.org/TR/css3-background/#the-background)
			 * @see SVG 1.1 (Second Edition), 11.3 Fill Properties
			 *      (http://www.w3.org/TR/SVG11/painting.html#FillProperties)
			 * @see XSL 1.1, 7.31.1 "background" (http://www.w3.org/TR/xsl/#background)
			 */
			struct Background {
				/**
				 * [Copied from CSS3] This property sets the background color of an element. The color
				 * is drawn behind any background images.
				 * @see CSS Backgrounds and Borders Module Level 3, 3.2. Base Color: the
				 *      ‘background-color’ property
				 *      (http://www.w3.org/TR/css3-background/#the-background-color)
				 * @see XSL 1.1, 7.8.2 "background-color" (http://www.w3.org/TR/xsl/#background-color)
				 */
				Color<Inherited<false>> color;
				/**
				 * @see CSS Backgrounds and Borders Module Level 3, 3.1. Layering Multiple Background
				 *      Images (http://www.w3.org/TR/css3-background/#layering)
				 */
				struct Layer {
					struct Image {} image;
					struct RepeatStyle {} repeat;
					enum Attachment {} attachment;
					struct Position {} position;
					enum Clip {} clip;
					enum Origin {} origin;
					struct Size {} size;
				};

				Background() : color(boost::make_optional(graphics::Color::TRANSPARENT_BLACK)) {}
			};
#if 0
			/**
			 * Computes the specified background properties with inheritance and defaulting.
			 * @param current The declared background property of the current element
			 * @param parent The declared background property of the parent element
			 * @param ancestor The declared background property of the ancestor element
			 * @return A computed background value as @c Paint
			 */
			inline std::unique_ptr<graphics::Paint> computeBackground(
					const Background* current, const Background* parent, const Background& ancestor) {
				// TODO: This code is not complete.
				if(current != nullptr && !current->color.inherits()
						&& current->color.get() != boost::none && current->color.get()->isFullyTransparent())
					return std::unique_ptr<graphics::Paint>(new graphics::SolidColor(*current->color.get()));
				else if(parent != nullptr && !parent->color.inherits()
						&& parent->color.get() != boost::none && parent->color.get()->isFullyTransparent())
					return std::unique_ptr<graphics::Paint>(new graphics::SolidColor(*parent->color.get()));
				else if(!ancestor.color.inherits()
						&& ancestor.color.get() != boost::none && ancestor.color.get()->isFullyTransparent())
					return std::unique_ptr<graphics::Paint>(new graphics::SolidColor(*ancestor.color.get()));
				return std::unique_ptr<graphics::Paint>(
					new graphics::SolidColor(boost::get_optional_value_or(
						graphics::SystemColors::get(graphics::SystemColors::WINDOW), graphics::Color::OPAQUE_WHITE)));
			}
#endif
			/**
			 * @see CSS Backgrounds and Borders Module Level 3, 4. Borders
			 *      (http://www.w3.org/TR/css3-background/#borders)
			 */
			struct Border {
				/**
				 * @see 
				 */
				enum Style {
					NONE, HIDDEN, DOTTED, DASHED, SOLID,
					DOT_DASH, DOT_DOT_DASH,
					DOUBLE, GROOVE, RIDGE, INSET, OUTSET
				};
				static const Length THIN, MEDIUM, THICK;
				struct Side {
					/**
					 * [Copied from CSS3] This property sets the foreground color of the border
					 * specified by the border-style properties. @c boost#none means 'currentColor'.
					 * @see CSS Backgrounds and Borders Module Level 3, 4.1. Line Colors: the
					 *      ‘border-color’ property
					 *      (http://www.w3.org/TR/css3-background/#the-border-color)
					 */
					Color<Inherited<false>> color;
					/**
					 * [Copied from CSS3] This property sets the style of the border, unless there is a
					 * border image.
					 * @see CSS Backgrounds and Borders Module Level 3, 4.2. Line Patterns: the
					 *      ‘border-style’ properties
					 *      (http://www.w3.org/TR/css3-background/#the-border-style)
					 */
					StyleProperty<
						Enumerated<Style, NONE>,
						Inherited<false>
					> style;

					struct WidthTypeSpec : public boost::mpl::identity<Length> {
						static const Length& initialValue() {return MEDIUM;}
					};
					/**
					 * [Copied from CSS3] This property sets the thickness of the border.
					 * @see CSS Backgrounds and Borders Module Level 3, 4.3. Line Thickness: the
					 *      ‘border-width’ properties
					 *      (http://www.w3.org/TR/css3-background/#the-border-width)
					 */
					StyleProperty<
						WidthTypeSpec,
						Inherited<false>,
						graphics::Scalar	// in pixels
					> width;
				};
				FlowRelativeFourSides<Side> sides;
			};
			/// @}
		}
	}
}

#endif // !ASCENSION_STYLES_BACKGROUND_HPP
