/**
 * @file word-break-iterator.hpp
 * Defines @c WordBreakIterator class.
 * @author exeal
 * @date 2005-2011 Was unicode.hpp
 * @date 2011-04-26 Separated from unicode.hpp.
 * @date 2016-07-24 Separated from break-iterator.hpp.
 */

#ifndef ASCENSION_WORD_BREAK_ITERATOR_HPP
#define ASCENSION_WORD_BREAK_ITERATOR_HPP
#include <ascension/corelib/text/break-iterator.hpp>

namespace ascension {
	namespace text {
		class IdentifierSyntax;

		/// Base class of @c WordBreakIterator.
		class WordBreakIteratorBase : public BreakIterator {
		public:
			/**
			 * Components of segment to search word boundaries. These values specify which boundary the iterator scans.
			 * @see WordBreakIterator
			 */
			enum Component {
				/// Breaks at each starts of segments.
				START_OF_SEGMENT			= 0x01,
				/// Breaks at each ends of segments.
				END_OF_SEGMENT				= 0x02,
				/// Breaks at each starts and ends of segments.
				BOUNDARY_OF_SEGMENT			= START_OF_SEGMENT | END_OF_SEGMENT,
				/// Only words consist of alpha-numerics.
				ALPHA_NUMERIC				= 0x04,
				/// Start of word consists of alpha-numerics.
				START_OF_ALPHANUMERICS		= START_OF_SEGMENT | ALPHA_NUMERIC,
				/// End of word consists of alpha-numerics.
				END_OF_ALPHANUMERICS		= END_OF_SEGMENT | ALPHA_NUMERIC,
				/// Start or end of word consists of alpha-numerics.
				BOUNDARY_OF_ALPHANUMERICS	= BOUNDARY_OF_SEGMENT | ALPHA_NUMERIC
			};
		public:
			/// Returns the word component to search.
			Component component() const BOOST_NOEXCEPT {return component_;}
			void setComponent(Component component);

		protected:
			template<typename CharacterIterator>
			WordBreakIteratorBase(
				const CharacterIterator& characterIterator, const std::locale& locale,
				Component component, const IdentifierSyntax& syntax) BOOST_NOEXCEPT
				: BreakIterator(locale), characterIterator_(characterIterator), component_(component), syntax_(syntax) {}
			ASCENSION_DETAIL_DEFINE_BREAK_ITERATOR_BASE_METHODS();

		private:
			detail::CharacterIterator characterIterator_;
			Component component_;
			const IdentifierSyntax& syntax_;
		};

		/**
		 * @c WordBreakIterator locates word boundaries in text.
		 * @tparam BaseIterator
		 */
		template<class BaseIterator>
		class WordBreakIterator : public BreakIteratorImpl<
			WordBreakIterator<BaseIterator>, WordBreakIteratorBase, BaseIterator> {
		public:
			/**
			 * Constructor.
			 * @param base The base iterator
			 * @param component The component of word to search
			 * @param syntax The identifier syntax for detecting identifier characters
			 * @param locale The locale
			 */
			WordBreakIterator(
				BaseIterator base, WordBreakIteratorBase::Component component,
				const IdentifierSyntax& syntax, const std::locale& locale = std::locale::classic())
				: BreakIteratorImpl(base, locale, component, syntax) {}
		};
	}
} // namespace ascension.text

#endif // !ASCENSION_WORD_BREAK_ITERATOR_HPP
