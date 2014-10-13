/**
 * @file text-line-style.cpp
 * @author exeal
 * @date 2014-09-28 Created.
 */

#include <ascension/presentation/text-line-style.hpp>
#include <ascension/presentation/text-run-style.hpp>
#include <boost/core/null_deleter.hpp>

namespace ascension {
	namespace presentation {
		namespace {
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

			void computeTextIndent(const styles::SpecifiedValueType<styles::TextIndent>::type& specifiedValue,
					const styles::Length::Context& context, styles::ComputedValueType<styles::TextIndent>::type& computedValue) {
				bool illegal = true;
				if(const styles::Percentage* const percentage = boost::get<styles::Percentage>(&specifiedValue.length)) {
					computedValue.length = *percentage;
					illegal = false;
				} else if(const styles::Length* const length = boost::get<styles::Length>(&specifiedValue.length)) {
					if(styles::Length::isValidUnit(length->unitType())) {
						computedValue.length = Pixels(length->value(context));
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

		/**
		 * Computes @c TextLineStyle.
		 * @param specifiedValues The "Specified Value"s to compute
		 * @param context The length context
		 * @param[out] computedValues The "Computed Value"s
		 */
		void computeTextLineStyle(const SpecifiedTextLineStyle& specifiedValues,
				const styles::Length::Context& context, ComputedTextLineStyle& computedValues) {
			styles::computeAsSpecified<styles::Direction>(specifiedValues, computedValues);
//			styles::computeAsSpecified<styles::UnicodeBidi>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::TextOrientation>(specifiedValues, computedValues);

			styles::detail::computeLineHeight(
				*boost::fusion::find<styles::SpecifiedValueType<styles::LineHeight>::type>(specifiedValues),
				Pixels(styles::Length(1, styles::Length::EM_HEIGHT).value(context)),
				*boost::fusion::find<styles::ComputedValueType<styles::LineHeight>::type>(computedValues));
			styles::computeAsSpecified<styles::LineBoxContain>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::DominantBaseline>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::InlineBoxAlignment>(specifiedValues, computedValues);

			styles::computeAsSpecified<styles::WhiteSpace>(specifiedValues, computedValues);
			computeTabSize(
				*boost::fusion::find<styles::SpecifiedValueType<styles::TabSize>::type>(specifiedValues),
				context,
				*boost::fusion::find<styles::ComputedValueType<styles::TabSize>::type>(computedValues));
			styles::computeAsSpecified<styles::LineBreak>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::WordBreak>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::OverflowWrap>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::TextAlignment>(specifiedValues, computedValues);	// TODO: Handle 'match-parent' keyword correctly.
			styles::computeAsSpecified<styles::TextAlignmentLast>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::TextJustification>(specifiedValues, computedValues);
			computeTextIndent(
				*boost::fusion::find<styles::SpecifiedValueType<styles::TextIndent>::type>(specifiedValues),
				context,
				*boost::fusion::find<styles::ComputedValueType<styles::TextIndent>::type>(computedValues));
			styles::computeAsSpecified<styles::HangingPunctuation>(specifiedValues, computedValues);

//			styles::computeAsSpecified<styles::Measure>(specifiedValues, computedValues);

			styles::computeAsSpecified<styles::NumberSubstitution>(specifiedValues, computedValues);
		}

		/**
		 * Returns the default line style of the given "Declared" line style.
		 * @param lineStyle The "Declared" line style
		 * @return lineStyle.defaultRunStyle, of a default-constructed @c TextRunStyle instance if @c null
		 * @see defaultTextLineStyle
		 */
		std::shared_ptr<const DeclaredTextRunStyle> defaultTextRunStyle(const DeclaredTextLineStyle& lineStyle) {
			static const DeclaredTextRunStyle defaultInstance;
			return (lineStyle.defaultRunStyle.get() != nullptr) ?
				lineStyle.defaultRunStyle : std::shared_ptr<const DeclaredTextRunStyle>(&defaultInstance, boost::null_deleter());
		}
	}
}
