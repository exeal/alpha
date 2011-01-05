/**
 * @file fileio.cpp
 * Implements @c ascension#kernel#fileio namespace.
 * @note Currently, this implementation does not support OpenVMS.
 * @author exeal
 * @date 2007 (separated from document.cpp)
 * @date 2007-2010
 */

#include <ascension/config.hpp>	// ASCENSION_NO_STANDARD_ENCODINGS
#include <ascension/kernel/fileio.hpp>
#include <limits>	// std.numeric_limits
#ifdef ASCENSION_POSIX
#	include <errno.h>		// errno
#	include <fcntl.h>		// fcntl
#	include <unistd.h>		// fcntl
#	include <sys/mman.h>	// mmap, munmap, ...
#endif // !ASCENSION_POSIX

using namespace ascension;
using namespace ascension::kernel;
using namespace ascension::kernel::fileio;
using namespace ascension::encoding;
using namespace std;


// free function ////////////////////////////////////////////////////////////

namespace {
#ifdef ASCENSION_WINDOWS
	static const PathCharacter PATH_SEPARATORS[] = L"\\/";
#else // ASCENSION_POSIX
	static const PathCharacter PATH_SEPARATORS[] = "/";
#endif
	static const PathCharacter PREFERRED_PATH_SEPARATOR = PATH_SEPARATORS[0];
	/// Returns @c true if the given character is a path separator.
	inline bool isPathSeparator(PathCharacter c) /*throw()*/ {
		return find(PATH_SEPARATORS, ASCENSION_ENDOF(PATH_SEPARATORS) - 1, c) != ASCENSION_ENDOF(PATH_SEPARATORS) - 1;}
	/**
	 * Returns @c true if the specified file or directory exists.
	 * @param name the name of the file
	 * @return @c true if the file exists
	 * @throw NullPointerException @a fileName is @c null
	 * @throw IOException any I/O error occurred
	 */
	bool pathExists(const PathCharacter* name) {
		if(name == 0)
			throw NullPointerException("name");
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
#endif // PathFileExists
#else // ASCENSION_POSIX
		struct stat s;
		if(::stat(name, &s) == 0)
			return true;
		else if(errno == ENOENT)
			return false;
#endif
		throw IOException(name);
	}

	/// Finds the base name in the given file path name.
	inline PathString::const_iterator findFileName(const PathString& s) {
		const PathString::size_type i = s.find_last_of(PATH_SEPARATORS);
		return s.begin() + ((i != PathString::npos) ? i + 1 : 0);
	}

	/**
	 * Returns the last write time of the specified file.
	 * @param fileName the name of the file
	 * @param[out] timeStamp the time
	 * @throw IOException any I/O error occurred
	 */
	void getFileLastWriteTime(const PathString& fileName, TextFileDocumentInput::Time& timeStamp) {
#ifdef ASCENSION_WINDOWS
		WIN32_FILE_ATTRIBUTE_DATA attributes;
		if(::GetFileAttributesExW(fileName.c_str(), GetFileExInfoStandard, &attributes) != 0)
			timeStamp = attributes.ftLastWriteTime;
#else // ASCENSION_POSIX
		struct stat s;
		if(::stat(fileName.c_str(), &s) == 0)
			timeStamp = s.st_mtime;
#endif
		else
			throw IOException(fileName);
	}

	/**
	 * Returns the size of the specified file.
	 * @param fileName the name of the file
	 * @return the size of the file in bytes or -1 if the file is too large
	 * @throw NullPointerException @a fileName is @c null
	 * @throw IOException any I/O error occurred
	 */
	ptrdiff_t getFileSize(const PathCharacter* fileName) {
		if(fileName == 0)
			throw NullPointerException("fileName");
#ifdef ASCENSION_WINDOWS
		WIN32_FILE_ATTRIBUTE_DATA attributes;
		if(::GetFileAttributesExW(fileName, GetFileExInfoStandard, &attributes) != 0)
			return (attributes.nFileSizeHigh == 0
				&& attributes.nFileSizeLow <= static_cast<DWORD>(numeric_limits<ptrdiff_t>::max())) ?
					static_cast<ptrdiff_t>(attributes.nFileSizeLow) : -1;
#else // ASCENSION_POSIX
		struct stat s;
		if(::stat(fileName, &s) == 0)
			return s.st_size;
#endif
		else
			throw IOException(fileName);
	}

	/**
	 * Creates a name for a temporary file.
	 * @param seed the string contains a directory path and a prefix string
	 * @return the result string
	 * @throw std#bad_alloc POSIX @c tempnam failed (only when @c ASCENSION_POSIX was defined)
	 * @throw IOException any I/O error occurred
	 */
	PathString makeTemporaryFileName(const PathString& seed) {
		AutoBuffer<PathCharacter> s(new PathCharacter[seed.length() + 1]);
		copy(seed.begin(), seed.end(), s.get());
		s[seed.length()] = 0;
		PathCharacter* const name = s.get() + (findFileName(seed) - seed.begin());
		if(name != s.get())
			name[-1] = 0;
#ifdef ASCENSION_WINDOWS
		WCHAR result[MAX_PATH];
		if(::GetTempFileNameW(s.get(), name, 0, result) != 0)
			return result;
#else // ASCENSION_POSIX
		if(char* p = ::tempnam(s.get(), name)) {
			PathString result(p);
			::free(p);
			return result;
		} else if(errno == ENOMEM)
			throw bad_alloc("tempnam failed.");
#endif
		throw IOException(PathString());
	}

