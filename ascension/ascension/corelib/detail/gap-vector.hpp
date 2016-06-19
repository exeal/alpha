/**
 * @file gap-vector.hpp
 * @author exeal
 * @date 2005-2009 (was gap-buffer.hpp)
 * @date 2010-10-20 Renamed GapBuffer to GapVector.
 * @date 2011-2015
 */

#ifndef ASCENSION_GAP_VECTOR_HPP
#define ASCENSION_GAP_VECTOR_HPP
#ifndef ASCENSION_GAP_VECTOR_INITIAL_SIZE
#	define ASCENSION_GAP_VECTOR_INITIAL_SIZE 10
#endif // !ASCENSION_GAP_VECTOR_INITIAL_SIZE
#include <algorithm>	// std.fill_n, std.max, std.swap
#include <cassert>
#include <cstring>		// std.memmove
#include <initializer_list>
#include <iterator>		// std.iterator, ...
#include <memory>		// std.allocator, std.uninitialized_copy
#include <stdexcept>	// std.length_error, std.out_of_range
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_traits.hpp>
#include <boost/operators.hpp>

namespace ascension {
	namespace detail {

		template<typename InputIterator, typename ForwardIterator, typename Allocator>
		inline ForwardIterator uninitializedCopy(
				InputIterator first, InputIterator last,
				ForwardIterator destination, Allocator& allocator) {
			ForwardIterator i(destination);
			try {
				for(; first != last; ++first, ++i)
					allocator.construct(&*i, *first);
			} catch(...) {
				for(; i != destination; ++i)
					allocator.destroy(&*i);
				throw;
			}
		}

		template<typename InputIterator, typename ForwardIterator, typename T>
		inline ForwardIterator uninitializedCopy(
				InputIterator first, InputIterator last,
				ForwardIterator destination, std::allocator<T>&) {
			return std::uninitialized_copy(first, last, destination);
		}

		template<typename Target, typename Pointer, typename Reference>
		class GapVectorIterator : public boost::iterators::iterator_facade<
			GapVectorIterator<Target, Pointer, Reference>, typename Target::value_type,
			boost::iterators::random_access_traversal_tag, Reference, typename Target::difference_type> {
		public:
			typedef GapVectorIterator<Target, Pointer, Reference> Self;
			GapVectorIterator() BOOST_NOEXCEPT : target_(nullptr), p_(nullptr) {}
			GapVectorIterator(const Target& target, Pointer position) BOOST_NOEXCEPT : target_(&target), p_(position) {
				assert(p_ != nullptr);
				if(p_ == target.gapFirst_)
					p_ = target.gapLast_;
				assert(validate(false));
			}
			template<typename Pointer2, typename Reference2>
			GapVectorIterator(const GapVectorIterator<Target, Pointer2, Reference2>& other)
					: target_(other.target()), p_(std::next(target_->first_, other.p_ - other.target()->first_)) {}
			Self& operator=(const Self& other) {
				target_ = other.target_;
				p_ = other.p_;
				return *this;
			}
			Pointer after() const BOOST_NOEXCEPT {
				assert(validate(true));
				return (p_ == target()->gapFirst_) ? target()->gapLast_ : p_;
			}
			Pointer before() const BOOST_NOEXCEPT {
				assert(validate(true));
				return (p_ == target()->gapLast_) ? target()->gapFirst_ : p_;
			}
			BOOST_CONSTEXPR const Target* target() const BOOST_NOEXCEPT {
				return target_;
			}

		private:
			template<typename Target2, typename Pointer2, typename Reference2> friend class GapVectorIterator;
			BOOST_CONSTEXPR typename boost::iterators::iterator_difference<Self>::type offset() const BOOST_NOEXCEPT {
				return (p_ < target()->gapLast_) ? (p_ - target()->first_) : ((p_ - target()->first_) - target()->gap());
			}
			BOOST_CONSTEXPR bool validate(bool allowGapFirst) const BOOST_NOEXCEPT {
				auto gf(target()->gapFirst_);
				if(allowGapFirst)
					++gf;
				return (p_ >= target()->gapLast_ && p_ <= target()->last_) || (p_ >= target()->first_ && p_ < gf);
			}
			// boost.iterators.iterator_facade
			friend class boost::iterators::iterator_core_access;
			void advance(typename boost::iterators::iterator_difference<Self>::type n) {
				const auto nextOpportunity(std::next(p_, n));
				if(nextOpportunity >= target()->gapFirst_ && nextOpportunity < target()->gapLast_)
					std::advance(p_, n += target()->gap());
				else
					p_ = nextOpportunity;
			}
			void decrement() BOOST_NOEXCEPT {
				if(p_ != target()->gapLast_)
					--p_;
				else
					p_ = std::prev(target()->gapFirst_);
			}
			BOOST_CONSTEXPR typename boost::iterators::iterator_reference<Self>::type dereference() const BOOST_NOEXCEPT {
				return *p_;
			}
			BOOST_CONSTEXPR typename boost::iterators::iterator_difference<Self>::type distance_to(const GapVectorIterator& other) const BOOST_NOEXCEPT {
				return other.offset() - offset();
			}
			BOOST_CONSTEXPR bool equal(const GapVectorIterator& other) const BOOST_NOEXCEPT {
				return target() == other.target() && offset() == other.offset();
			}
			void increment() BOOST_NOEXCEPT {
				assert(p_ != target()->gapFirst_);
				if(std::next(p_) != target()->gapFirst_)
					++p_;
				else
					p_ = target()->gapLast_;
			}
		private:
			const Target* target_;
			Pointer p_;
		};

