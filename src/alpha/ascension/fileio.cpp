/**
 * @file fileio.cpp
 * Implements @c ascension#kernel#fileio namespace.
 * @note Currently, this implementation does not support OpenVMS.
 * @author exeal
 * @date 2007 (separated from document.cpp)
 * @date 2007-2008
 */

#include "document.hpp"
#include <limits>	// std.numeric_limits
#ifdef ASCENSION_POSIX
#	include <errno.h>		// errno
#	include <fcntl.h>		// fcntl
#	include <unistd.h>		// fcntl
#	include <sys/mman.h>	// mmap, munmap, ...
#endif // !ASCENSION_POSIX

using namespace ascension::kernel;
using namespace ascension::kernel::fileio;
using namespace ascension::encoding;
using namespace std;
namespace a = ascension;
using manah::toBoolean;


// free function ////////////////////////////////////////////////////////////

namespace {
#ifdef ASCENSION_WINDOWS
	static const Char PATH_SEPARATORS[] = L"\\/";
#else // ASCENSION_POSIX
	static const Char PATH_SEPARATORS[] = "/";
#endif
	static const Char PREFERRED_PATH_SEPARATOR = PATH_SEPARATORS[0];
	/// Returns true if the given character is a path separator.
	inline bool isPathSeparator(Char c) /*throw()*/ {
		return find(PATH_SEPARATORS, MANAH_ENDOF(PATH_SEPARATORS) - 1, c) != MANAH_ENDOF(PATH_SEPARATORS) - 1;}
	/**
	 * Returns true if the specified file or directory exists.
	 * @param name the name of the file
	 * @return if the file exists
	 * @throw NullPointerException @a fileName is @c null
	 * @throw IOException(files#IOException#PLATFORM_DEPENDENT_ERROR) any I/O error occured. for
	 * details, use POSIX @c errno or Win32 @c GetLastError
	 */
	bool pathExists(const Char* name) {
		if(name == 0)
			throw a::NullPointerException("name");
#ifdef ASCENSION_WINDOWS
#ifdef PathFileExists
		return toBoolean(::PathFileExistsW(name));
#else
		if(::GetFileAttributesW(name) != INVALID_FILE_ATTRIBUTES)
			return true;
		const DWORD e = ::GetLastError();
		if(e == ERROR_FILE_NOT_FOUND || e == ERROR_PATH_NOT_FOUND
				|| e == ERROR_INVALID_NAME || e == ERROR_INVALID_PARAMETER || e == ERROR_BAD_NETPATH)
			return false;
#endif /* PathFileExists */
#else // ASCENSION_POSIX
		struct stat s;
		if(::stat(name, &s) == 0)
			return true;
		else if(errno == ENOENT)
			return false;
#endif
		throw IOException(IOException::PLATFORM_DEPENDENT_ERROR);
	}

	/// Finds the base name in the given file path name.
	inline String::const_iterator findFileName(const String& s) {
		const String::size_type i = s.find_last_of(PATH_SEPARATORS);
		return s.begin() + ((i != String::npos) ? i + 1 : 0);
	}

	/**
	 * Returns the last write time of the specified file.
	 * @param fileName the name of the file
	 * @param[out] timeStamp the time
	 * @throw IOException any I/O error occured
	 */
	void getFileLastWriteTime(const String& fileName, TextFileDocumentInput::Time& timeStamp) {
#ifdef ASCENSION_WINDOWS
		WIN32_FILE_ATTRIBUTE_DATA attributes;
		if(::GetFileAttributesExW(fileName.c_str(), GetFileExInfoStandard, &attributes) != 0)
			timeStamp = attributes.ftLastWriteTime;
		else {
			const DWORD e = ::GetLastError();
			throw IOException((e == ERROR_FILE_NOT_FOUND
				|| e == ERROR_PATH_NOT_FOUND) ? IOException::FILE_NOT_FOUND : IOException::PLATFORM_DEPENDENT_ERROR);
		}
#else // ASCENSION_POSIX
		struct stat s;
		if(::stat(fileName.c_str(), &s) == 0)
			timeStamp = s.st_mtime;
		else
			throw IOException((errno == ENOENT) ? IOException::FILE_NOT_FOUND : IOException::PLATFORM_DEPENDENT_ERROR);
#endif
	}

	/**
	 * Returns the size of the specified file.
	 * @param fileName the name of the file
	 * @return the size of the file in bytes or -1 if the file is too large
	 * @throw NullPointerException @a fileName is @c null
	 * @throw IOException any I/O error occured
	 */
	ptrdiff_t getFileSize(const Char* fileName) {
		if(fileName == 0)
			throw a::NullPointerException("fileName");
#ifdef ASCENSION_WINDOWS
		WIN32_FILE_ATTRIBUTE_DATA attributes;
		if(::GetFileAttributesExW(fileName, GetFileExInfoStandard, &attributes) == 0) {
			DWORD e = ::GetLastError();
			throw IOException((e == ERROR_PATH_NOT_FOUND || e == ERROR_INVALID_NAME
				|| e == ERROR_BAD_NETPATH) ? IOException::FILE_NOT_FOUND : IOException::PLATFORM_DEPENDENT_ERROR);
		}
		return (attributes.nFileSizeHigh == 0
			&& attributes.nFileSizeLow <= static_cast<DWORD>(numeric_limits<ptrdiff_t>::max())) ?
				static_cast<ptrdiff_t>(attributes.nFileSizeLow) : -1;
#else // ASCENSION_POSIX
		struct stat s;
		if(::stat(fileName, &s) != 0)
			throw IOException((errno == ENOENT) ? IOException::FILE_NOT_FOUND : IOException::PLATFORM_DEPENDENT_ERROR);
		return s.st_size;
#endif
	}

	/**
	 * Creates a name for a temporary file.
	 * @param seed the string contains a directory path and a prefix string
	 * @return the result string
	 * @throw IOException any I/O error occured
	 */
	String getTemporaryFileName(const String& seed) {
		manah::AutoBuffer<Char> s(new Char[seed.length() + 1]);
		copy(seed.begin(), seed.end(), s.get());
		s[seed.length()] = 0;
		Char* name = s.get() + (findFileName(seed) - seed.begin());
		if(name != s.get())
			name[-1] = 0;
#ifdef ASCENSION_WINDOWS
		WCHAR result[MAX_PATH];
		if(::GetTempFileNameW(s.get(), name, 0, result) != 0)
			return result;
#else // ASCENSION_POSIX
		if(char* p = ::tempnam(s.get(), name)) {
			String result(p);
			::free(p);
			return result;
		}
#endif
		throw IOException(IOException::PLATFORM_DEPENDENT_ERROR);
	}
} // namespace @0

