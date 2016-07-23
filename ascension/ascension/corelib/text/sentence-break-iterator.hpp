/**
 * @file sentence-break-iterator.hpp
 * Defines @c SentenceBreakIterator class.
 * @author exeal
 * @date 2005-2011 was unicode.hpp
 * @date 2011-04-26 Separated from unicode.hpp.
 * @date 2016-07-24 Separated from break-iterator.hpp.
 */

#ifndef ASCENSION_SENTENCE_BREAK_ITERATOR_HPP
#define ASCENSION_SENTENCE_BREAK_ITERATOR_HPP
#include <ascension/corelib/text/break-iterator.hpp>

namespace ascension {
	namespace text {
		class IdentifierSyntax;

		/// Base class of @c SentenceBreakIterator.
		class SentenceBreakIteratorBase : public BreakIterator {
		public:
			/**
			 * Components of segment to search word boundaries. These values specify which boundary the iterator scans.
			 * @see WordBreakIterator
			 */
			enum Component {
				/// Breaks at each starts of segments.
				START_OF_SEGMENT	= 0x01,
				/// Breaks at each ends of segments.
				END_OF_SEGMENT		= 0x02,
				/// Breaks at each starts and ends of segments.
				BOUNDARY_OF_SEGMENT	= START_OF_SEGMENT | END_OF_SEGMENT,
			};
		public:
			/// Returns the sentence component to search.
			Component component() const BOOST_NOEXCEPT {return component_;}
			void next(std::ptrdiff_t amount) override;
			void setComponent(Component component);

		protected:
			template<typename CharacterIterator>
			SentenceBreakIteratorBase(
				const CharacterIterator& characterIterator, const std::locale& locale,
				Component component, const IdentifierSyntax& syntax) BOOST_NOEXCEPT
				: characterIterator_(characterIterator), component_(component), syntax_(syntax) {}
			ASCENSION_DETAIL_DEFINE_BREAK_ITERATOR_BASE_METHODS();

		private:
			detail::CharacterIterator characterIterator_;
			Component component_;
			const IdentifierSyntax& syntax_;
		};

		/**
		 * @c SentenceBreakIterator locates sentence boundaries in text.
		 * @tparam BaseIterator
		 */
		template<class BaseIterator>
		class SentenceBreakIterator : public BreakIteratorImpl<
			SentenceBreakIterator<BaseIterator>, SentenceBreakIteratorBase, BaseIterator> {
		public:
			/**
			 * Constructor.
			 * @param base The base iterator
			 * @param component The component of sentence to search
			 * @param syntax The identifier syntax to detect alphabets
			 * @param locale The locale
			 */
			SentenceBreakIterator(
				BaseIterator base, SentenceBreakIteratorBase::Component component,
				const IdentifierSyntax& syntax, const std::locale& locale = std::locale::classic())
				: BreakIteratorImpl(base, locale, component, syntax) {}
		};
	}
} // namespace ascension.text

#endif // !ASCENSION_SENTENCE_BREAK_ITERATOR_HPP
