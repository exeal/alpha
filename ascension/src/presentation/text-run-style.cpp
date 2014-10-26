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
			void computeColor(const styles::SpecifiedValueType<styles::Color<styles::Inherited<true>>>::type& specifiedValue,
					const graphics::Color& computedParentColor,
					styles::ComputedValueType<styles::Color<styles::Inherited<true>>>::type& computedValue) {
				computedValue = boost::get_optional_value_or(specifiedValue, computedParentColor);
			}

			void computeBackground(const styles::SpecifiedValueType<styles::Background>::type& specifiedValue,
					const graphics::Color& foregroundColor, const styles::Length::Context& lengthContext,
					styles::ComputedValueType<styles::Background>::type& computedValue) {
				computedValue.color = boost::get_optional_value_or(specifiedValue.color, foregroundColor);
			}

			void computeBorder(const styles::SpecifiedValueType<styles::Border>::type& specifiedValue,
					const graphics::Color& foregroundColor, const styles::Length::Context& lengthContext,
					styles::ComputedValueType<styles::Border>::type& computedValue) {
				for(std::size_t i = 0; specifiedValue.sides.size(); ++i) {
					computedValue.sides[i].color = boost::get_optional_value_or(specifiedValue.sides[i].color, foregroundColor);
					computedValue.sides[i].style = specifiedValue.sides[i].style;
					const bool absent = computedValue.sides[i].style == styles::Border::NONE || computedValue.sides[i].style == styles::Border::HIDDEN;
					computedValue.sides[i].width = Pixels(!absent ? specifiedValue.sides[i].width.value(lengthContext) : 0);
				}
			}

			template<typename Property>
			inline void computePaddingOrMargin(const typename styles::SpecifiedValueType<FlowRelativeFourSides<Property>>::type& specifiedValue,
					const styles::Length::Context& lengthContext, typename styles::ComputedValueType<FlowRelativeFourSides<Property>>::type& computedValue,
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

			inline void computeFontSize(const styles::SpecifiedValueType<styles::FontSize>::type& specifiedValue,
					const styles::Length::Context& lengthContext, const Pixels& computedParentFontSize, const Pixels& mediumFontSize,
					styles::ComputedValueType<styles::FontSize>::type& computedValue) {
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

			inline void computeAlignmentAdjust(const styles::SpecifiedValueType<styles::AlignmentAdjust>::type& specifiedValue,
					const styles::Length::Context& lengthContext, styles::ComputedValueType<styles::AlignmentAdjust>::type& computedValue) {
				// TODO: [CSS3INLINE] does not describe the computation for other than <percentage>.
				computedValue = Pixels::zero();
			}

			inline void computeBaselineShift(const styles::SpecifiedValueType<styles::BaselineShift>::type& specifiedValue,
					const styles::Length::Context& lengthContext, styles::ComputedValueType<styles::BaselineShift>::type& computedValue) {
				// TODO: [CSS3INLINE] does not describe the computation for other than <percentage>.
				computedValue = Pixels::zero();
			}

			inline void computeWordSpacing(const styles::SpecifiedValueType<styles::WordSpacing>::type& specifiedValue,
					const styles::Length::Context& lengthContext, styles::ComputedValueType<styles::WordSpacing>::type& computedValue) {
				if(const styles::Length* const length = boost::get<styles::Length>(&specifiedValue)) {
					if(styles::Length::isValidUnit(length->unitType())) {
						computedValue = Pixels(length->value(lengthContext));
						return;
					}
				}
				// TODO: Handle <percentage> values.
				computedValue = Pixels::zero();
			}

			inline void computeLetterSpacing(const styles::SpecifiedValueType<styles::LetterSpacing>::type& specifiedValue,
					const styles::Length::Context& lengthContext, styles::ComputedValueType<styles::LetterSpacing>::type& computedValue) {
				if(specifiedValue != boost::none && styles::Length::isValidUnit(specifiedValue->unitType()))
					computedValue = Pixels(specifiedValue->value(lengthContext));
				else
					computedValue = Pixels::zero();
			}

			void computeTextDecoration(const styles::SpecifiedValueType<styles::TextDecoration>::type& specifiedValue,
					const graphics::Color& foregroundColor, styles::ComputedValueType<styles::TextDecoration>::type& computedValue) {
				computedValue.lines = specifiedValue.lines;
				computedValue.color = boost::get_optional_value_or(specifiedValue.color, foregroundColor);
				computedValue.style = specifiedValue.style;
				computedValue.skip = specifiedValue.skip;
				computedValue.underlinePosition = specifiedValue.underlinePosition;
			}

			void computeTextEmphasis(const styles::SpecifiedValueType<styles::TextEmphasis>::type& specifiedValue,
					const graphics::Color& foregroundColor, styles::ComputedValueType<styles::TextEmphasis>::type& computedValue) {
				if(const styles::TextEmphasis::StyleEnums* keyword = boost::get<styles::TextEmphasis::StyleEnums>(&specifiedValue.style))
					computedValue.style = static_cast<CodePoint>(*keyword);
				else if(const CodePoint* codePoint = boost::get<CodePoint>(&specifiedValue.style))
					computedValue.style = *codePoint;
				else
					computedValue.style = boost::none;
				computedValue.color = boost::get_optional_value_or(specifiedValue.color, foregroundColor);
				computedValue.position = specifiedValue.position;
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
				*boost::fusion::find<styles::SpecifiedValueType<styles::Color<styles::Inherited<true>>>::type>(specifiedValues),
				*boost::fusion::find<styles::ComputedValueType<styles::Color<styles::Inherited<true>>>::type>(parentComputedValues),
				*boost::fusion::find<styles::ComputedValueType<styles::Color<styles::Inherited<true>>>::type>(computedValues));
			const auto& computedColor = *boost::fusion::find<styles::ComputedValueType<styles::Color<styles::Inherited<true>>>::type>(computedValues);

			computeBackground(
				*boost::fusion::find<styles::SpecifiedValueType<styles::Background>::type>(specifiedValues),
				computedColor, context,
				*boost::fusion::find<styles::ComputedValueType<styles::Background>::type>(computedValues));
			computeBorder(
				*boost::fusion::find<styles::SpecifiedValueType<styles::Border>::type>(specifiedValues),
				computedColor, context,
				*boost::fusion::find<styles::ComputedValueType<styles::Border>::type>(computedValues));

			computePaddingOrMargin<styles::PaddingSide>(
				*boost::fusion::find<styles::SpecifiedValueType<FlowRelativeFourSides<styles::PaddingSide>>::type>(specifiedValues),
				context,
				*boost::fusion::find<styles::ComputedValueType<FlowRelativeFourSides<styles::PaddingSide>>::type>(computedValues));
			computePaddingOrMargin<styles::MarginSide>(
				*boost::fusion::find<styles::SpecifiedValueType<FlowRelativeFourSides<styles::MarginSide>>::type>(specifiedValues),
				context,
				*boost::fusion::find<styles::ComputedValueType<FlowRelativeFourSides<styles::MarginSide>>::type>(computedValues));

			styles::computeAsSpecified<styles::FontFamily>(specifiedValues, computedValues);
			computeFontSize(
				*boost::fusion::find<styles::SpecifiedValueType<styles::FontSize>::type>(specifiedValues),
				context,
				*boost::fusion::find<styles::ComputedValueType<styles::FontSize>::type>(parentComputedValues),
				Pixels(12),	// TODO: This is temporary.
				*boost::fusion::find<styles::ComputedValueType<styles::FontSize>::type>(computedValues));
			const auto& computedFontSize = *boost::fusion::find<styles::ComputedValueType<styles::FontSize>::type>(computedValues);
			styles::computeAsSpecified<styles::FontWeight>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::FontStretch>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::FontStyle>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::FontSizeAdjust>(specifiedValues, computedValues);
//			styles::computeAsSpecified<styles::FontFeatureSettings>(specifiedValues, computedValues);
//			styles::computeAsSpecified<styles::FontLanguageOverride>(specifiedValues, computedValues);

			styles::computeAsSpecified<styles::TextHeight>(specifiedValues, computedValues);
			styles::detail::computeLineHeight(
				*boost::fusion::find<styles::SpecifiedValueType<styles::LineHeight>::type>(specifiedValues),
				computedFontSize,
				*boost::fusion::find<styles::ComputedValueType<styles::LineHeight>::type>(computedValues));
			styles::computeAsSpecified<styles::DominantBaseline>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::AlignmentBaseline>(specifiedValues, computedValues);
			computeAlignmentAdjust(
				*boost::fusion::find<styles::SpecifiedValueType<styles::AlignmentAdjust>::type>(specifiedValues),
				context,
				*boost::fusion::find<styles::ComputedValueType<styles::AlignmentAdjust>::type>(computedValues));
			computeBaselineShift(
				*boost::fusion::find<styles::SpecifiedValueType<styles::BaselineShift>::type>(specifiedValues),
				context,
				*boost::fusion::find<styles::ComputedValueType<styles::BaselineShift>::type>(computedValues));

			styles::computeAsSpecified<styles::TextTransform>(specifiedValues, computedValues);
			styles::computeAsSpecified<styles::Hyphens>(specifiedValues, computedValues);
			computeWordSpacing(
				*boost::fusion::find<styles::SpecifiedValueType<styles::WordSpacing>::type>(specifiedValues),
				context,
				*boost::fusion::find<styles::ComputedValueType<styles::WordSpacing>::type>(computedValues));
			computeLetterSpacing(
				*boost::fusion::find<styles::SpecifiedValueType<styles::LetterSpacing>::type>(specifiedValues),
				context,
				*boost::fusion::find<styles::ComputedValueType<styles::LetterSpacing>::type>(computedValues));

			computeTextDecoration(
				*boost::fusion::find<styles::SpecifiedValueType<styles::TextDecoration>::type>(specifiedValues),
				computedColor,
				*boost::fusion::find<styles::ComputedValueType<styles::TextDecoration>::type>(computedValues));
			computeTextEmphasis(
				*boost::fusion::find<styles::SpecifiedValueType<styles::TextEmphasis>::type>(specifiedValues),
				computedColor,
				*boost::fusion::find<styles::ComputedValueType<styles::TextEmphasis>::type>(computedValues));
			computeTextShadow();

			styles::computeAsSpecified<styles::ShapingEnabled>(specifiedValues, computedValues);

			return boost::flyweight<ComputedTextRunStyle>(computedValues);
		}

		/// Extend @c boost#hash_value for @c ComputedTextRunStyle.
		std::size_t hash_value(const ComputedTextRunStyle& style) {
			std::size_t seed = 0;

			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::Color<styles::Inherited<true>>>::type>(style));

			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::Background>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::Border>::type>(style));

			const auto& padding = *boost::fusion::find<styles::ComputedValueType<FlowRelativeFourSides<styles::PaddingSide>>::type>(style);
			boost::hash_range(seed, std::begin(padding), std::end(padding));
			const auto& margin = *boost::fusion::find<styles::ComputedValueType<FlowRelativeFourSides<styles::MarginSide>>::type>(style);
			boost::hash_range(seed, std::begin(margin), std::end(margin));

			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::FontFamily>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::FontWeight>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::FontStretch>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::FontStyle>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::FontSize>::type>(style));
			const auto& fontSizeAdjust = *boost::fusion::find<styles::ComputedValueType<styles::FontSizeAdjust>::type>(style);
			if(fontSizeAdjust != boost::none)
				boost::hash_combine(seed, boost::get(fontSizeAdjust));
			else
				boost::hash_combine(seed, std::make_tuple());
//			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::FontFeatureSettings>::type>(style));
//			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::FontLanguageOverride>::type>(style));

			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::TextHeight>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::LineHeight>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::DominantBaseline>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::AlignmentBaseline>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::AlignmentAdjust>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::BaselineShift>::type>(style));

			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::TextTransform>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::Hyphens>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::WordSpacing>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::LetterSpacing>::type>(style));

			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::TextDecoration>::type>(style));
			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::TextEmphasis>::type>(style));
//			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::TextShadow>::type>(style));

			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::ShapingEnabled>::type>(style));
//			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::DeprecatedFormatCharactersDisabled>::type>(style));
//			boost::hash_combine(seed, *boost::fusion::find<styles::ComputedValueType<styles::SymmetricSwappingInhibited>::type>(style));

			return seed;
		}
	}
}
