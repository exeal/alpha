/**
 * @file document.cpp
 * @author exeal
 * @date 2003-2006 (was EditDoc.h)
 * @date 2006-2007
 */

#include "document.hpp"
//#include <shlwapi.h>	// PathXxxx
//#include <shlobj.h>	// SHGetDesktopFolder, IShellFolder, ...
//#include <MAPI.h>		// MAPISendMail
#include <algorithm>
#include <limits>	// std.numeric_limits
#ifdef ASCENSION_POSIX
#	include <errno.h>		// errno
#	include <fcntl.h>		// fcntl
#	include <unistd.h>		// fcntl
#	include <sys/mman.h>	// mmap, munmap, ...
#endif /* !ASCENSION_POSIX */

using namespace ascension;
using namespace ascension::kernel;
using namespace ascension::text;
using namespace std;


const Position Position::ZERO_POSITION(0, 0);
const Position Position::INVALID_POSITION(INVALID_INDEX, INVALID_INDEX);


// kernel free functions ////////////////////////////////////////////////////

/**
 * Returns absolute character offset of the specified position from the start of the document.
 * @param document the document
 * @param at the position
 * @param fromAccessibleStart
 * @throw BadPositionException @a at is outside of the document
 */
length_t kernel::getAbsoluteOffset(const Document& document, const Position& at, bool fromAccessibleStart) {
	if(at > document.region().second)
		throw BadPositionException();
	length_t offset = 0;
	const Position start((fromAccessibleStart ? document.accessibleRegion() : document.region()).first);
	for(length_t line = start.line; ; ++line) {
		if(line == at.line) {
			offset += at.column;
			break;
		} else {
			offset += document.lineLength(line) + 1;	// +1 is for a newline character
			if(line == start.line)
				offset -= start.column;
		}
	}
	return offset;
}

/**
 * Returns the number of lines in the specified text string.
 * @param first the start of the text string
 * @param last the end of the text string
 * @return the number of lines
 */
length_t kernel::getNumberOfLines(const Char* first, const Char* last) throw() {
	if(first == last)
		return 0;
	length_t lines = 1;
	while(true) {
		first = find_first_of(first, last, LINE_BREAK_CHARACTERS, endof(LINE_BREAK_CHARACTERS));
		if(first == last)
			break;
		++lines;
		first += (*first == CARRIAGE_RETURN && first < last - 1 && first[1] == LINE_FEED) ? 2 : 1;
	}
	return lines;
}

/**
 * Reads the content of the specified input stream and write into the document.
 * @param in the input stream
 * @param document the document
 * @param at the position to which
 * @param newline the newline written into the document. if this is not @c NLF_AUTO, the method
 * replaces original newline characters with the corresponding ones
 * @return @a in
 * @throw ReadOnlyDocumentException @a document is read only
 * @throw BadPositionException @a at is outside of the document
 */
InputStream& kernel::readDocumentFromStream(InputStream& in, Document& document, const Position& at, Newline newline /* = NLF_AUTO */) {
	if(document.isReadOnly())
		throw ReadOnlyDocumentException();
	if(at > document.region().end())
		throw BadPositionException();

	Position p(at);
	Char buffer[8192];
	do {
		in.read(buffer, countof(buffer));
		p = document.insert(p, buffer, buffer + in.gcount());
	} while(static_cast<size_t>(in.gcount()) < countof(buffer));
	return in;
}

/**
 * Adapts the specified position to the document change.
 * @param position the original position
 * @param change the content of the document change
 * @param gravity the gravity which determines the direction to which the position should move if
 * a text was inserted at the position. if @c FORWARD is specified, the position will move to the
 * start of the inserted text (no movement occur). Otherwise, move to the end of the inserted text
 * @return the result position
 */
Position kernel::updatePosition(const Position& position, const DocumentChange& change, Direction gravity) throw() {
	Position newPosition(position);
	if(!change.isDeletion()) {	// insertion
		if(position < change.region().first)	// behind the current position
			return newPosition;
		else if(position == change.region().first && gravity == BACKWARD) // the current position + backward gravity
			return newPosition;
		else if(position.line > change.region().first.line)	// in front of the current line
			newPosition.line += change.region().second.line - change.region().first.line;
		else {	// in the current line
			newPosition.line += change.region().second.line - change.region().first.line;
			newPosition.column += change.region().second.column - change.region().first.column;
		}
	} else {	// deletion
		if(position < change.region().second) {	// the end is behind the current line
			if(position <= change.region().first)
				return newPosition;
			else	// in the region
				newPosition = change.region().first;
		} else if(position.line > change.region().second.line)	// in front of the current line
			newPosition.line -= change.region().second.line - change.region().first.line;
		else {	// the end is the current line
			if(position.line == change.region().first.line)	// the region is single-line
				newPosition.column -= change.region().second.column - change.region().first.column;
			else {	// the region is multiline
				newPosition.line -= change.region().second.line - change.region().first.line;
				newPosition.column -= change.region().second.column - change.region().first.column;
			}
		}
	}
	return newPosition;
}

/**
 * Writes the content of the document to the specified output stream.
 * <p>This method does not write Unicode byte order mark.</p>
 * @param out the output stream
 * @param document the document
 * @param region the region to be written (this region is not restricted with narrowing)
 * @param newline the newline string. if set to an empty string, actual contents (can obtain by
 * @c Document#Line#newline) are used
 * @return @a out
 * @see getNewlineString, readDocumentFromStream
 */
OutputStream& kernel::writeDocumentToStream(OutputStream& out,
		const Document& document, const Region& region, const String& newline /* = L"" */) {
	const Position& beginning = region.beginning();
	const Position end = min(region.end(), document.region().second);
	if(beginning.line == end.line)	// shortcut for single-line
		out << document.line(end.line).substr(beginning.column, end.column - beginning.column);
	else {
		const bool rawNewline = newline.empty();
		for(length_t i = beginning.line; ; ++i) {
			const Document::Line& line = document.getLineInformation(i);
			const length_t first = (i == beginning.line) ? beginning.column : 0;
			const length_t last = (i == end.line) ? end.column : line.text().length();
			out.write(line.text().data() + first, static_cast<streamsize>(last - first));
			if(i == end.line)
				break;
			if(rawNewline)
				out << getNewlineString(line.newline());
			else
				out.write(newline.data(), static_cast<streamsize>(newline.length()));
		}
	}
	return out;
}


// kernel.files free functions //////////////////////////////////////////////

namespace {
#ifdef ASCENSION_WINDOWS
	static const files::Char PATH_SEPARATORS[] = L"/\\";
#else // ASCENSION_POSIX
	static const files::Char PATH_SEPARATORS[] = "/";
#endif
	/**
	 * Returns true if the specified file exists.
	 * @param fileName the name of the file
	 * @return if the file exists
	 * @throw NullPointerException @a fileName is @c null
	 * @throw files#IOException(files#IOException#PLATFORM_DEPENDENT_ERROR) any I/O error occured.
	 * for details, use POSIX @c errno or Win32 @c GetLastError
	 */
	bool fileExists(const files::Char* fileName) {
		if(fileName == 0)
			throw NullPointerException("fileName");
#ifdef ASCENSION_WINDOWS
#ifdef PathFileExists
		return toBoolean(::PathFileExistsW(fileName));
#else
		if(::GetFileAttributesW(fileName) != INVALID_FILE_ATTRIBUTES)
			return true;
		const ::DWORD e = ::GetLastError();
		if(e == ERROR_FILE_NOT_FOUND || e == ERROR_PATH_NOT_FOUND
				|| e == ERROR_INVALID_NAME || e == ERROR_INVALID_PARAMETER || e == ERROR_BAD_NETPATH)
			return false;
#endif /* PathFileExists */
#else // ASCENSION_POSIX
		struct stat s;
		if(::stat(fileName, &s) == 0)
			return true;
		else if(errno == ENOENT)
			return false;
#endif
		throw files::IOException(files::IOException::PLATFORM_DEPENDENT_ERROR);
	}

	/// Finds the base name in the given file path name.
	inline files::String::const_iterator findFileName(const files::String& s) {
		const files::String::size_type i = s.find_last_of(PATH_SEPARATORS);
		return s.begin() + ((i != files::String::npos) ? i + 1 : 0);
	}

	/**
	 * Returns the last write time of the specified file.
	 * @param fileName the name of the file
	 * @param[out] timeStamp the time
	 * @throw files#IOException any I/O error occured
	 */
	void getFileLastWriteTime(const files::String& fileName, files::FileBinder::Time& timeStamp) {
#ifdef ASCENSION_WINDOWS
		::WIN32_FILE_ATTRIBUTE_DATA attributes;
		if(::GetFileAttributesExW(fileName.c_str(), GetFileExInfoStandard, &attributes) != 0)
			timeStamp = attributes.ftLastWriteTime;
		else {
			const ::DWORD e = ::GetLastError();
			throw files::IOException((e == ERROR_FILE_NOT_FOUND || e == ERROR_PATH_NOT_FOUND) ?
				files::IOException::FILE_NOT_FOUND : files::IOException::PLATFORM_DEPENDENT_ERROR);
		}
#else // ASCENSION_POSIX
		struct stat s;
		if(::stat(fileName.c_str(), &s) == 0)
			timeStamp = s.st_mtime;
		else
			throw files::IOException((errno == ENOENT) ?
				files::IOException::FILE_NOT_FOUND : files::IOException::PLATFORM_DEPENDENT_ERROR);
#endif
	}

	/**
	 * Returns the size of the specified file.
	 * @param fileName the name of the file
	 * @return the size of the file in bytes or -1 if the file is too large
	 * @throw NullPointerException @a fileName is @c null
	 * @throw files#IOException any I/O error occured
	 */
	ptrdiff_t getFileSize(const files::Char* fileName) {
		if(fileName == 0)
			throw NullPointerException("fileName");
#ifdef ASCENSION_WINDOWS
		::WIN32_FILE_ATTRIBUTE_DATA attributes;
		if(::GetFileAttributesExW(fileName, GetFileExInfoStandard, &attributes) == 0) {
			::DWORD e = ::GetLastError();
			throw files::IOException(
				(e == ERROR_PATH_NOT_FOUND || e == ERROR_INVALID_NAME || e == ERROR_BAD_NETPATH) ?
					files::IOException::FILE_NOT_FOUND : files::IOException::PLATFORM_DEPENDENT_ERROR);
		}
		return (attributes.nFileSizeHigh == 0
			&& attributes.nFileSizeLow <= static_cast<::DWORD>(numeric_limits<ptrdiff_t>::max())) ?
				static_cast<ptrdiff_t>(attributes.nFileSizeLow) : -1;
#else // ASCENSION_POSIX
		struct stat s;
		if(::stat(fileName, &s) != 0)
			throw files::IOException((errno == ENOENT) ?
				files::IOException::FILE_NOT_FOUND : files::IOException::PLATFORM_DEPENDENT_ERROR);
		return s.st_size;
#endif
	}

