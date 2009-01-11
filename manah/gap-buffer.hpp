// gap-buffer.hpp
// (c) 2005-2007 exeal

#ifndef MANAH_GAP_BUFFER_HPP
#define MANAH_GAP_BUFFER_HPP
#include <memory>		// std.allocator, std.uninitialized_copy
#include <iterator>		// std.iterator, ...
#include <stdexcept>	// std.out_of_range
#include <algorithm>


namespace manah {

	template<typename T> struct GapBuffer_DoNothing {void operator()(T*, T*) {}};
	template<typename T> struct GapBuffer_DeletePointer {void operator()(T* first, T* last) {while(first != last) delete *(first++);}};

	template<typename T, typename ElementsDeleter = GapBuffer_DoNothing<T>, typename Allocator = std::allocator<T> >	// T must be primitive...
	/* final */ class GapBuffer {
	public:
		// types
		typedef typename Allocator::size_type SizeType;
		typedef typename Allocator::difference_type DifferenceType;
		typedef typename Allocator::value_type ValueType;
		typedef typename Allocator::pointer Pointer;
		typedef typename Allocator::reference Reference;
		typedef typename Allocator::const_pointer ConstPointer;
		typedef typename Allocator::const_reference ConstReference;
		class Iterator;
		class ConstIterator;
		typedef std::reverse_iterator<Iterator> ReverseIterator;
		typedef std::reverse_iterator<ConstIterator> ConstReverseIterator;
	public:
		// constructors
		explicit GapBuffer(SizeType initialSize = 10, const Allocator& allocator = Allocator()) :
			allocator_(allocator),
			first_(allocator_.allocate(std::max<SizeType>(initialSize, 10), 0)),
			last_(first_ + std::max<SizeType>(initialSize, 10)),
			gapFirst_(first_), gapLast_(last_) {}
		GapBuffer(SizeType count, ConstReference value, const Allocator& allocator = Allocator()) : 
			allocator_(allocator),
			first_(allocator_.allocate(count, 0)), last_(first_ + count),
			gapFirst_(first_), gapLast_(last_) {insert(0, count, value);}
		template<typename InputIterator> GapBuffer(InputIterator first, InputIterator last, const Allocator& allocator = Allocator()) :
			allocator_(allocator), first_(0), last_(0), gapFirst_(0), gapLast_(0) {insert(0, first, last);}
		GapBuffer(const GapBuffer& rhs) : allocator_(rhs.allocator),
				first_(allocator_.allocate(last_ - first_)), last_(first_ + (rhs.last_ - rhs.first_)),
				gapFirst_(first_ + (rhs.gapFirst_ - rhs.first_)), gapLast_(first_ + (rhs.gapLast_ - rhs.first_)) {
			std::uninitialized_copy(rhs.first_, rhs.gapFirst_, first_);
			std::uninitialized_copy(rhs.gapLast_, rhs.last_, gapFirst_);
		}
		GapBuffer& operator=(const GapBuffer& rhs) {GapBuffer(rhs).swap(*this); return *this;}
		~GapBuffer() {clear(); allocator_.deallocate(first_, capacity());}
		// iterations
		Iterator begin() /*throw()*/ {return Iterator(*this, first_);}
		ConstIterator begin() const /*throw()*/ {return ConstIterator(*this, first_);}
		Iterator end() /*throw()*/ {return Iterator(*this, last_);}
		ConstIterator end() const /*throw()*/ {return ConstIterator(*this, last_);}
		ReverseIterator rbegin() /*throw()*/ {return ReverseIterator(end());}
		ConstReverseIterator rbegin() const /*throw()*/ {return ConstReverseIterator(end());}
		ReverseIterator rend() /*throw()*/ {return ReverseIterator(begin());}
		ConstReverseIterator rend() const /*throw()*/ {return ConstReverseIterator(begin());}
		// attributes
		Reference at(SizeType index) {if(index >= size()) throw std::out_of_range("index"); return operator[](index);}
		ConstReference at(SizeType index) const {if(index >= size()) throw std::out_of_range("index"); return operator[](index);}
		Reference operator[](SizeType index) /*throw()*/ {return first_[(first_ + index < gapFirst_) ? index : index + gap()];}
		ConstReference operator[](SizeType index) const /*throw()*/ {return first_[(first_ + index < gapFirst_) ? index : index + gap()];}
		bool empty() const /*throw()*/ {return size() == 0;}
		SizeType size() const /*throw()*/ {return capacity() - gap();}	// returns not byte-count but element count
		SizeType capacity() const /*throw()*/ {return last_ - first_;}	// returns not byte-count but element count
		SizeType maxSize() const {return allocator_.max_size();}	// returns not byte-count but element count
		Reference front() /*throw()*/ {return *begin();}
		ConstReference front() const /*throw()*/ {return *begin();}
		Reference back() /*throw()*/ {return *--end();}
		ConstReference back() const /*throw()*/ {return *--end();}
		// operations (versions take iterator are slow)
		template<typename InputIterator> void assign(InputIterator first, InputIterator last) {clear(); insert(0, first, last);}
		void assign(SizeType count, ConstReference value = ValueType()) {clear(); insert(0, count, value);}
		void insert(SizeType index, ConstReference value) {
			makeGapAt(first_ + index);
			*(gapFirst_++) = value;
			if(gapFirst_ == gapLast_)
				reallocate(capacity() * 2);
		}
		Iterator insert(Iterator position, ConstReference value) {
			const DifferenceType offset = position.offset(); insert(offset, value); return begin() + offset;}
		void insert(SizeType index, SizeType count, ConstReference value) {
			makeGapAt(first_ + size());
			makeGapAt(first_ + index);
			if(static_cast<SizeType>(gap()) <= count)
				reallocate(std::max(capacity() + count + 1, capacity() * 2));
			std::fill_n(gapFirst_, count, value);
			gapFirst_ += count;
		}
		void insert(Iterator position, SizeType count, ConstReference value) {insert(position.offset(), count, value);}
		template<typename InputIterator> void insert(SizeType index, InputIterator first, InputIterator last) {
			insert(index, first, last, typename PointerType<InputIterator>::Tag());}
		template<typename InputIterator>
		void insert(ConstIterator position, InputIterator first, InputIterator last) {insert(position.offset(), first, last);}
		void clear() {erase(begin(), end());}
		void erase(SizeType index, SizeType length = 1) {
			if(first_ + index <= gapFirst_ && gapFirst_ <= first_ + index + length) {
				ElementsDeleter()(first_ + index, gapFirst_);
				length -= (gapFirst_ - first_) - index;
				gapFirst_ = first_ + index;
			} else
				makeGapAt(first_ + index);
			ElementsDeleter()(gapLast_, gapLast_ + length);
			gapLast_ += length;
		}
		Iterator erase(Iterator position) {const DifferenceType offset = position.offset(); erase(offset); return begin() + offset;}
		void erase(ConstIterator first, ConstIterator last) {erase(first.offset(), last - first);}
		void swap(GapBuffer<ValueType, Allocator>& rhs) {
			std::swap(allocator_, rhs.allocator);
			std::swap(first_, rhs.first_);
			std::swap(last_, rhs.last_);
			std::swap(gapFirst_, rhs.gapFirst_);
			std::swap(gapLast_, rhs.gapLast_);
		}

		class ConstIterator : public std::iterator<std::random_access_iterator_tag, ValueType, DifferenceType, ConstPointer, ConstReference> {
		public:
			ConstIterator() : target_(0), current_(0) {}
		protected:
			ConstIterator(const GapBuffer<ValueType, ElementsDeleter, Allocator>& target, Pointer position)
				: target_(&target), current_(position) {assert(current_ != 0);}
		public:
			ConstReference operator*() const /*throw()*/ {return *current_;}
			ConstReference operator->() const /*throw()*/ {return **this;}
			ConstIterator& operator++() /*throw()*/ {if(++current_ == target_->gapFirst_) current_ = target_->gapLast_; return *this;}
			ConstIterator operator++(int) /*throw()*/ {ConstIterator temp(*this); ++*this; return temp;}
			ConstIterator& operator--() /*throw()*/ {if(--current_ == target_->gapLast_ - 1) current_ = target_->gapFirst_ - 1; return *this;}
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
			const GapBuffer<ValueType, ElementsDeleter, Allocator>* target_;
			Pointer current_;
			friend class GapBuffer<ValueType, ElementsDeleter, Allocator>;
		};
		class Iterator : public ConstIterator {
		public:
			Iterator() {}
		private:
			Iterator(const GapBuffer<ValueType, ElementsDeleter, Allocator>& target, Pointer position) : ConstIterator(target, position) {}
		public:
			typedef Pointer pointer;
			typedef Reference reference;
			Reference operator*() const /*throw()*/ {return const_cast<Reference>(*current_);}
			Reference operator->() const /*throw()*/ {return **this;}
			Iterator& operator++() /*throw()*/ {if(++current_ == target_->gapFirst_) current_ = target_->gapLast_; return *this;}
			Iterator operator++(int) /*throw()*/ {Iterator temp(*this); ++*this; return temp;}
			Iterator& operator--() /*throw()*/ {if(--current_ == target_->gapLast_ - 1) current_ = target_->gapFirst_ - 1; return *this;}
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
			friend class GapBuffer<ValueType, ElementsDeleter, Allocator>;
		};
	private:
		// helpers
		template<typename U> struct PointerType {typedef void* Tag;};
		template<typename U> struct PointerType<U*> {typedef int Tag;};
		DifferenceType gap() const /*throw()*/ {return gapLast_ - gapFirst_;}
		template<typename InputIterator> void insert(SizeType index, InputIterator first, InputIterator last, int) {
			makeGapAt(first_ + size());
			makeGapAt(first_ + index);
			const SizeType c = last - first;
			if(gap() <= c)
				reallocate(std::max(capacity() + c + 1, capacity() * 2));
			std::memcpy(gapFirst_, first, c * sizeof(ValueType));
			gapFirst_ += c;
		}
		template<typename InputIterator> void insert(SizeType index, InputIterator first, InputIterator last, void*) {
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
			std::uninitialized_copy(old + tailOffset, old + tailOffset + tailLength, newBuffer + newSize - tailLength);
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

} // namespace manah

#endif /* MANAH_GAP_BUFFER_HPP */
