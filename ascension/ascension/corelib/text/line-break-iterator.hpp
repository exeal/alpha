/**
 * @file line-break-iterator.hpp
 * Defines @c LineBreakIterator class.
 * @author exeal
 * @date 2005-2011 Was unicode.hpp
 * @date 2011-04-26 Separated from unicode.hpp.
 * @date 2016-07-24 Separated from break-iterator.hpp.
 */

#ifndef ASCENSION_LINE_BREAK_ITERATOR_HPP
#define ASCENSION_LINE_BREAK_ITERATOR_HPP
#include <ascension/corelib/text/break-iterator.hpp>

namespace ascension {
	namespace text {
		/// Base class of @c LineBreakIterator.
		class LineBreakIteratorBase : public BreakIterator {
		public:
			void next(std::ptrdiff_t amount) override;

		protected:
			template<typename CharacterIterator>
			LineBreakIteratorBase(
				const CharacterIterator& characterIterator, const std::locale& locale) BOOST_NOEXCEPT
				: BreakIterator(locale), characterIterator_(characterIterator) {}
			ASCENSION_DETAIL_DEFINE_BREAK_ITERATOR_BASE_METHODS();

		private:
			detail::CharacterIterator characterIterator_;
		};

		/**
		 * @c LineBreakIterator locates line break opportunities in text.
		 * @tparam BaseIterator
		 */
		template<class BaseIterator>
		class LineBreakIterator : public BreakIteratorImpl<
			LineBreakIterator<BaseIterator>, LineBreakIteratorBase, BaseIterator> {
		public:
			/**
			 * Constructor.
			 * @param base The base iterator
			 * @param locale The locale
			 */
			LineBreakIterator(
				BaseIterator base, const std::locale& locale = std::locale::classic())
				: BreakIteratorImpl(base, locale) {}
		};
	}
} // namespace ascension.text

#endif // !ASCENSION_LINE_BREAK_ITERATOR_HPP
