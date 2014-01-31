/**
 * @file break-iterator.hpp
 * @author exeal
 * @date 2005-2011 was unicode.hpp
 * @date 2011-04-26 separated from unicode.hpp
 * @date 2011-2014
 */

#ifndef ASCENSION_BREAK_ITERATOR_HPP
#define ASCENSION_BREAK_ITERATOR_HPP

#include <ascension/corelib/text/character-iterator.hpp>	// CharacterIterator
#include <iterator>
#include <locale>
#include <boost/iterator/iterator_facade.hpp>

#if ASCENSION_UNICODE_VERSION > 0x0510
#	error These class definitions and implementations are based on old version of Unicode.
#endif
/// Tracking revision number of UAX #14 ("Line Breaking Properties")
#define ASCENSION_UAX14_REVISION_NUMBER 19	// 2006-05-23
/// Tracking revision number of UAX #29 ("Text Boundary")
#define ASCENSION_UAX29_REVISION_NUMBER 11	// 2006-10-12

namespace ascension {
	namespace text {
		/**
		 * An abstract base class for concrete break iterator classes. Break iterators are used to
		 * find and enumerate the location of boundaries in text. These iterators are based on
		 * <a href="http://www.unicode.org/reports/tr29/">UAX #29: Text Boudaries</a>. Clients can
		 * use each concrete iterator class or abstract @c BreakIterator for their polymorphism.
		 *
		 * This class does not have an interface for standard C++ iterator.
		 */
		class BreakIterator {
		public:
			/// Destructor.
			virtual ~BreakIterator() BOOST_NOEXCEPT {}
			/// Returns the locale.
			const std::locale& locale() const BOOST_NOEXCEPT {return locale_;}
			/// Returns true if @a at addresses a boundary.
			virtual bool isBoundary(const CharacterIterator& at) const = 0;
			/// Moves to the next boundary.
			virtual void next(std::ptrdiff_t amount) = 0;
		protected:
			BreakIterator(const std::locale& lc) BOOST_NOEXCEPT : locale_(lc) {}
		private:
			const std::locale& locale_;
		};

		namespace detail {
			/**
			 * @internal Provides standard C++ iterator interface and facilities for the concrete
			 * iterator class.
			 * @tparam ConcreteIterator The concrete iterator
			 */
			template<typename ConcreteIterator>
			class BreakIteratorFacade :
				public boost::iterator_facade<ConcreteIterator, Char, boost::random_access_traversal_tag> {
			private:
				friend class boost::iterator_core_access;
				void advance(difference_type n) {static_cast<ConcreteIterator*>(this)->next(n);}
				void decrement() {return advance(-1);}
				reference dereference() const {
					return *static_cast<const ConcreteIterator*>(this)->tell();
				}
				difference_type distance_to(const ConcreteIterator& other) const {
					return static_cast<const ConcreteIterator*>(this)->tell() - other.tell();
				}
				bool equal(const ConcreteIterator& other) const {return distance_to(other) == 0;}
				void increment() {return advance(+1);}
			};
		} // namespace detail

		/// Base class of @c GraphemeBreakIterator.
		class AbstractGraphemeBreakIterator : public BreakIterator {
		public:
			bool isBoundary(const CharacterIterator& at) const;
			void next(std::ptrdiff_t amount);
		protected:
			AbstractGraphemeBreakIterator(const std::locale& lc) BOOST_NOEXCEPT;
			virtual CharacterIterator& characterIterator() BOOST_NOEXCEPT = 0;
			virtual const CharacterIterator& characterIterator() const BOOST_NOEXCEPT = 0;
		private:
			void doNext(std::ptrdiff_t amount);
			void doPrevious(std::ptrdiff_t amount);
		};

		/**
		 * @c GraphemeBreakIterator locates grapheme cluster (character) boundaries in text.
		 * @tparam BaseIterator
		 */
		template<class BaseIterator>
		class GraphemeBreakIterator : public AbstractGraphemeBreakIterator,
			public detail::BreakIteratorFacade<GraphemeBreakIterator<BaseIterator>> {
		public:
			/**
			 * Constructor.
			 * @param base The base iterator
			 * @param lc The locale
			 */
			GraphemeBreakIterator(
				BaseIterator base, const std::locale& lc = std::locale::classic())
				: AbstractGraphemeBreakIterator(lc), p_(base) {}
			/// Returns the base iterator.
			BaseIterator& base() BOOST_NOEXCEPT {return p_;}
			/// Returns the base iterator.
			const BaseIterator& base() const BOOST_NOEXCEPT {return p_;}
		private:
			CharacterIterator& characterIterator() BOOST_NOEXCEPT {
				return static_cast<CharacterIterator&>(p_);
			}
			const CharacterIterator& characterIterator() const BOOST_NOEXCEPT {
				return static_cast<const CharacterIterator&>(p_);
			}
			BaseIterator p_;
		};

		class IdentifierSyntax;

