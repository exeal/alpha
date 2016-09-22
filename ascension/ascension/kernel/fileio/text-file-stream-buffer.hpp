/**
 * @file text-file-stream-buffer.hpp
 * Defines @c TextFileStreamBuffer class.
 * @author exeal
 * @date 2009 separated from document.hpp
 * @date 2016-09-21 Separated from fileio.hpp.
 */

#ifndef ASCENSION_TEXT_FILE_STREAM_BUFFER_HPP
#define ASCENSION_TEXT_FILE_STREAM_BUFFER_HPP
#include <ascension/platforms.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-input.hpp>
#include <ascension/corelib/encoder.hpp>	// encoding.Encoder.*
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/range/const_iterator.hpp>
#include <array>

namespace ascension {
	namespace kernel {
		namespace fileio {
			/**
			 * The encoding failed for unmappable character.
			 * @see encoding#Encoder#UNMAPPABLE_CHARACTER, text#MalformedInputException
			 */
			class UnmappableCharacterException : public std::ios_base::failure {
			public:
				UnmappableCharacterException();
			};

			/**
			 * @c std#basic_streambuf implementation of the text file with encoding conversion.
			 * @note This class is not intended to be subclassed.
			 */
			class TextFileStreamBuffer : public std::basic_streambuf<Char>, private boost::noncopyable {
			public:
				TextFileStreamBuffer(const boost::filesystem::path& fileName,
					std::ios_base::openmode mode, const std::string& encoding,
					encoding::Encoder::SubstitutionPolicy encodingSubstitutionPolicy,
					bool writeUnicodeByteOrderMark);
				~TextFileStreamBuffer();
				TextFileStreamBuffer* close();
				TextFileStreamBuffer* closeAndDiscard();
				std::string encoding() const BOOST_NOEXCEPT;
				const boost::filesystem::path& fileName() const BOOST_NOEXCEPT;
				bool isOpen() const BOOST_NOEXCEPT;
				std::ios_base::openmode mode() const BOOST_NOEXCEPT;
				bool unicodeByteOrderMark() const BOOST_NOEXCEPT;
			private:
				void buildEncoder(const std::string& encoding, bool detectEncoding);
				void buildInputMapping();
				TextFileStreamBuffer* closeFile() BOOST_NOEXCEPT;
				void openForReading(const std::string& encoding);
				void openForWriting(const std::string& encoding, bool writeUnicodeByteOrderMark);
				// std.basic_streambuf
				int_type overflow(int_type c /* = traits_type::eof() */) override;
				int_type pbackfail(int_type c /* = traits_type::eof() */) override;
				int sync();
				int_type underflow();
			private:
				typedef std::basic_streambuf<Char> Base;
#if BOOST_OS_WINDOWS
				HANDLE fileHandle_, fileMapping_;
#else // ASCENSION_OS_POSIX
				int fileDescriptor_;
#endif
				const boost::filesystem::path fileName_;
				std::ios_base::openmode mode_;
				struct InputMapping {
					boost::iterator_range<const Byte*> buffer;
					const Byte* current;
					InputMapping() BOOST_NOEXCEPT : buffer(nullptr, nullptr), current(nullptr) {}
				} inputMapping_;
#if BOOST_OS_WINDOWS
				LARGE_INTEGER originalFileEnd_;
#else // ASCENSION_OS_POSIX
				off_t originalFileEnd_;
#endif
				std::unique_ptr<encoding::Encoder> encoder_;
				std::array<Char, 8192> ucsBuffer_;
			};

			/// Returns the file name.
			inline const boost::filesystem::path& TextFileStreamBuffer::fileName() const BOOST_NOEXCEPT {
				return fileName_;
			}

			/// Returns the open mode.
			inline std::ios_base::openmode TextFileStreamBuffer::mode() const BOOST_NOEXCEPT {
				return mode_;
			}

			namespace detail {
				boost::filesystem::filesystem_error makeGenericFileSystemError(
					const std::string& what, const boost::filesystem::path& path, boost::system::errc::errc_t value);
				boost::filesystem::filesystem_error makeFileSystemError(const std::string& what,
					const boost::filesystem::path& path, const boost::optional<int>& value = boost::none);
			}
		}
	}
}	// namespace ascension.kernel.fileio

#endif // !ASCENSION_TEXT_FILE_STREAM_BUFFER_HPP
