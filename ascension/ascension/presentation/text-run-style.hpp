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
#include <boost/fusion/algorithm/transformation/join.hpp>
#include <boost/fusion/container/vector.hpp>

namespace ascension {
	namespace presentation {
		/**
		 * A text run style collection, part 1.
		 * @see TextRunStyle2
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
			styles::DominantBaseline,					// 'dominant-baseline' property
			styles::AlignmentBaseline,					// 'alignment-baseline' property
			styles::AlignmentAdjust						// 'alignment-adjust' property
		> TextRunStyle1;

		/**
		 * A text run style collection, part 2.
		 * @see TextRunStyle1
		 * @see TextLineStyle, TextToplevelStyle, StyledTextRunIterator
		 */
		typedef boost::fusion::vector<
			// Text
			styles::TextTransform,						// 'text-transform' property
			styles::WhiteSpace,							// 'white-space' property
			styles::Hyphens,							// 'hyphens' property
			styles::WordSpacing,						// 'word-spacing' property
			styles::LetterSpacing,						// 'letter-spacing' property
			styles::HangingPunctuation,					// 'hanging-punctuation' property
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
			// Writing Modes
			styles::Direction,							// 'direction' property
			// Auxiliary
//			styles::Effects,							// 'effects' property
			styles::ShapingEnabled,						// 'shaping-enabled' property
			styles::NumberSubstitution					// 'number-substitution' property
		> TextRunStyle2;

		/// "Declared Values" of @c TextRunStyle1.
		class DeclaredTextRunStyle1 :
			public detail::TransformedAsMap<
				TextRunStyle1, detail::ValueConverter<styles::DeclaredValue>
			>,
			public std::enable_shared_from_this<DeclaredTextRunStyle1> {
		public:
#ifndef BOOST_NO_CXX11_DEFAULTED_FUNCTIONS
			DeclaredTextRunStyle1() = default;
#else
			DeclaredTextRunStyle1();
#endif
			static const DeclaredTextRunStyle1& unsetInstance();
		};

		/// "Declared Values" of @c TextRunStyle2.
		class DeclaredTextRunStyle2 :
			public detail::TransformedAsMap<
				TextRunStyle2, detail::ValueConverter<styles::DeclaredValue>
			>,
			public std::enable_shared_from_this<DeclaredTextRunStyle2> {
		public:
#ifndef BOOST_NO_CXX11_DEFAULTED_FUNCTIONS
			DeclaredTextRunStyle2() = default;
#else
			DeclaredTextRunStyle2();
#endif
			static const DeclaredTextRunStyle2& unsetInstance();
		};

		ASCENSION_ASSERT_STYLE_SEQUECE_UNIQUE(TextRunStyle1);
		ASCENSION_ASSERT_STYLE_SEQUECE_UNIQUE(TextRunStyle2);
//		ASCENSION_ASSERT_STYLE_SEQUECE_UNIQUE(SpecifiedTextRunStyle);
//		ASCENSION_ASSERT_STYLE_SEQUECE_UNIQUE(ComputedTextRunStyle);

		/// "Specified Value"s of @c TextRunStyle1.
		struct SpecifiedTextRunStyle1 : presentation::detail::TransformedAsMap<
			TextRunStyle1, presentation::detail::KeyValueConverter<styles::SpecifiedValue>
		> {};

		/// "Specified Value"s of @c TextRunStyle2.
		struct SpecifiedTextRunStyle2 : presentation::detail::TransformedAsMap<
			TextRunStyle2, presentation::detail::KeyValueConverter<styles::SpecifiedValue>
		> {};

		/// "Specified Value"s of @c TextRunStyle.
		struct SpecifiedTextRunStyle : boost::fusion::result_of::join<SpecifiedTextRunStyle1, SpecifiedTextRunStyle2>::type {};

		/// "Computed Value"s of @c TextRunStyle1.
		struct ComputedTextRunStyle1 : presentation::detail::TransformedAsMap<
			TextRunStyle1, presentation::detail::KeyValueConverter<styles::ComputedValue>
		> {};

		/// "Computed Value"s of @c TextRunStyle2.
		struct ComputedTextRunStyle2 : presentation::detail::TransformedAsMap<
			TextRunStyle2, presentation::detail::KeyValueConverter<styles::ComputedValue>
		> {};

		/// "Computed Value"s of @c TextRunStyle.
		struct ComputedTextRunStyle : boost::fusion::result_of::join<ComputedTextRunStyle1, ComputedTextRunStyle2>::type {};

		namespace styles {
			template<> class DeclaredValue<TextRunStyle1> : public ValueBase<TextRunStyle1, DeclaredTextRunStyle1> {};
			template<> class DeclaredValue<TextRunStyle2> : public ValueBase<TextRunStyle2, DeclaredTextRunStyle2> {};
			template<> struct SpecifiedValue<TextRunStyle1> : boost::mpl::identity<SpecifiedTextRunStyle1> {};
			template<> struct SpecifiedValue<TextRunStyle2> : boost::mpl::identity<SpecifiedTextRunStyle2> {};
			template<> struct ComputedValue<TextRunStyle1> : boost::mpl::identity<ComputedTextRunStyle1> {};
			template<> struct ComputedValue<TextRunStyle2> : boost::mpl::identity<ComputedTextRunStyle2> {};
		}

		/// "Declared Value"s of @c TextRunStyle.
		class DeclaredTextRunStyle : public boost::fusion::result_of::as_map<
			boost::fusion::result_of::as_vector<
				boost::fusion::result_of::join<
					boost::fusion::result_of::as_vector<DeclaredTextRunStyle1>::type,
					boost::fusion::result_of::as_vector<DeclaredTextRunStyle2>::type
				>::type
			>::type
		>::type {
		public:
#ifndef BOOST_NO_CXX11_DEFAULTED_FUNCTIONS
			DeclaredTextRunStyle() = default;
#else
			DeclaredTextRunStyle();
#endif
			static const DeclaredTextRunStyle& unsetInstance();
		};

		boost::flyweight<styles::ComputedValue<TextRunStyle1>::type> compute(
			const styles::SpecifiedValue<TextRunStyle1>::type& specifiedValues,
			const styles::Length::Context& context,
			const styles::ComputedValue<TextRunStyle1>::type& parentComputedValues1,
			const styles::ComputedValue<TextRunStyle2>::type& parentComputedValues2);
		boost::flyweight<styles::ComputedValue<TextRunStyle2>::type> compute(
			const styles::SpecifiedValue<TextRunStyle2>::type& specifiedValues,
			const styles::Length::Context& context,
			const styles::ComputedValue<styles::Color>::type& computedColor,
			const styles::ComputedValue<TextRunStyle2>::type& parentComputedValues2);
		std::size_t hash_value(const styles::ComputedValue<TextRunStyle1>::type& style);
		std::size_t hash_value(const styles::ComputedValue<TextRunStyle2>::type& style);
	}
} // namespace ascension.presentation

#endif // !ASCENSION_TEXT_RUN_STYLE_HPP
