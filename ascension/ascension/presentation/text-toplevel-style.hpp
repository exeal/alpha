/**
 * @file text-toplevel-style.hpp
 * @author exeal
 * @see presentation.hpp, graphics/text-alignment.hpp, graphics/text-layout-styles.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2012-07-16 reunioned with text-line-style.hpp
 * @date 2014-09-27 Separated from text-style.hpp
 */

#ifndef ASCENSION_TEXT_TOP_LEVEL_STYLE_HPP
#define ASCENSION_TEXT_TOP_LEVEL_STYLE_HPP

#include <ascension/corelib/memory.hpp>
#include <ascension/presentation/styles/writing-modes.hpp>
#include <boost/flyweight/flyweight_fwd.hpp>
#include <boost/fusion/algorithm/transformation/transform.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/transform.hpp>
#include <memory>

namespace ascension {
	namespace presentation {
		class DeclaredTextLineStyle;

		/**
		 * A text toplevel style collection.
		 * The writing modes specified by this style may be overridden by @c graphics#font#TextRenderer#writingMode.
		 * @see DeclaredTextToplevelStyle, SpecifiedTextToplevelStyle, ComputedTextToplevelStyle
		 * @see TextRunStyle, TextLineStyle, Presentation#textToplevelStyle, Presentation#setTextToplevelStyle
		 */
		typedef boost::fusion::vector<
			// Writing Modes
			styles::WritingMode	// 'writing-mode' property
		> TextToplevelStyle;

		// TODO: Check uniqueness of the members of TextToplevelStyle.

		/// "Declared Values" of @c TextToplevelStyle.
		class DeclaredTextToplevelStyle : public TextToplevelStyle,
			public FastArenaObject<DeclaredTextToplevelStyle>, public std::enable_shared_from_this<DeclaredTextToplevelStyle> {
		public:
			DeclaredTextToplevelStyle();
			/// Returns the default @c DeclaredTextLineStyle of this toplevel element.
			BOOST_CONSTEXPR std::shared_ptr<const DeclaredTextLineStyle> linesStyle() const BOOST_NOEXCEPT {
				return linesStyle_;
			}
			void setLinesStyle(std::shared_ptr<const DeclaredTextLineStyle> newStyle) BOOST_NOEXCEPT;

		private:
			std::shared_ptr<const DeclaredTextLineStyle> linesStyle_;
		};

		/// "Specified Values" of @c TextToplevelStyle.
#if 1
		struct SpecifiedTextToplevelStyle :
			boost::mpl::transform<TextToplevelStyle, styles::SpecifiedValueType<boost::mpl::_1>>::type {};
#else
		typedef boost::fusion::result_of::as_vector<
			boost::mpl::transform<TextToplevelStyle, styles::SpecifiedValueType<boost::mpl::_1>>::type
		>::type SpecifiedTextToplevelStyle;
#endif

		/// "Computed Values" of @c TextToplevelStyle.
#if 1
		struct ComputedTextToplevelStyle :
			boost::mpl::transform<TextToplevelStyle, styles::ComputedValueType<boost::mpl::_1>>::type {};
#else
		typedef boost::fusion::result_of::as_vector<
			boost::mpl::transform<TextToplevelStyle, styles::ComputedValueType<boost::mpl::_1>>::type
		>::type ComputedTextToplevelStyle;
#endif

		boost::flyweight<ComputedTextToplevelStyle> compute(const SpecifiedTextToplevelStyle& specifiedValues);
		std::size_t hash_value(const ComputedTextToplevelStyle& style);
	}
} // namespace ascension.presentation

#endif // !ASCENSION_TEXT_TOPLEVEL_STYLE_HPP
