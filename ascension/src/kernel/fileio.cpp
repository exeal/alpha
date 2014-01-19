/**
 * @file fileio.cpp
 * Implements @c ascension#kernel#fileio namespace.
 * @note Currently, this implementation does not support OpenVMS.
 * @author exeal
 * @date 2007 (separated from document.cpp)
 * @date 2007-2014
 */

#include <ascension/config.hpp>	// ASCENSION_NO_STANDARD_ENCODINGS
#include <ascension/kernel/fileio.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/range/algorithm/find_first_of.hpp>
#include <limits>	// std.numeric_limits
#ifdef ASCENSION_OS_POSIX
#	include <cstdio>		// std.tempnam
#	include <fcntl.h>		// fcntl
#	include <unistd.h>		// fcntl
#	include <sys/mman.h>	// mmap, munmap, ...
#endif // !ASCENSION_OS_POSIX


namespace ascension {
	namespace kernel {
		namespace fileio {
			// free function //////////////////////////////////////////////////////////////////////////////////////////

			namespace {
				inline void sanityCheckPathName(const PathStringPiece& s, const std::string& variableName) {
					if(s.cbegin() == nullptr || s.cend() == nullptr)
						throw NullPointerException(variableName);
					if(s.cbegin() > s.cend())
						throw std::invalid_argument(variableName + ".cbegin() > " + variableName + ".cend()");
				}
			}

			namespace {
#ifdef ASCENSION_OS_WINDOWS
				static const std::array<PathCharacter, 2> PATH_SEPARATORS = {0x005cu, 0x002fu};	// \ or /
#else // ASCENSION_OS_POSIX
				static const std::array<PathCharacter, 1> PATH_SEPARATORS = {'/'};
#endif
				static const PathCharacter PREFERRED_PATH_SEPARATOR = PATH_SEPARATORS[0];
				/// Returns @c true if the given character is a path separator.
				inline bool isPathSeparator(PathCharacter c) BOOST_NOEXCEPT {
					return boost::find(PATH_SEPARATORS, c) != boost::end(PATH_SEPARATORS);
				}
				/**
				 * Returns @c true if the specified file or directory exists.
				 * @param name the name of the file
				 * @return @c true if the file exists
				 * @throw IOException any I/O error occurred
				 */
				bool pathExists(const PathStringPiece& name) {
#ifdef ASCENSION_OS_WINDOWS
#ifdef PathFileExists
					return win32::boole(::PathFileExistsW(name.cbegin()));
#else
					if(::GetFileAttributesW(name.cbegin()) != INVALID_FILE_ATTRIBUTES)
						return true;
					const DWORD e = ::GetLastError();
					if(e == ERROR_FILE_NOT_FOUND || e == ERROR_PATH_NOT_FOUND
							|| e == ERROR_INVALID_NAME || e == ERROR_INVALID_PARAMETER || e == ERROR_BAD_NETPATH)
						return false;
#endif // PathFileExists
#else // ASCENSION_OS_POSIX
					struct stat s;
					if(::stat(name, &s) == 0)
						return true;
					else if(errno == ENOENT)
						return false;
#endif
					throw IOException(name);
				}

				/// Finds the base name in the given file path name.
				inline PathStringPiece::const_iterator findFileName(const PathStringPiece& s) {
					return s.cbegin() + std::distance(boost::find_first_of(s | boost::adaptors::reversed, PATH_SEPARATORS), boost::rend(s));
				}

				/**
				 * Returns the last write time of the specified file.
				 * @param fileName The name of the file
				 * @param[out] timeStamp The time
				 * @throw IOException Any I/O error occurred
				 */
				void getFileLastWriteTime(const PathStringPiece& fileName, TextFileDocumentInput::Time& timeStamp) {
#ifdef ASCENSION_OS_WINDOWS
					WIN32_FILE_ATTRIBUTE_DATA attributes;
					if(::GetFileAttributesExW(fileName.cbegin(), GetFileExInfoStandard, &attributes) != 0)
						timeStamp = attributes.ftLastWriteTime;
#else // ASCENSION_OS_POSIX
					struct stat s;
					if(::stat(fileName.cbegin(), &s) == 0)
						timeStamp = s.st_mtime;
#endif
					else
						throw IOException(fileName);
				}

				/**
				 * Returns the size of the specified file.
				 * @param fileName The name of the file
				 * @return The size of the file in bytes or -1 if the file is too large
				 * @throw IOException Any I/O error occurred
				 */
				std::ptrdiff_t getFileSize(const PathStringPiece& fileName) {
#ifdef ASCENSION_OS_WINDOWS
					WIN32_FILE_ATTRIBUTE_DATA attributes;
					if(::GetFileAttributesExW(fileName.cbegin(), GetFileExInfoStandard, &attributes) != 0)
						return (attributes.nFileSizeHigh == 0
							&& attributes.nFileSizeLow <= static_cast<DWORD>(std::numeric_limits<std::ptrdiff_t>::max())) ?
								static_cast<std::ptrdiff_t>(attributes.nFileSizeLow) : -1;
#else // ASCENSION_OS_POSIX
					struct stat s;
					if(::stat(fileName.cbegin(), &s) == 0)
						return s.st_size;
#endif
					else
						throw IOException(fileName);
				}

				/**
				 * Creates a name for a temporary file.
				 * @param seed The string contains a directory path and a prefix string
				 * @return The result string
				 * @throw std#bad_alloc POSIX @c tempnam failed (only when @c ASCENSION_OS_POSIX was defined)
				 * @throw IOException Any I/O error occurred
				 */
				PathString makeTemporaryFileName(const PathStringPiece& seed) {
#ifndef BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS
					PathString s(seed);
#else
					PathString s(seed.cbegin(), seed.cend());
#endif
					const PathString::const_iterator name(s.cbegin() + std::distance(seed.cbegin(), findFileName(seed)));
					if(name != s.cbegin())
						s.resize(distance(s.cbegin(), name) - 1);
#ifdef ASCENSION_OS_WINDOWS
					WCHAR result[MAX_PATH];
					if(::GetTempFileNameW(s.c_str(), s.data() + distance(s.cbegin(), name), 0, result) != 0)
						return result;
#else // ASCENSION_OS_POSIX
					if(char* p = ::tempnam(s.get(), name)) {
						PathString result(p);
						::free(p);
						return result;
					} else if(errno == ENOMEM)
						throw std::bad_alloc();	// tempnam failed
#endif
					throw IOException(PathString());
				}

