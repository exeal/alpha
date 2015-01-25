/**
 * @file text-run-style.hpp
 * @author exeal
 * @see text-line-style.hpp, text-toplevel-style.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2012-07-16 reunioned with text-line-style.hpp
 * @date 2014-09-27 Separated from text-style.hpp
 */

#ifndef ASCENSION_TEXT_RUN_STYLE_HPP
#define ASCENSION_TEXT_RUN_STYLE_HPP
#include <ascension/presentation/flow-relative-directions-dimensions.hpp>
#include <ascension/presentation/detail/style-sequence.hpp>
#include <ascension/presentation/styles/auxiliary.hpp>
#include <ascension/presentation/styles/background.hpp>
#include <ascension/presentation/styles/box.hpp>
#include <ascension/presentation/styles/color.hpp>
#include <ascension/presentation/styles/fonts.hpp>
#include <ascension/presentation/styles/inline.hpp>
#include <ascension/presentation/styles/text.hpp>
#include <ascension/presentation/styles/text-decor.hpp>
#include <ascension/presentation/styles/writing-modes.hpp>
#include <boost/flyweight/flyweight_fwd.hpp>
#include <boost/fusion/container/vector.hpp>

namespace ascension {
	namespace presentation {
		struct TextRunStyleParts {
			/// "Colors" part.
			typedef boost::fusion::vector<
				// Colors
#if 1
				styles::Color								// 'color' property
#else
				std::shared_ptr<graphics::Paint>			// text paint style
#endif
			> Colors;

			/// "Backgrounds and Borders" part.
			typedef boost::fusion::vector<
				styles::BackgroundColor,					// 'background-color' properties
				FlowRelativeFourSides<styles::BorderColor>,	// 'border-color' properties
				FlowRelativeFourSides<styles::BorderStyle>,	// 'border-style' properties
				FlowRelativeFourSides<styles::BorderWidth>	// 'border-width' properties
			> BackgroundsAndBorders;

			/// "BasicBoxModel" part.
			typedef boost::fusion::vector<
				FlowRelativeFourSides<styles::PaddingSide>,	// 'padding' properties			
				FlowRelativeFourSides<styles::MarginSide>	// 'margin' properties
			> BasicBoxModel;

			/// "Fonts" part.
			typedef boost::fusion::vector<
				styles::FontFamily,							// 'font-family' property
				styles::FontWeight,							// 'font-weight' property
				styles::FontStretch,						// 'font-stretch' property
				styles::FontStyle,							// 'font-style' property
				styles::FontSize,							// 'font-size property
				styles::FontSizeAdjust						// 'font-size-adjust' property
//				styles::FontFeatureSettings,				// 'font-feature-settings' property
//				styles::FontLanguageOverride				// 'font-language-override' property
			> Fonts;

			/// "Inline Layout" part.
			typedef boost::fusion::vector<
				styles::TextHeight,							// 'text-height' property
				styles::DominantBaseline,					// 'dominant-baseline' property
				styles::AlignmentBaseline,					// 'alignment-baseline' property
				styles::AlignmentAdjust						// 'alignment-adjust' property
			> InlineLayout;

			/// "Text" part.
			typedef boost::fusion::vector<
				styles::TextTransform,						// 'text-transform' property
				styles::WhiteSpace,							// 'white-space' property
				styles::Hyphens,							// 'hyphens' property
				styles::WordSpacing,						// 'word-spacing' property
				styles::LetterSpacing,						// 'letter-spacing' property
				styles::HangingPunctuation					// 'hanging-punctuation' property
			> Text;

			/// "Text Decoration" part.
			typedef boost::fusion::vector<
				styles::TextDecorationLine,					// 'text-decoration-line' properties
				styles::TextDecorationColor,				// 'text-decoration-color' properties
				styles::TextDecorationStyle,				// 'text-decoration-style' properties
				styles::TextDecorationSkip,					// 'text-decoration-skip' properties
				styles::TextUnderlinePosition,				// 'text-underline-position' properties
				styles::TextEmphasisStyle,					// 'text-emphasis-style' properties
				styles::TextEmphasisColor,					// 'text-emphasis-color' properties
				styles::TextEmphasisPosition				// 'text-emphasis-position' properties
//				styles::TextShadow							// 'text-shadow' property
			> TextDecoration;

