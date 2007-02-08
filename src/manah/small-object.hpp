// small-object.hpp
// (c) 2007 exeal

#ifndef MANAH_SMALL_OBJECT_HPP
#define MANAH_SMALL_OBJECT_HPP

#include "memory.hpp"
#include <vector>
#include <limits>

namespace manah {

	// SmallObject //////////////////////////////////////////////////////////////

#ifndef SMALL_OBJECT_DEFAULT_CHUNK_SIZE
#define SMALL_OBJECT_DEFAULT_CHUNK_SIZE	4096
#endif
#ifndef MAX_SMALL_OBJECT_SIZE
#define MAX_SMALL_OBJECT_SIZE	64
#endif

	template<std::size_t chunkSize, std::size_t maxSmallObjectSize>
	class SmallObjectAllocator;

	struct NoLock {};

	// small object (from Loki (http://sourceforge.net/projects/loki-lib/))
	// this implementation has bugs
	template<
		class Locker = NoLock,
		std::size_t chunkSize = SMALL_OBJECT_DEFAULT_CHUNK_SIZE,
		std::size_t maxSmallObjectSize = MAX_SMALL_OBJECT_SIZE>
	class SmallObject {
	public:
		virtual	~SmallObject() {}
		static void* operator new(std::size_t size) {
#if(SMALL_OBJECT_DEFAULT_CHUNK_SIZE != 0) && (MAX_SMALL_OBJECT_SIZE != 0)
			Locker locker;
			return getAllocator().allocate(size);
#else
			return ::operator new(size);
#endif
	}
		static void operator delete(void* p, std::size_t size) {
#if(DEFAULT_CHUNK_SIZE != 0) && (MAX_SMALL_OBJECT_SIZE != 0)
			Locker locker;
			getAllocator().deallocate(p, size);
#else
			::operator delete(p, size);
#endif
	}
	private:
		static SmallObjectAllocator<chunkSize, maxSmallObjectSize>&	getAllocator() {
			static SmallObjectAllocator<chunkSize, maxSmallObjectSize> allocator;
			return allocator;
		}
	};

	class FixedAllocator {
	public:
		typedef unsigned char SizeType;
		explicit FixedAllocator(std::size_t blockSize = 0) : blockSize_(blockSize), lastAllocatedChunk_(0), lastDeallocatedChunk_(0) {
			assert(blockSize_ > 0);
			next_ = prev_ = this;
			std::size_t blockCount = SMALL_OBJECT_DEFAULT_CHUNK_SIZE / blockSize;
			if(blockCount > std::numeric_limits<SizeType>::max())
				blockCount = std::numeric_limits<SizeType>::max();
			else if(blockCount == 0)
				blockCount = 8 * blockSize;
			blockCount_ = static_cast<SizeType>(blockCount);
			assert(blockCount_ == blockCount);
		}
		FixedAllocator(const FixedAllocator& rhs) : blockSize_(rhs.blockSize_), blockCount_(rhs.blockCount_), chunks_(rhs.chunks_) {
			prev_ = &rhs;
			next_ = rhs.next_;
			rhs.next_->prev_ = this;
			rhs.next_ = this;
			lastAllocatedChunk_ = (rhs.lastAllocatedChunk_ != 0) ?
				&chunks_.front() + (rhs.lastAllocatedChunk_ - &rhs.chunks_.front()) : 0;
			lastDeallocatedChunk_ = (rhs.lastDeallocatedChunk_ != 0) ?
				&chunks_.front() + (rhs.lastDeallocatedChunk_ - &rhs.chunks_.front()) : 0;
		}
		~FixedAllocator() {
			if(prev_ != this) {
				prev_->next_ = next_;
				next_->prev_ = prev_;
				return;
			}
			assert(prev_ == next_);
			for(ChunkVector::iterator it = chunks_.begin(); it != chunks_.end(); ++it) {
//				assert(it->getAvailableBlockCount() == blockCount_);
				it->release();
			}
		}

		// メソッド
	public:
		void* allocate() {
			if(lastAllocatedChunk_ != 0 && lastAllocatedChunk_->isAvailable())
				return lastAllocatedChunk_->allocate(blockSize_);
			for(ChunkVector::iterator it = chunks_.begin(); ; ++it) {
				if(it == chunks_.end()) {
					chunks_.reserve(chunks_.size() + 1);
					chunks_.push_back(Chunk(blockSize_, blockCount_));
					lastAllocatedChunk_ = &chunks_.back();
					lastDeallocatedChunk_ = &chunks_.front();
					break;
				} else if(it->isAvailable()) {
					lastAllocatedChunk_ = &*it;
					break;
				}
			}
			assert(lastAllocatedChunk_ != 0 && lastAllocatedChunk_->isAvailable());
			return lastAllocatedChunk_->allocate(blockSize_);
		}
		void deallocate(void* p) {
			assert(p != 0);
			assert(!chunks_.empty());
			assert(&chunks_.front() <= lastDeallocatedChunk_);
			assert(&chunks_.back() >= lastDeallocatedChunk_);
			assert(lastDeallocatedChunk_ != 0);

			const std::size_t	chunkLength = blockSize_ * blockCount_;
			Chunk*				lower = lastDeallocatedChunk_;
			Chunk*				upper = lower + 1;
			const Chunk*		lowerBound = &chunks_.front();
			const Chunk*		upperBound = &chunks_.back() + 1;

			if(upper == upperBound)
				upper = 0;
			while(true) {
				if(lower != 0) {
					if(lower->isContaining(p, chunkLength)) {
						lastDeallocatedChunk_ = lower;
						break;
					} else if(lower == lowerBound)
						lower = 0;
					else
						--lower;
				}
				if(upper != 0) {
					if(upper->isContaining(p, chunkLength)) {
						lastDeallocatedChunk_ = upper;
						break;
					} else if(++upper == upperBound)
						upper = 0;
				}
			}

			assert(lastDeallocatedChunk_ != 0);
			assert(lastDeallocatedChunk_->isContaining(p, chunkLength));
			lastDeallocatedChunk_->deallocate(p, blockSize_);
			if(lastDeallocatedChunk_->getAvailableBlockCount() == blockCount_) {
				Chunk* lastChunk = &chunks_.back();
				if(lastChunk == lastDeallocatedChunk_) {
					if(chunks_.size() > 1 && lastDeallocatedChunk_[-1].getAvailableBlockCount() == blockCount_) {
						lastChunk->release();
						chunks_.pop_back();
						lastAllocatedChunk_ = lastDeallocatedChunk_ = &chunks_.front();
					}
					return;
				}
				if(lastChunk->getAvailableBlockCount() == blockCount_) {
					lastChunk->release();
					chunks_.pop_back();
					lastAllocatedChunk_ = lastDeallocatedChunk_;
				} else {
					std::swap(*lastDeallocatedChunk_, *lastChunk);
					lastAllocatedChunk_ = &chunks_.back();
				}
			}
		}
		std::size_t	GetBlockSize() const throw() {return blockSize_;}

