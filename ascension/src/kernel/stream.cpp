/**
 * @file stream.cpp
 * @author exeal
 * @date 2009 separated from document.cpp
 * @date 2010-2012
 */

#include <ascension/kernel/document-stream.hpp>
using namespace ascension::kernel;
using namespace std;
using ascension::text::Newline;


// DocumentBuffer /////////////////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param document The document
 * @param initialPosition The initial position of streams
 * @param nlr The newline representation
 * @param streamMode The streaming mode. this can be @c std#ios_base#in and @c std#ios_base#out
 * @throw UnknownValueException @a streamMode is invalid
 */
DocumentBuffer::DocumentBuffer(Document& document, const Position& initialPosition /* = Position::ZERO_POSITION */,
		Newline newline /* = NLF_RAW_VALUE */, ios_base::openmode streamMode /* = ios_base::in | ios_base::out */) :
		document_(document), newline_(newline), mode_(streamMode), current_(initialPosition) {
	if((mode_ & ~(ios_base::in | ios_base::out)) != 0)
		throw UnknownValueException("streamMode");
	setp(buffer_.data(), buffer_.data() + buffer_.size()/*tuple_size<decltype(buffer_)>::value*/ - 1);
}

/// Destructor.
DocumentBuffer::~DocumentBuffer() /*throw()*/ {
	sync();
}

/// Returns the current position in the document.
const Position& DocumentBuffer::tell() const /*throw()*/ {
	return current_;
}

/// @see std#basic_streambuf#overflow
DocumentBuffer::int_type DocumentBuffer::overflow(int_type c) {
	if((mode_ & ios_base::out) == 0)
		return traits_type::eof();
	char_type* p = pptr();
	if(!traits_type::eq_int_type(c, traits_type::eof()))
		*p++ = traits_type::to_char_type(c);
	setp(buffer_.data(), buffer_.data() + buffer_.size()/*tuple_size<decltype(buffer_)>::value*/ - 1);
	if(buffer_.data() < p)
		insert(document_, current_, StringPiece(buffer_.data(), p), &current_);
	return traits_type::not_eof(c);
}

/// @see std#basic_streambuf#sync
int DocumentBuffer::sync() {
	if((mode_ & ios_base::out) != 0)
		return traits_type::eq_int_type(overflow(traits_type::eof()), traits_type::eof()) ? -1 : 0;
	else
		return 0;
}

/// @see std#basic_streambuf#uflow
DocumentBuffer::int_type DocumentBuffer::uflow() {
	if(gptr() != egptr()) {
		const int_type temp = traits_type::to_int_type(*gptr());
		gbump(1);
		return temp;
	} else
		return traits_type::eof();
}

/// @see std#basic_streambuf#underflow
DocumentBuffer::int_type DocumentBuffer::underflow() {
	return (gptr() != egptr()) ? traits_type::to_int_type(*gptr()) : traits_type::eof();
}


// document stream classes ////////////////////////////////////////////////////////////////////////

/// Constructor.
DocumentInputStream::DocumentInputStream(Document& document, const Position& initialPosition /* = Position::ZERO_POSITION */,
		Newline newline /* = NLF_RAW_VALUE */) : basic_istream<Char>(&buffer_), buffer_(document, initialPosition, newline) {
}

/// Constructor.
DocumentOutputStream::DocumentOutputStream(Document& document, const Position& initialPosition /* = Position::ZERO_POSITION */,
		Newline newline /* = NLF_RAW_VALUE */) : basic_ostream<Char>(&buffer_), buffer_(document, initialPosition, newline) {
}

/// Constructor.
DocumentStream::DocumentStream(Document& document, const Position& initialPosition /* = Position::ZERO_POSITION */,
		Newline newline /* = NLF_RAW_VALUE */) : basic_iostream<Char>(&buffer_), buffer_(document, initialPosition, newline) {
}
