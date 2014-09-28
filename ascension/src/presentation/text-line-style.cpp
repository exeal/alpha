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
		}

		/**
		 * Computes @c TextLineStyle.
		 * @param specifiedValues The "Specified Value"s to compute
		 * @param[out] computedValues The "Computed Value"s
		 */
		void computeTextLineStyle(const SpecifiedTextLineStyle& specifiedValues, ComputedTextLineStyle& computedValues) {
			computeAsSpecified<styles::Direction>(specifiedValues, computedValues);
//			computeAsSpecified<styles::UnicodeBidi>(specifiedValues, computedValues);
			computeAsSpecified<styles::TextOrientation>(specifiedValues, computedValues);

//			computeAsSpecified<styles::LineHeight>(specifiedValues, computedValues);
			computeAsSpecified<styles::LineBoxContain>(specifiedValues, computedValues);
			computeAsSpecified<styles::DominantBaseline>(specifiedValues, computedValues);
			computeAsSpecified<styles::InlineBoxAlignment>(specifiedValues, computedValues);

			computeAsSpecified<styles::WhiteSpace>(specifiedValues, computedValues);
//			computeAsSpecified<>(specifiedValues, computedValues);
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
