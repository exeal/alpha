/**
 * @file grapheme-break-iterator.hpp
 * Defines @c GraphemeBreakIterator class.
 * @author exeal
 * @date 2005-2011 Was unicode.hpp
 * @date 2011-04-26 Separated from unicode.hpp.
 * @date 2016-07-24 Separated from break-iterator.hpp.
 */

#ifndef ASCENSION_GRAPHEME_BREAK_ITERATOR_HPP
#define ASCENSION_GRAPHEME_BREAK_ITERATOR_HPP
#include <ascension/corelib/text/break-iterator.hpp>

namespace ascension {
	namespace text {
		/// Base class of @c GraphemeBreakIterator.
		class GraphemeBreakIteratorBase : public BreakIterator {
		protected:
			template<typename CharacterIterator>
			explicit GraphemeBreakIteratorBase(
				const CharacterIterator& characterIterator, const std::locale& locale) BOOST_NOEXCEPT
				: BreakIterator(locale), characterIterator_(characterIterator) {}
			ASCENSION_DETAIL_DEFINE_BREAK_ITERATOR_BASE_METHODS();

		private:
			detail::CharacterIterator characterIterator_;
		};

		/**
		 * @c GraphemeBreakIterator locates grapheme cluster (character) boundaries in text.
		 * @tparam BaseIterator
		 */
		template<class BaseIterator>
		class GraphemeBreakIterator : public BreakIteratorImpl<
			GraphemeBreakIterator<BaseIterator>, GraphemeBreakIteratorBase, BaseIterator> {
		public:
			/**
			 * Constructor.
			 * @param base The base iterator
			 * @param locale The locale
			 */
			GraphemeBreakIterator(
				BaseIterator base, const std::locale& locale = std::locale::classic())
				: BreakIteratorImpl(base, locale) {}
		};
	}
} // namespace ascension.text

#endif // !ASCENSION_GRAPHEME_BREAK_ITERATOR_HPP
