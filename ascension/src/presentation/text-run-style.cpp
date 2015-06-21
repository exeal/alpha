/**
 * @file text-run-style.cpp
 * @author exeal
 * @date 2014-10-06 Created.
 */

#include <ascension/presentation/text-run-style.hpp>
#include <boost/flyweight.hpp>

namespace boost {
	template<typename T>
	inline void hash_combine(std::size_t& seed, const boost::optional<T>& value) {
		if(value != boost::none)
			hash_combine(seed, get(value));
	}
}

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
				for(std::size_t i = 0; i < specifiedColors.size(); ++i) {
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
					else if(const styles::PaddingEnums* const paddingEnums = boost::relaxed_get<styles::PaddingEnums>(&specifiedValue[i]))
						computedValue[i] = std::make_tuple();
					else if(const styles::MarginEnums* const marginEnums = boost::relaxed_get<styles::MarginEnums>(&specifiedValue[i]))
						computedValue[i] = std::make_tuple();	// TODO: Handle 'fill' keyword.
					else
						computedValue[i] = Property::initialValue();
				}
			}
#if 0
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

			void compute(
					const styles::SpecifiedValue<TextRunStyle>::type& specifiedValues,
					const styles::ComputedValue<styles::Color>::type& parentComputedColor,
					styles::ComputedValue<TextRunStyle>::type& computedValues) {
				computeColor(
					boost::fusion::at_key<styles::Color>(specifiedValues.colors),
					parentComputedColor,
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
//				styles::computeAsSpecified<styles::FontFeatureSettings>(specifiedValues.fonts, computedValues.fonts);
//				styles::computeAsSpecified<styles::FontLanguageOverride>(specifiedValues.fonts, computedValues.fonts);

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
//					context,
					boost::fusion::at_key<styles::WordSpacing>(computedValues.text));
				computeLetterSpacing(
					boost::fusion::at_key<styles::LetterSpacing>(specifiedValues.text),
//					context,
					boost::fusion::at_key<styles::LetterSpacing>(computedValues.text));
				styles::computeAsSpecified<styles::HangingPunctuation>(specifiedValues.text, computedValues.text);

				computeTextDecoration(specifiedValues.textDecoration, computedColor, computedValues.textDecoration);
				computeTextEmphasis(specifiedValues.textDecoration, computedColor, computedValues.textDecoration);
				computeTextShadow();

				styles::computeAsSpecified<styles::Direction>(specifiedValues.writingModes, computedValues.writingModes);

				styles::computeAsSpecified<styles::ShapingEnabled>(specifiedValues.auxiliary, computedValues.auxiliary);
				styles::computeAsSpecified<styles::NumberSubstitution>(specifiedValues.auxiliary, computedValues.auxiliary);
			}
		}
#if 0
		const SpecifiedTextRunStyle ComputedTextRunStyle::specifiedInstance_;
#endif
		/// Default constructor initializes the all members with their default constructors.
		ComputedTextRunStyle::ComputedTextRunStyle() {
		}

		/**
		 * Computes and creates a @c ComputedTextRunStyle.
		 * @param parameters The first element is the "Specified Value"s of @c TextRunStyle properties. The second
		 *                   element is the inherited 'color' value used to handle 'currentColor' of the 'color'
		 *                   property
		 */
		ComputedTextRunStyle::ComputedTextRunStyle(const ConstructionParameters& parameters) {
			if(std::get<0>(parameters) == nullptr || std::get<1>(parameters) == nullptr)
				throw NullPointerException("parameters");
			compute(*std::get<0>(parameters), *std::get<1>(parameters), *this);
		}

		/**
		 * Computes and creates a @c ComputedTextRunStyle as a root element.
		 * @param parameters The first element is the "Specified Value"s of @c TextRunStyle properties. The second
		 *                   element should be @c styles#HANDLE_AS_ROOT
		 */
		ComputedTextRunStyle::ComputedTextRunStyle(const ConstructionParametersAsRoot& parameters) {
			if(std::get<0>(parameters) == nullptr)
				throw NullPointerException("parameters");
			compute(*std::get<0>(parameters), boost::get(styles::Color::initialValue()), *this);
		}
#if 0		
		ComputedTextRunStyle::ConstructionParametersAsRoot::ConstructionParametersAsRoot()
				: std::tuple<const SpecifiedTextRunStyle*, styles::HandleAsRoot>(&specifiedInstance_, styles::HANDLE_AS_ROOT) {
		}
		
		ComputedTextRunStyle::ConstructionParametersAsRoot::ConstructionParametersAsRoot(const SpecifiedTextRunStyle* specifiedValue, styles::HandleAsRoot)
				: std::tuple<const SpecifiedTextRunStyle*, styles::HandleAsRoot>(specifiedValue, styles::HANDLE_AS_ROOT) {
		}
