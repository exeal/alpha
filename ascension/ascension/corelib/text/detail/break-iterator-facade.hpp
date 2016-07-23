/**
 * @file break-iterator-facade.hpp
 * @author exeal
 * @date 2005-2011 Was unicode.hpp
 * @date 2011-04-26 Separated from unicode.hpp.
 * @date 2016-07-24 Separated from break-iterator.hpp.
 */

#ifndef ASCENSION_BREAK_ITERATOR_FACADE_HPP
#define ASCENSION_BREAK_ITERATOR_FACADE_HPP
#include <boost/iterator/iterator_facade.hpp>
#include <boost/type_erasure/any_cast.hpp>

namespace ascension {
	namespace text {
		namespace detail {
			/**
			 * @internal Provides standard C++ iterator interface and facilities for the concrete iterator class.
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
				void decrement() {
					return advance(-1);
				}
				typename boost::iterators::iterator_reference<BreakIteratorFacade>::type dereference() const {
					return *static_cast<const ConcreteIterator*>(this)->tell();
				}
				typename boost::iterators::iterator_difference<BreakIteratorFacade>::type distance_to(const ConcreteIterator& other) const {
					return static_cast<const ConcreteIterator*>(this)->tell() - other.tell();
				}
				bool equal(const ConcreteIterator& other) const {
					return distance_to(other) == 0;
				}
				void increment() {
					return advance(+1);
				}
			};
		} // namespace detail

#define ASCENSION_DETAIL_DEFINE_BREAK_ITERATOR_BASE_METHODS()												\
	template<typename BaseIterator>																			\
	BaseIterator& base() {return boost::type_erasure::any_cast<BaseIterator&>(characterIterator_);}			\
	template<typename BaseIterator>																			\
	BaseIterator& base() const {return boost::type_erasure::any_cast<BaseIterator&>(characterIterator_);}	\
	bool isBoundary(const detail::CharacterIterator& at) const;												\
	void next(std::size_t n);																				\
	void previous(std::size_t n)

#undef ASCENSION_DEFINE_ABSTRACT_BASE_METHODS

	}
} // namespace ascension.text

#endif // !ASCENSION_BREAK_ITERATOR_FACADE_HPP