		/**
		 * Implements "gap buffer" data structure.
		 * @tparam T The element type. Should be primitive
		 * @tparam Allocator The allocator to use for all memory allocations of this container
		 * @note This class is not intended to be derived.
		 * @todo Rewrite with std.allocator_traits.
		 */
		template<typename T, typename Allocator = std::allocator<T>>
		class GapVector : boost::totally_ordered<GapVector<T, Allocator>> {
		public:
			/// A type represents the allocator class.
			typedef Allocator allocator_type;
			/// A type counts the number of elements.
			typedef typename Allocator::size_type size_type;
			/// A type provides the difference between two iterators that refer to elements.
			typedef typename Allocator::difference_type difference_type;
			/// A type represents the data type.
			typedef typename Allocator::value_type value_type;
			/// A type provides a pointer to an element.
			typedef typename Allocator::pointer pointer;
			/// A type provides a reference to an element.
			typedef typename Allocator::reference reference;
			/// A type provides a pointer to a const element.
			typedef typename Allocator::const_pointer const_pointer;
			/// A type provides a reference to a const element.
			typedef typename Allocator::const_reference const_reference;
			///
			typedef GapVectorIterator<GapVector, pointer, reference> iterator;
			///
			typedef GapVectorIterator<GapVector, const_pointer, const_reference> const_iterator;
			/// A type provides a random-access iterator can read or modify any element in the
			/// reversed content.
			typedef std::reverse_iterator<iterator> reverse_iterator;
			/// A type provides a random-access iterator can read any element in the content.
			typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

			/// @name Construct/Copy/Destroy
			/// @{
			/**
			 * Constructs an empty gap vector, using the specified allocator.
			 * @param allocator The allocator object
			 */
			explicit GapVector(const allocator_type& allocator = allocator_type()) :
				allocator_(allocator),
				first_(allocator_.allocate(ASCENSION_GAP_VECTOR_INITIAL_SIZE, 0)),
				last_(std::next(first_, ASCENSION_GAP_VECTOR_INITIAL_SIZE)),
				gapFirst_(first_), gapLast_(last_) {}

			/**
			 * Constructs a gap vector with @a n copies of value, using the specified allocator.
			 * @param n The number of elements in the constructed content
			 * @param value The value of elements in the constructed content
			 * @param allocator The allocator object
			 */
			GapVector(size_type n, const_reference value,
					const allocator_type& allocator = allocator_type()) : allocator_(allocator),
					first_(allocator_.allocate(n, 0)), last_(std::next(first_, n)),
					gapFirst_(first_), gapLast_(last_) {
				insert(0, n, value);
			}

