/**
 * @file actual-text-styles.hpp
 * Defines "Actual Values" of text styles.
 * @see CSS Cascading and Inheritance Level 3 (http://www.w3.org/TR/css-cascade-3/)
 * @see text-alignment.hpp, presentation/text-style.hpp
 * @date 2003-2006 (was LineLayout.h)
 * @date 2006-2010
 * @date 2010-11-20 renamed from ascension/layout.hpp
 * @date 2011-2012 was text-layout.hpp
 * @date 2012-08-17 separated from text-layout.hpp
 * @date 2013-04-25 separated from text-layout-styles.hpp
 * @date 2014-12-09 Renamed from computed-text-styles.hpp
 */

#ifndef ACTUAL_TEXT_STYLES_HPP
#define ACTUAL_TEXT_STYLES_HPP

#include <ascension/presentation/styles/background.hpp>
#include <ascension/presentation/styles/fonts.hpp>
#include <ascension/presentation/styles/text-decor.hpp>

namespace ascension {
	namespace graphics {
		class PaintContext;

		namespace font {
			/// "Actual Value"s of border style properties.
			struct ActualBorderSide : private boost::equality_comparable<ActualBorderSide> {
				/// The "Actual Value" of 'border-color' property.
				presentation::styles::ComputedValue<presentation::styles::BorderColor>::type color;
				/// The "Actual Value" of 'border-style' property.
				presentation::styles::ComputedValue<presentation::styles::BorderStyle>::type style;
				/// The "Actual Value" of 'border-width' property in user units (not in integer pixels).
				graphics::Scalar width;

				/// Default constructor.
				ActualBorderSide() BOOST_NOEXCEPT :
					color(Color::TRANSPARENT_BLACK), style(presentation::styles::BorderStyleEnums::NONE), width(0) {}
				/// Equality operator.
				bool operator==(const ActualBorderSide& other) const BOOST_NOEXCEPT {
					return color == other.color && style == other.style && width == other.width;
				}
				/// Returns the actual width in user units (not in integer pixels).
				graphics::Scalar actualWidth() const BOOST_NOEXCEPT {
					return (style != presentation::styles::BorderStyleEnums::NONE) ? width : 0;
				}
				/// Returns @c true if this side has visible style (but may or may not consume place).
				bool hasVisibleStyle() const BOOST_NOEXCEPT {
					return style != presentation::styles::BorderStyleEnums::NONE && style != presentation::styles::BorderStyleEnums::HIDDEN;
				}
				/// Returns @c true if the actual thickness of this side is zero.
				bool isAbsent() const BOOST_NOEXCEPT {
					return actualWidth() == 0;
				}
			};

			/// Specialization of @c boost#hash_value function template for @c ActualBorderSide.
			inline std::size_t hash_value(const ActualBorderSide& object) BOOST_NOEXCEPT {
				std::size_t seed = 0;
				boost::hash_combine(seed, object.color);
				boost::hash_combine(seed, object.style);
				boost::hash_combine(seed, object.width);
				return seed;
			}

			/// "Actual Value"s of font-related style properties.
			struct ActualFontSpecification : private boost::equality_comparable<ActualFontSpecification> {
				/// The "Actual Value" of 'font-family' property.
				presentation::styles::ComputedValue<presentation::styles::FontFamily>::type families;
				/// The "Actual Value" of 'font-size' property in points.
				double pointSize;
				/// The "Actual Value"s of other properties.
				FontProperties properties;
				/// The "Actual Value" of 'font-size-adjust' property.
				presentation::styles::ComputedValue<presentation::styles::FontSizeAdjust>::type sizeAdjust;

				/// Equality operator.
				bool operator==(const ActualFontSpecification& other) const BOOST_NOEXCEPT {
					return families == other.families && pointSize == other.pointSize
						&& properties == other.properties && sizeAdjust == other.sizeAdjust;
				}
			};

			/// Specialization of @c boost#hash_value function template for @c ActualFontSpecification.
			inline std::size_t hash_value(const ActualFontSpecification& object) BOOST_NOEXCEPT {
				std::size_t seed = boost::hash_range(std::begin(object.families), std::end(object.families));
				boost::hash_combine(seed, object.pointSize);
				boost::hash_combine(seed, object.properties);
				if(object.sizeAdjust != boost::none)
					boost::hash_combine(seed, boost::get(object.sizeAdjust));
				return seed;
			}

			/// "Actual Value"s of text decoration style properties.
			struct ActualTextDecoration : private boost::equality_comparable<ActualTextDecoration> {
				/// The "Actual Value" of 'text-decoration-line' property.
				presentation::styles::ComputedValue<presentation::styles::TextDecorationLine>::type lines;
				/// The "Actual Value" of 'text-decoration-color' property.
				presentation::styles::ComputedValue<presentation::styles::TextDecorationColor>::type color;
				/// The "Actual Value" of 'text-decoration-style' property.
				presentation::styles::ComputedValue<presentation::styles::TextDecorationStyle>::type style;
				/// The "Actual Value" of 'text-decoration-skip' property.
				presentation::styles::ComputedValue<presentation::styles::TextDecorationSkip>::type skip;
				/// The "Actual Value" of 'text-underline-position' property.
				presentation::styles::ComputedValue<presentation::styles::TextUnderlinePosition>::type underlinePosition;

