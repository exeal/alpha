// memory.hpp
// (c) 2005-2007 exeal

#ifndef MANAH_MEMORY_HPP
#define MANAH_MEMORY_HPP

// This file contains following classes
//	AutoBuffer
//	SharedPointer
//	MemoryPool
//	FastArenaObject

#include <algorithm>	// std::max
#include <new>			// new[], delete[], std::bad_alloc, std::nothrow
#include <cstddef>		// std::size_t
#include <memory>		// std::auto_ptr


namespace manah {

	// AutoBuffer ///////////////////////////////////////////////////////////

	// std::auto_ptr for arrays
	template<typename T> /* final */ class AutoBuffer {
	public:
		// type
		typedef T ElementType;
		// constructors
		explicit AutoBuffer(ElementType* p = 0) throw() : buffer_(p) {}
		AutoBuffer(AutoBuffer& rhs) throw() : buffer_(rhs.release()) {}
		template<typename Other> AutoBuffer(AutoBuffer<Other>& rhs) throw() : buffer_(rhs.release()) {}
		~AutoBuffer() throw() {delete[] buffer_;}
		// operators
		AutoBuffer& operator=(AutoBuffer& rhs) throw() {reset(rhs.release()); return *this;}
		template<typename Other> AutoBuffer& operator=(AutoBuffer<Other>& rhs) throw() {reset(rhs.release()); return *this;}
		ElementType& operator[](int i) const throw() {return buffer_[i];}
		ElementType& operator[](std::size_t i) const throw() {return buffer_[i];}
		// methods
		ElementType* get() const throw() {return buffer_;}
		ElementType* release() throw() {ElementType* const temp = buffer_; buffer_ = 0; return temp;}
		void reset(ElementType* p = 0) {if(p != buffer_) {delete[] buffer_; buffer_ = p;}}
	private:
		ElementType* buffer_;
	};


	// MemoryPool ///////////////////////////////////////////////////////////

	// efficient memory pool implementation (from MemoryPool of Efficient C++).
	// template paramater of Allocator always bound to char
	class MemoryPool {
	public:
		MemoryPool(std::size_t chunkSize) : chunkSize_(std::max(chunkSize, sizeof(Chunk))), chunks_(0) {expandChunks();}
		~MemoryPool() throw() {release();}
		void* allocate() {
			if(chunks_ == 0)
				expandChunks();
			if(Chunk* head = chunks_) {
				chunks_ = head->next;
				return head;
			}
			return 0;
		}
		void deallocate(void* doomed) throw() {
			if(Chunk* p = static_cast<Chunk*>(doomed)) {
				p->next = chunks_;
				chunks_ = p;
			}
		}
		void release() throw() {
			for(Chunk* next = chunks_; next != 0; next = chunks_) {
				chunks_ = chunks_->next;
				::operator delete(next);
			}
		}
	private:
		void expandChunks() {
			assert(chunks_ == 0);
			Chunk* p = static_cast<Chunk*>(::operator new(chunkSize_, std::nothrow));
			if(p == 0)
				return;
			Chunk* q = p;
			p->next = 0;
			int i;
			for(i = 0; i < NUMBER_OF_CHUNKS_TO_EXPAND_AT_ONCE; ++i) {
				if(0 == (p->next = static_cast<Chunk*>(::operator new(chunkSize_, std::nothrow))))
					break;
				p = p->next;
				p->next = 0;
			}
			if(i != NUMBER_OF_CHUNKS_TO_EXPAND_AT_ONCE) {
				Chunk* next;
				do {
					next = q->next;
					::operator delete(q);
					q = next;
				} while(next != 0);
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


	// FastArenaObject //////////////////////////////////////////////////////

	template<typename T> /* final */ class FastArenaObject {
	public:
		static void* operator new(std::size_t bytes) {
			if(pool_.get() == 0) {
				pool_.reset(new(std::nothrow) MemoryPool(std::max(sizeof(T), bytes)));
				if(pool_.get() == 0)
					throw std::bad_alloc();
			}
			if(void* p = pool_->allocate())
				return p;
			throw std::bad_alloc();
		}
		static void* operator new(std::size_t, const std::nothrow_t&) throw() {
			if(pool_.get() == 0)
				pool_.reset(new(std::nothrow) MemoryPool(sizeof(T)));
			return (pool_.get() != 0) ? pool_->allocate() : 0;
		}
		static void operator delete(void* p) throw() {if(pool_.get() != 0) pool_->deallocate(p);}
	private:
		static std::auto_ptr<MemoryPool> pool_;
	};

	template<typename T> std::auto_ptr<MemoryPool> FastArenaObject<T>::pool_;


	// SharedPointer ////////////////////////////////////////////////////////

	// reference-counted smart pointer (from boost.shared_ptr)
	template<typename T> /* final */ class SharedPointer {
	public:
		//types
		typedef T ValueType;
		typedef T* Pointer;
		typedef T& Reference;
		// constructors
		explicit SharedPointer(Pointer p = 0) : data_((p != 0) ? new Data(*p) : 0) {}
		~SharedPointer() {reset();}
		SharedPointer(const SharedPointer& rhs) : data_(rhs.data_) {if(data_ != 0) data_->addReference();}
		// operators
		SharedPointer& operator=(const SharedPointer& rhs) {
			if(data_ != rhs.data_) {
				reset();
				data_ = rhs.data_;
				if(data_ != 0)
					data_->addReference();
			}
			return *this;
		}
		Reference operator*() const {const Pointer p(get()); assert(p != 0); return *p;}
		Pointer operator->() const {const Pointer p(get()); assert(p != 0); return p;}
		// methods
		Pointer get() const throw() {return (data_ != 0) ? data_->get() : 0;}
		void reset(Pointer p = 0) {
			if(p == get())
				return;
			if(data_ != 0) {
				data_->release();
				if(data_->get() == 0)
					delete data_;
			}
			data_ = (p != 0) ? new Data(*p) : 0;
		}
	private:
		class Data : public FastArenaObject<Data> {
		public:
			Data(Reference p) throw() : p_(&p), c_(1) {}
			void addReference() throw() {++c_;}
			Pointer get() const throw() {return p_;}
			void release() {if(--c_ == 0) {delete p_; p_ = 0;}}
		private:
			Pointer p_;
			long c_;
		} * data_;
	};

} // namespace manah

#endif /* MANAH_MEMORY_HPP */
