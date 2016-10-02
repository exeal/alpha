/**
 * @file text-file-stream-buffer.cpp
 * Implements @c TextFileStreamBuffer and @c UnmappableCharacterException classes.
 * @author exeal
 * @date 2007 (separated from document.cpp)
 * @date 2016-09-21 Separated from fileio.cpp.
 */

#include <ascension/corelib/encoding/encoder-factory.hpp>
#include <ascension/corelib/encoding/encoding-detector.hpp>
#include <ascension/kernel/fileio/text-file-stream-buffer.hpp>
#if ASCENSION_OS_POSIX
#	include <sys/mman.h>	// mmap, munmap, ...
#endif // !ASCENSION_OS_POSIX
#if BOOST_OS_WINDOWS
#	include <cwctype>
#	include <limits>	// std.numeric_limits
#endif


namespace ascension {
	namespace kernel {
		namespace fileio {
			// exception classes //////////////////////////////////////////////////////////////////////////////////////

			/// Default constructor.
			UnmappableCharacterException::UnmappableCharacterException() : std::ios_base::failure("encountered an unmappable character in encoding/decoding.") {
			}


			// TextFileStreamBuffer ///////////////////////////////////////////////////////////////////////////////////

			namespace {
				class SystemErrorSaver {
				public:
					SystemErrorSaver() BOOST_NOEXCEPT : code_(makePlatformError().code().value()) {}
#if BOOST_OS_WINDOWS
					~SystemErrorSaver() BOOST_NOEXCEPT {::SetLastError(code_);}
#else // ASCENSION_OS_POSIX
					~SystemErrorSaver() BOOST_NOEXCEPT {errno = code_;}
#endif
					std::error_code::value_type code() const BOOST_NOEXCEPT {return code_;}
				private:
					std::error_code::value_type code_;
				};
			} // namespace @0

			/**
			 * Constructor opens the specified file.
			 * @param fileName The name of the file
			 * @param mode The file open mode. valid values are the following:
			 *   <dl>
			 *     <dt>@c std#ios_base#in</dt>
			 *     <dd>Opens the existing file for reading.</dd>
			 *     <dt>@c std#ios_base#out</dt>
			 *     <dt>@c std#ios_base#out | @c std#ios_base#trunc</dt>
			 *     <dd>Truncates the existing file or creates for writing.</dd>
			 *     <dt>@c std#ios_base#out | @c std#ios_base#app</dt>
			 *     <dd>Opens the existing file for appending all writes. If the file was not existing, this is
			 *         same as @c std#ios_base#out.</dd>
			 *   </dl>
			 * @param encoding The file encoding or auto detection name
			 * @param encodingSubstitutionPolicy The substitution policy used in encoding conversion
			 * @param writeUnicodeByteOrderMark Aet @c true to write Unicode byte order mark into the file. This
			 *                                  parameter is ignored if @a mode contained @c std#ios_base#app and the
			 *                                  output file was existing
			 * @throw UnknownValueException @a mode or @a encodingSubstitutionPolicy is invalid
			 * @throw UnsupportedEncodingException The encoding specified by @a encoding is not supported
			 * @throw PlatformDependentIOError
			 */
			TextFileStreamBuffer::TextFileStreamBuffer(const boost::filesystem::path& fileName, std::ios_base::openmode mode,
					const std::string& encoding, encoding::Encoder::SubstitutionPolicy encodingSubstitutionPolicy,
					bool writeUnicodeByteOrderMark) : fileName_(fileName), mode_(mode) {
//				sanityCheckPathName(fileName, "fileName");
				if(mode == std::ios_base::in)
					openForReading(encoding);
				else if(mode == std::ios_base::out
						|| mode == (std::ios_base::out | std::ios_base::trunc)
						|| mode == (std::ios_base::out | std::ios_base::app))
					openForWriting(encoding, writeUnicodeByteOrderMark);
				else
					throw UnknownValueException("mode");
				assert(encoder_.get() != nullptr);
				encoder_->setSubstitutionPolicy(encodingSubstitutionPolicy);
			}

