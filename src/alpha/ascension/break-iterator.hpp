/**
 * @file break-iterator.hpp
 * Defines iterator classes find and enumerate the location of boundaries in text.
 * These iterators are based on "UAX #29 : Text Boudaries" (http://www.unicode.org/reports/tr29/).
 * Clients can use each concrete iterator class or abstract @c BreakIterator for their polymorphism.
 * @date 2006-2007 (was iterator.hpp)
 * @date 2007
 * @author exeal
 */

#ifndef ASCENSION_BREAK_ITERATOR_HPP
#define ASCENSION_BREAK_ITERATOR_HPP
#include "unicode-utils.hpp"
#include <locale>	// std::locale

/// Tracking revision number of UAX #14 ("Line Breaking Properties")
#define ASCENSION_UAX14_REVISION_NUMBER	17	// 2005-08-29
/// Tracking revision number of UAX #29 ("Text Boundary")
#define ASCENSION_UAX29_REVISION_NUMBER	11	// 2006-10-12

namespace ascension {
	namespace unicode {

		/**
		 * An abstract base class for concrete iterator classes.
		 * This class does not have an interface for standard C++ iterator.
		 */
		class BreakIterator {
		public:
			/// Destructor.
			virtual ~BreakIterator() throw() {}
			/// Returns the locale.
			const std::locale& getLocale() const throw() {return locale_;}
			/// Returns true if @p at addresses a boundary.
			virtual bool isBoundary(const CharacterIterator& at) const = 0;
			/// Moves to the next boundary.
			virtual void next(std::ptrdiff_t amount) = 0;
		protected:
			BreakIterator(const std::locale& lc) throw() : locale_(lc) {}
		private:
			const std::locale& locale_;
		};

		namespace internal {
			/**
			 * Provides standard C++ iterator interface and facilities for the concrete iterator class.
			 * @param ConcreteIterator the concrete iterator
			 */
			template<class ConcreteIterator> class BreakIteratorFacade : public std::iterator<std::random_access_iterator_tag, Char> {
			public:
				reference operator*() const {return *tell();}
				reference operator[](difference_type index) const {return tell()[index];}
				ConcreteIterator& operator++() {getConcrete().next(+1); return getConcrete();}
				const ConcreteIterator operator++(int) {ConcreteIterator temp(getConcrete()); ++*this; return temp;}
				ConcreteIterator& operator--() {getConcrete().next(-1); return getConcrete();}
				const ConcreteIterator operator--(int) {ConcreteIterator temp(getConcrete()); --*this; return temp;}
				ConcreteIterator& operator+=(difference_type offset) {getConcrete().next(+offset); return getConcrete();}
				ConcreteIterator& operator-=(difference_type offset) {getConcrete().next(-offset); return getConcrete();}
				const ConcreteIterator operator+(difference_type offset) {ConcreteIterator temp(*this); return temp += offset;}
				const ConcreteIterator operator-(difference_type offset) {ConcreteIterator temp(*this); return temp -= offset;}
				bool operator==(const ConcreteIterator& rhs) const {return tell() == rhs.tell();}
				bool operator!=(const ConcreteIterator& rhs) const {return tell() != rhs.tell();}
				bool operator<(const ConcreteIterator& rhs) const {return tell() < rhs.tell();}
				bool operator<=(const ConcreteIterator& rhs) const {return tell() <= rhs.tell();}
				bool operator>(const ConcreteIterator& rhs) const {return tell() > rhs.tell();}
				bool operator>=(const ConcreteIterator& rhs) const {return tell() >= rhs.tell();}
			private:
				ConcreteIterator& getConcrete() {return *static_cast<ConcreteIterator*>(this);}
			};
		} // namespace internal

		/// Base class of @c GraphemeBreakIterator.
		class AbstractGraphemeBreakIterator : public BreakIterator {
		public:
			bool	isBoundary(const CharacterIterator& at) const;
			void	next(std::ptrdiff_t amount);
		protected:
			AbstractGraphemeBreakIterator(const std::locale& lc) throw();
			virtual CharacterIterator& getCharacterIterator() throw() = 0;
			virtual const CharacterIterator& getCharacterIterator() const throw() = 0;
		private:
			void	doNext(std::ptrdiff_t amount);
			void	doPrevious(std::ptrdiff_t amount);
		};

