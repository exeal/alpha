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
#include <ascension/corelib/text/detail/break-iterator-facade.hpp>
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

		template<typename Derived, typename Base, typename BaseIterator>
		class BreakIteratorImpl : public Base, public detail::BreakIteratorFacade<Derived> {
		public:
			/// Returns the base iterator.
			BaseIterator& base() BOOST_NOEXCEPT {
				return Base::template base<BaseIterator>();
			}
			/// Returns the base iterator.
			const BaseIterator& base() const BOOST_NOEXCEPT {
				return Base::template base<const BaseIterator>();
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
	}
} // namespace ascension.text

#endif // !ASCENSION_BREAK_ITERATOR_HPP
