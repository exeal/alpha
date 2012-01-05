/**
 * @file document-stream.hpp
 * @author exeal
 * @date 2009 separated from document.hpp
 * @date 2003-2011
 */

#ifndef ASCENSION_STREAM_HPP
#define ASCENSION_STREAM_HPP
#include <ascension/kernel/document.hpp>
#include <array>
#include <iostream>		// std.basic_istream, std.basic_istream, std.basic_iostream
#include <streambuf>	// std.basic_streambuf

namespace ascension {
	namespace kernel {

		/**
		 * @c std#basic_streambuf implementation for @c Document. This supports both input and
		 * output streams. Seeking is not supported. Virtual methods this class overrides are:
		 * - @c overflow
		 * - @c sync
		 * - @c uflow
		 * - @c underflow
		 * Destructor automatically flushes the internal buffer.
		 * @note This class is not intended to be subclassed.
		 */
		class DocumentBuffer : public std::basic_streambuf<Char> {
		public:
			explicit DocumentBuffer(Document& document,
				const Position& initialPosition = Position(0, 0), text::Newline newline = text::NLF_RAW_VALUE,
				std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out);
			~DocumentBuffer() /*throw()*/;
			const Position&	tell() const /*throw()*/;
		private:
			int_type overflow(int_type c);
			int sync();
			int_type uflow();
			int_type underflow();
		private:
			Document& document_;
			const text::Newline newline_;
			const std::ios_base::openmode mode_;
			Position current_;
			std::array<char_type, 8192> buffer_;
		};

		/// Input stream for @c Document.
		class DocumentInputStream : public std::basic_istream<Char> {
		public:
			explicit DocumentInputStream(Document& document,
				const Position& initialPosition = Position(0, 0), text::Newline newline = text::NLF_RAW_VALUE);
			DocumentBuffer* rdbuf() const;
		private:
			DocumentBuffer buffer_;
		};

		/// Output stream for @c Document.
		class DocumentOutputStream : public std::basic_ostream<Char> {
		public:
			explicit DocumentOutputStream(Document& document,
				const Position& initialPosition = Position(0, 0), text::Newline newline = text::NLF_RAW_VALUE);
			DocumentBuffer* rdbuf() const;
		private:
			DocumentBuffer buffer_;
		};

		/// Stream for @c Document.
		class DocumentStream : public std::basic_iostream<Char> {
		public:
			explicit DocumentStream(Document& document,
				const Position& initialPosition = Position(0, 0), text::Newline newline = text::NLF_RAW_VALUE);
			DocumentBuffer* rdbuf() const;
		private:
			DocumentBuffer buffer_;
		};

		/// Returns the stored stream buffer.
		inline DocumentBuffer* DocumentInputStream::rdbuf() const {return const_cast<DocumentBuffer*>(&buffer_);}

		/// Returns the stored stream buffer.
		inline DocumentBuffer* DocumentOutputStream::rdbuf() const {return const_cast<DocumentBuffer*>(&buffer_);}

		/// Returns the stored stream buffer.
		inline DocumentBuffer* DocumentStream::rdbuf() const {return const_cast<DocumentBuffer*>(&buffer_);}

	}	// namespace kernel
}	// namespace ascension

#endif // !ASCENSION_STREAM_HPP
