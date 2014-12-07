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
#ifndef FUSION_MAX_VECTOR_SIZE
#	define FUSION_MAX_VECTOR_SIZE 40
#endif

#include <ascension/corelib/memory.hpp>
#include <ascension/presentation/styles/writing-modes.hpp>
#include <boost/flyweight/flyweight_fwd.hpp>
#include <boost/fusion/algorithm/transformation/transform.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
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
		class DeclaredTextToplevelStyle : 
			public boost::fusion::result_of::as_vector<
				boost::fusion::result_of::transform<
					TextToplevelStyle, styles::detail::ValueConverter<styles::DeclaredValue>
				>::type
			>::type,
			public std::enable_shared_from_this<DeclaredTextToplevelStyle> {
		public:
			DeclaredTextToplevelStyle();
			/// Returns the default @c DeclaredTextLineStyle of this toplevel element.
			BOOST_CONSTEXPR std::shared_ptr<const DeclaredTextLineStyle> linesStyle() const BOOST_NOEXCEPT {
				return linesStyle_;
			}
			void setLinesStyle(std::shared_ptr<const DeclaredTextLineStyle> newStyle) BOOST_NOEXCEPT;
			static const DeclaredTextToplevelStyle& unsetInstance();

		private:
			std::shared_ptr<const DeclaredTextLineStyle> linesStyle_;
		};

		/// "Specified Values" of @c TextToplevelStyle.
		struct SpecifiedTextToplevelStyle :
			boost::fusion::result_of::as_vector<
				boost::fusion::result_of::transform<
					TextToplevelStyle, styles::detail::ValueConverter<styles::SpecifiedValue>
				>::type
			>::type {};

		/// "Computed Values" of @c TextToplevelStyle.
		struct ComputedTextToplevelStyle :
			boost::fusion::result_of::as_vector<
				boost::fusion::result_of::transform<
					TextToplevelStyle, styles::detail::ValueConverter<styles::ComputedValue>
				>::type
			>::type {};

		namespace styles {
			template<> class DeclaredValue<TextToplevelStyle> : public boost::mpl::identity<DeclaredTextToplevelStyle> {};
			template<> struct SpecifiedValue<TextToplevelStyle> : boost::mpl::identity<SpecifiedTextToplevelStyle> {};
			template<> struct ComputedValue<TextToplevelStyle> : boost::mpl::identity<ComputedTextToplevelStyle> {};
		}

		boost::flyweight<ComputedTextToplevelStyle> compute(const SpecifiedTextToplevelStyle& specifiedValues);
		std::size_t hash_value(const ComputedTextToplevelStyle& style);
	}
} // namespace ascension.presentation

#endif // !ASCENSION_TEXT_TOPLEVEL_STYLE_HPP
