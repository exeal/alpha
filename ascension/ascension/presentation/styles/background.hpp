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

#include <ascension/graphics/paint.hpp>	// graphics.Paint
#include <ascension/presentation/absolute-length.hpp>
#include <ascension/presentation/styles/color.hpp>
#include <ascension/presentation/style-property.hpp>
#include <memory>						// std.unique_ptr
#include <boost/mpl/identity.hpp>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

namespace ascension {
	namespace presentation {
		namespace styles {
			/**
			 * @defgroup css_background_3 CSS Backgrounds and Borders Module Level 3
			 * @see CSS Backgrounds and Borders Module Level 3 - W3C Candidate Recommendation, 9 September 2014
			 *      (http://www.w3.org/TR/css-background-3/)
			 * @{
			 */
			/// A tag type for @c BackgroundColor.
			struct BackgroundColorSpec : BasicColorSpec<Inherited<false>> {
				static boost::optional<graphics::Color> initialValue() BOOST_NOEXCEPT {
					return boost::make_optional(graphics::Color::TRANSPARENT_BLACK);
				}
			};

			/**
			 * [Copied from CSS3] This property sets the background color of an element. The color is drawn behind any
			 * background images.
			 * @see CSS Backgrounds and Borders Module Level 3, 3.2. Base Color: the ‘background-color’ property
			 *      (http://www.w3.org/TR/css-background-3/#the-background-color)
			 * @see XSL 1.1, 7.8.2 "background-color" (http://www.w3.org/TR/xsl/#background-color)
			 */
			typedef TypedColor<BackgroundColorSpec> BackgroundColor;

			/**
			 * @see CSS Backgrounds and Borders Module Level 3, 3.1. Layering Multiple Background Images
			 *      (http://www.w3.org/TR/css-background-3/#layering)
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
			/// A tag type for @c BorderColor.
			struct BorderColorSpec : BasicColorSpec<Inherited<false>> {};

			/**
			 * [Copied from CSS3] This property sets the foreground color of the border specified by the border-style
			 * properties. @c boost#none means 'currentColor'.
			 * @see CSS Backgrounds and Borders Module Level 3, 4.1. Line Colors: the ‘border-color’ property
			 *      (http://www.w3.org/TR/css-background-3/#the-border-color)
			 */
			typedef TypedColor<BorderColorSpec> BorderColor;

			/// Enumerated values for @c BorderStyle.
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(BorderStyleEnums)
				NONE, HIDDEN, DOTTED, DASHED, SOLID,
				DOT_DASH, DOT_DOT_DASH,
				DOUBLE, GROOVE, RIDGE, INSET, OUTSET
			ASCENSION_SCOPED_ENUM_DECLARE_END(BorderStyleEnums)

			/**
			 * [Copied from CSS3] This property sets the style of the border, unless there is a border image.
			 * @see CSS Backgrounds and Borders Module Level 3, 4.2. Line Patterns: the ‘border-style’ properties
			 *      (http://www.w3.org/TR/css-background-3/#the-border-style)
			 */
			typedef StyleProperty<
				Enumerated<BOOST_SCOPED_ENUM_NATIVE(BorderStyleEnums), BorderStyleEnums::NONE>,
				Inherited<false>
			> BorderStyle;

			/// Defines &lt;line-width&gt; keywords.
			namespace linewidthkeywords {
				/// The ‘thin’ value.
				BOOST_CONSTEXPR_OR_CONST Length THIN(0.05f, Length::EM_HEIGHT);
				/// The ‘medium’ value.
				BOOST_CONSTEXPR_OR_CONST Length MEDIUM(0.10f, Length::EM_HEIGHT);
				/// The ‘thick’ value.
				BOOST_CONSTEXPR_OR_CONST Length THICK(0.20f, Length::EM_HEIGHT);
			}

			namespace detail {
				BOOST_CONSTEXPR inline Length makeMediumBorderWidth() {
					return linewidthkeywords::MEDIUM;
				}
			}

			/**
			 * [Copied from CSS3] This property sets the thickness of the border.
			 * @see CSS Backgrounds and Borders Module Level 3, 4.3. Line Thickness: the ‘border-width’ properties
			 *      (http://www.w3.org/TR/css-background-3/#the-border-width)
			 */
			typedef StyleProperty<
				Complex<Length, &detail::makeMediumBorderWidth>,
				Inherited<false>
			> BorderWidth;

			/**
			 * Returns @c true if the given border style has visible style (but may or may not consume place).
			 * @param style The border style to check
			 */
			BOOST_CONSTEXPR inline bool hasVisibleStyle(BorderStyleEnums style) BOOST_NOEXCEPT {
				return style != BorderStyleEnums::NONE && style != BorderStyleEnums::HIDDEN;
			}

			/**
			 * Returns @c true if the computed thickness of this side is zero.
			 * @tparam BorderWidthType The type of @a width
			 * @param width The border width to check
			 * @see Strictly speaking, the used width is needed to check if absent
			 */
			template<typename BorderWidthType>
			BOOST_CONSTEXPR inline bool isAbsent(const BorderWidthType& width) BOOST_NOEXCEPT {
				return width == BorderWidthType(0);
			}
			/// @}
		}
	}
}

#endif // !ASCENSION_STYLES_BACKGROUND_HPP
