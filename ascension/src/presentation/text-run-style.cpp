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

		namespace {
			inline void computeColor(const styles::SpecifiedValue<styles::Color>::type& specifiedValue,
					const graphics::Color& computedParentColor, styles::ComputedValue<styles::Color>::type& computedValue) {
				computedValue = boost::get_optional_value_or(specifiedValue, computedParentColor);
			}

			inline void computeBackground(const styles::SpecifiedValue<TextRunStyle>::type& specifiedValues,
					const graphics::Color& foregroundColor, styles::ComputedValue<TextRunStyle>::type& computedValues) {
				boost::fusion::at_key<styles::BackgroundColor>(computedValues.backgroundsAndBorders) =
					boost::get_optional_value_or(boost::fusion::at_key<styles::BackgroundColor>(specifiedValues.backgroundsAndBorders), foregroundColor);
			}

			void computeBorder(const styles::SpecifiedValue<TextRunStyle>::type& specifiedValues,
					const graphics::Color& foregroundColor, /*const styles::Length::Context& lengthContext,*/ styles::ComputedValue<TextRunStyle>::type& computedValues) {
				const auto& specifiedColors = boost::fusion::at_key<FlowRelativeFourSides<styles::BorderColor>>(specifiedValues.backgroundsAndBorders);
				const auto& specifiedStyles = boost::fusion::at_key<FlowRelativeFourSides<styles::BorderStyle>>(specifiedValues.backgroundsAndBorders);
				const auto& specifiedWidths = boost::fusion::at_key<FlowRelativeFourSides<styles::BorderWidth>>(specifiedValues.backgroundsAndBorders);
				auto& computedColors = boost::fusion::at_key<FlowRelativeFourSides<styles::BorderColor>>(computedValues.backgroundsAndBorders);
				auto& computedStyles = boost::fusion::at_key<FlowRelativeFourSides<styles::BorderStyle>>(computedValues.backgroundsAndBorders);
				auto& computedWidths = boost::fusion::at_key<FlowRelativeFourSides<styles::BorderWidth>>(computedValues.backgroundsAndBorders);
				for(std::size_t i = 0; specifiedColors.size(); ++i) {
					computedColors[i] = boost::get_optional_value_or(specifiedColors[i], foregroundColor);
					computedStyles[i] = specifiedStyles[i];
#if 0
					computedWidths[i] = Pixels(!styles::isAbsent(computedStyles[i]) ? specifiedWidths[i].value(lengthContext) : 0);
#else
					computedWidths[i] = specifiedWidths[i];
#endif
				}
			}

			template<typename Property>
			inline void computePaddingOrMargin(const typename styles::SpecifiedValue<FlowRelativeFourSides<Property>>::type& specifiedValue,
					typename styles::ComputedValue<FlowRelativeFourSides<Property>>::type& computedValue,
					typename std::enable_if<std::is_same<Property, styles::PaddingSide>::value || std::is_same<Property, styles::MarginSide>::value>::type* = nullptr) {
				for(std::size_t i = 0; i < specifiedValue.size(); ++i) {
					if(const styles::Length* const length = boost::get<styles::Length>(&specifiedValue[i]))
						computedValue[i] = *length;
					else if(const styles::Percentage* const percentage = boost::get<styles::Percentage>(&specifiedValue[i]))
						computedValue[i] = *percentage;
					else if(const styles::PaddingEnums* const paddingEnums = boost::get<styles::PaddingEnums>(&specifiedValue[i]))
						computedValue[i] = std::make_tuple();
					else if(const styles::MarginEnums* const marginEnums = boost::get<styles::MarginEnums>(&specifiedValue[i]))
						computedValue[i] = std::make_tuple();	// TODO: Handle 'fill' keyword.
					else
						computedValue[i] = Property::initialValue();
				}
			}
#if 0
			inline void computeFontSize(const styles::SpecifiedValue<styles::FontSize>::type& specifiedValue,
					/*const styles::Length::Context& lengthContext,*/ const Pixels& computedParentFontSize, const Pixels& mediumFontSize,
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
#endif
#if 0
			inline void computeWordSpacing(const styles::SpecifiedValue<styles::WordSpacing>::type& specifiedValue,
					const styles::Length::Context& lengthContext, styles::ComputedValue<styles::WordSpacing>::type& computedValue) {
				if(const styles::Length* const length = boost::get<styles::Length>(&specifiedValue)) {
					if(styles::Length::isValidUnit(length->unitType())) {
						computedValue = Pixels(length->value(lengthContext));
							return;
					}
				}
				// TODO: Handle <percentage> values.
				computedValue = Pixels::zero();
			}
#else
			inline void computeWordSpacing(
					const styles::SpecifiedValue<styles::WordSpacing>::type& specifiedValue,
					styles::ComputedValue<styles::WordSpacing>::type& computedValue) {
				if(const styles::Length* const length = boost::get<styles::Length>(&specifiedValue))
					computedValue = *length;
				else if(const styles::Percentage* const percentage = boost::get<styles::Percentage>(&specifiedValue))
					computedValue = *percentage;
				else
					computedValue = styles::Length(0);
			}
#endif
#if 0
			inline void computeLetterSpacing(const styles::SpecifiedValue<styles::LetterSpacing>::type& specifiedValue,
					const styles::Length::Context& lengthContext, styles::ComputedValue<styles::LetterSpacing>::type& computedValue) {
				if(specifiedValue != boost::none && styles::Length::isValidUnit(specifiedValue->unitType()))
					computedValue = Pixels(specifiedValue->value(lengthContext));
				else
					computedValue = Pixels::zero();
			}
#else
			inline void computeLetterSpacing(
					const styles::SpecifiedValue<styles::LetterSpacing>::type& specifiedValue,
					styles::ComputedValue<styles::LetterSpacing>::type& computedValue) {
				computedValue = boost::get_optional_value_or(specifiedValue, styles::Length(0));
			}
#endif
			inline void computeTextDecoration(const styles::SpecifiedValuesOfParts<TextRunStyleParts::TextDecoration>::type& specifiedValues,
					const graphics::Color& foregroundColor, styles::ComputedValuesOfParts<TextRunStyleParts::TextDecoration>::type& computedValues) {
				styles::computeAsSpecified<styles::TextDecorationLine>(specifiedValues, computedValues);
				boost::fusion::at_key<styles::TextDecorationColor>(computedValues)
					= boost::get_optional_value_or(boost::fusion::at_key<styles::TextDecorationColor>(specifiedValues), foregroundColor);
				styles::computeAsSpecified<styles::TextDecorationStyle>(specifiedValues, computedValues);
				styles::computeAsSpecified<styles::TextDecorationSkip>(specifiedValues, computedValues);
				styles::computeAsSpecified<styles::TextUnderlinePosition>(specifiedValues, computedValues);
			}

			inline void computeTextEmphasis(const styles::SpecifiedValuesOfParts<TextRunStyleParts::TextDecoration>::type& specifiedValues,
					const graphics::Color& foregroundColor, styles::ComputedValuesOfParts<TextRunStyleParts::TextDecoration>::type& computedValues) {
				const auto& specifiedStyle = boost::fusion::at_key<styles::TextEmphasisStyle>(specifiedValues);
				auto& computedStyle = boost::fusion::at_key<styles::TextEmphasisStyle>(computedValues);
				if(const styles::TextEmphasisStyleEnums* const keyword = boost::get<styles::TextEmphasisStyleEnums>(&specifiedStyle))
					computedStyle = boost::underlying_cast<CodePoint>(*keyword);
				else if(const CodePoint* const codePoint = boost::get<CodePoint>(&specifiedStyle))
					computedStyle = *codePoint;
				else
					computedStyle = boost::none;
				boost::fusion::at_key<styles::TextEmphasisColor>(computedValues)
					= boost::get_optional_value_or(boost::fusion::at_key<styles::TextEmphasisColor>(specifiedValues), foregroundColor);
				styles::computeAsSpecified<styles::TextEmphasisPosition>(specifiedValues, computedValues);
			}

			inline void computeTextShadow() {
				// TODO: Not implemented.
			}
		}

		boost::flyweight<styles::ComputedValue<TextRunStyle>::type> compute(
				const styles::SpecifiedValue<TextRunStyle>::type& specifiedValues,
				const styles::ComputedValue<TextRunStyle>::type& parentComputedValues) {
			styles::ComputedValue<TextRunStyle>::type computedValues;

			computeColor(
				boost::fusion::at_key<styles::Color>(specifiedValues.colors),
				boost::fusion::at_key<styles::Color>(parentComputedValues.colors),
				boost::fusion::at_key<styles::Color>(computedValues.colors));
			const auto& computedColor = boost::fusion::at_key<styles::Color>(computedValues.colors);

			computeBackground(specifiedValues, computedColor, computedValues);
			computeBorder(specifiedValues, computedColor, computedValues);

			computePaddingOrMargin<styles::PaddingSide>(
				boost::fusion::at_key<FlowRelativeFourSides<styles::PaddingSide>>(specifiedValues.basicBoxModel),
				boost::fusion::at_key<FlowRelativeFourSides<styles::PaddingSide>>(computedValues.basicBoxModel));
			computePaddingOrMargin<styles::MarginSide>(
				boost::fusion::at_key<FlowRelativeFourSides<styles::MarginSide>>(specifiedValues.basicBoxModel),
				boost::fusion::at_key<FlowRelativeFourSides<styles::MarginSide>>(computedValues.basicBoxModel));

			styles::computeAsSpecified<styles::FontFamily>(specifiedValues.fonts, computedValues.fonts);
#if 0
			computeFontSize(
				boost::fusion::at_key<styles::FontSize>(specifiedValues.fonts),
				context,
				boost::fusion::at_key<styles::FontSize>(parentComputedValues.fonts),
				Pixels(12),	// TODO: This is temporary.
				boost::fusion::at_key<styles::FontSize>(computedValues.fonts));
#else
			styles::computeAsSpecified<styles::FontSize>(specifiedValues.fonts, computedValues.fonts);
#endif
			const auto& computedFontSize = boost::fusion::at_key<styles::FontSize>(computedValues.fonts);
			styles::computeAsSpecified<styles::FontWeight>(specifiedValues.fonts, computedValues.fonts);
			styles::computeAsSpecified<styles::FontStretch>(specifiedValues.fonts, computedValues.fonts);
			styles::computeAsSpecified<styles::FontStyle>(specifiedValues.fonts, computedValues.fonts);
			styles::computeAsSpecified<styles::FontSizeAdjust>(specifiedValues.fonts, computedValues.fonts);
//			styles::computeAsSpecified<styles::FontFeatureSettings>(specifiedValues.fonts, computedValues.fonts);
//			styles::computeAsSpecified<styles::FontLanguageOverride>(specifiedValues.fonts, computedValues.fonts);

			styles::computeAsSpecified<styles::TextHeight>(specifiedValues.inlineLayout, computedValues.inlineLayout);
			styles::computeAsSpecified<styles::DominantBaseline>(specifiedValues.inlineLayout, computedValues.inlineLayout);
			styles::computeAsSpecified<styles::AlignmentBaseline>(specifiedValues.inlineLayout, computedValues.inlineLayout);
#if 0
			computeAlignmentAdjust(
				boost::fusion::at_key<styles::AlignmentAdjust>(specifiedValues.inlineLayout),
				context,
				boost::fusion::at_key<styles::AlignmentAdjust>(computedValues.inlineLayout));
#else
			styles::computeAsSpecified<styles::AlignmentAdjust>(specifiedValues.inlineLayout, computedValues.inlineLayout);
#endif

			styles::computeAsSpecified<styles::TextTransform>(specifiedValues.text, computedValues.text);
			styles::computeAsSpecified<styles::WhiteSpace>(specifiedValues.text, computedValues.text);
			styles::computeAsSpecified<styles::Hyphens>(specifiedValues.text, computedValues.text);
			computeWordSpacing(
				boost::fusion::at_key<styles::WordSpacing>(specifiedValues.text),
//				context,
				boost::fusion::at_key<styles::WordSpacing>(computedValues.text));
			computeLetterSpacing(
				boost::fusion::at_key<styles::LetterSpacing>(specifiedValues.text),
//				context,
				boost::fusion::at_key<styles::LetterSpacing>(computedValues.text));
			styles::computeAsSpecified<styles::HangingPunctuation>(specifiedValues.text, computedValues.text);

			computeTextDecoration(specifiedValues.textDecoration, computedColor, computedValues.textDecoration);
			computeTextEmphasis(specifiedValues.textDecoration, computedColor, computedValues.textDecoration);
			computeTextShadow();

			styles::computeAsSpecified<styles::Direction>(specifiedValues.writingModes, computedValues.writingModes);

			styles::computeAsSpecified<styles::ShapingEnabled>(specifiedValues.auxiliary, computedValues.auxiliary);
			styles::computeAsSpecified<styles::NumberSubstitution>(specifiedValues.auxiliary, computedValues.auxiliary);

			return boost::flyweight<styles::ComputedValue<TextRunStyle>::type>(computedValues);
		}

		std::size_t hash_value(const styles::ComputedValue<TextRunStyle>::type& style) {
			std::size_t seed = 0;

			boost::hash_combine(seed, boost::fusion::at_key<styles::Color>(style.colors));

			boost::hash_combine(seed, boost::fusion::at_key<styles::BackgroundColor>(style.backgroundsAndBorders));
			const auto& borderColors = boost::fusion::at_key<FlowRelativeFourSides<styles::BorderColor>>(style.backgroundsAndBorders);
			boost::hash_range(seed, std::begin(borderColors), std::end(borderColors));
			const auto& borderStyles = boost::fusion::at_key<FlowRelativeFourSides<styles::BorderStyle>>(style.backgroundsAndBorders);
			boost::hash_range(seed, std::begin(borderStyles), std::end(borderStyles));
			const auto& borderWidths = boost::fusion::at_key<FlowRelativeFourSides<styles::BorderWidth>>(style.backgroundsAndBorders);
			boost::hash_range(seed, std::begin(borderWidths), std::end(borderWidths));

			const auto& padding = boost::fusion::at_key<FlowRelativeFourSides<styles::PaddingSide>>(style.basicBoxModel);
			boost::hash_range(seed, std::begin(padding), std::end(padding));
			const auto& margin = boost::fusion::at_key<FlowRelativeFourSides<styles::MarginSide>>(style.basicBoxModel);
			boost::hash_range(seed, std::begin(margin), std::end(margin));

			boost::hash_combine(seed, boost::fusion::at_key<styles::FontFamily>(style.fonts));
			boost::hash_combine(seed, boost::fusion::at_key<styles::FontWeight>(style.fonts));
			boost::hash_combine(seed, boost::fusion::at_key<styles::FontStretch>(style.fonts));
			boost::hash_combine(seed, boost::fusion::at_key<styles::FontStyle>(style.fonts));
			boost::hash_combine(seed, boost::fusion::at_key<styles::FontSize>(style.fonts));
			const auto& fontSizeAdjust = boost::fusion::at_key<styles::FontSizeAdjust>(style.fonts);
			if(fontSizeAdjust != boost::none)
				boost::hash_combine(seed, boost::get(fontSizeAdjust));
			else
				boost::hash_combine(seed, std::make_tuple());
//			boost::hash_combine(seed, boost::fusion::at_key<styles::FontFeatureSettings>(style.fonts));
//			boost::hash_combine(seed, boost::fusion::at_key<styles::FontLanguageOverride>(style.fonts));

			boost::hash_combine(seed, boost::fusion::at_key<styles::TextHeight>(style.inlineLayout));
			boost::hash_combine(seed, boost::fusion::at_key<styles::DominantBaseline>(style.inlineLayout));
			boost::hash_combine(seed, boost::fusion::at_key<styles::AlignmentBaseline>(style.inlineLayout));
			boost::hash_combine(seed, boost::fusion::at_key<styles::AlignmentAdjust>(style.inlineLayout));

			boost::hash_combine(seed, boost::fusion::at_key<styles::TextTransform>(style.text));
			boost::hash_combine(seed, boost::fusion::at_key<styles::WhiteSpace>(style.text));
			boost::hash_combine(seed, boost::fusion::at_key<styles::Hyphens>(style.text));
			boost::hash_combine(seed, boost::fusion::at_key<styles::WordSpacing>(style.text));
			boost::hash_combine(seed, boost::fusion::at_key<styles::LetterSpacing>(style.text));
			boost::hash_combine(seed, boost::fusion::at_key<styles::HangingPunctuation>(style.text));

			boost::hash_combine(seed, boost::fusion::at_key<styles::TextDecorationLine>(style.textDecoration));
			boost::hash_combine(seed, boost::fusion::at_key<styles::TextDecorationColor>(style.textDecoration));
			boost::hash_combine(seed, boost::fusion::at_key<styles::TextDecorationStyle>(style.textDecoration));
			boost::hash_combine(seed, boost::fusion::at_key<styles::TextDecorationSkip>(style.textDecoration));
			boost::hash_combine(seed, boost::fusion::at_key<styles::TextUnderlinePosition>(style.textDecoration));
			if(const auto& textEmphasisStyle = boost::fusion::at_key<styles::TextEmphasisStyle>(style.textDecoration))
				boost::hash_combine(seed, boost::get(textEmphasisStyle));
			else
				boost::hash_combine(seed, std::make_tuple());
			boost::hash_combine(seed, boost::fusion::at_key<styles::TextEmphasisColor>(style.textDecoration));
			boost::hash_combine(seed, boost::fusion::at_key<styles::TextEmphasisPosition>(style.textDecoration));
//			boost::hash_combine(seed, boost::fusion::at_key<styles::TextShadow>(style.textDecoration));

			boost::hash_combine(seed, boost::fusion::at_key<styles::Direction>(style.writingModes));

			boost::hash_combine(seed, boost::fusion::at_key<styles::ShapingEnabled>(style.auxiliary));
			boost::hash_combine(seed, boost::fusion::at_key<styles::NumberSubstitution>(style.auxiliary));

			return seed;
		}
	}
}
