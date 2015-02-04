/**
 * @file text-line-style.cpp
 * @author exeal
 * @date 2014-09-28 Created.
 */

#include <ascension/presentation/text-line-style.hpp>
#include <ascension/presentation/text-run-style.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/flyweight.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>

namespace ascension {
	namespace presentation {
		namespace {
			inline void computeBaselineShift(const styles::SpecifiedValue<styles::BaselineShift>::type& specifiedValue,
					const styles::Length::Context& lengthContext, styles::ComputedValue<styles::BaselineShift>::type& computedValue) {
				// TODO: [CSS3INLINE] does not describe the computation for other than <percentage>.
				computedValue = Pixels::zero();
			}

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
		 * @return The "Computed Value"s
		 */
		boost::flyweight<styles::ComputedValue<TextLineStyle>::type> compute(
				const styles::SpecifiedValue<TextLineStyle>::type& specifiedValues, const styles::Length::Context& context) {
			styles::ComputedValue<TextLineStyle>::type computedValues;
			styles::computeAsSpecified<styles::Direction>(specifiedValues, computedValues);
//			styles::computeAsSpecified<styles::UnicodeBidi>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::TextOrientation>(specifiedValues, computedValues);

			styles::detail::computeLineHeight(
				boost::fusion::at_key<styles::LineHeight>(specifiedValues),
				Pixels(styles::Length(1, styles::Length::EM_HEIGHT).value(context)),
				boost::fusion::at_key<styles::LineHeight>(computedValues));
			styles::computeAsSpecified<styles::LineBoxContain>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::DominantBaseline>(specifiedValues, computedValues);
			computeBaselineShift(
				boost::fusion::at_key<styles::BaselineShift>(specifiedValues),
				context,
				boost::fusion::at_key<styles::BaselineShift>(computedValues));
			styles::computeAsSpecified<styles::InlineBoxAlignment>(specifiedValues, computedValues);

			styles::computeAsSpecified<styles::WhiteSpace>(specifiedValues, computedValues);
			computeTabSize(
				boost::fusion::at_key<styles::TabSize>(specifiedValues),
				context,
				boost::fusion::at_key<styles::TabSize>(computedValues));
			styles::computeAsSpecified<styles::LineBreak>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::WordBreak>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::OverflowWrap>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::TextAlignment>(specifiedValues, computedValues);	// TODO: Handle 'match-parent' keyword correctly.
			styles::computeAsSpecified<styles::TextAlignmentLast>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::TextJustification>(specifiedValues, computedValues);
			computeTextIndent(
				boost::fusion::at_key<styles::TextIndent>(specifiedValues),
				context,
				boost::fusion::at_key<styles::TextIndent>(computedValues));
			styles::computeAsSpecified<styles::HangingPunctuation>(specifiedValues, computedValues);

//			styles::computeAsSpecified<styles::Measure>(specifiedValues, computedValues);

			styles::computeAsSpecified<styles::NumberSubstitution>(specifiedValues, computedValues);

			return boost::flyweight<styles::ComputedValue<TextLineStyle>::type>(computedValues);
		}

		/// @c boost#hash_value for @c ComputedTextLineStyle.
		std::size_t hash_value(const styles::ComputedValue<TextLineStyle>::type& style) {
			std::size_t seed = 0;

			boost::hash_combine(seed, boost::fusion::at_key<styles::Direction>(style));
//			boost::hash_combine(seed, boost::fusion::at_key<styles::UnicodeBidi>(style));
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
