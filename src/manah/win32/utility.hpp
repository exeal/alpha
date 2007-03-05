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
	Point(const SIZE& size) throw() {x = size.cx; y = size.cy;}
	Point(DWORD dwPoint) throw() {x = LOWORD(dwPoint); y = HIWORD(dwPoint);}
	// methods
	void offset(int dx, int dy) throw() {x += dx; y += dy;}
	void offset(const POINT& pt) throw() {offset(pt.x, pt.y);}
	void offset(const SIZE& size) throw() {offset(size.cx, size.cy);}
};


class Size : public ::tagSIZE {
public:
	// constructors
	Size() throw() {}
	Size(int cxValue, int cyValue) throw() {cx = cxValue; cy = cyValue;}
	Size(const ::SIZE& size) throw() {cx = size.cx; cy = size.cy;}
	Size(DWORD size) throw() {cx = LOWORD(size); cy = HIWORD(size);}
};


class Rect : public ::tagRECT {
public:
	// constructors
	Rect() throw() {}
	Rect(int l, int t, int r, int b) throw() {setRect(l, t, r, b);}
	Rect(const ::RECT& rect) throw() {copyRect(rect);}
	Rect(const ::POINT& pt, const ::SIZE& size) throw() {setRect(pt.x, pt.y, pt.x + size.cx, pt.y + size.cy);}
	Rect(const ::POINT& leftTop, const ::POINT& rightBottom) throw() {setRect(leftTop.x, leftTop.y, rightBottom.x, rightBottom.y);}
	// attributes
	void	copyRect(const ::RECT& other) {*this = other;}
	bool	equalRect(const ::RECT& other) const throw() {return toBoolean(::EqualRect(this, &other));}
	Point	getCenter() const throw() {return Point(getWidth() / 2, getHeight() / 2);}
	long	getHeight() const throw() {return bottom - top;}
	Point	getLeftTop() const throw() {return Point(left, top);}
	Point	getRightBottom() const throw() {return Point(right, bottom);}
	Size	getSize() const throw() {return Size(getWidth(), getHeight());}
	long	getWidth() const throw() {return right - left;}
	bool	isRectEmpty() const throw() {return toBoolean(::IsRectEmpty(this));}
	bool	isRectNull() const throw() {return left == 0 && top == 0 && right == 0 && bottom == 0;}
	bool	ptInRect(const ::POINT& pt) const throw() {return toBoolean(::PtInRect(this, pt));}
	void	setRect(int l, int t, int r, int b) throw() {::SetRect(this, l, t, r, b);}
	void	setRectEmpty() throw() {::SetRectEmpty(this);}
	// operations
	void	deflateRect(int x, int y) throw() {inflateRect(-x, -y);}
	void	deflateRect(const ::SIZE& size) throw() {inflateRect(-size.cx, -size.cy);}
	void	deflateRect(const ::RECT& rect) throw() {setRect(left + rect.left, top + rect.top, right - rect.right, bottom - rect.bottom);}
	void	deflateRect(int l, int t, int r, int b) throw() {setRect(left + l, top + t, right - r, bottom - b);}
	void	inflateRect(int x, int y) throw() {::InflateRect(this, x, y);}
	void	inflateRect(const ::SIZE& size) throw() {inflateRect(size.cx, size.cy);}
	void	inflateRect(const ::RECT& rect) throw() {setRect(left - rect.left, top - rect.top, right + rect.right, bottom + rect.bottom);}
	void	inflateRect(int l, int t, int r, int b) throw() {setRect(left - l, top - t, right + r, bottom + b);}
	bool	intersectRect(const ::RECT& rect1, const ::RECT& rect2) throw() {toBoolean(::IntersectRect(this, &rect1, &rect2));}
	void	normalizeRect() throw() {if(top > bottom) std::swap(top, bottom); if(left > right) std::swap(left, right);}
	void	offsetRect(int x, int y) throw() {::OffsetRect(this, x, y);}
	void	offsetRect(const ::POINT& pt) throw() {offsetRect(pt.x, pt.y);}
	void	offsetRect(const ::SIZE& size) throw() {offsetRect(size.cx, size.cy);}
	bool	subtractRect(const ::RECT& rect1, const ::RECT& rect2) throw() {return toBoolean(::SubtractRect(this, &rect1, &rect2));}
	bool	unionRect(const ::RECT& rect1, const ::RECT& rect2) throw() {toBoolean(::UnionRect(this, &rect1, &rect2));}
};


