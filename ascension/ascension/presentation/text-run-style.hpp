/**
 * @file text-run-style.hpp
 * @author exeal
 * @see presentation.hpp, graphics/text-alignment.hpp, graphics/text-layout-styles.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2012-07-16 reunioned with text-line-style.hpp
 * @date 2014-09-27 Separated from text-style.hpp
 */

#ifndef ASCENSION_TEXT_RUN_STYLE_HPP
#define ASCENSION_TEXT_RUN_STYLE_HPP
#ifndef FUSION_MAX_VECTOR_SIZE
#	define FUSION_MAX_VECTOR_SIZE 40
#endif

#include <ascension/directions.hpp>
#include <ascension/presentation/styles/auxiliary.hpp>
#include <ascension/presentation/styles/background.hpp>
#include <ascension/presentation/styles/box.hpp>
#include <ascension/presentation/styles/color.hpp>
#include <ascension/presentation/styles/fonts.hpp>
#include <ascension/presentation/styles/inline.hpp>
#include <ascension/presentation/styles/text.hpp>
#include <ascension/presentation/styles/text-decor.hpp>
#include <boost/flyweight/flyweight_fwd.hpp>
#include <boost/fusion/algorithm/transformation/transform.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/transform.hpp>

namespace ascension {
	namespace presentation {
		/**
		 * A text run style collection.
		 * @see DeclaredTextRunStyle, SpecifiedTextRunStyle, ComputedTextRunStyle
		 * @see TextLineStyle, TextToplevelStyle, StyledTextRunIterator
		 */
		typedef boost::fusion::vector<
			// Colors
#if 1
			styles::Color,								// 'color' property
#else
			std::shared_ptr<graphics::Paint>,			// text paint style
#endif
			// Backgrounds and Borders
			styles::BackgroundColor,					// 'background-color' properties
			FlowRelativeFourSides<styles::BorderColor>,	// 'border-color' properties
			FlowRelativeFourSides<styles::BorderStyle>,	// 'border-style' properties
			FlowRelativeFourSides<styles::BorderWidth>,	// 'border-width' properties
			// Basic Box Model
			FlowRelativeFourSides<styles::PaddingSide>,	// 'padding' properties			
			FlowRelativeFourSides<styles::MarginSide>,	// 'margin' properties
			// Fonts
			styles::FontFamily,							// 'font-family' property
			styles::FontWeight,							// 'font-weight' property
			styles::FontStretch,						// 'font-stretch' property
			styles::FontStyle,							// 'font-style' property
			styles::FontSize,							// 'font-size property
			styles::FontSizeAdjust,						// 'font-size-adjust' property
//			styles::FontFeatureSettings,				// 'font-feature-settings' property
//			styles::FontLanguageOverride,				// 'font-language-override' property
			// Inline Layout
			styles::TextHeight,							// 'text-height' property
			styles::LineHeight,							// 'line-height' property
			styles::DominantBaseline,					// 'dominant-baseline' property
			styles::AlignmentBaseline,					// 'alignment-baseline' property
			styles::AlignmentAdjust,					// 'alignment-adjust' property
			styles::BaselineShift,						// 'baseline-shift' property
			// Text
			styles::TextTransform,						// 'text-transform' property
			styles::Hyphens,							// 'hyphens' property
			styles::WordSpacing,						// 'word-spacing' property
			styles::LetterSpacing,						// 'letter-spacing' property
			// Text Decoration
			styles::TextDecorationLine,					// 'text-decoration-line' properties
			styles::TextDecorationColor,				// 'text-decoration-color' properties
			styles::TextDecorationStyle,				// 'text-decoration-style' properties
			styles::TextDecorationSkip,					// 'text-decoration-skip' properties
			styles::TextUnderlinePosition,				// 'text-underline-position' properties
			styles::TextEmphasisStyle,					// 'text-emphasis-style' properties
			styles::TextEmphasisColor,					// 'text-emphasis-color' properties
			styles::TextEmphasisPosition,				// 'text-emphasis-position' properties
//			styles::TextShadow,							// 'text-shadow' property
			// Auxiliary
			styles::ShapingEnabled/*,					// 'shaping-enabled' property
			styles::DeprecatedFormatCharactersDisabled,	// 'deprecated-format-characters-disabled' property
			styles::SymmetricSwappingInhibited			// 'symmetric-swapping-inhibited' property
*/		> TextRunStyle;

		// TODO: Check uniqueness of the members of TextRunStyle.

		class DeclaredTextRunStyle : public TextRunStyle,
				public FastArenaObject<DeclaredTextRunStyle>, public std::enable_shared_from_this<DeclaredTextRunStyle> {
		public:
#ifndef BOOST_NO_CXX11_DEFAULTED_FUNCTIONS
			DeclaredTextRunStyle() = default;
#else
			DeclaredTextRunStyle();
#endif
			static const DeclaredTextRunStyle& unsetInstance();
		};

		/// "Specified Values" of @c TextRunStyle.
#if 1
		struct SpecifiedTextRunStyle :
			boost::mpl::transform<TextRunStyle, styles::SpecifiedValue<boost::mpl::_1>>::type {};
#else
		typedef boost::fusion::result_of::as_vector<
			boost::mpl::transform<TextRunStyle, styles::SpecifiedValue<boost::mpl::_1>>::type
		>::type SpecifiedTextRunStyle;
#endif

		/// "Computed Values" of @c TextRunStyle.
#if 1
		struct ComputedTextRunStyle :
			boost::mpl::transform<TextRunStyle, styles::ComputedValue<boost::mpl::_1>>::type {};
#else
		typedef boost::fusion::result_of::as_vector<
			boost::mpl::transform<TextRunStyle, styles::ComputedValue<boost::mpl::_1>>::type
		>::type ComputedTextRunStyle;
#endif

		namespace styles {
			template<> struct SpecifiedValue<TextRunStyle> : boost::mpl::identity<SpecifiedTextRunStyle> {};
			template<> struct ComputedValue<TextRunStyle> : boost::mpl::identity<ComputedTextRunStyle> {};
		}

		boost::flyweight<ComputedTextRunStyle> compute(const SpecifiedTextRunStyle& specifiedValues,
			const styles::Length::Context& context, const ComputedTextRunStyle& parentComputedValues);
		std::size_t hash_value(const ComputedTextRunStyle& style);
	}
} // namespace ascension.presentation

#endif // !ASCENSION_TEXT_RUN_STYLE_HPP
