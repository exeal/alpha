// utility.hpp
// (c) 2004-2007 exeal

#ifndef MANAH_UTILITY_HPP
#define MANAH_UTILITY_HPP
#include "dc.hpp"

namespace manah {
namespace win32 {

// this header contains following classes:
class Point;
class Size;
class Rect;
class FileFind;


class Point : public ::tagPOINT {
public:
	// constructors
	Point() throw() {}
	Point(int xValue, int yValue) throw() {x = xValue, y = yValue;}
	Point(const ::POINT& pt) throw() {x = pt.x; y = pt.y;}
	Point(const ::SIZE& size) throw() {x = size.cx; y = size.cy;}
	Point(::DWORD dwPoint) throw() {x = LOWORD(dwPoint); y = HIWORD(dwPoint);}
	// methods
	void offset(int dx, int dy) throw() {x += dx; y += dy;}
	void offset(const ::POINT& pt) throw() {offset(pt.x, pt.y);}
	void offset(const ::SIZE& size) throw() {offset(size.cx, size.cy);}
};


class Size : public ::tagSIZE {
public:
	// constructors
	Size() throw() {}
	Size(int cxValue, int cyValue) throw() {cx = cxValue; cy = cyValue;}
	Size(const ::SIZE& size) throw() {cx = size.cx; cy = size.cy;}
	Size(::DWORD size) throw() {cx = LOWORD(size); cy = HIWORD(size);}
};


class Rect : public ::tagRECT {
public:
	// constructors
	Rect() throw() {}
	Rect(int l, int t, int r, int b) throw() {set(l, t, r, b);}
	Rect(const ::RECT& rect) throw() {copy(rect);}
	Rect(const ::POINT& pt, const ::SIZE& size) throw() {set(pt.x, pt.y, pt.x + size.cx, pt.y + size.cy);}
	Rect(const ::POINT& leftTop, const ::POINT& rightBottom) throw() {set(leftTop.x, leftTop.y, rightBottom.x, rightBottom.y);}
	// attributes
	void	copy(const ::RECT& other) {static_cast<::RECT&>(*this) = other;}
	bool	equals(const ::RECT& other) const throw() {return toBoolean(::EqualRect(this, &other));}
	Point	getCenter() const throw() {return Point(getWidth() / 2, getHeight() / 2);}
	long	getHeight() const throw() {return bottom - top;}
	Point	getLeftTop() const throw() {return Point(left, top);}
	Point	getRightBottom() const throw() {return Point(right, bottom);}
	Size	getSize() const throw() {return Size(getWidth(), getHeight());}
	long	getWidth() const throw() {return right - left;}
	bool	includes(const ::POINT& pt) const throw() {return toBoolean(::PtInRect(this, pt));}
	bool	isEmpty() const throw() {return toBoolean(::IsRectEmpty(this));}
	bool	isNull() const throw() {return left == 0 && top == 0 && right == 0 && bottom == 0;}
	void	set(int l, int t, int r, int b) throw() {::SetRect(this, l, t, r, b);}
	void	setEmpty() throw() {::SetRectEmpty(this);}
	// operations
	void	deflate(int x, int y) throw() {inflate(-x, -y);}
	void	deflate(const ::SIZE& size) throw() {inflate(-size.cx, -size.cy);}
	void	deflate(const ::RECT& rect) throw() {set(left + rect.left, top + rect.top, right - rect.right, bottom - rect.bottom);}
	void	deflate(int l, int t, int r, int b) throw() {set(left + l, top + t, right - r, bottom - b);}
	bool	getUnion(const ::RECT& rect1, const ::RECT& rect2) throw() {toBoolean(::UnionRect(this, &rect1, &rect2));}
	void	inflate(int x, int y) throw() {::InflateRect(this, x, y);}
	void	inflate(const ::SIZE& size) throw() {inflate(size.cx, size.cy);}
	void	inflate(const ::RECT& rect) throw() {set(left - rect.left, top - rect.top, right + rect.right, bottom + rect.bottom);}
	void	inflate(int l, int t, int r, int b) throw() {set(left - l, top - t, right + r, bottom + b);}
	bool	intersects(const ::RECT& rect1, const ::RECT& rect2) throw() {toBoolean(::IntersectRect(this, &rect1, &rect2));}
	void	normalize() throw() {if(top > bottom) std::swap(top, bottom); if(left > right) std::swap(left, right);}
	void	offset(int x, int y) throw() {::OffsetRect(this, x, y);}
	void	offset(const ::POINT& pt) throw() {offset(pt.x, pt.y);}
	void	offset(const ::SIZE& size) throw() {offset(size.cx, size.cy);}
	bool	subtract(const ::RECT& rect1, const ::RECT& rect2) throw() {return toBoolean(::SubtractRect(this, &rect1, &rect2));}
};


class FileFind {
public:
	// constructors
	FileFind() : find_(0), found_(false) {}
	~FileFind() {close();}
	// attributes
	void			getCreationTime(::FILETIME& timeStamp) const throw();
	std::wstring	getFileName() const throw();
	std::wstring	getFilePath() const;
	::ULONGLONG		getFileSize() const throw();
	std::wstring	getFileTitle() const;
	std::wstring	getFileUrl() const;
	void			getLastAccessTime(::FILETIME& timeStamp) const throw();
	void			getLastWriteTime(::FILETIME& timeStamp) const throw();
	std::wstring	getRoot() const throw();
	bool			isArchived() const throw();
	bool			isCompressed() const throw();
	bool			isDirectory() const throw();
	bool			isDots() const throw();
	bool			isHidden() const throw();
	bool			isNormal() const throw();
	bool			isReadOnly() const throw();
	bool			isSystem() const throw();
	bool			isTemporary() const throw();
	bool			matchesMask(::DWORD mask) const throw();
	// operations
	void	close();
	bool	find(const ::WCHAR* name = L"*.*");
	bool	findNext();

private:
	::HANDLE find_;
	::WIN32_FIND_DATAW wfd_;
	bool found_;
};


// FileFind /////////////////////////////////////////////////////////////////

inline void FileFind::close() {
	if(find_ != 0) {
		::FindClose(find_);
		find_ = 0;
		found_ = false;
	}
}

inline bool FileFind::find(const ::WCHAR* name /* = L"*.*" */) {
	close();
	assert(name != 0);
	assert(std::wcslen(name) < MAX_PATH);

	std::wcscpy(wfd_.cFileName, name);
	find_ = ::FindFirstFileW(name, &wfd_);
	if(find_ == INVALID_HANDLE_VALUE) {
		find_ = 0;
		return false;
	}
	found_ = true;
	return true;
}

inline bool FileFind::findNext() {
	if(find_ != 0 && found_)
		found_ = toBoolean(::FindNextFileW(find_, &wfd_));
	return found_;
}

inline void FileFind::getCreationTime(FILETIME& timeStamp) const throw() {assert(found_); timeStamp = wfd_.ftCreationTime;}

inline std::wstring FileFind::getFileName() const throw() {assert(found_); return wfd_.cFileName;}

inline std::wstring FileFind::getFilePath() const throw() {
	assert(found_);
	::WCHAR path[MAX_PATH];
	return (::_wfullpath(path, wfd_.cFileName, MAX_PATH) != 0) ? path : L"";
}

inline ::ULONGLONG FileFind::getFileSize() const throw() {
	assert(found_);
	::ULARGE_INTEGER size = {0};
	size.HighPart = wfd_.nFileSizeHigh;
	size.LowPart = wfd_.nFileSizeLow;
	return size.QuadPart;
}

inline std::wstring FileFind::getFileTitle() const {
	assert(found_);
	const std::wstring name(getFileName());
	if(!name.empty()) {
		::WCHAR title[MAX_PATH];
		::_wsplitpath(name.c_str(), 0, 0, title, 0);
		return title;
	}
	return L"";
}

inline std::wstring FileFind::getFileUrl() const {
	assert(found_);
	const std::wstring path(getFilePath());
	return !path.empty() ? (L"file://" + path) : L"";
}

inline void FileFind::getLastAccessTime(::FILETIME& timeStamp) const throw() {assert(found_); timeStamp = wfd_.ftLastAccessTime;}

inline void FileFind::getLastWriteTime(::FILETIME& timeStamp) const throw() {assert(found_); timeStamp = wfd_.ftLastWriteTime;}

inline std::wstring FileFind::getRoot() const throw() {
	assert(found_);
	::WCHAR path[MAX_PATH];
	return (::_wfullpath(path, wfd_.cFileName, MAX_PATH) != 0) ? path : L"";
}

inline bool FileFind::isArchived() const throw() {return matchesMask(FILE_ATTRIBUTE_ARCHIVE);}

inline bool FileFind::isCompressed() const throw() {return matchesMask(FILE_ATTRIBUTE_COMPRESSED);}

inline bool FileFind::isDirectory() const throw() {return matchesMask(FILE_ATTRIBUTE_DIRECTORY);}

inline bool FileFind::isDots() const throw() {
	return isDirectory() && (std::wcscmp(wfd_.cFileName, L".") == 0 || std::wcscmp(wfd_.cFileName, L"..") == 0);}

inline bool FileFind::isHidden() const throw() {return matchesMask(FILE_ATTRIBUTE_HIDDEN);}

inline bool FileFind::isNormal() const throw() {return matchesMask(FILE_ATTRIBUTE_NORMAL);}

inline bool FileFind::isReadOnly() const throw() {return matchesMask(FILE_ATTRIBUTE_READONLY);}

inline bool FileFind::isSystem() const throw() {return matchesMask(FILE_ATTRIBUTE_SYSTEM);}

inline bool FileFind::isTemporary() const throw() {return matchesMask(FILE_ATTRIBUTE_TEMPORARY);}

inline bool FileFind::matchesMask(::DWORD mask) const throw() {assert(found_); return toBoolean(wfd_.dwFileAttributes & mask);}

}} // namespace manah.win32

#endif /* !MANAH_UTILITY_HPP */
