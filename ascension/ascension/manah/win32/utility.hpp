// utility.hpp
// (c) 2004-2008 exeal

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
	Point(const POINT& pt) throw() {x = pt.x; y = pt.y;}
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
	Size(const SIZE& size) throw() {cx = size.cx; cy = size.cy;}
	Size(DWORD size) throw() {cx = LOWORD(size); cy = HIWORD(size);}
};


class Rect : public ::tagRECT {
public:
	// constructors
	Rect() throw() {}
	Rect(int l, int t, int r, int b) throw() {set(l, t, r, b);}
	Rect(const RECT& rect) throw() {copy(rect);}
	Rect(const POINT& pt, const SIZE& size) throw() {set(pt.x, pt.y, pt.x + size.cx, pt.y + size.cy);}
	Rect(const POINT& leftTop, const POINT& rightBottom) throw() {set(leftTop.x, leftTop.y, rightBottom.x, rightBottom.y);}
	// attributes
	void copy(const RECT& other) {static_cast<RECT&>(*this) = other;}
	bool equals(const RECT& other) const throw() {return toBoolean(::EqualRect(this, &other));}
	Point getCenter() const throw() {return Point(getWidth() / 2, getHeight() / 2);}
	long getHeight() const throw() {return bottom - top;}
	Point getLeftTop() const throw() {return Point(left, top);}
	Point getRightBottom() const throw() {return Point(right, bottom);}
	Size getSize() const throw() {return Size(getWidth(), getHeight());}
	long getWidth() const throw() {return right - left;}
	bool includes(const POINT& pt) const throw() {return toBoolean(::PtInRect(this, pt));}
	bool isEmpty() const throw() {return toBoolean(::IsRectEmpty(this));}
	bool isNull() const throw() {return left == 0 && top == 0 && right == 0 && bottom == 0;}
	void set(int l, int t, int r, int b) throw() {::SetRect(this, l, t, r, b);}
	void setEmpty() throw() {::SetRectEmpty(this);}
	// operations
	void deflate(int x, int y) throw() {inflate(-x, -y);}
	void deflate(const SIZE& size) throw() {inflate(-size.cx, -size.cy);}
	void deflate(const RECT& rect) throw() {set(left + rect.left, top + rect.top, right - rect.right, bottom - rect.bottom);}
	void deflate(int l, int t, int r, int b) throw() {set(left + l, top + t, right - r, bottom - b);}
	bool getUnion(const RECT& rect1, const RECT& rect2) throw() {return toBoolean(::UnionRect(this, &rect1, &rect2));}
	void inflate(int x, int y) throw() {::InflateRect(this, x, y);}
	void inflate(const SIZE& size) throw() {inflate(size.cx, size.cy);}
	void inflate(const RECT& rect) throw() {set(left - rect.left, top - rect.top, right + rect.right, bottom + rect.bottom);}
	void inflate(int l, int t, int r, int b) throw() {set(left - l, top - t, right + r, bottom + b);}
	bool intersects(const RECT& rect1, const RECT& rect2) throw() {return toBoolean(::IntersectRect(this, &rect1, &rect2));}
	void normalize() throw() {if(top > bottom) std::swap(top, bottom); if(left > right) std::swap(left, right);}
	void offset(int x, int y) throw() {::OffsetRect(this, x, y);}
	void offset(const POINT& pt) throw() {offset(pt.x, pt.y);}
	void offset(const SIZE& size) throw() {offset(size.cx, size.cy);}
	bool subtract(const RECT& rect1, const RECT& rect2) throw() {return toBoolean(::SubtractRect(this, &rect1, &rect2));}
};

}} // namespace manah.win32

#endif // !MANAH_UTILITY_HPP