/**
 * Makes the given path name real. This method will not fail even if the path name is not exist.
 * Win32 target platform: If @a pathName is a UNC, the case of @a pathName will not be fixed. All
 * slashes will be replaced by backslashes.
 * @param pathName the absolute path name
 * @return the result real path name
 * @throw NullPointerException @a pathName is @c null
 * @see comparePathNames
 */
String fileio::canonicalizePathName(const Char* pathName) {
	if(pathName == 0)
		throw a::NullPointerException("pathName");

#ifdef ASCENSION_WINDOWS

	if(wcslen(pathName) >= MAX_PATH)	// too long name
		return pathName;

	// resolve relative path name
	WCHAR path[MAX_PATH];
	WCHAR* dummy;
	if(::GetFullPathNameW(pathName, MANAH_COUNTOF(path), path, &dummy) == 0)
		wcscpy(path, pathName);

	// get real component names (from Ftruename implementation in xyzzy)
	String result;
	result.reserve(MAX_PATH);
	const Char* p = path;
	if(((p[0] >= L'A' && p[0] <= L'Z') || (p[0] >= L'a' && p[0] <= L'z'))
			&& p[1] == L':' && isPathSeparator(p[2])) {	// drive letter
		result.append(path, 3);
		result[0] = towupper(path[0]);	// unify with uppercase letters...
		p += 3;
	} else if(isPathSeparator(p[0]) && isPathSeparator(p[1])) {	// UNC?
		if((p = wcspbrk(p + 2, PATH_SEPARATORS)) == 0)	// server name
			return false;
		if((p = wcspbrk(p + 1, PATH_SEPARATORS)) == 0)	// shared name
			return false;
		result.append(path, ++p - path);
	} else	// not absolute name
		return pathName;

	WIN32_FIND_DATAW wfd;
	while(true) {
		if(Char* next = wcspbrk(p, PATH_SEPARATORS)) {
			const Char c = *next;
			*next = 0;
			HANDLE h = ::FindFirstFileW(path, &wfd);
			if(h != INVALID_HANDLE_VALUE) {
				::FindClose(h);
				result += wfd.cFileName;
			} else
				result += p;
			*next = c;
			result += PREFERRED_PATH_SEPARATOR;
			p = next + 1;
		} else {
			HANDLE h = ::FindFirstFileW(path, &wfd);
			if(h != INVALID_HANDLE_VALUE) {
				::FindClose(h);
				result += wfd.cFileName;
			} else
				result += p;
			break;
		}
	}
	return result;

#else // ASCENSION_POSIX

	Char resolved[PATH_MAX];
	return (::realpath(pathName, resolved) != 0) ? resolved : pathName;

#endif
}

/**
 * Returns true if the specified two file path names are equivalent.
 * @param s1 the first path name
 * @param s2 the second path name
 * @return true if @a s1 and @a s2 are equivalent
 * @throw NullPointerException either file name is @c null
 * @see canonicalizePathName
 */