			/// Destructor closes the file.
			TextFileStreamBuffer::~TextFileStreamBuffer() {
				try {
					close();	// this may throw
				} catch(...) {
				}
			}

			void TextFileStreamBuffer::buildEncoder(const std::string& encoding, bool detectEncoding) {
				assert(encoder_.get() == nullptr);
				encoder_ = encoding::EncoderRegistry::instance().forName(encoding);
				encodingState_ = decodingState_ = encoding::Encoder::State();
				if(encoder_.get() != nullptr)
					return;
				else if(detectEncoding) {
					if(const std::shared_ptr<const encoding::EncodingDetector> detector = encoding::EncodingDetector::forName(encoding)) {
						const auto detected(detector->detect(
							boost::make_iterator_range(
								std::begin(inputMapping_.buffer),
								std::min(std::end(inputMapping_.buffer), std::begin(inputMapping_.buffer) + 1024 * 10))));
						if(std::get<0>(detected) != encoding::MIB_OTHER)
							encoder_ = encoding::EncoderRegistry::instance().forMIB(std::get<0>(detected));
						else
							encoder_ = encoding::EncoderRegistry::instance().forName(std::get<1>(detected));
						if(encoder_.get() != nullptr)
							return;	// resolved
					}
				}
				throw encoding::UnsupportedEncodingException(encoding);
			}

			void TextFileStreamBuffer::buildInputMapping() {
				assert(isOpen());
				const std::uintmax_t fileSize = boost::filesystem::file_size(fileName());
#if BOOST_OS_WINDOWS
				if(fileSize != 0) {
					fileMapping_ = ::CreateFileMappingW(fileHandle_, nullptr, PAGE_READONLY, 0, 0, nullptr);
					if(fileMapping_ == nullptr)
						throw detail::makeFileSystemError("CreateFileMappingW() returned nullptr.", fileName());
					inputMapping_.buffer = boost::make_iterator_range<const Byte*>(static_cast<const Byte*>(::MapViewOfFile(fileMapping_, FILE_MAP_READ, 0, 0, 0)), nullptr);
					if(std::begin(inputMapping_.buffer) == nullptr) {
						SystemErrorSaver ses;
						::CloseHandle(fileMapping_);
						throw detail::makeFileSystemError("MapViewOfFile() returned nullptr.", fileName());
					}
				} else
					fileMapping_ = nullptr;
#else // ASCENSION_OS_POSIX
				inputMapping_.buffer = boost::make_iterator_range<const Byte*>(static_cast<const Byte*>(::mmap(0, fileSize, PROT_READ, MAP_PRIVATE, fileDescriptor_, 0)), nullptr);
				if(std::begin(inputMapping_.buffer) == MAP_FAILED)
					throw detail::makeFileSystemError("mmap(2) returned MAP_FAILED.", fileName());
				bool succeeded = false;
				off_t org = ::lseek(fileDescriptor_, 0, SEEK_CUR);
				if(org != -1) {
					off_t end = ::lseek(fileDescriptor_, 0, SEEK_END);
					if(end != -1) {
						::lseek(fileDescriptor_, org, SEEK_SET);
						succeeded = true;
					}
				}
				if(!succeeded)
					throw detail::makeGenericFileSystemError("lseek(2) returned -1.", fileName());
#endif
				inputMapping_.buffer = boost::make_iterator_range(std::begin(inputMapping_.buffer), std::begin(inputMapping_.buffer) + fileSize);
				inputMapping_.current = std::begin(inputMapping_.buffer);
			}

			/**
			 * Closes the file.
			 * @return This or @c null if the file is not open
			 * @throw ... Any exceptions @c #sync throws
			 */
			TextFileStreamBuffer* TextFileStreamBuffer::close() {
				sync();
				return closeFile();
			}

