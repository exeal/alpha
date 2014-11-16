/**
 * @file text-line-style.hpp
 * @author exeal
 * @see presentation.hpp, graphics/text-alignment.hpp, graphics/text-layout-styles.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2012-07-16 reunioned with text-line-style.hpp
 * @date 2014-09-27 Separated from text-style.hpp
 */

#ifndef ASCENSION_TEXT_LINE_STYLE_HPP
#define ASCENSION_TEXT_LINE_STYLE_HPP
#ifndef FUSION_MAX_VECTOR_SIZE
#	define FUSION_MAX_VECTOR_SIZE 40
#endif

#include <ascension/corelib/memory.hpp>
#include <ascension/presentation/styles/auxiliary.hpp>
#include <ascension/presentation/styles/box.hpp>
#include <ascension/presentation/styles/inline.hpp>
#include <ascension/presentation/styles/text.hpp>
#include <ascension/presentation/styles/writing-modes.hpp>
#include <boost/flyweight/flyweight_fwd.hpp>
#include <boost/fusion/algorithm/transformation/transform.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/transform.hpp>
#include <memory>

namespace ascension {
	namespace presentation {
		class DeclaredTextRunStyle;

		/**
		 * A text line style collection.
		 * @see DeclaredTextLineStyle, SpecifiedTextLineStyle, ComputedTextLineStyle
		 * @see TextRunStyle, TextToplevelStyle, TextLineStyleDirector
		 */
		typedef boost::fusion::vector<
			// Writing Modes
			styles::Direction,			// 'direction' property
//			styles::UnicodeBidi,		// 'unicode-bidi' property
			styles::TextOrientation,	// 'text-orientation' property
			// Inline Layout
			styles::LineHeight,			// 'line-height' property
			styles::LineBoxContain,		// 'line-box-contain' property
			styles::DominantBaseline,	// 'dominant-baseline' property
			styles::InlineBoxAlignment,	// einline-box-alignf property
			// Text
			styles::WhiteSpace,			// 'white-space' property
			styles::TabSize,			// 'tab-size' property
			styles::LineBreak,			// 'line-break' property
			styles::WordBreak,			// 'word-break' property
			styles::OverflowWrap,		// 'overflow-wrap' property
			styles::TextAlignment,		// 'text-align' property
			styles::TextAlignmentLast,	// 'text-align-last' property
			styles::TextJustification,	// 'text-justify' property
			styles::TextIndent,			// 'text-indent' property
			styles::HangingPunctuation,	// 'hanging-punctuation' property
			// Basic Box Model
			styles::Measure,			// 'width' property. boost#none stands eautof value
			// Auxiliary
			styles::NumberSubstitution	// 'number-substitution' property
		> TextLineStyle;

		// TODO: Check uniqueness of the members of TextLineStyle.

		/// "Declared Values" of @c TextLineStyle.
		class DeclaredTextLineStyle : public boost::mpl::transform<TextLineStyle, styles::DeclaredValue<boost::mpl::_1>>::type,
			public FastArenaObject<DeclaredTextLineStyle>, public std::enable_shared_from_this<DeclaredTextLineStyle> {
		public:
			DeclaredTextLineStyle();
			/// Returns the default @c DeclaredTextRunStyle of this line element.
			BOOST_CONSTEXPR std::shared_ptr<const DeclaredTextRunStyle> linesStyle() const BOOST_NOEXCEPT {
				return runsStyle_;
			}
			void setRunsStyle(std::shared_ptr<const DeclaredTextRunStyle> newStyle) BOOST_NOEXCEPT;
			static const DeclaredTextLineStyle& unsetInstance();

		private:
			std::shared_ptr<const DeclaredTextRunStyle> runsStyle_;
		};

		/// "Specified Values" of @c TextLineStyle.
#if 1
		struct SpecifiedTextLineStyle :
			boost::mpl::transform<TextLineStyle, styles::SpecifiedValue<boost::mpl::_1>>::type {};
#else
		typedef boost::fusion::result_of::as_vector<
			boost::mpl::transform<TextLineStyle, styles::SpecifiedValue<boost::mpl::_1>>::type
		>::type SpecifiedTextLineStyle;
#endif

		/// "Computed Values" of @c TextLineStyle.
#if 1
		struct ComputedTextLineStyle :
			boost::mpl::transform<TextLineStyle, styles::ComputedValue<boost::mpl::_1>>::type {};
#else
		typedef boost::fusion::result_of::as_vector<
			boost::mpl::transform<TextLineStyle, styles::ComputedValue<boost::mpl::_1>>::type
		>::type ComputedTextLineStyle;
#endif

		namespace styles {
			template<> struct SpecifiedValue<TextLineStyle> : boost::mpl::identity<SpecifiedTextLineStyle> {};
			template<> struct ComputedValue<TextLineStyle> : boost::mpl::identity<ComputedTextLineStyle> {};
		}

		boost::flyweight<ComputedTextLineStyle> compute(
			const SpecifiedTextLineStyle& specifiedValues, const styles::Length::Context& context);
		std::size_t hash_value(const ComputedTextLineStyle& style);
	}
} // namespace ascension.presentation

#endif // !ASCENSION_TEXT_LINE_STYLE_HPP
