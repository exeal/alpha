// text-buffer-test.cpp

#include "stdafx.h"
#include "../../../manah/memory.hpp"
#include <string>


template<class Character>
class TextBuffer {
public:
	// types
	typedef std::size_t SizeType;
	typedef std::ptrdiff_t DifferenceType;
	typedef Character ValueType;
	typedef Character* Pointer;
	typedef Character& Reference;
	typedef const Character* ConstPointer;
	typedef const Character& ConstReference;
	struct Position {
		Chunk* chunk;
		std::ptrdiff_t offset;
	};
	// constructors
	TextBuffer() : pool_(sizeof(Character) * CHUNK_SIZE), head_(new Chunk), tail_(head_), length_(0) {
		head_->data = head_->gapFirst = static_cast<Pointer>(pool_.allocate());
		head_->gapLast = head_->data + CHUNK_SIZE;
		head_->next = head_->previous = 0;
	}
	~TextBuffer() {
		Chunk* next;
		for(Chunk* p = head_; p != 0; p = next) {
			next = p->next;
			pool_.deallocate(p->data);
			delete p;
		}
	}
	// attributes
	SizeType getLength() const throw() {return length_;}
	std::basic_string<ValueType> getString() const throw() {
		std::basic_string<ValueType> s;
		s.reserve(length_);
		for(Chunk* p = head_; p != 0; p = p->next) {
			s.append(p->data, p->gapFirst);
			s.append(p->gapLast, p->data + CHUNK_SIZE);
		}
		return s;
	}
	// operations
	void erase(SizeType start, DifferenceType length) {
	}
	void insert(const Position& at, ConstPointer first, ConstPointer last) {
		if(first == last)
			return;
		assert(last > first);
		if(last - first > at.chunk.gapLast - at.chunk.gapFirst) {
		}
		length_ += last - first;
	}
	void replace(SizeType start, DifferenceType length, ConstPointer first, ConstPointer last) {
	}
private:
private:
	static const std::size_t CHUNK_SIZE = 4096;
	struct Chunk {
		Chunk* next;
		Chunk* previous;
		Pointer data, gapFirst, gapLast;
	};
	manah::MemoryPool<> pool_;
	Chunk* head_;
	Chunk* tail_;
	SizeType length_;
};


void testTextBuffer() {
	const char s[] = "This is my first text.";
	TextBuffer<char> tb;
	tb.insert(0, s, endof(s) - 1);
	BOOST_CHECK_EQUAL(tb.getString(), s);
}