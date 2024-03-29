/**
 * @file text-line-style.cpp
 * @author exeal
 * @date 2014-09-28 Created.
 */

#include <ascension/presentation/text-line-style.hpp>
#include <ascension/presentation/text-run-style.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>

namespace ascension {
	namespace presentation {
		namespace {
			struct Initializer {
				template<typename Property>
				void operator()(Property& specifiedValue) const {
					specifiedValue = Property::first_type::initialValue();
				}
			};
		}

		/// Creates a @c SpecifiedTextLineStyle and initializes the all members with the initial values.
		SpecifiedTextLineStyle::SpecifiedTextLineStyle() {
			boost::fusion::for_each(*this, Initializer());
		}

		namespace {
#if 1
			inline void computeLineHeight(
					const styles::SpecifiedValue<styles::LineHeight>::type& specifiedValue,
					styles::ComputedValue<styles::LineHeight>::type& computedValue) {
				if(const auto* const keyword = boost::get<BOOST_SCOPED_ENUM_NATIVE(styles::LineHeightEnums)>(&specifiedValue)) {
					if(*keyword == styles::LineHeightEnums::NONE) {
						computedValue = std::make_tuple();
						return;
					}
				} else if(const styles::Length* const length = boost::get<styles::Length>(&specifiedValue)) {
					computedValue = *length;
					return;
				} else if(const styles::Number* const number = boost::get<styles::Number>(&specifiedValue)) {
					computedValue = *number;
					return;
				} else if(const styles::Percentage* const percentage = boost::get<styles::Percentage>(&specifiedValue)) {
					computedValue = *percentage;
					return;
				}
				computedValue = static_cast<styles::Number>(1.1f);	// [CSS3-INLINE] recommends between 1.0 to 1.2 for 'normal'
			}
#else
			inline void computeLineHeight(const styles::SpecifiedValue<LineHeight>::type& specifiedValue,
					const Pixels& computedFontSize, styles::ComputedValue<LineHeight>::type& computedValue) {
				if(const LineHeightEnums* const keyword = boost::get<LineHeightEnums>(&specifiedValue)) {
					if(*keyword == LineHeightEnums::NONE) {
						computedValue = std::make_tuple();
						return;
					}
				} else if(const Length* const length = boost::get<Length>(&specifiedValue)) {
					if(Length::isValidUnit(length->unitType()) && length->valueInSpecifiedUnits() >= 0) {
						computedValue = *length;
						return;
					}
				} else if(const Number* const number = boost::get<Number>(&specifiedValue)) {
					if(*number >= 0) {
						computedValue = *number;
						return;
					}
				} else if(const Percentage* const percentage = boost::get<Percentage>(&specifiedValue)) {
					if(*percentage >= 0) {
						computedValue = computedFontSize * boost::rational_cast<Number>(*percentage);
						return;
					}
				}
				computedValue = static_cast<Number>(1.1f);	// [CSS3-INLINE] recommends between 1.0 to 1.2 for 'normal'
			}

			inline void computeBaselineShift(const styles::SpecifiedValue<styles::BaselineShift>::type& specifiedValue,
					const styles::Length::Context& lengthContext, styles::ComputedValue<styles::BaselineShift>::type& computedValue) {
				// TODO: [CSS3INLINE] does not describe the computation for other than <percentage>.
				computedValue = Pixels::zero();
			}
#endif
			void computeTabSize(const styles::SpecifiedValue<styles::TabSize>::type& specifiedValue,
					const styles::Length::Context& context, styles::ComputedValue<styles::TabSize>::type& computedValue) {
				if(const styles::Integer* const integer = boost::get<styles::Integer>(&specifiedValue)) {
					if(*integer >= 0) {
						computedValue = *integer;
						return;
					}
				} else if(const styles::Length* const length = boost::get<styles::Length>(&specifiedValue)) {
					if(styles::Length::isValidUnit(length->unitType()) && length->valueInSpecifiedUnits() >= 0) {
						computedValue = 0;//Pixels(length->value(context));
						return;
					}
				}
				computeTabSize(styles::TabSize::initialValue(), context, computedValue);
			}