			/**
			 * Constructs a gap vector equal to the range <code>[first,last)</code>, using the
			 * specified allocator.
			 * @tparam Inputiterator The input iterator
			 * @param first The first element in the range of elements to be copied
			 * @param last The last element in the range of elements to be copied
			 * @param allocator The allocator object
			 */
			template<typename Inputiterator>
			GapVector(Inputiterator first, Inputiterator last,
					const allocator_type& allocator = allocator_type()) : allocator_(allocator),
					first_(nullptr), last_(nullptr), gapFirst_(nullptr), gapLast_(nullptr) {
				insert(0, first, last);
			}

			/**
			 * Copy-constructor.
			 * @param other The another container to be used as source to initialize the elements
			 *              of the container with
			 */
			GapVector(const GapVector& other) : allocator_(other.allocator_),
					first_(allocator_.allocate(other.capacity())),
					last_(std::next(first_, other.capacity())),
					gapFirst_(std::next(first_, std::distance(other.first_, other.gapFirst_))),
					gapLast_(std::next(first_, std::distance(other.first_, other.gapLast_))) {
				try {
					uninitializedCopy(other.first_, other.gapFirst_, first_, allocator_);
					uninitializedCopy(other.gapLast_, other.last_, gapFirst_, allocator_);
				} catch(...) {
					allocator_.deallocate(first_, capacity());
				}
			}

			/**
			 * Copy-constructor.
			 * @param other The another container to be used as source to initialize the elements
			 *              of the container with
			 * @param allocator The allocator object
			 */
			GapVector(const GapVector& other, const allocator_type& allocator) :
					allocator_(allocator),
					first_(allocator_.allocate(other.capacity())),
					last_(std::next(first_, other.capacity())),
					gapFirst_(std::next(first_, std::distance(other.first_, other.gapFirst_))),
					gapLast_(std::next(first_, std::distance(other.first_, other.gapLast_))) {
				try {
					uninitializedCopy(other.first_, other.gapFirst_, first_, allocator);
					uninitializedCopy(other.gapLast_, other.last_, gapFirst_, allocator);
				} catch(...) {
					allocator.deallocate(first_, capacity());
				}
			}

			/**
			 * Move-constructor.
			 * @param other The another container to be used as source to initialize the elements
			 *              of the container with
			 */
			GapVector(GapVector&& other) : allocator_(std::move(other.allocator_)),
					first_(other.first_), last_(other.gapLast_),
					gapFirst_(other.gapFirst_), gapLast_(other.gapLast_) {
				other.first_ = other.last_ = other.gapFirst_ = other.gapLast_ = nullptr;
			}

			/**
			 * Move-constructor.
			 * @param other The another container to be used as source to initialize the elements
			 *              of the container with
			 * @param allocator The allocator object
			 */
			GapVector(GapVector&& other, const allocator_type& allocator) : allocator_(allocator_),
					first_(allocator_.allocate(other.capacity())),
					last_(std::next(first_, other.capacity())),
					gapFirst_(std::next(first_, std::distance(other.first_, other.gapFirst_))),
					gapLast_(std::next(first_, std::distance(other.first_, other.gapLast_))) {
				for(pointer p(first_); p != gapFirst_; ++p)
					*p = std::move(std::next(other.first_, std::distance(first_, p)));
				for(pointer p(gapLast_); p != last_; ++p)
					*p = std::move(std::next(other.first_, std::distance(first_, p)));
				other.first_ = other.last_ = other.gapFirst_ = other.gapLast_ = nullptr;
			}
#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
			/**
			 * Constructor with an initializer list.
			 * @param values The initializer list to initialize the elements of the container with
			 * @param allocator The allocator object
			 */
			GapVector(std::initializer_list<value_type> values, const allocator_type& allocator);
#endif	// !BOOST_NO_CXX11_HDR_INITIALIZER_LIST
			/// Destructor.
			~GapVector() {
				release();
			}