		/// Base class of @c WordBreakIterator.
		class AbstractWordBreakIterator : public BreakIterator {
		public:
			/**
			 * Components of segment to search word boundaries.
			 * These values specify which boundary the iterator scans.
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
			/// Returns the word component to search.
			AbstractWordBreakIterator::Component component() const BOOST_NOEXCEPT {return component_;}
			bool isBoundary(const CharacterIterator& at) const;
			void next(std::ptrdiff_t amount);
			void setComponent(Component component);
		protected:
			AbstractWordBreakIterator(Component component,
				const IdentifierSyntax& syntax, const std::locale& lc) BOOST_NOEXCEPT;
			virtual CharacterIterator& characterIterator() BOOST_NOEXCEPT = 0;
			virtual const CharacterIterator& characterIterator() const BOOST_NOEXCEPT = 0;
		private:
			void doNext(std::ptrdiff_t amount);
			void doPrevious(std::ptrdiff_t amount);
			Component component_;
			const IdentifierSyntax& syntax_;
		};

		/**
		 * @c WordBreakIterator locates word boundaries in text.
		 * @tparam BaseIterator
		 */
		template<class BaseIterator>
		class WordBreakIterator : public AbstractWordBreakIterator,
			public detail::BreakIteratorFacade<WordBreakIterator<BaseIterator>> {
		public:
			/**
			 * Constructor.
			 * @param base The base iterator
			 * @param component The component of word to search
			 * @param syntax The identifier syntax for detecting identifier characters
			 * @param lc The locale
			 */
			WordBreakIterator(BaseIterator base, Component component,
				const IdentifierSyntax& syntax, const std::locale& lc = std::locale::classic())
				: AbstractWordBreakIterator(component, syntax, lc), p_(base) {}
			/// Returns the base iterator.
			BaseIterator& base() BOOST_NOEXCEPT {return p_;}
			/// Returns the base iterator.
			const BaseIterator& base() const BOOST_NOEXCEPT {return p_;}
		private:
			CharacterIterator& characterIterator() BOOST_NOEXCEPT {
				return static_cast<CharacterIterator&>(p_);
			}
			const CharacterIterator& characterIterator() const BOOST_NOEXCEPT {
				return static_cast<const CharacterIterator&>(p_);
			}
			BaseIterator p_;
		};

		/// Base class of @c SentenceBreakIterator.
		class AbstractSentenceBreakIterator : public BreakIterator {
		public:
			/**
			 * Components of segment to search word boundaries.
			 * These values specify which boundary the iterator scans.
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
			/// Returns the sentence component to search.
			AbstractSentenceBreakIterator::Component component() const BOOST_NOEXCEPT {return component_;}
			bool isBoundary(const CharacterIterator& at) const;
			void next(std::ptrdiff_t amount);
			void setComponent(Component component);
		protected:
			AbstractSentenceBreakIterator(Component component,
				const IdentifierSyntax& syntax, const std::locale& lc) BOOST_NOEXCEPT;
			virtual CharacterIterator& characterIterator() BOOST_NOEXCEPT = 0;
			virtual const CharacterIterator& characterIterator() const BOOST_NOEXCEPT = 0;
		private:
			void doNext(std::ptrdiff_t amount);
			void doPrevious(std::ptrdiff_t amount);
			Component component_;
			const IdentifierSyntax& syntax_;
		};

		/**
		 * @c SentenceBreakIterator locates sentence boundaries in text.
		 * @tparam BaseIterator
		 */
		template<class BaseIterator>
		class SentenceBreakIterator : public AbstractSentenceBreakIterator,
			public detail::BreakIteratorFacade<SentenceBreakIterator<BaseIterator>> {
		public:
			/**
			 * Constructor.
			 * @param base The base iterator
			 * @param component The component of sentence to search
			 * @param syntax The identifier syntax to detect alphabets
			 * @param lc The locale
			 */
			SentenceBreakIterator(BaseIterator base, Component component,
				const IdentifierSyntax& syntax, const std::locale& lc = std::locale::classic())
				: AbstractSentenceBreakIterator(component, syntax, lc), p_(base) {}
			/// Returns the base iterator.
			BaseIterator& base() BOOST_NOEXCEPT {return p_;}
			/// Returns the base iterator.
			const BaseIterator& base() const BOOST_NOEXCEPT {return p_;}
		private:
			CharacterIterator& characterIterator() BOOST_NOEXCEPT {
				return static_cast<CharacterIterator&>(p_);
			}
			const CharacterIterator& characterIterator() const BOOST_NOEXCEPT {
				return static_cast<const CharacterIterator&>(p_);
			}
			BaseIterator p_;
		};

		/// Base class of @c LineBreakIterator.
		class AbstractLineBreakIterator : public BreakIterator {
		public:
			bool isBoundary(const CharacterIterator& at) const;
			void next(std::ptrdiff_t amount);
		protected:
			AbstractLineBreakIterator(const std::locale& lc) BOOST_NOEXCEPT;
			virtual CharacterIterator& characterIterator() BOOST_NOEXCEPT = 0;
			virtual const CharacterIterator& characterIterator() const BOOST_NOEXCEPT = 0;
		};

		/**
		 * @c LineBreakIterator locates line break opportunities in text.
		 * @tparam BaseIterator
		 */
		template<class BaseIterator>
		class LineBreakIterator : public AbstractLineBreakIterator,
			public detail::BreakIteratorFacade<LineBreakIterator<BaseIterator>> {
		public:
			/**
			 * Constructor.
			 * @param base The base iterator
			 * @param lc The locale
			 */
			LineBreakIterator(BaseIterator base,
				const std::locale& lc = std::locale::classic())
				: AbstractLineBreakIterator(lc), p_(base) {}
			/// Returns the base iterator.
			BaseIterator& base() BOOST_NOEXCEPT {return p_;}
			/// Returns the base iterator.
			const BaseIterator& base() const BOOST_NOEXCEPT {return p_;}
		private:
			CharacterIterator& characterIterator() BOOST_NOEXCEPT {
				return static_cast<CharacterIterator&>(p_);
			}
			const CharacterIterator& characterIterator() const BOOST_NOEXCEPT {
				return static_cast<const CharacterIterator&>(p_);
			}
			BaseIterator p_;
		};
	}
} // namespace ascension.text

#endif // !ASCENSION_BREAK_ITERATOR_HPP
