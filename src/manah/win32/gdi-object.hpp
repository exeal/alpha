// gdi-object.hpp
// (c) 2004-2007 exeal

#ifndef MANAH_GDI_OBJECT_HPP
#define MANAH_GDI_OBJECT_HPP

#include "utility.hpp"
#include <commctrl.h>	// COLORMAP

namespace manah {
namespace windows {
namespace gdi {


class GDIObject : public HandleHolder<HGDIOBJ>, public Noncopyable {
public:
	// constructors
	explicit GDIObject(HGDIOBJ handle = 0) : HandleHolder<HGDIOBJ>(handle), stockObject_(false) {}
	virtual ~GDIObject() {deleteObject();}
	// methods
	virtual bool createStockObject(int index) = 0;
	bool deleteObject() {if(get() != 0 && !stockObject_) return toBoolean(::DeleteObject(detach())); return false;}
	bool unrealizeObject() {return toBoolean(::UnrealizeObject(get()));}

	// data members
protected:
	bool stockObject_;
};

class Bitmap : public GDIObject {
public:
	// constructor
	explicit Bitmap(HBITMAP handle = 0) : GDIObject(handle) {}
	// operator
	operator HBITMAP() const throw() {return static_cast<HBITMAP>(get());}
	// methods
	bool	createStockObject(int index);
	bool	loadBitmap(const ResourceID& id);
	bool	loadOEMBitmap(uint id);
	bool	loadMappedBitmap(uint id, uint flags = 0, LPCOLORMAP colorMap = 0, int mapSize = 0);
	bool	createBitmap(int width, int height, uint planes, uint bitCount, const void* bits);
	bool	createBitmapIndirect(const BITMAP& bitmap);
	bool	createCompatibleBitmap(const DC& dc, int width, int height);
	bool	createDIBitmap(const DC& dc, const BITMAPINFOHEADER& header,
				DWORD options, const void* data, const BITMAPINFO& bitmapInfo, UINT usage);
	bool	createDiscardableBitmap(const DC& dc, int width, int height);
	bool	getBitmap(BITMAP& bitmap) const;
	DWORD	setBitmapBits(DWORD count, const void* bits);
	DWORD	getBitmapBits(DWORD count, void* bits) const;
	Size	setBitmapDimension(int width, int height);
	Size	getBitmapDimension() const;
};

class Brush : public GDIObject {
public:
	// constructors
	explicit Brush(HBRUSH handle = 0) : GDIObject(handle) {}
	explicit Brush(COLORREF color) {createSolidBrush(color);}
	Brush(int index, COLORREF color) {createHatchBrush(index, color);}
	explicit Brush(const Bitmap& bitmap) {createPatternBrush(bitmap);}
	operator HBRUSH() const throw() {return static_cast<HBRUSH>(get());}
	// methods
	bool	createStockObject(int index);
	HBRUSH	getSafeHandle() const;
	bool	createSolidBrush(COLORREF color);
	bool	createHatchBrush(int index, COLORREF color);
	bool	createBrushIndirect(const LOGBRUSH& logbrush);
	bool	createPatternBrush(const Bitmap& bitmap);
	bool	createDIBPatternBrush(HGLOBAL data, uint usage);
	bool	createDIBPatternBrush(const void* packedDIB, uint usage);
	bool	createSysColorBrush(int index);
	bool	getLogBrush(LOGBRUSH& logbrush) const;
};

class Font : public GDIObject {
public:
	// constructor
	Font(HFONT handle = 0) : GDIObject(handle) {}
	// operator
	operator HFONT() const {return static_cast<HFONT>(get());}
	// methods
	bool	createStockObject(int index);
	HFONT	getSafeHandle() const;
	bool	createFont(int width, int height, int escapement, int orientation, int weight,
				bool italic, bool underlined, bool strikeOut, BYTE charset, BYTE outPrecision,
				BYTE clipPrecision, BYTE quality, BYTE pitchAndFamily, const TCHAR* faceName);
	bool	createFontIndirect(const LOGFONT& logfont);
	bool	createPointFont(int pointSize, const TCHAR* faceName, DC* dc = 0);
	bool	createPointFontIndirect(const LOGFONT& logfont, DC* dc = 0);
	bool	getLogFont(LOGFONT& logfont) const;
};

class Palette : public GDIObject {
public:
	// constructor
	Palette(HPALETTE handle = 0) : GDIObject(handle) {}
	// operator
	operator HPALETTE() const throw() {return static_cast<HPALETTE>(get());}
	// methods
	bool		createStockObject(int index);
	HPALETTE	getSafeHandle() const;
	bool		createPalette(const LOGPALETTE& logpalette);
	bool		createHalftonePalette(DC& dc);
	void		animatePalette(uint start, uint count, const PALETTEENTRY paletteColors[]);
	uint		getNearestPaletteIndex(COLORREF color) const;
	bool		resizePalette(uint count);
	int			getEntryCount() const;
	uint		getPaletteEntries(uint start, uint count, PALETTEENTRY paletteColors[]) const;
	uint		setPaletteEntries(uint start, uint count, const PALETTEENTRY paletteColors[]);
};

class Pen : public GDIObject {
public:
	// constructors
	explicit Pen(HPEN handle = 0) : GDIObject(handle) {}
	Pen(int penStyle, int width, COLORREF color) {createPen(penStyle, width, color);}
	Pen(int penStyle, int width, const LOGBRUSH& logbrush, int styleCount = 0, const DWORD styles[] = 0) {
		createPen(penStyle, width, logbrush, styleCount, styles);}
	// operator
	operator HPEN() const {return static_cast<HPEN>(get());}
	// methods
	bool	createStockObject(int index);
	HPEN	getSafeHandle() const;
	bool	createPen(int penStyle, int width, COLORREF color);
	bool	createPen(int penStyle, int width, const LOGBRUSH& logbrush, int styleCount = 0, const DWORD styles[] = 0);
	bool	createPenIndirect(const LOGPEN& logpen);
	bool	getLogPen(LOGPEN& logpen) const;
	bool	getExtLogPen(EXTLOGPEN& extlogpen) const;
};

class Rgn : public GDIObject {
public:
	// constructor
	explicit Rgn(HRGN handle = 0) : GDIObject(handle) {}
	// operator
	operator HRGN() const throw() {return static_cast<HRGN>(get());}
	// methods
	bool	createStockObject(int index);
	HRGN	getSafeHandle() const;
	bool	createRectRgn(int left, int top, int right, int bottom);
	bool	createRectRgnIndirect(const RECT& rect);
	bool	createEllipticRgn(int left, int top, int right, int bottom);
	bool	createEllipticRgnIndirect(const RECT& rect);
	bool	createPolygonRgn(const POINT points[], int count, int polyFillMode);
	bool	createPolyPolygonRgn(const POINT points[], const int* polyCount, int count, int polyFillMode);
	bool	createRoundRectRgn(int x1, int y1, int x2, int y2, int x3, int y3);
	int		combineRgn(const Rgn& rgn1, const Rgn& rgn2, int combineMode);
	int		copyRgn(const Rgn& other);
	bool	createFromPath(const DC& dc);
	bool	createFromData(const XFORM* xForm, int count, const RGNDATA rgnData[]);
	bool	equalRgn(const Rgn& other) const;
	int		getRegionData(RGNDATA rgnData[], int count) const;
	int		getRgnBox(RECT& rect) const;
	int		offsetRgn(int x, int y);
	int		offsetRgn(const POINT& pt);
	bool	ptInRegion(int x, int y) const;
	bool	ptInRegion(const POINT& pt) const;
	bool	rectInRegion(const RECT& rect) const;
	bool	setRectRgn(int left, int top, int right, int bottom);
	bool	setRectRgn(const RECT& rect);
protected:
	HRGN get() const throw() {return static_cast<HRGN>(GDIObject::get());}
};

#define CREATE_NATIVE_OBJECT(expression)	\
	if(get() != 0)					\
		return false;						\
	setHandle(expression);					\
	return get() != 0


// Bitmap ///////////////////////////////////////////////////////////////////

inline bool Bitmap::createBitmap(int width, int height, uint planes, uint bitCount, const void* bits) {
	CREATE_NATIVE_OBJECT(::CreateBitmap(width, height, planes, bitCount, bits));}

inline bool Bitmap::createBitmapIndirect(const BITMAP& bitmap) {CREATE_NATIVE_OBJECT(::CreateBitmapIndirect(&(const_cast<BITMAP&>(bitmap))));}

inline bool Bitmap::createCompatibleBitmap(const DC& dc, int width, int height) {CREATE_NATIVE_OBJECT(::CreateCompatibleBitmap(dc.get(), width, height));}

inline bool	Bitmap::createDIBitmap(const DC& dc, const BITMAPINFOHEADER& header, DWORD options, const void* data,
	const BITMAPINFO& bitmapInfo, UINT usage) {CREATE_NATIVE_OBJECT(::CreateDIBitmap(dc.get(), &header, options, data, &bitmapInfo, usage));}

inline bool Bitmap::createDiscardableBitmap(const DC& dc, int width, int height) {CREATE_NATIVE_OBJECT(::CreateDiscardableBitmap(dc.get(), width, height));}

inline bool Bitmap::createStockObject(int) {return false;}

inline bool Bitmap::getBitmap(BITMAP& bitmap) const {return ::GetObject(get(), sizeof(HBITMAP), &bitmap) != 0;}

inline DWORD Bitmap::getBitmapBits(DWORD count, void* bits) const {return ::GetBitmapBits(static_cast<HBITMAP>(get()), count, bits);}

inline Size Bitmap::getBitmapDimension() const {
	SIZE size;
	::GetBitmapDimensionEx(static_cast<HBITMAP>(get()), &size);
	return size;
}

inline bool Bitmap::loadBitmap(const ResourceID& id) {CREATE_NATIVE_OBJECT(::LoadBitmap(::GetModuleHandle(0), id.name));}

inline bool Bitmap::loadMappedBitmap(uint id, uint flags /* = 0 */, LPCOLORMAP colorMap /* = 0 */, int mapSize /* = 0 */) {
	CREATE_NATIVE_OBJECT(::CreateMappedBitmap(::GetModuleHandle(0), id, flags, colorMap, mapSize));}

inline bool Bitmap::loadOEMBitmap(uint id) {CREATE_NATIVE_OBJECT(::LoadBitmap(0, MAKEINTRESOURCE(id)));}

inline DWORD Bitmap::setBitmapBits(DWORD count, const void* bits) {return ::SetBitmapBits(static_cast<HBITMAP>(get()), count, bits);}

inline Size Bitmap::setBitmapDimension(int width, int height) {
	SIZE size;
	::SetBitmapDimensionEx(static_cast<HBITMAP>(get()), width, height, &size);
	return size;
}


// Brush ////////////////////////////////////////////////////////////////////

inline bool Brush::createBrushIndirect(const LOGBRUSH& logbrush) {CREATE_NATIVE_OBJECT(::CreateBrushIndirect(&logbrush));}

inline bool Brush::createDIBPatternBrush(HGLOBAL data, uint usage) {CREATE_NATIVE_OBJECT(::CreateDIBPatternBrush(data, usage));}

inline bool Brush::createDIBPatternBrush(const void* packedDIB, uint usage) {CREATE_NATIVE_OBJECT(::CreateDIBPatternBrushPt(packedDIB, usage));}

inline bool Brush::createHatchBrush(int index, COLORREF color) {CREATE_NATIVE_OBJECT(::CreateHatchBrush(index, color));}

inline bool Brush::createPatternBrush(const Bitmap& bitmap) {CREATE_NATIVE_OBJECT(::CreatePatternBrush(bitmap));}

inline bool Brush::createSolidBrush(COLORREF color) {CREATE_NATIVE_OBJECT(::CreateSolidBrush(color));}

inline bool Brush::createStockObject(int index) {
	if(get() != 0 || index > HOLLOW_BRUSH)
		return false;
	setHandle(::GetStockObject(index));
	if(get() != 0) {
		stockObject_ = true;
		return true;
	}
	return false;
}

inline bool Brush::createSysColorBrush(int index) {CREATE_NATIVE_OBJECT(::GetSysColorBrush(index));}

inline bool Brush::getLogBrush(LOGBRUSH& logbrush) const {return ::GetObject(get(), sizeof(HBRUSH), &logbrush) != 0;}

inline HBRUSH Brush::getSafeHandle() const {return static_cast<HBRUSH>((this != 0) ? get() : 0);}


// Font /////////////////////////////////////////////////////////////////////

inline bool Font::createFont(int width, int height, int escapement, int orientation, int weight,
		bool italic, bool underlined, bool strikeOut, BYTE charset, BYTE outPrecision, BYTE clipPrecision,
		BYTE quality, BYTE pitchAndFamily, const TCHAR* faceName) {
	CREATE_NATIVE_OBJECT(::CreateFont(width, height, escapement, orientation, weight,
				italic, underlined, strikeOut, charset, outPrecision, clipPrecision,
				quality, pitchAndFamily, faceName));
}

inline bool Font::createFontIndirect(const LOGFONT& logfont) {CREATE_NATIVE_OBJECT(::CreateFontIndirect(&logfont));}

inline bool Font::createStockObject(int index) {
	if(get() != 0 || index < ANSI_FIXED_FONT || index > DEFAULT_GUI_FONT)
		return false;
	setHandle(::GetStockObject(index));
	if(get() != 0) {
		stockObject_ = true;
		return true;
	}
	return false;
}

inline bool Font::getLogFont(LOGFONT& logfont) const {return ::GetObject(get(), sizeof(LOGFONT), &logfont) != 0;}

inline HFONT Font::getSafeHandle() const {return static_cast<HFONT>((this != 0) ? get() : 0);}


// Palette //////////////////////////////////////////////////////////////////

inline void Palette::animatePalette(uint start, uint count, const PALETTEENTRY paletteColors[]) {::AnimatePalette(*this, start, count, paletteColors);}

inline bool Palette::createHalftonePalette(DC& dc) {CREATE_NATIVE_OBJECT(::CreateHalftonePalette(dc.get()));}

inline bool Palette::createPalette(const LOGPALETTE& logpalette) {CREATE_NATIVE_OBJECT(::CreatePalette(&logpalette));}

inline bool Palette::createStockObject(int index) {
	if(get() != 0 || index != DEFAULT_PALETTE)
		return false;
	setHandle(::GetStockObject(index));
	return get() != 0;
}

inline int Palette::getEntryCount() const {return ::GetPaletteEntries(*this, 0, 0, 0);}

inline uint Palette::getNearestPaletteIndex(COLORREF color) const {return ::GetNearestPaletteIndex(*this, color);}

inline uint Palette::getPaletteEntries(uint start, uint count, PALETTEENTRY paletteColors[]) const {return ::GetPaletteEntries(*this, start, count, paletteColors);}

inline HPALETTE Palette::getSafeHandle() const {return static_cast<HPALETTE>((this != 0) ? get() : 0);}

inline bool Palette::resizePalette(uint count) {return toBoolean(::ResizePalette(*this, count));}

inline uint Palette::setPaletteEntries(uint start, uint count, const PALETTEENTRY paletteColors[]) {return ::SetPaletteEntries(*this, start, count, paletteColors);}


// Pen //////////////////////////////////////////////////////////////////////

inline bool Pen::createPen(int penStyle, int width, COLORREF color) {CREATE_NATIVE_OBJECT(::CreatePen(penStyle, width, color));}

inline bool Pen::createPen(int penStyle, int width, const LOGBRUSH& logbrush, int styleCount /* = 0 */, const DWORD styles[] /* = 0*/) {
	CREATE_NATIVE_OBJECT(::ExtCreatePen(penStyle, width, &logbrush, styleCount, styles));}

inline bool Pen::createPenIndirect(const LOGPEN& logpen) {CREATE_NATIVE_OBJECT(::CreatePenIndirect(&logpen));}

inline bool Pen::createStockObject(int index) {
	if(get() != 0 || index < BLACK_PEN || index > WHITE_PEN)
		return false;
	setHandle(::GetStockObject(index));
	return get() != 0;
}

inline bool Pen::getExtLogPen(EXTLOGPEN& extlogpen) const {return toBoolean(::GetObject(get(), sizeof(EXTLOGPEN), &extlogpen));}

inline bool Pen::getLogPen(LOGPEN& logpen) const {return toBoolean(::GetObject(get(), sizeof(LOGPEN), &logpen));}

inline HPEN Pen::getSafeHandle() const {return static_cast<HPEN>((this != 0) ? get() : 0);}


// Rgn //////////////////////////////////////////////////////////////////////

inline int Rgn::combineRgn(const Rgn& rgn1, const Rgn& rgn2, int combineMode) {
	if(get() != 0)
		return false;
	return ::CombineRgn(get(), rgn1, rgn2, combineMode);
}

inline bool Rgn::createEllipticRgn(int left, int top, int right, int bottom) {CREATE_NATIVE_OBJECT(::CreateEllipticRgn(left, top, right, bottom));}

inline bool Rgn::createEllipticRgnIndirect(const RECT& rect) {CREATE_NATIVE_OBJECT(::CreateEllipticRgnIndirect(&rect));}

inline bool Rgn::createFromData(const XFORM* xForm, int count, const RGNDATA rgnData[]) {
	CREATE_NATIVE_OBJECT(::ExtCreateRegion(xForm, count, rgnData));}

inline bool Rgn::createFromPath(const DC& dc) {CREATE_NATIVE_OBJECT(::PathToRegion(dc.get()));}

inline bool Rgn::createPolygonRgn(const POINT points[], int count, int polyFillMode) {
	CREATE_NATIVE_OBJECT(::CreatePolygonRgn(points, count, polyFillMode));}

inline bool Rgn::createPolyPolygonRgn(const POINT points[], const int* polyCount, int count, int polyFillMode) {
	CREATE_NATIVE_OBJECT(::CreatePolyPolygonRgn(points, polyCount, count, polyFillMode));}

inline bool Rgn::createRectRgn(int left, int top, int right, int bottom) {CREATE_NATIVE_OBJECT(::CreateRectRgn(left, top, right, bottom));}

inline bool Rgn::createRectRgnIndirect(const RECT& rect) {CREATE_NATIVE_OBJECT(::CreateRectRgnIndirect(&rect));}

inline bool Rgn::createRoundRectRgn(int x1, int y1, int x2, int y2, int x3, int y3) {CREATE_NATIVE_OBJECT(::CreateRoundRectRgn(x1, y1, x2, y2, x3, y3));}

inline bool Rgn::createStockObject(int index) {return false;}

inline bool Rgn::equalRgn(const Rgn& other) const {return toBoolean(::EqualRgn(get(), other));}

inline int Rgn::getRegionData(RGNDATA rgnData[], int count) const {return toBoolean(::GetRegionData(get(), count, rgnData));}

inline int Rgn::getRgnBox(RECT& rect) const {return ::GetRgnBox(get(), &rect);}

inline HRGN Rgn::getSafeHandle() const throw() {return (this != 0) ? static_cast<HRGN>(get()) : 0;}

inline int Rgn::offsetRgn(int x, int y) {return ::OffsetRgn(get(), x, y);}

inline int Rgn::offsetRgn(const POINT& pt) {return offsetRgn(pt.x, pt.y);}

inline bool Rgn::ptInRegion(int x, int y) const {return toBoolean(::PtInRegion(get(), x, y));}

inline bool Rgn::ptInRegion(const POINT& pt) const {return ptInRegion(pt.x, pt.y);}

inline bool Rgn::rectInRegion(const RECT& rect) const {return toBoolean(::RectInRegion(get(), &rect));}

inline bool Rgn::setRectRgn(int left, int top, int right, int bottom) {return toBoolean(::SetRectRgn(get(), left, top, right, bottom));}

inline bool Rgn::setRectRgn(const RECT& rect) {return toBoolean(::SetRectRgn(get(), rect.left, rect.top, rect.right, rect.bottom));}

#undef CREATE_NATIVE_OBJECT

}}} // namespace manah::windows::gdi

#endif /* MANAH_GDI_OBJECT_HPP */
