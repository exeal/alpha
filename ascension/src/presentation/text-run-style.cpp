/**
 * @file text-run-style.cpp
 * @author exeal
 * @date 2014-10-06 Created.
 */

#include <ascension/presentation/text-run-style.hpp>
#include <boost/flyweight.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>

namespace ascension {
	namespace presentation {
#if 0
#ifdef BOOST_NO_CXX11_DEFAULTED_FUNCTIONS
		/// Default constructor.
		DeclaredTextRunStyle::DeclaredTextRunStyle() {
		}
#endif
		/// Returns a @c DeclaredTextRunStyle instance filled with @c styles#UNSET values.
		const DeclaredTextRunStyle& DeclaredTextRunStyle::unsetInstance() {
			static const DeclaredTextRunStyle SINGLETON;
			return SINGLETON;
		}
			void computeColor(const styles::SpecifiedValue<styles::Color>::type& specifiedValue,
					const graphics::Color& computedParentColor, styles::ComputedValue<styles::Color>::type& computedValue) {
				computedValue = boost::get_optional_value_or(specifiedValue, computedParentColor);
			}

			void computeBackground(const styles::SpecifiedValue<TextRunStyle1>::type& specifiedValues,
					const graphics::Color& foregroundColor, const styles::Length::Context& lengthContext, styles::ComputedValue<TextRunStyle1>::type& computedValues) {
				boost::fusion::at_key<styles::BackgroundColor>(computedValues) =
					boost::get_optional_value_or(boost::fusion::at_key<styles::BackgroundColor>(specifiedValues), foregroundColor);
			}

			void computeBorder(const styles::SpecifiedValue<TextRunStyle1>::type& specifiedValues,
					const graphics::Color& foregroundColor, const styles::Length::Context& lengthContext, styles::ComputedValue<TextRunStyle1>::type& computedValues) {
				const auto& specifiedColors = boost::fusion::at_key<FlowRelativeFourSides<styles::BorderColor>>(specifiedValues);
				const auto& specifiedStyles = boost::fusion::at_key<FlowRelativeFourSides<styles::BorderStyle>>(specifiedValues);
				const auto& specifiedWidths = boost::fusion::at_key<FlowRelativeFourSides<styles::BorderWidth>>(specifiedValues);
				auto& computedColors = boost::fusion::at_key<FlowRelativeFourSides<styles::BorderColor>>(computedValues);
				auto& computedStyles = boost::fusion::at_key<FlowRelativeFourSides<styles::BorderStyle>>(computedValues);
				auto& computedWidths = boost::fusion::at_key<FlowRelativeFourSides<styles::BorderWidth>>(computedValues);
				for(std::size_t i = 0; specifiedColors.size(); ++i) {
					computedColors[i] = boost::get_optional_value_or(specifiedColors[i], foregroundColor);
					computedStyles[i] = specifiedStyles[i];
					computedWidths[i] = Pixels(!styles::isAbsent(computedStyles[i]) ? specifiedWidths[i].value(lengthContext) : 0);
				}
			}

			template<typename Property>
			inline void computePaddingOrMargin(const typename styles::SpecifiedValue<FlowRelativeFourSides<Property>>::type& specifiedValue,
					const styles::Length::Context& lengthContext, typename styles::ComputedValue<FlowRelativeFourSides<Property>>::type& computedValue,
					typename std::enable_if<std::is_same<Property, styles::PaddingSide>::value || std::is_same<Property, styles::MarginSide>::value>::type* = nullptr) {
				for(std::size_t i = 0; i < specifiedValue.size(); ++i) {
					if(const styles::Length* const length = boost::get<styles::Length>(&specifiedValue[i])) {
						computedValue[i] = Pixels(length->value(lengthContext));
						return;
					} else if(const styles::Percentage* const percentage = boost::get<styles::Percentage>(&specifiedValue[i])) {
						computedValue[i] = *percentage;
						return;
					}
					// TODO: Handle 'fill' keyword for Margin.
					computedValue[i] = Pixels::zero();
				}
			}

