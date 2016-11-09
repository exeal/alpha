/**
 * @file text-file-document-input.cpp
 * Implements @c TextFileDocumentInput class.
 * @author exeal
 * @date 2007 (separated from document.cpp)
 * @date 2016-09-21 Separated from fileio.cpp.
 */

#include <ascension/corelib/encoding/encoder-factory.hpp>
#include <ascension/kernel/fileio/text-file-document-input.hpp>
#include <ascension/kernel/fileio/text-file-stream-buffer.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/range/algorithm/copy.hpp>
#if ASCENSION_OS_POSIX
#	include <cstdio>		// std.tempnam
#	include <fcntl.h>		// fcntl
#	include <unistd.h>		// fcntl
#endif // !ASCENSION_OS_POSIX
#if BOOST_OS_WINDOWS
#	include <cwctype>
#endif


namespace ascension {
	namespace kernel {
		namespace fileio {
			namespace {
				/**
				 * Creates a name for a temporary file.
				 * @param seed The string contains a directory path and a prefix string
				 * @return The result temporary file path name
				 * @throw std#bad_alloc POSIX @c tempnam failed (only when @c ASCENSION_OS_POSIX was defined)
				 * @throw IOException Any I/O error occurred
				 */
				boost::filesystem::path makeTemporaryFileName(const boost::filesystem::path& seed) {
					const boost::filesystem::path name(seed.filename());
					boost::filesystem::path parentPath(seed);
					if(name != parentPath)
						parentPath.remove_filename();
#if BOOST_OS_WINDOWS
					boost::filesystem::path::value_type result[MAX_PATH];
					if(::GetTempFileNameW(parentPath.c_str(), name.c_str(), 0, result) != 0)
						return result;
#else // ASCENSION_OS_POSIX
					if(boost::filesystem::path::value_type* p = ::tempnam(parentPath.c_str(), name.c_str())) {
						boost::filesystem::path result(p);
						::free(p);
						return result;
					} else if(errno == ENOMEM)
						throw std::bad_alloc();	// tempnam failed
#endif
					throw makePlatformError();
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
						std::unique_ptr<encoding::Encoder> encoder(encoding::EncoderRegistry::instance().forName(encoding));
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
			 * @note Unlike @c boost#filesystem#canonical, this function resolve real names of path.
			 * @see comparePathNames
			 */
			boost::filesystem::path canonicalizePathName(const boost::filesystem::path& pathName) {
#if BOOST_OS_WINDOWS
				static const std::array<boost::filesystem::path::value_type, 2> PATH_SEPARATORS = {0x005cu, 0x002fu};	// \ or /
#else // ASCENSION_OS_POSIX
				static const std::array<boost::filesystem::path::value_type, 1> PATH_SEPARATORS = {'/'};
#endif
//				static const boost::filesystem::path::value_type PREFERRED_PATH_SEPARATOR = PATH_SEPARATORS[0];
				const auto native(pathName.native());

#if BOOST_OS_WINDOWS
				// resolve relative path name
				std::array<WCHAR, MAX_PATH> shortFullName;
				std::unique_ptr<WCHAR[]> longFullName;
				WCHAR* fullName = shortFullName.data();
				DWORD fullNameLength = ::GetFullPathNameW(native.c_str(), std::tuple_size<decltype(shortFullName)>::value, fullName, nullptr);
				if(fullNameLength > std::tuple_size<decltype(shortFullName)>::value) {
					longFullName.reset(new WCHAR[fullNameLength + 1]);
					fullName = longFullName.get();
					fullNameLength = ::GetFullPathNameW(native.c_str(), fullNameLength + 1, fullName, nullptr);
				}
				if(fullNameLength == 0)
					throw makePlatformError();

				// get real component names (from Ftruename implementation in xyzzy)
				boost::filesystem::path result;
				auto view(boost::make_iterator_range(fullName, fullName + fullNameLength));
				if(((view[0] >= L'A' && view[0] <= L'Z') || (view[0] >= L'a' && view[0] <= L'z'))
						&& view[1] == L':' && boost::find(PATH_SEPARATORS, view[2]) != boost::end(PATH_SEPARATORS)) {	// drive letter
					fullName[0] = std::towupper(fullName[0]);	// unify with uppercase letters... (no longer needed)
					result /= boost::filesystem::path::string_type(fullName, 3);
					view.advance_begin(+3);
				} else if(std::all_of(std::begin(view), std::begin(view) + 1,
						[](boost::filesystem::path::value_type c) {
							return boost::find(PATH_SEPARATORS, c) != boost::end(PATH_SEPARATORS);
						})) {	// UNC?
					view = boost::make_iterator_range(boost::find_first_of(view.advance_begin(+2), PATH_SEPARATORS), boost::end(view));
					if(view.empty())	// server name
						return pathName;
					view = boost::make_iterator_range(boost::find_first_of(view.advance_begin(+1), PATH_SEPARATORS), boost::end(view));
					if(view.empty())	// shared name
						return pathName;
					result /= boost::filesystem::path::string_type(fullName, std::begin(view.advance_begin(+1)));
				} else	// not absolute name
					return pathName;

				WIN32_FIND_DATAW wfd;
				while(true) {
					auto next = boost::find_first_of(view, PATH_SEPARATORS);
					if(next != boost::end(view)) {
						const boost::filesystem::path::value_type c = *next;
						*next = 0;
						HANDLE h = ::FindFirstFileW(fullName, &wfd);
						if(h != INVALID_HANDLE_VALUE) {
							::FindClose(h);
							result /= wfd.cFileName;
						} else
							result /= boost::filesystem::path::string_type(std::begin(view), std::end(view));
						*next = c;
						view = boost::make_iterator_range(std::next(next), std::end(view));
					} else {
						HANDLE h = ::FindFirstFileW(fullName, &wfd);
						if(h != INVALID_HANDLE_VALUE) {
							::FindClose(h);
							result /= wfd.cFileName;
						} else
							result /= boost::filesystem::path::string_type(std::begin(view), std::end(view));
						break;
					}
				}
				return result;

#else // ASCENSION_OS_POSIX

				if(char* const resolved = ::realpath(pathName, nullptr)) {
					boost::filesystem::path result(resolved);
					::free(resolved);
					return result;
				}
				throw makePlatformError();
#endif
			}

			/**
			 * Inserts the contents of the file into the specified position.
			 * @param document The document
			 * @param at The position into which the contents is inserted
			 * @param fileName The file name
			 * @param encoding The character encoding of the input file or auto detection name
			 * @param encodingSubstitutionPolicy The substitution policy used in encoding conversion
			 * @return A tuple consists of three values: (0) the encoding used to convert, (1) the boolean value means
			 *         if the input contained Unicode byte order mark and (2) the position of the end of the inserted
			 *         text string
			 * @throw UnmappableCharacterException
			 * @throw MalformedInputException
			 * @throw ... Any exceptions @c TextFileStreamBuffer#TextFileStreamBuffer and @c kernel#insert throw
			 */
			std::tuple<std::string, bool, Position> insertFileContents(
					Document& document, const Position& at, const boost::filesystem::path& fileName,
					const std::string& encoding, encoding::Encoder::SubstitutionPolicy encodingSubstitutionPolicy) {
				TextFileStreamBuffer sb(fileName, std::ios_base::in, encoding, encodingSubstitutionPolicy, false);
				std::istreambuf_iterator<Char> i(&sb);
				const auto eos(insert(document, at, i, std::istreambuf_iterator<Char>()));
				const auto realEncoding(sb.encoding());
				const auto unicodeByteOrderMark(sb.unicodeByteOrderMark());
				sb.close();
				return std::make_tuple(realEncoding, unicodeByteOrderMark, eos);
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
			void writeRegion(const Document& document, const Region& region,
					const boost::filesystem::path& fileName, const WritingFormat& format, bool append /* = false */) {
				// verify encoding-specific newline
				verifyNewline(format.encoding, format.newline);

				// check if not special file
				if(!boost::filesystem::is_regular_file(fileName))
					throw detail::makeGenericFileSystemError("ascension.fileio.writeRegion", fileName, boost::system::errc::no_such_device_or_address);

				// check if writable
#if BOOST_OS_WINDOWS
				const DWORD originalAttributes = ::GetFileAttributesW(fileName.native().c_str());
				if(originalAttributes != INVALID_FILE_ATTRIBUTES && win32::boole(originalAttributes & FILE_ATTRIBUTE_READONLY))
#else // ASCENSION_OS_POSIX
				struct stat originalStat;
				bool gotStat = ::stat(fileName.native().c_str(), &originalStat) == 0;
#if 1
				if(::euidaccess(fileName.native().c_str(), 2) < 0)
#else
				if(::access(fileName.native().c_str(), 2) < 0)
#endif
#endif
					throw detail::makeGenericFileSystemError("ascension.fileio.writeRegion", fileName, boost::system::errc::permission_denied);

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


			// TextFileDocumentInput.FileLocker ///////////////////////////////////////////////////////////////////////

			class TextFileDocumentInput::FileLocker : private boost::noncopyable {
			public:
				FileLocker() BOOST_NOEXCEPT;
				~FileLocker() BOOST_NOEXCEPT;
				bool hasLock() const BOOST_NOEXCEPT;
				bool lock(const boost::filesystem::path& fileName, bool share);
				LockType type() const BOOST_NOEXCEPT;
				bool unlock() BOOST_NOEXCEPT;
private:
				LockType type_;
#if BOOST_OS_WINDOWS
				HANDLE file_;
#else // ASCENSION_OS_POSIX
				int file_;
				bool deleteFileOnClose_;
#endif
				boost::filesystem::path fileName_;
			};

			/// Default constructor.
			TextFileDocumentInput::FileLocker::FileLocker() BOOST_NOEXCEPT : type_(NO_LOCK), file_(
#if BOOST_OS_WINDOWS
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
#if BOOST_OS_WINDOWS
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
			 * @throw boost#filesystem#filesystem_error
			 */
			bool TextFileDocumentInput::FileLocker::lock(const boost::filesystem::path& fileName, bool share) {
				if(fileName.empty())
					throw detail::makeGenericFileSystemError("ascension.fileio.TextFileDocumentInput.FileLocker.lock", fileName, boost::system::errc::no_such_file_or_directory);
				bool alreadyShared = false;
#if ASCENSION_OS_POSIX
				flock fl;
				fl.l_whence = SEEK_SET;
				fl.l_start = 0;
				fl.l_len = 0;
#endif // ASCENSION_OS_POSIX

				if(share) {
					// check the file had already been shared-locked
#if BOOST_OS_WINDOWS
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

#if BOOST_OS_WINDOWS
				HANDLE f = ::CreateFileW(fileName.c_str(), GENERIC_READ,
					share ? FILE_SHARE_READ : 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
				if(f == INVALID_HANDLE_VALUE) {
					const DWORD e = ::GetLastError();
					if(e != ERROR_FILE_NOT_FOUND)
						throw detail::makeFileSystemError("CreateFileW() returned INVALID_HANDLE_VALUE.", fileName, e);
					f = ::CreateFileW(fileName.c_str(), GENERIC_READ,
						share ? FILE_SHARE_READ : 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
					if(f == INVALID_HANDLE_VALUE)
						throw detail::makeFileSystemError("CreateFileW() returned INVALID_HANDLE_VALUE.", fileName);
				}
#else // ASCENSION_OS_POSIX
				int f = ::open(fileName.c_str(), O_RDONLY);
				if(f == -1) {
					if(errno != ENOENT)
						throw detail::makeGenericFileSystemError("open(2) returned -1.", fileName);
					f = ::open(fileName.c_str(), O_RDONLY | O_CREAT);
					if(f == -1)
						throw detail::makeGenericFileSystemError("open(2) returned -1.", fileName);
					fl.l_type = share ? F_RDLCK : F_WRLCK;
					if(::fcntl(f, F_SETLK, &fl) == 0) {
						SystemErrorSaver ses;
						::close(f);
						throw detail::makeFileSystemError("fcntl(2) returned 0.", fileName, ses.code().value());
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
#if BOOST_OS_WINDOWS
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
			 * @c TextFileDocumentInput uses @c TextFileStreamBuffer class to read/write the text file. @c #bind opens
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
			 * You can lock the opened file to guard from other processes' changing. @c LockMode parameter of @c #bind
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
			void TextFileDocumentInput::bind(const boost::filesystem::path& fileName) {
//				sanityCheckPathName(fileName, "fileName");
				if(fileName.empty())
					return unbind();

				const boost::filesystem::path realName(canonicalizePathName(fileName));
				if(!boost::filesystem::exists(realName))
					throw detail::makeGenericFileSystemError("ascension.fileio.TextFileDocumentInput.bind", fileName, boost::system::errc::no_such_file_or_directory);
				if(fileLocker_->hasLock()) {
					assert(fileLocker_->type() == desiredLockMode_.type);
					if(desiredLockMode_.onlyAsEditing)
						assert(!document().isModified());
//						assert(savedDocumentRevision_ != document().revisionNumber());
					fileLocker_->lock(realName, fileLocker_->type() == SHARED_LOCK);
				}

				if(weakSelf_.get() == nullptr)
					weakSelf_.reset(this, boost::null_deleter());
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
				std::time_t newTimeStamp;
				if(!verifyTimeStamp(false, newTimeStamp)) {
					const boost::optional<std::time_t> original(userLastWriteTime_);
					userLastWriteTime_ = boost::none;
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
			bool TextFileDocumentInput::isChangeable(const Document&) const BOOST_NOEXCEPT {
				if(isBoundToFile()) {
					// check the time stamp if this is the first modification
					if(timeStampDirector_ != nullptr && !document().isModified()) {
						std::time_t realTimeStamp;
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
			DocumentInput::LocationType TextFileDocumentInput::location() const BOOST_NOEXCEPT {
#ifndef ASCENSION_ABANDONED_AT_VERSION_08
				return fileName().native();
#else
#if BOOST_OS_WINDOWS
				return fileName().native();
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
				const bool recorded = document().isRecordingChanges();
				document_.recordChanges(false);
				try {
					std::tie(encoding_, unicodeByteOrderMark_, std::ignore) =
						insertFileContents(document_, *boost::const_begin(document().region()), fileName(), encoding, encodingSubstitutionPolicy);
				} catch(...) {
					document_.resetContent();
					document_.recordChanges(recorded);
					throw;
				}
				document_.recordChanges(recorded);

				// set the new properties of the document
				savedDocumentRevision_ = document().revisionNumber();
				timeStampDirector_ = unexpectedTimeStampDirector;
				{
					String titleString;
#if BOOST_OS_WINDOWS
					static_assert(sizeof(wchar_t) == 2, "");
					const auto& title(fileName().native());
#elif !defined(BOOST_NO_CXX11_CHAR16_T)
#	error Not implemented.
#else // ASCENSION_OS_POSIX
#	error Not implemented.
#endif
					boost::copy(title, std::back_inserter(titleString));
					document_.setProperty(Document::TITLE_PROPERTY, titleString);
				}

				newline_ = document().lineContent(0).newline();	// use the newline of the first line
				listeners_.notify<const TextFileDocumentInput&>(&FilePropertyListener::fileEncodingChanged, *this);
				listeners_.notify<const TextFileDocumentInput&>(&FilePropertyListener::fileNameChanged, *this);

				document_.clearUndoBuffer();
				document_.markUnmodified();

				// update the internal time stamp
				try {
					internalLastWriteTime_ = boost::filesystem::last_write_time(fileName());
					userLastWriteTime_ = internalLastWriteTime_;
				} catch(std::ios_base::failure&) {
					// ignore...
				}
			}

			/**
			 * Sets the character encoding. This method invokes @c FilePropertyListener#fileEncodingChanged.
			 * @param encoding The encoding
			 * @return This object
			 * @throw encoding#UnsupportedEncodingException @a encoding is not supported
			 * @see #encoding
			 */
			TextFileDocumentInput& TextFileDocumentInput::setEncoding(const std::string& encoding) {
				if(!encoding.empty() && !encoding::EncoderRegistry::instance().supports(encoding))
					throw encoding::UnsupportedEncodingException("encoding");
				encoding_ = encoding;
				listeners_.notify<const TextFileDocumentInput&>(&FilePropertyListener::fileEncodingChanged, *this);
				return *this;
			}

			/**
			 * Sets the newline. This method invokes @c FilePropertyListener#fileEncodingChanged.
			 * @param newline The newline
			 * @return This object
			 * @throw UnknownValueException @a newline is not literal
			 * @see #newline
			 */
			TextFileDocumentInput& TextFileDocumentInput::setNewline(const text::Newline& newline) {
				if(!newline.isLiteral())
					throw UnknownValueException("newline");
				else if(newline != this->newline()) {
					newline_ = newline;
					listeners_.notify<const TextFileDocumentInput&>(&FilePropertyListener::fileEncodingChanged, *this);
				}
				return *this;
			}

			/**
			 * Sets the unicode-byte-order-mark attribute. This method invokes
			 * @c FilePropertyListener#fileEncodingChanged.
			 * @param set The new value to set
			 * @return This object
			 * @tsee #unicodeByteOrder
			 */
			TextFileDocumentInput& TextFileDocumentInput::setUnicodeByteOrderMark(bool set) BOOST_NOEXCEPT {
				if(set != unicodeByteOrderMark()) {
					unicodeByteOrderMark_ = set;
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
					fileName_.clear();
					listeners_.notify<const TextFileDocumentInput&>(&FilePropertyListener::fileNameChanged, *this);
					setEncoding(encoding::Encoder::defaultInstance().properties().name());
					userLastWriteTime_ = internalLastWriteTime_ = boost::none;
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
			bool TextFileDocumentInput::verifyTimeStamp(bool internal, std::time_t& newTimeStamp) BOOST_NOEXCEPT {
				const boost::optional<std::time_t>& about = internal ? internalLastWriteTime_ : userLastWriteTime_;
				if(!isBoundToFile() || about == boost::none || fileLocker_->hasLock())
					return true;	// not managed

				try {
					newTimeStamp = boost::filesystem::last_write_time(fileName());
				} catch(const boost::filesystem::filesystem_error&) {
					return true;
				}
				return std::difftime(*about, newTimeStamp) >= 0;
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
			 * @throw ...
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
					std::time_t realTimeStamp;
					if(!verifyTimeStamp(true, realTimeStamp)) {
						if(!timeStampDirector_->queryAboutUnexpectedDocumentFileTimeStamp(
								document_, UnexpectedFileTimeStampDirector::OVERWRITE_FILE))
							return;
					}
				}

				// TODO: backup the file.
				const bool makeBackup = false;

				// create a temporary file and write into
				const boost::filesystem::path tempFileName(makeTemporaryFileName(fileName()));
				writeRegion(document(), document().region(), tempFileName, format, false);

				// copy file attributes (file mode) and delete the old file
				try {
					if(fileLocker_->type() != NO_LOCK)
						unlockFile();

					bool fileMayLost = false;
					boost::system::error_code ignored;
#if BOOST_OS_WINDOWS
					const DWORD attributes = ::GetFileAttributesW(fileName().c_str());
					if(attributes != INVALID_FILE_ATTRIBUTES) {
						::SetFileAttributesW(tempFileName.c_str(), attributes);
#else // ASCENSION_OS_POSIX
					struct stat s;
					bool fileLost = false;
					if(::stat(fileName().c_str(), &s) != -1) {
						::chmod(tempFileName.c_str(), s.st_mode);
#endif
						fileMayLost = true;

						if(makeBackup) {
						} else {
							try {
								boost::filesystem::remove(fileName());
							} catch(const boost::filesystem::filesystem_error& e) {
								assert(e.code().value() != boost::system::errc::no_such_file_or_directory);
								boost::filesystem::remove(tempFileName, ignored);	// ignore the result
								throw;
							}
						}
					}
					try {
						boost::filesystem::rename(tempFileName, fileName());
					} catch(const boost::filesystem::filesystem_error&) {
						if(fileMayLost)
							throw std::ios_base::failure("lost the disk file.");
						boost::filesystem::remove(fileName(), ignored);	// ignore the result
						throw;
					}
				} catch(...) {
					try {
						lockFile(desiredLockMode_);
					} catch(...) {
						throw;
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
					internalLastWriteTime_ = boost::filesystem::last_write_time(fileName_);
				} catch(const boost::filesystem::filesystem_error&) {
					internalLastWriteTime_ = boost::none;
				}
				userLastWriteTime_ = internalLastWriteTime_;
			}
		}
	}
}
