/**
 * @file styled-text-run-iterator.hpp
 * @author exeal
 * @see presentation.hpp, graphics/text-alignment.hpp, graphics/text-layout-styles.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2012-07-16 reunioned with text-line-style.hpp
 * @date 2014-09-27 Separated from text-style.hpp
 * @date 2014-10-29 Separated from text-run-style.hpp
 */

#ifndef ASCENSION_STYLED_TEXT_RUN_ITERATOR_HPP
#define ASCENSION_STYLED_TEXT_RUN_ITERATOR_HPP

#include <boost/flyweight/flyweight_fwd.hpp>
#include <boost/range/irange.hpp>
#include <memory>

namespace ascension {
	namespace presentation {
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
		 * Abstract input iterator to obtain text run style objects.
		 * @tparam Style The return type of @c #currentStyle method
		 */
		template<typename Style>
		struct StyledTextRunIterator {
		public:
			/// Destructor.
			virtual ~StyledTextRunIterator() BOOST_NOEXCEPT {}
			/**
			 * Returns the range of the current text run addressed by this iterator.
			 * @return The range of the current text run this iterator addresses in character offsets in the line.
			 *         @c front() should be greater than or equal to @c back of the previous text run. If @c back is
			 *         greater than the length of the line, the range is truncated. Otherwise if @c front() is greater
			 *         than @c back() of the previous text run, treated as if there is a text run with the range
			 *         [previous's @c back(), front()) and unset style
			 * @throw NoSuchElementException This iterator is done
			 * @see #currentStyle
			 */
			virtual boost::integer_range<Index> currentRange() const = 0;
			/**
			 * Returns the style of the current text run addressed by this iterator.
			 * @return The style of the current text run this iterator addresses. If @c null, the default style is used
			 * @throw NoSuchElementException This iterator is done
		 	 * @see #currentRange
			 */
			virtual Style currentStyle() const = 0;
			/// Returns @c true if the iterator addresses the end of the range.
			virtual bool isDone() const BOOST_NOEXCEPT = 0;
			/**
			 * Moves the iterator to the next styled text run.
			 * @throw NoSuchElementException This iterator is done.
			 */
			virtual void next() = 0;
		};
		
		class DeclaredTextRunStyle;
		struct ComputedTextRunStyle;

		/**
		 * Specialization of @c StyledTextRunIterator class template which returns @c DeclaredTextRunStyle.
		 * @see TextRunStyleDeclarator
		 */
		struct DeclaredStyledTextRunIterator : StyledTextRunIterator<std::shared_ptr<const DeclaredTextRunStyle>> {};

		/**
		 * Specialization of @c StyledTextRunIterator class template which returns @c ComputedTextRunStyle.
		 * @see TextLayout#TextLayout
		 */
		struct ComputedStyledTextRunIterator : StyledTextRunIterator<boost::flyweight<ComputedTextRunStyle>> {};
	}
} // namespace ascension.presentation

#endif // !ASCENSION_STYLED_TEXT_RUN_ITERATOR_HPP