				/**
				 * Returns @c true if the specified file is special.
				 * @param fileName The file name
				 * @throw IOException Any I/O error occurred
				 */
				bool isSpecialFile(const PathStringPiece& fileName) {
#ifdef ASCENSION_OS_WINDOWS
					HANDLE file = ::CreateFileW(fileName.cbegin(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
					if(file != INVALID_HANDLE_VALUE) {
						const DWORD fileType = ::GetFileType(file);
						::CloseHandle(file);
						switch(fileType) {
						case FILE_TYPE_DISK:
							return false;
						case FILE_TYPE_UNKNOWN:
							if(::GetLastError() != NO_ERROR)
								throw IOException(fileName);
						default:
							return true;
						}
					}
#else // ASCENSION_OS_POSIX
					struct stat s;
					if(::stat(fileName.cbegin(), &s) == 0)
						return !S_ISREG(s.st_mode);
#endif
					else
						throw IOException(fileName);
				}

				/**
				 * Verifies if the newline is allowed in the given character encoding.
				 * @param encoding The character encoding
				 * @param newline The newline to verify
				 * @throw encoding#UnsupportedEncodingException @a encoding is not supported
				 * @throw UnknownValueExcepion @a newline is undefined value
				 * @throw std#invalid_argument @a newline is not allowed or not a literal value
				 */
				void verifyNewline(const std::string& encoding, const text::Newline& newline) {
					if(newline == text::Newline::NEXT_LINE || newline == text::Newline::LINE_SEPARATOR || newline == text::Newline::PARAGRAPH_SEPARATOR) {
						std::unique_ptr<encoding::Encoder> encoder(encoding::Encoder::forName(encoding));
						if(encoder.get() == nullptr)
							throw encoding::UnsupportedEncodingException("the specified encoding is not supported.");
						else if(!encoder->canEncode(newline.asString()[0]))
							throw std::invalid_argument("the specified newline is not allowed in the specified character encoding.");
#if 0
						const MIBenum mib = encoding.properties().mibEnum();
						if(mib != fundamental::UTF_8
								&& mib != fundamental::UTF_16LE && mib != fundamental::UTF_16BE && fundamental::UTF_16
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
								&& mib != standard::UTF_7 && mib != standard::UTF_32 && mib != standard::UTF_32LE && mib != standard::UTF_32BE
#endif // !ASCENSION_NO_STANDARD_ENCODINGS
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
								&& encoding.properties().name() == "UTF-5"
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
							)
							throw std::invalid_argument("the specified newline is not allowed in the specified character encoding.");
#endif
					}
				}
			} // namespace @0

			/**
			 * Makes the given path name real. This method will not fail even if the path name is not exist. Win32
			 * target platform: If @a pathName is a UNC, the case of @a pathName will not be fixed. All slashes will be
			 * replaced by backslashes.
			 * @param pathName The absolute path name
			 * @return The result real path name
			 * @throw NullPointerException @a pathName is @c null
			 * @see comparePathNames
			 */
			PathString fileio::canonicalizePathName(const PathStringPiece& pathName) {
				sanityCheckPathName(pathName, "pathName");

#ifdef ASCENSION_OS_WINDOWS

				if(pathName.length() >= MAX_PATH)	// too long name
#ifndef BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS
					return PathString(pathName);
#else
					return pathName.to_string();
#endif

				// resolve relative path name
				std::array<WCHAR, MAX_PATH> fullName;
				WCHAR* dummy;
				if(::GetFullPathNameW(pathName.cbegin(), fullName.size(), fullName.data(), &dummy) == 0)
					std::wcscpy(fullName.data(), pathName.cbegin());

				// get real component names (from Ftruename implementation in xyzzy)
				PathString result;
				result.reserve(MAX_PATH);
				auto view(boost::make_iterator_range(fullName));
				if(((view[0] >= L'A' && view[0] <= L'Z') || (view[0] >= L'a' && view[0] <= L'z'))
						&& view[1] == L':' && isPathSeparator(view[2])) {	// drive letter
					result.append(fullName.data(), 3);
					result[0] = towupper(fullName[0]);	// unify with uppercase letters...
					view.advance_begin(+3);
				} else if(std::all_of(std::begin(view), std::begin(view) + 1, &isPathSeparator)) {	// UNC?
					view = boost::make_iterator_range(boost::find_first_of(view.advance_begin(+2), PATH_SEPARATORS), std::end(view));
					if(view.empty())	// server name
						return false;
					view = boost::make_iterator_range(boost::find_first_of(view.advance_begin(+1), PATH_SEPARATORS), std::end(view));
					if(view.empty())	// shared name
						return false;
					result.append(std::begin(fullName), std::begin(view.advance_begin(+1)));
				} else	// not absolute name
#ifndef BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS
					return PathString(pathName);
#else
					return pathName.to_string();
#endif

				WIN32_FIND_DATAW wfd;
				while(true) {
					auto next = boost::find_first_of(view, PATH_SEPARATORS);
					if(next != boost::end(view)) {
						const PathCharacter c = *next;
						*next = 0;
						HANDLE h = ::FindFirstFileW(fullName.data(), &wfd);
						if(h != INVALID_HANDLE_VALUE) {
							::FindClose(h);
							result += wfd.cFileName;
						} else
							result.append(std::begin(view), std::end(view));
						*next = c;
						result += PREFERRED_PATH_SEPARATOR;
						view = boost::make_iterator_range(next + 1, std::end(view));
					} else {
						HANDLE h = ::FindFirstFileW(fullName.data(), &wfd);
						if(h != INVALID_HANDLE_VALUE) {
							::FindClose(h);
							result += wfd.cFileName;
						} else
							result.append(std::begin(view), std::end(view));
						break;
					}
				}
				return result;

#else // ASCENSION_OS_POSIX

				PathCharacter resolved[PATH_MAX];
				return (::realpath(pathName, resolved) != nullptr) ? resolved : pathName;

#endif
			}

			/**
			 * Returns @c true if the specified two file path names are equivalent.
			 * @param s1 The first path name
			 * @param s2 The second path name
			 * @return @c true if @a s1 and @a s2 are equivalent
			 * @throw NullPointerException Either file name is @c null
			 * @see canonicalizePathName
			 */
			bool fileio::comparePathNames(const PathStringPiece& s1, const PathStringPiece& s2) {
				sanityCheckPathName(s1, "s1");
				sanityCheckPathName(s2, "s2");

#ifdef ASCENSION_OS_WINDOWS
#ifdef PathMatchSpec
				if(win32::boole(::PathMatchSpecW(s1, s2)))
					return true;
#endif // PathMatchSpec
				// by lexicographical comparison
				const int c1 = static_cast<int>(s1.length()) + 1, c2 = static_cast<int>(s2.length()) + 1;
				const int fc1 = ::LCMapStringW(LOCALE_NEUTRAL, LCMAP_LOWERCASE, s1.cbegin(), c1, nullptr, 0);
				const int fc2 = ::LCMapStringW(LOCALE_NEUTRAL, LCMAP_LOWERCASE, s2.cbegin(), c2, nullptr, 0);
				if(fc1 != 0 && fc2 != 0 && fc1 == fc2) {
					std::unique_ptr<WCHAR[]> fs1(new WCHAR[fc1]), fs2(new WCHAR[fc2]);
					::LCMapStringW(LOCALE_NEUTRAL, LCMAP_LOWERCASE, s1.cbegin(), c1, fs1.get(), fc1);
					::LCMapStringW(LOCALE_NEUTRAL, LCMAP_LOWERCASE, s2.cbegin(), c2, fs2.get(), fc2);
					if(std::wmemcmp(fs1.get(), fs2.get(), fc1) == 0)
						return pathExists(s1);
				}
				// by volume information
				bool eq = false;
				HANDLE f1 = ::CreateFileW(s1.cbegin(), 0,
					FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
				if(f1 != INVALID_HANDLE_VALUE) {
					HANDLE f2 = ::CreateFileW(s2.cbegin(), 0,
						FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
					if(f2 != INVALID_HANDLE_VALUE) {
						BY_HANDLE_FILE_INFORMATION fi1;
						if(win32::boole(::GetFileInformationByHandle(f1, &fi1))) {
							BY_HANDLE_FILE_INFORMATION fi2;
							if(win32::boole(::GetFileInformationByHandle(f2, &fi2)))
								eq = fi1.dwVolumeSerialNumber == fi2.dwVolumeSerialNumber
									&& fi1.nFileIndexHigh == fi2.nFileIndexHigh
									&& fi1.nFileIndexLow == fi2.nFileIndexLow;
						}
						::CloseHandle(f2);
					}
					::CloseHandle(f1);
				}
				return eq;
#else // ASCENSION_OS_POSIX
				// by lexicographical comparison
				if(boost::lexicographical_compare(s1, s2) == 0)
					return true;
				// by volume information
				struct stat st1, st2;
				return ::stat(s1.cbegin(), &st1) == 0 && ::stat(s2.cbegin(), &st2) == 0
					&& st1.st_dev == st2.st_dev && st1.st_ino == st2.st_ino
					&& st1.st_size == st2.st_size && st1.st_mtime == st2.st_mtime;
#endif
			}

			/**
			 * Inserts the contents of the file into the specified position.
			 * @param document The document
			 * @param at The position into which the contents is inserted
			 * @param fileName The file name
			 * @param encoding The character encoding of the input file or auto detection name
			 * @param encodingSubstitutionPolicy The substitution policy used in encoding conversion
			 * @param[out] endOfInsertedString The position of the end of the inserted text. Can be @c null if not
			 *                                 needed
			 * @return A pair consists of the encoding used to convert and the boolean value means if the input
			 *         contained Unicode byte order mark
			 * @throw UnmappableCharacterException
			 * @throw MalformedInputException
			 * @throw ... Any exceptions @c TextFileStreamBuffer#TextFileStreamBuffer and @c kernel#insert throw
			 */
			std::pair<std::string, bool> fileio::insertFileContents(Document& document,
					const Position& at, const PathStringPiece& fileName, const std::string& encoding,
					encoding::Encoder::SubstitutionPolicy encodingSubstitutionPolicy, Position* endOfInsertedString /* = nullptr */) {
				TextFileStreamBuffer sb(fileName, std::ios_base::in, encoding, encodingSubstitutionPolicy, false);
				std::basic_istream<Char> in(&sb);
				in.exceptions(std::ios_base::badbit);
				insert(document, at, in, endOfInsertedString);
				const std::pair<std::string, bool> result(make_pair(sb.encoding(), sb.unicodeByteOrderMark()));
				sb.close();
				return result;
			}

			/**
			 * Writes the specified region of the document into the specified file.
			 * @param document The document
			 * @param region The region to write
			 * @param fileName The file name
			 * @param format The encoding and the newline
			 * @param append Set @c true to append to the file. If this is @c true, this function does not write a
			 *               unicode order mark regardless of the value of @a format.unicodeByteOrderMark. See
			 *               constructor of TextFileStreamBuffer
			 * @throw UnsupportedEncodingException The character encoding specified by @a format.encoding is not
			 *                                     supported
			 * @throw std#invalid_argument @a format.newline is not supported or not allowed in the specified character
			 *                             encoding
			 * @throw ... Any I/O error occurred
			 */
			void fileio::writeRegion(const Document& document, const Region& region,
					const PathStringPiece& fileName, const WritingFormat& format, bool append /* = false */) {
				// verify encoding-specific newline
				verifyNewline(format.encoding, format.newline);

				// check if not special file
				if(isSpecialFile(fileName))
#ifdef ASCENSION_OS_WINDOWS
					throw IOException(fileName, ERROR_BAD_FILE_TYPE);
#else // ASCENSION_OS_POSIX
					throw IOException(fileName, ENXIO);
#endif

				// check if writable
#ifdef ASCENSION_OS_WINDOWS
				const DWORD originalAttributes = ::GetFileAttributesW(fileName.cbegin());
				if(originalAttributes != INVALID_FILE_ATTRIBUTES && win32::boole(originalAttributes & FILE_ATTRIBUTE_READONLY))
					throw IOException(fileName, ERROR_ACCESS_DENIED);
#else // ASCENSION_OS_POSIX
				struct stat originalStat;
				bool gotStat = ::stat(fileName.c_str(), &originalStat) == 0;
#if 1
				if(::euidaccess(fileName.c_str(), 2) < 0)
#else
				if(::access(fileName.c_str(), 2) < 0)
#endif
					throw IOException(fileName, EACCES);	// EROFS is an alternative
#endif

				// open file to write
				TextFileStreamBuffer sb(fileName,
					append ? (std::ios_base::out | std::ios_base::app) : std::ios_base::out,
					format.encoding, format.encodingSubstitutionPolicy, format.unicodeByteOrderMark);
				try {
					std::basic_ostream<Char> out(&sb);
					out.exceptions(std::ios_base::badbit);
					// write into file
					writeDocumentToStream(out, document, region, format.newline);
				} catch(...) {
					sb.closeAndDiscard();
					throw;
				}
				sb.close();
			}


			// exception classes //////////////////////////////////////////////////////////////////////////////////////

			/**
			 * Constructor.
			 * @param fileName
			 */
			IOException::IOException(const PathStringPiece& fileName) :
					std::ios_base::failure(makePlatformError().what(), makePlatformError().code()), fileName_(fileName.cbegin(), fileName.cend()) {
			}

			/**
			 * Constructor.
			 * @param fileName
			 * @param code
			 */
			IOException::IOException(const PathStringPiece& fileName, std::error_code::value_type code) :
					std::ios_base::failure(makePlatformError(code).what(), makePlatformError(code).code()), fileName_(fileName.cbegin(), fileName.cend()) {
			}

			/// Returns the file name.
			const PathString& IOException::fileName() const BOOST_NOEXCEPT {
				return fileName_;
			}

			bool IOException::isFileNotFound(const IOException& e) BOOST_NOEXCEPT {
				const std::error_code::value_type code(e.code().value());
#ifdef ASCENSION_OS_WINDOWS
				return code == ERROR_FILE_NOT_FOUND || code == ERROR_PATH_NOT_FOUND
					|| code == ERROR_BAD_NETPATH /*|| code == ERROR_INVALID_PARAMETER*/ || code == ERROR_INVALID_NAME;

#else // ASCENSION_OS_POSIX
				return code == ENOENT || code == ENOTDIR;
#endif
			}

			bool IOException::isPermissionDenied(const IOException& e) BOOST_NOEXCEPT {
				const std::error_code::value_type code(e.code().value());
#ifdef ASCENSION_OS_WINDOWS
				return code == ERROR_ACCESS_DENIED || code == ERROR_SHARING_VIOLATION;
#else // ASCENSION_OS_POSIX
				return code == EACCES;
#endif
			}

			/// Default constructor.
			UnmappableCharacterException::UnmappableCharacterException() : std::ios_base::failure("encountered an unmappable character in encoding/decoding.") {
			}


			// TextFileStreamBuffer ///////////////////////////////////////////////////////////////////////////////////

			namespace {
				class SystemErrorSaver {
				public:
					SystemErrorSaver() BOOST_NOEXCEPT : code_(makePlatformError().code().value()) {}
#ifdef ASCENSION_OS_WINDOWS
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
			TextFileStreamBuffer::TextFileStreamBuffer(const PathStringPiece& fileName, std::ios_base::openmode mode,
					const std::string& encoding, encoding::Encoder::SubstitutionPolicy encodingSubstitutionPolicy,
					bool writeUnicodeByteOrderMark) : fileName_(fileName.cbegin(), fileName.cend()), mode_(mode) {
				sanityCheckPathName(fileName, "fileName");
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
				} catch(const IOException&) {
				}
			}

			void TextFileStreamBuffer::buildEncoder(const std::string& encoding, bool detectEncoding) {
				assert(encoder_.get() == nullptr);
				encoder_ = encoding::Encoder::forName(encoding);
				if(encoder_.get() != nullptr)
					return;
				else if(detectEncoding) {
					if(const std::shared_ptr<const encoding::EncodingDetector> detector = encoding::EncodingDetector::forName(encoding)) {
						const std::pair<encoding::MIBenum, std::string> detected(detector->detect(
							std::begin(inputMapping_.buffer), std::min(std::end(inputMapping_.buffer), std::begin(inputMapping_.buffer) + 1024 * 10), nullptr));
						if(detected.first != encoding::MIB_OTHER)
							encoder_ = encoding::Encoder::forMIB(detected.first);
						else
							encoder_ = encoding::Encoder::forName(detected.second);
						if(encoder_.get() != nullptr)
							return;	// resolved
					}
				}
				throw encoding::UnsupportedEncodingException(encoding);
			}

			void TextFileStreamBuffer::buildInputMapping() {
				assert(isOpen());
				const std::ptrdiff_t fileSize = getFileSize(fileName().c_str());
#ifdef ASCENSION_OS_WINDOWS
				if(fileSize != 0) {
					fileMapping_ = ::CreateFileMappingW(fileHandle_, nullptr, PAGE_READONLY, 0, 0, nullptr);
					if(fileMapping_ == nullptr)
						throw IOException(fileName());
					inputMapping_.buffer = boost::make_iterator_range<const Byte*>(static_cast<const Byte*>(::MapViewOfFile(fileMapping_, FILE_MAP_READ, 0, 0, 0)), nullptr);
					if(std::begin(inputMapping_.buffer) == nullptr) {
						SystemErrorSaver ses;
						::CloseHandle(fileMapping_);
						throw IOException(fileName(), ses.code());
					}
				} else
					fileMapping_ = nullptr;
#else // ASCENSION_OS_POSIX
				inputMapping_.buffer = boost::make_iterator_range<const Byte*>(static_cast<const Byte*>(::mmap(0, fileSize, PROT_READ, MAP_PRIVATE, fileDescriptor_, 0)), nullptr);
				if(std::begin(inputMapping_.buffer) == MAP_FAILED)
					throw IOException(fileName());
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
					throw IOException(fileName());
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
#ifdef ASCENSION_OS_WINDOWS
						::DeleteFileW(fileName_.c_str());
#else // ASCENSION_OS_POSIX
						::unlink(fileName_.c_str());
#endif
						return self;
					} else
						return nullptr;
				} else if(mode() == (std::ios_base::out | std::ios_base::app)) {
#ifdef ASCENSION_OS_WINDOWS
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
#ifdef ASCENSION_OS_WINDOWS
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
				if(encoder_.get() != nullptr) {
					encoder_->resetEncodingState();
					encoder_->resetDecodingState();
				}
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
#ifdef ASCENSION_OS_WINDOWS
				return fileHandle_ != INVALID_HANDLE_VALUE;
#else // ASCENSION_OS_POSIX
				return fileDescriptor_ != -1;
#endif
			}

			// called by only the constructor
			void TextFileStreamBuffer::openForReading(const std::string& encoding) {
				// open the file
#ifdef ASCENSION_OS_WINDOWS
				fileHandle_ = ::CreateFileW(fileName().c_str(), GENERIC_READ,
					FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
				if(fileHandle_ == INVALID_HANDLE_VALUE)
#else // ASCENSION_OS_POSIX
				if(-1 == (fileDescriptor_ = ::open(fileName().c_str(), O_RDONLY)))
#endif
					throw IOException(fileName());

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
#ifdef ASCENSION_OS_WINDOWS
					fileHandle_ = ::CreateFileW(fileName_.c_str(), GENERIC_READ | GENERIC_WRITE,
						0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
					if(fileHandle_ == INVALID_HANDLE_VALUE) {
						const DWORD e = ::GetLastError();
						if(e == ERROR_FILE_NOT_FOUND)
							mode_ &= ~std::ios_base::app;
						else
							throw IOException(fileName(), e);
					} else {
						originalFileEnd_.QuadPart = 0;
						if(!win32::boole(::SetFilePointerEx(fileHandle_, originalFileEnd_, &originalFileEnd_, FILE_END)))
							throw IOException(fileName());
						writeUnicodeByteOrderMark = false;
					}
#else // ASCENSION_OS_POSIX
					fileDescriptor_ = ::open(fileName_.c_str(), O_WRONLY | O_APPEND);
					if(fileDescriptor_ != -1) {
						originalFileEnd_ = ::lseek(fileDescriptor_, 0, SEEK_CUR);
						if(originalFileEnd_ == static_cast<off_t>(-1))
							throw IOException(fileName());
					} else {
						if(errno == ENOENT)
							mode_ &= ~ios_base::app;
						else
							throw IOException(fileName());
					}
#endif
				}
				if((mode_ & ~std::ios_base::trunc) == std::ios_base::out) {
#ifdef ASCENSION_OS_WINDOWS
					fileHandle_ = ::CreateFileW(fileName().c_str(), GENERIC_WRITE, 0, nullptr,
						CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
					if(fileHandle_ == INVALID_HANDLE_VALUE)
#else // ASCENSION_OS_POSIX
					fileDescriptor_ = ::open(fileName().c_str(), O_WRONLY | O_CREAT);
					if(fileDescriptor_ == -1)
#endif
						throw IOException(fileName());
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
					encoder_->setFlags(encoder_->flags() | encoding::Encoder::UNICODE_BYTE_ORDER_MARK);
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
					encoder_->setFlags(encoder_->flags() | encoding::Encoder::BEGINNING_OF_BUFFER | encoding::Encoder::END_OF_BUFFER);
					while(true) {
						const Char* const fromEnd = pptr();

						// conversion
						const encoding::Encoder::Result encodingResult = encoder_->fromUnicode(
							nativeBuffer.data(), nativeBuffer.data() + nativeBuffer.size(), toNext, pbase(), fromEnd, fromNext);
						if(encodingResult == encoding::Encoder::UNMAPPABLE_CHARACTER)
							throw UnmappableCharacterException();
						else if(encodingResult == encoding::Encoder::MALFORMED_INPUT)
							throw text::MalformedInputException<Char>(*fromNext);

						// write into the file
#ifdef ASCENSION_OS_WINDOWS
						DWORD writtenBytes;
						assert(static_cast<std::size_t>(toNext - nativeBuffer.data()) <= std::numeric_limits<DWORD>::max());
						const DWORD bytes = static_cast<DWORD>(toNext - nativeBuffer.data());
						if(::WriteFile(fileHandle_, nativeBuffer.data(), bytes, &writtenBytes, 0) == 0 || writtenBytes != bytes)
#else // ASCENSION_OS_POSIX
						const std::size_t bytes = toNext - nativeBuffer;
						const ssize_t writtenBytes = ::write(fileDescriptor_, nativeBuffer, bytes);
						if(writtenBytes == -1 || static_cast<std::size_t>(writtenBytes) != bytes)
#endif
							throw IOException(fileName());

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
				encoder_->setFlags(encoder_->flags() | encoding::Encoder::BEGINNING_OF_BUFFER | encoding::Encoder::END_OF_BUFFER);
				switch(encoder_->toUnicode(ucsBuffer_.data(), ucsBuffer_.data() + ucsBuffer_.size(), toNext, inputMapping_.current, std::end(inputMapping_.buffer), fromNext)) {
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

			/// Returns @c true if the internal encoder has @c Encoder#UNICODE_BYTE_ORDER_MARK flag.
			bool TextFileStreamBuffer::unicodeByteOrderMark() const BOOST_NOEXCEPT {
				return (encoder_->flags() & encoding::Encoder::UNICODE_BYTE_ORDER_MARK) != 0;
			}


			// TextFileDocumentInput.FileLocker ///////////////////////////////////////////////////////////////////////

			class TextFileDocumentInput::FileLocker {
				ASCENSION_NONCOPYABLE_TAG(FileLocker);
			public:
				FileLocker() BOOST_NOEXCEPT;
				~FileLocker() BOOST_NOEXCEPT;
				bool hasLock() const BOOST_NOEXCEPT;
				bool lock(const PathString& fileName, bool share);
				LockType type() const BOOST_NOEXCEPT;
				bool unlock() BOOST_NOEXCEPT;
private:
				LockType type_;
#ifdef ASCENSION_OS_WINDOWS
				HANDLE file_;
#else // ASCENSION_OS_POSIX
				int file_;
				bool deleteFileOnClose_;
#endif
				PathString fileName_;
			};

			/// Default constructor.
			TextFileDocumentInput::FileLocker::FileLocker() BOOST_NOEXCEPT : type_(NO_LOCK), file_(
#ifdef ASCENSION_OS_WINDOWS
					INVALID_HANDLE_VALUE
#else // ASCENSION_OS_POSIX
					-1), deleteFileOnClose_(false
#endif
					) {
			}

			/// Destructor.
			TextFileDocumentInput::FileLocker::~FileLocker() BOOST_NOEXCEPT {
				unlock();
			}

			inline bool TextFileDocumentInput::FileLocker::hasLock() const BOOST_NOEXCEPT {
#ifdef ASCENSION_OS_WINDOWS
				return file_ != INVALID_HANDLE_VALUE;
#else // ASCENSION_OS_POSIX
				return file_ != -1;
#endif
			}

			/**
			 * Locks the file.
			 * @param fileName
			 * @param share Set @c true if shared-lock
			 * @retval true If locked successfully or the lock mode is @c DONT_LOCK
			 * @retval false The current lock mode was @c SHARED_LOCK and an other existing process had already locked
			 *               the file with same lock mode
			 * @throw IOException
			 */
			bool TextFileDocumentInput::FileLocker::lock(const PathString& fileName, bool share) {
				sanityCheckPathName(fileName, "fileName");
				if(fileName.empty())
#ifdef ASCENSION_OS_WINDOWS
					throw IOException(fileName, ERROR_FILE_NOT_FOUND);
#else // ASCENSION_OS_POSIX
					throw IOException(fileName, ENOENT);
#endif
				bool alreadyShared = false;
#ifdef ASCENSION_OS_POSIX
				flock fl;
				fl.l_whence = SEEK_SET;
				fl.l_start = 0;
				fl.l_len = 0;
#endif // ASCENSION_OS_POSIX

				if(share) {
					// check the file had already been shared-locked
#ifdef ASCENSION_OS_WINDOWS
					HANDLE temp = ::CreateFileW(fileName.c_str(),
						GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
					if(temp == INVALID_HANDLE_VALUE) {
						if(::GetLastError() == ERROR_SHARING_VIOLATION)
							alreadyShared = true;
					} else
						::CloseHandle(temp);
#else // ASCENSION_OS_POSIX
					int temp = ::open(fileName.c_str(), O_RDONLY);
					if(temp != -1) {
						fl.l_type = F_WRLCK;
						if(::fcntl(temp, F_SETLK, &fl) == -1) {
							if(errno == EACCES || errno == EAGAIN)
								alreadyShared = true;
							fl.l_type = F_UNLCK;
							::fcntl(temp, F_SETLK, &fl);
						}
						::close(temp);
					}
#endif
				}

#ifdef ASCENSION_OS_WINDOWS
				HANDLE f = ::CreateFileW(fileName.c_str(), GENERIC_READ,
					share ? FILE_SHARE_READ : 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
				if(f == INVALID_HANDLE_VALUE) {
					DWORD e = ::GetLastError();
					if(e != ERROR_FILE_NOT_FOUND)
						throw IOException(fileName, e);
					f = ::CreateFileW(fileName.c_str(), GENERIC_READ,
						share ? FILE_SHARE_READ : 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
					if(f == INVALID_HANDLE_VALUE)
						throw IOException(fileName);
				}
#else // ASCENSION_OS_POSIX
				int f = ::open(fileName.c_str(), O_RDONLY);
				if(f == -1) {
					if(errno != ENOENT)
						throw IOException(fileName);
					f = ::open(fileName.c_str(), O_RDONLY | O_CREAT);
					if(f == -1)
						throw IOException(fileName);
					fl.l_type = share ? F_RDLCK : F_WRLCK;
					if(::fcntl(f, F_SETLK, &fl) == 0) {
						::close(f);
						throw IOException(fileName);
					}
					deleteFileOnClose_ = true;
					fileName_ = fileName;
				}
#endif

				unlock();
				file_ = f;
				type_ = share ? SHARED_LOCK : EXCLUSIVE_LOCK;
				return !alreadyShared;
			}

			inline TextFileDocumentInput::LockType TextFileDocumentInput::FileLocker::type() const BOOST_NOEXCEPT {
				return hasLock() ? type_ : NO_LOCK;
			}

			/**
			 * Unlocks the file.
			 * @return succeeded or not
			 */
			bool TextFileDocumentInput::FileLocker::unlock() BOOST_NOEXCEPT {
				bool succeeded = true;
				if(hasLock()) {
#ifdef ASCENSION_OS_WINDOWS
					succeeded = win32::boole(::CloseHandle(file_));
					file_ = INVALID_HANDLE_VALUE;
#else // ASCENSION_OS_POSIX
					succeeded = ::close(file_) == 0;
					if(deleteFileOnClose_) {
						::remove(fileName_.c_str());
						deleteFileOnClose_ = false;
					}
					file_ = -1;
#endif
				}
				return succeeded;
			}


			// TextFileDocumentInput //////////////////////////////////////////////////////////////////////////////////

			/**
			 * @class ascension::kernel::fileio::TextFileDocumentInput
			 * @c DocumentInput implementation which initializes a document with the content of the text file.
			 *
			 * @note This class is not intended to be subclassed.
			 *
			 * @c TextFileDocumentInput uses @c TextFileStreamBuffer class to read/write the text file. @c #open opens
			 * a text file and binds the document to the file. @c #write writes the content of the document into the
			 * specified file.
			 *
			 * @code
			 * extern Document d;
			 * TextFileDocumentInput in(d);
			 * in.bind(...);  // bind the document to a file
			 * in.revert(...);
			 *
			 * // .. edit the document ..
			 *
			 * in.save(...);  // write the file
			 * in.unbind();
			 * @endcode
			 *
			 * <h3>Encoding and newline of the file</h3>
			 *
			 * The encoding and newline of the opened file can be obtained by @c #encoding and @c #newline methods.
			 *
			 * <h3>Locking the opened file</h3>
			 *
			 * You can lock the opened file to guard from other processes' changing. @c LockMode parameter of @c #open
			 * method specifies the mode of locking.
			 *
			 * <h3>When the other process modified the opened file</h3>
			 *
			 * You can detect any modification by other process using @c IUnexpectedFileTimeStampDirector.
			 */

			/**
			 * Constructor.
			 * @param document The document
			 */
			TextFileDocumentInput::TextFileDocumentInput(Document& document) :
					fileLocker_(new FileLocker), document_(document), encoding_(encoding::Encoder::defaultInstance().properties().name()),
					unicodeByteOrderMark_(false), newline_(ASCENSION_DEFAULT_NEWLINE), savedDocumentRevision_(0), timeStampDirector_(nullptr) {
				std::memset(&userLastWriteTime_, 0, sizeof(Time));
				std::memset(&internalLastWriteTime_, 0, sizeof(Time));
				desiredLockMode_.type = NO_LOCK;
				desiredLockMode_.onlyAsEditing = false;
				documentModificationSignChangedConnection_ =
					document_.modificationSignChangedSignal().connect(
						std::bind(&TextFileDocumentInput::documentModificationSignChanged, this, std::placeholders::_1));
				document.setProperty(Document::TITLE_PROPERTY, String());
			}

			/// Destructor.
			TextFileDocumentInput::~TextFileDocumentInput() BOOST_NOEXCEPT {
				unbind();
			}

			/**
			 * Registers the file property listener.
			 * @param listener The listener to be registered
			 * @throw std#invalid_argument @a listener is already registered
			 */
			void TextFileDocumentInput::addListener(FilePropertyListener& listener) {
				listeners_.add(listener);
			}

			/**
			 *
			 * @param fileName
			 */
			void TextFileDocumentInput::bind(const PathStringPiece& fileName) {
				sanityCheckPathName(fileName, "fileName");
				if(fileName.empty())
					return unbind();

				const PathString realName(canonicalizePathName(fileName));
				if(!pathExists(realName))
#ifdef ASCENSION_OS_WINDOWS
					throw IOException(fileName, ERROR_FILE_NOT_FOUND);
#else // ASCENSION_OS_POSIX
					throw IOException(fileName, ENOENT);
#endif
				if(fileLocker_->hasLock()) {
					assert(fileLocker_->type() == desiredLockMode_.type);
					if(desiredLockMode_.onlyAsEditing)
						assert(!document().isModified());
//						assert(savedDocumentRevision_ != document().revisionNumber());
					fileLocker_->lock(realName, fileLocker_->type() == SHARED_LOCK);
				}

				if(weakSelf_.get() == nullptr)
					weakSelf_.reset(this, detail::NullDeleter());
				document_.setInput(std::weak_ptr<DocumentInput>(weakSelf_));
				fileName_ = realName;
				listeners_.notify<const TextFileDocumentInput&>(&FilePropertyListener::fileNameChanged, *this);
				document_.setModified();
			}

			/**
			 * Checks the last modified date/time of the bound file and verifies if the other modified the
			 * file. If the file is modified, the listener's
			 * @c IUnexpectedFileTimeStampDerector#queryAboutUnexpectedDocumentFileTimeStamp will be called.
			 * @return The value which the listener returned or @c true if the listener is not set
			 */
			bool TextFileDocumentInput::checkTimeStamp() {
				Time newTimeStamp;
				if(!verifyTimeStamp(false, newTimeStamp)) {
					Time original = userLastWriteTime_;
					std::memset(&userLastWriteTime_, 0, sizeof(Time));
					if(timeStampDirector_ == nullptr
							|| timeStampDirector_->queryAboutUnexpectedDocumentFileTimeStamp(
								document_, UnexpectedFileTimeStampDirector::CLIENT_INVOCATION)) {
						userLastWriteTime_ = newTimeStamp;
						return true;
					}
					userLastWriteTime_ = original;
					return false;
				}
				return true;
			}

			/// @see Document#ModificationSignChangedSignal
			void TextFileDocumentInput::documentModificationSignChanged(const Document&) {
				if(isBoundToFile() && desiredLockMode_.onlyAsEditing) {
					if(!document().isModified())
						fileLocker_->unlock();
					else if(desiredLockMode_.type != NO_LOCK)
						fileLocker_->lock(fileName(), desiredLockMode_.type == SHARED_LOCK);
				}
			}

			/// @see DocumentInput#isChangeable
			bool TextFileDocumentInput::isChangeable(const Document&) const {
				if(isBoundToFile()) {
					// check the time stamp if this is the first modification
					if(timeStampDirector_ != nullptr && !document().isModified()) {
						Time realTimeStamp;
						TextFileDocumentInput& self = const_cast<TextFileDocumentInput&>(*this);
						if(!self.verifyTimeStamp(true, realTimeStamp)) {	// the other overwrote the file
							if(!timeStampDirector_->queryAboutUnexpectedDocumentFileTimeStamp(
									document_, UnexpectedFileTimeStampDirector::FIRST_MODIFICATION))
								return false;
							self.internalLastWriteTime_ = self.userLastWriteTime_ = realTimeStamp;
						}
					}

					// lock the bound file
					if(desiredLockMode_.onlyAsEditing)
						fileLocker_->lock(fileName(), desiredLockMode_.type == SHARED_LOCK);
				}

				return true;
			}

			/// @see DocumentInput#location
			String TextFileDocumentInput::location() const BOOST_NOEXCEPT {
#ifdef ASCENSION_OS_WINDOWS
				return fileName();
#else // ASCENSION_OS_POSIX
				const std::codecvt<Char, PathCharacter, std::mbstate_t>& converter =
					std::use_facet<std::codecvt<Char, PathCharacter, std::mbstate_t>>(std::locale());
				Char result[PATH_MAX * 2];
				std::mbstate_t dummy;
				const PathCharacter* fromNext;
				Char* toNext;
				return (converter.in(dummy, fileName().c_str(),
					fileName().c_str() + fileName().length() + 1, fromNext,
					result, ASCENSION_ENDOF(result), toNext) == std::codecvt_base::ok) ? result : String();
#endif
			}

			/**
			 * Locks the bound file.
			 * @param mode The lock mode.
			 */
			void TextFileDocumentInput::lockFile(const LockMode& mode) {
				if(!isBoundToFile())
					throw IllegalStateException("the input is not bound to a file.");
				if(mode.type == NO_LOCK)
					fileLocker_->unlock();
				else if(!mode.onlyAsEditing || !document().isModified())
					fileLocker_->lock(fileName(), mode.type == SHARED_LOCK);
				desiredLockMode_ = mode;
			}

			/**
			 * Returns the file lock type, or @c NO_LOCK when @c onlyAsEditing value of the desired lock mode
			 * and the file was not locked actually.
			 */
			TextFileDocumentInput::LockType TextFileDocumentInput::lockType() const BOOST_NOEXCEPT {
				return fileLocker_->type();
			}

			/// @see DocumentInput#postFirstDocumentChange
			void TextFileDocumentInput::postFirstDocumentChange(const Document&) BOOST_NOEXCEPT {
				if(!document().isModified() && desiredLockMode_.onlyAsEditing)
					fileLocker_->unlock();
			}

			/**
			 * Removes the file property listener.
			 * @param listener The listener to be removed
			 * @throw std#invalid_argument @a listener is not registered
			 */
			void TextFileDocumentInput::removeListener(FilePropertyListener& listener) {
				listeners_.remove(listener);
			}

			/**
			 * Replaces the document's content with the text of the bound file on disk.
			 * @param encoding The file encoding or auto detection name
			 * @param encodingSubstitutionPolicy The substitution policy used in encoding conversion
			 * @param unexpectedTimeStampDirector
			 * @throw IllegalStateException The object was not bound to a file
			 * @throw IOException Any I/O error occurred. in this case, the document's content will be lost
			 * @throw ... Any exceptions @c insertFileContents throws
			 */
			void TextFileDocumentInput::revert(
					const std::string& encoding, encoding::Encoder::SubstitutionPolicy encodingSubstitutionPolicy,
					UnexpectedFileTimeStampDirector* unexpectedTimeStampDirector /* = nullptr */) {
				if(!isBoundToFile())
					throw IllegalStateException("the object is not bound to a file.");
				document_.resetContent();
				timeStampDirector_ = nullptr;

				// read from the file
				std::pair<std::string, bool> resultEncoding;
				const bool recorded = document().isRecordingChanges();
				document_.recordChanges(false);
				try {
					resultEncoding = insertFileContents(document_, document().region().beginning(), fileName(), encoding, encodingSubstitutionPolicy);
				} catch(...) {
					document_.resetContent();
					document_.recordChanges(recorded);
					throw;
				}
				document_.recordChanges(recorded);
				unicodeByteOrderMark_ = resultEncoding.second;

				// set the new properties of the document
				savedDocumentRevision_ = document().revisionNumber();
				timeStampDirector_ = unexpectedTimeStampDirector;
#ifdef ASCENSION_OS_WINDOWS
				document_.setProperty(Document::TITLE_PROPERTY, fileName());
#else // ASCENSION_OS_POSIX
				const PathString title(fileName());
				const std::locale lc("");
				const std::codecvt<Char, PathCharacter, std::mbstate_t>& conv = std::use_facet<std::codecvt<Char, PathCharacter, std::mbstate_t>>(lc);
				std::mbstate_t state;
				const PathCharacter* fromNext;
				Char* ucsNext;
				std::unique_ptr<Char[]> ucs(new Char[title.length() * 2]);
				if(std::codecvt_base::ok == conv.in(state,
						title.data(), title.data() + title.length(), fromNext, ucs.get(), ucs.get() + title.length() * 2, ucsNext)) {
					*ucsNext = L'0';
					document_.setProperty(Document::TITLE_PROPERTY, ucs.get());
				}
#endif
				encoding_ = resultEncoding.first;
				newline_ = document().getLineInformation(0).newline();	// use the newline of the first line
				listeners_.notify<const TextFileDocumentInput&>(&FilePropertyListener::fileEncodingChanged, *this);
				listeners_.notify<const TextFileDocumentInput&>(&FilePropertyListener::fileNameChanged, *this);

				document_.clearUndoBuffer();
				document_.markUnmodified();

				// update the internal time stamp
				try {
					getFileLastWriteTime(fileName(), internalLastWriteTime_);
					userLastWriteTime_ = internalLastWriteTime_;
				} catch(std::ios_base::failure&) {
					// ignore...
				}
			}

			/**
			 * Sets the character encoding.
			 * @param encoding The encoding
			 * @return This object
			 * @throw encoding#UnsupportedEncodingException @a encoding is not supported
			 * @see #encoding
			 */
			TextFileDocumentInput& TextFileDocumentInput::setEncoding(const std::string& encoding) {
				if(!encoding.empty() && !encoding::Encoder::supports(encoding))
					throw encoding::UnsupportedEncodingException("encoding");
				encoding_ = encoding;
				listeners_.notify<const TextFileDocumentInput&>(&FilePropertyListener::fileEncodingChanged, *this);
				return *this;
			}

			/**
			 * Sets the newline.
			 * @param newline The newline
			 * @return This object
			 * @throw UnknownValueException @a newline is not literal
			 */
			TextFileDocumentInput& TextFileDocumentInput::setNewline(const text::Newline& newline) {
				if(!newline.isLiteral())
					throw UnknownValueException("newline");
				else if(newline != newline_) {
					newline_ = newline;
					listeners_.notify<const TextFileDocumentInput&>(&FilePropertyListener::fileEncodingChanged, *this);
				}
				return *this;
			}

			/**
			 * Unbinds the file and the document.
			 * @note This method does NOT reset the content of the document.
			 */
			void TextFileDocumentInput::unbind() BOOST_NOEXCEPT {
				if(isBoundToFile()) {
					fileLocker_->unlock();	// this may return false
					if(const std::shared_ptr<DocumentInput> input = document().input().lock()) {
						if(input.get() == static_cast<const DocumentInput*>(this))
							document_.setInput(std::weak_ptr<DocumentInput>());
					}
					fileName_.erase();
					listeners_.notify<const TextFileDocumentInput&>(&FilePropertyListener::fileNameChanged, *this);
					setEncoding(encoding::Encoder::defaultInstance().properties().name());
					std::memset(&userLastWriteTime_, 0, sizeof(Time));
					std::memset(&internalLastWriteTime_, 0, sizeof(Time));
				}
			}

			void TextFileDocumentInput::unlockFile() {
				fileLocker_->unlock();
			}

			/**
			 * Returns last modified time.
			 * @param internal Set @c true for @c internalLastWriteTime_, @c false for @c userLastWriteTime_
			 * @param[out] newTimeStamp The actual time stamp
			 * @return @c false if not match
			 */
			bool TextFileDocumentInput::verifyTimeStamp(bool internal, Time& newTimeStamp) BOOST_NOEXCEPT {
				static Time uninitialized;
				static bool initializedUninitialized = false;
				if(!initializedUninitialized)
					std::memset(&uninitialized, 0, sizeof(Time));

				const Time& about = internal ? internalLastWriteTime_ : userLastWriteTime_;
				if(!isBoundToFile()
						|| std::memcmp(&about, &uninitialized, sizeof(Time)) == 0
						|| fileLocker_->hasLock())
					return true;	// not managed

				try {
					getFileLastWriteTime(fileName().c_str(), newTimeStamp);
				} catch(IOException&) {
					return true;
				}
#ifdef ASCENSION_OS_WINDOWS
				return ::CompareFileTime(&about, &newTimeStamp) != -1;
#else // ASCENSION_OS_POSIX
				return about >= newTimeStamp;
#endif
			}

			/*
			void backupAtRecycleBin(const PathString& fileName) {
				sanityCheckPathName(fileName, "fileName");
				if(pathExists(fileName.c_str())) {
					WCHAR backupPath[MAX_PATH + 1];
					SHFILEOPSTRUCTW	shfos = {
						0, FO_DELETE, backupPath, 0, FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT, false, 0
					};
					std::wcscpy(backupPath, fileName.c_str());
					std::wcscat(backupPath, L".bak");
					backupPath[wcslen(backupPath) + 1] = 0;
					::CopyFileW(filePath.c_str(), backupPath, false);
					::SHFileOperationW(&shfos);
				}
			}
			*/

			/**
			 * Writes the content of the document into the bound file.
			 * @param format The character encoding and the newlines
			 * @param options The other options
			 * @throw
			 */
			void TextFileDocumentInput::write(const WritingFormat& format, const WritingOption* options /* = nullptr */) {
				if(!document().isModified())
					return;
				if(!isBoundToFile())
					throw IllegalStateException("no file name.");
				verifyNewline(format.encoding, format.newline);

				// TODO: check if the input had been truncated.

				// check if the disk file had changed
				if(timeStampDirector_ != nullptr) {
					Time realTimeStamp;
					if(!verifyTimeStamp(true, realTimeStamp)) {
						if(!timeStampDirector_->queryAboutUnexpectedDocumentFileTimeStamp(
								document_, UnexpectedFileTimeStampDirector::OVERWRITE_FILE))
							return;
					}
				}

				// TODO: backup the file.
				const bool makeBackup = false;

				// create a temporary file and write into
				const PathString tempFileName(makeTemporaryFileName(fileName()));
				writeRegion(document(), document().region(), tempFileName, format, false);

				// copy file attributes (file mode) and delete the old file
				try {
					if(fileLocker_->type() != NO_LOCK)
						unlockFile();
#ifdef ASCENSION_OS_WINDOWS
					const DWORD attributes = ::GetFileAttributesW(fileName().c_str());
					if(attributes != INVALID_FILE_ATTRIBUTES) {
						::SetFileAttributesW(tempFileName.c_str(), attributes);
						if(makeBackup) {
						} else if(!win32::boole(::DeleteFileW(fileName().c_str()))) {
							SystemErrorSaver ses;
							if(ses.code() != ERROR_FILE_NOT_FOUND) {
								::DeleteFileW(tempFileName.c_str());
								throw IOException(tempFileName, ses.code());
							}
						}
					}
					if(!win32::boole(::MoveFileW(tempFileName.c_str(), fileName().c_str()))) {
						if(attributes != INVALID_FILE_ATTRIBUTES)
							throw std::ios_base::failure("lost the disk file.");
						SystemErrorSaver ses;
						::DeleteFileW(tempFileName.c_str());
						throw IOException(fileName(), ses.code());
					}
#else // ASCENSION_OS_POSIX
					struct stat s;
					bool fileLost = false;
					if(::stat(fileName().c_str(), &s) != -1) {
						::chmod(tempFileName.c_str(), s.st_mode);
						if(makeBackup) {
						} else if(::remove(fileName().c_str()) != 0) {
							SystemErrorSaver ses;
							if(ses.code() != ENOENT) {
								::remove(tempFileName.c_str());
								throw IOException(fileName(), ses.code());
							}
						}
						fileLost = true;
					}
					if(::rename(tempFileName.c_str(), fileName().c_str()) != 0) {
						if(fileLost)
							throw std::ios_base::failure("lost the disk file.");
						SystemErrorSaver ses;
						::remove(tempFileName.c_str());
						throw IOException(fileName(), ses.code());
					}
#endif
				} catch(...) {
					try {
						lockFile(desiredLockMode_);
					} catch(...) {
					}
				}

				// relock the file
				lockFile(desiredLockMode_);

				// update internal status
				savedDocumentRevision_ = document().revisionNumber();
				document_.markUnmodified();
				document_.setReadOnly(false);

				// update the internal time stamp
				try {
					getFileLastWriteTime(fileName_, internalLastWriteTime_);
				} catch(IOException&) {
					std::memset(&internalLastWriteTime_, 0, sizeof(Time));
				}
				userLastWriteTime_ = internalLastWriteTime_;
			}


#ifndef ASCENSION_NO_GREP

			// DirectoryIteratorBase ////////////////////////////////////////////////////

			/// Protected default constructor.
			DirectoryIteratorBase::DirectoryIteratorBase() BOOST_NOEXCEPT {
			}

			/// Destructor.
			DirectoryIteratorBase::~DirectoryIteratorBase() BOOST_NOEXCEPT {
			}


			// DirectoryIterator ////////////////////////////////////////////////////////

			namespace {
				inline bool isDotOrDotDot(const PathStringPiece& s) {
					return !s.empty() && s[0] == '.' && (s.length() == 1 || (s.length() == 2 && s[1] == '.'));
				}
			} // namespace @0

			/**
			 * Constructor.
			 * @param directoryName The directory to traverse
			 * @throw NullPointerException @a directoryName is @c null
			 * @throw IOException Any I/O error occurred
			 */
			DirectoryIterator::DirectoryIterator(const PathStringPiece& directoryName) :
#ifdef ASCENSION_OS_WINDOWS
					handle_(INVALID_HANDLE_VALUE),
#else // ASCENSION_OS_POSIX
					handle_(0),
#endif
					done_(false) {
				sanityCheckPathName(directoryName, "directoryName");
				if(directoryName[0] == 0)
#ifdef ASCENSION_OS_WINDOWS
					throw IOException(directoryName, ERROR_PATH_NOT_FOUND);
#else // ASCENSION_OS_POSIX
					throw IOException(directoryName, ENOENT);
#endif

#ifdef ASCENSION_OS_WINDOWS
				if(!pathExists(directoryName))
					throw IOException(directoryName, ERROR_PATH_NOT_FOUND);
				PathString pattern;
				pattern.reserve(directoryName.length() + 2);
				pattern.assign(directoryName.cbegin(), directoryName.cend());
				pattern.append(isPathSeparator(pattern.back()) ? L"*" : L"\\*");
				WIN32_FIND_DATAW data;
				handle_ = ::FindFirstFileW(pattern.c_str(), &data);
				if(handle_ == INVALID_HANDLE_VALUE)
					throw IOException(directoryName);
				update(&data);
				directory_.assign(pattern.c_str(), isPathSeparator(pattern[directoryName.length() - 1]) ? directoryName.length() - 1 : directoryName.length());
#else // ASCENSION_OS_POSIX
				handle_ = ::opendir(directoryName.cbegin());
				if(handle_ == 0)
					throw IOException(directoryName);
				update(0);
				directory_.assign(directoryName);
				if(isPathSeparator(directory_[directory_.length() - 1]))
					directory_.resize(directory_.length() - 1);
#endif

				if(!done_ && currentIsDirectory_ && isDotOrDotDot(current_))
					next();
			}

			/// Destructor.
			DirectoryIterator::~DirectoryIterator() BOOST_NOEXCEPT {
#ifdef ASCENSION_OS_WINDOWS
				if(handle_ != INVALID_HANDLE_VALUE)
					::FindClose(handle_);
#else // ASCENSION_OS_POSIX
				if(handle_ != 0)
					::closedir(handle_);
#endif
			}

			/// @see DirectoryIteratorBase#current
			const PathString& DirectoryIterator::current() const {
				if(done_)
					throw NoSuchElementException();
				return current_;
			}

			/// @see DirectoryIteratorBase#directory
			const PathString& DirectoryIterator::directory() const {
				return directory_;
			}

			/// @see DirectoryIteratorBase#hasNext
			bool DirectoryIterator::hasNext() const BOOST_NOEXCEPT {
				return !done_;
			}

			/// @see DirectoryIteratorBase#isDirectory
			bool DirectoryIterator::isDirectory() const {
				if(done_)
					throw NoSuchElementException();
				return currentIsDirectory_;
			}

			/// @see DirectoryIteratorBase#next
			void DirectoryIterator::next() {
				if(!done_) {
#ifdef ASCENSION_OS_WINDOWS
					WIN32_FIND_DATAW data;
					if(::FindNextFileW(handle_, &data) == 0) {
						if(::GetLastError() == ERROR_NO_MORE_FILES)
							done_ = true;
						else
							throw IOException(directory_);
					} else
						update(&data);
#else // ASCENSION_OS_POSIX
					update(0);
#endif
				}
				if(!done_ && currentIsDirectory_ && isDotOrDotDot(current_))
					next();
			}

			void DirectoryIterator::update(const void* info) {
#ifdef ASCENSION_OS_WINDOWS
				const WIN32_FIND_DATAW& data = *static_cast<const WIN32_FIND_DATAW*>(info);
				current_ = data.cFileName;
				currentIsDirectory_ = win32::boole(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
#else // ASCENSION_OS_POSIX
				if(dirent* entry = ::readdir(handle_)) {
					current_ = entry->d_name;
					currentIsDirectory_ = entry->d_type == DT_DIR;
				} else
					done_ = true;
#endif
			}


			// RecursiveDirectoryIterator /////////////////////////////////////////////////////////////////////////////

			/**
			 * Constructor.
			 * @param directoryName The directory to traverse
			 * @throw NullPointerException @a directoryName is @c null
			 * @throw IOException Can be @c IOException#FILE_NOT_FOUND or @c IOException#PLATFORM_DEPENDENT_ERROR
			 */
			RecursiveDirectoryIterator::RecursiveDirectoryIterator(const PathStringPiece& directoryName) : doesntPushNext_(false) {
				stack_.push(new DirectoryIterator(directoryName));
			}

			/// Destructor.
			RecursiveDirectoryIterator::~RecursiveDirectoryIterator() BOOST_NOEXCEPT {
				while(!stack_.empty()) {
					delete stack_.top();
					stack_.pop();
				}
			}

			/// @see DirectoryIteratorBase#current
			const PathString& RecursiveDirectoryIterator::current() const {
				if(!hasNext())
					throw NoSuchElementException();
				return stack_.top()->current();
			}

			/// @see DirectoryIteratorBase#directory
			const PathString& RecursiveDirectoryIterator::directory() const BOOST_NOEXCEPT {
				return stack_.top()->directory();
			}

			/**
			 * @throw NoSuchElementException
			 */
			void RecursiveDirectoryIterator::dontPush() {
				if(!hasNext())
					throw NoSuchElementException();
				doesntPushNext_ = true;
			}

			/// @see DirectoryIteratorBase#hasNext
			bool RecursiveDirectoryIterator::hasNext() const BOOST_NOEXCEPT {
				return stack_.size() != 1 || stack_.top()->hasNext();
			}

			/// @see DirectoryIteratorBase#isDirectory
			bool RecursiveDirectoryIterator::isDirectory() const {
				if(!hasNext())
					throw NoSuchElementException();
				return stack_.top()->isDirectory();
			}

			/// Returns the depth of the recursion.
			std::size_t RecursiveDirectoryIterator::level() const BOOST_NOEXCEPT {
				return stack_.size() - 1;
			}

			/// @see DirectoryIteratorBase#next
			void RecursiveDirectoryIterator::next() {
				if(!hasNext())
					throw NoSuchElementException();
				if(doesntPushNext_)
					doesntPushNext_ = false;
				else if(stack_.top()->isDirectory()) {
					PathString subdir(directory());
					subdir += PATH_SEPARATORS[0];
					subdir += current();
					std::unique_ptr<DirectoryIterator> sub(new DirectoryIterator(subdir.c_str()));
					if(sub->hasNext()) {
						stack_.push(sub.release());
						return;
					}
				}
				stack_.top()->next();
				while(!stack_.top()->hasNext() && stack_.size() > 1) {
					delete stack_.top();
					stack_.pop();
					assert(stack_.top()->hasNext());
					stack_.top()->next();
				}
			}

			/// 
			void RecursiveDirectoryIterator::pop() {
				while(!stack_.empty()) {
					stack_.top()->next();
					if(stack_.top()->hasNext())
						break;
					delete stack_.top();
					stack_.pop();
				}
			}
#endif // !ASCENSION_NO_GREP
		}
	}
}

