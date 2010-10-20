/**
 * @file gdi-object.hpp Defines GDI object wrapper classes.
 * @author exeal
 * @date 2004-2010
 */

#ifndef MANAH_GDI_OBJECT_HPP
#define MANAH_GDI_OBJECT_HPP

#include <commctrl.h>	// COLORMAP
#include "dc.hpp"

namespace manah {
namespace win32 {
namespace gdi {

template<typename HandleType> class GDIObject : public Object<HGDIOBJ, ::DeleteObject, HandleType> {
public:
	typedef GDIObject<HandleType> BaseObject;
	typedef HandleType Handle;
public:
	GDIObject() : Object<HGDIOBJ, ::DeleteObject, HandleType>() {}
	explicit GDIObject(Managed<HandleType>* handle) : Object<HGDIOBJ, ::DeleteObject, HandleType>(handle) {}
	explicit GDIObject(Borrowed<HandleType>* handle) : Object<HGDIOBJ, ::DeleteObject, HandleType>(handle) {}
	bool unrealize() {return toBoolean(::UnrealizeObject(use()));}
};

class Bitmap : public GDIObject<HBITMAP> {
public:
	// constructor
	MANAH_WIN32_OBJECT_CONSTRUCTORS(Bitmap)
	static Bitmap create(int width, int height, uint planes, uint bitCount, const void* bits);
	static Bitmap create(const BITMAP& bitmap);
	static Bitmap createCompatibleBitmap(const DC& dc, int width, int height);
	static Bitmap createDIBitmap(const DC& dc, const BITMAPINFOHEADER& header,
		DWORD options, const void* data, const BITMAPINFO& bitmapInfo, UINT usage);
	static Bitmap createDIBSection(HDC dc, const BITMAPINFO& , UINT usage, void*& bits);
	static Bitmap createDIBSection(HDC dc, const BITMAPINFO& , UINT usage, void*& bits, HANDLE section, DWORD offset);
	static Bitmap createDiscardableBitmap(const DC& dc, int width, int height);
	Bitmap createStockObject(int index);
	static Bitmap load(const ResourceID& id);
	static Bitmap loadOEMBitmap(uint id);
	static Bitmap loadMappedBitmap(uint id, uint flags = 0, LPCOLORMAP colorMap = 0, int mapSize = 0);
	// methods
	bool getBitmap(BITMAP& bitmap) const;
	DWORD getBits(DWORD count, void* bits) const;
	SIZE getDimension() const;
	DWORD setBits(DWORD count, const void* bits);
	SIZE setDimension(int width, int height);
};

class Brush : public GDIObject<HBRUSH> {
public:
	// constructors
	MANAH_WIN32_OBJECT_CONSTRUCTORS(Brush)
	static Brush create(COLORREF color);
	static Brush create(const LOGBRUSH& logbrush);
	static Brush createHatchBrush(int index, COLORREF color);
	static Brush createPatternBrush(const Bitmap& bitmap);
	static Brush createDIBPatternBrush(HGLOBAL data, uint usage);
	static Brush createDIBPatternBrush(const void* packedDIB, uint usage);
	static Brush getStockObject(int index);
	static Brush getSystemColorBrush(int index);
	// methods
	bool getLogBrush(LOGBRUSH& logbrush) const;
};

class Font : public GDIObject<HFONT> {
public:
	// constructor
	MANAH_WIN32_OBJECT_CONSTRUCTORS(Font)
	static Font	create(int width, int height, int escapement, int orientation, int weight,
		bool italic, bool underlined, bool strikeOut, BYTE charset, BYTE outPrecision,
		BYTE clipPrecision, BYTE quality, BYTE pitchAndFamily, const WCHAR* faceName);
	static Font create(const LOGFONTW& logfont);
	static Font getStockObject(int index);
	// methods
	bool getLogFont(LOGFONTW& logfont) const;
};

class Palette : public GDIObject<HPALETTE> {
public:
	// constructor
	MANAH_WIN32_OBJECT_CONSTRUCTORS(Palette)
	static Palette create(const LOGPALETTE& logpalette);
	static Palette createHalftonePalette(DC& dc);
	static Palette getStockObject(int index);
	// methods
	void animate(uint start, uint count, const PALETTEENTRY paletteColors[]);
	int getEntryCount() const;
	uint getEntries(uint start, uint count, PALETTEENTRY paletteColors[]) const;
	uint getNearestIndex(COLORREF color) const;
	bool resize(uint count);
	uint setEntries(uint start, uint count, const PALETTEENTRY paletteColors[]);
};

class Pen : public GDIObject<HPEN> {
public:
	// constructors
	MANAH_WIN32_OBJECT_CONSTRUCTORS(Pen)
	static Pen create(int penStyle, int width, COLORREF color);
	static Pen create(int penStyle, int width, const LOGBRUSH& logbrush, int styleCount = 0, const DWORD styles[] = 0);
	static Pen create(const LOGPEN& logpen);
	static Pen getStockObject(int index);
	// methods
	bool getLogPen(LOGPEN& logpen) const;
	bool getExtLogPen(EXTLOGPEN& extlogpen) const;
};

class Rgn : public GDIObject<HRGN> {
public:
	// constructor
	MANAH_WIN32_OBJECT_CONSTRUCTORS(Rgn)
	static Rgn createRect(int left, int top, int right, int bottom);
	static Rgn createRect(const RECT& rect);
	static Rgn createElliptic(int left, int top, int right, int bottom);
	static Rgn createElliptic(const RECT& rect);
	static Rgn createPolygon(const POINT points[], int count, int polyFillMode);
	static Rgn createPolyPolygon(const POINT points[], const int* polyCount, int count, int polyFillMode);
	static Rgn createRoundRect(int x1, int y1, int x2, int y2, int x3, int y3);
	static Rgn fromData(const XFORM* xForm, int count, const RGNDATA rgnData[]);
	static Rgn fromPath(const DC& dc);
	// methods
	int combine(const Rgn& rgn1, const Rgn& rgn2, int combineMode);
	int copy(const Rgn& other);
	bool equals(const Rgn& other) const;
	int getBox(RECT& rect) const;
	int getData(RGNDATA rgnData[], int count) const;
	bool includes(int x, int y) const;
	bool includes(const POINT& pt) const;
	bool includes(const RECT& rect) const;
	int offset(int x, int y);
	int offset(const POINT& pt);
	bool setRect(int left, int top, int right, int bottom);
	bool setRect(const RECT& rect);
};


// Bitmap ///////////////////////////////////////////////////////////////////

inline Bitmap Bitmap::create(int width, int height, uint planes, uint bitCount, const void* bits) {
	return Bitmap(managed(::CreateBitmap(width, height, planes, bitCount, bits)));}

inline Bitmap Bitmap::create(const BITMAP& bitmap) {return Bitmap(managed(::CreateBitmapIndirect(&(const_cast<BITMAP&>(bitmap)))));}

inline Bitmap Bitmap::createCompatibleBitmap(const DC& dc, int width, int height) {
	return Bitmap(managed(::CreateCompatibleBitmap(dc.use(), width, height)));}

inline Bitmap Bitmap::createDIBitmap(const DC& dc, const BITMAPINFOHEADER& header, DWORD options, const void* data,
	const BITMAPINFO& bitmapInfo, UINT usage) {return Bitmap(managed(::CreateDIBitmap(dc.use(), &header, options, data, &bitmapInfo, usage)));}

inline Bitmap Bitmap::createDIBSection(HDC dc, const BITMAPINFO& info, UINT usage, void*& bits) {return createDIBSection(dc, info, usage, bits, 0, 0);}

inline Bitmap Bitmap::createDIBSection(HDC dc, const BITMAPINFO& info, UINT usage, void*& bits,
	HANDLE section, DWORD offset) {return Bitmap(managed(::CreateDIBSection(dc, &info, usage, &bits, section, offset)));}

inline Bitmap Bitmap::createDiscardableBitmap(const DC& dc, int width, int height) {return Bitmap(managed(::CreateDiscardableBitmap(dc.use(), width, height)));}

inline bool Bitmap::getBitmap(BITMAP& bitmap) const {return ::GetObjectW(use(), sizeof(HBITMAP), &bitmap) != 0;}

inline DWORD Bitmap::getBits(DWORD count, void* bits) const {return ::GetBitmapBits(use(), count, bits);}

inline SIZE Bitmap::getDimension() const {SIZE size; ::GetBitmapDimensionEx(use(), &size); return size;}

inline Bitmap Bitmap::load(const ResourceID& id) {return Bitmap(managed(::LoadBitmapW(::GetModuleHandleW(0), id)));}

inline Bitmap Bitmap::loadMappedBitmap(uint id, uint flags /* = 0 */, LPCOLORMAP colorMap /* = 0 */, int mapSize /* = 0 */) {
	return Bitmap(managed(::CreateMappedBitmap(::GetModuleHandleW(0), id, flags, colorMap, mapSize)));}

inline Bitmap Bitmap::loadOEMBitmap(uint id) {return Bitmap(managed(::LoadBitmapW(0, MAKEINTRESOURCEW(id))));}

inline DWORD Bitmap::setBits(DWORD count, const void* bits) {return ::SetBitmapBits(use(), count, bits);}

inline SIZE Bitmap::setDimension(int width, int height) {SIZE size; ::SetBitmapDimensionEx(use(), width, height, &size); return size;}


// Brush ////////////////////////////////////////////////////////////////////

inline Brush Brush::create(COLORREF color) {return Brush(managed(::CreateSolidBrush(color)));}

inline Brush Brush::create(const LOGBRUSH& logbrush) {return Brush(managed(::CreateBrushIndirect(&logbrush)));}

inline Brush Brush::createDIBPatternBrush(HGLOBAL data, uint usage) {return Brush(managed(::CreateDIBPatternBrush(data, usage)));}

inline Brush Brush::createDIBPatternBrush(const void* packedDIB, uint usage) {return Brush(managed(::CreateDIBPatternBrushPt(packedDIB, usage)));}

inline Brush Brush::createHatchBrush(int index, COLORREF color) {return Brush(managed(::CreateHatchBrush(index, color)));}

inline Brush Brush::createPatternBrush(const Bitmap& bitmap) {return Brush(managed(::CreatePatternBrush(bitmap.use())));}

inline bool Brush::getLogBrush(LOGBRUSH& logbrush) const {return ::GetObjectW(use(), sizeof(HBRUSH), &logbrush) != 0;}

inline Brush Brush::getStockObject(int index) {return Brush(borrowed(static_cast<HBRUSH>(::GetStockObject(index))));}

inline Brush Brush::getSystemColorBrush(int index) {return Brush(borrowed(::GetSysColorBrush(index)));}


// Font /////////////////////////////////////////////////////////////////////

inline Font Font::create(int width, int height, int escapement, int orientation, int weight,
		bool italic, bool underlined, bool strikeOut, BYTE charset, BYTE outPrecision, BYTE clipPrecision,
		BYTE quality, BYTE pitchAndFamily, const WCHAR* faceName) {
	return Font(managed(::CreateFontW(width, height, escapement, orientation, weight,
		italic, underlined, strikeOut, charset, outPrecision, clipPrecision, quality, pitchAndFamily, faceName)));
}

inline Font Font::create(const LOGFONTW& logfont) {return Font(managed(::CreateFontIndirectW(&logfont)));}

inline bool Font::getLogFont(LOGFONTW& logfont) const {return ::GetObjectW(use(), sizeof(LOGFONTW), &logfont) != 0;}

inline Font Font::getStockObject(int index) {return Font(borrowed(static_cast<HFONT>(::GetStockObject(index))));}


// Palette //////////////////////////////////////////////////////////////////

inline void Palette::animate(uint start, uint count, const PALETTEENTRY paletteColors[]) {::AnimatePalette(use(), start, count, paletteColors);}

inline Palette Palette::create(const LOGPALETTE& logpalette) {return Palette(managed(::CreatePalette(&logpalette)));}

inline Palette Palette::createHalftonePalette(DC& dc) {return Palette(managed(::CreateHalftonePalette(dc.use())));}

inline uint Palette::getEntries(uint start, uint count, PALETTEENTRY paletteColors[]) const {return ::GetPaletteEntries(use(), start, count, paletteColors);}

inline int Palette::getEntryCount() const {return ::GetPaletteEntries(use(), 0, 0, 0);}

inline uint Palette::getNearestIndex(COLORREF color) const {return ::GetNearestPaletteIndex(use(), color);}

inline Palette Palette::getStockObject(int index) {return Palette(borrowed(static_cast<HPALETTE>(::GetStockObject(index))));}

inline bool Palette::resize(uint count) {return toBoolean(::ResizePalette(use(), count));}

inline uint Palette::setEntries(uint start, uint count, const PALETTEENTRY paletteColors[]) {return ::SetPaletteEntries(use(), start, count, paletteColors);}


// Pen //////////////////////////////////////////////////////////////////////

inline Pen Pen::create(int penStyle, int width, COLORREF color) {return Pen(managed(::CreatePen(penStyle, width, color)));}

inline Pen Pen::create(int penStyle, int width, const LOGBRUSH& logbrush, int styleCount /* = 0 */,
	const DWORD styles[] /* = 0*/) {return Pen(managed(::ExtCreatePen(penStyle, width, &logbrush, styleCount, styles)));}

inline Pen Pen::create(const LOGPEN& logpen) {return Pen(managed(::CreatePenIndirect(&logpen)));}

inline bool Pen::getExtLogPen(EXTLOGPEN& extlogpen) const {return toBoolean(::GetObjectW(use(), sizeof(EXTLOGPEN), &extlogpen));}

inline bool Pen::getLogPen(LOGPEN& logpen) const {return toBoolean(::GetObjectW(use(), sizeof(LOGPEN), &logpen));}

inline Pen Pen::getStockObject(int index) {return Pen(borrowed(static_cast<HPEN>(::GetStockObject(index))));}


// Rgn //////////////////////////////////////////////////////////////////////

inline int Rgn::combine(const Rgn& rgn1, const Rgn& rgn2, int combineMode) {
	if(get() != 0) return false; return ::CombineRgn(use(), rgn1.use(), rgn2.use(), combineMode);}

inline Rgn Rgn::createElliptic(int left, int top, int right, int bottom) {return Rgn(managed(::CreateEllipticRgn(left, top, right, bottom)));}

inline Rgn Rgn::createElliptic(const RECT& rect) {return Rgn(managed(::CreateEllipticRgnIndirect(&rect)));}

inline Rgn Rgn::createPolygon(const POINT points[], int count, int polyFillMode) {return Rgn(managed(::CreatePolygonRgn(points, count, polyFillMode)));}

inline Rgn Rgn::createPolyPolygon(const POINT points[], const int* polyCount,
	int count, int polyFillMode) {return Rgn(managed(::CreatePolyPolygonRgn(points, polyCount, count, polyFillMode)));}

inline Rgn Rgn::createRect(int left, int top, int right, int bottom) {return Rgn(managed(::CreateRectRgn(left, top, right, bottom)));}

inline Rgn Rgn::createRect(const RECT& rect) {return Rgn(managed(::CreateRectRgnIndirect(&rect)));}

inline Rgn Rgn::createRoundRect(int x1, int y1, int x2, int y2, int x3, int y3) {return Rgn(managed(::CreateRoundRectRgn(x1, y1, x2, y2, x3, y3)));}

inline bool Rgn::equals(const Rgn& other) const {return toBoolean(::EqualRgn(use(), other.use()));}

inline Rgn Rgn::fromData(const XFORM* xForm, int count, const RGNDATA rgnData[]) {return Rgn(managed(::ExtCreateRegion(xForm, count, rgnData)));}

inline Rgn Rgn::fromPath(const DC& dc) {return Rgn(managed(::PathToRegion(dc.use())));}

inline int Rgn::getBox(RECT& rect) const {return ::GetRgnBox(use(), &rect);}

inline int Rgn::getData(RGNDATA rgnData[], int count) const {return toBoolean(::GetRegionData(use(), count, rgnData));}

inline bool Rgn::includes(int x, int y) const {return toBoolean(::PtInRegion(use(), x, y));}

inline bool Rgn::includes(const POINT& pt) const {return includes(pt.x, pt.y);}

inline bool Rgn::includes(const RECT& rect) const {return toBoolean(::RectInRegion(use(), &rect));}

inline int Rgn::offset(int x, int y) {return ::OffsetRgn(use(), x, y);}

inline int Rgn::offset(const POINT& pt) {return offset(pt.x, pt.y);}

inline bool Rgn::setRect(int left, int top, int right, int bottom) {return toBoolean(::SetRectRgn(use(), left, top, right, bottom));}

inline bool Rgn::setRect(const RECT& rect) {return toBoolean(::SetRectRgn(use(), rect.left, rect.top, rect.right, rect.bottom));}

}}} // namespace manah.win32.gdi

#endif // !MANAH_GDI_OBJECT_HPP