			/// "Writing Modes" part.
			typedef boost::fusion::vector<
				styles::Direction							// 'direction' property
			> WritingModes;

			/// "Auxiliary" part.
			typedef boost::fusion::vector<
//				styles::Effects,							// 'effects' property
				styles::ShapingEnabled,						// 'shaping-enabled' property
				styles::NumberSubstitution					// 'number-substitution' property
			> Auxiliary;
		};

		namespace styles {
			template<typename Parts>
			struct DeclaredValuesOfParts : presentation::detail::TransformAsMap<
				Parts, presentation::detail::KeyValueConverter<DeclaredValue>
			> {};
			template<typename Parts>
			struct SpecifiedValuesOfParts : presentation::detail::TransformAsMap<
				Parts, presentation::detail::KeyValueConverter<SpecifiedValue>
			> {};
			template<typename Parts>
			struct ComputedValuesOfParts : presentation::detail::TransformAsMap<
				Parts, presentation::detail::KeyValueConverter<ComputedValue>
			> {};
		}

		/**
		 * A text run style collection.
		 * @see TextLineStyle, TextToplevelStyle, StyledTextRunIterator
		 * @note This structure is defined as joint of the multiple subparts (TextRunStyleN), because some compilers
		 *       use large memory and may crash when compile the client codes.
		 */
		template<template<typename> class Transformation>
		struct BasicTextRunStyle {
			typename Transformation<TextRunStyleParts::Colors>::type colors;
			typename Transformation<TextRunStyleParts::BackgroundsAndBorders>::type backgroundsAndBorders;
			typename Transformation<TextRunStyleParts::BasicBoxModel>::type basicBoxModel;
			typename Transformation<TextRunStyleParts::Fonts>::type fonts;
			typename Transformation<TextRunStyleParts::InlineLayout>::type inlineLayout;
			typename Transformation<TextRunStyleParts::Text>::type text;
			typename Transformation<TextRunStyleParts::TextDecoration>::type textDecoration;
			typename Transformation<TextRunStyleParts::WritingModes>::type writingModes;
			typename Transformation<TextRunStyleParts::Auxiliary>::type auxiliary;
		};

		typedef BasicTextRunStyle<boost::mpl::identity> TextRunStyle;
		/// "Specified Value"s of @c TextRunStyle.
		struct SpecifiedTextRunStyle : BasicTextRunStyle<styles::SpecifiedValuesOfParts> {};
		/// "Computed Value"s of @c TextRunStyle.
		struct ComputedTextRunStyle : BasicTextRunStyle<styles::ComputedValuesOfParts> {};

//		ASCENSION_ASSERT_STYLE_SEQUECE_UNIQUE(TextRunStyle);
//		ASCENSION_ASSERT_STYLE_SEQUECE_UNIQUE(SpecifiedTextRunStyle);
//		ASCENSION_ASSERT_STYLE_SEQUECE_UNIQUE(ComputedTextRunStyle);

		/// "Declared Value"s of @c TextRunStyle.
		class DeclaredTextRunStyle : public BasicTextRunStyle<styles::DeclaredValuesOfParts> {
		public:
#ifndef BOOST_NO_CXX11_DEFAULTED_FUNCTIONS
			DeclaredTextRunStyle() = default;
#else
			DeclaredTextRunStyle();
#endif
			static const DeclaredTextRunStyle& unsetInstance();
		};

		namespace styles {
			template<> class DeclaredValue<TextRunStyle> : public ValueBase<TextRunStyle, DeclaredTextRunStyle> {};
			template<> struct SpecifiedValue<TextRunStyle> : boost::mpl::identity<SpecifiedTextRunStyle> {};
			template<> struct ComputedValue<TextRunStyle> : boost::mpl::identity<ComputedTextRunStyle> {};
		}

		boost::flyweight<styles::ComputedValue<TextRunStyle>::type> compute(
			const styles::SpecifiedValue<TextRunStyle>::type& specifiedValues,
			const styles::Length::Context& context,
			const styles::ComputedValue<TextRunStyle>::type& parentComputedValues);
		std::size_t hash_value(const styles::ComputedValue<TextRunStyle>::type& style);
	}
} // namespace ascension.presentation

#endif // !ASCENSION_TEXT_RUN_STYLE_HPP
