/**
 * @file computed-text-styles.hpp
 * @see text-alignment.hpp, presentation/text-style.hpp
 * @date 2003-2006 (was LineLayout.h)
 * @date 2006-2010
 * @date 2010-11-20 renamed from ascension/layout.hpp
 * @date 2011-2012 was text-layout.hpp
 * @date 2012-08-17 separated from text-layout.hpp
 * @date 2013-04-25 separated from text-layout-styles.hpp
 */

#ifndef COMPUTED_TEXT_STYLES_HPP
#define COMPUTED_TEXT_STYLES_HPP

#include <ascension/graphics/color.hpp>
#include <ascension/presentation/text-style.hpp>

namespace ascension {
	namespace graphics {

		class PaintContext;

		namespace font {
			/// Computed value of @c presentation#Border#Side.
			struct ComputedBorderSide {
				presentation::sp::IntrinsicType<
					decltype(presentation::Border().sides.start().color)
				>::Type::value_type color;
				presentation::sp::IntrinsicType<
					decltype(presentation::Border().sides.start().style)
				>::Type style;
				Scalar width;

				/// Default constructor.
				ComputedBorderSide() BOOST_NOEXCEPT :
					color(Color::TRANSPARENT_BLACK), style(presentation::Border::NONE), width(0) {}
				/// Returns the computed width in device units.
				Scalar computedWidth() const BOOST_NOEXCEPT {
					return (style != presentation::Border::NONE) ? width : 0;
				}
				/// Returns @c true if this side has visible style (but may or may not consume place).
				bool hasVisibleStyle() const BOOST_NOEXCEPT {
					return style != presentation::Border::NONE && style != presentation::Border::HIDDEN;
				}
				/// Returns @c true if the computed thickness of this side is zero.
				bool isAbsent() const BOOST_NOEXCEPT {
					return computedWidth() == 0;
				}
			};

			/**
			 * @see ComputedTextRunStyle
			 */
			struct ComputedFontSpecification : private boost::equality_comparable<ComputedFontSpecification> {
				const presentation::sp::IntrinsicType<
					decltype(presentation::TextRunStyle().fontFamily)
				>::Type families;
				double pointSize;
//				presentation::sp::IntrinsicType<
//					decltype(presentation::TextRunStyle().fontSize)
//				> pointSize;
				FontProperties properties;
				presentation::sp::IntrinsicType<
					decltype(presentation::TextRunStyle().fontSizeAdjust)
				>::Type sizeAdjust;

				/// Default constructor.
				ComputedFontSpecification() : families() {}
				ComputedFontSpecification& operator=(const ComputedFontSpecification&);
				/// Equality operator.
				bool operator==(const ComputedFontSpecification& other) const;
			};

			/// Computed value of @c presentation#TextDecoration.
			struct ComputedTextDecoration : private boost::equality_comparable<ComputedTextDecoration> {
				presentation::sp::IntrinsicType<
					decltype(presentation::TextDecoration().lines)
				>::Type lines;
				presentation::sp::IntrinsicType<
					decltype(presentation::TextDecoration().color)
				>::Type::value_type color;
				presentation::sp::IntrinsicType<
					decltype(presentation::TextDecoration().style)
				>::Type style;
				presentation::sp::IntrinsicType<
					decltype(presentation::TextDecoration().skip)
				>::Type skip;
				presentation::sp::IntrinsicType<
					decltype(presentation::TextDecoration().underlinePosition)
				>::Type underlinePosition;

				/// Default constructor initializes the members with initial values.
				ComputedTextDecoration() BOOST_NOEXCEPT :
					lines(presentation::TextDecoration::Line::NONE),
					color(Color::TRANSPARENT_BLACK),
					style(presentation::TextDecoration::Style::SOLID),
					skip(presentation::TextDecoration::Skip::OBJECTS),
					underlinePosition(presentation::TextDecoration::UnderlinePosition::AUTO) {}
				/// Equality operator.
				bool operator==(const ComputedTextDecoration& other) const BOOST_NOEXCEPT;
			};

			/// Computed value of @c presentation#TextEmphasis.
			struct ComputedTextEmphasis : private boost::equality_comparable<ComputedTextEmphasis> {
				presentation::sp::IntrinsicType<
					decltype(presentation::TextEmphasis().style)
				>::Type style;
				presentation::sp::IntrinsicType<
					decltype(presentation::TextEmphasis().color)
				>::Type::value_type color;
				presentation::sp::IntrinsicType<
					decltype(presentation::TextEmphasis().position)
				>::Type position;

				/// Default constructor initializes the members with initial values.
				ComputedTextEmphasis() BOOST_NOEXCEPT :
					style(presentation::TextEmphasis::NONE), color(Color::TRANSPARENT_BLACK),
					position(presentation::TextEmphasis::ABOVE | presentation::TextEmphasis::RIGHT) {}
				/// Equality operator.
				bool operator==(const ComputedTextEmphasis& other) const BOOST_NOEXCEPT;
			};
		}
	}

	namespace detail {
		void paintBorder(graphics::PaintContext& context,
			const graphics::Rectangle& rectangle,
			const graphics::PhysicalFourSides<graphics::font::ComputedBorderSide>& style,
			const presentation::WritingMode& writingMode);
	}
} // namespace ascension.graphics.font

#endif // !COMPUTED_TEXT_STYLES_HPP
