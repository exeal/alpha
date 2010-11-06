/**
 * @file gap-vector.hpp
 * @author exeal
 * @date 2005-2009 (was gap-buffer.hpp)
 * @date 2010.10.20
 */

#ifndef ASCENSION_GAP_VECTOR_HPP
#define ASCENSION_GAP_VECTOR_HPP
#include <memory>		// std.allocator, std.uninitialized_copy
#include <iterator>		// std.iterator, ...
#include <stdexcept>	// std.out_of_range
#include <algorithm>

namespace ascension {
	namespace internal {

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
			typedef Allocator AllocatorType;
			/// A type counts the number of elements.
			typedef typename Allocator::size_type SizeType;
			/// A type provides the difference between two iterators that refer to elements.
			typedef typename Allocator::difference_type DifferenceType;
			/// A type represents the data type.
			typedef typename Allocator::value_type ValueType;
			/// A type provides a pointer to an element.
			typedef typename Allocator::pointer Pointer;
			/// A type provides a reference to an element.
			typedef typename Allocator::reference Reference;
			/// A type provides a pointer to a const element.
			typedef typename Allocator::const_pointer ConstPointer;
			/// A type provides a reference to a const element.
			typedef typename Allocator::const_reference ConstReference;
			class Iterator;
			class ConstIterator;
			/// A type provides a random-access iterator can read or modify any element in the reversed content.
			typedef std::reverse_iterator<Iterator> ReverseIterator;
			/// A type provides a random-access iterator can read any element in the content.
			typedef std::reverse_iterator<ConstIterator> ConstReverseIterator;
		public:
			/**
			 * Constructor.
			 * @param initialSize the initial size
			 * @param allocator The allocator object
			 */
			explicit GapVector(SizeType initialSize = 10, const Allocator& allocator = Allocator()) :
				allocator_(allocator),
				first_(allocator_.allocate(std::max<SizeType>(initialSize, 10), 0)),
				last_(first_ + std::max<SizeType>(initialSize, 10)),
				gapFirst_(first_), gapLast_(last_) {}
			/**
			 * Constructor specifies repition of a specified number of elements.
			 * @param count The number of elements in the constructed content
			 * @param value The value of elements in the constructed content
			 * @param allocator The allocator object
			 */
			GapVector(SizeType count, ConstReference value, const Allocator& allocator = Allocator()) : 
				allocator_(allocator),
				first_(allocator_.allocate(count, 0)), last_(first_ + count),
				gapFirst_(first_), gapLast_(last_) {insert(0, count, value);}
			/**
			 * Constructor copies a range of a gap vector.
			 * @tparam InputIterator The input iterator
			 * @param first The first element in the range of elements to be copied
			 * @param last The last element in the range of elements to be copied
			 * @param allocator The allocator object
			 */
			template<typename InputIterator>
			GapVector(InputIterator first, InputIterator last, const Allocator& allocator = Allocator()) :
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
			GapVector& operator=(const GapVector& other) {GapVector(other).swap(*this); return *this;}
			/// Destructor.
			~GapVector() {clear(); allocator_.deallocate(first_, capacity());}

			/// Returns a random-access iterator to the beginning of the content.
			Iterator begin() /*throw()*/ {return Iterator(*this, first_);}
			/// Returns a const random-access iterator to the beginning of the content.
			ConstIterator begin() const /*throw()*/ {return ConstIterator(*this, first_);}
			/// Returns a random-access iterator to the end of the content.
			Iterator end() /*throw()*/ {return Iterator(*this, last_);}
			/// Returns a const random-access iterator to the end of the content.
			ConstIterator end() const /*throw()*/ {return ConstIterator(*this, last_);}
			/// Returns a random-access iterator to the beginning of the reversed content.
			ReverseIterator rbegin() /*throw()*/ {return ReverseIterator(end());}
			/// Returns a const random-access iterator to the beginning of the reversed content.
			ConstReverseIterator rbegin() const /*throw()*/ {return ConstReverseIterator(end());}
			/// Returns a random-access iterator to the end of the reversed content.
			ReverseIterator rend() /*throw()*/ {return ReverseIterator(begin());}
			/// Returns a const random-access iterator to the end of the reversed content.
			ConstReverseIterator rend() const /*throw()*/ {return ConstReverseIterator(begin());}

