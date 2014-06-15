/**
 * @file computed-text-styles.hpp
 * @see text-alignment.hpp, presentation/text-style.hpp
 * @date 2003-2006 (was LineLayout.h)
 * @date 2006-2010
 * @date 2010-11-20 renamed from ascension/layout.hpp
 * @date 2011-2012 was text-layout.hpp
 * @date 2012-08-17 separated from text-layout.hpp
 * @date 2013-04-25 separated from text-layout-styles.hpp
 * @date 2013-2014
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
			struct ComputedBorderSide : private boost::equality_comparable<ComputedBorderSide> {
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
				/// Equality operator.
				bool operator==(const ComputedBorderSide& other) const BOOST_NOEXCEPT {
					return color == other.color && style == other.style && width == other.width;
				}
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

			/// Specialization of @c boost#hash_value function template for @c ComputedBorderSide.
			inline std::size_t hash_value(const ComputedBorderSide& object) BOOST_NOEXCEPT {
				std::size_t seed = 0;
				boost::hash_combine(seed, object.color);
				boost::hash_combine(seed, object.style);
				boost::hash_combine(seed, object.width);
				return seed;
			}

			/**
			 * @see ComputedTextRunStyle
			 */
			struct ComputedFontSpecification : private boost::equality_comparable<ComputedFontSpecification> {
				presentation::sp::IntrinsicType<
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

				/// Equality operator.
				bool operator==(const ComputedFontSpecification& other) const BOOST_NOEXCEPT {
					return families == other.families && pointSize == other.pointSize
						&& properties == other.properties && sizeAdjust == other.sizeAdjust;
				}
			};

			/// Specialization of @c boost#hash_value function template for @c ComputedFontSpecification.
			inline std::size_t hash_value(const ComputedFontSpecification& object) BOOST_NOEXCEPT {
				std::size_t seed = 0;
				for(auto i(std::begin(object.families)), e(std::end(object.families)); i != e; ++i)
					boost::hash_combine(seed, i->name());
				boost::hash_combine(seed, object.pointSize);
				boost::hash_combine(seed, object.properties);
				if(object.sizeAdjust != boost::none)
					boost::hash_combine(seed, boost::get(object.sizeAdjust));
				return seed;
			}

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
				bool operator==(const ComputedTextDecoration& other) const BOOST_NOEXCEPT {
					return lines == other.lines && color == other.color
						&& style == other.style && skip == other.skip && underlinePosition == other.underlinePosition;
				}
			};

			/// Specialization of @c boost#hash_value function template for @c ComputedTextDecoration.
			inline std::size_t hash_value(const ComputedTextDecoration& object) BOOST_NOEXCEPT {
				std::size_t seed = 0;
				boost::hash_combine<int>(seed, object.lines);
				boost::hash_combine(seed, object.color);
				boost::hash_combine<int>(seed, object.style);
				boost::hash_combine<int>(seed, object.skip);
				boost::hash_combine<int>(seed, object.underlinePosition);
				return seed;
			}

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
				bool operator==(const ComputedTextEmphasis& other) const BOOST_NOEXCEPT {
					return style == other.style && color == other.color && position == other.position;
				}
			};

			/// Specialization of @c boost#hash_value function template for @c ComputedTextEmphasis.
			inline std::size_t hash_value(const ComputedTextEmphasis& object) BOOST_NOEXCEPT {
				std::size_t seed = 0;
				boost::hash_combine(seed, object.style);
				boost::hash_combine(seed, object.color);
				boost::hash_combine(seed, object.position);
				return seed;
			}

			namespace detail {
				void paintBorder(PaintContext& context, const Rectangle& rectangle,
					const PhysicalFourSides<ComputedBorderSide>& style, const presentation::WritingMode& writingMode);
			}
		}
	}
} // namespace ascension.graphics.font

namespace std {
	/// Specialization of @c std#hash class template for @c ComputedBorderSide.
	template<>
	class hash<ascension::graphics::font::ComputedBorderSide> :
		public std::function<std::hash<void*>::result_type(const ascension::graphics::font::ComputedBorderSide&)> {
	public:
		result_type operator()(const argument_type& key) const BOOST_NOEXCEPT {
			return boost::hash<argument_type>()(key);
		}
	};

	/// Specialization of @c std#hash class template for @c ComputedFontSpecification.
	template<>
	class hash<ascension::graphics::font::ComputedFontSpecification> :
		public std::function<std::hash<void*>::result_type(const ascension::graphics::font::ComputedFontSpecification&)> {
	public:
		result_type operator()(const argument_type& key) const BOOST_NOEXCEPT {
			return boost::hash<argument_type>()(key);
		}
	};

	/// Specialization of @c std#hash class template for @c ComputedTextDecoration.
	template<>
	class hash<ascension::graphics::font::ComputedTextDecoration> :
		public std::function<std::hash<void*>::result_type(const ascension::graphics::font::ComputedTextDecoration&)> {
	public:
		result_type operator()(const argument_type& key) const BOOST_NOEXCEPT {
			return boost::hash<argument_type>()(key);
		}
	};

	/// Specialization of @c std#hash class template for @c ComputedTextEmphasis.
	template<>
	class hash<ascension::graphics::font::ComputedTextEmphasis> :
		public std::function<std::hash<void*>::result_type(const ascension::graphics::font::ComputedTextEmphasis&)> {
	public:
		result_type operator()(const argument_type& key) const BOOST_NOEXCEPT {
			return boost::hash<argument_type>()(key);
		}
	};
}

#endif // !COMPUTED_TEXT_STYLES_HPP