			/**
			 * Copy-assignment operator.
			 * @param other The another container to be used as source
			 * @return This gap vector
			 */
			GapVector<value_type, allocator_type>& operator=(const GapVector& other) {
				GapVector<value_type, allocator_type>(other).swap(*this);
				return *this;
			}

			/**
			 * Move-assignment operator.
			 * @param other The another container to be used as source
			 * @return This gap vector
			 */
			GapVector<value_type, allocator_type>& operator=(GapVector&& other) {
				GapVector<value_type, allocator_type>(std::forward(other)).swap(*this);
				return *this;
			}
#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
			GapVector& operator=(std::initializer_list<value_type> values);
#endif	// !BOOST_NO_CXX11_HDR_INITIALIZER_LIST
			/**
			 * Assigns a range of elements.
			 * @tparam InputIterator The input iterator type gives the range
			 * @param first The beginning of the range to copy the elements from
			 * @param last The end of the range to copy the elements from
			 */
			template<typename InputIterator>
			void assign(InputIterator first, InputIterator last) {
				GapVector<value_type, allocator_type> temp(first, last);
				*this = std::move(temp);
			}

			/**
			 * Assigns a number of elements.
			 * @param n The new size of the container
			 * @param value The value to initialize elements of the container with
			 */
			void assign(size_type n, const_reference value) {
				GapVector<value_type, allocator_type> temp(n, value);
				*this = std::move(temp);
			}
#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
			void assign(std::initializer_list<value_type> values);
#endif	// !BOOST_NO_CXX11_HDR_INITIALIZER_LIST

			/// Returns the allocator associated with the container.
			BOOST_CONSTEXPR allocator_type allocator() const {
				return allocator_;
			}
			/// @}


			/// @name Element Access
			/// @{
			/**
			 * Returns a reference to the element at a specified position.
			 * @param position The position of the element to return
			 * @return A reference to the requested element
			 * @throw std#out_of_range @a position is outside of the container
			 */
			reference at(size_type position) {
				if(position >= size())
					throw std::out_of_range("position");
				return operator[](position);
			}

			/**
			 * Returns a reference to the element at a specified position.
			 * @param position The position of the element to return
			 * @return A reference to the requested element
			 * @throw std#out_of_range @a position is outside of the container
			 */
			const_reference at(size_type position) const {
				if(position >= size())
					throw std::out_of_range("position");
				return operator[](position);
			}

			/**
			 * Returns a reference to the element at a specified position.
			 * @param position The position of the element to return
			 * @return A reference to the requested element
			 */
			reference operator[](size_type position) BOOST_NOEXCEPT {
				return first_[(std::next(first_, position) < gapFirst_) ? position : position + gap()];
			}

			/**
			 * Returns a reference to the element at a specified position.
			 * @param position The position of the element to return
			 * @return A reference to the requested element
			 */
			BOOST_CONSTEXPR const_reference operator[](size_type position) const BOOST_NOEXCEPT {
				return first_[(std::next(first_, position) < gapFirst_) ? position : position + gap()];
			}

			/// Returns a reference to the first element in the container.
			reference front() BOOST_NOEXCEPT {return (*this)[0];}

			/// Returns a reference to the first element in the container.
			BOOST_CONSTEXPR const_reference front() const BOOST_NOEXCEPT {return (*this)[0];}

			/// Returns a reference to the last element in the container.
			reference back() BOOST_NOEXCEPT {return (*this)[size() - 1];}

			/// Returns a const reference to the last element in the container.
			BOOST_CONSTEXPR const_reference back() const BOOST_NOEXCEPT {return (*this)[size() - 1];}
			/// @}

			/// @name Iterators
			/// @{
			/** Returns an iterator to the first element of the container. */
			iterator begin() BOOST_NOEXCEPT {return iterator(*this, first_);}

			/// Returns an iterator to the first element of the container.
			const_iterator begin() const BOOST_NOEXCEPT {return const_iterator(*this, first_);}

			/// Returns an iterator to the first element of the container.
			const_iterator cbegin() const BOOST_NOEXCEPT {return begin();}