			inline void computeFontSize(const styles::SpecifiedValue<styles::FontSize>::type& specifiedValue,
					const styles::Length::Context& lengthContext, const Pixels& computedParentFontSize, const Pixels& mediumFontSize,
					styles::ComputedValue<styles::FontSize>::type& computedValue) {
				if(const styles::AbsoluteFontSize* const absoluteFontSize = boost::get<styles::AbsoluteFontSize>(&specifiedValue)) {
					// TODO: styles.AbsoluteFontSize should be double constant, not enum?
					static const std::array<styles::Number,
						styles::AbsoluteFontSize::XX_LARGE - styles::AbsoluteFontSize::XX_SMALL + 1>
							ABSOLUTE_SIZE_RATIOS = {3.f / 5.f, 3.f / 4.f, 8.f / 9.f, 1.f, 6.f / 5.f, 3.f / 2.f, 2.f / 1.f};
					static_assert(styles::AbsoluteFontSize::XX_SMALL == 0, "");
					if(*absoluteFontSize >= styles::AbsoluteFontSize::XX_SMALL && *absoluteFontSize <= styles::AbsoluteFontSize::XX_LARGE) {
						computedValue = mediumFontSize * ABSOLUTE_SIZE_RATIOS[boost::underlying_cast<std::size_t>(*absoluteFontSize)];
						return;
					}
				} else if(const styles::RelativeFontSize* const relativeFontSize = boost::get<styles::RelativeFontSize>(&specifiedValue)) {
					static const styles::Number RELATIVE_FACTOR = 1.2f;	// TODO: Is this right ?
					switch(boost::native_value(*relativeFontSize)) {
						case styles::RelativeFontSize::LARGER:
							computedValue = computedParentFontSize * RELATIVE_FACTOR;
							return;
						case styles::RelativeFontSize::SMALLER:
							computedValue = computedParentFontSize / RELATIVE_FACTOR;
							return;
					}
				} else if(const styles::Length* const length = boost::get<styles::Length>(&specifiedValue)) {
					if(length->valueInSpecifiedUnits() >= 0.0) {
						computedValue = Pixels(length->value(lengthContext));
						return;
					}
				} else if(const styles::Percentage* const percentage = boost::get<styles::Percentage>(&specifiedValue)) {
					if(*percentage >= 0) {	// [CSS3-FONTS] does not disallow negative value, but...
						computedValue = computedParentFontSize * boost::rational_cast<styles::Number>(*percentage);
						return;
					}
				}
				computedValue = mediumFontSize;
			}

			inline void computeAlignmentAdjust(const styles::SpecifiedValue<styles::AlignmentAdjust>::type& specifiedValue,
					const styles::Length::Context& lengthContext, styles::ComputedValue<styles::AlignmentAdjust>::type& computedValue) {
				// TODO: [CSS3INLINE] does not describe the computation for other than <percentage>.
				computedValue = Pixels::zero();
			}
		}

		boost::flyweight<styles::ComputedValue<TextRunStyle>::type> compute(
				const styles::SpecifiedValue<TextRunStyle>::type& specifiedValues,
				const styles::Length::Context& context,
				const styles::ComputedValue<TextRunStyle>::type& parentComputedValues) {
			computeColor(
				boost::fusion::at_key<styles::Color>(specifiedValues),
				boost::fusion::at_key<styles::Color>(parentComputedValues),
				boost::fusion::at_key<styles::Color>(computedValues));
			const auto& computedColor = boost::fusion::at_key<styles::Color>(computedValues);

			computeBackground(specifiedValues, computedColor, context, computedValues);
			computeBorder(specifiedValues, computedColor, context, computedValues);

			computePaddingOrMargin<styles::PaddingSide>(
				boost::fusion::at_key<FlowRelativeFourSides<styles::PaddingSide>>(specifiedValues),
				context,
				boost::fusion::at_key<FlowRelativeFourSides<styles::PaddingSide>>(computedValues));
			computePaddingOrMargin<styles::MarginSide>(
				boost::fusion::at_key<FlowRelativeFourSides<styles::MarginSide>>(specifiedValues),
				context,
				boost::fusion::at_key<FlowRelativeFourSides<styles::MarginSide>>(computedValues));

			styles::computeAsSpecified<styles::FontFamily>(specifiedValues, computedValues);
			computeFontSize(
				boost::fusion::at_key<styles::FontSize>(specifiedValues),
				context,
				boost::fusion::at_key<styles::FontSize>(parentComputedValues),
				Pixels(12),	// TODO: This is temporary.
				boost::fusion::at_key<styles::FontSize>(computedValues));
			const auto& computedFontSize = boost::fusion::at_key<styles::FontSize>(computedValues);
			styles::computeAsSpecified<styles::FontWeight>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::FontStretch>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::FontStyle>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::FontSizeAdjust>(specifiedValues, computedValues);
//			styles::computeAsSpecified<styles::FontFeatureSettings>(specifiedValues, computedValues);
//			styles::computeAsSpecified<styles::FontLanguageOverride>(specifiedValues, computedValues);

			styles::computeAsSpecified<styles::TextHeight>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::DominantBaseline>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::AlignmentBaseline>(specifiedValues, computedValues);
			computeAlignmentAdjust(
				boost::fusion::at_key<styles::AlignmentAdjust>(specifiedValues),
				context,
				boost::fusion::at_key<styles::AlignmentAdjust>(computedValues));

			styles::computeAsSpecified<styles::TextTransform>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::WhiteSpace>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::Hyphens>(specifiedValues, computedValues);
			computeWordSpacing(
				boost::fusion::at_key<styles::WordSpacing>(specifiedValues),
				context,
				boost::fusion::at_key<styles::WordSpacing>(computedValues));
			computeLetterSpacing(
				boost::fusion::at_key<styles::LetterSpacing>(specifiedValues),
				context,
				boost::fusion::at_key<styles::LetterSpacing>(computedValues));
			styles::computeAsSpecified<styles::HangingPunctuation>(specifiedValues, computedValues);

			computeTextDecoration(specifiedValues, computedColor, computedValues);
			computeTextEmphasis(specifiedValues, computedColor, computedValues);
			computeTextShadow();

			styles::computeAsSpecified<styles::Direction>(specifiedValues, computedValues);

			styles::computeAsSpecified<styles::ShapingEnabled>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::NumberSubstitution>(specifiedValues, computedValues);
		}