			void computeTextIndent(const styles::SpecifiedValue<styles::TextIndent>::type& specifiedValue,
					const styles::Length::Context& context, styles::ComputedValue<styles::TextIndent>::type& computedValue) {
				bool illegal = true;
				if(const styles::Percentage* const percentage = boost::get<styles::Percentage>(&specifiedValue.length)) {
					computedValue.length = *percentage;
					illegal = false;
				} else if(const styles::Length* const length = boost::get<styles::Length>(&specifiedValue.length)) {
					if(styles::Length::isValidUnit(length->unitType())) {
						computedValue.length = *length;
						illegal = false;
					}
				}

				if(!illegal) {
					computedValue.hanging = specifiedValue.hanging;
					computedValue.eachLine = specifiedValue.eachLine;
				} else
					computeTextIndent(styles::TextIndent::initialValue(), context, computedValue);
			}
		}

		namespace {
			void compute(ComputedTextLineStyle& computedValues, const SpecifiedTextLineStyle& specifiedValues, const styles::Length::Context* context) {
				styles::computeAsSpecified<styles::Direction>(specifiedValues, computedValues);
//				styles::computeAsSpecified<styles::UnicodeBidi>(specifiedValues, computedValues);
				styles::computeAsSpecified<styles::TextOrientation>(specifiedValues, computedValues);

				computeLineHeight(
					boost::fusion::at_key<styles::LineHeight>(specifiedValues),
					boost::fusion::at_key<styles::LineHeight>(computedValues));
				styles::computeAsSpecified<styles::LineBoxContain>(specifiedValues, computedValues);
				styles::computeAsSpecified<styles::DominantBaseline>(specifiedValues, computedValues);
				styles::computeAsSpecified<styles::BaselineShift>(specifiedValues, computedValues);
				styles::computeAsSpecified<styles::InlineBoxAlignment>(specifiedValues, computedValues);

				styles::computeAsSpecified<styles::WhiteSpace>(specifiedValues, computedValues);
				if(context != nullptr)
					computeTabSize(boost::fusion::at_key<styles::TabSize>(specifiedValues), *context, boost::fusion::at_key<styles::TabSize>(computedValues));
				else
					styles::computeAsSpecified<styles::TabSize>(specifiedValues, computedValues);
				styles::computeAsSpecified<styles::LineBreak>(specifiedValues, computedValues);
				styles::computeAsSpecified<styles::WordBreak>(specifiedValues, computedValues);
				styles::computeAsSpecified<styles::OverflowWrap>(specifiedValues, computedValues);
				styles::computeAsSpecified<styles::TextAlignment>(specifiedValues, computedValues);	// TODO: Handle 'match-parent' keyword correctly.
				styles::computeAsSpecified<styles::TextAlignmentLast>(specifiedValues, computedValues);
				styles::computeAsSpecified<styles::TextJustification>(specifiedValues, computedValues);
				if(context != nullptr)
					computeTextIndent(boost::fusion::at_key<styles::TextIndent>(specifiedValues), *context, boost::fusion::at_key<styles::TextIndent>(computedValues));
				else
					styles::computeAsSpecified<styles::TextIndent>(specifiedValues, computedValues);
				styles::computeAsSpecified<styles::HangingPunctuation>(specifiedValues, computedValues);

				styles::computeAsSpecified<styles::Measure>(specifiedValues, computedValues);

				styles::computeAsSpecified<styles::NumberSubstitution>(specifiedValues, computedValues);
			}
		}

		/**
		 * Computes and creates a @c ComputedTextLineStyle without a @c styles#Length#Context.
		 * @param specifiedValues The "Specified Value"s of @c TextLineStyle
		 */
		ComputedTextLineStyle::ComputedTextLineStyle(const SpecifiedTextLineStyle& specifiedValues) {
			compute(*this, specifiedValues, nullptr);
		}