			/// Returns an iterator to the element following the last element of the container.
			iterator end() BOOST_NOEXCEPT {return iterator(*this, last_);}

			/// Returns an iterator to the element following the last element of the container.
			const_iterator end() const BOOST_NOEXCEPT {return const_iterator(*this, last_);}

			/// Returns an iterator to the element following the last element of the container.
			const_iterator cend() const BOOST_NOEXCEPT {return end();}

			/// Returns a reverse iterator to the first element of the reversed container.
			reverse_iterator rbegin() BOOST_NOEXCEPT {return reverse_iterator(end());}

			/// Returns a reverse iterator to the first element of the reversed container.
			const_reverse_iterator rbegin() const BOOST_NOEXCEPT {return const_reverse_iterator(end());}

			/// Returns a reverse iterator to the first element of the reversed container.
			const_reverse_iterator crbegin() const BOOST_NOEXCEPT {return rbegin();}

			/// Returns a reverse iterator to the element following the last element of the
			/// reversed container.
			reverse_iterator rend() BOOST_NOEXCEPT {return reverse_iterator(begin());}

			/// Returns a reverse iterator to the element following the last element of the
			/// reversed container.
			const_reverse_iterator rend() const BOOST_NOEXCEPT {return const_reverse_iterator(begin());}

			/// Returns a reverse iterator to the element following the last element of the
			/// reversed container.
			const_reverse_iterator crend() const BOOST_NOEXCEPT {return rend();}

			/// @name Capacity
			/// @note @c GapVector does not provide @c resize methods.
			/// @{
			/** Returns @c true if the container is empty. */
			BOOST_CONSTEXPR bool empty() const BOOST_NOEXCEPT {return size() == 0;}

			/// Returns the number of elements in the container.
			BOOST_CONSTEXPR size_type size() const BOOST_NOEXCEPT {return capacity() - gap();}

			/// Returns the maximum size of the container.
			BOOST_CONSTEXPR size_type maxSize() const BOOST_NOEXCEPT {return allocator_.max_size();}

			/**
			 * Sets the capacity of the container to at least @a size.
			 * @param newCapacity The new capacity of the container
			 */
			void reserve(size_type newCapacity) {reallocate(newCapacity);}

			/**
			 * Resizes the container to contain @a count elements.
			 * @param size The new size of the container
			 */
			void resize(size_type size);

			/**
			 * Resizes the container to contain @a count elements.
			 * @param count The new size of the container
			 * @param value The value to initialize the new elements with
			 */
			void resize(size_type count, const_reference value);

			/// Returns the total number of elements that the gap vector can hold without requiring reallocation.
			BOOST_CONSTEXPR size_type capacity() const BOOST_NOEXCEPT {return std::distance(first_, last_);}

			/// Requests the removal of unused capacity.
			void shrinkToFit() {reallocate(size());}
			/// @}

			/// @name Modifiers
			/// @{

			/// Removes all elements from the container.
			void clear() {erase(begin(), end());}	// TODO: Follow std.vector.clear semantics.

			/**
			 * Inserts an element to the specified position in the container.
			 * @param position The element before which the content will be inserted
			 * @param value The element value to insert
			 */
			iterator insert(const_iterator position, const_reference value) {
				makeGapAt(position.before());
				*(gapFirst_++) = value;
				if(gapFirst_ == gapLast_)
					reallocate(capacity() * 2);
				return iterator(*this, gapFirst_);
			}

			/**
			 * Inserts an element to the specified position in the container.
			 * @param position The element before which the content will be inserted
			 * @param value The element value to insert
			 */
			iterator insert(const_iterator position, value_type&& value) {
				makeGapAt(position.before());
				*(gapFirst_++) = std::move(value);
				if(gapFirst_ == gapLast_)
					reallocate(capacity() * 2);
				return iterator(*this, gapFirst_);
			}

