/**
 * @file gap-vector.hpp
 * @author exeal
 * @date 2005-2009 (was gap-buffer.hpp)
 * @date 2010-10-20 Renamed GapBuffer to GapVector.
 * @date
 */

#ifndef ASCENSION_GAP_VECTOR_HPP
#define ASCENSION_GAP_VECTOR_HPP
#include <memory>		// std.allocator, std.uninitialized_copy
#include <iterator>		// std.iterator, ...
#include <stdexcept>	// std.out_of_range
#include <algorithm>

namespace ascension {
	namespace detail {

		/**
		 * Implements "gap buffer" data structure.
		 * @tparam T The element type. Should be primitive.
		 * @tparam Allocator The allocator class.
		 * @note This class is not intended to be derived.
		 */
		template<typename T, typename Allocator = std::allocator<T> >
		class GapVector {
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
			class iterator;
			class const_iterator;
			/// A type provides a random-access iterator can read or modify any element in the reversed content.
			typedef std::reverse_iterator<iterator> reverse_iterator;
			/// A type provides a random-access iterator can read any element in the content.
			typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
		public:
			/**
			 * Constructor.
			 * @param initialSize the initial size
			 * @param allocator The allocator object
			 */
			explicit GapVector(size_type initialSize = 10, const allocator_type& allocator = Allocator()) :
				allocator_(allocator),
				first_(allocator_.allocate(std::max<size_type>(initialSize, 10), 0)),
				last_(first_ + std::max<size_type>(initialSize, 10)),
				gapFirst_(first_), gapLast_(last_) {}
			/**
			 * Constructor specifies repition of a specified number of elements.
			 * @param count The number of elements in the constructed content
			 * @param value The value of elements in the constructed content
			 * @param allocator The allocator object
			 */
			GapVector(size_type count, const_reference value, const allocator_type& allocator = Allocator()) : 
				allocator_(allocator),
				first_(allocator_.allocate(count, 0)), last_(first_ + count),
				gapFirst_(first_), gapLast_(last_) {insert(0, count, value);}
			/**
			 * Constructor copies a range of a gap vector.
			 * @tparam Inputiterator The input iterator
			 * @param first The first element in the range of elements to be copied
			 * @param last The last element in the range of elements to be copied
			 * @param allocator The allocator object
			 */
			template<typename Inputiterator>
			GapVector(Inputiterator first, Inputiterator last, const allocator_type& allocator = allocator_type()) :
				allocator_(allocator), first_(0), last_(0), gapFirst_(0), gapLast_(0) {insert(0, first, last);}
			/**
			 * Copy-constructor.
			 * @param other The source object.
			 */
			GapVector(const GapVector& other) : allocator_(other.allocator_),
					first_(allocator_.allocate(last_ - first_)), last_(first_ + (other.last_ - other.first_)),
					gapFirst_(first_ + (other.gapFirst_ - other.first_)), gapLast_(first_ + (other.gapLast_ - rhs.first_)) {
				std::uninitialized_copy(other.first_, other.gapFirst_, first_);
				std::uninitialized_copy(other.gapLast_, other.last_, gapFirst_);
			}
			/**
			 * Assignment operator.
			 * @param other The source object
			 * @return This gap vector.
			 */
			GapVector& operator=(const GapVector<T, Allocator>& other) {GapVector(other).swap(*this); return *this;}
			/// Destructor.
			~GapVector() {clear(); allocator_.deallocate(first_, capacity());}

			/// Returns a random-access iterator to the beginning of the content.
			iterator begin() /*throw()*/ {return iterator(*this, first_);}
			/// Returns a const random-access iterator to the beginning of the content.
			const_iterator begin() const /*throw()*/ {return const_iterator(*this, first_);}
			/// Returns a random-access iterator to the end of the content.
			iterator end() /*throw()*/ {return iterator(*this, last_);}
			/// Returns a const random-access iterator to the end of the content.
			const_iterator end() const /*throw()*/ {return const_iterator(*this, last_);}
			/// Returns a random-access iterator to the beginning of the reversed content.
			reverse_iterator rbegin() /*throw()*/ {return reverse_iterator(end());}
			/// Returns a const random-access iterator to the beginning of the reversed content.
			const_reverse_iterator rbegin() const /*throw()*/ {return const_reverse_iterator(end());}
			/// Returns a random-access iterator to the end of the reversed content.
			reverse_iterator rend() /*throw()*/ {return reverse_iterator(begin());}
			/// Returns a const random-access iterator to the end of the reversed content.
			const_reverse_iterator rend() const /*throw()*/ {return const_reverse_iterator(begin());}