		// 演算子
	public:
		FixedAllocator& operator =(const FixedAllocator& rhs) {
			FixedAllocator copy(rhs);
			std::swap(blockSize_, copy.blockSize_);
			std::swap(blockCount_, copy.blockCount_);
			chunks_.swap(copy.chunks_);
			std::swap(lastAllocatedChunk_, copy.lastAllocatedChunk_);
			std::swap(lastDeallocatedChunk_, copy.lastDeallocatedChunk_);
			return *this;
		}
		bool operator <(std::size_t rhs) const throw() {return blockSize_ < rhs;}
	private:

		// 内部クラス
	private:
		class Chunk {
		public:
			Chunk(std::size_t blockSize, SizeType blockCount) : firstAvailableBlock_(0), availableBlockCount_(blockCount) {
				assert(blockSize > 0 && blockCount > 0);
				assert((blockSize * blockCount) / blockSize == blockCount);
				data_ = new SizeType[blockSize * blockCount];
				SizeType* p = data_;
				for(SizeType i = 0; i < blockCount; p += blockSize)
					*p = ++i;
			}
			void* allocate(std::size_t size) {
				if(availableBlockCount_ == 0)
					return 0;
				assert((firstAvailableBlock_ * size) / size == firstAvailableBlock_);
				SizeType* p = data_ + firstAvailableBlock_ * size;
				firstAvailableBlock_ = *p;
				--availableBlockCount_;
				return p;
			}
			void deallocate(void* p, std::size_t blockSize) {
				assert(p >= data_);
				SizeType* releasing = static_cast<SizeType*>(p);
				assert((releasing - data_) % blockSize == 0);
				*releasing = firstAvailableBlock_;
				firstAvailableBlock_ = static_cast<SizeType>((releasing - data_) / blockSize);
				assert(firstAvailableBlock_ == (releasing - data_) / blockSize);
				++availableBlockCount_;
			}
			SizeType getAvailableBlockCount() const throw() {return availableBlockCount_;}
			bool isAvailable() const throw() {return availableBlockCount_ > 0;}
			bool isContaining(const void* p, std::size_t size) const throw() {return p >= data_ && p < data_ + size;}
			void release() {delete[] data_;}
		private:
			SizeType*	data_;
			SizeType	firstAvailableBlock_;
			SizeType	availableBlockCount_;
		};

		// データメンバ
	private:
		typedef std::vector<Chunk> ChunkVector;
		std::size_t	blockSize_;
		SizeType	blockCount_;
		ChunkVector	chunks_;
		Chunk*		lastAllocatedChunk_;
		Chunk*		lastDeallocatedChunk_;
		mutable const FixedAllocator*	next_;
		mutable const FixedAllocator*	prev_;
	};

	template<std::size_t chunkSize, std::size_t maxSmallObjectSize> class SmallObjectAllocator : public Noncopyable {
		friend class SmallObject<>;
	private:
		SmallObjectAllocator() : lastAllocated_(0), lastDeallocated_(0) {}
//		~SmallObjectAllocator() {}
	public:
		void* allocate(std::size_t bytes) {
			if(bytes > maxSmallObjectSize)
				return ::operator new(bytes);
			else if(lastAllocated_ != 0 && lastAllocated_->getBlockSize() == bytes)
				return lastAllocated_->allocate();
			std::vector<FixedAllocator>::iterator it = std::lower_bound(pools_.begin(), pools_.end(), bytes);
			if(it == pools_.end() || it->getBlockSize() != bytes) {
				it = pools_.insert(it, FixedAllocator(bytes));
				lastDeallocated_ = &*pools_.begin();
			}
			lastAllocated_ = &*it;
			return lastAllocated_->allocate();
		}
		void deallocate(void* p, std::size_t bytes) {
			if(bytes > maxSmallObjectSize) {
				::operator delete(p);
				return;
			} else if(lastDeallocated_ != 0 && lastDeallocated_->getBlockSize() == bytes) {
				lastDeallocated_->deallocate(p);
				return;
			}		
			std::vector<FixedAllocator>::iterator it = std::lower_bound(pools_.begin(), pools_.end(), bytes);
			assert(it != pools_.end() && it->getBlockSize() == bytes);
			lastDeallocated_ = &*it;
			lastDeallocated_->deallocate(p);
		}
	private:
		std::vector<FixedAllocator> pools_;
		FixedAllocator* lastAllocated_;
		FixedAllocator* lastDeallocated_;
	};
}

#endif /* !MANAH_SMALL_OBJECT_HPP */