#endif
		namespace {
			inline void combineHashedTextEmphasisStyle(std::size_t& seed, const styles::ComputedValue<styles::TextEmphasisStyle>::type& style) {
				if(style != boost::none)
					boost::hash_combine(seed, boost::get(style));
			}

			inline void combineHashedTextEmphasisStyle(std::size_t& seed, const styles::SpecifiedValue<styles::TextEmphasisStyle>::type& style) {
				if(const styles::TextEmphasisStyleEnums* const enums = boost::get<styles::TextEmphasisStyleEnums>(&style))
					boost::hash_combine(seed, *enums);
				else if(const CodePoint* const codePoint = boost::get<CodePoint>(&style))
					boost::hash_combine(seed, *codePoint);
				else
					boost::hash_combine(seed, std::make_tuple());
			}

			template<template<typename> class Metafunction>
			inline std::size_t hashTextRunStyle(const typename Metafunction<TextRunStyle>::type& style) {
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
//				boost::hash_combine(seed, boost::fusion::at_key<styles::FontFeatureSettings>(style.fonts));
//				boost::hash_combine(seed, boost::fusion::at_key<styles::FontLanguageOverride>(style.fonts));

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
				combineHashedTextEmphasisStyle(seed, boost::fusion::at_key<styles::TextEmphasisStyle>(style.textDecoration));
				boost::hash_combine(seed, boost::fusion::at_key<styles::TextEmphasisColor>(style.textDecoration));
				boost::hash_combine(seed, boost::fusion::at_key<styles::TextEmphasisPosition>(style.textDecoration));
//				boost::hash_combine(seed, boost::fusion::at_key<styles::TextShadow>(style.textDecoration));

				boost::hash_combine(seed, boost::fusion::at_key<styles::Direction>(style.writingModes));

				boost::hash_combine(seed, boost::fusion::at_key<styles::ShapingEnabled>(style.auxiliary));
				boost::hash_combine(seed, boost::fusion::at_key<styles::NumberSubstitution>(style.auxiliary));

				return seed;
			}
		}

		/// @c boost#hash_value for @c ComputedTextRunStyle.
		std::size_t hash_value(const styles::ComputedValue<TextRunStyle>::type& style) {
			return hashTextRunStyle<styles::ComputedValue>(style);
		}

		/// @c boost#hash_value for @c SpecifiedTextRunStyle.
		std::size_t hash_value(const styles::SpecifiedValue<TextRunStyle>::type& style) {
			return hashTextRunStyle<styles::SpecifiedValue>(style);
		}

		namespace styles {
			/// Private default constructor.
			GlobalFontSettings::GlobalFontSettings() : size_(12), minimumSize_(9) {
				// TODO: Initial values are adhoc.
			}

			/// Returns the singleton instance.
			GlobalFontSettings& GlobalFontSettings::instance() BOOST_NOEXCEPT {
				static GlobalFontSettings singleton;
				return singleton;
			}

			inline Pixels _useFontSize(const ComputedValue<FontSize>::type& computedValue,
					const Length::Context& context, boost::optional<Pixels> computedParentFontSize, boost::optional<Pixels> mediumFontSize) {
				const Pixels medium(boost::get_optional_value_or(mediumFontSize, GlobalFontSettings::instance().size()));
				if(const AbsoluteFontSize* const absoluteFontSize = boost::get<AbsoluteFontSize>(&computedValue)) {
					// TODO: AbsoluteFontSize should be double constant, not enum?
					static const std::array<Number, AbsoluteFontSize::XX_LARGE - AbsoluteFontSize::XX_SMALL + 1>
						ABSOLUTE_SIZE_RATIOS = {3.f / 5.f, 3.f / 4.f, 8.f / 9.f, 1.f, 6.f / 5.f, 3.f / 2.f, 2.f / 1.f};
					static_assert(AbsoluteFontSize::XX_SMALL == 0, "");
					if(*absoluteFontSize >= AbsoluteFontSize::XX_SMALL && *absoluteFontSize <= AbsoluteFontSize::XX_LARGE)
						return medium * ABSOLUTE_SIZE_RATIOS[boost::underlying_cast<std::size_t>(*absoluteFontSize)];
				} else if(const RelativeFontSize* const relativeFontSize = boost::get<RelativeFontSize>(&computedValue)) {
					static const Number RELATIVE_FACTOR = 1.2f;	// TODO: Is this right ?
					switch(boost::native_value(*relativeFontSize)) {
						case RelativeFontSize::LARGER:
							return boost::get_optional_value_or(computedParentFontSize, medium) * RELATIVE_FACTOR;
						case RelativeFontSize::SMALLER:
							return boost::get_optional_value_or(computedParentFontSize, medium) / RELATIVE_FACTOR;
					}
				} else if(const Length* const length = boost::get<Length>(&computedValue)) {
					if(length->valueInSpecifiedUnits() >= 0.0)
						return Pixels(length->value(context));
				} else if(const Percentage* const percentage = boost::get<Percentage>(&computedValue)) {
					if(*percentage >= 0)	// [CSS3-FONTS] does not disallow negative value, but...
						return boost::get_optional_value_or(computedParentFontSize, medium) * boost::rational_cast<Number>(*percentage);
				}
				return medium;
			}

			/**
			 * Converts the "Computed Value" of @c FontSize into "Used Value", as root element.
			 * @param computedValue The "Computed Value"
			 * @param mediumFontSize The pixel size for 'medium' value. If this is @c boost#none, @c GlobalFontSettings
			 *                       is used
			 * @return The "Used Value" in pixels
			 */
			Pixels useFontSize(const ComputedValue<FontSize>::type& computedValue,
					const Length::Context& context, HandleAsRoot, boost::optional<Pixels> mediumFontSize /* = boost::none */) {
				return _useFontSize(computedValue, context, boost::none, mediumFontSize);
			}

			/**
			 * Converts the "Computed Value" of @c FontSize into "Used Value".
			 * @param computedValue The "Computed Value"
			 * @param computedParentFontSize The "Computed Value" of the parent element
			 * @param mediumFontSize The pixel size for 'medium' value. If this is @c boost#none, @c GlobalFontSettings
			 *                       is used
			 * @return The "Used Value" in pixels
			 */
			Pixels useFontSize(const ComputedValue<FontSize>::type& computedValue,
					const Length::Context& context, const Pixels& computedParentFontSize, boost::optional<Pixels> mediumFontSize /* = boost::none */) {
				return _useFontSize(computedValue, context, boost::make_optional(computedParentFontSize), mediumFontSize);
			}
		}
	}
}
