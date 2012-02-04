/**
 * @file memory.hpp
 * This file provides the following classes:
 * - AutoBuffer
 * - MemoryPool
 * - FastArenaObject
 * @author exeal
 * @date 2005-2010 (was manah/memory.hpp)
 * @date 2010-10-21
 * @date 2011-2012
 */

#ifndef ASCENSION_MEMORY_HPP
#define ASCENSION_MEMORY_HPP
#include <ascension/corelib/basic-types.hpp>	// ASCENSION_NONCOPYABLE_TAG
#include <cassert>
#include <algorithm>	// std.max
#include <new>			// new[], delete[], std.bad_alloc, std.nothrow
#include <cstddef>		// std.size_t
#include <memory>		// std.unique_ptr
#undef min
#undef max

namespace ascension {

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
	// std.auto_ptr for arrays.
	template<typename T> class AutoBuffer {
	public:
		/// The element type.
		typedef T element_type;
	public:
		// constructors
		explicit AutoBuffer(element_type* p = nullptr) /*throw()*/ : buffer_(p) {}
		AutoBuffer(AutoBuffer<element_type>& other) /*throw()*/ : buffer_(other.release()) {}
		template<typename Other>
		AutoBuffer(AutoBuffer<Other>& other) /*throw()*/ : buffer_(other.release()) {}
		/// Destructor deallocates the buffer by using @c #reset.
		~AutoBuffer() /*throw()*/ {delete[] buffer_;}
		/// Assignment operator.
		AutoBuffer& operator=(AutoBuffer& other) /*throw()*/ {
			reset(other.release());
			return *this;
		}
		/// Assignment operator.
		template<typename Other>
		AutoBuffer& operator=(AutoBuffer<Other>& other) /*throw()*/ {
			reset(other.release());
			return *this;
		}
		element_type& operator[](std::ptrdiff_t i) const /*throw()*/ {return buffer_[i];}
		/// Returns the pointer to the buffer or @c null.
		element_type* get() const /*throw()*/ {return buffer_;}
		/**
		 * Sets the internal pointer to @c null without deallocation the buffer currently pointed
		 * by @c AutoBuffer.
		 * @retval A pointer to the buffer pointed by @c AutoBuffer before.
		 */
		element_type* release() /*throw()*/ {
			element_type* const temp = buffer_;
			buffer_ = nullptr;
			return temp;
		}
		/**
		 * Deallocates the buffer pointed by @c AutoBuffer (if any). And initializes with the given
		 * new buffer.
		 * @param p The new buffer or @c null
		 * @note This method does not destruct the each elements in the buffer.
		 */
		void reset(element_type* p = nullptr) {if(p != buffer_) {delete[] buffer_; buffer_ = p;}}
		/// Swaps two @c AutoBuffer objects.
		void swap(AutoBuffer<element_type>& other) /*throw()*/ {std::swap(buffer_, other.buffer_);}
	private:
		element_type* buffer_;
	};

	template<typename T>
	inline void swap(AutoBuffer<T>& left, AutoBuffer<T>& right) {return left.swap(right);}
#endif // ASCENSION_ABANDONED_AT_VERSION_08

	// Efficient memory pool implementation (from MemoryPool of Efficient C++).
	class MemoryPool {
		ASCENSION_NONCOPYABLE_TAG(MemoryPool);
	public:
		MemoryPool(std::size_t chunkSize) /*throw()*/ : chunkSize_(std::max(chunkSize, sizeof(Chunk))), chunks_(nullptr) {}
		~MemoryPool() /*throw()*/ {release();}
		void* allocate() {
			if(void* const chunk = allocate(std::nothrow))
				return chunk;
			throw std::bad_alloc();
		}
		void* allocate(const std::nothrow_t&) {
			if(chunks_ == nullptr)
				expandChunks();
			if(Chunk* head = chunks_) {
				chunks_ = head->next;
				return head;
			}
			return nullptr;
		}
		void deallocate(void* doomed) /*throw()*/ {
			if(Chunk* p = static_cast<Chunk*>(doomed)) {
				p->next = chunks_;
				chunks_ = p;
			}
		}
		void release() /*throw()*/ {
			for(Chunk* next = chunks_; next != nullptr; next = chunks_) {
				chunks_ = chunks_->next;
				::operator delete(next);
			}
		}
	private:
		void expandChunks() /*throw()*/ {
			assert(chunks_ == nullptr);
			Chunk* p = static_cast<Chunk*>(::operator new(chunkSize_, std::nothrow));
			if(p == nullptr)
				return;
			Chunk* q = p;
			p->next = nullptr;
			int i;
			for(i = 0; i < NUMBER_OF_CHUNKS_TO_EXPAND_AT_ONCE; ++i) {
				if(nullptr == (p->next = static_cast<Chunk*>(::operator new(chunkSize_, std::nothrow))))
					break;
				p = p->next;
				p->next = nullptr;
			}
			if(i != NUMBER_OF_CHUNKS_TO_EXPAND_AT_ONCE) {
				Chunk* next;
				do {
					next = q->next;
					::operator delete(q);
					q = next;
				} while(next != nullptr);
				return;
			}
			chunks_ = q;
		}
	private:
		struct Chunk {Chunk* next;};
		enum {NUMBER_OF_CHUNKS_TO_EXPAND_AT_ONCE = 32};
		const std::size_t chunkSize_;
		Chunk* chunks_;
	};

	// Base class for types whose new and delete are fast.
	template<typename T> /* final */ class FastArenaObject {
	public:
		static void* operator new(std::size_t bytes) /*throw(std::bad_alloc)*/ {
			if(pool_.get() == nullptr) {
				pool_.reset(new(std::nothrow) MemoryPool(std::max(sizeof(T), bytes)));
				if(pool_.get() == nullptr)
					throw std::bad_alloc();
			}
			if(void* p = pool_->allocate())
				return p;
			throw std::bad_alloc();
		}
		static void* operator new(std::size_t, const std::nothrow_t&) /*throw()*/ {
			if(pool_.get() == nullptr)
				pool_.reset(new(std::nothrow) MemoryPool(sizeof(T)));
			return (pool_.get() != nullptr) ? pool_->allocate(std::nothrow) : nullptr;
		}
		static void* operator new(std::size_t bytes, void* where) /*throw()*/ {return ::operator new(bytes, where);}
		static void operator delete(void* p) /*throw()*/ {if(pool_.get() != nullptr) pool_->deallocate(p);}
		static void operator delete(void* p, const std::nothrow_t&) /*throw()*/ {return operator delete(p);}
		static void operator delete(void* p, void* where) /*throw()*/ {return ::operator delete(p, where);}
	private:
		static std::unique_ptr<MemoryPool> pool_;
	};

	template<typename T> std::unique_ptr<MemoryPool> FastArenaObject<T>::pool_;

} // namespace ascension

#endif // !ASCENSION_MEMORY_HPP