			/**
			 * Inserts elements to the specified position in the container.
			 * @param position The element before which the content will be inserted
			 * @param n The number of the elements to insert
			 * @param value The element value to insert
			 */
			iterator insert(const_iterator position, size_type n, const_reference value) {
				makeGapAt(std::next(first_, size()));
				makeGapAt(position.before());
				if(static_cast<size_type>(gap()) <= n)
					reallocate(std::max(capacity() + n + 1, capacity() * 2));
				std::fill_n(gapFirst_, n, value);
				const auto oldGapFirst(gapFirst);
				std::next(gapFirst_, n);
				return iterator(*this, oldGapFirst);
			}

			/**
			 * Inserts elements to the specified position in the container.
			 * @tparam Inputiterator The input iterator
			 * @param position The element before which the content will be inserted
			 * @param first The beginning of the range of elements to insert
			 * @param last The end of the range of elements to insert
			 */
			template<typename InputIterator>
			iterator insert(const_iterator position, InputIterator first, InputIterator last) {
				const difference_type n = std::distance(first, last);
				if(n == 0)
					return position;
				if(n > gap()) {
					const const_iterator::difference_type index = std::distance(cbegin(), position);
					reallocate(std::max(n + size(), capacity() * 2));
					position = std::next(cbegin(), index);
				}
//				makeGapAt(first_ + size());
				makeGapAt(position.before());
				const pointer p(const_cast<pointer>(position.before()));
				gapFirst_ = std::next(first_, std::distance(first_, uninitializedCopy(first, last, p, allocator_)));
				return iterator(*this, std::prev(gapFirst_, n));
			}
#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
			/**
			 * Inserts elements to the specified position in the container.
			 * @param position The element before which the content will be inserted
			 * @param values The initializer list to insert the values from
			 */
			iterator insert(const_iterator position, std::initializer_list<value_type> values);
#endif	// !BOOST_NO_CXX11_HDR_INITIALIZER_LIST
			/**
			 * Removes an element in this gap vector.
			 * @param position The position of the element to remove
			 * @return An iterator addresses the first element remaining beyond the removed element
			 */
			iterator erase(const_iterator position) {
				return erase(position, position + 1);
			}

			/**
			 * Removes the specified element from the container.
			 * @param first The beginning of the range of elements to remove
			 * @param last The end of the range of elements to remove
			 * @return An iterator following the last removed element
			 */
			iterator erase(const_iterator first, const_iterator last) {
				if(first == last)
					return first;
				assert(first < last);
				if(first.before() <= gapFirst_ && last.after() >= gapLast_) {
					const auto b(const_cast<pointer>(first.before())), e(const_cast<pointer>(last.after()));
					destroy(b, gapFirst_);
					destroy(gapLast_, e);
					gapFirst_ = b;
					return iterator(*this, gapLast_ = e);
				} else {
					const auto n = std::distance(first, last);
					makeGapAt(first.after());
					destroy(gapLast_, std::next(gapLast_));
					std::advance(gapLast_, n);
					return iterator(*this, gapLast_);
				}
			}

			void pushBack(const_reference value);
			void pushBack(value_type&& value);
#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES
			template<typename... Arguments> void emplace(const_iterator position, Arguments&& ...arguments);
			template<typename... Arguments> void emplaceBack(Arguments&& ...arguments);
#endif	// !BOOST_NO_CXX11_VARIADIC_TEMPLATES
			void popBack();

			/**
			 * Exchanges the elements of two gap vectors.
			 * @param other A gap vector whose elements to be exchanged
			 */
			void swap(GapVector& other) BOOST_NOEXCEPT {
				using std::swap;
				swap(allocator_, other.allocator_);
				swap(first_, other.first_);
				swap(last_, other.last_);
				swap(gapFirst_, other.gapFirst_);
				swap(gapLast_, other.gapLast_);
			}
			/// @}