			/**
			 * Returns a reference to the element at a specified position.
			 * @param index The position of the element to retrieve
			 * @return A reference to the element
			 * @throw std#out_of_range @a index is greater than the size of the content
			 */
			Reference at(SizeType index) {
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
			ConstReference at(SizeType index) const {
				if(index >= size())
					throw std::out_of_range("index");
				return operator[](index);
			}
			/**
			 * Returns a reference to the element at a specified position.
			 * @param index The position of the element to retrieve
			 * @return A reference to the element
			 */
			Reference operator[](SizeType index) /*throw()*/ {
				return first_[(first_ + index < gapFirst_) ? index : index + gap()];
			}
			/**
			 * Returns a const reference to the element at a specified position.
			 * @param index The position of the element to retrieve
			 * @return A reference to the element
			 */
			ConstReference operator[](SizeType index) const /*throw()*/ {
				return first_[(first_ + index < gapFirst_) ? index : index + gap()];
			}
			/// Returns @c true if the content is empty.
			bool empty() const /*throw()*/ {return size() == 0;}
			/// Returns the number of elements in the content.
			SizeType size() const /*throw()*/ {return capacity() - gap();}
			/// Returns the number of elements that the content could contain without allocating more storage.
			SizeType capacity() const /*throw()*/ {return last_ - first_;}
			/// Returns the maximum size of the gap vector.
			SizeType maxSize() const {return allocator_.max_size();}
			/// Returns a reference to the first element in this gap vector.
			Reference front() /*throw()*/ {return *begin();}
			/// Returns a const reference to the first element in this gap vector.
			ConstReference front() const /*throw()*/ {return *begin();}
			/// Returns a reference to the last element in this gap vector.
			Reference back() /*throw()*/ {return *--end();}
			/// Returns a const reference to the last element in this gap vector.
			ConstReference back() const /*throw()*/ {return *--end();}

			/**
			 * Assigns a range of elements.
			 * @tparam InputIterator
			 * @param first The first element to assign
			 * @param last The last element to assign
			 */
			template<typename InputIterator>
			void assign(InputIterator first, InputIterator last) {clear(); insert(0, first, last);}
			/**
			 * Assigns a number of elements.
			 * @param count The number of elements to assign
			 * @param value The value of element to assign
			 */
			void assign(SizeType count, ConstReference value = ValueType()) {clear(); insert(0, count, value);}
			/**
			 * Inserts an element into the specified position.
			 * @param index The position in this gap vector where the element is inserted
			 * @param value The value of the element to insert
			 */
			void insert(SizeType index, ConstReference value) {
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
			Iterator insert(Iterator position, ConstReference value) {
				const DifferenceType offset(position.offset());
				insert(offset, value);
				return begin() + offset;
			}
			/**
			 * Inserts a number of elements into the specified position.
			 * @param index The position in this gap vector where the first element is inserted
			 * @param count The number of elements to insert
			 * @param value The value of the element to insert
			 */
			void insert(SizeType index, SizeType count, ConstReference value) {
				makeGapAt(first_ + size());
				makeGapAt(first_ + index);
				if(static_cast<SizeType>(gap()) <= count)
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
			void insert(Iterator position, SizeType count, ConstReference value) {
				insert(position.offset(), count, value);
			}
			/**
			 * Inserts a range of elements into the specified position.
			 * @tparam InputIterator The input iterator
			 * @param index The position in this gap vector where the first element is inserted
			 * @param first The first element to insert
			 * @param last The last element to insert
			 */
			template<typename InputIterator>
			void insert(SizeType index, InputIterator first, InputIterator last) {
				insert(index, first, last, typename PointerType<InputIterator>::Tag());
			}
			/**
			 * Inserts a range of elements into the specified position.
			 * @tparam InputIterator The input iterator
			 * @param position The position in this gap vector where the first element is inserted
			 * @param first The first element to insert
			 * @param last The last element to insert
			 */
			template<typename InputIterator>
			void insert(ConstIterator position, InputIterator first, InputIterator last) {
				insert(position.offset(), first, last);
			}
			/// Erases the all element in this gap vector.
			void clear() {erase(begin(), end());}
			/**
			 * Removes a number of elements in this gap vector.
			 * @param index The position of the first element to remove
			 * @param length The number of elements to remove
			 */
			void erase(SizeType index, SizeType length = 1) {
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
			Iterator erase(Iterator position) {
				const DifferenceType offset(position.offset());
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
			Iterator erase(ConstIterator first, ConstIterator last) {
				const DifferenceType offset(first.offset());
				erase(first.offset(), last - first);
				return begin() + offset;
			}
			/**
			 * Exchanges the elements of two gap vectors.
			 * @param other A gap vector whose elements to be exchanged
			 */
			void swap(GapVector<ValueType, Allocator>& other) {
				std::swap(allocator_, other.allocator);
				std::swap(first_, other.first_);
				std::swap(last_, other.last_);
				std::swap(gapFirst_, other.gapFirst_);
				std::swap(gapLast_, other.gapLast_);
			}

			class ConstIterator : public std::iterator<
				std::random_access_iterator_tag, ValueType, DifferenceType, ConstPointer, ConstReference> {
			public:
				ConstIterator() : target_(0), current_(0) {}
			protected:
				ConstIterator(const GapVector<ValueType, Allocator>& target,
					Pointer position) : target_(&target), current_(position) {assert(current_ != 0);}
			public:
				ConstReference operator*() const /*throw()*/ {return *current_;}
				ConstReference operator->() const /*throw()*/ {return **this;}
				ConstIterator& operator++() /*throw()*/ {
					if(++current_ == target_->gapFirst_) current_ = target_->gapLast_; return *this;}
				ConstIterator operator++(int) /*throw()*/ {ConstIterator temp(*this); ++*this; return temp;}
				ConstIterator& operator--() /*throw()*/ {
					if(--current_ == target_->gapLast_ - 1) current_ = target_->gapFirst_ - 1; return *this;}
				ConstIterator operator--(int) /*throw()*/ {ConstIterator temp(*this); --*this; return temp;}
				ConstIterator& operator+=(DifferenceType n) /*throw()*/ {
					if(current_ + n >= target_->gapFirst_ && current_ + n < target_->gapLast_)
						n += target_->gap();
					current_ += n;
					return *this;
				}
				ConstIterator& operator-=(DifferenceType n) /*throw()*/ {return *this += -n;}
				ConstIterator operator+(DifferenceType n) const /*throw()*/ {ConstIterator temp(*this); return temp += n;}
				ConstIterator operator-(DifferenceType n) const /*throw()*/ {ConstIterator temp(*this); return temp -= n;}
				DifferenceType operator-(const ConstIterator& rhs) const {return offset() - rhs.offset();}
				bool operator==(const ConstIterator& rhs) const /*throw()*/ {return offset() == rhs.offset();}
				bool operator!=(const ConstIterator& rhs) const /*throw()*/ {return !(*this == rhs);}
				bool operator<(const ConstIterator& rhs) const /*throw()*/ {return offset() < rhs.offset();}
				bool operator<=(const ConstIterator& rhs) const /*throw()*/ {return *this == rhs || *this < rhs;}
				bool operator>(const ConstIterator& rhs) const /*throw()*/ {return !(*this <= rhs);}
				bool operator>=(const ConstIterator& rhs) const /*throw()*/ {return !(*this < rhs);}
				friend ConstIterator operator+(DifferenceType lhs, const ConstIterator& rhs) /*throw()*/ {return rhs + lhs;}
			protected:
				DifferenceType offset() const /*throw()*/ {
					return (current_ <= target_->gapFirst_) ?
						current_ - target_->first_ : current_ - target_->gapLast_ + target_->gapFirst_ - target_->first_;}
				const GapVector<ValueType, Allocator>* target_;
				Pointer current_;
				friend class GapVector<ValueType, Allocator>;
			};
			class Iterator : public ConstIterator {
			public:
				Iterator() {}
			private:
				Iterator(const GapVector<ValueType, Allocator>& target, Pointer position) : ConstIterator(target, position) {}
			public:
				typedef Pointer pointer;
				typedef Reference reference;
				Reference operator*() const /*throw()*/ {return const_cast<Reference>(*current_);}
				Reference operator->() const /*throw()*/ {return **this;}
				Iterator& operator++() /*throw()*/ {
					if(++current_ == target_->gapFirst_) current_ = target_->gapLast_; return *this;}
				Iterator operator++(int) /*throw()*/ {Iterator temp(*this); ++*this; return temp;}
				Iterator& operator--() /*throw()*/ {
					if(--current_ == target_->gapLast_ - 1) current_ = target_->gapFirst_ - 1; return *this;}
				Iterator operator--(int) /*throw()*/ {Iterator temp(*this); --*this; return temp;}
				Iterator& operator+=(DifferenceType n) /*throw()*/ {
					if(current_ + n >= target_->gapFirst_ && current_ + n < target_->gapLast_)
						n += target_->gap();
					current_ += n;
					return *this;
				}
				Iterator& operator-=(DifferenceType n) /*throw()*/ {return *this += -n;}
				Iterator operator+(DifferenceType n) const /*throw()*/ {Iterator temp(*this); return temp += n;}
				Iterator operator-(DifferenceType n) const /*throw()*/ {Iterator temp(*this); return temp -= n;}
				DifferenceType operator-(const Iterator& rhs) const /*throw()*/ {return offset() - rhs.offset();}
				bool operator==(const Iterator& rhs) const /*throw()*/ {return offset() == rhs.offset();}
				bool operator!=(const Iterator& rhs) const /*throw()*/ {return !(*this == rhs);}
				bool operator<(const Iterator& rhs) const /*throw()*/ {return offset() < rhs.offset();}
				bool operator<=(const Iterator& rhs) const /*throw()*/ {return *this == rhs || *this < rhs;}
				bool operator>(const Iterator& rhs) const /*throw()*/ {return !(*this <= rhs);}
				bool operator>=(const Iterator& rhs) const /*throw()*/ {return !(*this < rhs);}
			private:
				using ConstIterator::target_;
				using ConstIterator::current_;
				using ConstIterator::offset;
				friend Iterator operator+(DifferenceType lhs, const Iterator& rhs) /*throw()*/ {return rhs + lhs;}
				friend class GapVector<ValueType, Allocator>;
			};
		private:
			// helpers
			template<typename U> struct PointerType {typedef void* Tag;};
			template<typename U> struct PointerType<U*> {typedef int Tag;};
			DifferenceType gap() const /*throw()*/ {return gapLast_ - gapFirst_;}
			template<typename InputIterator>
			void insert(SizeType index, InputIterator first, InputIterator last, int) {
				makeGapAt(first_ + size());
				makeGapAt(first_ + index);
				const SizeType c = last - first;
				if(gap() <= c)
					reallocate(std::max(capacity() + c + 1, capacity() * 2));
				std::memcpy(gapFirst_, first, c * sizeof(ValueType));
				gapFirst_ += c;
			}
			template<typename InputIterator>
			void insert(SizeType index, InputIterator first, InputIterator last, void*) {
				makeGapAt(first_ + size());
				makeGapAt(first_ + index);
				const DifferenceType c = std::distance(first, last);
				if(gap() <= c)
					reallocate(std::max(capacity() + c + 1, capacity() * 2));
				std::copy(first, first + c, gapFirst_);
				gapFirst_ += c;
			}
			void makeGapAt(Pointer position) /*throw()*/ {
				if(position < gapFirst_) {
					gapLast_ -= gapFirst_ - position;
					std::memmove(gapLast_, position, (gapFirst_ - position) * sizeof(T));
				} else if(position > gapFirst_) {
					const Pointer p = position + gap();
					std::memmove(gapFirst_, gapLast_, (p - gapLast_) * sizeof(T));
					gapLast_ = p;
				}
				gapFirst_ = position;
			}
			void reallocate(SizeType newSize) {	// size is not byte-count but element-count
				Pointer newBuffer = allocator_.allocate(newSize, 0);
				Pointer old = first_;
				const DifferenceType tailOffset = gapLast_ - first_;
				const SizeType tailLength = capacity() - tailOffset;
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
			Allocator allocator_;
			Pointer first_, last_;
			Pointer gapFirst_, gapLast_;
			friend class Iterator;
			friend class ConstIterator;
		};

	}
} // namespace ascension.internal

#endif // !ASCENSION_GAP_VECTOR_HPP
