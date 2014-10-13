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
#	define FUSION_MAX_VECTOR_SIZE 30
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
#include <boost/fusion/algorithm/transformation/transform.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/transform.hpp>
#include <memory>
#include <boost/range/irange.hpp>

namespace ascension {
	namespace presentation {
		/**
		 * Declares visual style settings of a text run.
		 * @see TextLineStyle, TextToplevelStyle, StyledTextRunIterator
		 */
		typedef boost::fusion::vector<
			// Colors
#if 1
			styles::Color<styles::Inherited<true>>,		// 'color' property
#else
			std::shared_ptr<graphics::Paint>,			// text paint style
#endif
			// Backgrounds and Borders
			styles::Background,							// 'background properties
			styles::Border,								// 'border' properties
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
			styles::TextDecoration,						// 'text-decoration' properties
			styles::TextEmphasis,						// 'text-emphasis' properties
			styles::TextShadow,							// 'text-shadow' property
			// Auxiliary
			styles::ShapingEnabled/*,					// 'shaping-enabled' property
			styles::DeprecatedFormatCharactersDisabled,	// 'deprecated-format-characters-disabled' property
			styles::SymmetricSwappingInhibited			// 'symmetric-swapping-inhibited' property
*/		> TextRunStyle;

		// TODO: Check uniqueness of the members of TextRunStyle.

		struct DeclaredTextRunStyle : public TextRunStyle,
			public FastArenaObject<DeclaredTextRunStyle>, std::enable_shared_from_this<DeclaredTextRunStyle> {};
#if 0
		/**
		 * A @c StyledTextRun represents a text range with declared style. @c #beginning and
		 * @c #end return pointers to characters in the line text string.
		 * @note This class is not intended to be derived.
		 * @see StyledTextRunIterator, StyledTextRunEnumerator
		 */
		struct StyledTextRun : public StringPiece, public FastArenaObject<StyledTextRun> {
			/// The declared style in this text run.
			std::shared_ptr<const TextRunStyle> style;
			/// Default constructor.
			StyledTextRun() BOOST_NOEXCEPT {}
			/**
			 * Constructor.
			 * @param characterRange The range of the text run in the line
			 * @param style The declared style of the text run. Can be @c null
			 */
			StyledTextRun(const StringPiece& characterRange,
				std::shared_ptr<const TextRunStyle> style) BOOST_NOEXCEPT
				: StringPiece(characterRange), style_(style) {}
		};
#endif
		/**
		 * Abstract input iterator to obtain @c TextRunStyle objects.
		 * @see TextRunStyleDeclarator, graphics#font#ComputedStyledTextRunIterator
		 */
		class StyledTextRunIterator {
		public:
			/// Destructor.
			virtual ~StyledTextRunIterator() BOOST_NOEXCEPT {}
			/**
			 * Returns the range of the current text run addressed by this iterator.
			 * @return The range of the current text run this iterator addresses in character offsets in the line.
			 *         @c front() should be greater than or equal to @c back for the previous text run. If @c back is
			 *         greater than the length of the line, the range is truncated. Otherwise if @c front() is greater
			 *         than @c back() of the previous text run, treated as if there is a text run with the range
			 *         [previous's @c back(), front()) and default style
			 * @throw NoSuchElementException This iterator is done
			 * @see #currentStyle
			 */
			virtual boost::integer_range<Index> currentRange() const = 0;
			/**
			 * Returns the declared style of the current text run addressed by this iterator.
			 * @return The style of the current text run this iterator addresses. If @c null, the default text run is
			 *         used
			 * @throw NoSuchElementException This iterator is done
		 	 * @see #currentRange
			 */
			virtual std::shared_ptr<const TextRunStyle> currentStyle() const = 0;
			/// Returns @c true if the iterator addresses the end of the range.
			virtual bool isDone() const BOOST_NOEXCEPT = 0;
			/**
			 * Moves the iterator to the next styled text run.
			 * @throw NoSuchElementException This iterator is done.
			 */
			virtual void next() = 0;
		};

		typedef boost::fusion::result_of::as_vector<
			boost::mpl::transform<TextRunStyle, styles::SpecifiedValueType<boost::mpl::_1>>::type
		>::type SpecifiedTextRunStyle;
		typedef boost::fusion::result_of::as_vector<
			boost::mpl::transform<TextRunStyle, styles::ComputedValueType<boost::mpl::_1>>::type
		>::type ComputedTextRunStyle;

		/**
		 * @see TextLayout#TextLayout, presentation#StyledTextRunIterator
		 */
		class ComputedStyledTextRunIterator {
		public:
			/// Destructor.
			virtual ~ComputedStyledTextRunIterator() BOOST_NOEXCEPT {}
			/**
			 */
			virtual boost::integer_range<Index> currentRange() const = 0;
			/**
			 */
			virtual void currentStyle(ComputedTextRunStyle& style) const = 0;
			virtual bool isDone() const BOOST_NOEXCEPT = 0;
			virtual void next() = 0;
		};

		void computeTextRunStyle(const SpecifiedTextRunStyle& specifiedValues,
			const styles::Length::Context& context, const ComputedTextRunStyle& parentComputedValues, ComputedTextRunStyle& computedValues);
	}
} // namespace ascension.presentation

#endif // !ASCENSION_TEXT_RUN_STYLE_HPP