class FileFind {
public:
	// constructors
	FileFind() : find_(0), found_(false) {}
	~FileFind() {close();}
	// attributes
	void						getCreationTime(::FILETIME& timeStamp) const throw();
	std::basic_string<TCHAR>	getFileName() const throw();
	std::basic_string<TCHAR>	getFilePath() const;
	ULONGLONG					getFileSize() const throw();
	std::basic_string<TCHAR>	getFileTitle() const;
	std::basic_string<TCHAR>	getFileUrl() const;
	void						getLastAccessTime(::FILETIME& timeStamp) const throw();
	void						getLastWriteTime(::FILETIME& timeStamp) const throw();
	std::basic_string<TCHAR>	getRoot() const throw();
	bool						isArchived() const throw();
	bool						isCompressed() const throw();
	bool						isDirectory() const throw();
	bool						isDots() const throw();
	bool						isHidden() const throw();
	bool						isNormal() const throw();
	bool						isReadOnly() const throw();
	bool						isSystem() const throw();
	bool						isTemporary() const throw();
	bool						matchesMask(DWORD mask) const throw();
	// operations
	void	close();
	bool	find(const TCHAR* name = _T("*.*"));
	bool	findNext();

private:
	HANDLE find_;
	::WIN32_FIND_DATA wfd_;
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

inline bool FileFind::find(const TCHAR* name /* = _T("*.*") */) {
	close();
	assert(name != 0);
	assert(std::_tcslen(name) < MAX_PATH);

	std::_tcscpy(wfd_.cFileName, name);
	find_ = ::FindFirstFile(name, &wfd_);
	if(find_ == INVALID_HANDLE_VALUE) {
		find_ = 0;
		return false;
	}
	found_ = true;
	return true;
}

inline bool FileFind::findNext() {
	if(find_ != 0 && found_)
		found_ = toBoolean(::FindNextFile(find_, &wfd_));
	return found_;
}

inline void FileFind::getCreationTime(FILETIME& timeStamp) const throw() {assert(found_); timeStamp = wfd_.ftCreationTime;}

inline std::basic_string<TCHAR> FileFind::getFileName() const throw() {assert(found_); return wfd_.cFileName;}

inline std::basic_string<TCHAR> FileFind::getFilePath() const throw() {
	assert(found_);
	TCHAR path[MAX_PATH];
	return (::_tfullpath(path, wfd_.cFileName, MAX_PATH) != 0) ? path : _T("");
}

inline ULONGLONG FileFind::getFileSize() const throw() {
	assert(found_);
	::ULARGE_INTEGER size = {0};
	size.HighPart = wfd_.nFileSizeHigh;
	size.LowPart = wfd_.nFileSizeLow;
	return size.QuadPart;
}

inline std::basic_string<TCHAR> FileFind::getFileTitle() const {
	assert(found_);
	const std::basic_string<TCHAR> name = getFileName();
	if(!name.empty()) {
		TCHAR title[MAX_PATH];
		::_tsplitpath(name.c_str(), 0, 0, title, 0);
		return title;
	}
	return _T("");
}

inline std::basic_string<TCHAR> FileFind::getFileUrl() const {
	assert(found_);
	const std::basic_string<TCHAR> path = getFilePath();
	return !path.empty() ? (_T("file://") + path) : _T("");
}

inline void FileFind::getLastAccessTime(FILETIME& timeStamp) const throw() {assert(found_); timeStamp = wfd_.ftLastAccessTime;}

inline void FileFind::getLastWriteTime(FILETIME& timeStamp) const throw() {assert(found_); timeStamp = wfd_.ftLastWriteTime;}

inline std::basic_string<TCHAR> FileFind::getRoot() const throw() {
	assert(found_);
	TCHAR path[MAX_PATH];
	return (::_tfullpath(path, wfd_.cFileName, MAX_PATH) != 0) ? path : _T("");
}

inline bool FileFind::isArchived() const throw() {return matchesMask(FILE_ATTRIBUTE_ARCHIVE);}

inline bool FileFind::isCompressed() const throw() {return matchesMask(FILE_ATTRIBUTE_COMPRESSED);}

inline bool FileFind::isDirectory() const throw() {return matchesMask(FILE_ATTRIBUTE_DIRECTORY);}

inline bool FileFind::isDots() const throw() {
	return isDirectory() && (std::_tcscmp(wfd_.cFileName, _T(".")) == 0 || std::_tcscmp(wfd_.cFileName, _T("..")) == 0);}

inline bool FileFind::isHidden() const throw() {return matchesMask(FILE_ATTRIBUTE_HIDDEN);}

inline bool FileFind::isNormal() const throw() {return matchesMask(FILE_ATTRIBUTE_NORMAL);}

inline bool FileFind::isReadOnly() const throw() {return matchesMask(FILE_ATTRIBUTE_READONLY);}

inline bool FileFind::isSystem() const throw() {return matchesMask(FILE_ATTRIBUTE_SYSTEM);}

inline bool FileFind::isTemporary() const throw() {return matchesMask(FILE_ATTRIBUTE_TEMPORARY);}

inline bool FileFind::matchesMask(DWORD mask) const throw() {assert(found_); return toBoolean(wfd_.dwFileAttributes & mask);}

}} // namespace manah.win32

#endif /* !MANAH_UTILITY_HPP */
