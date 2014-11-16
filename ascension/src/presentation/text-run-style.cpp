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
		namespace {
			void computeColor(const styles::SpecifiedValue<styles::Color>::type& specifiedValue,
					const graphics::Color& computedParentColor, styles::ComputedValue<styles::Color>::type& computedValue) {
				computedValue = boost::get_optional_value_or(specifiedValue, computedParentColor);
			}

			void computeBackground(const SpecifiedTextRunStyle& specifiedValues,
					const graphics::Color& foregroundColor, const styles::Length::Context& lengthContext, ComputedTextRunStyle& computedValues) {
				*boost::fusion::find<styles::ComputedValue<styles::BackgroundColor>::type>(computedValues) =
					boost::get_optional_value_or(*boost::fusion::find<styles::SpecifiedValue<styles::BackgroundColor>::type>(specifiedValues), foregroundColor);
			}

			void computeBorder(const SpecifiedTextRunStyle& specifiedValues,
					const graphics::Color& foregroundColor, const styles::Length::Context& lengthContext, ComputedTextRunStyle& computedValues) {
				const auto& specifiedColors = *boost::fusion::find<styles::SpecifiedValue<FlowRelativeFourSides<styles::BorderColor>>::type>(specifiedValues);
				const auto& specifiedStyles = *boost::fusion::find<styles::SpecifiedValue<FlowRelativeFourSides<styles::BorderStyle>>::type>(specifiedValues);
				const auto& specifiedWidths = *boost::fusion::find<styles::SpecifiedValue<FlowRelativeFourSides<styles::BorderWidth>>::type>(specifiedValues);
				auto& computedColors = *boost::fusion::find<styles::ComputedValue<FlowRelativeFourSides<styles::BorderColor>>::type>(computedValues);
				auto& computedStyles = *boost::fusion::find<styles::ComputedValue<FlowRelativeFourSides<styles::BorderStyle>>::type>(computedValues);
				auto& computedWidths = *boost::fusion::find<styles::ComputedValue<FlowRelativeFourSides<styles::BorderWidth>>::type>(computedValues);
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

			inline void computeBaselineShift(const styles::SpecifiedValue<styles::BaselineShift>::type& specifiedValue,
					const styles::Length::Context& lengthContext, styles::ComputedValue<styles::BaselineShift>::type& computedValue) {
				// TODO: [CSS3INLINE] does not describe the computation for other than <percentage>.
				computedValue = Pixels::zero();
			}

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

			inline void computeLetterSpacing(const styles::SpecifiedValue<styles::LetterSpacing>::type& specifiedValue,
					const styles::Length::Context& lengthContext, styles::ComputedValue<styles::LetterSpacing>::type& computedValue) {
				if(specifiedValue != boost::none && styles::Length::isValidUnit(specifiedValue->unitType()))
					computedValue = Pixels(specifiedValue->value(lengthContext));
				else
					computedValue = Pixels::zero();
			}

			void computeTextDecoration(const SpecifiedTextRunStyle& specifiedValues,
					const graphics::Color& foregroundColor, ComputedTextRunStyle& computedValues) {
				*boost::fusion::find<styles::ComputedValue<styles::TextDecorationLine>::type>(computedValues)
					= *boost::fusion::find<styles::SpecifiedValue<styles::TextDecorationLine>::type>(specifiedValues);
				*boost::fusion::find<styles::ComputedValue<styles::TextDecorationColor>::type>(computedValues)
					= boost::get_optional_value_or(*boost::fusion::find<styles::SpecifiedValue<styles::TextDecorationColor>::type>(specifiedValues), foregroundColor);
				*boost::fusion::find<styles::ComputedValue<styles::TextDecorationStyle>::type>(computedValues)
					= *boost::fusion::find<styles::SpecifiedValue<styles::TextDecorationStyle>::type>(specifiedValues);
				*boost::fusion::find<styles::ComputedValue<styles::TextDecorationSkip>::type>(computedValues)
					= *boost::fusion::find<styles::SpecifiedValue<styles::TextDecorationSkip>::type>(specifiedValues);
				*boost::fusion::find<styles::ComputedValue<styles::TextUnderlinePosition>::type>(computedValues)
					= *boost::fusion::find<styles::SpecifiedValue<styles::TextUnderlinePosition>::type>(specifiedValues);
			}

			void computeTextEmphasis(const SpecifiedTextRunStyle& specifiedValues,
					const graphics::Color& foregroundColor, ComputedTextRunStyle& computedValues) {
				if(const styles::TextEmphasisStyleEnums* keyword = boost::get<styles::TextEmphasisStyleEnums>(
						&*boost::fusion::find<styles::SpecifiedValue<styles::TextEmphasisStyle>::type>(specifiedValues)))
					*boost::fusion::find<styles::ComputedValue<styles::TextEmphasisStyle>::type>(computedValues) = static_cast<CodePoint>(boost::native_value(*keyword));
				else if(const CodePoint* codePoint = boost::get<CodePoint>(
						&*boost::fusion::find<styles::SpecifiedValue<styles::TextEmphasisStyle>::type>(specifiedValues)))
					*boost::fusion::find<styles::ComputedValue<styles::TextEmphasisStyle>::type>(computedValues) = *codePoint;
				else
					*boost::fusion::find<styles::ComputedValue<styles::TextEmphasisStyle>::type>(computedValues) = boost::none;
				*boost::fusion::find<styles::ComputedValue<styles::TextEmphasisColor>::type>(computedValues)
					= boost::get_optional_value_or(*boost::fusion::find<styles::SpecifiedValue<styles::TextEmphasisColor>::type>(specifiedValues), foregroundColor);
				*boost::fusion::find<styles::ComputedValue<styles::TextEmphasisPosition>::type>(computedValues)
					= *boost::fusion::find<styles::SpecifiedValue<styles::TextEmphasisPosition>::type>(specifiedValues);
			}

			inline void computeTextShadow() {
				// TODO: Not implemented.
			}
		}

		/**
		 * Computes @c TextRunStyle.
		 * @param specifiedValues The "Specified Value"s to compute
		 * @param context The length context
		 * @param parentComputedValues The "Computed Value"s of the parent element
		 * @param[out] computedValues The "Computed Value"s
		 */
		boost::flyweight<ComputedTextRunStyle> compute(const SpecifiedTextRunStyle& specifiedValues,
				const styles::Length::Context& context, const ComputedTextRunStyle& parentComputedValues) {
			ComputedTextRunStyle computedValues;

			computeColor(
				*boost::fusion::find<styles::SpecifiedValue<styles::Color>::type>(specifiedValues),
				*boost::fusion::find<styles::ComputedValue<styles::Color>::type>(parentComputedValues),
				*boost::fusion::find<styles::ComputedValue<styles::Color>::type>(computedValues));
			const auto& computedColor = *boost::fusion::find<styles::ComputedValue<styles::Color>::type>(computedValues);

			computeBackground(specifiedValues, computedColor, context, computedValues);
			computeBorder(specifiedValues, computedColor, context, computedValues);

			computePaddingOrMargin<styles::PaddingSide>(
				*boost::fusion::find<styles::SpecifiedValue<FlowRelativeFourSides<styles::PaddingSide>>::type>(specifiedValues),
				context,
				*boost::fusion::find<styles::ComputedValue<FlowRelativeFourSides<styles::PaddingSide>>::type>(computedValues));
			computePaddingOrMargin<styles::MarginSide>(
				*boost::fusion::find<styles::SpecifiedValue<FlowRelativeFourSides<styles::MarginSide>>::type>(specifiedValues),
				context,
				*boost::fusion::find<styles::ComputedValue<FlowRelativeFourSides<styles::MarginSide>>::type>(computedValues));

			styles::computeAsSpecified<styles::FontFamily>(specifiedValues, computedValues);
			computeFontSize(
				*boost::fusion::find<styles::SpecifiedValue<styles::FontSize>::type>(specifiedValues),
				context,
				*boost::fusion::find<styles::ComputedValue<styles::FontSize>::type>(parentComputedValues),
				Pixels(12),	// TODO: This is temporary.
				*boost::fusion::find<styles::ComputedValue<styles::FontSize>::type>(computedValues));
			const auto& computedFontSize = *boost::fusion::find<styles::ComputedValue<styles::FontSize>::type>(computedValues);
			styles::computeAsSpecified<styles::FontWeight>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::FontStretch>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::FontStyle>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::FontSizeAdjust>(specifiedValues, computedValues);
//			styles::computeAsSpecified<styles::FontFeatureSettings>(specifiedValues, computedValues);
//			styles::computeAsSpecified<styles::FontLanguageOverride>(specifiedValues, computedValues);

			styles::computeAsSpecified<styles::TextHeight>(specifiedValues, computedValues);
			styles::detail::computeLineHeight(
				*boost::fusion::find<styles::SpecifiedValue<styles::LineHeight>::type>(specifiedValues),
				computedFontSize,
				*boost::fusion::find<styles::ComputedValue<styles::LineHeight>::type>(computedValues));
			styles::computeAsSpecified<styles::DominantBaseline>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::AlignmentBaseline>(specifiedValues, computedValues);
			computeAlignmentAdjust(
				*boost::fusion::find<styles::SpecifiedValue<styles::AlignmentAdjust>::type>(specifiedValues),
				context,
				*boost::fusion::find<styles::ComputedValue<styles::AlignmentAdjust>::type>(computedValues));
			computeBaselineShift(
				*boost::fusion::find<styles::SpecifiedValue<styles::BaselineShift>::type>(specifiedValues),
				context,
				*boost::fusion::find<styles::ComputedValue<styles::BaselineShift>::type>(computedValues));

			styles::computeAsSpecified<styles::TextTransform>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::Hyphens>(specifiedValues, computedValues);
			computeWordSpacing(
				*boost::fusion::find<styles::SpecifiedValue<styles::WordSpacing>::type>(specifiedValues),
				context,
				*boost::fusion::find<styles::ComputedValue<styles::WordSpacing>::type>(computedValues));
			computeLetterSpacing(
				*boost::fusion::find<styles::SpecifiedValue<styles::LetterSpacing>::type>(specifiedValues),
				context,
				*boost::fusion::find<styles::ComputedValue<styles::LetterSpacing>::type>(computedValues));

			computeTextDecoration(specifiedValues, computedColor, computedValues);
			computeTextEmphasis(specifiedValues, computedColor, computedValues);
			computeTextShadow();

			styles::computeAsSpecified<styles::ShapingEnabled>(specifiedValues, computedValues);

			return boost::flyweight<ComputedTextRunStyle>(computedValues);
		}

		/// Extend @c boost#hash_value for @c ComputedTextRunStyle.
		std::size_t hash_value(const ComputedTextRunStyle& style) {
			std::size_t seed = 0;

			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::Color>::type>(style));

			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::BackgroundColor>::type>(style));
			const auto& borderColors = *boost::fusion::find<styles::ComputedValue<FlowRelativeFourSides<styles::BorderColor>>::type>(style);
			boost::hash_range(seed, std::begin(borderColors), std::end(borderColors));
			const auto& borderStyles = *boost::fusion::find<styles::ComputedValue<FlowRelativeFourSides<styles::BorderStyle>>::type>(style);
			boost::hash_range(seed, std::begin(borderStyles), std::end(borderStyles));
			const auto& borderWidths = *boost::fusion::find<styles::ComputedValue<FlowRelativeFourSides<styles::BorderWidth>>::type>(style);
			boost::hash_range(seed, std::begin(borderWidths), std::end(borderWidths));

			const auto& padding = *boost::fusion::find<styles::ComputedValue<FlowRelativeFourSides<styles::PaddingSide>>::type>(style);
			boost::hash_range(seed, std::begin(padding), std::end(padding));
			const auto& margin = *boost::fusion::find<styles::ComputedValue<FlowRelativeFourSides<styles::MarginSide>>::type>(style);
			boost::hash_range(seed, std::begin(margin), std::end(margin));

			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::FontFamily>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::FontWeight>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::FontStretch>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::FontStyle>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::FontSize>::type>(style));
			const auto& fontSizeAdjust = *boost::fusion::find<styles::ComputedValue<styles::FontSizeAdjust>::type>(style);
			if(fontSizeAdjust != boost::none)
				boost::hash_combine(seed, boost::get(fontSizeAdjust));
			else
				boost::hash_combine(seed, std::make_tuple());
//			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::FontFeatureSettings>::type>(style));
//			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::FontLanguageOverride>::type>(style));

			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::TextHeight>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::LineHeight>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::DominantBaseline>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::AlignmentBaseline>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::AlignmentAdjust>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::BaselineShift>::type>(style));

			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::TextTransform>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::Hyphens>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::WordSpacing>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::LetterSpacing>::type>(style));

			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::TextDecorationLine>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::TextDecorationColor>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::TextDecorationStyle>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::TextDecorationSkip>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::TextUnderlinePosition>::type>(style));
			if(const auto& textEmphasisStyle = *boost::fusion::find<styles::ComputedValue<styles::TextEmphasisStyle>::type>(style))
				boost::hash_combine(seed, boost::get(textEmphasisStyle));
			else
				boost::hash_combine(seed, std::make_tuple());
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::TextEmphasisColor>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::TextEmphasisPosition>::type>(style));
//			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::TextShadow>::type>(style));

			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::ShapingEnabled>::type>(style));
//			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::DeprecatedFormatCharactersDisabled>::type>(style));
//			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValue<styles::SymmetricSwappingInhibited>::type>(style));

			return seed;
		}

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
	}
}
