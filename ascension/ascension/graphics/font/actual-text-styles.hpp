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

#ifndef ASCENSION_ACTUAL_TEXT_STYLES_HPP
#define ASCENSION_ACTUAL_TEXT_STYLES_HPP
#include <ascension/presentation/detail/style-sequence.hpp>
#include <ascension/presentation/styles/background.hpp>
#include <ascension/presentation/styles/fonts.hpp>
#include <ascension/presentation/styles/text-decor.hpp>
#include <boost/fusion/container/map.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>

namespace ascension {
	namespace presentation {
		struct ComputedTextRunStyle;
	}

	namespace graphics {
		class PaintContext;
		template<typename T> class PhysicalFourSides;

		namespace font {
			/**
			 * "Actual Value"s of border style properties. This includes:
			 * - The "Actual Value" of 'border-color' property.
			 * - The "Actual Value" of 'border-style' property.
			 * - The "Actual Value" of 'border-width' property in logical coordinate space units (not in integer pixels).
			 */
			struct ActualBorderSide : public boost::fusion::map<
				boost::fusion::pair<
					presentation::styles::BorderColor,
					presentation::styles::ComputedValue<presentation::styles::BorderColor>::type
				>,
				boost::fusion::pair<
					presentation::styles::BorderStyle,
					presentation::styles::ComputedValue<presentation::styles::BorderStyle>::type
				>,
				boost::fusion::pair<
					presentation::styles::BorderWidth,
					graphics::Scalar
				>
			> {
				/// Default constructor.
				ActualBorderSide() BOOST_NOEXCEPT {
					boost::fusion::at_key<presentation::styles::BorderColor>(*this) = Color::TRANSPARENT_BLACK;
					boost::fusion::at_key<presentation::styles::BorderStyle>(*this) = presentation::styles::BorderStyleEnums::NONE;
					boost::fusion::at_key<presentation::styles::BorderWidth>(*this) = 0;
				}
				/// Returns the actual width in user units (not in integer pixels).
				graphics::Scalar actualWidth() const BOOST_NOEXCEPT {
					return (boost::fusion::at_key<presentation::styles::BorderStyle>(*this) != presentation::styles::BorderStyleEnums::NONE) ?
						boost::fusion::at_key<presentation::styles::BorderWidth>(*this) : 0;
				}
				/// Returns @c true if this side has visible style (but may or may not consume place).
				bool hasVisibleStyle() const BOOST_NOEXCEPT {
					const auto style(boost::fusion::at_key<presentation::styles::BorderStyle>(*this));
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
				boost::hash_combine(seed, boost::fusion::at_key<presentation::styles::BorderColor>(object));
				boost::hash_combine(seed, boost::fusion::at_key<presentation::styles::BorderStyle>(object));
				boost::hash_combine(seed, boost::fusion::at_key<presentation::styles::BorderWidth>(object));
				return seed;
			}

			/**
			 * "Actual Value"s of font-related style properties. This includes:
			 * - The "Actual Value" of 'font-family' property.
			 * - The "Actual Value" of 'font-size' property in points.
			 * - The "Actual Value"s of other properties.
			 * - The "Actual Value" of 'font-size-adjust' property.
			 */
			typedef boost::fusion::map<
				boost::fusion::pair<presentation::styles::FontFamily, presentation::styles::ComputedValue<presentation::styles::FontFamily>::type>,
				boost::fusion::pair<presentation::styles::FontSize, double>,
				boost::fusion::pair<void, FontProperties>,
				boost::fusion::pair<presentation::styles::FontSizeAdjust, presentation::styles::ComputedValue<presentation::styles::FontSizeAdjust>::type>
			> ActualFontSpecification;

			/// Specialization of @c boost#hash_value function template for @c ActualFontSpecification.
			inline std::size_t hash_value(const ActualFontSpecification& object) BOOST_NOEXCEPT {
				const auto& families = boost::fusion::at_key<presentation::styles::FontFamily>(object);
				std::size_t seed = boost::hash_range(std::begin(families), std::end(families));
				boost::hash_combine(seed, boost::fusion::at_key<presentation::styles::FontSize>(object));
				boost::hash_combine(seed, boost::fusion::at_key<void>(object));
				const auto sizeAdjust(boost::fusion::at_key<presentation::styles::FontSizeAdjust>(object));
				if(sizeAdjust != boost::none)
					boost::hash_combine(seed, boost::get(sizeAdjust));
				return seed;
			}

			/**
			 * "Actual Value"s of text decoration style properties. This includes:
			 * - The "Actual Value" of 'text-decoration-line' property.
			 * - The "Actual Value" of 'text-decoration-color' property.
			 * - The "Actual Value" of 'text-decoration-style' property.
			 * - The "Actual Value" of 'text-decoration-skip' property.
			 * - The "Actual Value" of 'text-underline-position' property.
			 */
			typedef presentation::detail::TransformAsMap<
				boost::fusion::vector<
					presentation::styles::TextDecorationLine,
					presentation::styles::TextDecorationColor,
					presentation::styles::TextDecorationStyle,
					presentation::styles::TextDecorationSkip,
					presentation::styles::TextUnderlinePosition
				>,
				presentation::detail::KeyValueConverter<presentation::styles::ComputedValue>
			>::type ActualTextDecoration;

			/// Specialization of @c boost#hash_value function template for @c ActualTextDecoration.
			inline std::size_t hash_value(const ActualTextDecoration& object) BOOST_NOEXCEPT {
				std::size_t seed = 0;
				boost::hash_combine(seed, boost::native_value(boost::fusion::at_key<presentation::styles::TextDecorationLine>(object)));
				boost::hash_combine(seed, boost::fusion::at_key<presentation::styles::TextDecorationColor>(object));
				boost::hash_combine(seed, boost::native_value(boost::fusion::at_key<presentation::styles::TextDecorationStyle>(object)));
				boost::hash_combine(seed, boost::native_value(boost::fusion::at_key<presentation::styles::TextDecorationSkip>(object)));
				boost::hash_combine(seed, boost::native_value(boost::fusion::at_key<presentation::styles::TextUnderlinePosition>(object)));
				return seed;
			}

			class TextRun;

			void paintTextDecoration(PaintContext& context,
				const TextRun& run, const Point& origin, const ActualTextDecoration& style);

			/**
			 * "Actual Value"s of text emphasis style properties.
			 * - The "Actual Value" of 'text-emphasis-style' property.
			 * - The "Actual Value" of 'text-emphasis-color' property.
			 * - The "Actual Value" of 'text-emphasis-position' property.
			 */
			typedef presentation::detail::TransformAsMap<
				boost::fusion::vector<
					presentation::styles::TextEmphasisStyle,
					presentation::styles::TextEmphasisColor,
					presentation::styles::TextEmphasisPosition
				>,
				presentation::detail::KeyValueConverter<presentation::styles::ComputedValue>
			>::type ActualTextEmphasis;

			/// Specialization of @c boost#hash_value function template for @c ActualTextEmphasis.
			inline std::size_t hash_value(const ActualTextEmphasis& object) BOOST_NOEXCEPT {
				std::size_t seed = 0;
				const auto& style = boost::fusion::at_key<presentation::styles::TextEmphasisStyle>(object);
				if(style != boost::none)
					boost::hash_combine(seed, boost::get(style));
				boost::hash_combine(seed, boost::fusion::at_key<presentation::styles::TextEmphasisColor>(object));
				boost::hash_combine(seed, boost::fusion::at_key<presentation::styles::TextEmphasisPosition>(object));
				return seed;
			}

			namespace detail {
				void paintBorder(PaintContext& context, const int& rectangle,
					const PhysicalFourSides<ActualBorderSide>& style, const presentation::WritingMode& writingMode);
			}

			struct ActualTextRunStyleCore : private boost::equality_comparable<ActualTextRunStyleCore> {
				presentation::styles::ComputedValue<presentation::styles::Color>::type color;
				presentation::styles::ComputedValue<presentation::styles::BackgroundColor>::type backgroundColor;
				presentation::FlowRelativeFourSides<ActualBorderSide> borders;	// not PhysicalFourSides<> because of TextRun' interface
				presentation::FlowRelativeFourSides<Scalar> margins, paddings;	// not PhysicalFourSides<> because of TextRun' interface
				ActualTextDecoration textDecoration;
				ActualTextEmphasis textEmphasis;
//				ActualTextShadow textShadow;

				explicit ActualTextRunStyleCore(const presentation::ComputedTextRunStyle& computed,
					const presentation::styles::Length::Context& context, Scalar computedParentMeasure);
				bool operator==(const ActualTextRunStyleCore& other) const {
					return color == other.color && backgroundColor == other.backgroundColor
						&& borders == other.borders && paddings == other.paddings
						&& textDecoration == other.textDecoration && textEmphasis == other.textEmphasis
//						&& textShadow == other.textShadow
						;
				}
			};

			/// Specialization of @c boost#hash_value function template for @c ActualTextRunStyleCore.
			inline std::size_t hash_value(const ActualTextRunStyleCore& object) BOOST_NOEXCEPT {
				std::size_t seed = 0;
				boost::hash_combine(seed, object.color);
				boost::hash_combine(seed, object.backgroundColor);
				boost::hash_combine(seed, object.borders);
				boost::hash_combine(seed, object.paddings);
				boost::hash_combine(seed, object.textDecoration);
				boost::hash_combine(seed, object.textEmphasis);
//				boost::hash_combine(seed, object.textShadow);
				return seed;
			}
		}
	}
} // namespace ascension.graphics.font

namespace boost {
	using ascension::graphics::font::hash_value;
}

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

#endif // !ASCENSION_ACTUAL_TEXT_STYLES_HPP
