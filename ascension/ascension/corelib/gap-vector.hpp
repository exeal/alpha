/**
 * @file gap-vector.hpp
 * @author exeal
 * @date 2005-2009 (was gap-buffer.hpp)
 * @date 2010-10-20 Renamed GapBuffer to GapVector.
 * @date 2011-2012
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
		class GapVectorIterator : public boost::iterator_facade<
			GapVectorIterator<Target, Pointer, Reference>, typename Target::value_type,
			std::random_access_iterator_tag, Reference, typename Target::difference_type> {
		public:
			typedef GapVectorIterator<Target, Pointer, Reference> Self;
			GapVectorIterator() /*noexcept*/ : target_(nullptr) {}
			GapVectorIterator(const Target& target, Pointer position) /*noexcept*/
					: target_(&target), current_(position - target.first_) {}
			template<typename Pointer2, typename Reference2>
			GapVectorIterator(const GapVectorIterator<Target, Pointer2, Reference2>& other)
					: target_(other.target()), current_(other.get() - other.target()->first_) {}
			Self& operator=(const Self& other) {
				target_ = other.target_;
				current_ = other.current_;
				return *this;
			}
			const Pointer get() const /*noexcept*/ {return target()->first_ + current_;}
			const Target* target() const /*noexcept*/ {return target_;}
		private:
			difference_type offset() const /*noexcept*/ {
				return (get() <= target_->gapFirst_) ?
					get() - target_->first_ :
						get() - target_->gapLast_ + target_->gapFirst_ - target_->first_;
			}
			friend class boost::iterator_core_access;
			void advance(difference_type n) {
				if(get() + n >= target_->gapFirst_ && get() + n < target_->gapLast_)
					n += target_->gap();
				current_ += n;
			}
			void decrement() /*noexcept*/ {
				if(get() != target_->gapLast_)
					--current_;
				else
					current_ = (target()->gapFirst_ - target()->first_) - 1;
			}
			reference dereference() const /*noexcept*/ {
				return target_->first_[current_];
			}
			difference_type distance_to(const GapVectorIterator& other) const /*noexcept*/ {
				return current_ - other.current_;
			}
			bool equal(const GapVectorIterator& other) const /*noexcept*/ {
				return current_ == current_;
			}
			void increment() /*noexcept*/ {
				assert(get() != target_->gapFirst_);
				++current_;
				if(get() == target_->gapFirst_)
					current_ += target_->gap();
			}
		private:
			const Target* target_;
			difference_type current_;
		};

		/**
		 * Implements "gap buffer" data structure.
		 * @tparam T The element type. Should be primitive
		 * @tparam Allocator The allocator to use for all memory allocations of this container
		 * @note This class is not intended to be derived.
		 */
		template<typename T, typename Allocator = std::allocator<T>>
		class GapVector : boost::totally_ordered<GapVector<T, Allocator>> {
		public:

			// member types ///////////////////////////////////////////////////////////////////////

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


			// core member functions //////////////////////////////////////////////////////////////

			/**
			 * Constructor.
			 * @param allocator The allocator object
			 */
			explicit GapVector(const allocator_type& allocator = allocator_type()) :
				allocator_(allocator),
				first_(allocator_.allocate(ASCENSION_GAP_VECTOR_INITIAL_SIZE, 0)),
				last_(first_ + ASCENSION_GAP_VECTOR_INITIAL_SIZE),
				gapFirst_(first_), gapLast_(last_) {}

			/**
			 * Constructor specifies repition of a specified number of elements.
			 * @param count The number of elements in the constructed content
			 * @param value The value of elements in the constructed content
			 * @param allocator The allocator object
			 */
			GapVector(size_type count, const_reference value,
					const allocator_type& allocator = allocator_type()) : allocator_(allocator),
					first_(allocator_.allocate(count, 0)), last_(first_ + count),
					gapFirst_(first_), gapLast_(last_) {
				insert(0, count, value);
			}

			/**
			 * Constructor copies a range of a gap vector.
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
					first_(allocator_.allocate(last_ - first_)),
					last_(first_ + (other.last_ - other.first_)),
					gapFirst_(first_ + (other.gapFirst_ - other.first_)),
					gapLast_(first_ + (other.gapLast_ - other.first_)) {
				try {
					uninitializedCopy(other.first_, other.gapFirst_, first_, allocator);
					uninitializedCopy(other.gapLast_, other.last_, gapFirst_, allocator);
				} catch(...) {
					allocator.deallocate(first_, last_ - first_);
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
					first_(allocator_.allocate(last_ - first_)),
					last_(first_ + (other.last_ - other.first_)),
					gapFirst_(first_ + (other.gapFirst_ - other.first_)),
					gapLast_(first_ + (other.gapLast_ - other.first_)) {
				try {
					uninitializedCopy(other.first_, other.gapFirst_, first_, allocator);
					uninitializedCopy(other.gapLast_, other.last_, gapFirst_, allocator);
				} catch(...) {
					allocator.deallocate(first_, last_ - first_);
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
					first_(allocator_.allocate(last_ - first_)),
					last_(first_ + (other.last_ - other.first_)),
					gapFirst_(first_ + (other.gapFirst_ - other.first_)),
					gapLast_(first_ + (other.gapLast_ - other.first_)) {
				for(pointer p = first_; p != gapFirst_; ++p)
					*p = std::move(other.first_ + (p - first_));
				for(pointer p = gapLast_; p != last_; ++p)
					*p = std::move(other.first_ + (p - first_));
				other.first_ = other.last_ = other.gapFirst_ = other.gapLast_ = nullptr;
			}

			/**
			 * Constructor with an initializer list.
			 * @param values The initializer list to initialize the elements of the container with
			 * @param allocator The allocator object
			 */
			GapVector(std::initializer_list<value_type> values, const allocator_type& allocator);

			/// Destructor.
			~GapVector() {release();}

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
			 * @param count The new size of the container
			 * @param value The value to initialize elements of the container with
			 */
			void assign(size_type count, const_reference value) {
				GapVector<value_type, allocator_type> temp(count, value);
				*this = std::move(temp);
			}

			/// Returns the allocator associated with the container.
			allocator_type allocator() const {return allocator_;}


			// element accesses ///////////////////////////////////////////////////////////////////

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
			reference operator[](size_type position) /*throw()*/ {
				return first_[(first_ + position < gapFirst_) ? position : position + gap()];
			}

			/**
			 * Returns a reference to the element at a specified position.
			 * @param position The position of the element to return
			 * @return A reference to the requested element
			 */
			const_reference operator[](size_type position) const /*throw()*/ {
				return first_[(first_ + position < gapFirst_) ? position : position + gap()];
			}

			/// Returns a reference to the first element in the container.
			reference front() /*throw()*/ {return (*this)[0];}

			/// Returns a reference to the first element in the container.
			const_reference front() const /*throw()*/ {return (*this)[0];}

			/// Returns a reference to the last element in the container.
			reference back() /*throw()*/ {return (*this)[size() - 1];}

			/// Returns a const reference to the last element in the container.
			const_reference back() const /*throw()*/ {return (*this)[size() - 1];}


			// iterators //////////////////////////////////////////////////////////////////////////

			/// Returns an iterator to the first element of the container.
			iterator begin() /*noexcept*/ {return iterator(*this, first_);}

			/// Returns an iterator to the first element of the container.
			const_iterator begin() const /*noexcept*/ {return const_iterator(*this, first_);}

			/// Returns an iterator to the first element of the container.
			const_iterator cbegin() const /*noexcept*/ {return begin();}

			/// Returns an iterator to the element following the last element of the container.
			iterator end() /*noexcept*/ {return iterator(*this, last_);}

			/// Returns an iterator to the element following the last element of the container.
			const_iterator end() const /*noexcept*/ {return const_iterator(*this, last_);}

			/// Returns an iterator to the element following the last element of the container.
			const_iterator cend() const /*noexcept*/ {return end();}

			/// Returns a reverse iterator to the first element of the reversed container.
			reverse_iterator rbegin() /*noexcept*/ {return reverse_iterator(end());}

			/// Returns a reverse iterator to the first element of the reversed container.
			const_reverse_iterator rbegin() const /*noexcept*/ {return const_reverse_iterator(end());}

			/// Returns a reverse iterator to the first element of the reversed container.
			const_reverse_iterator crbegin() const /*noexcept*/ {return rbegin();}

			/// Returns a reverse iterator to the element following the last element of the
			/// reversed container.
			reverse_iterator rend() /*noexcept*/ {return reverse_iterator(begin());}

			/// Returns a reverse iterator to the element following the last element of the
			/// reversed container.
			const_reverse_iterator rend() const /*noexcept*/ {return const_reverse_iterator(begin());}

			/// Returns a reverse iterator to the element following the last element of the
			/// reversed container.
			const_reverse_iterator crend() const /*noexcept*/ {return rend();}


			// capacities /////////////////////////////////////////////////////////////////////////

			/// Returns @c true if the container is empty.
			bool empty() const /*noexcept*/ {return size() == 0;}

			/// Returns the number of elements in the container.
			size_type size() const /*noexcept*/ {return capacity() - gap();}

			/// Returns the maximum size of the container.
			size_type maxSize() const /*noexcept*/ {return allocator_.max_size();}

			/**
			 * Sets the capacity of the container to at least @a size.
			 * @param newCapacity The new capacity of the container
			 */
			void reserve(size_type newCapacity) {reallocate(newCapacity);}

			/// Returns the number of elements that the content could contain without allocating
			/// more storage.
			size_type capacity() const /*noexcept*/ {return last_ - first_;}

			/// Requests the removal of unused capacity.
			void shrinkToFit() {reallocate(size());}


			// modifiers //////////////////////////////////////////////////////////////////////////

			/// Removes all elements from the container.
			void clear() {erase(begin(), end());}	// TODO: Follow std.vector.clear semantics.

			/**
			 * Inserts an element to the specified position in the container.
			 * @param position The element before which the content will be inserted
			 * @param value The element value to insert
			 */
			void insert(const_iterator position, const_reference value) {
				makeGapAt(position.get());
				*(gapFirst_++) = value;
				if(gapFirst_ == gapLast_)
					reallocate(capacity() * 2);
			}

			/**
			 * Inserts an element to the specified position in the container.
			 * @param position The element before which the content will be inserted
			 * @param value The element value to insert
			 */
			void insert(const_iterator position, value_type&& value) {
				makeGapAt(position.get());
				*(gapFirst_++) = std::move(value);
				if(gapFirst_ == gapLast_)
					reallocate(capacity() * 2);
			}

			/**
			 * Inserts elements to the specified position in the container.
			 * @param position The element before which the content will be inserted
			 * @param count The number of the elements to insert
			 * @param value The element value to insert
			 */
			void insert(const_iterator position, size_type count, const_reference value) {
				makeGapAt(first_ + size());
				makeGapAt(position.get());
				if(static_cast<size_type>(gap()) <= count)
					reallocate(std::max(capacity() + count + 1, capacity() * 2));
				std::fill_n(gapFirst_, count, value);
				gapFirst_ += count;
			}

			/**
			 * Inserts elements to the specified position in the container.
			 * @tparam Inputiterator The input iterator
			 * @param position The element before which the content will be inserted
			 * @param first The beginning of the range of elements to insert
			 * @param last The end of the range of elements to insert
			 */
			template<typename InputIterator>
			void insert(const_iterator position, InputIterator first, InputIterator last) {
				const difference_type c = std::distance(first, last);
				if(c != 0) {
					if(c > gap())
						reallocate(std::max(c + size(), capacity() * 2));
//					makeGapAt(first_ + size());
					makeGapAt(position.get());
					pointer p = const_cast<pointer>(position.get());
					gapFirst_ = first_ + (uninitializedCopy(first, last, p, allocator_) - first_);
				}
			}

			/**
			 * Inserts elements to the specified position in the container.
			 * @param position The element before which the content will be inserted
			 * @param values The initializer list to insert the values from
			 */
			void insert(const_iterator position, std::initializer_list<value_type> values);

//			template<typename... Arguments>
//			iterator emplace(const_iterator position, Arguments&& ...arguments);

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
				const pointer b = first_ + (first.get() - first_);
				const pointer e = first_ + (last.get() - first_);
				if(b >= gapLast_) {
					makeGapAt(b);
					destroy(b, e);
					gapLast_ = e;
				} else if(e <= gapFirst_) {
					makeGapAt(e);
					destroy(b, e);
					gapFirst_ = b;
				} else {
					destroy(b, gapFirst_);
					destroy(gapLast_, e);
					gapFirst_ = b;
					gapLast_ = e;
				}
				return iterator(*this, gapLast_);
			}

			void pushBack(const_reference value);
			void pushBack(value_type&& value);
