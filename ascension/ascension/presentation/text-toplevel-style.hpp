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
		struct SpecifiedTextToplevelStyle : boost::fusion::result_of::as_vector<
			boost::mpl::transform<TextToplevelStyle, styles::SpecifiedValueType<boost::mpl::_1>>::type
		>::type {};
#else
		typedef boost::fusion::result_of::as_vector<
			boost::mpl::transform<TextToplevelStyle, styles::SpecifiedValueType<boost::mpl::_1>>::type
		>::type SpecifiedTextToplevelStyle;
#endif

		/// "Computed Values" of @c TextToplevelStyle.
#if 1
		struct ComputedTextToplevelStyle : boost::fusion::result_of::as_vector<
			boost::mpl::transform<TextToplevelStyle, styles::ComputedValueType<boost::mpl::_1>>::type
		>::type {};
#else
		typedef boost::fusion::result_of::as_vector<
			boost::mpl::transform<TextToplevelStyle, styles::ComputedValueType<boost::mpl::_1>>::type
		>::type ComputedTextToplevelStyle;
#endif

		std::size_t hash_value(const ComputedTextToplevelStyle& style);

		/**
		 * @see Presentation#computeTextLineStyle
		 */
		class GlobalTextStyleSwitch {
		public:
			/// Return type of @c #direction method.
			typedef decltype(TextLineStyle().direction) Direction;
			/// Return type of @c #textAlignment method.
			typedef decltype(TextLineStyle().textAlignment) TextAlignment;
			/// Return type of @c #textOrientation method.
			typedef decltype(TextLineStyle().textOrientation) TextOrientation;
			/// Return type of @c #whiteSpace method.
			typedef decltype(TextLineStyle().whiteSpace) WhiteSpace;
			/// Return type of @c #writingMode method.
			typedef decltype(TextToplevelStyle().writingMode) WritingMode;
		public:
			/// Destructor.
			virtual ~GlobalTextStyleSwitch() BOOST_NOEXCEPT {}

		private:
			/**
			 * Returns the 'direction' style property which follows @c TextLineStyle#direction and
			 * overrides @c TextToplevelStyle#defaultLineStyle#direction.
			 * @return The declared value of 'direction' style property
			 */
			virtual Direction direction() const BOOST_NOEXCEPT = 0;
			/**
			 * Returns the 'text-align' style property which follows @c TextLineStyle#textAlignment
			 * and overrides @c TextToplevelStyle#defaultLineStyle#textAlignment.
			 * @return The declared value of 'text-align' style property
			 */
			virtual TextAlignment textAlignment() const BOOST_NOEXCEPT = 0;
			/**
			 * Returns 'text-orientation' style property which follows
			 * @c TextLineStyle#textOrientation and overrides
			 * @c TextToplevelStyle#defaultLineStyle#textOrientation.
			 * @return The declared value of 'text-orientation' style property
			 */
			virtual TextOrientation textOrientation() const BOOST_NOEXCEPT = 0;
			/**
			 * Returns 'white-space' style property which follows @c TextLineStyle#whiteSpace and
			 * overrides @c TextToplevelStyle#defaultLineStyle#whiteSpace.
			 * @return The declared value of 'white-space' style property
			 */
			virtual WhiteSpace whiteSpace() const BOOST_NOEXCEPT = 0;
			/**
			 * Returns the 'writing-mode' style property which follows
			 * @c TextToplevelStyle#writingMode.
			 * @return The declared value of 'writing-mode' style property
			 */
			virtual WritingMode writingMode() const BOOST_NOEXCEPT = 0;
			friend class Presentation;
		};

		boost::flyweight<ComputedTextToplevelStyle> compute(const SpecifiedTextToplevelStyle& specifiedValues);
	}
} // namespace ascension.presentation

#endif // !ASCENSION_TEXT_TOPLEVEL_STYLE_HPP