	/**
	 * Creates a name for a temporary file.
	 * @param seed the string contains a directory path and a prefix string
	 * @return the result string
	 * @throw files#IOException any I/O error occured
	 */
	files::String getTemporaryFileName(const files::String& seed) {
		manah::AutoBuffer<files::Char> s(new files::Char[seed.length() + 1]);
		copy(seed.begin(), seed.end(), s.get());
		s[seed.length()] = 0;
		files::Char* name = s.get() + (findFileName(seed) - seed.begin());
		if(name != s.get())
			name[-1] = 0;
#ifdef ASCENSION_WINDOWS
		::WCHAR result[MAX_PATH];
		if(::GetTempFileNameW(s.get(), name, 0, result) != 0)
			return result;
#else // ASCENSION_POSIX
		if(files::Char* p = ::tempnam(s.get(), name)) {
			files::String result(p);
			::free(p);
			return result;
		}
#endif
		throw files::IOException(files::IOException::PLATFORM_DEPENDENT_ERROR);
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
files::String kernel::files::canonicalizePathName(const files::Char* pathName) {
	if(pathName == 0)
		throw NullPointerException("pathName");

#ifdef ASCENSION_WINDOWS

	if(wcslen(pathName) >= MAX_PATH)	// too long name
		return pathName;

	// resolve relative path name
	::WCHAR path[MAX_PATH];
	::WCHAR* dummy;
	if(::GetFullPathNameW(pathName, countof(path), path, &dummy) == 0)
		wcscpy(path, pathName);

	// get real component names (from Ftruename implementation in xyzzy)
	files::String result;
	result.reserve(MAX_PATH);
	const files::Char* p = path;
	if(((p[0] >= L'A' && p[0] <= L'Z') || (p[0] >= L'a' && p[0] <= L'z'))
			&& p[1] == L':' && (p[2] == L'\\' || p[2] == L'/')) {	// drive letter
		result.append(path, 3);
		result[0] = towupper(path[0]);	// unify with uppercase letters...
		p += 3;
	} else if((p[0] == L'\\' || p[0] == L'/') && (p[1] == L'\\' || p[1] == L'/')) {	// UNC?
		if((p = wcspbrk(p + 2, L"\\/")) == 0)	// server name
			return false;
		if((p = wcspbrk(p + 1, L"\\/")) == 0)	// shared name
			return false;
		result.append(path, ++p - path);
	} else	// not absolute name
		return pathName;

	::WIN32_FIND_DATAW wfd;
	while(true) {
		if(files::Char* next = wcspbrk(p, L"\\/")) {
			const files::Char c = *next;
			*next = 0;
			::HANDLE h = ::FindFirstFileW(path, &wfd);
			if(h != INVALID_HANDLE_VALUE) {
				::FindClose(h);
				result += wfd.cFileName;
			} else
				result += p;
			*next = c;
			result += L'\\';
			p = next + 1;
		} else {
			::HANDLE h = ::FindFirstFileW(path, &wfd);
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

	files::Char resolved[PATH_MAX];
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
bool kernel::files::comparePathNames(const files::Char* s1, const files::Char* s2) {
	if(s1 == 0 || s2 == 0)
		throw NullPointerException("either file name is null.");

#ifdef ASCENSION_WINDOWS
#ifdef PathMatchSpec
	if(toBoolean(::PathMatchSpecW(s1, s2)))
		return true;
#endif /* PathMatchSpec */
	// by lexicographical comparison
	const int c1 = static_cast<int>(wcslen(s1)) + 1, c2 = static_cast<int>(wcslen(s2)) + 1;
	const int fc1 = ::LCMapStringW(LOCALE_NEUTRAL, LCMAP_LOWERCASE, s1, c1, 0, 0);
	const int fc2 = ::LCMapStringW(LOCALE_NEUTRAL, LCMAP_LOWERCASE, s2, c2, 0, 0);
	if(fc1 != 0 && fc2 != 0 && fc1 == fc2) {
		manah::AutoBuffer<WCHAR> fs1(new ::WCHAR[fc1]), fs2(new ::WCHAR[fc2]);
		::LCMapStringW(LOCALE_NEUTRAL, LCMAP_LOWERCASE, s1, c1, fs1.get(), fc1);
		::LCMapStringW(LOCALE_NEUTRAL, LCMAP_LOWERCASE, s2, c2, fs2.get(), fc2);
		if(wmemcmp(fs1.get(), fs2.get(), fc1) == 0)
			return fileExists(s1);
	}
	// by volume information
	bool eq = false;
	::HANDLE f1 = ::CreateFileW(s1, 0,
		FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	if(f1 != INVALID_HANDLE_VALUE) {
		::HANDLE f2 = ::CreateFileW(s1, 0,
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

namespace {
#ifdef _DEBUG
	// for Document.length_ diagnostic
	length_t calculateDocumentLength(const Document& document) {
		length_t c = 0;
		const length_t lines = document.numberOfLines();
		for(length_t i = 0; i < lines; ++i)
			c += document.lineLength(i);
		return c;
	}
#endif /* _DEBUG */
} // namespace @0


// Point ////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param document the document to which the point attaches
 * @param position the initial position of the point
 * @throw BadPositionException @a position is outside of the document
 */
Point::Point(Document& document, const Position& position /* = Position() */) :
		document_(&document), position_(position), adapting_(true), excludedFromRestriction_(false), gravity_(FORWARD) {
	if(!document.region().includes(position))
		throw BadPositionException();
	static_cast<internal::IPointCollection<Point>&>(document).addNewPoint(*this);
}

/// Copy-constructor.
Point::Point(const Point& rhs) :
		document_(rhs.document_), position_(rhs.position_), adapting_(rhs.adapting_),
		excludedFromRestriction_(rhs.excludedFromRestriction_), gravity_(rhs.gravity_) {
	if(document_ == 0)
		throw DisposedDocumentException();
	static_cast<internal::IPointCollection<Point>*>(document_)->addNewPoint(*this);
}

/// Destructor.
Point::~Point() throw() {
	lifeCycleListeners_.notify(&IPointLifeCycleListener::pointDestroyed);
	if(document_ != 0)
		static_cast<internal::IPointCollection<Point>*>(document_)->removePoint(*this);
}

/**
 * Registers the lifecycle listener.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void Point::addLifeCycleListener(IPointLifeCycleListener& listener) {
	lifeCycleListeners_.add(listener);
}

/**
 * Moves to the specified position.
 * Derived classes can override this method to hook all movement of the point.
 * @param to the position
 */
void Point::doMoveTo(const Position& to) {
	verifyDocument();
	if(position_ != to) {
		position_ = to;
		normalize();
	}
}

/**
 * Moves to the specified position.
 * @param to the position
 */
void Point::moveTo(const Position& to) {
	verifyDocument();
	doMoveTo(to);
}

/**
 * Normalizes the position of the point.
 * This method does <strong>not</strong> inform to the listeners about any movement.
 */
void Point::normalize() const {
	verifyDocument();
	Position& position = const_cast<Point*>(this)->position_;
	position.line = min(position.line, document_->numberOfLines() - 1);
	position.column = min(position.column, document_->lineLength(position.line));
	if(document_->isNarrowed() && excludedFromRestriction_) {
		const Region r(document_->accessibleRegion());
		position = max(position_, r.first);
		position = min(position_, r.second);
	}
}

/**
 * Removes the lifecycle listener
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void Point::removeLifeCycleListener(IPointLifeCycleListener& listener) {
	lifeCycleListeners_.remove(listener);
}

/**
 * Called when the document was changed.
 * @param change the content of the document change
 */
void Point::update(const DocumentChange& change) {
	if(document_ == 0 || !adapting_)
		return;

//	normalize();
	const Position newPosition = updatePosition(position_, change, gravity_);
	if(newPosition == position_)
		return;
	doMoveTo(newPosition);
}


// Bookmarker ///////////////////////////////////////////////////////////////

/**
 * Private constructor.
 * @param document the document
 */
Bookmarker::Bookmarker(Document& document) throw() : document_(document) {
}

/**
 * Registers the listener.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void Bookmarker::addListener(IBookmarkListener& listener) {
	listeners_.add(listener);
}

/// Deletes all bookmarks.
void Bookmarker::clear() throw() {
	const length_t lines = document_.numberOfLines();
	bool clearedOnce = false;
	for(length_t i = 0; i < lines; ++i) {
		const Document::Line& line = document_.getLineInformation(i);
		if(line.bookmarked_) {
			line.bookmarked_ = false;
			clearedOnce = true;
		}
	}
	if(clearedOnce)
		listeners_.notify(&IBookmarkListener::bookmarkCleared);
}

/**
 * Returns the line number of the next bookmarked line.
 * @param startLine the start line number to search. this line may be the result
 * @param direction direction to search
 * @return the next bookmarked line or @c INVALID_INDEX if not found
 * @throw BadPositionException @a line is outside of the document
 */
length_t Bookmarker::getNext(length_t startLine, Direction direction) const {
	const length_t lines = document_.numberOfLines();
	if(startLine >= lines)
		throw BadPositionException();
	else if(direction == FORWARD) {
		for(length_t line = startLine; line < lines; ++line) {
			if(document_.getLineInformation(line).bookmarked_)
				return line;
		}
	} else {
		for(length_t line = startLine + 1; line >= 0; --line) {
			if(document_.getLineInformation(line - 1).bookmarked_)
				return line - 1;
		}
	}
	return INVALID_INDEX;
}

/**
 * Returns true if the specified line is bookmarked.
 * @param line the line
 * @throw BadPositionException @a line is outside of the document
 */
bool Bookmarker::isMarked(length_t line) const {
	return document_.getLineInformation(line).bookmarked_;
}

/**
 * Sets or clears the bookmark of the specified line.
 * @param line the line
 * @param set true to set bookmark, false to clear
 * @throw BadPositionException @a line is outside of the document
 */
void Bookmarker::mark(length_t line, bool set) {
	const Document::Line& l = document_.getLineInformation(line);
	if(l.bookmarked_ != set) {
		l.bookmarked_ = set;
		listeners_.notify<length_t>(&IBookmarkListener::bookmarkChanged, line);
	}
}

/**
 * Removes the listener.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void Bookmarker::removeListener(IBookmarkListener& listener) {
	listeners_.remove(listener);
}

/**
 * Toggles the bookmark of the spcified line.
 * @param line the line
 * @throw BadPositionException @a line is outside of the document
 */
void Bookmarker::toggle(length_t line) {
	const Document::Line& l = document_.getLineInformation(line);
	l.bookmarked_ = !l.bookmarked_;
	listeners_.notify<length_t>(&IBookmarkListener::bookmarkChanged, line);
}


// DocumentPartitioner //////////////////////////////////////////////////////

/// Constructor.
DocumentPartitioner::DocumentPartitioner() throw() : document_(0) {
}

/// Destructor.
DocumentPartitioner::~DocumentPartitioner() throw() {
}


// Document.UndoManager /////////////////////////////////////////////////////

// about document undo/redo
namespace {
	class InsertOperation;
	class DeleteOperation;

	/**
	 * An abstract edit operation.
	 * @see DeleteOperation, InsertOperation
	 */
	class IOperation {
	public:
		/// Destructor
		virtual ~IOperation() {}
		/// Returns the operation is executable.
		virtual bool canExecute(Document& document) const = 0;
		/// Returns true if the operation can be appended to insertion @a postOperation.
		virtual bool isConcatenatable(InsertOperation& postOperation, const Document& document) const = 0;
		/// Returns true if the operation can be appended to deletion @a postOperation.
		virtual bool isConcatenatable(DeleteOperation& postOperation, const Document& document) const = 0;
		/// Executes the operation.
		virtual Position execute(Document& document) = 0;
	};

	/// An insertion operation.
	class InsertOperation : virtual public IOperation, public manah::FastArenaObject<InsertOperation> {
	public:
		InsertOperation(const Position& pos, const String& text) : position_(pos), text_(text) {}
		bool canExecute(Document& document) const throw() {return !document.isNarrowed() || document.region().includes(position_);}
		bool isConcatenatable(InsertOperation&, const Document&) const throw() {return false;}
		bool isConcatenatable(DeleteOperation&, const Document&) const throw() {return false;}
		Position execute(Document& document) {return document.insert(position_, text_);}
	private:
		Position position_;
		String text_;
	};

	/// An deletion operation.
	class DeleteOperation : virtual public IOperation, public manah::FastArenaObject<DeleteOperation> {
	public:
		DeleteOperation(const Region& region) throw() : region_(region) {}
		bool canExecute(Document& document) const throw() {return !document.isNarrowed() || document.region().encompasses(region_);}
		bool isConcatenatable(InsertOperation&, const Document&) const throw() {return false;}
		bool isConcatenatable(DeleteOperation& postOperation, const Document&) const throw() {
			const Position& bottom = region_.end();
			if(bottom.column == 0 || bottom != postOperation.region_.beginning()) return false;
			else {const_cast<DeleteOperation*>(this)->region_.end() = postOperation.region_.end(); return true;}
		}
		Position execute(Document& document) {return document.erase(region_);}
	private:
		Region region_;
	};

	/// A compound operation.
	class CompoundOperation : public manah::FastArenaObject<CompoundOperation> {
	public:
		~CompoundOperation() throw();
		pair<bool, size_t> execute(Document& document, Position& resultPosition);
		void pop() {operations_.pop();}
		void push(InsertOperation& operation, const Document&) {operations_.push(&operation);}
		void push(DeleteOperation& operation, const Document& document);
		IOperation& top() const {return *operations_.top();}
	private:
		stack<IOperation*> operations_;
	};
} // namespace @0

CompoundOperation::~CompoundOperation() throw() {
	while(!operations_.empty()) {
		delete operations_.top();
		operations_.pop();
	}
}

inline pair<bool, size_t> CompoundOperation::execute(Document& document, Position& resultPosition) {
	const size_t c = operations_.size();
	resultPosition = Position::INVALID_POSITION;
	while(!operations_.empty()) {
		if(!operations_.top()->canExecute(document))
			break;
		resultPosition = operations_.top()->execute(document);
		delete operations_.top();
		operations_.pop();
	}
	// return <completely executed?, # of executed operations>
	return make_pair(operations_.empty(), c - operations_.size());
}

inline void CompoundOperation::push(DeleteOperation& operation, const Document& document) {
	// if also the previous operation is deletion, extend the region to concatenate the operations
	if(!operations_.empty() && operations_.top()->isConcatenatable(operation, document))
		delete &operation;
	else
		operations_.push(&operation);
}

/// Manages undo/redo of the document.
class Document::UndoManager {
	MANAH_NONCOPYABLE_TAG(UndoManager);
public:
	// constructors
	UndoManager(Document& document) throw();
	virtual ~UndoManager() throw();
	// attributes
	size_t	numberOfRedoableCompoundOperations() const throw();
	size_t	numberOfUndoableCompoundOperations() const throw();
	bool	isStackingCompoundOperation() const throw();
	// operations
	void				beginCompoundOperation() throw();
	void				clear() throw();
	void				endCompoundOperation() throw();
	template<typename Operation>
	void				pushUndoableOperation(Operation& operation);
	pair<bool, size_t>	redo(Position& resultPosition);
	pair<bool, size_t>	undo(Position& resultPosition);
private:
	Document& document_;
	stack<CompoundOperation*> undoStack_, redoStack_;
	enum {
		NONE, WAIT_FOR_FIRST_PUSH, WAIT_FOR_CONTINUATION
	} compoundOperationStackingState_;
	bool virtualOperation_;
	CompoundOperation* virtualUnit_;
	CompoundOperation* lastUnit_;
	IOperation* savedOperation_;
};

/**
 * Constructor.
 * @param document the target document
 */
Document::UndoManager::UndoManager(Document& document) throw()
		: document_(document), compoundOperationStackingState_(NONE),
		virtualOperation_(false), virtualUnit_(0), lastUnit_(0), savedOperation_(0) {
}

/// Destructor.
Document::UndoManager::~UndoManager() throw() {
	clear();
}

/// Starts the compound operation.
inline void Document::UndoManager::beginCompoundOperation() throw() {
	assert(compoundOperationStackingState_ == NONE);
	compoundOperationStackingState_ = WAIT_FOR_FIRST_PUSH;
}

/// Clears the stacks.
inline void Document::UndoManager::clear() throw() {
	compoundOperationStackingState_ = NONE;
	lastUnit_ = 0;
	while(!undoStack_.empty()) {
		delete undoStack_.top();
		undoStack_.pop();
	}
	while(!redoStack_.empty()) {
		delete redoStack_.top();
		redoStack_.pop();
	}
}

/// Ends the compound operation.
inline void Document::UndoManager::endCompoundOperation() throw() {
	compoundOperationStackingState_ = NONE;
}

/// Returns true if the compound operation is running.
inline bool Document::UndoManager::isStackingCompoundOperation() const throw() {
	return compoundOperationStackingState_ != NONE;
}

/// Returns the number of the redoable operations.
inline size_t Document::UndoManager::numberOfRedoableCompoundOperations() const throw() {
	return redoStack_.size();
}

/// Returns the number of the undoable operations.
inline size_t Document::UndoManager::numberOfUndoableCompoundOperations() const throw() {
	return undoStack_.size();
}

/**
 * Pushes the operation into the undo stack.
 * @param operation the operation to be pushed
 */
template<typename Operation> void Document::UndoManager::pushUndoableOperation(Operation& operation) {
	// make the redo stack empty
	if(!virtualOperation_) {
		while(!redoStack_.empty()) {
			delete redoStack_.top();
			redoStack_.pop();
		}
	}

	if(virtualOperation_) {	// 仮想操作時はスタックへの追加を遅延する
		if(virtualUnit_ == 0)	// 初回
			virtualUnit_ = new CompoundOperation();
		virtualUnit_->push(operation, document_);
	} else if(compoundOperationStackingState_ == WAIT_FOR_CONTINUATION && lastUnit_ != 0)	// 最後の操作単位に結合
		lastUnit_->push(operation, document_);
	else {
		CompoundOperation* newUnit = new CompoundOperation();
		newUnit->push(operation, document_);
		undoStack_.push(newUnit);
		lastUnit_ = newUnit;
		if(compoundOperationStackingState_ == WAIT_FOR_FIRST_PUSH)
			compoundOperationStackingState_ = WAIT_FOR_CONTINUATION;
	}
}

/// Redoes one operation.
pair<bool, size_t> Document::UndoManager::redo(Position& resultPosition) {
	if(redoStack_.empty())
		return make_pair(false, 0);

	CompoundOperation* unit = redoStack_.top();
	virtualOperation_ = true;			// 仮想操作開始
	const pair<bool, size_t> status(unit->execute(document_, resultPosition));
	if(status.first)
		redoStack_.pop();
	if(virtualUnit_ != 0)
		undoStack_.push(virtualUnit_);	// 仮想操作単位をアンドゥスタックへ移す
	virtualUnit_ = lastUnit_ = 0;
	virtualOperation_ = false;			// 仮想操作終了
	if(status.first)
		delete unit;
	return status;
}

/// Undoes one operation.
pair<bool, size_t> Document::UndoManager::undo(Position& resultPosition) {
	if(undoStack_.empty())
		return make_pair(false, 0);

	CompoundOperation* unit = undoStack_.top();
	virtualOperation_ = true;			// 仮想操作開始
	const pair<bool, size_t> status = unit->execute(document_, resultPosition);
	if(status.first)
		undoStack_.pop();
	if(virtualUnit_ != 0)
		redoStack_.push(virtualUnit_);	// 仮想操作単位をリドゥスタックへ移す
	virtualUnit_ = lastUnit_ = 0;
	virtualOperation_ = false;			// 仮想操作終了
	if(status.first)
		delete unit;
	return status;
}


// Document //////////////////////////////////////////////////////////////////

/**
 * @class ascension::kernel::Document
 * A document manages a text content and supports text manipulations.
 *
 * All text content is represented in UTF-16. To treat this as UTF-32, use
 * @c DocumentCharacterIterator.
 *
 * A document manages also its operation history, encoding, and line-breaks
 * and writes to or reads the content from files or streams.
 *
 * @c #insert inserts a text string into any position. @c #erase deletes any text region.
 * Other classes also provide text manipulation for the document.
 *
 * A document manages a revision number indicates how many times the document was changed. This
 * value is initially zero and incremented by @c #insert, @c #insertFromStream, @c #erase, and
 * @c #resetContent methods. A revision number is never decremented or reset to zero. The current
 * revision number can be obtained by @c #getRevisionNumber.
 *
 * A document can be devides into a sequence of semantic segments called partition.
 * Document partitioners expressed by @c DocumentPartitioner class define these
 * partitioning. Each partitions have its content type and region (see @c DocumentPartition).
 * To set the new partitioner, use @c #setPartitioner method. The partitioner's ownership
 * will be transferred to the document.
 *
 * @see Viewer, IDocumentPartitioner, Point, EditPoint
 */

const DocumentPropertyKey Document::TITLE_PROPERTY;

/// Constructor.
Document::Document() : session_(0), partitioner_(0),
		contentTypeInformationProvider_(new DefaultContentTypeInformationProvider),
		readOnly_(false), modified_(false), length_(0), revisionNumber_(0),
		onceUndoBufferCleared_(false), recordingOperations_(true), changing_(false), accessibleArea_(0) {
	bookmarker_.reset(new Bookmarker(*this));
	undoManager_ = new UndoManager(*this);
	resetContent();
}

/// Destructor.
Document::~Document() {
	for(set<Point*>::iterator i(points_.begin()), e(points_.end()); i != e; ++i)
		(*i)->documentDisposed();
	if(accessibleArea_ != 0) {
		delete accessibleArea_->second;
		delete accessibleArea_;
	}
	for(map<const DocumentPropertyKey*, String*>::iterator i(properties_.begin()), e(properties_.end()); i != e; ++i)
		delete i->second;
	delete undoManager_;
}

/**
 * Registers the document listener with the document. After registration @a listener is notified
 * about each modification of this document.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void Document::addListener(IDocumentListener& listener) {
	if(find(listeners_.begin(), listeners_.end(), &listener) != listeners_.end())
		throw invalid_argument("the listener already has been registered.");
	listeners_.push_back(&listener);
}

/**
 * Registers the document listener as one which is notified before those document listeners
 * registered with @c #addListener are notified.
 * @internal This method is not for public use.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void Document::addPrenotifiedListener(IDocumentListener& listener) {
	if(find(prenotifiedListeners_.begin(), prenotifiedListeners_.end(), &listener) != prenotifiedListeners_.end())
		throw invalid_argument("the listener already has been registered.");
	prenotifiedListeners_.push_back(&listener);
}

/**
 * Starts the sequential edit. Restarts if the sequential edit is already running.
 * @see #endSequentialEdit, #isSequentialEditing
 */
void Document::beginSequentialEdit() throw() {
	if(isSequentialEditing())
		endSequentialEdit();
	undoManager_->beginCompoundOperation();
	sequentialEditListeners_.notify<Document&>(&ISequentialEditListener::documentSequentialEditStarted, *this);
}

/// Clears the undo/redo stacks and deletes the history.
void Document::clearUndoBuffer() {
	undoManager_->clear();
	onceUndoBufferCleared_ = true;
}

/// @c #resetContent invokes this method finally. Default implementation does nothing.
void Document::doResetContent() {
}

/**
 * Ends the active sequential edit.
 * @see #beginSequentialEdit, #isSequentialEditing
 */
void Document::endSequentialEdit() throw() {
	undoManager_->endCompoundOperation();
	sequentialEditListeners_.notify<Document&>(&ISequentialEditListener::documentSequentialEditStopped, *this);
}

/**
 * Deletes the specified region of the document.
 *
 * This method sets the modification flag and calls the listeners'
 * @c IDocumentListener#documentAboutToBeChanged and @c IDocumentListener#documentChanged.
 *
 * If the specified region intersects the inaccessible region, the union is not deleted.
 * @param region the region to be deleted
 * @return the position where the point which deletes the text will move to
 * @throw ReadOnlyDocumentException the document is read only
 */
Position Document::erase(const Region& region) {
	if(changing_ || isReadOnly())
		throw ReadOnlyDocumentException();
	else if(region.isEmpty())	// empty -> ignore
		return region.beginning();
	else if(isNarrowed()) {
		const Region r(accessibleRegion());
		if(region.end() <= r.first)
			return region.end();
		else if(region.beginning() >= r.second)
			return region.beginning();
	}

	ModificationGuard guard(*this);
	if(!fireDocumentAboutToBeChanged(DocumentChange(true, region)))
		return region.beginning();
	return eraseText(region);
}

Position Document::eraseText(const Region& region) {
	const Position beginning(isNarrowed() ? max(region.beginning(), accessibleRegion().first) : region.beginning());
	const Position end(isNarrowed() ? min(region.end(), accessibleRegion().second) : region.end());
	StringBuffer deletedString;

	if(beginning.line == end.line) {	// region is single line
		const Line&	lineInfo = getLineInformation(end.line);
		String&	line = const_cast<String&>(this->line(end.line));

		++const_cast<Line&>(lineInfo).operationHistory_;
		deletedString.sputn(line.data() + beginning.column, static_cast<streamsize>(end.column - beginning.column));
		line.erase(beginning.column, end.column - beginning.column);
		length_ -= end.column - beginning.column;
	} else {						// region is multiline
		Line* line = lines_[beginning.line];
		auto_ptr<String> tail;
		deletedString.sputn(line->text_.data() + beginning.column, static_cast<streamsize>(line->text_.length() - beginning.column));
		length_ -= line->text_.length() - beginning.column;
		line->text_.erase(beginning.column);

		Line& firstLine = *lines_[beginning.line];
		Newline lastNewline;
		deletedString.sputn(getNewlineString(lines_[beginning.line]->newline_),
			static_cast<streamsize>(getNewlineStringLength(lines_[beginning.line]->newline_)));
		for(length_t i = beginning.line + 1; ; ++i) {
			line = lines_[i];
			deletedString.sputn(line->text_.data(), static_cast<streamsize>((i != end.line) ? line->text_.length() : end.column));
			length_ -= line->text_.length();
			if(i != end.line)
				deletedString.sputn(getNewlineString(line->newline_), static_cast<streamsize>(getNewlineStringLength(line->newline_)));
			else {	// last
				tail.reset(new String(line->text_.substr(end.column)));
				lastNewline = line->newline_;
				break;
			}
		}
		lines_.erase(beginning.line + 1, end.line - beginning.line);

		// concatinate before and after erased part
		firstLine.newline_ = lastNewline;
		++firstLine.operationHistory_;
		if(!tail->empty()) {
			firstLine.text_ += *tail;
			length_ += tail->length();
		}
	}

	if(recordingOperations_)
		undoManager_->pushUndoableOperation(*(new InsertOperation(beginning, deletedString.str())));

	// notify the change
	++revisionNumber_;
	fireDocumentChanged(DocumentChange(true, Region(beginning, end)));
	setModified();

	return beginning;
}

bool Document::fireDocumentAboutToBeChanged(const DocumentChange& c) throw() {
	if(partitioner_.get() != 0)
		partitioner_->documentAboutToBeChanged();
	for(list<IDocumentListener*>::iterator i(prenotifiedListeners_.begin()), e(prenotifiedListeners_.end()); i != e; ++i) {
		if(!(*i)->documentAboutToBeChanged(*this, c))
			return false;
	}
	for(list<IDocumentListener*>::iterator i(listeners_.begin()), e(listeners_.end()); i != e; ++i) {
		if(!(*i)->documentAboutToBeChanged(*this, c))
			return false;
	}
	return true;
}

void Document::fireDocumentChanged(const DocumentChange& c, bool updateAllPoints /* = true */) throw() {
	if(partitioner_.get() != 0)
		partitioner_->documentChanged(c);
	if(updateAllPoints)
		updatePoints(c);
	for(list<IDocumentListener*>::iterator i(prenotifiedListeners_.begin()), e(prenotifiedListeners_.end()); i != e; ++i)
		(*i)->documentChanged(*this, c);
	for(list<IDocumentListener*>::iterator i(listeners_.begin()), e(listeners_.end()); i != e; ++i)
		(*i)->documentChanged(*this, c);
}

/**
 * Inserts the text into the specified position. For detail, see two-parameter version of this method.
 *
 * This method sets the modification flag and calls the listeners'
 * @c IDocumentListener#documentAboutToBeChanged and @c IDocumentListener#documentChanged.

 * @param position the position
 * @param first the start of the text
 * @param last the end of the text
 * @return the result position
 * @throw ReadOnlyDocumentException the document is read only
 * @throw NullPointerException either @a first or @a last is @c null
 * @throw std#invalid_argument either @a first is greater than @a last
 */
Position Document::insert(const Position& position, const Char* first, const Char* last) {
	if(changing_ || isReadOnly())
		throw ReadOnlyDocumentException();
	else if(first == 0 || last == 0)
		throw NullPointerException("first and/or last are null.");
	else if(first > last)
		throw invalid_argument("first > last");
	else if(isNarrowed() && !accessibleRegion().includes(position))	// ignore the insertion position is out of the accessible region
		return position;
	else if(first == last)	// ignore if the input is empty
		return position;

	ModificationGuard guard(*this);
	if(!fireDocumentAboutToBeChanged(DocumentChange(false, Region(position))))
		return position;
	return insertText(position, first, last);
}

Position Document::insertText(const Position& position, const Char* first, const Char* last) {
	Position resultPosition(position.line, 0);
	const Char* breakPoint = find_first_of(first, last, LINE_BREAK_CHARACTERS, endof(LINE_BREAK_CHARACTERS));

	if(breakPoint == last) {	// single-line
		Line& line = const_cast<Line&>(getLineInformation(position.line));
		line.text_.insert(position.column, first, static_cast<String::size_type>(last - first));
		length_ += static_cast<length_t>(last - first);
		++line.operationHistory_;
		resultPosition.column = position.column + (last - first);
	} else {	// multiline
		length_t line = position.line;
		Line& firstLine = *lines_[line];
		const Char* lastBreak;
		const Newline firstNewline = firstLine.newline_;	// 先頭行の改行文字 (挿入後、一番後ろに付けられる)

		// 最後の改行位置を探し、resultPosition の文字位置も決定する
		for(lastBreak = last - 1; ; --lastBreak) {
			if(binary_search(LINE_BREAK_CHARACTERS, endof(LINE_BREAK_CHARACTERS), *lastBreak))
				break;
		}
		resultPosition.column = static_cast<length_t>((last - first) - (lastBreak - first) - 1);
		if(*lastBreak == LINE_FEED && lastBreak != breakPoint && *(lastBreak - 1) == CARRIAGE_RETURN)
			--lastBreak;

		// 先頭行の置換
		const String firstLineRest = firstLine.text_.substr(position.column, firstLine.text_.length() - position.column);
		length_ += breakPoint - first - firstLineRest.length();
		firstLine.text_.replace(position.column, firstLineRest.length(), first, static_cast<String::size_type>(breakPoint - first));
		firstLine.newline_ = eatNewline(breakPoint, last);
		breakPoint += (firstLine.newline_ != NLF_CRLF) ? 1 : 2;
		++firstLine.operationHistory_;
		++line;
		++resultPosition.line;

		// 改行ごとに行に区切っていく
		while(true) {
			if(breakPoint <= lastBreak) {
				const Char* const nextBreak =
					find_first_of(breakPoint, last, LINE_BREAK_CHARACTERS, endof(LINE_BREAK_CHARACTERS));
				assert(nextBreak != last);
				const Newline newline = eatNewline(nextBreak, last);

				length_ += nextBreak - breakPoint;
				lines_.insert(line, new Line(String(breakPoint, nextBreak), newline, true));
				++line;
				++resultPosition.line;
				breakPoint = nextBreak + ((newline != NLF_CRLF) ? 1 : 2);
			} else {	// 最終行
				length_ += last - breakPoint + firstLineRest.length();
				lines_.insert(line, new Line(String(breakPoint, last) + firstLineRest, firstNewline, true));
				break;
			}
		}
	}

	if(recordingOperations_)
		undoManager_->pushUndoableOperation(*(new DeleteOperation(Region(position, resultPosition))));

	// notify the change
	++revisionNumber_;
	fireDocumentChanged(DocumentChange(false, Region(position, resultPosition)));
	setModified();
//	assert(length_ == calculateDocumentLength(*this));	// length_ メンバの診断

	return resultPosition;
}

/**
 * Returns true if the document is sequential editing.
 * @see #beginSequentialEdit, #endSequentialEdit
 */
bool Document::isSequentialEditing() const throw() {
	return undoManager_->isStackingCompoundOperation();
}

/**
 * Returns the number of characters (UTF-16 code units) in the document.
 * @param nlr the method to count newlines
 * @return the number of characters
 * @throw std#invalid_argument @a nlr is invalid
 */
length_t Document::length(NewlineRepresentation nlr) const {
	switch(nlr) {
	case NLR_LINE_FEED:
	case NLR_LINE_SEPARATOR:
		return length_ + numberOfLines() - 1;
	case NLR_CRLF:
		return length_ + numberOfLines() * 2 - 1;
	case NLR_PHYSICAL_DATA: {
		length_t len = length_;
		const length_t lines = numberOfLines();
		for(length_t i = 0; i < lines; ++i)
			len += getNewlineStringLength(lines_[i]->newline_);
		return len;
	}
	case NLR_SKIP:
		return length_;
	}
	throw invalid_argument("invalid parameter.");
}

/**
 * Returns the offset of the line.
 * @param line the line
 * @param nlr the line representation policy for character counting
 * @throw BadPostionException @a line is outside of the document
 */
length_t Document::lineOffset(length_t line, NewlineRepresentation nlr) const {
	if(line >= lines_.getSize())
		throw BadPositionException();

	length_t offset = 0;
	for(length_t i = 0; i < line; ++i) {
		const Line& ln = *lines_[i];
		offset += ln.text_.length();
		switch(nlr) {
		case NLR_LINE_FEED:
		case NLR_LINE_SEPARATOR:	offset += 1; break;
		case NLR_CRLF:				offset += 2; break;
		case NLR_PHYSICAL_DATA:		offset += getNewlineStringLength(ln.newline_); break;
		case NLR_SKIP:				break;
		}
	}
	return offset;
}

/**
 * Narrows the accessible area to the specified region.
 * @param region the region
 * @see #isNarrowed, #widen
 */
void Document::narrow(const Region& region) {
	if(accessibleArea_ == 0)
		accessibleArea_ = new pair<Position, Point*>;
	accessibleArea_->first = region.beginning();
	accessibleArea_->second = new Point(*this);
	accessibleArea_->second->moveTo(region.end());
	for(set<Point*>::iterator i = points_.begin(); i != points_.end(); ++i) {
		if((*i)->isExcludedFromRestriction())
			(*i)->normalize();
	}
	stateListeners_.notify<Document&>(&IDocumentStateListener::documentAccessibleRegionChanged, *this);
}

/// Returns the number of undoable edits.
size_t Document::numberOfUndoableEdits() const throw() {
	return undoManager_->numberOfUndoableCompoundOperations();
}

/// Returns the number of redoable edits.
size_t Document::numberOfRedoableEdits() const throw() {
	return undoManager_->numberOfRedoableCompoundOperations();
}

/**
 * Sets whether the document records or not the operations for undo/redo.
 *
 * The default is true. If change the setting, recorded contents will be disposed.
 * @param record set true to record
 * @see #isRecordingOperation, #undo, #redo
 */
void Document::recordOperations(bool record) {
	if(!(recordingOperations_ = record))
		clearUndoBuffer();
}

/**
 * Performs the redo.
 * @return false if the redo was not completely performed
 * @throw ReadOnlyDocumentException the document is read only
 * @see #undo
 */
bool Document::redo() {
	if(isReadOnly())
		throw ReadOnlyDocumentException();
	else if(numberOfRedoableEdits() == 0)
		return false;

	beginSequentialEdit();
	sequentialEditListeners_.notify<Document&>(
		&ISequentialEditListener::documentUndoSequenceStarted, *this);
	Position resultPosition;
	const bool succeeded = undoManager_->redo(resultPosition).first;
	sequentialEditListeners_.notify<Document&, const Position&>(
		&ISequentialEditListener::documentUndoSequenceStopped, *this, resultPosition);
	endSequentialEdit();
	return succeeded;
}

/**
 * Removes the document listener from the document.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void Document::removeListener(IDocumentListener& listener) {
	const list<IDocumentListener*>::iterator i(find(listeners_.begin(), listeners_.end(), &listener));
	if(i == listeners_.end())
		throw invalid_argument("the listener is not registered.");
	listeners_.erase(i);
}

/**
 * Removes the pre-notified document listener from the document.
 * @internal This method is not for public use.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void Document::removePrenotifiedListener(IDocumentListener& listener) {
	const list<IDocumentListener*>::iterator i(find(prenotifiedListeners_.begin(), prenotifiedListeners_.end(), &listener));
	if(i == prenotifiedListeners_.end())
		throw invalid_argument("the listener is not registered.");
	prenotifiedListeners_.erase(i);
}

/**
 * Resets and initializes the content of the document. Does the following:
 * - Clears the text buffer, invokes the two methods of @c IDocumentListener and increments the
 *   revision number even if the document was empty.
 * - Moves the all point to the beginning of the document.
 * - Clears the undo/redo buffers.
 * - Resets the modification flag to false.
 * - Resets the read-only flag to false.
 * - Revokes the narrowing.
 * @see #doResetContent
 */
void Document::resetContent() {
	widen();
	for(set<Point*>::iterator i(points_.begin()), e(points_.end()); i != e; ++i)
		(*i)->moveTo(0, 0);

	const DocumentChange ca(true, region());
	fireDocumentAboutToBeChanged(ca);
	if(length_ != 0) {
		assert(!lines_.isEmpty());
		lines_.clear();
		lines_.insert(lines_.begin(), new Line);
		length_ = 0;
		++revisionNumber_;
	} else if(lines_.isEmpty())
		lines_.insert(lines_.begin(), new Line);
	fireDocumentChanged(ca, false);

	setReadOnly(false);
	setModified(false);
	clearUndoBuffer();
	onceUndoBufferCleared_ = false;
	doResetContent();
}

#if 0
/**
 * Sends the document to the user's mailer.
 * @param asAttachment true to send as attachment. in this case, the modification  is not 
 * 現在の変更は反映されない。本文として送信する場合は現在のドキュメントが使用される
 * @param showDialog true to show the user selection dialog
 * @return true if succeeded
 * @deprecated 0.8
 */
bool Document::sendFile(bool asAttachment, bool showDialog /* = true */) {
	if(asAttachment && getFilePathName() == 0)
		return false;

	ui::WaitCursor wc;
	HMODULE dll = ::LoadLibraryW(L"MAPI32.DLL");
	if(dll == 0)
		return false;

	MAPISENDMAIL* MAPISendMailPtr = reinterpret_cast<MAPISENDMAIL*>(::GetProcAddress(dll, "MAPISendMail"));
	if(MAPISendMailPtr == 0) {
		::FreeLibrary(dll);
		return false;
	}

	MANAH_AUTO_STRUCT(::MapiMessage, message);
	ulong error;

	message.flFlags = MAPI_RECEIPT_REQUESTED;

	if(asAttachment) {	// 添付ファイルにするとき
		MANAH_AUTO_STRUCT(::MapiFileDesc, fileDesc);
		const int cb = ::WideCharToMultiByte(CP_ACP, 0,
							getFilePathName(), static_cast<int>(wcslen(getFileName())), 0, 0, 0, 0);
		char* const filePath = new char[cb + 1];

		::WideCharToMultiByte(CP_ACP, 0, getFilePathName(), static_cast<int>(wcslen(getFileName())), filePath, cb, 0, 0);
		filePath[cb] = 0;
		message.nFileCount = 1;
		message.lpFiles = &fileDesc;

		fileDesc.lpszPathName = filePath;
		fileDesc.nPosition = static_cast<ulong>(-1);
		error = MAPISendMailPtr(0, 0, &message, MAPI_LOGON_UI | (showDialog ? MAPI_DIALOG : 0), 0);
		delete[] filePath;
	} else {	// 本文として送信するとき
		wchar_t* const content = new wchar_t[length_ + 1];
		for(length_t i = 0, offset = 0; ; ++i) {
			const Line& line = *lines_[i];
			wmemcpy(content + offset, line.text_.data(), line.text_.length());
			offset += line.text_.length();
			if(i != lines_.getSize() - 1) {
				wcscpy(content + offset, getNewlineString(line.newline_));
				offset += getNewlineStringLength(line.newline_);
			} else
				break;
		}
		content[length_] = 0;

		// ユーザ既定のマルチバイト文字列に変換
		const int contentSize = ::WideCharToMultiByte(CP_ACP, 0, content, static_cast<int>(length_), 0, 0, 0, 0);
		char* const	nativeContent = new char[contentSize + 1];
		::WideCharToMultiByte(CP_ACP, 0, content, static_cast<int>(length_), nativeContent, contentSize, 0, 0);
		nativeContent[contentSize] = 0;
		message.lpszNoteText = nativeContent;
		delete[] content;
		error = MAPISendMailPtr(0, 0, &message, MAPI_LOGON_UI | (showDialog ? MAPI_DIALOG : 0), 0);
		delete[] nativeContent;
	}

	::FreeLibrary(dll);
	return error == SUCCESS_SUCCESS || error == MAPI_USER_ABORT || error == MAPI_E_LOGIN_FAILURE;
}
#endif /* 0 */

/**
 * Sets the new document input.
 * @param newInput the new document input. can be @c null
 * @param delegateOwnership set true to transfer the ownership into the callee
 */
void Document::setInput(IDocumentInput* newInput, bool delegateOwnership) throw() {
	input_.reset(newInput, delegateOwnership);
}

/**
 * Sets the modification flag of the document.
 * @param modified set true to make the document modfied
 * @see #isModified, IDocumentStateListener#documentModificationSignChanged
 */
void Document::setModified(bool modified /* = true */) throw() {
	if(modified != modified_) {
		modified_ = modified;
		stateListeners_.notify<Document&>(&IDocumentStateListener::documentModificationSignChanged, *this);
	}
}

/**
 * Sets the new document partitioner.
 * @param newPartitioner the new partitioner. the ownership will be transferred to the callee
 */
void Document::setPartitioner(auto_ptr<DocumentPartitioner> newPartitioner) throw() {
	partitioner_ = newPartitioner;
	if(partitioner_.get() != 0)
		partitioner_->install(*this);
	partitioningChanged(region());
}

/**
 * Associates the given property with the document.
 * @param key the key of the property
 * @param property the property value
 * @see #property
 */
void Document::setProperty(const DocumentPropertyKey& key, const String& property) {
	std::map<const DocumentPropertyKey*, String*>::iterator i(properties_.find(&key));
	if(i == properties_.end())
		properties_.insert(make_pair(&key, new String(property)));
	else
		i->second->assign(property);
	stateListeners_.notify<Document&, const DocumentPropertyKey&>(&IDocumentStateListener::documentPropertyChanged, *this, key);
}

/**
 * Makes the document read only or not.
 * @see ReadOnlyDocumentException, #isReadOnly
 */
void Document::setReadOnly(bool readOnly /* = true */) {
	if(readOnly != isReadOnly()) {
		readOnly_ = readOnly;
		stateListeners_.notify<Document&>(&IDocumentStateListener::documentReadOnlySignChanged, *this);
	}
}

#if 0
/**
 * Translates the special Win32 code page to concrete one.
 * @param cp the code page to be translated
 * @return the concrete code page
 * @deprecated 0.8
 */
::UINT Document::translateSpecialCodePage(::UINT codePage) {
	if(codePage == CP_ACP)
		return ::GetACP();
	else if(codePage == CP_OEMCP)
		return ::GetOEMCP();
	else if(codePage == CP_MACCP) {
		wchar_t	wsz[7];
		::GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IDEFAULTMACCODEPAGE, wsz, 6);
		return (wcscmp(wsz, L"2") != 0) ? wcstoul(wsz, 0, 10) : 0;
	} else if(codePage == CP_THREAD_ACP) {
		wchar_t	wsz[7];
		::GetLocaleInfoW(::GetThreadLocale(), LOCALE_IDEFAULTANSICODEPAGE, wsz, 6);
		return (wcscmp(wsz, L"3") != 0) ? wcstoul(wsz, 0, 10) : 0;
	}
	return codePage;
}
#endif /* 0 */

/**
 * @brief アンドゥの実行
 *
 * 操作対象がアクセス不能であればリドゥは行われない
 * @return アンドゥできなかった場合は false を返す
 * @throw ReadOnlyDocumentException the document is read only
 * @see #redo
 */
bool Document::undo() {
	if(isReadOnly())
		throw ReadOnlyDocumentException();
	else if(numberOfUndoableEdits() == 0)
		return false;

	beginSequentialEdit();
	sequentialEditListeners_.notify<Document&>(
		&ISequentialEditListener::documentUndoSequenceStarted, *this);
	Position resultPosition;
	const pair<bool, size_t> status(undoManager_->undo(resultPosition));
	sequentialEditListeners_.notify<Document&, const Position&>(
		&ISequentialEditListener::documentUndoSequenceStopped, *this, resultPosition);
	endSequentialEdit();

	revisionNumber_ -= status.second * 2;
	return status.first;
}

/**
 * Informs the document change to the adapting points.
 * @param change the document change
 */
inline void Document::updatePoints(const DocumentChange& change) throw() {
	for(set<Point*>::iterator i = points_.begin(); i != points_.end(); ++i) {
		if((*i)->adaptsToDocument())
			(*i)->update(change);
	}
}

/**
 * Revokes the narrowing.
 * @see #isNarrowed, #narrow
 */
void Document::widen() {
	if(accessibleArea_ != 0) {
		delete accessibleArea_->second;
		delete accessibleArea_;
		accessibleArea_ = 0;
		stateListeners_.notify<Document&>(&IDocumentStateListener::documentAccessibleRegionChanged, *this);
	}
}


// DocumentCharacterIterator ////////////////////////////////////////////////

/**
 * @class ascension::text::DocumentCharacterIterator
 * Bidirectional iterator scans characters in the specified document.
 *
 * @c #current implementation of this class returns a character at which the iterator addresses. A
 * returned character is as a UTF-32 code unit (not UTF-16). In the following cases, returns a
 * special value depending on the context:
 *
 * - @c CharacterIterator#DONE at the end of the region of the iterator
 * - @c LINE_SEPARATOR at the end of the line
 * - a raw code unit value at any unpaired surrogate
 *
 * This class can't detect any change of the document. When the document changed, the existing
 * iterators may be invalid.
 *
 * @note This class is not intended to be subclassed.
 */

const CharacterIterator::ConcreteTypeTag DocumentCharacterIterator::CONCRETE_TYPE_TAG_ = CharacterIterator::ConcreteTypeTag();

/// Default constructor.
DocumentCharacterIterator::DocumentCharacterIterator() throw() : CharacterIterator(CONCRETE_TYPE_TAG_), document_(0), line_(0) {
}

/**
 * Constructor. The iteration region is the accessible area of the document.
 * @param document the document to iterate
 * @param position the position at which the iteration starts
 * @throw BadPositionException @a position is outside of the accessible area of the document
 */
DocumentCharacterIterator::DocumentCharacterIterator(const Document& document, const Position& position) :
		CharacterIterator(CONCRETE_TYPE_TAG_), document_(&document),
		region_(document.region()), line_(&document.line(position.line)), p_(position) {
	if(!region_.includes(p_))
		throw BadPositionException();
}

/**
 * Constructor. The iteration is started at @a region.beginning().
 * @param document the document to iterate
 * @param region the region to iterate
 * @throw BadRegionException @a region intersects outside of the document
 */
DocumentCharacterIterator::DocumentCharacterIterator(const Document& document, const Region& region) :
		CharacterIterator(CONCRETE_TYPE_TAG_), document_(&document), region_(region),
		line_(&document.line(region.beginning().line)), p_(region.beginning()) {
	region_.normalize();
	if(!document.region().encompasses(region_))
		throw BadRegionException();
}

/**
 * Constructor.
 * @param document the document to iterate
 * @param region the region to iterate
 * @param position the position at which the iteration starts
 * @throw BadRegionException @a region intersects outside of the document
 * @throw BadPositionException @a position is outside of @a region
 */
DocumentCharacterIterator::DocumentCharacterIterator(const Document& document, const Region& region, const Position& position) :
		CharacterIterator(CONCRETE_TYPE_TAG_), document_(&document), region_(region), line_(&document.line(position.line)), p_(position) {
	region_.normalize();
	if(!document.region().encompasses(region_))
		throw BadRegionException();
	else if(!region_.includes(p_))
		throw BadPositionException();
}

/// Copy-constructor.
DocumentCharacterIterator::DocumentCharacterIterator(const DocumentCharacterIterator& rhs) throw() :
		text::CharacterIterator(rhs), document_(rhs.document_), region_(rhs.region_), line_(rhs.line_), p_(rhs.p_) {
}

/// @see text#CharacterIterator#current
CodePoint DocumentCharacterIterator::current() const throw() {
	if(p_ == region_.second)
		return DONE;
	else if(p_.column == line_->length())
		return LINE_SEPARATOR;
	else
		return (surrogates::isHighSurrogate((*line_)[p_.column])
			&& p_.column + 1 < line_->length() && surrogates::isLowSurrogate((*line_)[p_.column + 1])) ?
			surrogates::decode((*line_)[p_.column], (*line_)[p_.column + 1]) : (*line_)[p_.column];
}

/// @see text#CharacterIterator#doAssign
void DocumentCharacterIterator::doAssign(const CharacterIterator& rhs) {
	CharacterIterator::operator=(rhs);
	const DocumentCharacterIterator& r = static_cast<const DocumentCharacterIterator&>(rhs);
	document_ = r.document_;
	line_ = r.line_;
	p_ = r.p_;
	region_ = r.region_;
}

/// @see text#CharacterIterator#doClone
auto_ptr<CharacterIterator> DocumentCharacterIterator::doClone() const {
	return auto_ptr<CharacterIterator>(new DocumentCharacterIterator(*this));
}

/// @see text#CharacterIterator#doFirst
void DocumentCharacterIterator::doFirst() {
	seek(region_.first);
}

/// @see text#CharacterIterator#doLast
void DocumentCharacterIterator::doLast() {
	seek(region_.second);
}

/// @see text#CharacterIterator#doEquals
bool DocumentCharacterIterator::doEquals(const CharacterIterator& rhs) const {
	return p_ == static_cast<const DocumentCharacterIterator&>(rhs).p_;
}

/// @see text#CharacterIterator#doLess
bool DocumentCharacterIterator::doLess(const CharacterIterator& rhs) const {
	return p_ < static_cast<const DocumentCharacterIterator&>(rhs).p_;
}

/// @see text#CharacterIterator#doNext
void DocumentCharacterIterator::doNext() {
	if(!hasNext())
//		throw out_of_range("the iterator is at the last.");
		return;
	else if(p_.column == line_->length()) {
		line_ = &document_->line(++p_.line);
		p_.column = 0;
	} else if(++p_.column < line_->length()
			&& surrogates::isLowSurrogate((*line_)[p_.column]) && surrogates::isHighSurrogate((*line_)[p_.column - 1]))
		++p_.column;
}

/// @see text#CharacterIterator#doPrevious
void DocumentCharacterIterator::doPrevious() {
	if(!hasPrevious())
//		throw out_of_range("the iterator is at the first.");
		return;
	else if(p_.column == 0)
		p_.column = (line_ = &document_->line(--p_.line))->length();
	else if(--p_.column > 0 && surrogates::isLowSurrogate((*line_)[p_.column]) && surrogates::isHighSurrogate((*line_)[p_.column - 1]))
		--p_.column;
}


// DocumentBuffer ///////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param document the document
 * @param initialPosition the initial position of streams
 * @param nlr the newline representation
 * @param streamMode the streaming mode. this can be @c std#ios_base#in and @c std#ios_base#out
 * @throw std#invalid_argument @a mode is invalid
 */
DocumentBuffer::DocumentBuffer(Document& document, const Position& initialPosition /* = Position::ZERO_POSITION */,
		NewlineRepresentation nlr /* = NLR_PHYSICAL_DATA */, ios_base::openmode streamMode /* = ios_base::in | ios_base::out */) :
		document_(document), nlr_(nlr), mode_(streamMode), current_(initialPosition) {
	if((mode_ & ~(ios_base::in | ios_base::out)) != 0)
		throw invalid_argument("the given mode is invalid.");
	setp(buffer_, endof(buffer_) - 1);
}

/// Destructor.
DocumentBuffer::~DocumentBuffer() throw() {
	sync();
}

/// Returns the current position in the document.
const Position& DocumentBuffer::tell() const throw() {
	return current_;
}

/// @see std#basic_streambuf#overflow
DocumentBuffer::int_type DocumentBuffer::overflow(int_type c) {
	if((mode_ & ios_base::out) == 0)
		return traits_type::eof();
	char_type* p = pptr();
	if(!traits_type::eq_int_type(c, traits_type::eof()))
		*p++ = traits_type::to_char_type(c);
	setp(buffer_, endof(buffer_) - 1);
	if(buffer_ < p)
		current_ = document_.insert(current_, buffer_, p);
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


// NullPartitioner //////////////////////////////////////////////////////////

/// Constructor.
NullPartitioner::NullPartitioner() throw() : p_(DEFAULT_CONTENT_TYPE, Region(Position::ZERO_POSITION, Position::INVALID_POSITION)) {
}

/// @see DocumentPartitioner#documentAboutToBeChanged
void NullPartitioner::documentAboutToBeChanged() throw() {
}

/// @see DocumentPartitioner#documentChanged
void NullPartitioner::documentChanged(const DocumentChange&) throw() {
	p_.region.second.line = INVALID_INDEX;
}

/// @see DocumentPartitioner#doGetPartition
void NullPartitioner::doGetPartition(const Position&, DocumentPartition& partition) const throw() {
	if(p_.region.second.line == INVALID_INDEX)
		const_cast<NullPartitioner*>(this)->p_.region.second = document()->region().second;
	partition = p_;
}

/// @see DocumentPartitioner#doInstall
void NullPartitioner::doInstall() throw() {
	p_.region.second.line = INVALID_INDEX;
}

// files.TextFileStreamBuffer ///////////////////////////////////////////////

namespace {
	class SystemErrorSaver {
	public:
#ifdef ASCENSION_WINDOWS
		SystemErrorSaver() throw() : e_(::GetLastError()) {}
		~SystemErrorSaver() throw() {::SetLastError(e_);}
	private:
		::DWORD e_;
#else // ASCENSION_POSIX
		SystemErrorSaver() throw() : e_(errno) {}
		~SystemErrorSaver() throw() {errno = e_;}
	private:
		int e_;
#endif
	};
} // namespace @0

/**
 * Constructor opens the specified file.
 * @param fileName the name of the file
 * @param mode the file open mode. must be either @c std#ios_base#in or @c std#ios_base#out
 * @param encoding the MIBenum value of the file encoding or auto detection identifier
 * @param encodingPolicy the policy about encoding conversion
 * @param writeByteOrderMark set true to write Unicode byte order mark into the output file
 * @throw IOException any I/O error occured
 */
files::TextFileStreamBuffer::TextFileStreamBuffer(const files::String& fileName, ios_base::openmode mode,
		const encoding::MIBenum encoding, encoding::Encoder::Policy encodingPolicy, bool writeByteOrderMark) :
		encoder_(encoding::Encoder::forMIB(encoding)), encodingError_(encoding::Encoder::COMPLETED) {
	using namespace ascension::encoding;
	if(!fileExists(fileName.c_str()))
		throw IOException(IOException::FILE_NOT_FOUND);
	inputMapping_.first = inputMapping_.last = inputMapping_.current = 0;
	if(mode == ios_base::in) {
		EncodingDetector* encodingDetector = 0;
		if(encoder_ == 0) {	// 'encoding' may be for auto-detection
			encodingDetector = EncodingDetector::forID(encoding);
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
			const ::DWORD e = ::GetLastError();
			throw IOException((e == ERROR_PATH_NOT_FOUND || e == ERROR_INVALID_NAME || e == ERROR_INVALID_PARAMETER
				|| e == ERROR_BAD_NETPATH) ? IOException::FILE_NOT_FOUND : IOException::PLATFORM_DEPENDENT_ERROR);
		}
		if(0 != (fileMapping_ = ::CreateFileMappingW(fileHandle_, 0, PAGE_READONLY, 0, 0, 0)))
			inputMapping_.first = static_cast<const ::uchar*>(::MapViewOfFile(fileMapping_, FILE_MAP_READ, 0, 0, 0));
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
		if(MAP_FAILED == (inputMapping_.first = static_cast<const uchar*>(::mmap(0, fileSize, PROT_READ, MAP_PRIVATE, fileDescriptor_, 0)))) {
			SystemErrorSaver temp;
			inputMapping_.first = 0;
			::close(fileDescriptor_);
			throw IOException(IOException::PLATFORM_DEPENDENT_ERROR);
		}
#endif
		inputMapping_.last = inputMapping_.first + fileSize;
		// detect input encoding if neccssary
		if(encodingDetector != 0) {
			encoder_ = Encoder::forMIB(encodingDetector->detect(
				inputMapping_.first, min(inputMapping_.last, inputMapping_.first + 1024 * 10), 0));
			if(encoder_ == 0)
				throw IOException(IOException::INVALID_ENCODING);	// can't resolve
		}
		// skip Unicode byte order mark if necessary
		const uchar* bom = 0;
		ptrdiff_t bomLength = 0;
		switch(encoder_->mibEnum()) {
		case fundamental::UTF_8:	bom = UTF8_BOM; bomLength = countof(UTF8_BOM); break;
		case fundamental::UTF_16LE:	bom = UTF16LE_BOM; bomLength = countof(UTF16LE_BOM); break;
		case fundamental::UTF_16BE:	bom = UTF16BE_BOM; bomLength = countof(UTF16BE_BOM); break;
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
		case extended::UTF_32LE:	bom = UTF32LE_BOM; bomLength = countof(UTF32LE_BOM); break;
		case extended::UTF_32BE:	bom = UTF32BE_BOM; bomLength = countof(UTF32BE_BOM); break;
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
		}
		if(bomLength >= fileSize && memcmp(inputMapping_.first, bom, bomLength) == 0)
			inputMapping_.first += bomLength;
		inputMapping_.current = inputMapping_.first;
	} else if(mode == ios_base::out) {
		if(encoder_ == 0)
			throw IOException(IOException::INVALID_ENCODING);
#ifdef ASCENSION_WINDOWS
		if(INVALID_HANDLE_VALUE == (fileHandle_ =
				::CreateFileW(fileName.c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, 0)))
			throw IOException(IOException::PLATFORM_DEPENDENT_ERROR);
#else // ASCENSION_POSIX
		if(-1 == (fileDescriptor_ = ::open(fileName.c_str(), O_WRONLY | O_CREAT)))
			throw IOException(IOException::PLATFORM_DEPENDENT_ERROR);
#endif
		if(writeByteOrderMark) {
			const uchar* bom = 0;
			size_t bomLength;
			switch(encoder_->mibEnum()) {
			case encoding::fundamental::UTF_8:
				bom = encoding::UTF8_BOM; bomLength = countof(encoding::UTF8_BOM); break;
			case encoding::fundamental::UTF_16LE:
				bom = encoding::UTF16LE_BOM; bomLength = countof(encoding::UTF16LE_BOM); break;
			case encoding::fundamental::UTF_16BE:
				bom = encoding::UTF16BE_BOM; bomLength = countof(encoding::UTF16BE_BOM); break;
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
			case encoding::extended::UTF_32LE:
				bom = encoding::UTF32LE_BOM; bomLength = countof(encoding::UTF32LE_BOM); break;
			case encoding::extended::UTF_32BE:
				bom = encoding::UTF32BE_BOM; bomLength = countof(encoding::UTF32BE_BOM); break;
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
			}
#ifdef ASCENSION_WINDOWS
			::WriteFile(fileHandle_, bom, static_cast<::DWORD>(bomLength), 0, 0);
#else // ASCENSION_POSIX
			::write(fileDescriptor_, bom, bomLength);
#endif
		}
	} else
		throw invalid_argument("the mode must be either std.ios_base.in or std.ios_base.out.");
	encoder_->setPolicy(encodingPolicy);
}

/// Destructor.
files::TextFileStreamBuffer::~TextFileStreamBuffer() {
	close();
}

/**
 * Closes the file.
 * @return this or @c null if the file is not open
 */
files::TextFileStreamBuffer* files::TextFileStreamBuffer::close() {
	sync();
#ifdef ASCENSION_WINDOWS
	if(inputMapping_.first != 0) {
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
		::munmap(const_cast<uchar*>(inputMapping_.first), inputMapping_.last - inputMapping_.first);
		inputMapping_.first = 0;
	}
	if(fileDescriptor_ == -1) {
		::close(fileDescriptor_);
		fileDescriptor_ = -1;
		return this;
	}
#endif
	return 0;
}

/// Returns the MIBenum value of the encoding.
encoding::MIBenum files::TextFileStreamBuffer::encoding() const throw() {
	return encoder_->mibEnum();
}

/// Returns true if the file is open.
bool files::TextFileStreamBuffer::isOpen() const throw() {
#ifdef ASCENSION_WINDOWS
	return fileHandle_ != INVALID_HANDLE_VALUE;
#else // ASCENSION_POSIX
	return fileDescriptor_ != -1;
#endif
}

/// Returns the result of the previous encoding conversion.
encoding::Encoder::Result files::TextFileStreamBuffer::lastEncodingError() const throw() {
	return encodingError_;
}

/// @see std#basic_streambuf#overflow
files::TextFileStreamBuffer::int_type files::TextFileStreamBuffer::overflow(int_type c) {
	return traits_type::eof();
}

/// @see std#basic_streambuf#pbackfail
files::TextFileStreamBuffer::int_type files::TextFileStreamBuffer::pbackfail(int_type c) {
	if(inputMapping_.first != 0) {
		if(gptr() > eback()) {
			gbump(-1);
			return traits_type::not_eof(c);	// c is ignored
		}
	}
	return traits_type::eof();
}

/// @see std#basic_streambuf#showmanyc
streamsize files::TextFileStreamBuffer::showmanyc() {
	return static_cast<streamsize>(gptr() - egptr());
}

/// std#basic_streambuf#sync
int files::TextFileStreamBuffer::sync() {
	return 0;
}

/// @see std#basic_streambuf#underflow
files::TextFileStreamBuffer::int_type files::TextFileStreamBuffer::underflow() {
	if(inputMapping_.first == 0)
		return traits_type::eof();	// not input mode
	ascension::Char* toNext;
	const uchar* fromNext;
	if((encodingError_ = encoder_->toUnicode(ucsBuffer_, endof(ucsBuffer_), toNext,
			inputMapping_.current, inputMapping_.last, fromNext, &encodingState_)) == encoding::Encoder::COMPLETED)
		inputMapping_.current = inputMapping_.last;
	else
		inputMapping_.current = fromNext;
	if(encodingError_ == encoding::Encoder::INSUFFICIENT_BUFFER)
		encodingError_ = encoding::Encoder::COMPLETED;
	setg(ucsBuffer_, ucsBuffer_, toNext);
	return (toNext > ucsBuffer_) ? *gptr() : traits_type::eof();
}


// files.FileBinder /////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param document the document
 */
files::FileBinder::FileBinder(Document& document) : document_(document),
		encoding_(encoding::Encoder::getDefault()), newline_(ASCENSION_DEFAULT_NEWLINE), lockingFile_(
#ifdef ASCENSION_WINDOWS
		INVALID_HANDLE_VALUE
#else // ASCENSION_POSIX
		-1
#endif
		), savedDocumentRevision_(0), timeStampDirector_(0) {
	lockMode_.type = LockMode::DONT_LOCK;
	lockMode_.onlyAsEditing = true;
	memset(&userLastWriteTime_, 0, sizeof(Time));
	memset(&internalLastWriteTime_, 0, sizeof(Time));
	document.addListener(*this);
}

/// Destructor.
files::FileBinder::~FileBinder() throw() {
	unlock();
	document_.removeListener(*this);
	if(isBound())
		document_.setInput(0, false);
}

/**
 * Binds the document to the specified file. This method call document's @c Document#setInput.
 * @param fileName the file name. this method doesn't resolves the short cut
 * @param lockMode the lock mode. this method may fail to lock with desired mode. see the
 * description of the return value
 * @param encoding the MIBenum value of the file encoding or auto detection identifier
 * @param encodingPolicy the policy about encoding conversion
 * @param unexpectedTimeStampDirector
 * @return true if succeeded to lock the file with the desired mode @a lockMode.type
 * @throw IOException any I/O error occured. in this case, the document's content will be lost
 */
bool files::FileBinder::bind(const files::String& fileName, const LockMode& lockMode, encoding::MIBenum encoding,
		encoding::Encoder::Policy encodingPolicy, IUnexpectedFileTimeStampDirector* unexpectedTimeStampDirector /* = 0 */) {
//	Timer tm(L"FileBinder.bind");	// 2.86s / 1MB
	unlock();
	document_.resetContent();
	timeStampDirector_ = 0;

	// read from the file
	TextFileStreamBuffer sb(fileName, ios_base::in, encoding, encodingPolicy, false);
	const bool recorded = document_.isRecordingOperations();
	document_.recordOperations(false);
	try {
		InputStream in(&sb);
		readDocumentFromStream(in, document_, document_.region().beginning());
	} catch(...) {
		document_.recordOperations(recorded);
		throw;
	}
	document_.recordOperations(recorded);
	sb.close();

	// lock the file
	bool lockSucceeded = true;
	lockMode_ = lockMode;
	if(lockMode_.type != LockMode::DONT_LOCK && !lockMode_.onlyAsEditing) {
		if(!(lockSucceeded = lock())) {
			if(lockMode_.type == LockMode::EXCLUSIVE_LOCK) {
				lockMode_.type = LockMode::SHARED_LOCK;
				if(!lock())
					lockMode_.type = LockMode::DONT_LOCK;
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
	// TODO: convert name() into the 8-bit file system encoding.
#endif
	encoding_ = sb.encoding();
	newline_ = document_.getLineInformation(0).newline();	// use the newline of the first line

	document_.clearUndoBuffer();
	document_.setModified(false);

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
 * Checks the last modified date/time of the bound file and verifies if the other modified the
 * file. If the file is modified, the listener's
 * @c IUnexpectedFileTimeStampDerector#queryAboutUnexpectedDocumentFileTimeStamp will be called.
 * @return the value which the listener returned or true if the listener is not set
 */
bool files::FileBinder::checkTimeStamp() {
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

/// @see IDocumentListener#documentAboutToBeChanged
bool files::FileBinder::documentAboutToBeChanged(const Document&, const DocumentChange&) {
	// check the time stamp is this is the first modification
	if(timeStampDirector_ != 0 && !document_.isModified()) {
		Time realTimeStamp;
		if(!verifyTimeStamp(true, realTimeStamp)) {	// the other overwritten the file
			if(!timeStampDirector_->queryAboutUnexpectedDocumentFileTimeStamp(
					document_, IUnexpectedFileTimeStampDirector::FIRST_MODIFICATION))
				return false;
			internalLastWriteTime_ = userLastWriteTime_ = realTimeStamp;
		}
	}
	return true;
}

/// @see IDocumentListener#documentChanged
void files::FileBinder::documentChanged(const Document&, const DocumentChange&) {
}

/// @see IDocumentStateListener#documentModificationSignChanged
void files::FileBinder::documentModificationSignChanged(Document&) {
	if(lockMode_.onlyAsEditing && isBound()) {
		if(document_.isModified())
			lock();
		else
			unlock();
	}
}

/// @see IDocumentInput#encoding, #setEncoding
encoding::MIBenum files::FileBinder::encoding() const throw() {
	return encoding_;
}

/// Returns the file extension name or @c null if the document is not bound to any of the files.
files::String files::FileBinder::extensionName() const throw() {
	const files::String s(pathName());
	const files::String::size_type dot = s.rfind('.');
	return (dot != files::String::npos) ? s.substr(dot + 1) : files::String();
}

/**
 * Locks the file.
 * @return true if locked successfully or the lock mode is @c LockMode#DONT_LOCK
 */
bool files::FileBinder::lock() throw() {
	unlock();
	if(lockMode_.type != LockMode::DONT_LOCK && isBound()) {
#ifdef ASCENSION_WINDOWS
		assert(lockingFile_ == INVALID_HANDLE_VALUE);
		lockingFile_ = ::CreateFileW(fileName_.c_str(), GENERIC_READ,
			(lockMode_.type == LockMode::SHARED_LOCK) ? FILE_SHARE_READ : 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
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
		fl.l_type = (lockMode_.type == LockMode::SHARED_LOCK) ? F_RDLCK : F_WRLCK;
		if(::fcntl(lockingFile_, F_SETLK, &fl) == 0) {
			::close(lockingFile_);
			lockingFile_ = -1;
		}
#endif
	}
	return true;
}

/// Returns the file name or an empty string if the document is not bound to any of the files.
files::String files::FileBinder::name() const throw() {
	const String::const_iterator p = findFileName(fileName_);
	return fileName_.substr(p - fileName_.begin());
}

/// @see IDocumentInput#newline, #setNewline
Newline files::FileBinder::newline() const throw() {
	return newline_;
}

/**
 * Sets the encoding.
 * @param mib the MIBenum value of the encoding
 * @throw std#invalid_argument @a mib is invalid
 * @see #encoding
 */
void files::FileBinder::setEncoding(encoding::MIBenum mib) {
	if(!encoding::Encoder::supports(mib))
		throw invalid_argument("the given encoding is not available.");
	encoding_ = mib;
}

/**
 * Sets the newline.
 * @param newline the newline
 * @throw std#invalid_argument @a newline is invalid
 */
void files::FileBinder::setNewline(Newline newline) {
	if(newline == NLF_LF || newline == NLF_CR || newline == NLF_CRLF
			|| newline == NLF_NEL || newline == NLF_LS || newline == NLF_PS)
		newline_ = newline;
	else
		throw invalid_argument("unknown newline specified.");
}

/**
 * Unlocks the file.
 * @return succeeded or not
 */
bool files::FileBinder::unlock() throw() {
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
bool files::FileBinder::verifyTimeStamp(bool internal, Time& newTimeStamp) throw() {
	static Time uninitialized;
	static bool initializedUninitialized = false;
	if(!initializedUninitialized)
		memset(&uninitialized, 0, sizeof(Time));

	const Time& about = internal ? internalLastWriteTime_ : userLastWriteTime_;
	if(!isBound()
			|| memcmp(&about, &uninitialized, sizeof(Time)) == 0
			|| lockMode_.type != LockMode::DONT_LOCK)
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
 * @param params the options. set @a newline member of this object to @c NLF_AUTO not to unify the
 * line breaks
 * @return the result. @c FIR_OK if succeeded
 */
bool files::FileBinder::write(const files::String& fileName, const files::FileBinder::WriteParameters& params) {
	// check Unicode newlines
	if(params.newline == NLF_NEL || params.newline == NLF_LS || params.newline == NLF_PS) {
		if(params.encoding != encoding::fundamental::UTF_8
				&& params.encoding != encoding::fundamental::UTF_16LE
				&& params.encoding != encoding::fundamental::UTF_16BE
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
				&& params.encoding != encoding::extended::UTF_5
				&& params.encoding != encoding::extended::UTF_7
				&& params.encoding != encoding::extended::UTF_32LE
				&& params.encoding != encoding::extended::UTF_32BE
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
			)
			throw IOException(IOException::INVALID_NEWLINE);
	}

	// check if writable
#ifdef ASCENSION_WINDOWS
	const ::DWORD originalAttributes = ::GetFileAttributesW(fileName.c_str());
	if(originalAttributes != INVALID_FILE_ATTRIBUTES && toBoolean(originalAttributes & FILE_ATTRIBUTE_READONLY))
		throw IOException(IOException::UNWRITABLE_FILE);
#else // ASCENSION_POSIX
	struct stat originalStat;
	bool gotStat = false;
	if(::stat(fileName_.c_str(), &originalStat) == 0) {
		gotStat = true;
		// TODO: check the permission.
	}
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
	const files::String tempFileName(getTemporaryFileName(realName));
	TextFileStreamBuffer sb(tempFileName, ios_base::out, params.encoding,
		params.encodingPolicy, params.options.has(WriteParameters::WRITE_UNICODE_BYTE_ORDER_SIGNATURE));
	basic_ostream<ascension::Char> outputStream(&sb);
	writeDocumentToStream(outputStream, document_, document_.region(),
		(params.newline != NLF_AUTO) ? getNewlineString(params.newline) : L"");
	sb.close();

	// copy file attributes (file mode) and delete the old file
	unlock();
#ifdef ASCENSION_WINDOWS
	if(originalAttributes != INVALID_FILE_ATTRIBUTES) {
		::SetFileAttributesW(tempFileName.c_str(), originalAttributes);
		if(!toBoolean(::DeleteFileW(realName.c_str()))) {
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
		if(::remove(realName.c_str()) != 0) {
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
/*	// デバッグバージョンは常にバックアップを作る (上書きの場合のみ)
	if(
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
		::SHFileOperationW(&shfos);	// ごみ箱に持っていく
	}
*/

	if(params.newline != NLF_AUTO) {
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
	document_.setModified(false);
	document_.setReadOnly(false);
	setEncoding(params.encoding);

	// update the internal time stamp
	try {
		getFileLastWriteTime(fileName_ = realName, internalLastWriteTime_);
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
 * @param params the options. set @a newline member of this object to @c NLF_AUTO not to unify the
 * newlines
 * @param append true to append to the file
 * @param callback the callback or @c null
 * @return the result. @c FIR_OK if succeeded
 */
bool files::FileBinder::writeRegion(const files::String& fileName, const Region& region, const files::FileBinder::WriteParameters& params, bool append) {
	// TODO: not implemented.
	return false;
}