//			template<typename... Arguments> void emplaceBack(Arguments&& ...arguments);
			void popBack();

			/**
			 * Resizes the container to contain @a count elements.
			 * @param count The new size of the container
			 */
			void resize(size_type size);

			/**
			 * Resizes the container to contain @a count elements.
			 * @param count The new size of the container
			 * @param value The value to initialize the new elements with
			 */
			void resize(size_type count, const_reference value);

			/**
			 * Exchanges the elements of two gap vectors.
			 * @param other A gap vector whose elements to be exchanged
			 */
			void swap(GapVector& other) /*noexcept*/ {
				std::swap(allocator_, other.allocator);
				std::swap(first_, other.first_);
				std::swap(last_, other.last_);
				std::swap(gapFirst_, other.gapFirst_);
				std::swap(gapLast_, other.gapLast_);
			}

		private:
			// helpers
			void destroy(pointer first, pointer last) {
				for(; first != last; ++first)
					allocator_.destroy(first);
			}
			difference_type gap() const /*throw()*/ {return gapLast_ - gapFirst_;}
			void makeGapAt(const_pointer position) {
				pointer p = const_cast<pointer>(position);
				if(position < gapFirst_) {
					const size_type n = gapFirst_ - position;
					if(n <= static_cast<size_type>(gap())) {
						uninitializedCopy(std::make_move_iterator(p),
							std::make_move_iterator(gapFirst_), gapLast_ - n, allocator_);
						destroy(p, gapFirst_);
					} else {
						uninitializedCopy(std::make_move_iterator(gapFirst_ - gap()),
							std::make_move_iterator(gapFirst_), gapLast_ - gap(), allocator_);
						try {
							std::copy_backward(std::make_move_iterator(p),
								std::make_move_iterator(gapFirst_ - gap()), gapLast_ - gap());
						} catch(...) {
							destroy(gapLast_ - gap(), gapLast_);
							throw;
						}
						destroy(p, gapLast_ - gap());
					}
					gapFirst_ -= n;
					gapLast_ -= n;
				} else if(position > gapFirst_) {
					const size_type n = position - gapFirst_;
					if(n <= static_cast<size_type>(gap())) {
						uninitializedCopy(std::make_move_iterator(gapLast_),
							std::make_move_iterator(gapLast_ + n), gapFirst_, allocator_);
						destroy(gapLast_, gapLast_ + n);
					} else {
						uninitializedCopy(std::make_move_iterator(gapLast_),
							std::make_move_iterator(gapLast_ + gap()), gapFirst_, allocator_);
						try {
							std::copy(std::make_move_iterator(gapLast_ + gap()),
								std::make_move_iterator(gapLast_ + n), gapFirst_ + gap());
						} catch(...) {
							destroy(gapFirst_, gapLast_);
							throw;
						}
						destroy(gapLast_ + (n - gap()), gapLast_ + n);
					}
					gapFirst_ += n;
					gapLast_ += n;
				}
			}
			void reallocate(size_type newCapacity) {
				if(newCapacity > maxSize())
					throw std::length_error("size");
				assert(newCapacity >= size());
				const pointer temp = allocator_.allocate(newCapacity);
				uninitializedCopy(std::make_move_iterator(first_),
					std::make_move_iterator(gapFirst_), temp, allocator_);
				try {
					uninitializedCopy(
						std::make_move_iterator(gapLast_), std::make_move_iterator(last_),
						temp + (gapFirst_ - first_), allocator_);
				} catch(...) {
					destroy(first_, gapFirst_);
					throw;
				}
				release();
				gapFirst_ = temp + (gapFirst_ - first_);
				gapLast_ = temp + (gapLast_ - first_);
				first_ = temp;
				last_ = temp + newCapacity;
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
