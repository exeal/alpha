/**
 * @file break-iterator.hpp
 * @author exeal
 * @date 2005-2011 was unicode.hpp
 * @date 2011-04-26 separated from unicode.hpp
 * @date 2011-2014
 */

#ifndef ASCENSION_BREAK_ITERATOR_HPP
#define ASCENSION_BREAK_ITERATOR_HPP

#include <ascension/corelib/text/character-iterator.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_traits.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <iterator>
#include <locale>

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
			/// Moves to the next boundary.
			virtual void next(std::ptrdiff_t amount) = 0;
		protected:
			BreakIterator(const std::locale& locale) BOOST_NOEXCEPT : locale_(locale) {}
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
				public boost::iterators::iterator_facade<ConcreteIterator, Char, boost::iterators::random_access_traversal_tag> {
			private:
				friend class boost::iterators::iterator_core_access;
				void advance(typename boost::iterators::iterator_difference<BreakIteratorFacade>::type n) {
					static_cast<ConcreteIterator*>(this)->next(n);
				}
				void decrement() {return advance(-1);}
				typename boost::iterators::iterator_reference<BreakIteratorFacade>::type dereference() const {
					return *static_cast<const ConcreteIterator*>(this)->tell();
				}
				typename boost::iterators::iterator_difference<BreakIteratorFacade>::type distance_to(const ConcreteIterator& other) const {
					return static_cast<const ConcreteIterator*>(this)->tell() - other.tell();
				}
				bool equal(const ConcreteIterator& other) const {return distance_to(other) == 0;}
				void increment() {return advance(+1);}
			};
		} // namespace detail

#define ASCENSION_DEFINE_BREAK_ITERATOR_BASE_METHODS()														\
	template<typename BaseIterator>																			\
	BaseIterator& base() {return boost::type_erasure::any_cast<BaseIterator&>(characterIterator_);}			\
	template<typename BaseIterator>																			\
	BaseIterator& base() const {return boost::type_erasure::any_cast<BaseIterator&>(characterIterator_);}	\
	bool isBoundary(const detail::CharacterIterator& at) const;												\
	void next(std::size_t n);																				\
	void previous(std::size_t n)

		template<typename Derived, typename Base, typename BaseIterator>
		class BreakIteratorImpl : public Base, public detail::BreakIteratorFacade<Derived> {
		public:
			/// Returns the base iterator.
			BaseIterator& base() BOOST_NOEXCEPT {
				return Base::base<BaseIterator>();
			}
			/// Returns the base iterator.
			const BaseIterator& base() const BOOST_NOEXCEPT {
				return Base::base<const BaseIterator>();
			}
			/**
			 * Checks the specified position is grapheme cluster boundary
			 * @tparam CharacterIterator The type of @a at, which should satisfy @c detail#CharacterIteratorConcepts
			 * @param at The position to check
			 * @return true if @a at is grapheme cluster boundary
			 */
			template<typename CharacterIterator>
			bool isBoundary(const CharacterIterator& at) const {
				return Base::isBoundary(detail::CharacterIterator(at));
			}
			/// @see BreakIterator#next
			void next(std::ptrdiff_t n) override {
				if(n > 0)
					Base::next(+n);
				else if(n < 0)
					Base::previous(-n);
			}

		protected:
			template<typename Argument1>
			BreakIteratorImpl(BaseIterator base,
				const Argument1& locale) : Base(base, locale) {}
			template<typename Argument1, typename Argument2>
			BreakIteratorImpl(BaseIterator base,
				const Argument1& locale, const Argument2& a2) : Base(base, locale, a2) {}
			template<typename Argument1, typename Argument2, typename Argument3>
			BreakIteratorImpl(BaseIterator base,
				const Argument1& locale, const Argument2& a2, const Argument3& a3) : Base(base, locale, a2, a3) {}
		};

		/// Base class of @c GraphemeBreakIterator.
		class GraphemeBreakIteratorBase : public BreakIterator {
		protected:
			template<typename CharacterIterator>
			explicit GraphemeBreakIteratorBase(
				const CharacterIterator& characterIterator, const std::locale& locale) BOOST_NOEXCEPT
				: BreakIterator(locale), characterIterator_(characterIterator) {}
			ASCENSION_DEFINE_BREAK_ITERATOR_BASE_METHODS();
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
			void next(std::ptrdiff_t amount) override;
			void setComponent(Component component);
		protected:
			template<typename CharacterIterator>
			WordBreakIteratorBase(
				const CharacterIterator& characterIterator, const std::locale& locale,
				Component component, const IdentifierSyntax& syntax) BOOST_NOEXCEPT
				: BreakIterator(locale), characterIterator_(characterIterator), component_(component), syntax_(syntax) {}
			ASCENSION_DEFINE_BREAK_ITERATOR_BASE_METHODS();
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
			ASCENSION_DEFINE_BREAK_ITERATOR_BASE_METHODS();
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

		/// Base class of @c LineBreakIterator.
		class LineBreakIteratorBase : public BreakIterator {
		public:
			void next(std::ptrdiff_t amount) override;
		protected:
			template<typename CharacterIterator>
			LineBreakIteratorBase(
				const CharacterIterator& characterIterator, const std::locale& locale) BOOST_NOEXCEPT
				: BreakIterator(locale), characterIterator_(characterIterator) {}
			ASCENSION_DEFINE_BREAK_ITERATOR_BASE_METHODS();
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

#undef ASCENSION_DEFINE_ABSTRACT_BASE_METHODS

	}
} // namespace ascension.text

#endif // !ASCENSION_BREAK_ITERATOR_HPP