			/**
			 * Closes the file and discard the change.
			 * @return This or @c null if the file is not open
			 * @throw ... Any exceptions @c #close throws when @c #mode returned @c std#ios_base#in
			 */
			TextFileStreamBuffer* TextFileStreamBuffer::closeAndDiscard() {
				if(mode() == std::ios_base::in)
					return close();
				else if((mode() & ~std::ios_base::trunc) == std::ios_base::out) {
					if(TextFileStreamBuffer* const self = closeFile()) {
						boost::filesystem::remove(fileName_);
						return self;
					} else
						return nullptr;
				} else if(mode() == (std::ios_base::out | std::ios_base::app)) {
#if BOOST_OS_WINDOWS
					::SetFilePointerEx(fileHandle_, originalFileEnd_, nullptr, FILE_BEGIN);
					::SetEndOfFile(fileHandle_);
#else // ASCENSION_OS_POSIX
					::lseek(fileDescriptor_, originalFileEnd_, SEEK_SET);
					::ftruncate(fileDescriptor_, originalFileEnd_);
#endif
					return closeFile();
				} else
					ASCENSION_ASSERT_NOT_REACHED();
			}

			TextFileStreamBuffer* TextFileStreamBuffer::closeFile() BOOST_NOEXCEPT {
#if BOOST_OS_WINDOWS
				if(fileMapping_ != nullptr) {
					::UnmapViewOfFile(const_cast<Byte*>(std::begin(inputMapping_.buffer)));
					::CloseHandle(fileMapping_);
					inputMapping_.buffer = boost::make_iterator_range<const Byte*>(nullptr, nullptr);
					fileMapping_ = nullptr;
				}
				if(fileHandle_ != INVALID_HANDLE_VALUE) {
					::CloseHandle(fileHandle_);
					fileHandle_ = INVALID_HANDLE_VALUE;
					return this;
				}
#else // ASCENSION_OS_POSIX
				if(inputMapping_.first != nullptr) {
					::munmap(const_cast<Byte*>(inputMapping_.first), inputMapping_.last - inputMapping_.first);
					inputMapping_.first = nullptr;
				}
				if(fileDescriptor_ == -1) {
					::close(fileDescriptor_);
					fileDescriptor_ = -1;
					return this;
				}
#endif
				if(encoder_.get() != nullptr)
					encodingState_ = decodingState_ = encoding::Encoder::State();
				return nullptr;	// didn't close the file actually
			}

			/**
			 * Returns the character encoding. If the encoding name passed to the constructor was detection
			 * name, returns the detected encoding.
			 */
			std::string TextFileStreamBuffer::encoding() const BOOST_NOEXCEPT {
				return encoder_->properties().name();
			}

			/// Returns @c true if the file is open.
			bool TextFileStreamBuffer::isOpen() const BOOST_NOEXCEPT {
#if BOOST_OS_WINDOWS
				return fileHandle_ != INVALID_HANDLE_VALUE;
#else // ASCENSION_OS_POSIX
				return fileDescriptor_ != -1;
#endif
			}