bool fileio::comparePathNames(const Char* s1, const Char* s2) {
	if(s1 == 0 || s2 == 0)
		throw a::NullPointerException("either file name is null.");

#ifdef ASCENSION_WINDOWS
#ifdef PathMatchSpec
	if(toBoolean(::PathMatchSpecW(s1, s2)))
		return true;
#endif /* PathMatchSpec */
	// by lexicographical comparison
	const int c1 = static_cast<int>(wcslen(s1)) + 1, c2 = static_cast<int>(wcslen(s2)) + 1;
	const int fc1 = LCMapStringW(LOCALE_NEUTRAL, LCMAP_LOWERCASE, s1, c1, 0, 0);
	const int fc2 = LCMapStringW(LOCALE_NEUTRAL, LCMAP_LOWERCASE, s2, c2, 0, 0);
	if(fc1 != 0 && fc2 != 0 && fc1 == fc2) {
		manah::AutoBuffer<WCHAR> fs1(new WCHAR[fc1]), fs2(new WCHAR[fc2]);
		LCMapStringW(LOCALE_NEUTRAL, LCMAP_LOWERCASE, s1, c1, fs1.get(), fc1);
		LCMapStringW(LOCALE_NEUTRAL, LCMAP_LOWERCASE, s2, c2, fs2.get(), fc2);
		if(wmemcmp(fs1.get(), fs2.get(), fc1) == 0)
			return pathExists(s1);
	}
	// by volume information
	bool eq = false;
	HANDLE f1 = ::CreateFileW(s1, 0,
		FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	if(f1 != INVALID_HANDLE_VALUE) {
		HANDLE f2 = ::CreateFileW(s2, 0,
			FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
		if(f2 != INVALID_HANDLE_VALUE) {
			::BY_HANDLE_FILE_INFORMATION fi1;
			if(toBoolean(::GetFileInformationByHandle(f1, &fi1))) {
				::BY_HANDLE_FILE_INFORMATION fi2;
				if(toBoolean(::GetFileInformationByHandle(f2, &fi2)))
					eq = fi1.dwVolumeSerialNumber == fi2.dwVolumeSerialNumber
						&& fi1.nFileIndexHigh == fi2.nFileIndexHigh
						&& fi1.nFileIndexLow == fi2.nFileIndexLow;
			}
			::CloseHandle(f2);
		}
		::CloseHandle(f1);
	}
	return eq;
#else // ASCENSION_POSIX
	// by lexicographical comparison
	if(strcmp(s1, s2) == 0)
		return true;
	// by volume information
	struct stat st1, st2;
	return ::stat(s1, &st1) == 0 && ::stat(s2, &st2) == 0
		&& st1.st_dev == st2.st_dev && st1.st_ino == st2.st_ino
		&& st1.st_size == st2.st_size && st1.st_mtime == st2.st_mtime;
#endif
}


// TextFileStreamBuffer /////////////////////////////////////////////////////

namespace {
	class SystemErrorSaver {
	public:
#ifdef ASCENSION_WINDOWS
		SystemErrorSaver() /*throw()*/ : e_(::GetLastError()) {}
		~SystemErrorSaver() /*throw()*/ {::SetLastError(e_);}
	private:
		DWORD e_;
#else // ASCENSION_POSIX
		SystemErrorSaver() /*throw()*/ : e_(errno) {}
		~SystemErrorSaver() /*throw()*/ {errno = e_;}
	private:
		int e_;
#endif
	};
} // namespace @0

/**
 * Constructor opens the specified file.
 * @param fileName the name of the file
 * @param mode the file open mode. must be either @c std#ios_base#in or @c std#ios_base#out
 * @param encoding the file encoding or auto detection name
 * @param encodingSubstitutionPolicy the substitution policy used in encoding conversion
 * @param writeByteOrderMark set true to write Unicode byte order mark into the output file
 * @throw IOException any I/O error occured
 * @throw UnknownValueException<std#iose_base#openmode> @a mode is neigher @c in nor @c out
 */
TextFileStreamBuffer::TextFileStreamBuffer(const String& fileName, ios_base::openmode mode,
		const string& encoding, Encoder::SubstitutionPolicy encodingSubstitutionPolicy,
		bool writeByteOrderMark) : encoder_(Encoder::forName(encoding)) {
	if(!pathExists(fileName.c_str()))
		throw IOException(IOException::FILE_NOT_FOUND);
	inputMapping_.first = inputMapping_.last = inputMapping_.current = 0;
	if(mode == ios_base::in) {
		EncodingDetector* encodingDetector = 0;
		if(encoder_.get() == 0) {	// 'encoding' may be for auto-detection
			encodingDetector = EncodingDetector::forName(encoding);
			if(encodingDetector == 0)
				throw IOException(IOException::INVALID_ENCODING);
		}
		// open the file and create memory-mapped object
		const ptrdiff_t fileSize = getFileSize(fileName.c_str());
		if(fileSize == -1)
			throw IOException(IOException::HUGE_FILE);
#ifdef ASCENSION_WINDOWS
		fileHandle_ = ::CreateFileW(fileName.c_str(), GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
		if(fileHandle_ == INVALID_HANDLE_VALUE) {
			const DWORD e = ::GetLastError();
			throw IOException((e == ERROR_PATH_NOT_FOUND || e == ERROR_INVALID_NAME || e == ERROR_INVALID_PARAMETER
				|| e == ERROR_BAD_NETPATH) ? IOException::FILE_NOT_FOUND : IOException::PLATFORM_DEPENDENT_ERROR);
		}
		if(0 != (fileMapping_ = ::CreateFileMappingW(fileHandle_, 0, PAGE_READONLY, 0, 0, 0)))
			inputMapping_.first = static_cast<const byte*>(::MapViewOfFile(fileMapping_, FILE_MAP_READ, 0, 0, 0));
		if(inputMapping_.first == 0) {
			SystemErrorSaver temp;
			if(fileMapping_ != 0)
				::CloseHandle(fileMapping_);
			::CloseHandle(fileHandle_);
			throw IOException(IOException::PLATFORM_DEPENDENT_ERROR);
		}
#else // ASCENSION_POSIX
		if(-1 == (fileDescriptor_ = ::open(fileName.c_str(), O_RDONLY)))
			throw IOException((errno == ENOENT) ? IOException::FILE_NOT_FOUND : IOException::PLATFORM_DEPENDENT_ERROR);
		if(MAP_FAILED == (inputMapping_.first = static_cast<const byte*>(::mmap(0, fileSize, PROT_READ, MAP_PRIVATE, fileDescriptor_, 0)))) {
			SystemErrorSaver temp;
			inputMapping_.first = 0;
			::close(fileDescriptor_);
			throw IOException(IOException::PLATFORM_DEPENDENT_ERROR);
		}
#endif
		inputMapping_.last = inputMapping_.first + fileSize;
		// detect input encoding if neccssary
		if(encodingDetector != 0) {
			const pair<MIBenum, string> detected(encodingDetector->detect(
				inputMapping_.first, min(inputMapping_.last, inputMapping_.first + 1024 * 10), 0));
			if(detected.first != MIB_OTHER)
				encoder_ = Encoder::forMIB(detected.first);
			else
				encoder_ = Encoder::forName(detected.second);
			if(encoder_.get() == 0)
				throw IOException(IOException::INVALID_ENCODING);	// can't resolve
		}
		inputMapping_.current = inputMapping_.first;
	} else if(mode == ios_base::out) {
		if(encoder_.get() == 0)
			throw IOException(IOException::INVALID_ENCODING);
#ifdef ASCENSION_WINDOWS
		if(INVALID_HANDLE_VALUE == (fileHandle_ =
				::CreateFileW(fileName.c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, 0)))
			throw IOException(IOException::PLATFORM_DEPENDENT_ERROR);
#else // ASCENSION_POSIX
		if(-1 == (fileDescriptor_ = ::open(fileName.c_str(), O_WRONLY | O_CREAT)))
			throw IOException(IOException::PLATFORM_DEPENDENT_ERROR);
#endif
		if(writeByteOrderMark)
			encoder_->setFlags(encoder_->flags() | Encoder::UNICODE_BYTE_ORDER_MARK);
		setp(ucsBuffer_, MANAH_ENDOF(ucsBuffer_));
	} else
		throw UnknownValueException("mode");
	encoder_->setSubstitutionPolicy(encodingSubstitutionPolicy);
}

/// Destructor.
TextFileStreamBuffer::~TextFileStreamBuffer() {
	try {
		close();	// this may throw
	} catch(const IOException&) {
	}
}

/**
 * Closes the file.
 * @return this or @c null if the file is not open
 */
TextFileStreamBuffer* TextFileStreamBuffer::close() {
	sync();
#ifdef ASCENSION_WINDOWS
	if(inputMapping_.first != 0) {
		::UnmapViewOfFile(const_cast<byte*>(inputMapping_.first));
		::CloseHandle(fileMapping_);
		inputMapping_.first = 0;
		fileMapping_ = 0;
	}
	if(fileHandle_ != 0) {
		::CloseHandle(fileHandle_);
		fileHandle_ = 0;
		return this;
	}
#else // ASCENSION_POSIX
	if(inputMapping_.first != 0) {
		::munmap(const_cast<byte*>(inputMapping_.first), inputMapping_.last - inputMapping_.first);
		inputMapping_.first = 0;
	}
	if(fileDescriptor_ == -1) {
		::close(fileDescriptor_);
		fileDescriptor_ = -1;
		return this;
	}
#endif
	encoder_->resetEncodingState();
	encoder_->resetDecodingState();
	return 0;
}

/// Returns the MIBenum value of the encoding.
string TextFileStreamBuffer::encoding() const /*throw()*/ {
	return encoder_->properties().name();
}

/// Returns true if the file is open.
bool TextFileStreamBuffer::isOpen() const /*throw()*/ {
#ifdef ASCENSION_WINDOWS
	return fileHandle_ != INVALID_HANDLE_VALUE;
#else // ASCENSION_POSIX
	return fileDescriptor_ != -1;
#endif
}

/// @see std#basic_streambuf#overflow
TextFileStreamBuffer::int_type TextFileStreamBuffer::overflow(int_type c) {
	if(inputMapping_.first != 0 || sync() == -1)
		return traits_type::eof();	// not output mode or can't synchronize

	*pptr() = traits_type::to_char_type(c);
	pbump(+1);
	return traits_type::not_eof(c);
}

/// @see std#basic_streambuf#pbackfail
TextFileStreamBuffer::int_type TextFileStreamBuffer::pbackfail(int_type c) {
	if(inputMapping_.first != 0) {
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
	if(inputMapping_.first == 0 && pptr() > pbase()) {
		byte* toNext;
		const a::Char* fromNext;
		byte nativeBuffer[MANAH_COUNTOF(ucsBuffer_)];
		encoder_->setFlags(encoder_->flags() | Encoder::BEGINNING_OF_BUFFER | Encoder::END_OF_BUFFER);
		while(true) {
			const a::Char* const fromEnd = pptr();

			// conversion
			const Encoder::Result encodingResult = encoder_->fromUnicode(
				nativeBuffer, MANAH_ENDOF(nativeBuffer), toNext, pbase(), fromEnd, fromNext);
			if(encodingResult == Encoder::UNMAPPABLE_CHARACTER)
				throw IOException(IOException::UNMAPPABLE_CHARACTER);
			else if(encodingResult == Encoder::MALFORMED_INPUT)
				throw IOException(IOException::MALFORMED_INPUT);

			// write into the file
#ifdef ASCENSION_WINDOWS
			DWORD writtenBytes;
			assert(static_cast<size_t>(toNext - nativeBuffer) <= numeric_limits<DWORD>::max());
			const DWORD bytes = static_cast<DWORD>(toNext - nativeBuffer);
			if(::WriteFile(fileHandle_, nativeBuffer, bytes, &writtenBytes, 0) == 0 || writtenBytes != bytes)
				throw IOException(IOException::PLATFORM_DEPENDENT_ERROR);
#else // ASCENSION_POSIX
			const size_t bytes = toNext - nativeBuffer;
			const ssize_t writtenBytes = ::write(fileDescriptor_, nativeBuffer, bytes);
			if(writtenBytes == -1 || static_cast<size_t>(writtenBytes) != bytes)
				throw IOException(IOException::PLATFORM_DEPENDENT_ERROR);
#endif

			setp(ucsBuffer_ + (fromNext - ucsBuffer_), epptr());
			pbump(static_cast<int>(fromEnd - pbase()));	// TODO: this cast may be danger.
			if(encodingResult == Encoder::COMPLETED)
				break;
		}
		setp(ucsBuffer_, MANAH_ENDOF(ucsBuffer_));
	}
	return 0;
}

/// @see std#basic_streambuf#underflow
TextFileStreamBuffer::int_type TextFileStreamBuffer::underflow() {
	if(inputMapping_.first == 0 || inputMapping_.current >= inputMapping_.last)
		return traits_type::eof();	// not input mode or reached EOF

	a::Char* toNext;
	const byte* fromNext;
	encoder_->setFlags(encoder_->flags() | Encoder::BEGINNING_OF_BUFFER | Encoder::END_OF_BUFFER);
	switch(encoder_->toUnicode(ucsBuffer_, MANAH_ENDOF(ucsBuffer_), toNext, inputMapping_.current, inputMapping_.last, fromNext)) {
	case Encoder::UNMAPPABLE_CHARACTER:
		throw IOException(IOException::UNMAPPABLE_CHARACTER);
	case Encoder::MALFORMED_INPUT:
		throw IOException(IOException::MALFORMED_INPUT);
	default:
		break;
	}

	inputMapping_.current = fromNext;
	setg(ucsBuffer_, ucsBuffer_, toNext);
	return (toNext > ucsBuffer_) ? traits_type::to_int_type(*gptr()) : traits_type::eof();
}

/// Returns true if the internal encoder has @c Encoder#UNICODE_BYTE_ORDER_MARK flag.
bool TextFileStreamBuffer::unicodeByteOrderMark() const /*throw()*/ {
	return encoder_->flags().has(Encoder::UNICODE_BYTE_ORDER_MARK);
}


// TextFileDocumentInput ////////////////////////////////////////////////////

/**
 * @class ascension::kernel::fileio::TextFileDocumentInput
 * @c IDocumentInput implementation which initializes a document with the content of the text file.
 *
 * @note This class is not intended to be subclassed.
 *
 * @c TextFileDocumentInput uses @c TextFileStreamBuffer class to read/write the text file.
 * @c #open opens a text file and binds the document to the file. @c #write writes the content of
 * the document into the specified file.
 *
 * @code
 * extern Document d;
 * TextFileDocumentInput in(d);
 * in.open(...);   // open the file
 *
 * // .. edit the document ..
 *
 * in.write(...);  // write the file
 * in.close();
 * @endcode
 *
 * <h3>Encoding and newline of the file</h3>
 *
 * The encoding and newline of the opened file can be obtained by @c #encoding and @c #newline methods.
 *
 * <h3>Locking the opened file</h3>
 *
 * You can lock the opened file to guard from other processes' changing. @c LockMode parameter of
 * @c #open method specifies the mode of locking.
 *
 * <h3>When the other process modified the opened file</h3>
 *
 * You can detect any modification by other process using @c IUnexpectedFileTimeStampDirector.
 */

/**
 * Constructor.
 * @param document the document
 */
TextFileDocumentInput::TextFileDocumentInput(Document& document) :
		document_(document), encoding_(Encoder::getDefault().properties().name()),
		unicodeByteOrderMark_(false), newline_(ASCENSION_DEFAULT_NEWLINE), lockingFile_(
#ifdef ASCENSION_WINDOWS
		INVALID_HANDLE_VALUE
#else // ASCENSION_POSIX
		-1
#endif
		), savedDocumentRevision_(0), timeStampDirector_(0) {
	lockMode_ = DONT_LOCK | LOCK_ONLY_AS_EDITING;
	memset(&userLastWriteTime_, 0, sizeof(Time));
	memset(&internalLastWriteTime_, 0, sizeof(Time));
	document.setProperty(Document::TITLE_PROPERTY, L"");
}

/// Destructor.
TextFileDocumentInput::~TextFileDocumentInput() /*throw()*/ {
	try {
		close();
	} catch(IOException&) {
	}
}

/**
 * Registers the file property listener.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void TextFileDocumentInput::addListener(IFilePropertyListener& listener) {
	listeners_.add(listener);
}

/**
 * Checks the last modified date/time of the bound file and verifies if the other modified the
 * file. If the file is modified, the listener's
 * @c IUnexpectedFileTimeStampDerector#queryAboutUnexpectedDocumentFileTimeStamp will be called.
 * @return the value which the listener returned or true if the listener is not set
 */
bool TextFileDocumentInput::checkTimeStamp() {
	Time newTimeStamp;
	if(!verifyTimeStamp(false, newTimeStamp)) {
		Time original = userLastWriteTime_;
		memset(&userLastWriteTime_, 0, sizeof(Time));
		if(timeStampDirector_ == 0
				|| timeStampDirector_->queryAboutUnexpectedDocumentFileTimeStamp(
					document_, IUnexpectedFileTimeStampDirector::CLIENT_INVOCATION)) {
			userLastWriteTime_ = newTimeStamp;
			return true;
		}
		userLastWriteTime_ = original;
		return false;
	}
	return true;
}

/**
 * Closes the file and unbind from the document.
 * @note This method does NOT reset the content of the document.
 * @throw IOException any I/O error occured
 */
void TextFileDocumentInput::close() {
	if(!unlock())
		throw IOException(IOException::PLATFORM_DEPENDENT_ERROR);
	if(isOpen()) {
		if(document_.input() == this)
			document_.setInput(0, false);	// unbind
		fileName_.erase();
		listeners_.notify<const TextFileDocumentInput&>(&IFilePropertyListener::fileNameChanged, *this);
		setEncoding(Encoder::getDefault().properties().name());
		memset(&userLastWriteTime_, 0, sizeof(Time));
		memset(&internalLastWriteTime_, 0, sizeof(Time));
	}
}

/// @see IDocumentStateListener#documentAccessibleRegionChanged
void TextFileDocumentInput::documentAccessibleRegionChanged(const Document&) {
}

/// @see IDocumentStateListener#documentModificationSignChanged
void TextFileDocumentInput::documentModificationSignChanged(const Document&) {
	if(toBoolean(lockMode_ & LOCK_ONLY_AS_EDITING) && isOpen()) {
		if(document_.isModified())
			lock();
		else
			unlock();
	}
}

/// @see IDocumentStateListener#documentPropertyChanged
void TextFileDocumentInput::documentPropertyChanged(const Document&, const DocumentPropertyKey&) {
}

/// @see IDocumentStateListener#documentAccessibleRegionChanged
void TextFileDocumentInput::documentReadOnlySignChanged(const Document&) {
}

/// Returns the file extension name or an ampty string if the document is not bound to any of the files.
String TextFileDocumentInput::extensionName() const /*throw()*/ {
	const String s(pathName());
	const String::size_type dot = s.rfind('.');
	return (dot != String::npos) ? s.substr(dot + 1) : String();
}

/// @see IDocumentInput#isChangeable
bool TextFileDocumentInput::isChangeable() const {
	// check the time stamp if this is the first modification
	if(timeStampDirector_ != 0 && !document_.isModified()) {
		Time realTimeStamp;
		TextFileDocumentInput& self = const_cast<TextFileDocumentInput&>(*this);
		if(!self.verifyTimeStamp(true, realTimeStamp)) {	// the other overwrote the file
			if(!timeStampDirector_->queryAboutUnexpectedDocumentFileTimeStamp(
					document_, IUnexpectedFileTimeStampDirector::FIRST_MODIFICATION))
				return false;
			self.internalLastWriteTime_ = self.userLastWriteTime_ = realTimeStamp;
		}
	}
	return true;
}

/// @see IDocumentInput#location
a::String TextFileDocumentInput::location() const /*throw()*/ {
#ifdef ASCENSION_WINDOWS
	return fileName_;
#else // ASCENSION_POSIX
	const codecvt<a::Char, Char, mbstate_t>& converter = use_facet<codecvt<a::Char, Char, mbstate_t> >(locale());
	a::Char result[PATH_MAX * 2];
	mbstate_t dummy;
	const Char* fromNext;
	a::Char* toNext;
	return (converter.in(dummy, fileName_.c_str(),
		fileName_.c_str() + fileName_.length() + 1, fromNext, result, endof(result), toNext) == codecvt_base::ok ? result : L"");
#endif
}

/**
 * Locks the file.
 * @return true if locked successfully or the lock mode is @c DONT_LOCK
 */
bool TextFileDocumentInput::lock() /*throw()*/ {
	unlock();
	if((lockMode_ & LOCK_TYPE_MASK) == DONT_LOCK && isOpen()) {
#ifdef ASCENSION_WINDOWS
		assert(lockingFile_ == INVALID_HANDLE_VALUE);
		lockingFile_ = ::CreateFileW(fileName_.c_str(), GENERIC_READ,
			((lockMode_ & LOCK_TYPE_MASK) == SHARED_LOCK) ? FILE_SHARE_READ : 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if(lockingFile_ == INVALID_HANDLE_VALUE)
			return false;
#else // ASCENSION_POSIX
		assert(lockingFile_ == -1);
		if(-1 == (lockingFile_ = ::open(fileName_.c_str(), O_RDONLY)))
			return false;
		::flock fl;
		fl.l_whence = SEEK_SET;
		fl.l_start = 0;
		fl.l_len = 0;
		fl.l_type = ((lockMode_ & LOCK_TYPE_MASK) == SHARED_LOCK) ? F_RDLCK : F_WRLCK;
		if(::fcntl(lockingFile_, F_SETLK, &fl) == 0) {
			::close(lockingFile_);
			lockingFile_ = -1;
		}
#endif
	}
	return true;
}

/// Returns the file name or an empty string if the document is not bound to any of the files.
String TextFileDocumentInput::name() const /*throw()*/ {
	const String::const_iterator p = findFileName(fileName_);
	return fileName_.substr(p - fileName_.begin());
}

/**
 * Binds the document to the specified file. This method call document's @c Document#setInput.
 * @param fileName the file name. this method doesn't resolve the short cut
 * @param lockMode the lock mode. this method may fail to lock with desired mode. see the
 * description of the return value
 * @param encoding the file encoding or auto detection name
 * @param encodingSubstitutionPolicy the substitution policy used in encoding conversion
 * @param unexpectedTimeStampDirector
 * @return true if succeeded to lock the file with the desired mode @a lockMode
 * @throw IOException any I/O error occured. in this case, the document's content will be lost
 */
bool TextFileDocumentInput::open(const String& fileName, LockMode lockMode, const std::string& encoding,
		Encoder::SubstitutionPolicy encodingSubstitutionPolicy, IUnexpectedFileTimeStampDirector* unexpectedTimeStampDirector /* = 0 */) {
//	Timer tm(L"FileBinder.bind");	// 2.86s / 1MB
	unlock();
	document_.resetContent();
	timeStampDirector_ = 0;

	// read from the file
	TextFileStreamBuffer sb(fileName, ios_base::in, encoding, encodingSubstitutionPolicy, false);
	const bool recorded = document_.isRecordingChanges();
	document_.recordChanges(false);
	try {
		basic_istream<a::Char> in(&sb);
		in.exceptions(ios_base::badbit);
		document_.insert(document_.region().beginning(), in);
	} catch(...) {
		document_.resetContent();
		document_.recordChanges(recorded);
		throw;
	}
	document_.recordChanges(recorded);
	unicodeByteOrderMark_ = sb.unicodeByteOrderMark();
	sb.close();

	// lock the file
	bool lockSucceeded = true;
	lockMode_ = lockMode;
	if((lockMode_ & LOCK_TYPE_MASK) != DONT_LOCK && !toBoolean(lockMode_ & LOCK_ONLY_AS_EDITING)) {
		if(!(lockSucceeded = lock())) {
			if((lockMode_ & LOCK_TYPE_MASK) == EXCLUSIVE_LOCK) {
				lockMode_ &= ~LOCK_TYPE_MASK;
				lockMode_ |= SHARED_LOCK;
				if(!lock()) {
					lockMode_ &= ~LOCK_TYPE_MASK;
					lockMode_ |= DONT_LOCK;
				}
			}
		}
	}

	// set the new properties of the document
	savedDocumentRevision_ = document_.revisionNumber();
	timeStampDirector_ = unexpectedTimeStampDirector;
	fileName_ = canonicalizePathName(fileName.c_str());
#ifdef ASCENSION_WINDOWS
	document_.setProperty(Document::TITLE_PROPERTY, name());
#else // ASCENSION_POSIX
	const String title(name());
	const locale lc("");
	const codecvt<a::Char, Char, mbstate_t>& conv = use_facet<codecvt<a::Char, Char, mbstate_t> >(lc);
	mbstate_t state;
	const Char* fromNext;
	a::Char* ucsNext;
	manah::AutoBuffer<a::Char> ucs(new a::Char[title.length() * 2]);
	if(codecvt_base::ok == conv.in(state,
			title.data(), title.data() + title.length(), fromNext, ucs.get(), ucs.get() + title.length() * 2, ucsNext)) {
		*ucsNext = L'0';
		document_.setProperty(Document::TITLE_PROPERTY, ucs.get());
	}
#endif
	encoding_ = sb.encoding();
	newline_ = document_.getLineInformation(0).newline();	// use the newline of the first line
	listeners_.notify<const TextFileDocumentInput&>(&IFilePropertyListener::fileEncodingChanged, *this);
	listeners_.notify<const TextFileDocumentInput&>(&IFilePropertyListener::fileNameChanged, *this);

	document_.clearUndoBuffer();
	document_.markUnmodified();

	// update the internal time stamp
	try {
		getFileLastWriteTime(fileName_, internalLastWriteTime_);
		userLastWriteTime_ = internalLastWriteTime_;
	} catch(IOException&) {
		// ignore...
	}

	document_.setInput(this, false);
	return lockSucceeded;
}

/**
 * Removes the file property listener.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void TextFileDocumentInput::removeListener(IFilePropertyListener& listener) {
	listeners_.remove(listener);
}

/**
 * Sets the encoding.
 * @param encoding the encoding
 * @throw encoding#UnsupportedEncodingException @a encoding is not supported
 * @see #encoding
 */
void TextFileDocumentInput::setEncoding(const string& encoding) {
	if(!encoding.empty() && !Encoder::supports(encoding))
		throw UnsupportedEncodingException("encoding");
	encoding_ = encoding;
	listeners_.notify<const TextFileDocumentInput&>(&IFilePropertyListener::fileEncodingChanged, *this);
}

/**
 * Sets the newline.
 * @param newline the newline
 * @throw UnknownValueException @a newline is invalid
 */
void TextFileDocumentInput::setNewline(Newline newline) {
	if(!isLiteralNewline(newline))
		throw UnknownValueException("newline");
	else if(newline != newline_) {
		newline_ = newline;
		listeners_.notify<const TextFileDocumentInput&>(&IFilePropertyListener::fileEncodingChanged, *this);
	}
}

/**
 * Unlocks the file.
 * @return succeeded or not
 */
bool TextFileDocumentInput::unlock() /*throw()*/ {
#ifdef ASCENSION_WINDOWS
	if(lockingFile_ != INVALID_HANDLE_VALUE) {
		if(!toBoolean(::CloseHandle(lockingFile_)))
			return false;
		lockingFile_ = INVALID_HANDLE_VALUE;
	}
#else // ASCENSION_POSIX
	if(lockingFile_ != -1) {
		if(::close(lockingFile_) != 0)
			return false;
		lockingFile_ = -1;
	}
#endif
	return true;
}

/**
 * Returns last modified time.
 * @param internal set true for @c internalLastWriteTime_, false for @c userLastWriteTime_
 * @param[out] newTimeStamp the actual time stamp
 * @return false if not match
 */
bool TextFileDocumentInput::verifyTimeStamp(bool internal, Time& newTimeStamp) /*throw()*/ {
	static Time uninitialized;
	static bool initializedUninitialized = false;
	if(!initializedUninitialized)
		memset(&uninitialized, 0, sizeof(Time));

	const Time& about = internal ? internalLastWriteTime_ : userLastWriteTime_;
	if(!isOpen()
			|| memcmp(&about, &uninitialized, sizeof(Time)) == 0
			|| (lockMode_ & LOCK_TYPE_MASK) != DONT_LOCK)
		return true;	// not managed

	try {
		getFileLastWriteTime(fileName_.c_str(), newTimeStamp);
	} catch(IOException&) {
		return true;
	}
#ifdef ASCENSION_WINDOWS
	return ::CompareFileTime(&about, &newTimeStamp) != -1;
#else // ASCENSION_POSIX
	return about >= newTimeStamp;
#endif
}

/**
 * Writes the content of the document to the specified file.
 * @param fileName the file name
 * @param params the options
 * @return true if succeeded
 * @throw IOException any I/O error occured
 */
bool TextFileDocumentInput::write(const String& fileName, const TextFileDocumentInput::WriteParameters& params) {
	// check Unicode spcific newlines
	if(params.newline == NLF_NEXT_LINE || params.newline == NLF_LINE_SEPARATOR || params.newline == NLF_PARAGRAPH_SEPARATOR) {
		const auto_ptr<Encoder> encoder(Encoder::forName(params.encoding));
		if(encoder.get() == 0)
			throw IOException(IOException::INVALID_ENCODING);
		const MIBenum mib = encoder->properties().mibEnum();
		if(mib != fundamental::UTF_8
				&& mib != fundamental::UTF_16LE && mib != fundamental::UTF_16BE && fundamental::UTF_16
#ifndef ASCENSION_NO_STANDARD_ENCODINGS
				&& mib != standard::UTF_7 && mib != standard::UTF_32 && mib != standard::UTF_32LE && mib != standard::UTF_32BE
#endif // !ASCENSION_NO_STANDARD_ENCODINGS
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
				&& encoder->properties().name() == "UTF-5"
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
			)
			throw IOException(IOException::INVALID_NEWLINE);
	}

	// check if writable
#ifdef ASCENSION_WINDOWS
	const DWORD originalAttributes = ::GetFileAttributesW(fileName.c_str());
	if(originalAttributes != INVALID_FILE_ATTRIBUTES && toBoolean(originalAttributes & FILE_ATTRIBUTE_READONLY))
		throw IOException(IOException::UNWRITABLE_FILE);
#else // ASCENSION_POSIX
	struct stat originalStat;
	bool gotStat = ::stat(fileName_.c_str(), &originalStat) == 0;
#if 1
	if(::euidaccess(fileName_.c_str(), 2) < 0)
#else
	if(::access(fileName_.c_str(), 2) < 0)
#endif
		throw IOException(IOException::UNWRITABLE_FILE);
#endif

	// check if the existing file was modified by others
	if(timeStampDirector_ != 0) {
		Time realTimeStamp;
		if(!verifyTimeStamp(true, realTimeStamp)) {
			if(!timeStampDirector_->queryAboutUnexpectedDocumentFileTimeStamp(
					document_, IUnexpectedFileTimeStampDirector::OVERWRITE_FILE))
				return true;
		}
	}
	const String realName(canonicalizePathName(fileName.c_str()));

//	// query progression callback
//	IFileIOProgressListener* progressEvent = (callback != 0) ? callback->queryProgressCallback() : 0;
//	const length_t intervalLineCount = (progressEvent != 0) ? progressEvent->queryIntervalLineCount() : 0;

	// create a temporary file and write into
	const String tempFileName(getTemporaryFileName(realName));
	TextFileStreamBuffer sb(tempFileName, ios_base::out, params.encoding,
		params.encodingSubstitutionPolicy, params.options.has(WriteParameters::WRITE_UNICODE_BYTE_ORDER_SIGNATURE));
	basic_ostream<a::Char> outputStream(&sb);
	try {
		outputStream.exceptions(ios_base::badbit);
		writeDocumentToStream(outputStream, document_, document_.region(), params.newline);
		sb.close();
	} catch(...) {
		// delete the temporary file...
		SystemErrorSaver ses;
#ifdef ASCENSION_WINDOWS
		::DeleteFileW(tempFileName.c_str());
#else // ASCENSION_POSIX
		::remove(tempFileName.c_str());
#endif
		throw;
	}
	unicodeByteOrderMark_ = sb.unicodeByteOrderMark();

	const bool makeBackup = false;

	// copy file attributes (file mode) and delete the old file
	unlock();
#ifdef ASCENSION_WINDOWS
	if(originalAttributes != INVALID_FILE_ATTRIBUTES) {
		::SetFileAttributesW(tempFileName.c_str(), originalAttributes);
		if(makeBackup) {
		} else if(!toBoolean(::DeleteFileW(realName.c_str()))) {
			SystemErrorSaver ses;
			if(::GetLastError() != ERROR_FILE_NOT_FOUND) {
				::DeleteFileW(tempFileName.c_str());
				throw IOException(IOException::PLATFORM_DEPENDENT_ERROR);
			}
		}
	}
	if(!::MoveFileW(tempFileName.c_str(), realName.c_str())) {
		if(originalAttributes != INVALID_FILE_ATTRIBUTES)
			throw IOException(IOException::LOST_DISK_FILE);
		SystemErrorSaver ses;
		::DeleteFileW(tempFileName.c_str());
		throw IOException(IOException::PLATFORM_DEPENDENT_ERROR);
	}
#else // ASCENSION_POSIX
	if(gotStat) {
		::chmod(tempFileName.c_str(), originalStat.st_mode);
		if(makeBackup) {
		} else if(::remove(realName.c_str()) != 0) {
			SystemErrorSaver ses;
			if(errno != ENOENT) {
				::remove(tempFileName.c_str());
				throw IOException(IOException::PLATFORM_DEPENDENT_ERROR);
			}
		}
	}
	if(::rename(tempFileName.c_str(), realName.c_str()) != 0) {
		if(gotStat)
			throw IOException(IOException::LOST_DISK_FILE);
		SystemErrorSaver ses;
		::remove(tempFileName.c_str());
		throw IOException(IOException::PLATFORM_DEPENDENT_ERROR);
	}
#endif

	// TODO: support backup on writing.
/*	if(
#ifdef _DEBUG
	true ||
#endif
	(options.has(SDO_CREATE_BACKUP) && toBoolean(::PathFileExistsW(fileName.c_str())))) {
		WCHAR backupPath[MAX_PATH + 1];
		SHFILEOPSTRUCTW	shfos = {
			0, FO_DELETE, backupPath, 0, FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT, false, 0
		};
		wcscpy(backupPath, filePath.c_str());
		wcscat(backupPath, L".bak");
		backupPath[wcslen(backupPath) + 1] = 0;
		::CopyFileW(filePath.c_str(), backupPath, false);
		::SHFileOperationW(&shfos);
	}
*/

	if(isLiteralNewline(params.newline)) {
		setNewline(params.newline);	// determine the newlines
//		for(length_t i = 0; i < lines_.getSize(); ++i) {
//			Line& line = *lines_[i];
//			line.operationHistory_ = 0;		// erase the operation history
//			line.newline_ = params.newline;	// overwrite the newline
//		}
	} else {
//		for(length_t i = 0; i < lines_.getSize(); ++i)
//			lines_[i]->operationHistory_ = 0;
	}
	savedDocumentRevision_ = document_.revisionNumber();
	document_.markUnmodified();
	document_.setReadOnly(false);
	setEncoding(params.encoding);
	if(fileName_ != realName) {
		fileName_ = realName;
		listeners_.notify<const TextFileDocumentInput&>(&IFilePropertyListener::fileNameChanged, *this);
	}

	// update the internal time stamp
	try {
		getFileLastWriteTime(fileName_, internalLastWriteTime_);
	} catch(IOException&) {
		memset(&internalLastWriteTime_, 0, sizeof(Time));
	}
	userLastWriteTime_ = internalLastWriteTime_;

	return lock();
}

/**
 * Writes the specified region to the specified file.
 * @param fileName the file name
 * @param region the region to be written
 * @param params the options
 * @param append true to append to the file
 * @return the result. @c FIR_OK if succeeded
 * @throw IOException any I/O error occured
 */
bool TextFileDocumentInput::writeRegion(const String& fileName, const Region& region, const TextFileDocumentInput::WriteParameters& params, bool append) {
	// TODO: not implemented.
	return false;
}


#ifndef ASCENSION_NO_GREP

// DirectoryIteratorBase ////////////////////////////////////////////////////////

/// Protected default constructor.
DirectoryIteratorBase::DirectoryIteratorBase() /*throw()*/ {
}

/// Destructor.
DirectoryIteratorBase::~DirectoryIteratorBase() /*throw()*/ {
}


// DirectoryIterator ////////////////////////////////////////////////////////

namespace {
	inline bool isDotOrDotDot(const String& s) {
		return !s.empty() && s[0] == '.' && (s.length() == 1 || (s.length() == 2 && s[1] == '.'));
	}
} // namespace @0

/**
 * Constructor.
 * @param directoryName the directory to traverse
 * @throw NullPointerException @a directoryName is @c null
 * @throw IOException can be @c IOException#FILE_NOT_FOUND or @c IOException#PLATFORM_DEPENDENT_ERROR
 */
DirectoryIterator::DirectoryIterator(const Char* directoryName) :
#ifdef ASCENSION_WINDOWS
		handle_(INVALID_HANDLE_VALUE),
#else // ASCENSION_POSIX
		handle_(0),
#endif
		done_(false) {
	if(directoryName == 0)
		throw a::NullPointerException("directoryName");
	else if(directoryName[0] == 0)
		throw IOException(IOException::FILE_NOT_FOUND);

#ifdef ASCENSION_WINDOWS
	if(!pathExists(directoryName))
		throw IOException(IOException::FILE_NOT_FOUND);
	const size_t len = wcslen(directoryName);
	assert(len > 0);
	manah::AutoBuffer<Char> pattern(new Char[len + 3]);
	wmemcpy(pattern.get(), directoryName, len);
	wcscpy(pattern.get() + len, isPathSeparator(pattern[len - 1]) ? L"*" : L"\\*");
	WIN32_FIND_DATAW data;
	handle_ = ::FindFirstFileW(pattern.get(), &data);
	if(handle_ == INVALID_HANDLE_VALUE)
		throw IOException((::GetLastError() == ERROR_FILE_NOT_FOUND) ?
			IOException::FILE_NOT_FOUND : IOException::PLATFORM_DEPENDENT_ERROR);
	update(&data);
	directory_.assign(pattern.get(), isPathSeparator(pattern[len - 1]) ? len - 1 : len);
#else // ASCENSION_POSIX
	handle_ = ::opendir(directoryName);
	if(handle_ == 0)
		throw IOException((errno == ENOENT) ? IOException::FILE_NOT_FOUND : IOException::PLATFORM_DEPENDENT_ERROR);
	update(0);
	directory_.assign(pattern.get());
	if(isPathSeparator(directory_[directory_.length() - 1]))
		directory_.resize(directory_.length() - 1);
#endif

	if(!done_ && currentIsDirectory_ && isDotOrDotDot(current_))
		next();
}

/// Destructor.
DirectoryIterator::~DirectoryIterator() /*throw()*/ {
#ifdef ASCENSION_WINDOWS
	if(handle_ != INVALID_HANDLE_VALUE)
		::FindClose(handle_);
#else // ASCENSION_POSIX
	if(handle_ != 0)
		::closedir(handle_);
#endif
}

/// @see DirectoryIteratorBase#current
const String& DirectoryIterator::current() const {
	if(done_)
		throw NoSuchElementException();
	return current_;
}

/// @see DirectoryIteratorBase#directory
const String& DirectoryIterator::directory() const {
	return directory_;
}

/// @see DirectoryIteratorBase#isDirectory
bool DirectoryIterator::isDirectory() const {
	if(done_)
		throw NoSuchElementException();
	return currentIsDirectory_;
}

/// @see DirectoryIteratorBase#isDone
bool DirectoryIterator::isDone() const /*throw()*/ {
	return done_;
}

/// @see DirectoryIteratorBase#next
void DirectoryIterator::next() {
	if(!done_) {
#ifdef ASCENSION_WINDOWS
		WIN32_FIND_DATAW data;
		if(::FindNextFileW(handle_, &data) == 0) {
			if(::GetLastError() == ERROR_NO_MORE_FILES)
				done_ = true;
			else
				throw IOException(IOException::PLATFORM_DEPENDENT_ERROR);
		} else
			update(&data);
#else // ASCENSION_POSIX
		update(0);
#endif
	}
	if(!done_ && currentIsDirectory_ && isDotOrDotDot(current_))
		next();
}

void DirectoryIterator::update(const void* info) {
#ifdef ASCENSION_WINDOWS
	const WIN32_FIND_DATAW& data = *static_cast<const WIN32_FIND_DATAW*>(info);
	current_ = data.cFileName;
	currentIsDirectory_ = toBoolean(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
#else // ASCENSION_POSIX
	if(dirent* entry = ::readdir(handle_)) {
		current_ = entry->d_name;
		currentIsDirectory_ = entry->d_type == DT_DIR;
	} else
		done_ = true;
#endif
}


// RecursiveDirectoryIterator ///////////////////////////////////////////////

/**
 * Constructor.
 * @param directoryName the directory to traverse
 * @throw NullPointerException @a directoryName is @c null
 * @throw IOException can be @c IOException#FILE_NOT_FOUND or @c IOException#PLATFORM_DEPENDENT_ERROR
 */
RecursiveDirectoryIterator::RecursiveDirectoryIterator(const Char* directoryName) : doesntPushNext_(false) {
	stack_.push(new DirectoryIterator(directoryName));
}

/// Destructor.
RecursiveDirectoryIterator::~RecursiveDirectoryIterator() /*throw()*/ {
	while(!stack_.empty()) {
		delete stack_.top();
		stack_.pop();
	}
}

/// @see DirectoryIteratorBase#current
const String& RecursiveDirectoryIterator::current() const {
	if(isDone())
		throw NoSuchElementException();
	return stack_.top()->current();
}

/// @see DirectoryIteratorBase#directory
const String& RecursiveDirectoryIterator::directory() const /*throw()*/ {
	return stack_.top()->directory();
}

/**
 * @throw NoSuchElementException
 */
void RecursiveDirectoryIterator::dontPush() {
	if(isDone())
		throw NoSuchElementException();
	doesntPushNext_ = true;
}

/// @see DirectoryIteratorBase#isDirectory
bool RecursiveDirectoryIterator::isDirectory() const {
	if(isDone())
		throw NoSuchElementException();
	return stack_.top()->isDirectory();
}

/// @see DirectoryIteratorBase#isDone
bool RecursiveDirectoryIterator::isDone() const /*throw()*/ {
	return stack_.size() == 1 && stack_.top()->isDone();
}

/// Returns the depth of the recursion.
size_t RecursiveDirectoryIterator::level() const /*throw()*/ {
	return stack_.size() - 1;
}

/// @see DirectoryIteratorBase#next
void RecursiveDirectoryIterator::next() {
	if(isDone())
		throw NoSuchElementException();
	if(doesntPushNext_)
		doesntPushNext_ = false;
	else if(stack_.top()->isDirectory()) {
		String subdir(directory());
		subdir += PATH_SEPARATORS[0];
		subdir += current();
		auto_ptr<DirectoryIterator> sub(new DirectoryIterator(subdir.c_str()));
		if(!sub->isDone()) {
			stack_.push(sub.release());
			return;
		}
	}
	stack_.top()->next();
	while(stack_.top()->isDone() && stack_.size() > 1) {
		delete stack_.top();
		stack_.pop();
		assert(!stack_.top()->isDone());
		stack_.top()->next();
	}
}

/// 
void RecursiveDirectoryIterator::pop() {
	while(!stack_.empty()) {
		stack_.top()->next();
		if(!stack_.top()->isDone())
			break;
		delete stack_.top();
		stack_.pop();
	}
}

#endif // !ASCENSION_NO_GREP