		/**
		* Computes and creates a @c ComputedTextLineStyle.
		* @param specifiedValues The "Specified Value"s of @c TextLineStyle
		* @param context
		*/
		ComputedTextLineStyle::ComputedTextLineStyle(const SpecifiedTextLineStyle& specifiedValues, const styles::Length::Context& context) {
			compute(*this, specifiedValues, &context);
		}

		namespace {
			template<template<typename> class Metafunction>
			inline std::size_t hashTextLineStyle(const typename Metafunction<TextLineStyle>::type& style) {
				std::size_t seed = 0;

				boost::hash_combine(seed, boost::fusion::at_key<styles::Direction>(style));
//				boost::hash_combine(seed, boost::fusion::at_key<styles::UnicodeBidi>(style));
				boost::hash_combine(seed, boost::fusion::at_key<styles::TextOrientation>(style));

				boost::hash_combine(seed, boost::fusion::at_key<styles::LineHeight>(style));
				boost::hash_combine(seed, boost::fusion::at_key<styles::LineBoxContain>(style));
				boost::hash_combine(seed, boost::fusion::at_key<styles::DominantBaseline>(style));
				boost::hash_combine(seed, boost::fusion::at_key<styles::BaselineShift>(style));
				boost::hash_combine(seed, boost::fusion::at_key<styles::InlineBoxAlignment>(style));

				boost::hash_combine(seed, boost::fusion::at_key<styles::WhiteSpace>(style));
				boost::hash_combine(seed, boost::fusion::at_key<styles::TabSize>(style));
				boost::hash_combine(seed, boost::fusion::at_key<styles::LineBreak>(style));
				boost::hash_combine(seed, boost::fusion::at_key<styles::WordBreak>(style));
				boost::hash_combine(seed, boost::fusion::at_key<styles::OverflowWrap>(style));
				boost::hash_combine(seed, boost::fusion::at_key<styles::TextAlignment>(style));
				boost::hash_combine(seed, boost::fusion::at_key<styles::TextAlignmentLast>(style));
				boost::hash_combine(seed, boost::fusion::at_key<styles::TextJustification>(style));
				boost::hash_combine(seed, boost::fusion::at_key<styles::TextIndent>(style));
				boost::hash_combine(seed, boost::fusion::at_key<styles::HangingPunctuation>(style));

				boost::hash_combine(seed, boost::fusion::at_key<styles::Measure>(style));

				boost::hash_combine(seed, boost::fusion::at_key<styles::NumberSubstitution>(style));

				return seed;
			}
		}

		/// @c boost#hash_value for @c ComputedTextLineStyle.
		std::size_t hash_value(const styles::ComputedValue<TextLineStyle>::type& style) {
			return hashTextLineStyle<styles::ComputedValue>(style);
		}

		/// @c boost#hash_value for @c SpecifiedTextLineStyle.
		std::size_t hash_value(const styles::SpecifiedValue<TextLineStyle>::type& style) {
			return hashTextLineStyle<styles::SpecifiedValue>(style);
		}

		/// Default constructor.
		DeclaredTextLineStyle::DeclaredTextLineStyle() {
			setRunsStyle(std::shared_ptr<const DeclaredTextRunStyle>());
		}

		/**
		 * Sets the default @c DeclaredTextRunStyle of this line element.
		 * @param newStyle The style collection to set. If @c null, this @c DeclaredTextLineStyle holds a
		 *                 default-constructed @c DeclaredTextRunStyle
		 */
		void DeclaredTextLineStyle::setRunsStyle(std::shared_ptr<const DeclaredTextRunStyle> newStyle) BOOST_NOEXCEPT {
			if(newStyle.get() != nullptr)
				runsStyle_ = newStyle;
			else
				runsStyle_ = std::shared_ptr<const DeclaredTextRunStyle>(&DeclaredTextRunStyle::unsetInstance(), boost::null_deleter());
		}

		/// Returns a @c DeclaredTextLineStyle instance filled with @c styles#UNSET values.
		const DeclaredTextLineStyle& DeclaredTextLineStyle::unsetInstance() {
			static const DeclaredTextLineStyle SINGLETON;
			return SINGLETON;
		}
	}
}
