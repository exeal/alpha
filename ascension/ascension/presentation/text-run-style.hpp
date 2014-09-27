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

#include <ascension/directions.hpp>
#include <ascension/presentation/styles/auxiliary.hpp>
#include <ascension/presentation/styles/background.hpp>
#include <ascension/presentation/styles/box.hpp>
#include <ascension/presentation/styles/color.hpp>
#include <ascension/presentation/styles/fonts.hpp>
#include <ascension/presentation/styles/inline.hpp>
#include <ascension/presentation/styles/text.hpp>
#include <ascension/presentation/styles/text-decor.hpp>
#include <memory>
#include <boost/range/irange.hpp>

namespace ascension {
	namespace presentation {
		/**
		 * Declares visual style settings of a text run.
		 * @see TextLineStyle, TextToplevelStyle, StyledTextRunIterator
		 */
		struct TextRunStyle :
				public FastArenaObject<TextRunStyle>,
				public std::enable_shared_from_this<TextRunStyle> {
			/// @name Colors
			/// @{
#if 1
			styles::Color<styles::Inherited<true>> color;	///< The 'color' property.
#else
			std::shared_ptr<graphics::Paint> foreground;	///< Text paint style.
#endif
			/// @}

			/// @name Backgrounds and Borders
			/// @{
			styles::Background background;	///< The 'background properties.
			styles::Border border;			///< The 'border' properties.
			/// @}

			/// @name Basic Box Model
			/// @{			
			FlowRelativeFourSides<styles::PaddingSide> padding;	///< The 'padding' properties.			
			FlowRelativeFourSides<styles::MarginSide> margin;	///< The 'margin' properties.
			/// @}

			/// @name Fonts
			/// @{
			styles::FontFamily fontFamily;						///< The 'font-family' property.
			styles::FontWeight fontWeight;						///< The 'font-weight' property.
			styles::FontStretch fontStretch;					///< The 'font-stretch' property.
			styles::FontStyle fontStyle;						///< The 'font-style' property.
			styles::FontSize fontSize;							///< The 'font-size property.
			styles::FontSizeAdjust fontSizeAdjust;				///< The 'font-size-adjust' property.
//			styles::FontFeatureSettings fontFeatureSettings;	///< The 'font-feature-settings' property.
//			styles::FontLanguageOverride fontLanguageOverride;	///< The 'font-language-override' property.
			/// @}

			/// @name Inline Layout
			/// @{
			styles::TextHeight textHeight;					///< The 'text-height' property.
			styles::LineHeight lineHeight;					///< The 'line-height' property.
			styles::DominantBaseline dominantBaseline;		///< The 'dominant-baseline' property.
			styles::AlignmentBaseline alignmentBaseline;	///< The 'alignment-baseline' property.
			styles::AlignmentAdjust alignmentAdjust;		///< The 'alignment-adjust' property.
			styles::BaselineShift baselineShift;			///< The 'baseline-shift' property.
			/// @}

			/// @name Text
			/// @{
			styles::TextTransform textTransform;	///< The 'text-transform' property.
			styles::Hyphens hyphens;				///< The 'hyphens' property.
			styles::WordSpacing wordSpacing;		///< The 'word-spacing' property.
			styles::LetterSpacing letterSpacing;	///< The 'letter-spacing' property.
			/// @}

			/// @name Text Decoration
			/// @{
			styles::TextDecoration textDecoration;	///< The 'text-decoration' properties.
			styles::TextEmphasis textEmphasis;		///< The 'text-emphasis' properties.
			styles::TextShadow textShadow;			///< The 'text-shadow' property.
			/// @}

			/// @name Auxiliary
			/// @{
			styles::ShapingEnabled shapingEnabled;	///< The 'shaping-enabled' property.
			/// @}
		};
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
	}
} // namespace ascension.presentation

#endif // !ASCENSION_TEXT_RUN_STYLE_HPP