			// called by only the constructor
			void TextFileStreamBuffer::openForReading(const std::string& encoding) {
				// open the file
#if BOOST_OS_WINDOWS
				fileHandle_ = ::CreateFileW(fileName().c_str(), GENERIC_READ,
					FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
				if(fileHandle_ == INVALID_HANDLE_VALUE)
					throw detail::makeFileSystemError("CreateFileW() returned INVALID_HANDLE_VALUE.", fileName());
#else // ASCENSION_OS_POSIX
				if(-1 == (fileDescriptor_ = ::open(fileName().c_str(), O_RDONLY)))
					throw detail::makeFileSystemError("open(2) returned -1.", fileName());
#endif

				try {
					// create memory-mapped file
					buildInputMapping();
					// detect input encoding if neccssary, and create the encoder
					buildEncoder(encoding, true);
				} catch(...) {
					closeFile();
					throw;
				}
			}

			// called by only the constructor
			void TextFileStreamBuffer::openForWriting(const std::string& encoding, bool writeUnicodeByteOrderMark) {
				if((mode_ & std::ios_base::app) != 0) {
#if BOOST_OS_WINDOWS
					fileHandle_ = ::CreateFileW(fileName_.c_str(), GENERIC_READ | GENERIC_WRITE,
						0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
					if(fileHandle_ == INVALID_HANDLE_VALUE) {
						const DWORD e = ::GetLastError();
						if(e == ERROR_FILE_NOT_FOUND)
							mode_ &= ~std::ios_base::app;
						else
							throw detail::makeFileSystemError("CreateFileW() returned INVALID_HANDLE_VALUE", fileName(), e);
					} else {
						originalFileEnd_.QuadPart = 0;
						if(!win32::boole(::SetFilePointerEx(fileHandle_, originalFileEnd_, &originalFileEnd_, FILE_END)))
							throw detail::makeFileSystemError("SetFilePointerEx() returned FALSE.", fileName());
						writeUnicodeByteOrderMark = false;
					}
#else // ASCENSION_OS_POSIX
					fileDescriptor_ = ::open(fileName_.c_str(), O_WRONLY | O_APPEND);
					if(fileDescriptor_ != -1) {
						originalFileEnd_ = ::lseek(fileDescriptor_, 0, SEEK_CUR);
						if(originalFileEnd_ == static_cast<off_t>(-1))
							throw detail::makeFileSystemError("lseek(2) returned -1.", fileName());
					} else {
						if(errno == ENOENT)
							mode_ &= ~ios_base::app;
						else
							throw detail::makeFileSystemError("open(2) returned -1.", fileName());
					}
#endif
				}
				if((mode_ & ~std::ios_base::trunc) == std::ios_base::out) {
#if BOOST_OS_WINDOWS
					fileHandle_ = ::CreateFileW(fileName().c_str(), GENERIC_WRITE, 0, nullptr,
						CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
					if(fileHandle_ == INVALID_HANDLE_VALUE)
						throw detail::makeFileSystemError("CreateFileW() returned INVALID_HANDLE_VALUE.", fileName());
#else // ASCENSION_OS_POSIX
					fileDescriptor_ = ::open(fileName().c_str(), O_WRONLY | O_CREAT);
					if(fileDescriptor_ == -1)
						throw detail::makeFileSystemError("open(2) returned -1", fileName());
#endif
				}

				try {
					if((mode() & std::ios_base::app) != 0)
						buildInputMapping();
					else
						inputMapping_.buffer = boost::make_iterator_range<const Byte*>(nullptr, nullptr);
					buildEncoder(encoding, (mode() & std::ios_base::app) != 0);
				} catch(...) {
					closeFile();
					throw;
				}

				if(writeUnicodeByteOrderMark)
					encoder_->writeByteOrderMark();
				setp(ucsBuffer_.data(), ucsBuffer_.data() + ucsBuffer_.size());
			}

			/// @see std#basic_streambuf#overflow
			TextFileStreamBuffer::int_type TextFileStreamBuffer::overflow(int_type c) {
				if(std::begin(inputMapping_.buffer) != nullptr || sync() == -1)
					return traits_type::eof();	// not output mode or can't synchronize

				*pptr() = traits_type::to_char_type(c);
				pbump(+1);
				return traits_type::not_eof(c);
			}

			/// @see std#basic_streambuf#pbackfail
			TextFileStreamBuffer::int_type TextFileStreamBuffer::pbackfail(int_type c) {
				if(std::begin(inputMapping_.buffer) != nullptr) {
					if(gptr() > eback()) {
						gbump(-1);
						return traits_type::not_eof(c);	// c is ignored
					}
				}
				return traits_type::eof();
			}

			/// std#basic_streambuf#sync
			int TextFileStreamBuffer::sync() {
				// this method converts ucsBuffer_ into the native encoding and writes
				if(isOpen() && std::begin(inputMapping_.buffer) == nullptr && pptr() > pbase()) {
					Byte* toNext;
					const Char* fromNext;
					std::array<Byte, std::tuple_size<decltype(ucsBuffer_)>::value> nativeBuffer;
					while(true) {
						const Char* const fromEnd = pptr();

						// conversion
						const auto encodingResult = encoder_->fromUnicode(encodingState_,
							boost::make_iterator_range_n(nativeBuffer.data(), nativeBuffer.size()), toNext,
							boost::make_iterator_range<const Char*>(pbase(), fromEnd), fromNext);
						if(encodingResult == encoding::Encoder::UNMAPPABLE_CHARACTER)
							throw UnmappableCharacterException();
						else if(encodingResult == encoding::Encoder::MALFORMED_INPUT)
							throw text::MalformedInputException<Char>(*fromNext);

						// write into the file
#if BOOST_OS_WINDOWS
						DWORD writtenBytes;
						assert(static_cast<std::size_t>(toNext - nativeBuffer.data()) <= std::numeric_limits<DWORD>::max());
						const DWORD bytes = static_cast<DWORD>(toNext - nativeBuffer.data());
						if(::WriteFile(fileHandle_, nativeBuffer.data(), bytes, &writtenBytes, 0) == 0 || writtenBytes != bytes)
							throw detail::makeFileSystemError("WriteFile() failed.", fileName());
#else // ASCENSION_OS_POSIX
						const std::size_t bytes = toNext - nativeBuffer;
						const ssize_t writtenBytes = ::write(fileDescriptor_, nativeBuffer, bytes);
						if(writtenBytes == -1 || static_cast<std::size_t>(writtenBytes) != bytes)
							throw detail::makeFileSystemError("write(2) failed.", fileName());
#endif

						setp(ucsBuffer_.data() + (fromNext - ucsBuffer_.data()), epptr());
						pbump(static_cast<int>(fromEnd - pbase()));	// TODO: this cast may be danger.
						if(encodingResult == encoding::Encoder::COMPLETED)
							break;
					}
					setp(ucsBuffer_.data(), ucsBuffer_.data() + ucsBuffer_.size());
				}
				return 0;
			}

			/// @see std#basic_streambuf#underflow
			TextFileStreamBuffer::int_type TextFileStreamBuffer::underflow() {
				if(std::begin(inputMapping_.buffer) == nullptr || inputMapping_.current >= std::end(inputMapping_.buffer))
					return traits_type::eof();	// not input mode or reached EOF

				Char* toNext;
				const Byte* fromNext;
				switch(encoder_->toUnicode(encodingState_,
						boost::make_iterator_range_n(ucsBuffer_.data(), ucsBuffer_.size()), toNext,
						boost::make_iterator_range(inputMapping_.current, std::end(inputMapping_.buffer)), fromNext)) {
					case encoding::Encoder::UNMAPPABLE_CHARACTER:
						throw UnmappableCharacterException();
					case encoding::Encoder::MALFORMED_INPUT:
						throw text::MalformedInputException<Byte>(*fromNext);
					default:
						break;
				}

				inputMapping_.current = fromNext;
				setg(ucsBuffer_.data(), ucsBuffer_.data(), toNext);
				return (toNext > ucsBuffer_.data()) ? traits_type::to_int_type(*gptr()) : traits_type::eof();
			}

			/**
			 * Returns @c true if the internal encoder has scanned Unicode byte order mark in incoming byte sequence.
			 * @see encoding#Encoder#isByteOrderMarkEncountered
			 */
			bool TextFileStreamBuffer::unicodeByteOrderMark() const BOOST_NOEXCEPT {
				return encoder_->isByteOrderMarkEncountered(decodingState_);
			}

			namespace detail {
				boost::filesystem::filesystem_error makeGenericFileSystemError(const std::string& what,
						const boost::filesystem::path& path, boost::system::errc::errc_t value) {
					return boost::filesystem::filesystem_error(what, path, boost::system::errc::make_error_code(value));
				}

				boost::filesystem::filesystem_error makeFileSystemError(const std::string& what,
						const boost::filesystem::path& path, const boost::optional<int>& value /* = boost::none */) {
#if ASCENSION_OS_POSIX
					return makeGenericFileSystemError(what, path, boost::get_optional_value_or(value, makePlatformError().code().value()));
#else
					return boost::filesystem::filesystem_error(what, path,
						boost::system::error_code(boost::get_optional_value_or(value, makePlatformError().code().value()), boost::system::system_category()));
#endif
				}
			}
		}
	}
}