				/// Default constructor initializes the members with initial values.
				ActualTextDecoration() BOOST_NOEXCEPT :
					lines(presentation::styles::TextDecorationLineEnums::NONE),
					color(Color::TRANSPARENT_BLACK),
					style(presentation::styles::TextDecorationStyleEnums::SOLID),
					skip(presentation::styles::TextDecorationSkipEnums::OBJECTS),
					underlinePosition(presentation::styles::TextUnderlinePositionEnums::AUTO) {}
				/// Initializes the all members with the given computed values.
				ActualTextDecoration(
					const presentation::styles::ComputedValue<presentation::styles::TextDecorationLine>::type& lines,
					const presentation::styles::ComputedValue<presentation::styles::TextDecorationColor>::type& color,
					const presentation::styles::ComputedValue<presentation::styles::TextDecorationStyle>::type& style,
					const presentation::styles::ComputedValue<presentation::styles::TextDecorationSkip>::type& skip,
					const presentation::styles::ComputedValue<presentation::styles::TextUnderlinePosition>::type& underlinePosition) :
					lines(lines), color(color), style(style), skip(skip), underlinePosition(underlinePosition) {}
				/// Equality operator.
				bool operator==(const ActualTextDecoration& other) const BOOST_NOEXCEPT {
					return lines == other.lines && color == other.color
						&& style == other.style && skip == other.skip && underlinePosition == other.underlinePosition;
				}
			};

			/// Specialization of @c boost#hash_value function template for @c ActualTextDecoration.
			inline std::size_t hash_value(const ActualTextDecoration& object) BOOST_NOEXCEPT {
				std::size_t seed = 0;
				boost::hash_combine<int>(seed, object.lines);
				boost::hash_combine(seed, object.color);
				boost::hash_combine<int>(seed, object.style);
				boost::hash_combine<int>(seed, object.skip);
				boost::hash_combine<int>(seed, object.underlinePosition);
				return seed;
			}

			/// "Actual Value"s of text emphasis style properties.
			struct ActualTextEmphasis : private boost::equality_comparable<ActualTextEmphasis> {
				/// The "Actual Value" of 'text-emphasis-style' property.
				presentation::styles::ComputedValue<presentation::styles::TextEmphasisStyle>::type style;
				/// The "Actual Value" of 'text-emphasis-color' property.
				presentation::styles::ComputedValue<presentation::styles::TextEmphasisColor>::type color;
				/// The "Actual Value" of 'text-emphasis-position' property.
				presentation::styles::ComputedValue<presentation::styles::TextEmphasisPosition>::type position;

				/// Default constructor initializes the members with initial values.
				ActualTextEmphasis() BOOST_NOEXCEPT :
					style(/*presentation::styles::TextEmphasisStyleEnums::NONE*/), color(Color::TRANSPARENT_BLACK),
					position(static_cast<BOOST_SCOPED_ENUM_NATIVE(presentation::styles::TextEmphasisPositionEnums)>(presentation::styles::TextEmphasisPositionEnums::OVER | presentation::styles::TextEmphasisPositionEnums::RIGHT)) {}
				/// Equality operator.
				bool operator==(const ActualTextEmphasis& other) const BOOST_NOEXCEPT {
					return style == other.style && color == other.color && position == other.position;
				}
			};

			/// Specialization of @c boost#hash_value function template for @c ActualTextEmphasis.
			inline std::size_t hash_value(const ActualTextEmphasis& object) BOOST_NOEXCEPT {
				std::size_t seed = 0;
				if(object.style != boost::none)
					boost::hash_combine<int>(seed, boost::get(object.style));
				boost::hash_combine(seed, object.color);
				boost::hash_combine(seed, object.position);
				return seed;
			}

			namespace detail {
				void paintBorder(PaintContext& context, const int& rectangle,
					const PhysicalFourSides<ActualBorderSide>& style, const presentation::WritingMode& writingMode);
			}
		}
	}
} // namespace ascension.graphics.font

namespace std {
	/// Specialization of @c std#hash class template for @c ActualBorderSide.
	template<>
	class hash<ascension::graphics::font::ActualBorderSide> :
		public std::function<std::hash<void*>::result_type(const ascension::graphics::font::ActualBorderSide&)> {
	public:
		result_type operator()(const argument_type& key) const BOOST_NOEXCEPT {
			return boost::hash<argument_type>()(key);
		}
	};

	/// Specialization of @c std#hash class template for @c ActualFontSpecification.
	template<>
	class hash<ascension::graphics::font::ActualFontSpecification> :
		public std::function<std::hash<void*>::result_type(const ascension::graphics::font::ActualFontSpecification&)> {
	public:
		result_type operator()(const argument_type& key) const BOOST_NOEXCEPT {
			return boost::hash<argument_type>()(key);
		}
	};

	/// Specialization of @c std#hash class template for @c ActualTextDecoration.
	template<>
	class hash<ascension::graphics::font::ActualTextDecoration> :
		public std::function<std::hash<void*>::result_type(const ascension::graphics::font::ActualTextDecoration&)> {
	public:
		result_type operator()(const argument_type& key) const BOOST_NOEXCEPT {
			return boost::hash<argument_type>()(key);
		}
	};

	/// Specialization of @c std#hash class template for @c ActualTextEmphasis.
	template<>
	class hash<ascension::graphics::font::ActualTextEmphasis> :
		public std::function<std::hash<void*>::result_type(const ascension::graphics::font::ActualTextEmphasis&)> {
	public:
		result_type operator()(const argument_type& key) const BOOST_NOEXCEPT {
			return boost::hash<argument_type>()(key);
		}
	};
}

#endif // !ACTUAL_TEXT_STYLES_HPP