		/// @c GraphemeBreakIterator locates grapheme cluster (character) boundaries in text.
		template<class BaseIterator>
		class GraphemeBreakIterator : public AbstractGraphemeBreakIterator, public internal::BreakIteratorFacade<GraphemeBreakIterator> {
		public:
			/**
			 * Constructor.
			 * @param base the base iterator
			 * @param component the component of grapheme cluster to search
			 * @param lc the locale
			 */
			GraphemeBreakIterator(BaseIterator base,
				const std::locale& lc = std::locale::classic()) : AbstractGraphemeBreakIterator(lc), p_(base) {}
			/// Returns the base iterator.
			BaseIterator& base() throw() {return p_;}
			/// Returns the base iterator.
			const BaseIterator& base() const throw() {return p_;}
		private:
			CharacterIterator& getCharacterIterator() {return static_cast<CharacterIterator&>(p_);}
			const CharacterIterator& getCharacterIterator() const {return static_cast<const CharacterIterator&>(p_);}
			BaseIterator p_;
		};

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
			Component	getComponent() const throw();
			bool		isBoundary(const CharacterIterator& at) const;
			void		next(std::ptrdiff_t amount);
			void		setComponent(Component component) throw();
		protected:
			AbstractWordBreakIterator(Component component, const CharacterDetector& ctypes, const std::locale& lc) throw();
			virtual CharacterIterator& getCharacterIterator() throw() = 0;
			virtual const CharacterIterator& getCharacterIterator() const throw() = 0;
		private:
			void	doNext(std::ptrdiff_t amount);
			void	doPrevious(std::ptrdiff_t amount);
			Component component_;
			const CharacterDetector& ctypes_;
		};

		/// @c WordBreakIterator locates word boundaries in text.
		template<class BaseIterator>
		class WordBreakIterator : public AbstractWordBreakIterator, public internal::BreakIteratorFacade<WordBreakIterator> {
		public:
			/**
			 * Constructor.
			 * @param base the base iterator
			 * @param component the component of word to search
			 * @param ctypes the character detector to detect alphabets
			 * @param lc the locale
			 */
			WordBreakIterator(BaseIterator base, Component component, const CharacterDetector& ctypes,
				const std::locale& lc = std::locale::classic()) : AbstractWordBreakIterator(component, ctypes, lc), p_(base) {}
			/// Returns the base iterator.
			BaseIterator& base() throw() {return p_;}
			/// Returns the base iterator.
			const BaseIterator& base() const throw() {return p_;}
		private:
			CharacterIterator& getCharacterIterator() {return static_cast<CharacterIterator&>(p_);}
			const CharacterIterator& getCharacterIterator() const {return static_cast<const CharacterIterator&>(p_);}
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
			Component	getComponent() const throw();
			bool		isBoundary(const CharacterIterator& at) const;
			void		next(std::ptrdiff_t amount);
			void		setComponent(Component component) throw();
		protected:
			AbstractSentenceBreakIterator(Component component, const CharacterDetector& ctypes, const std::locale& lc) throw();
			virtual CharacterIterator& getCharacterIterator() throw() = 0;
			virtual const CharacterIterator& getCharacterIterator() const throw() = 0;
		private:
			void	doNext(std::ptrdiff_t amount);
			void	doPrevious(std::ptrdiff_t amount);
			Component component_;
			const CharacterDetector& ctypes_;
		};

		/// @c SentenceBreakIterator locates sentence boundaries in text.
		template<class BaseIterator>
		class SentenceBreakIterator : public AbstractSentenceBreakIterator, public internal::BreakIteratorFacade<SentenceBreakIterator> {
		public:
			/**
			 * Constructor.
			 * @param base the base iterator
			 * @param component the component of sentence to search
			 * @param ctypes the character detector to detect alphabets
			 * @param lc the locale
			 */
			SentenceBreakIterator(BaseIterator base, Component component, const CharacterDetector& ctypes,
				const std::locale& lc = std::locale::classic()) : AbstractSentenceBreakIterator(component, ctypes, lc), p_(base) {}
			/// Returns the base iterator.
			BaseIterator& base() throw() {return p_;}
			/// Returns the base iterator.
			const BaseIterator& base() const throw() {return p_;}
		private:
			CharacterIterator& getCharacterIterator() {return static_cast<CharacterIterator&>(p_);}
			const CharacterIterator& getCharacterIterator() const {return static_cast<const CharacterIterator&>(p_);}
			BaseIterator p_;
		};


		/// Returns the word component to search.
		inline AbstractWordBreakIterator::Component AbstractWordBreakIterator::getComponent() const throw() {return component_;}

		/// Sets the word component to search.
		inline void AbstractWordBreakIterator::setComponent(Component component) throw() {component_ = component;}

		/// Returns the sentence component to search.
		inline AbstractSentenceBreakIterator::Component AbstractSentenceBreakIterator::getComponent() const throw() {return component_;}

		/// Sets the sentence component to search.
		inline void AbstractSentenceBreakIterator::setComponent(Component component) throw() {component_ = component;}

	}
}

#endif /* !ASCENSION_BREAK_ITERATOR_HPP */