		private:
			// helpers
			void destroy(pointer first, pointer last) {
				for(; first != last; ++first)
					allocator_.destroy(first);
			}
			BOOST_CONSTEXPR difference_type gap() const BOOST_NOEXCEPT {
				return std::distance(gapFirst_, gapLast_);
			}
			void makeGapAt(const_pointer position) {
				pointer p = const_cast<pointer>(position);
				if(position < gapFirst_) {
					const difference_type n = std::distance(p, gapFirst_);
					const auto newGapLast(std::prev(gapLast_, n));
					if(n <= gap()) {
						uninitializedCopy(std::make_move_iterator(p), std::make_move_iterator(gapFirst_), newGapLast, allocator_);
						destroy(p, gapFirst_);
					} else {
						uninitializedCopy(std::make_move_iterator(gapFirst_ - gap()),
							std::make_move_iterator(gapFirst_), gapFirst_, allocator_);
						try {
							std::copy_backward(std::make_move_iterator(p),
								std::make_move_iterator(std::prev(gapFirst_, gap())), gapFirst_);
						} catch(...) {
							destroy(gapFirst_, gapLast_);
							throw;
						}
						destroy(p, gapFirst_);
					}
					std::advance(gapFirst_, -n);
					gapLast_ = newGapLast;
				} else if(position > gapFirst_) {
					const difference_type n = std::distance(gapFirst_, p);
					const auto newGapLast(std::next(gapLast_, n));
					if(n <= gap()) {
						uninitializedCopy(std::make_move_iterator(gapLast_), std::make_move_iterator(newGapLast), gapFirst_, allocator_);
						destroy(gapLast_, newGapLast);
					} else {
						uninitializedCopy(std::make_move_iterator(gapLast_),
							std::make_move_iterator(std::next(gapLast_, gap())), gapFirst_, allocator_);
						try {
							std::copy(std::make_move_iterator(gapLast_ + gap()),
								std::make_move_iterator(newGapLast), std::next(gapFirst_, gap()));
						} catch(...) {
							destroy(gapFirst_, gapLast_);
							throw;
						}
						destroy(std::next(gapLast_, n - gap()), newGapLast);
					}
					std::advance(gapFirst_, +n);
					gapLast_ = newGapLast;
				}
				assert(gapFirst_ == position);
			}
			void reallocate(size_type newCapacity) {
				if(newCapacity > maxSize())
					throw std::length_error("size");
				assert(newCapacity >= size());
				const pointer newBuffer(allocator_.allocate(newCapacity));
				const pointer newGapLast = std::next(newBuffer, newCapacity - std::distance(gapLast_, last_));
				try {
					uninitializedCopy(std::make_move_iterator(first_),
						std::make_move_iterator(gapFirst_), newBuffer, allocator_);
					try {
						uninitializedCopy(
							std::make_move_iterator(gapLast_), std::make_move_iterator(last_),
							newGapLast, allocator_);
					} catch(...) {
						destroy(first_, gapFirst_);
						throw;
					}
					release();
				} catch(...) {
					allocator_.deallocate(newBuffer, newCapacity);
				}
				gapFirst_ = std::next(newBuffer, std::distance(first_, gapFirst_));
				gapLast_ = newGapLast;
				first_ = newBuffer;
				last_ = std::next(newBuffer, newCapacity);
			}
			void release() {
				destroy(first_, gapFirst_);
				destroy(gapLast_, last_);
				allocator_.deallocate(first_, capacity());
			}

		private:
			allocator_type allocator_;
			pointer first_, last_;
			pointer gapFirst_, gapLast_;
			friend class iterator;
			friend class const_iterator;
		};


		// non-member functions ///////////////////////////////////////////////////////////////////

		template<typename T, typename A>
		inline bool operator==(const GapVector<T, A>& lhs, const GapVector<T, A>& rhs) {
			return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
		}

		template<typename T, typename A>
		inline bool operator<(const GapVector<T, A>& lhs, const GapVector<T, A>& rhs) {
			return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
		}

	}
} // namespace ascension.detail


// non-member functions in std ////////////////////////////////////////////////////////////////////

namespace std {
	template<typename T, typename A>
	inline void swap(ascension::detail::GapVector<T, A>& lhs, ascension::detail::GapVector<T, A>& rhs) {
		lhs.swap(rhs);
	}
}

#endif // !ASCENSION_GAP_VECTOR_HPP