	/**
	 * Returns @c true if the specified file is special.
	 * @param fileName the file name
	 * @throw IOException any I/O error occurred
	 */
	bool isSpecialFile(const PathString& fileName) {
#ifdef ASCENSION_WINDOWS
		HANDLE file = ::CreateFileW(fileName.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
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
#else // ASCENSION_POSIX
		struct stat s;
		if(::stat(fileName.c_str(), &s) == 0)
			return !S_ISREG(s.st_mode);
#endif
		else
			throw IOException(fileName);
	}

	/**
	 * Verifies if the newline is allowed in the given character encoding.
	 * @param encoding the character encoding
	 * @param newline the newline to verify
	 * @throw encoding#UnsupportedEncodingException @a encoding is not supported
	 * @throw UnknownValueExcepion @a newline is undefined value
	 * @throw std#invalid_argument @a newline is not allowed or not a literal value
	 */
	void verifyNewline(const string& encoding, Newline newline) {
		if(newline == NLF_NEXT_LINE || newline == NLF_LINE_SEPARATOR || newline == NLF_PARAGRAPH_SEPARATOR) {
			auto_ptr<Encoder> encoder(Encoder::forName(encoding));
			if(encoder.get() == 0)
				throw UnsupportedEncodingException("the specified encoding is not supported.");
			else if(!encoder->canEncode(newlineString(newline)[0]))
				throw invalid_argument("the specified newline is not allowed in the specified character encoding.");
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
				throw invalid_argument("the specified newline is not allowed in the specified character encoding.");
#endif
		}
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
PathString fileio::canonicalizePathName(const PathCharacter* pathName) {
	if(pathName == 0)
		throw NullPointerException("pathName");

#ifdef ASCENSION_WINDOWS

	if(wcslen(pathName) >= MAX_PATH)	// too long name
		return pathName;

	// resolve relative path name
	WCHAR path[MAX_PATH];
	WCHAR* dummy;
	if(::GetFullPathNameW(pathName, ASCENSION_COUNTOF(path), path, &dummy) == 0)
		wcscpy(path, pathName);

	// get real component names (from Ftruename implementation in xyzzy)
	PathString result;
	result.reserve(MAX_PATH);
	const PathCharacter* p = path;
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
		if(PathCharacter* next = wcspbrk(const_cast<PathCharacter*>(p), PATH_SEPARATORS)) {
			const PathCharacter c = *next;
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

	PathCharacter resolved[PATH_MAX];
	return (::realpath(pathName, resolved) != 0) ? resolved : pathName;

#endif
}

/**
 * Returns @c true if the specified two file path names are equivalent.
 * @param s1 the first path name
 * @param s2 the second path name
 * @return @c true if @a s1 and @a s2 are equivalent
 * @throw NullPointerException either file name is @c null
 * @see canonicalizePathName
 */
bool fileio::comparePathNames(const PathCharacter* s1, const PathCharacter* s2) {
	if(s1 == 0 || s2 == 0)
		throw NullPointerException("either file name is null.");

#ifdef ASCENSION_WINDOWS
#ifdef PathMatchSpec
	if(toBoolean(::PathMatchSpecW(s1, s2)))
		return true;
#endif // PathMatchSpec
	// by lexicographical comparison
	const int c1 = static_cast<int>(wcslen(s1)) + 1, c2 = static_cast<int>(wcslen(s2)) + 1;
	const int fc1 = ::LCMapStringW(LOCALE_NEUTRAL, LCMAP_LOWERCASE, s1, c1, 0, 0);
	const int fc2 = ::LCMapStringW(LOCALE_NEUTRAL, LCMAP_LOWERCASE, s2, c2, 0, 0);
	if(fc1 != 0 && fc2 != 0 && fc1 == fc2) {
		AutoBuffer<WCHAR> fs1(new WCHAR[fc1]), fs2(new WCHAR[fc2]);
		::LCMapStringW(LOCALE_NEUTRAL, LCMAP_LOWERCASE, s1, c1, fs1.get(), fc1);
		::LCMapStringW(LOCALE_NEUTRAL, LCMAP_LOWERCASE, s2, c2, fs2.get(), fc2);
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

/**
 * Inserts the contents of the file into the specified position.
 * @param document the document
 * @param at the position into which the contents is inserted
 * @param fileName the file name
 * @param encoding the character encoding of the input file or auto detection name
 * @param encodingSubstitutionPolicy the substitution policy used in encoding conversion
 * @param[out] endOfInsertedString the position of the end of the inserted text. can be @c null if
 *                                 not needed
 * @return a pair consists of the encoding used to convert and the boolean value means if the input
 *         contained Unicode byte order mark
 * @throw UnmappableCharacterException
 * @throw MalformedInputException
 * @throw ... any exceptions @c TextFileStreamBuffer#TextFileStreamBuffer and @c kernel#insert throw
 */
pair<string, bool> fileio::insertFileContents(Document& document, const Position& at,
		const PathString& fileName, const string& encoding, Encoder::SubstitutionPolicy encodingSubstitutionPolicy, Position* endOfInsertedString /* = 0 */) {
	TextFileStreamBuffer sb(fileName, ios_base::in, encoding, encodingSubstitutionPolicy, false);
	basic_istream<Char> in(&sb);
	in.exceptions(ios_base::badbit);
	insert(document, at, in, endOfInsertedString);
	const pair<string, bool> result(make_pair(sb.encoding(), sb.unicodeByteOrderMark()));
	sb.close();
	return result;
}

/**
 * Writes the specified region of the document into the specified file.
 * @param document the document
 * @param region the region to write
 * @param fileName the file name
 * @param format the encoding and the newline
 * @param append set @c true to append to the file. if this is @c true, this function does not
 *               write a unicode order mark regardless of the value of
 *               @a format.unicodeByteOrderMark. see constructor of TextFileStreamBuffer
 * @throw UnsupportedEncodingException the character encoding specified by @a format.encoding is
 *                                     not supported
 * @throw std#invalid_argument @a format.newline is not supported or not allowed in the specified
 *                             character encoding
 * @throw ... any I/O error occurred
 */
void fileio::writeRegion(const Document& document, const Region& region,
		const PathString& fileName, const WritingFormat& format, bool append /* = false */) {
	// verify encoding-specific newline
	verifyNewline(format.encoding, format.newline);

	// check if not special file
	if(isSpecialFile(fileName))
#ifdef ASCENSION_WINDOWS
		throw IOException(fileName, ERROR_BAD_FILE_TYPE);
#else // ASCENSION_POSIX
		throw IOException(fileName, ENXIO);
#endif

	// check if writable
#ifdef ASCENSION_WINDOWS
	const DWORD originalAttributes = ::GetFileAttributesW(fileName.c_str());
	if(originalAttributes != INVALID_FILE_ATTRIBUTES && win32::boole(originalAttributes & FILE_ATTRIBUTE_READONLY))
		throw IOException(fileName, ERROR_ACCESS_DENIED);
#else // ASCENSION_POSIX
	struct stat originalStat;
	bool gotStat = ::stat(fileName_.c_str(), &originalStat) == 0;
#if 1
	if(::euidaccess(fileName_.c_str(), 2) < 0)
#else
	if(::access(fileName_.c_str(), 2) < 0)
#endif
		throw IOException(fileName, EACCES);	// EROFS is an alternative
#endif

	// open file to write
	TextFileStreamBuffer sb(fileName,
		append ? (ios_base::out | ios_base::app) : ios_base::out,
		format.encoding, format.encodingSubstitutionPolicy, format.unicodeByteOrderMark);
	try {
		basic_ostream<Char> out(&sb);
		out.exceptions(ios_base::badbit);
		// write into file
		writeDocumentToStream(out, document, region, format.newline);
	} catch(...) {
		sb.closeAndDiscard();
		throw;
	}
	sb.close();
}


// exception classes ////////////////////////////////////////////////////////

namespace {
	IOException::Code currentSystemError() /*throw()*/ {
#ifdef ASCENSION_WINDOWS
		return ::GetLastError();
#else // ASCENSION_POSIX
		return errno;
#endif
	}
	string errorMessage(IOException::Code code = currentSystemError()) {
#ifdef ASCENSION_WINDOWS
		void* buffer;
		if(0 == ::FormatMessageA(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				0, code, LANG_USER_DEFAULT, reinterpret_cast<char*>(&buffer), 0, 0))
			return "";
		const string result(static_cast<char*>(buffer));
		::LocalFree(buffer);
		return result;
#else // ASCENSION_POSIX
		const char* const s = ::strerror(code);
		return (s != 0) ? s : "";
#endif
	}
}

/**
 * Constructor.
 */
IOException::IOException(const PathString& fileName) : ios_base::failure(errorMessage()), fileName_(fileName), code_(currentSystemError()) {
}

/**
 * Constructor.
 */
IOException::IOException(const PathString& fileName, Code code) : ios_base::failure(errorMessage()), fileName_(fileName), code_(code) {
}

/// Returns the platform-dependent error code.
IOException::Code IOException::code() const /*throw()*/ {
	return code_;
}

/// Returns the file name.
const PathString& IOException::fileName() const /*throw()*/ {
	return fileName_;
}

bool IOException::isFileNotFound(const IOException& e) /*throw()*/ {
#ifdef ASCENSION_WINDOWS
	return e.code() == ERROR_FILE_NOT_FOUND || e.code() == ERROR_PATH_NOT_FOUND
		|| e.code() == ERROR_BAD_NETPATH /*|| e.code() == ERROR_INVALID_PARAMETER*/ || e.code() == ERROR_INVALID_NAME;

#else // ASCENSION_POSIX
	return e.code() == ENOENT || e.code() == ENOTDIR;
#endif
}

bool IOException::isPermissionDenied(const IOException& e) /*throw()*/ {
#ifdef ASCENSION_WINDOWS
	return e.code() == ERROR_ACCESS_DENIED || e.code() == ERROR_SHARING_VIOLATION;
#else // ASCENSION_POSIX
	return e.code() == EACCES;
#endif
}

/// Default constructor.
UnmappableCharacterException::UnmappableCharacterException() : ios_base::failure("encountered an unmappable character in encoding/decoding.") {
}

/// Default constructor.
MalformedInputException::MalformedInputException() : ios_base::failure("detected malformed input in encoding/decoding.") {
}


// TextFileStreamBuffer /////////////////////////////////////////////////////

namespace {
	class SystemErrorSaver {
	public:
		SystemErrorSaver() /*throw()*/ : code_(currentSystemError()) {}
#ifdef ASCENSION_WINDOWS
		~SystemErrorSaver() /*throw()*/ {::SetLastError(code_);}
#else // ASCENSION_POSIX
		~SystemErrorSaver() /*throw()*/ {errno = e_;}
#endif
		IOException::Code code() const /*throw()*/ {return code_;}
	private:
		IOException::Code code_;
	};
} // namespace @0

/**
 * Constructor opens the specified file.
 * @param fileName the name of the file
 * @param mode the file open mode. valid values are the following:
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
 * @param encoding the file encoding or auto detection name
 * @param encodingSubstitutionPolicy the substitution policy used in encoding conversion
 * @param writeUnicodeByteOrderMark set @c true to write Unicode byte order mark into the file.
 *                                  this parameter is ignored if @a mode contained
 *                                  @c std#ios_base#app and the output file was existing
 * @throw UnknownValueException @a mode or @a encodingSubstitutionPolicy is invalid
 * @throw UnsupportedEncodingException the encoding specified by @a encoding is not supported
 * @throw PlatformDependentIOError
 */
TextFileStreamBuffer::TextFileStreamBuffer(const PathString& fileName, ios_base::openmode mode,
		const string& encoding, Encoder::SubstitutionPolicy encodingSubstitutionPolicy,
		bool writeUnicodeByteOrderMark) : fileName_(fileName), mode_(mode) {
	inputMapping_.first = inputMapping_.last = inputMapping_.current = 0;
	switch(mode) {
	case ios_base::in:
		openForReading(encoding);
		break;
	case ios_base::out:
	case ios_base::out | ios_base::trunc:
	case ios_base::out | ios_base::app:
		openForWriting(encoding, writeUnicodeByteOrderMark);
		break;
	default:
		throw UnknownValueException("mode");
	}
	assert(encoder_.get() != 0);
	encoder_->setSubstitutionPolicy(encodingSubstitutionPolicy);
}

/// Destructor closes the file.
TextFileStreamBuffer::~TextFileStreamBuffer() {
	try {
		close();	// this may throw
	} catch(const IOException&) {
	}
}

void TextFileStreamBuffer::buildEncoder(const string& encoding, bool detectEncoding) {
	assert(encoder_.get() == 0);
	encoder_ = Encoder::forName(encoding);
	if(encoder_.get() != 0)
		return;
	else if(detectEncoding) {
		if(EncodingDetector* detector = EncodingDetector::forName(encoding)) {
			const pair<MIBenum, string> detected(detector->detect(
				inputMapping_.first, min(inputMapping_.last, inputMapping_.first + 1024 * 10), 0));
			if(detected.first != MIB_OTHER)
				encoder_ = Encoder::forMIB(detected.first);
			else
				encoder_ = Encoder::forName(detected.second);
			if(encoder_.get() != 0)
				return;	// resolved
		}
	}
	throw UnsupportedEncodingException(encoding);
}

void TextFileStreamBuffer::buildInputMapping() {
	assert(isOpen());
#ifdef ASCENSION_WINDOWS
	LARGE_INTEGER fileSize;
	if(!win32::boole(::GetFileSizeEx(fileHandle_, &fileSize)))
		throw IOException(fileName());
	if(fileSize.QuadPart != 0) {
		fileMapping_ = ::CreateFileMappingW(fileHandle_, 0, PAGE_READONLY, 0, 0, 0);
		if(fileMapping_ == 0)
			throw IOException(fileName());
		inputMapping_.first = static_cast<const byte*>(::MapViewOfFile(fileMapping_, FILE_MAP_READ, 0, 0, 0));
		if(inputMapping_.first == 0) {
			SystemErrorSaver ses;
			::CloseHandle(fileMapping_);
			throw IOException(fileName(), ses.code());
		}
	} else
		fileMapping_ = 0;
	inputMapping_.last = inputMapping_.first + fileSize.QuadPart;
#else // ASCENSION_POSIX
	inputMapping_.first = static_cast<const byte*>(::mmap(0, fileSize, PROT_READ, MAP_PRIVATE, fileDescriptor_, 0));
	if(inputMapping_.first == MAP_FAILED)
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
	inputMapping_.last = first + fileSize;
#endif
	inputMapping_.current = inputMapping_.first;
}

/**
 * Closes the file.
 * @return this or @c null if the file is not open
 * @throw ... any exceptions @c #sync throws
 */
TextFileStreamBuffer* TextFileStreamBuffer::close() {
	sync();
	return closeFile();
}

/**
 * Closes the file and discard the change.
 * @return this or @c null if the file is not open
 * @throw ... any exceptions @c #close throws when @c #mode returned @c std#ios_base#in
 */
TextFileStreamBuffer* TextFileStreamBuffer::closeAndDiscard() {
	if(mode() == ios_base::in)
		return close();
	else if((mode() & ~ios_base::trunc) == ios_base::out) {
		if(TextFileStreamBuffer* const self = closeFile()) {
			::DeleteFileW(fileName_.c_str());
			return self;
		} else
			return 0;
	} else if(mode() == (ios_base::out | ios_base::app)) {
#ifdef ASCENSION_WINDOWS
		::SetFilePointerEx(fileHandle_, originalFileEnd_, 0, FILE_BEGIN);
		::SetEndOfFile(fileHandle_);
#else // ASCENSION_POSIX
		::lseek(fileDescriptor_, originalFileEnd_, SEEK_SET);
		::ftruncate(fileDescriptor_, originalFileEnd_);
#endif
		return closeFile();
	} else
		ASCENSION_ASSERT_NOT_REACHED();
}

TextFileStreamBuffer* TextFileStreamBuffer::closeFile() /*throw()*/ {
#ifdef ASCENSION_WINDOWS
	if(fileMapping_ != 0) {
		::UnmapViewOfFile(const_cast<byte*>(inputMapping_.first));
		::CloseHandle(fileMapping_);
		inputMapping_.first = 0;
		fileMapping_ = 0;
	}
	if(fileHandle_ != INVALID_HANDLE_VALUE) {
		::CloseHandle(fileHandle_);
		fileHandle_ = INVALID_HANDLE_VALUE;
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
	if(encoder_.get() != 0) {
		encoder_->resetEncodingState();
		encoder_->resetDecodingState();
	}
	return 0;	// didn't close the file actually
}

/**
 * Returns the character encoding. If the encoding name passed to the constructor was detection
 * name, returns the detected encoding.
 */
string TextFileStreamBuffer::encoding() const /*throw()*/ {
	return encoder_->properties().name();
}

/// Returns @c true if the file is open.
bool TextFileStreamBuffer::isOpen() const /*throw()*/ {
#ifdef ASCENSION_WINDOWS
	return fileHandle_ != INVALID_HANDLE_VALUE;
#else // ASCENSION_POSIX
	return fileDescriptor_ != -1;
#endif
}

// called by only the constructor
void TextFileStreamBuffer::openForReading(const string& encoding) {
	// open the file
#ifdef ASCENSION_WINDOWS
	fileHandle_ = ::CreateFileW(fileName().c_str(), GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
	if(fileHandle_ == INVALID_HANDLE_VALUE)
#else // ASCENSION_POSIX
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
void TextFileStreamBuffer::openForWriting(const string& encoding, bool writeUnicodeByteOrderMark) {
	if((mode_ & ios_base::app) != 0) {
#ifdef ASCENSION_WINDOWS
		fileHandle_ = ::CreateFileW(fileName_.c_str(), GENERIC_READ | GENERIC_WRITE,
			0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE | FILE_FLAG_SEQUENTIAL_SCAN, 0);
		if(fileHandle_ == INVALID_HANDLE_VALUE) {
			const DWORD e = ::GetLastError();
			if(e == ERROR_FILE_NOT_FOUND)
				mode_ &= ~ios_base::app;
			else
				throw IOException(fileName(), e);
		} else {
			originalFileEnd_.QuadPart = 0;
			if(!win32::boole(::SetFilePointerEx(fileHandle_, originalFileEnd_, &originalFileEnd_, FILE_END)))
				throw IOException(fileName());
			writeUnicodeByteOrderMark = false;
		}
#else // ASCENSION_POSIX
		fileDescriptor_  = ::open(fileName_.c_str(), O_WRONLY | O_APPEND);
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
	if((mode_ & ~ios_base::trunc) == ios_base::out) {
#ifdef ASCENSION_WINDOWS
		fileHandle_ = ::CreateFileW(fileName().c_str(), GENERIC_WRITE, 0, 0,
			CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE | FILE_FLAG_SEQUENTIAL_SCAN, 0);
		if(fileHandle_ == INVALID_HANDLE_VALUE)
#else // ASCENSION_POSIX
		fileDescriptor_ = ::open(fileName().c_str(), O_WRONLY | O_CREAT);
		if(fileDescriptor_ == -1)
#endif
			throw IOException(fileName());
	}

	try {
		if((mode() & ios_base::app) != 0)
			buildInputMapping();
		else
			fileMapping_ = 0;
		buildEncoder(encoding, (mode() & ios_base::app) != 0);
	} catch(...) {
		closeFile();
		throw;
	}

	if(writeUnicodeByteOrderMark)
		encoder_->setFlags(encoder_->flags() | Encoder::UNICODE_BYTE_ORDER_MARK);
	setp(ucsBuffer_, ASCENSION_ENDOF(ucsBuffer_));
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
	if(isOpen() && inputMapping_.first == 0 && pptr() > pbase()) {
		byte* toNext;
		const Char* fromNext;
		byte nativeBuffer[ASCENSION_COUNTOF(ucsBuffer_)];
		encoder_->setFlags(encoder_->flags() | Encoder::BEGINNING_OF_BUFFER | Encoder::END_OF_BUFFER);
		while(true) {
			const Char* const fromEnd = pptr();

			// conversion
			const Encoder::Result encodingResult = encoder_->fromUnicode(
				nativeBuffer, ASCENSION_ENDOF(nativeBuffer), toNext, pbase(), fromEnd, fromNext);
			if(encodingResult == Encoder::UNMAPPABLE_CHARACTER)
				throw UnmappableCharacterException();
			else if(encodingResult == Encoder::MALFORMED_INPUT)
				throw MalformedInputException();

			// write into the file
#ifdef ASCENSION_WINDOWS
			DWORD writtenBytes;
			assert(static_cast<size_t>(toNext - nativeBuffer) <= numeric_limits<DWORD>::max());
			const DWORD bytes = static_cast<DWORD>(toNext - nativeBuffer);
			if(::WriteFile(fileHandle_, nativeBuffer, bytes, &writtenBytes, 0) == 0 || writtenBytes != bytes)
#else // ASCENSION_POSIX
			const size_t bytes = toNext - nativeBuffer;
			const ssize_t writtenBytes = ::write(fileDescriptor_, nativeBuffer, bytes);
			if(writtenBytes == -1 || static_cast<size_t>(writtenBytes) != bytes)
#endif
				throw IOException(fileName());

			setp(ucsBuffer_ + (fromNext - ucsBuffer_), epptr());
			pbump(static_cast<int>(fromEnd - pbase()));	// TODO: this cast may be danger.
			if(encodingResult == Encoder::COMPLETED)
				break;
		}
		setp(ucsBuffer_, ASCENSION_ENDOF(ucsBuffer_));
	}
	return 0;
}

/// @see std#basic_streambuf#underflow
TextFileStreamBuffer::int_type TextFileStreamBuffer::underflow() {
	if(inputMapping_.first == 0 || inputMapping_.current >= inputMapping_.last)
		return traits_type::eof();	// not input mode or reached EOF

	Char* toNext;
	const byte* fromNext;
	encoder_->setFlags(encoder_->flags() | Encoder::BEGINNING_OF_BUFFER | Encoder::END_OF_BUFFER);
	switch(encoder_->toUnicode(ucsBuffer_, ASCENSION_ENDOF(ucsBuffer_), toNext, inputMapping_.current, inputMapping_.last, fromNext)) {
	case Encoder::UNMAPPABLE_CHARACTER:
		throw UnmappableCharacterException();
	case Encoder::MALFORMED_INPUT:
		throw MalformedInputException();
	default:
		break;
	}

	inputMapping_.current = fromNext;
	setg(ucsBuffer_, ucsBuffer_, toNext);
	return (toNext > ucsBuffer_) ? traits_type::to_int_type(*gptr()) : traits_type::eof();
}

/// Returns @c true if the internal encoder has @c Encoder#UNICODE_BYTE_ORDER_MARK flag.
bool TextFileStreamBuffer::unicodeByteOrderMark() const /*throw()*/ {
	return (encoder_->flags() & Encoder::UNICODE_BYTE_ORDER_MARK) != 0;
}


// TextFileDocumentInput.FileLocker /////////////////////////////////////////

class TextFileDocumentInput::FileLocker {
	ASCENSION_NONCOPYABLE_TAG(FileLocker);
public:
	FileLocker() /*throw()*/;
	~FileLocker() /*throw()*/;
	bool hasLock() const /*throw()*/;
	bool lock(const PathString& fileName, bool share);
	LockType type() const /*throw()*/;
	bool unlock() /*throw()*/;
private:
	LockType type_;
#ifdef ASCENSION_WINDOWS
	HANDLE file_;
#else // ASCENSION_POSIX
	int file_;
	bool deleteFileOnClose_;
#endif
	PathString fileName_;
};

/// Default constructor.
TextFileDocumentInput::FileLocker::FileLocker() /*throw()*/ : type_(NO_LOCK), file_(
#ifdef ASCENSION_WINDOWS
		INVALID_HANDLE_VALUE
#else // ASCENSION_POSIX
		-1), deleteFileOnClose_(false
#endif
		) {
}

/// Destructor.
TextFileDocumentInput::FileLocker::~FileLocker() /*throw()*/ {
	unlock();
}

inline bool TextFileDocumentInput::FileLocker::hasLock() const /*throw()*/ {
#ifdef ASCENSION_WINDOWS
	return file_ != INVALID_HANDLE_VALUE;
#else // ASCENSION_POSIX
	return file_ != -1;
#endif
}

/**
 * Locks the file.
 * @param fileName
 * @param share set @c true if shared-lock
 * @retval true if locked successfully or the lock mode is @c DONT_LOCK
 * @retval false the current lock mode was @c SHARED_LOCK and an other existing process had already
 *               locked the file with same lock mode
 * @throw IOException
 */
bool TextFileDocumentInput::FileLocker::lock(const PathString& fileName, bool share) {
	if(fileName.empty())
#ifdef ASCENSION_WINDOWS
		throw IOException(fileName, ERROR_FILE_NOT_FOUND);
#else // ASCENSION_POSIX
		throw IOException(fileName, ENOENT);
#endif
	bool alreadyShared = false;
#ifdef ASCENSION_POSIX
	flock fl;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 0;
#endif // ASCENSION_POSIX

	if(share) {
		// check the file had already been shared-locked
#ifdef ASCENSION_WINDOWS
		HANDLE temp = ::CreateFileW(fileName.c_str(),
			GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if(temp == INVALID_HANDLE_VALUE) {
			if(::GetLastError() == ERROR_SHARING_VIOLATION)
				alreadyShared = true;
		} else
			::CloseHandle(temp);
#else // ASCENSION_POSIX
		int temp = ::open(fileName.c_str(), O_RDONLY);
		if(temp != -1) {
			fl.l_type = F_WRLCK;
			if(::fcntl(temp, F_SETLK, &fl) == -1) {
				if(errno == EACCES || errno == EAGAIN)
					alreadyShared = true;
				fl.l_type = F_UNLCK;
				::fcntl(temp, &fl);
			}
			::close(temp);
		}
#endif
	}

#ifdef ASCENSION_WINDOWS
	HANDLE f = ::CreateFileW(fileName.c_str(), GENERIC_READ,
		share ? FILE_SHARE_READ : 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if(f == INVALID_HANDLE_VALUE) {
		DWORD e = ::GetLastError();
		if(e != ERROR_FILE_NOT_FOUND)
			throw IOException(fileName, e);
		f = ::CreateFileW(fileName.c_str(), GENERIC_READ,
			share ? FILE_SHARE_READ : 0, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, 0);
		if(f == INVALID_HANDLE_VALUE)
			throw IOException(fileName);
	}
#else // ASCENSION_POSIX
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

inline TextFileDocumentInput::LockType TextFileDocumentInput::FileLocker::type() const /*throw()*/ {
	return hasLock() ? type_ : NO_LOCK;
}

/**
 * Unlocks the file.
 * @return succeeded or not
 */
bool TextFileDocumentInput::FileLocker::unlock() /*throw()*/ {
	bool succeeded = true;
	if(hasLock()) {
#ifdef ASCENSION_WINDOWS
		succeeded = win32::boole(::CloseHandle(file_));
		file_ = INVALID_HANDLE_VALUE;
#else // ASCENSION_POSIX
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
		fileLocker_(new FileLocker), document_(document), encoding_(Encoder::defaultInstance().properties().name()),
		unicodeByteOrderMark_(false), newline_(ASCENSION_DEFAULT_NEWLINE), savedDocumentRevision_(0), timeStampDirector_(0) {
	memset(&userLastWriteTime_, 0, sizeof(Time));
	memset(&internalLastWriteTime_, 0, sizeof(Time));
	desiredLockMode_.type = NO_LOCK;
	desiredLockMode_.onlyAsEditing = false;
	document.setProperty(Document::TITLE_PROPERTY, String());
}

/// Destructor.
TextFileDocumentInput::~TextFileDocumentInput() /*throw()*/ {
	unbind();
	delete fileLocker_;
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
 *
 * @param fileName
 */
void TextFileDocumentInput::bind(const PathString& fileName) {
	if(fileName.empty())
		return unbind();

	const PathString realName(canonicalizePathName(fileName.c_str()));
	if(!pathExists(realName.c_str()))
#ifdef ASCENSION_WINDOWS
		throw IOException(fileName, ERROR_FILE_NOT_FOUND);
#else // ASCENSION_POSIX
		throw IOException(fileName, ENOENT);
#endif
	if(fileLocker_->hasLock()) {
		assert(fileLocker_->type() == desiredLockMode_.type);
		if(desiredLockMode_.onlyAsEditing)
			assert(!document().isModified());
//			assert(savedDocumentRevision_ != document().revisionNumber());
		fileLocker_->lock(realName, fileLocker_->type() == SHARED_LOCK);
	}
	document_.setInput(this, false);
	fileName_ = realName;
	listeners_.notify<const TextFileDocumentInput&>(&IFilePropertyListener::fileNameChanged, *this);
	document_.setModified();
}

/**
 * Checks the last modified date/time of the bound file and verifies if the other modified the
 * file. If the file is modified, the listener's
 * @c IUnexpectedFileTimeStampDerector#queryAboutUnexpectedDocumentFileTimeStamp will be called.
 * @return the value which the listener returned or @c true if the listener is not set
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

/// @see IDocumentStateListener#documentAccessibleRegionChanged
void TextFileDocumentInput::documentAccessibleRegionChanged(const Document&) {
}

/// @see IDocumentStateListener#documentModificationSignChanged
void TextFileDocumentInput::documentModificationSignChanged(const Document&) {
	if(isBoundToFile() && desiredLockMode_.onlyAsEditing) {
		if(!document().isModified())
			fileLocker_->unlock();
		else if(desiredLockMode_.type != NO_LOCK)
			fileLocker_->lock(fileName(), desiredLockMode_.type == SHARED_LOCK);
	}
}

/// @see IDocumentStateListener#documentPropertyChanged
void TextFileDocumentInput::documentPropertyChanged(const Document&, const DocumentPropertyKey&) {
}

/// @see IDocumentStateListener#documentAccessibleRegionChanged
void TextFileDocumentInput::documentReadOnlySignChanged(const Document&) {
}

/// @see IDocumentInput#isChangeable
bool TextFileDocumentInput::isChangeable(const Document&) const {
	if(isBoundToFile()) {
		// check the time stamp if this is the first modification
		if(timeStampDirector_ != 0 && !document().isModified()) {
			Time realTimeStamp;
			TextFileDocumentInput& self = const_cast<TextFileDocumentInput&>(*this);
			if(!self.verifyTimeStamp(true, realTimeStamp)) {	// the other overwrote the file
				if(!timeStampDirector_->queryAboutUnexpectedDocumentFileTimeStamp(
						document_, IUnexpectedFileTimeStampDirector::FIRST_MODIFICATION))
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

/// @see IDocumentInput#location
String TextFileDocumentInput::location() const /*throw()*/ {
#ifdef ASCENSION_WINDOWS
	return fileName();
#else // ASCENSION_POSIX
	const codecvt<Char, PathCharacter, mbstate_t>& converter = use_facet<codecvt<Char, PathCharacter, mbstate_t> >(locale());
	Char result[PATH_MAX * 2];
	mbstate_t dummy;
	const PathCharacter* fromNext;
	Char* toNext;
	return (converter.in(dummy, fileName().c_str(),
		fileName().c_str() + fileName().length() + 1, fromNext, result, endof(result), toNext) == codecvt_base::ok) ? result : String();
#endif
}

/**
 * Locks the bound file.
 * @param mode the lock mode.
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
TextFileDocumentInput::LockType TextFileDocumentInput::lockType() const /*throw()*/ {
	return fileLocker_->type();
}

/// @see IDocumentInput#postFirstDocumentChange
void TextFileDocumentInput::postFirstDocumentChange(const Document&) /*throw()*/ {
	if(!document().isModified() && desiredLockMode_.onlyAsEditing)
		fileLocker_->unlock();
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
 * Replaces the document's content with the text of the bound file on disk.
 * @param encoding the file encoding or auto detection name
 * @param encodingSubstitutionPolicy the substitution policy used in encoding conversion
 * @param unexpectedTimeStampDirector
 * @throw IllegalStateException the object was not bound to a file
 * @throw IOException any I/O error occurred. in this case, the document's content will be lost
 * @throw ... any exceptions @c insertFileContents throws
 */
void TextFileDocumentInput::revert(const string& encoding,
		Encoder::SubstitutionPolicy encodingSubstitutionPolicy, IUnexpectedFileTimeStampDirector* unexpectedTimeStampDirector /* = 0 */) {
	if(!isBoundToFile())
		throw IllegalStateException("the object is not bound to a file.");
	document_.resetContent();
	timeStampDirector_ = 0;

	// read from the file
	pair<string, bool> resultEncoding;
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
#ifdef ASCENSION_WINDOWS
	document_.setProperty(Document::TITLE_PROPERTY, fileName());
#else // ASCENSION_POSIX
	const PathString title(fileName());
	const locale lc("");
	const codecvt<Char, PathCharacter, mbstate_t>& conv = use_facet<codecvt<Char, PathCharacter, mbstate_t> >(lc);
	mbstate_t state;
	const PathCharacter* fromNext;
	Char* ucsNext;
	manah::AutoBuffer<Char> ucs(new Char[title.length() * 2]);
	if(codecvt_base::ok == conv.in(state,
			title.data(), title.data() + title.length(), fromNext, ucs.get(), ucs.get() + title.length() * 2, ucsNext)) {
		*ucsNext = L'0';
		document_.setProperty(Document::TITLE_PROPERTY, ucs.get());
	}
#endif
	encoding_ = resultEncoding.first;
	newline_ = document().getLineInformation(0).newline();	// use the newline of the first line
	listeners_.notify<const TextFileDocumentInput&>(&IFilePropertyListener::fileEncodingChanged, *this);
	listeners_.notify<const TextFileDocumentInput&>(&IFilePropertyListener::fileNameChanged, *this);

	document_.clearUndoBuffer();
	document_.markUnmodified();

	// update the internal time stamp
	try {
		getFileLastWriteTime(fileName(), internalLastWriteTime_);
		userLastWriteTime_ = internalLastWriteTime_;
	} catch(ios_base::failure&) {
		// ignore...
	}
}

/**
 * Sets the character encoding.
 * @param encoding the encoding
 * @return this object
 * @throw encoding#UnsupportedEncodingException @a encoding is not supported
 * @see #encoding
 */
TextFileDocumentInput& TextFileDocumentInput::setEncoding(const string& encoding) {
	if(!encoding.empty() && !Encoder::supports(encoding))
		throw UnsupportedEncodingException("encoding");
	encoding_ = encoding;
	listeners_.notify<const TextFileDocumentInput&>(&IFilePropertyListener::fileEncodingChanged, *this);
	return *this;
}

/**
 * Sets the newline.
 * @param newline the newline
 * @return this object
 * @throw UnknownValueException @a newline is invalid
 */
TextFileDocumentInput& TextFileDocumentInput::setNewline(Newline newline) {
	if(!isLiteralNewline(newline))
		throw UnknownValueException("newline");
	else if(newline != newline_) {
		newline_ = newline;
		listeners_.notify<const TextFileDocumentInput&>(&IFilePropertyListener::fileEncodingChanged, *this);
	}
	return *this;
}

/**
 * Unbinds the file and the document.
 * @note This method does NOT reset the content of the document.
 */
void TextFileDocumentInput::unbind() /*throw()*/ {
	if(isBoundToFile()) {
		fileLocker_->unlock();	// this may return false
		if(document().input() == this)
			document_.setInput(0, false);
		fileName_.erase();
		listeners_.notify<const TextFileDocumentInput&>(&IFilePropertyListener::fileNameChanged, *this);
		setEncoding(Encoder::defaultInstance().properties().name());
		memset(&userLastWriteTime_, 0, sizeof(Time));
		memset(&internalLastWriteTime_, 0, sizeof(Time));
	}
}

void TextFileDocumentInput::unlockFile() {
	fileLocker_->unlock();
}

/**
 * Returns last modified time.
 * @param internal set @c true for @c internalLastWriteTime_, @c false for @c userLastWriteTime_
 * @param[out] newTimeStamp the actual time stamp
 * @return @c false if not match
 */
bool TextFileDocumentInput::verifyTimeStamp(bool internal, Time& newTimeStamp) /*throw()*/ {
	static Time uninitialized;
	static bool initializedUninitialized = false;
	if(!initializedUninitialized)
		memset(&uninitialized, 0, sizeof(Time));

	const Time& about = internal ? internalLastWriteTime_ : userLastWriteTime_;
	if(!isBoundToFile()
			|| memcmp(&about, &uninitialized, sizeof(Time)) == 0
			|| fileLocker_->hasLock())
		return true;	// not managed

	try {
		getFileLastWriteTime(fileName().c_str(), newTimeStamp);
	} catch(IOException&) {
		return true;
	}
#ifdef ASCENSION_WINDOWS
	return ::CompareFileTime(&about, &newTimeStamp) != -1;
#else // ASCENSION_POSIX
	return about >= newTimeStamp;
#endif
}

/*
void backupAtRecycleBin(const PathString& fileName) {
	if(pathExists(fileName.c_str())) {
		WCHAR backupPath[MAX_PATH + 1];
		SHFILEOPSTRUCTW	shfos = {
			0, FO_DELETE, backupPath, 0, FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT, false, 0
		};
		wcscpy(backupPath, fileName.c_str());
		wcscat(backupPath, L".bak");
		backupPath[wcslen(backupPath) + 1] = 0;
		::CopyFileW(filePath.c_str(), backupPath, false);
		::SHFileOperationW(&shfos);
	}
}
*/

/**
 * Writes the content of the document into the bound file.
 * @param format the character encoding and the newlines
 * @param options the other options
 * @throw
 */
void TextFileDocumentInput::write(const WritingFormat& format, const WritingOption* options /* = 0 */) {
	if(!document().isModified())
		return;
	if(!isBoundToFile())
		throw IllegalStateException("no file name.");
	verifyNewline(format.encoding, format.newline);

	// TODO: check if the input had been truncated.

	// check if the disk file had changed
	if(timeStampDirector_ != 0) {
		Time realTimeStamp;
		if(!verifyTimeStamp(true, realTimeStamp)) {
			if(!timeStampDirector_->queryAboutUnexpectedDocumentFileTimeStamp(
					document_, IUnexpectedFileTimeStampDirector::OVERWRITE_FILE))
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
#ifdef ASCENSION_WINDOWS
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
				throw ios_base::failure("lost the disk file.");
			SystemErrorSaver ses;
			::DeleteFileW(tempFileName.c_str());
			throw IOException(fileName(), ses.code());
		}
#else // ASCENSION_POSIX
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
				throw ios_base::failure("lost the disk file.");
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
		memset(&internalLastWriteTime_, 0, sizeof(Time));
	}
	userLastWriteTime_ = internalLastWriteTime_;
}


#ifndef ASCENSION_NO_GREP

// DirectoryIteratorBase ////////////////////////////////////////////////////

/// Protected default constructor.
DirectoryIteratorBase::DirectoryIteratorBase() /*throw()*/ {
}

/// Destructor.
DirectoryIteratorBase::~DirectoryIteratorBase() /*throw()*/ {
}


// DirectoryIterator ////////////////////////////////////////////////////////

namespace {
	inline bool isDotOrDotDot(const PathString& s) {
		return !s.empty() && s[0] == '.' && (s.length() == 1 || (s.length() == 2 && s[1] == '.'));
	}
} // namespace @0

/**
 * Constructor.
 * @param directoryName the directory to traverse
 * @throw NullPointerException @a directoryName is @c null
 * @throw IOException any I/O error occurred
 */
DirectoryIterator::DirectoryIterator(const PathCharacter* directoryName) :
#ifdef ASCENSION_WINDOWS
		handle_(INVALID_HANDLE_VALUE),
#else // ASCENSION_POSIX
		handle_(0),
#endif
		done_(false) {
	if(directoryName == 0)
		throw NullPointerException("directoryName");
	else if(directoryName[0] == 0)
#ifdef ASCENSION_WINDOWS
		throw IOException(directoryName, ERROR_PATH_NOT_FOUND);
#else // ASCENSION_POSIX
		throw IOException(directoryName, ENOENT);
#endif

#ifdef ASCENSION_WINDOWS
	if(!pathExists(directoryName))
		throw IOException(directoryName, ERROR_PATH_NOT_FOUND);
	const size_t len = wcslen(directoryName);
	assert(len > 0);
	AutoBuffer<PathCharacter> pattern(new PathCharacter[len + 3]);
	wmemcpy(pattern.get(), directoryName, len);
	wcscpy(pattern.get() + len, isPathSeparator(pattern[len - 1]) ? L"*" : L"\\*");
	WIN32_FIND_DATAW data;
	handle_ = ::FindFirstFileW(pattern.get(), &data);
	if(handle_ == INVALID_HANDLE_VALUE)
		throw IOException(directoryName);
	update(&data);
	directory_.assign(pattern.get(), isPathSeparator(pattern[len - 1]) ? len - 1 : len);
#else // ASCENSION_POSIX
	handle_ = ::opendir(directoryName);
	if(handle_ == 0)
		throw IOException(directoryName);
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
bool DirectoryIterator::hasNext() const /*throw()*/ {
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
#ifdef ASCENSION_WINDOWS
		WIN32_FIND_DATAW data;
		if(::FindNextFileW(handle_, &data) == 0) {
			if(::GetLastError() == ERROR_NO_MORE_FILES)
				done_ = true;
			else
				throw IOException(directory_);
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
	currentIsDirectory_ = win32::boole(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
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
RecursiveDirectoryIterator::RecursiveDirectoryIterator(const PathCharacter* directoryName) : doesntPushNext_(false) {
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
const PathString& RecursiveDirectoryIterator::current() const {
	if(!hasNext())
		throw NoSuchElementException();
	return stack_.top()->current();
}

/// @see DirectoryIteratorBase#directory
const PathString& RecursiveDirectoryIterator::directory() const /*throw()*/ {
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
bool RecursiveDirectoryIterator::hasNext() const /*throw()*/ {
	return stack_.size() != 1 || stack_.top()->hasNext();
}

/// @see DirectoryIteratorBase#isDirectory
bool RecursiveDirectoryIterator::isDirectory() const {
	if(!hasNext())
		throw NoSuchElementException();
	return stack_.top()->isDirectory();
}

/// Returns the depth of the recursion.
size_t RecursiveDirectoryIterator::level() const /*throw()*/ {
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
		auto_ptr<DirectoryIterator> sub(new DirectoryIterator(subdir.c_str()));
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