			/**
			 * Returns a reference to the element at a specified position.
			 * @param index The position of the element to retrieve
			 * @return A reference to the element
			 * @throw std#out_of_range @a index is greater than the size of the content
			 */
			reference at(size_type index) {
				if(index >= size())
					throw std::out_of_range("index");
				return operator[](index);
			}
			/**
			 * Returns a const reference to the element at a specified position.
			 * @param index The position of the element to retrieve
			 * @return A reference to the element
			 * @throw std#out_of_range @a index is greater than the size of the content
			 */
			const_reference at(size_type index) const {
				if(index >= size())
					throw std::out_of_range("index");
				return operator[](index);
			}
			/**
			 * Returns a reference to the element at a specified position.
			 * @param index The position of the element to retrieve
			 * @return A reference to the element
			 */
			reference operator[](size_type index) /*throw()*/ {
				return first_[(first_ + index < gapFirst_) ? index : index + gap()];
			}
			/**
			 * Returns a const reference to the element at a specified position.
			 * @param index The position of the element to retrieve
			 * @return A reference to the element
			 */
			const_reference operator[](size_type index) const /*throw()*/ {
				return first_[(first_ + index < gapFirst_) ? index : index + gap()];
			}
			/// Returns @c true if the content is empty.
			bool empty() const /*throw()*/ {return size() == 0;}
			/// Returns the number of elements in the content.
			size_type size() const /*throw()*/ {return capacity() - gap();}
			/// Returns the number of elements that the content could contain without allocating more storage.
			size_type capacity() const /*throw()*/ {return last_ - first_;}
			/// Returns the maximum size of the gap vector.
			size_type maxSize() const {return allocator_.max_size();}
			/// Returns a reference to the first element in this gap vector.
			reference front() /*throw()*/ {return *begin();}
			/// Returns a const reference to the first element in this gap vector.
			const_reference front() const /*throw()*/ {return *begin();}
			/// Returns a reference to the last element in this gap vector.
			reference back() /*throw()*/ {
			iterator temp(end());
			--temp;
			reference r(*temp);
return r;
}
			/// Returns a const reference to the last element in this gap vector.
			const_reference back() const /*throw()*/ {return *--end();}

			/**
			 * Assigns a range of elements.
			 * @tparam Inputiterator
			 * @param first The first element to assign
			 * @param last The last element to assign
			 */
			template<typename Inputiterator>
			void assign(Inputiterator first, Inputiterator last) {clear(); insert(0, first, last);}
			/**
			 * Assigns a number of elements.
			 * @param count The number of elements to assign
			 * @param value The value of element to assign
			 */
			void assign(size_type count, const_reference value = value_type()) {clear(); insert(0, count, value);}
			/**
			 * Inserts an element into the specified position.
			 * @param index The position in this gap vector where the element is inserted
			 * @param value The value of the element to insert
			 */
			void insert(size_type index, const_reference value) {
				makeGapAt(first_ + index);
				*(gapFirst_++) = value;
				if(gapFirst_ == gapLast_)
					reallocate(capacity() * 2);
			}
			/**
			 * Inserts an element into the specified position.
			 * @param position The position in this gap vector where the element is inserted
			 * @param value The value of the element to insert
			 * @return An iterator addresses the new inserted element
			 */
			iterator insert(iterator position, const_reference value) {
				const difference_type offset(position.offset());
				insert(offset, value);
				return begin() + offset;
			}
			/**
			 * Inserts a number of elements into the specified position.
			 * @param index The position in this gap vector where the first element is inserted
			 * @param count The number of elements to insert
			 * @param value The value of the element to insert
			 */
			void insert(size_type index, size_type count, const_reference value) {
				makeGapAt(first_ + size());
				makeGapAt(first_ + index);
				if(static_cast<size_type>(gap()) <= count)
					reallocate(std::max(capacity() + count + 1, capacity() * 2));
				std::fill_n(gapFirst_, count, value);
				gapFirst_ += count;
			}
			/**
			 * Inserts a number of elements into the specified position.
			 * @param index The position in this gap vector where the first element is inserted
			 * @param count The number of elements to insert
			 * @param value The value of the element to insert
			 */
			void insert(iterator position, size_type count, const_reference value) {
				insert(position.offset(), count, value);
			}
			/**
			 * Inserts a range of elements into the specified position.
			 * @tparam Inputiterator The input iterator
			 * @param index The position in this gap vector where the first element is inserted
			 * @param first The first element to insert
			 * @param last The last element to insert
			 */
			template<typename Inputiterator>
			void insert(size_type index, Inputiterator first, Inputiterator last) {
				insert(index, first, last, typename PointerType<Inputiterator>::Tag());
			}
			/**
			 * Inserts a range of elements into the specified position.
			 * @tparam Inputiterator The input iterator
			 * @param position The position in this gap vector where the first element is inserted
			 * @param first The first element to insert
			 * @param last The last element to insert
			 */
			template<typename Inputiterator>
			void insert(const_iterator position, Inputiterator first, Inputiterator last) {
				insert(position.offset(), first, last);
			}
			/// Erases the all element in this gap vector.
			void clear() {erase(begin(), end());}
			/**
			 * Removes a number of elements in this gap vector.
			 * @param index The position of the first element to remove
			 * @param length The number of elements to remove
			 */
			void erase(size_type index, size_type length = 1) {
				if(first_ + index <= gapFirst_ && gapFirst_ <= first_ + index + length) {
					length -= (gapFirst_ - first_) - index;
					gapFirst_ = first_ + index;
				} else
					makeGapAt(first_ + index);
				gapLast_ += length;
			}
			/**
			 * Removes an element in this gap vector.
			 * @param position The position of the element to remove
			 * @return An iterator addresses the first element remaining beyond the removed element
			 */
			iterator erase(iterator position) {
				const difference_type offset(position.offset());
				erase(offset);
				return begin() + offset;
			}
			/**
			 * Removes elements in this gap vector.
			 * @param first The iterator addresses the beginning of the range to remove
			 * @param last The iterator addresses the end of the range to remove
			 * @return An iterator addresses the first element remaining beyond any elements
			 *         removed
			 */
			iterator erase(const_iterator first, const_iterator last) {
				const difference_type offset(first.offset());
				erase(first.offset(), last - first);
				return begin() + offset;
			}
			/**
			 * Exchanges the elements of two gap vectors.
			 * @param other A gap vector whose elements to be exchanged
			 */
			void swap(GapVector<value_type, allocator_type>& other) {
				std::swap(allocator_, other.allocator);
				std::swap(first_, other.first_);
				std::swap(last_, other.last_);
				std::swap(gapFirst_, other.gapFirst_);
				std::swap(gapLast_, other.gapLast_);
			}

