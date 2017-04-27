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
#include <ascension/corelib/memory.hpp>
#include <ascension/presentation/detail/style-sequence.hpp>
#include <ascension/presentation/styles/auxiliary.hpp>
#include <ascension/presentation/styles/box.hpp>
#include <ascension/presentation/styles/inline.hpp>
#include <ascension/presentation/styles/text.hpp>
#include <ascension/presentation/styles/writing-modes.hpp>
#include <boost/fusion/algorithm/transformation/transform.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/operators.hpp>
#include <memory>

namespace ascension {
	namespace presentation {
		/**
		 * A text line style collection.
		 * @note @c TextLineStyle does not have style properties for text runs, but see @c DeclaredTextLineStyle.
		 * @see DeclaredTextLineStyle, SpecifiedTextLineStyle, ComputedTextLineStyle
		 * @see BasicTextRunStyle, TextRunStyleParts, TextToplevelStyle, TextLineStyleDirector
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
			styles::BaselineShift,		// 'baseline-shift' property
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
		
		class DeclaredTextRunStyle;

		/// "Declared Values" of @c TextLineStyle.
		class DeclaredTextLineStyle :
			public detail::TransformAsMap<
				TextLineStyle, detail::KeyValueConverter<styles::DeclaredValue>
			>::type,
			public std::enable_shared_from_this<DeclaredTextLineStyle> {
		public:
			DeclaredTextLineStyle();
			/// Returns the @c DeclaredTextRunStyle of this line element.
			std::shared_ptr<const DeclaredTextRunStyle> runsStyle() const BOOST_NOEXCEPT {
				return runsStyle_;
			}
			void setRunsStyle(std::shared_ptr<const DeclaredTextRunStyle> newStyle) BOOST_NOEXCEPT;
			static const DeclaredTextLineStyle& unsetInstance();

		private:
			std::shared_ptr<const DeclaredTextRunStyle> runsStyle_;
		};

		ASCENSION_ASSERT_STYLE_SEQUECE_UNIQUE(TextLineStyle);
#if 0
		ASCENSION_ASSERT_STYLE_SEQUECE_UNIQUE(DeclaredTextLineStyle);
		ASCENSION_ASSERT_STYLE_SEQUECE_UNIQUE(SpecifiedTextLineStyle);
		ASCENSION_ASSERT_STYLE_SEQUECE_UNIQUE(ComputedTextLineStyle);
#endif

		/// "Specified Value"s of @c TextLineStyle.
		struct SpecifiedTextLineStyle :
			presentation::detail::TransformAsMap<
				TextLineStyle, presentation::detail::KeyValueConverter<styles::SpecifiedValue>
			>::type,
			private boost::equality_comparable<SpecifiedTextLineStyle> {
			SpecifiedTextLineStyle();
			BOOST_CONSTEXPR bool operator==(const SpecifiedTextLineStyle& other) const {
				return boost::fusion::equal_to(*this, other);
			}
		};

		/// "Computed Value"s of @c TextLineStyle.
		struct ComputedTextLineStyle :
			presentation::detail::TransformAsMap<
				TextLineStyle, presentation::detail::KeyValueConverter<styles::ComputedValue>
			>::type,
			private boost::equality_comparable<ComputedTextLineStyle> {
			explicit ComputedTextLineStyle(const SpecifiedTextLineStyle& specifiedValues);
			ComputedTextLineStyle(const SpecifiedTextLineStyle& specifiedValues, const styles::Length::Context& context);
			BOOST_CONSTEXPR bool operator==(const ComputedTextLineStyle& other) const {
				return boost::fusion::equal_to(*this, other);
			}
		};

		namespace styles {
			template<> class DeclaredValue<TextLineStyle> : public boost::mpl::identity<DeclaredTextLineStyle> {};
			template<> struct SpecifiedValue<TextLineStyle> : boost::mpl::identity<SpecifiedTextLineStyle> {};
			template<> struct ComputedValue<TextLineStyle> : boost::mpl::identity<ComputedTextLineStyle> {};
		}

		std::size_t hash_value(const styles::SpecifiedValue<TextLineStyle>::type& style);
		std::size_t hash_value(const styles::ComputedValue<TextLineStyle>::type& style);
	}
} // namespace ascension.presentation

#endif // !ASCENSION_TEXT_LINE_STYLE_HPP