		std::size_t hash_value(const styles::ComputedValue<TextRunStyle>::type& style) {
			std::size_t seed = 0;

			boost::hash_combine(seed, boost::fusion::at_key<styles::Color>(style));

			boost::hash_combine(seed, boost::fusion::at_key<styles::BackgroundColor>(style));
			const auto& borderColors = boost::fusion::at_key<FlowRelativeFourSides<styles::BorderColor>>(style);
			boost::hash_range(seed, std::begin(borderColors), std::end(borderColors));
			const auto& borderStyles = boost::fusion::at_key<FlowRelativeFourSides<styles::BorderStyle>>(style);
			boost::hash_range(seed, std::begin(borderStyles), std::end(borderStyles));
			const auto& borderWidths = boost::fusion::at_key<FlowRelativeFourSides<styles::BorderWidth>>(style);
			boost::hash_range(seed, std::begin(borderWidths), std::end(borderWidths));

			const auto& padding = boost::fusion::at_key<FlowRelativeFourSides<styles::PaddingSide>>(style);
			boost::hash_range(seed, std::begin(padding), std::end(padding));
			const auto& margin = boost::fusion::at_key<FlowRelativeFourSides<styles::MarginSide>>(style);
			boost::hash_range(seed, std::begin(margin), std::end(margin));

			boost::hash_combine(seed, boost::fusion::at_key<styles::FontFamily>(style));
			boost::hash_combine(seed, boost::fusion::at_key<styles::FontWeight>(style));
			boost::hash_combine(seed, boost::fusion::at_key<styles::FontStretch>(style));
			boost::hash_combine(seed, boost::fusion::at_key<styles::FontStyle>(style));
			boost::hash_combine(seed, boost::fusion::at_key<styles::FontSize>(style));
			const auto& fontSizeAdjust = boost::fusion::at_key<styles::FontSizeAdjust>(style);
			if(fontSizeAdjust != boost::none)
				boost::hash_combine(seed, boost::get(fontSizeAdjust));
			else
				boost::hash_combine(seed, std::make_tuple());
//			boost::hash_combine(seed, boost::fusion::at_key<styles::FontFeatureSettings>(style));
//			boost::hash_combine(seed, boost::fusion::at_key<styles::FontLanguageOverride>(style));

			boost::hash_combine(seed, boost::fusion::at_key<styles::TextHeight>(style));
			boost::hash_combine(seed, boost::fusion::at_key<styles::DominantBaseline>(style));
			boost::hash_combine(seed, boost::fusion::at_key<styles::AlignmentBaseline>(style));
			boost::hash_combine(seed, boost::fusion::at_key<styles::AlignmentAdjust>(style));

			boost::hash_combine(seed, boost::fusion::at_key<styles::TextTransform>(style));
			boost::hash_combine(seed, boost::fusion::at_key<styles::WhiteSpace>(style));
			boost::hash_combine(seed, boost::fusion::at_key<styles::Hyphens>(style));
			boost::hash_combine(seed, boost::fusion::at_key<styles::WordSpacing>(style));
			boost::hash_combine(seed, boost::fusion::at_key<styles::LetterSpacing>(style));
			boost::hash_combine(seed, boost::fusion::at_key<styles::HangingPunctuation>(style));

			boost::hash_combine(seed, boost::fusion::at_key<styles::TextDecorationLine>(style));
			boost::hash_combine(seed, boost::fusion::at_key<styles::TextDecorationColor>(style));
			boost::hash_combine(seed, boost::fusion::at_key<styles::TextDecorationStyle>(style));
			boost::hash_combine(seed, boost::fusion::at_key<styles::TextDecorationSkip>(style));
			boost::hash_combine(seed, boost::fusion::at_key<styles::TextUnderlinePosition>(style));
			if(const auto& textEmphasisStyle = boost::fusion::at_key<styles::TextEmphasisStyle>(style))
				boost::hash_combine(seed, boost::get(textEmphasisStyle));
			else
				boost::hash_combine(seed, std::make_tuple());
			boost::hash_combine(seed, boost::fusion::at_key<styles::TextEmphasisColor>(style));
			boost::hash_combine(seed, boost::fusion::at_key<styles::TextEmphasisPosition>(style));
//			boost::hash_combine(seed, boost::fusion::at_key<styles::TextShadow>(style));

			boost::hash_combine(seed, boost::fusion::at_key<styles::Direction>(style));

			boost::hash_combine(seed, boost::fusion::at_key<styles::ShapingEnabled>(style));
			boost::hash_combine(seed, boost::fusion::at_key<styles::NumberSubstitution>(style));

			return seed;
		}
#endif
	}
}