			class const_iterator : public std::iterator<
				std::random_access_iterator_tag, value_type, difference_type, const_pointer, const_reference> {
			public:
				const_iterator() : target_(0), current_(0) {}
			protected:
				const_iterator(const GapVector<value_type, allocator_type>& target,
					pointer position) : target_(&target), current_(position) {assert(current_ != 0);}
			public:
				const_reference operator*() const /*throw()*/ {return *current_;}
				const_reference operator->() const /*throw()*/ {return **this;}
				const_iterator& operator++() /*throw()*/ {
					if(++current_ == target_->gapFirst_) current_ = target_->gapLast_; return *this;}
				const_iterator operator++(int) /*throw()*/ {const_iterator temp(*this); ++*this; return temp;}
				const_iterator& operator--() /*throw()*/ {
					if(--current_ == target_->gapLast_ - 1) current_ = target_->gapFirst_ - 1; return *this;}
				const_iterator operator--(int) /*throw()*/ {const_iterator temp(*this); --*this; return temp;}
				const_iterator& operator+=(difference_type n) /*throw()*/ {
					if(current_ + n >= target_->gapFirst_ && current_ + n < target_->gapLast_)
						n += target_->gap();
					current_ += n;
					return *this;
				}
				const_iterator& operator-=(difference_type n) /*throw()*/ {return *this += -n;}
				const_iterator operator+(difference_type n) const /*throw()*/ {const_iterator temp(*this); return temp += n;}
				const_iterator operator-(difference_type n) const /*throw()*/ {const_iterator temp(*this); return temp -= n;}
				difference_type operator-(const const_iterator& rhs) const {return offset() - rhs.offset();}
				bool operator==(const const_iterator& rhs) const /*throw()*/ {return offset() == rhs.offset();}
				bool operator!=(const const_iterator& rhs) const /*throw()*/ {return !(*this == rhs);}
				bool operator<(const const_iterator& rhs) const /*throw()*/ {return offset() < rhs.offset();}
				bool operator<=(const const_iterator& rhs) const /*throw()*/ {return *this == rhs || *this < rhs;}
				bool operator>(const const_iterator& rhs) const /*throw()*/ {return !(*this <= rhs);}
				bool operator>=(const const_iterator& rhs) const /*throw()*/ {return !(*this < rhs);}
				friend const_iterator operator+(difference_type lhs, const const_iterator& rhs) /*throw()*/ {return rhs + lhs;}
			protected:
				difference_type offset() const /*throw()*/ {
					return (current_ <= target_->gapFirst_) ?
						current_ - target_->first_ : current_ - target_->gapLast_ + target_->gapFirst_ - target_->first_;}
				const GapVector<value_type, allocator_type>* target_;
				pointer current_;
				friend class GapVector<value_type, allocator_type>;
			};
			class iterator : public const_iterator {
			public:
				iterator() {}
			private:
				iterator(const GapVector<value_type, allocator_type>& target, pointer position) : const_iterator(target, position) {}
			public:
				typedef typename GapVector<value_type, allocator_type>::pointer pointer;
				typedef typename GapVector<value_type, allocator_type>::reference reference;
				reference operator*() const /*throw()*/ {return const_cast<reference>(*current_);}
				reference operator->() const /*throw()*/ {return **this;}
				iterator& operator++() /*throw()*/ {
					if(++current_ == target_->gapFirst_) current_ = target_->gapLast_; return *this;}
				iterator operator++(int) /*throw()*/ {iterator temp(*this); ++*this; return temp;}
				iterator& operator--() /*throw()*/ {
					if(--current_ == target_->gapLast_ - 1) current_ = target_->gapFirst_ - 1; return *this;}
				iterator operator--(int) /*throw()*/ {iterator temp(*this); --*this; return temp;}
				iterator& operator+=(difference_type n) /*throw()*/ {
					if(current_ + n >= target_->gapFirst_ && current_ + n < target_->gapLast_)
						n += target_->gap();
					current_ += n;
					return *this;
				}
				iterator& operator-=(difference_type n) /*throw()*/ {return *this += -n;}
				iterator operator+(difference_type n) const /*throw()*/ {iterator temp(*this); return temp += n;}
				iterator operator-(difference_type n) const /*throw()*/ {iterator temp(*this); return temp -= n;}
				difference_type operator-(const iterator& rhs) const /*throw()*/ {return offset() - rhs.offset();}
				bool operator==(const iterator& rhs) const /*throw()*/ {return offset() == rhs.offset();}
				bool operator!=(const iterator& rhs) const /*throw()*/ {return !(*this == rhs);}
				bool operator<(const iterator& rhs) const /*throw()*/ {return offset() < rhs.offset();}
				bool operator<=(const iterator& rhs) const /*throw()*/ {return *this == rhs || *this < rhs;}
				bool operator>(const iterator& rhs) const /*throw()*/ {return !(*this <= rhs);}
				bool operator>=(const iterator& rhs) const /*throw()*/ {return !(*this < rhs);}
			private:
				using const_iterator::target_;
				using const_iterator::current_;
				using const_iterator::offset;
				friend iterator operator+(difference_type lhs, const iterator& rhs) /*throw()*/ {return rhs + lhs;}
				friend class GapVector<value_type, allocator_type>;
			};
		private:
			// helpers
			template<typename U> struct PointerType {typedef void* Tag;};
			template<typename U> struct PointerType<U*> {typedef int Tag;};
			difference_type gap() const /*throw()*/ {return gapLast_ - gapFirst_;}
			template<typename Inputiterator>
			void insert(size_type index, Inputiterator first, Inputiterator last, int) {
				makeGapAt(first_ + size());
				makeGapAt(first_ + index);
				const size_type c = last - first;
				if(gap() <= c)
					reallocate(std::max(capacity() + c + 1, capacity() * 2));
				std::memcpy(gapFirst_, first, c * sizeof(value_type));
				gapFirst_ += c;
			}
			template<typename Inputiterator>
			void insert(size_type index, Inputiterator first, Inputiterator last, void*) {
				makeGapAt(first_ + size());
				makeGapAt(first_ + index);
				const difference_type c = std::distance(first, last);
				if(gap() <= c)
					reallocate(std::max(capacity() + c + 1, capacity() * 2));
				std::copy(first, first + c, gapFirst_);
				gapFirst_ += c;
			}
			void makeGapAt(pointer position) /*throw()*/ {
				if(position < gapFirst_) {
					gapLast_ -= gapFirst_ - position;
					std::memmove(gapLast_, position, (gapFirst_ - position) * sizeof(T));
				} else if(position > gapFirst_) {
					const pointer p = position + gap();
					std::memmove(gapFirst_, gapLast_, (p - gapLast_) * sizeof(T));
					gapLast_ = p;
				}
				gapFirst_ = position;
			}
			void reallocate(size_type newSize) {	// size is not byte-count but element-count
				pointer newBuffer = allocator_.allocate(newSize, 0);
				pointer old = first_;
				const difference_type tailOffset = gapLast_ - first_;
				const size_type tailLength = capacity() - tailOffset;
				std::uninitialized_copy(old, old + (gapFirst_ - first_), newBuffer);
				std::uninitialized_copy(old + tailOffset,
					old + tailOffset + tailLength, newBuffer + newSize - tailLength);
				allocator_.deallocate(old, capacity());
				gapFirst_ = newBuffer + (gapFirst_ - first_);
				first_ = newBuffer;
				gapLast_ = (first_ + newSize) - tailLength;
				last_ = first_ + newSize;
			}

		private:
			allocator_type allocator_;
			pointer first_, last_;
			pointer gapFirst_, gapLast_;
			friend class iterator;
			friend class const_iterator;
		};

	}
} // namespace ascension.detail

#endif // !ASCENSION_GAP_VECTOR_HPP
