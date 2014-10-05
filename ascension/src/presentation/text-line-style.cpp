/**
 * @file text-line-style.cpp
 * @author exeal
 * @date 2014-09-28 Created.
 */

#include <ascension/presentation/text-line-style.hpp>

namespace ascension {
	namespace presentation {
		namespace {
			template<typename Property>
			inline void computeAsSpecified(const SpecifiedTextLineStyle& specifiedValues, ComputedTextLineStyle& computedValues) {
				*boost::fusion::find<styles::ComputedValueType<Property>::type>(computedValues)
					= *boost::fusion::find<styles::SpecifiedValueType<Property>::type>(specifiedValues);
			}

			void computeLineHeight(const styles::SpecifiedValueType<styles::LineHeight>::type& specifiedValue,
					const Pixels& computedFontSize, styles::ComputedValueType<styles::LineHeight>::type& computedValue) {
				if(const styles::LineHeightEnums* const keyword = boost::get<styles::LineHeightEnums>(&specifiedValue)) {
					if(*keyword == styles::LineHeightEnums::NORMAL) {
						computedValue = static_cast<styles::Number>(1.1f);	// [CSS3-INLINE] recommends between 1.0 to 1.2
						return;
					} else if(*keyword == styles::LineHeightEnums::NONE) {
						computedValue = boost::none;
						return;
					}
				} else if(const styles::Length* const length = boost::get<styles::Length>(&specifiedValue)) {
					if(styles::Length::isValidUnit(length->unitType()) && length->valueInSpecifiedUnits() >= 0) {
						computedValue = *length;
						return;
					}
				} else if(const styles::Number* const number = boost::get<styles::Number>(&specifiedValue)) {
					if(*number >= 0) {
						computedValue = *number;
						return;
					}
				} else if(const styles::Percentage* const percentage = boost::get<styles::Percentage>(&specifiedValue)) {
					const styles::Number r = boost::rational_cast<styles::Number>(*percentage);
					if(r >= 0) {
						computedValue = computedFontSize * r;
						return;
					}
				}
			}

			void computeTabSize(const styles::SpecifiedValueType<styles::TabSize>::type& specifiedValue,
					const styles::Length::Context& context, styles::ComputedValueType<styles::TabSize>::type& computedValue) {
				if(const styles::Integer* const integer = boost::get<styles::Integer>(&specifiedValue)) {
					if(*integer >= 0) {
						computedValue = *integer;
						return;
					}
				} else if(const styles::Length* const length = boost::get<styles::Length>(&specifiedValue)) {
					if(styles::Length::isValidUnit(length->unitType()) && length->valueInSpecifiedUnits() >= 0) {
						computedValue = Pixels(length->value(context));
						return;
					}
				}
				computeTabSize(styles::TabSize::initialValue(), context, computedValue);
			}
		}

		/**
		 * Computes @c TextLineStyle.
		 * @param specifiedValues The "Specified Value"s to compute
		 * @param context The length context
		 * @param[out] computedValues The "Computed Value"s
		 */
		void computeTextLineStyle(const SpecifiedTextLineStyle& specifiedValues,
				const styles::Length::Context& context, ComputedTextLineStyle& computedValues) {
			computeAsSpecified<styles::Direction>(specifiedValues, computedValues);
//			computeAsSpecified<styles::UnicodeBidi>(specifiedValues, computedValues);
			computeAsSpecified<styles::TextOrientation>(specifiedValues, computedValues);

			computeLineHeight(
				*boost::fusion::find<styles::SpecifiedValueType<styles::LineHeight>::type>(specifiedValues),
				Pixels(styles::Length(1, styles::Length::EM_HEIGHT).value(context)),
				*boost::fusion::find<styles::ComputedValueType<styles::LineHeight>::type>(computedValues));
			computeAsSpecified<styles::LineBoxContain>(specifiedValues, computedValues);
			computeAsSpecified<styles::DominantBaseline>(specifiedValues, computedValues);
			computeAsSpecified<styles::InlineBoxAlignment>(specifiedValues, computedValues);

			computeAsSpecified<styles::WhiteSpace>(specifiedValues, computedValues);
			computeTabSize(
				*boost::fusion::find<styles::SpecifiedValueType<styles::TabSize>::type>(specifiedValues),
				context,
				*boost::fusion::find<styles::ComputedValueType<styles::TabSize>::type>(computedValues));
			computeAsSpecified<styles::LineBreak>(specifiedValues, computedValues);
			computeAsSpecified<styles::WordBreak>(specifiedValues, computedValues);
			computeAsSpecified<styles::OverflowWrap>(specifiedValues, computedValues);
//			computeAsSpecified<styles::TextAlignment>(specifiedValues, computedValues);
			computeAsSpecified<styles::TextAlignmentLast>(specifiedValues, computedValues);
			computeAsSpecified<styles::TextJustification>(specifiedValues, computedValues);
//			computeAsSpecified<styles::TextIndent>(specifiedValues, computedValues);
			computeAsSpecified<styles::HangingPunctuation>(specifiedValues, computedValues);

//			computeAsSpecified<styles::Measure>(specifiedValues, computedValues);

			computeAsSpecified<styles::NumberSubstitution>(specifiedValues, computedValues);
		}
	}
}
