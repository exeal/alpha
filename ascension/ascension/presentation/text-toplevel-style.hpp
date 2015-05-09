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
#include <ascension/presentation/detail/style-sequence.hpp>
#include <ascension/presentation/styles/writing-modes.hpp>
#include <boost/fusion/algorithm/transformation/transform.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/operators.hpp>
#include <memory>

namespace ascension {
	namespace presentation {
		class DeclaredTextLineStyle;

		/**
		 * A text toplevel style collection.
		 * The writing modes specified by this style may be overridden by @c graphics#font#TextRenderer#writingMode.
		 * @see DeclaredTextToplevelStyle, SpecifiedTextToplevelStyle, ComputedTextToplevelStyle
		 * @see TextRunStyle1, TextRunStyle2, TextLineStyle, Presentation#textToplevelStyle,
		 *      Presentation#setTextToplevelStyle
		 */
		typedef boost::fusion::vector<
			// Writing Modes
			styles::WritingMode	// 'writing-mode' property
		> TextToplevelStyle;

		ASCENSION_ASSERT_STYLE_SEQUECE_UNIQUE(TextToplevelStyle);
#if 0
		ASCENSION_ASSERT_STYLE_SEQUECE_UNIQUE(DeclaredTextToplevelStyle);
		ASCENSION_ASSERT_STYLE_SEQUECE_UNIQUE(SpecifiedTextToplevelStyle);
		ASCENSION_ASSERT_STYLE_SEQUECE_UNIQUE(ComputedTextToplevelStyle);
#endif
		/// "Declared Values" of @c TextToplevelStyle.
		class DeclaredTextToplevelStyle : 
			public detail::TransformAsMap<
				TextToplevelStyle, detail::KeyValueConverter<styles::DeclaredValue>
			>::type,
			public std::enable_shared_from_this<DeclaredTextToplevelStyle> {
		public:
			DeclaredTextToplevelStyle();
			/// Returns the @c DeclaredTextLineStyle of this toplevel element.
			BOOST_CONSTEXPR std::shared_ptr<const DeclaredTextLineStyle> linesStyle() const BOOST_NOEXCEPT {
				return linesStyle_;
			}
			void setLinesStyle(std::shared_ptr<const DeclaredTextLineStyle> newStyle) BOOST_NOEXCEPT;
			static const DeclaredTextToplevelStyle& unsetInstance();

		private:
			std::shared_ptr<const DeclaredTextLineStyle> linesStyle_;
		};

		/// "Specified Value"s of @c TextToplevelStyle.
		struct SpecifiedTextToplevelStyle :
			presentation::detail::TransformAsMap<
				TextToplevelStyle, presentation::detail::KeyValueConverter<styles::SpecifiedValue>
			>::type,
			private boost::equality_comparable<SpecifiedTextToplevelStyle> {
			BOOST_CONSTEXPR bool operator==(const SpecifiedTextToplevelStyle& other) const {
				return boost::fusion::equal_to(*this, other);
			}
		};

		/// "Computed Value"s of @c TextToplevelStyle.
		struct ComputedTextToplevelStyle :
			presentation::detail::TransformAsMap<
				TextToplevelStyle, presentation::detail::KeyValueConverter<styles::ComputedValue>
			>::type,
			private boost::equality_comparable<ComputedTextToplevelStyle> {
			explicit ComputedTextToplevelStyle(const SpecifiedTextToplevelStyle& specifiedValues);
			BOOST_CONSTEXPR bool operator==(const ComputedTextToplevelStyle& other) const {
				return boost::fusion::equal_to(*this, other);
			}
		};

		namespace styles {
			template<> class DeclaredValue<TextToplevelStyle> : public boost::mpl::identity<DeclaredTextToplevelStyle> {};
			template<> struct SpecifiedValue<TextToplevelStyle> : boost::mpl::identity<SpecifiedTextToplevelStyle> {};
			template<> struct ComputedValue<TextToplevelStyle> : boost::mpl::identity<ComputedTextToplevelStyle> {};
		}

		std::size_t hash_value(const styles::SpecifiedValue<TextToplevelStyle>::type& style);
		std::size_t hash_value(const styles::ComputedValue<TextToplevelStyle>::type& style);
	}
} // namespace ascension.presentation

#endif // !ASCENSION_TEXT_TOPLEVEL_STYLE_HPP
