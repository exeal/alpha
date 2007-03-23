// gdi-object.hpp
// (c) 2004-2007 exeal

#ifndef MANAH_GDI_OBJECT_HPP
#define MANAH_GDI_OBJECT_HPP

#include "utility.hpp"
#include <commctrl.h>	// COLORMAP

namespace manah {
namespace win32 {
namespace gdi {


class GDIObject : public Handle<HGDIOBJ, ::DeleteObject>, public Noncopyable {
public:
	// constructors
	explicit GDIObject(HGDIOBJ handle = 0) : Handle<HGDIOBJ, ::DeleteObject>(handle) {}
	GDIObject(const GDIObject& rhs) : Handle<HGDIOBJ, ::DeleteObject>(rhs) {}
	virtual ~GDIObject() throw() {}
	// methods
	bool unrealize() {return toBoolean(::UnrealizeObject(getHandle()));}
};

class Bitmap : public GDIObject {
public:
	// constructor
	explicit Bitmap(HBITMAP handle = 0) : GDIObject(handle) {}
	static std::auto_ptr<Bitmap>	create(int width, int height, uint planes, uint bitCount, const void* bits);
	static std::auto_ptr<Bitmap>	create(const ::BITMAP& bitmap);
	static std::auto_ptr<Bitmap>	createCompatibleBitmap(const DC& dc, int width, int height);
	static std::auto_ptr<Bitmap>	createDIBitmap(const DC& dc, const ::BITMAPINFOHEADER& header,
										DWORD options, const void* data, const ::BITMAPINFO& bitmapInfo, UINT usage);
	static std::auto_ptr<Bitmap>	createDiscardableBitmap(const DC& dc, int width, int height);
	Bitmap							createStockObject(int index);
	static std::auto_ptr<Bitmap>	load(const ResourceID& id);
	static std::auto_ptr<Bitmap>	loadOEMBitmap(uint id);
	static std::auto_ptr<Bitmap>	loadMappedBitmap(uint id, uint flags = 0, ::LPCOLORMAP colorMap = 0, int mapSize = 0);
	// methods
	bool	getBitmap(::BITMAP& bitmap) const;
	DWORD	getBits(DWORD count, void* bits) const;
	Size	getDimension() const;
	HBITMAP	getHandle() const throw() {return static_cast<HBITMAP>(GDIObject::getHandle());}
	DWORD	setBits(DWORD count, const void* bits);
	Size	setDimension(int width, int height);
};

class Brush : public GDIObject {
public:
	// constructors
	explicit Brush(HBRUSH handle = 0) : GDIObject(handle) {}
	static std::auto_ptr<Brush>	create(COLORREF color);
	static std::auto_ptr<Brush>	create(const ::LOGBRUSH& logbrush);
	static std::auto_ptr<Brush>	createHatchBrush(int index, COLORREF color);
	static std::auto_ptr<Brush>	createPatternBrush(const Bitmap& bitmap);
	static std::auto_ptr<Brush>	createDIBPatternBrush(HGLOBAL data, uint usage);
	static std::auto_ptr<Brush>	createDIBPatternBrush(const void* packedDIB, uint usage);
	static Brush				getStockObject(int index);
	static Brush				getSystemColorBrush(int index);
	// methods
	HBRUSH	getHandle() const throw() {return static_cast<HBRUSH>(GDIObject::getHandle());}
	bool	getLogBrush(::LOGBRUSH& logbrush) const;
};

class Font : public GDIObject {
public:
	// constructor
	Font(HFONT handle = 0) : GDIObject(handle) {}
	static std::auto_ptr<Font>	create(int width, int height, int escapement, int orientation, int weight,
									bool italic, bool underlined, bool strikeOut, BYTE charset, BYTE outPrecision,
									BYTE clipPrecision, BYTE quality, BYTE pitchAndFamily, const TCHAR* faceName);
	static std::auto_ptr<Font>	create(const ::LOGFONT& logfont);
	static Font					getStockObject(int index);
	// methods
	HFONT	getHandle() const {return static_cast<HFONT>(GDIObject::getHandle());}
	bool	getLogFont(::LOGFONT& logfont) const;
};

class Palette : public GDIObject {
public:
	// constructor
	Palette(HPALETTE handle = 0) : GDIObject(handle) {}
	static std::auto_ptr<Palette>	create(const ::LOGPALETTE& logpalette);
	static std::auto_ptr<Palette>	createHalftonePalette(DC& dc);
	static Palette					getStockObject(int index);
	// methods
	void		animate(uint start, uint count, const ::PALETTEENTRY paletteColors[]);
	int			getEntryCount() const;
	uint		getEntries(uint start, uint count, ::PALETTEENTRY paletteColors[]) const;
	HPALETTE	getHandle() const throw() {return static_cast<HPALETTE>(GDIObject::getHandle());}
	uint		getNearestIndex(COLORREF color) const;
	bool		resize(uint count);
	uint		setEntries(uint start, uint count, const ::PALETTEENTRY paletteColors[]);
};

class Pen : public GDIObject {
public:
	// constructors
	explicit Pen(HPEN handle = 0) : GDIObject(handle) {}
	static std::auto_ptr<Pen>	create(int penStyle, int width, COLORREF color);
	static std::auto_ptr<Pen>	create(int penStyle, int width, const ::LOGBRUSH& logbrush, int styleCount = 0, const DWORD styles[] = 0);
	static std::auto_ptr<Pen>	create(const ::LOGPEN& logpen);
	static Pen					getStockObject(int index);
	// methods
	HPEN	getHandle() const {return static_cast<HPEN>(GDIObject::getHandle());}
	bool	getLogPen(::LOGPEN& logpen) const;
	bool	getExtLogPen(::EXTLOGPEN& extlogpen) const;
};

class Rgn : public GDIObject {
public:
	// constructor
	explicit Rgn(HRGN handle = 0) : GDIObject(handle) {}
	static std::auto_ptr<Rgn>	createRect(int left, int top, int right, int bottom);
	static std::auto_ptr<Rgn>	createRect(const ::RECT& rect);
	static std::auto_ptr<Rgn>	createElliptic(int left, int top, int right, int bottom);
	static std::auto_ptr<Rgn>	createElliptic(const ::RECT& rect);
	static std::auto_ptr<Rgn>	createPolygon(const ::POINT points[], int count, int polyFillMode);
	static std::auto_ptr<Rgn>	createPolyPolygon(const ::POINT points[], const int* polyCount, int count, int polyFillMode);
	static std::auto_ptr<Rgn>	createRoundRect(int x1, int y1, int x2, int y2, int x3, int y3);
	static std::auto_ptr<Rgn>	fromData(const ::XFORM* xForm, int count, const ::RGNDATA rgnData[]);
	static std::auto_ptr<Rgn>	fromPath(const DC& dc);
	// methods
	int		combine(const Rgn& rgn1, const Rgn& rgn2, int combineMode);
	int		copy(const Rgn& other);
	bool	equals(const Rgn& other) const;
	int		getBox(::RECT& rect) const;
	int		getData(::RGNDATA rgnData[], int count) const;
	HRGN	getHandle() const throw() {return static_cast<HRGN>(GDIObject::getHandle());}
	bool	includes(int x, int y) const;
	bool	includes(const ::POINT& pt) const;
	bool	includes(const ::RECT& rect) const;
	int		offset(int x, int y);
	int		offset(const ::POINT& pt);
	bool	setRect(int left, int top, int right, int bottom);
	bool	setRect(const ::RECT& rect);
};

#define CREATE_NATIVE_OBJECT(type, expression)	\
	std::auto_ptr<type> p(new type); p->reset(expression); return p


// Bitmap ///////////////////////////////////////////////////////////////////

inline std::auto_ptr<Bitmap> Bitmap::create(int width, int height, uint planes, uint bitCount, const void* bits) {
	CREATE_NATIVE_OBJECT(Bitmap, ::CreateBitmap(width, height, planes, bitCount, bits));}

inline std::auto_ptr<Bitmap> Bitmap::create(const ::BITMAP& bitmap) {
	CREATE_NATIVE_OBJECT(Bitmap, ::CreateBitmapIndirect(&(const_cast<::BITMAP&>(bitmap))));}

inline std::auto_ptr<Bitmap> Bitmap::createCompatibleBitmap(const DC& dc, int width, int height) {
	CREATE_NATIVE_OBJECT(Bitmap, ::CreateCompatibleBitmap(dc.getHandle(), width, height));}

inline std::auto_ptr<Bitmap> Bitmap::createDIBitmap(const DC& dc, const ::BITMAPINFOHEADER& header, DWORD options, const void* data,
	const ::BITMAPINFO& bitmapInfo, UINT usage) {CREATE_NATIVE_OBJECT(Bitmap, ::CreateDIBitmap(dc.getHandle(), &header, options, data, &bitmapInfo, usage));}

inline std::auto_ptr<Bitmap> Bitmap::createDiscardableBitmap(const DC& dc, int width, int height) {
	CREATE_NATIVE_OBJECT(Bitmap, ::CreateDiscardableBitmap(dc.getHandle(), width, height));}

inline bool Bitmap::getBitmap(::BITMAP& bitmap) const {return ::GetObject(getHandle(), sizeof(HBITMAP), &bitmap) != 0;}

inline DWORD Bitmap::getBits(DWORD count, void* bits) const {return ::GetBitmapBits(getHandle(), count, bits);}

inline Size Bitmap::getDimension() const {::SIZE size; ::GetBitmapDimensionEx(getHandle(), &size); return size;}

inline std::auto_ptr<Bitmap> Bitmap::load(const ResourceID& id) {CREATE_NATIVE_OBJECT(Bitmap, ::LoadBitmap(::GetModuleHandle(0), id.name));}

inline std::auto_ptr<Bitmap> Bitmap::loadMappedBitmap(uint id, uint flags /* = 0 */, ::LPCOLORMAP colorMap /* = 0 */, int mapSize /* = 0 */) {
	CREATE_NATIVE_OBJECT(Bitmap, ::CreateMappedBitmap(::GetModuleHandle(0), id, flags, colorMap, mapSize));}

inline std::auto_ptr<Bitmap> Bitmap::loadOEMBitmap(uint id) {CREATE_NATIVE_OBJECT(Bitmap, ::LoadBitmap(0, MAKEINTRESOURCE(id)));}

inline DWORD Bitmap::setBits(DWORD count, const void* bits) {return ::SetBitmapBits(getHandle(), count, bits);}

inline Size Bitmap::setDimension(int width, int height) {::SIZE size; ::SetBitmapDimensionEx(getHandle(), width, height, &size); return size;}


// Brush ////////////////////////////////////////////////////////////////////

inline std::auto_ptr<Brush> Brush::create(COLORREF color) {CREATE_NATIVE_OBJECT(Brush, ::CreateSolidBrush(color));}

inline std::auto_ptr<Brush> Brush::create(const ::LOGBRUSH& logbrush) {CREATE_NATIVE_OBJECT(Brush, ::CreateBrushIndirect(&logbrush));}

inline std::auto_ptr<Brush> Brush::createDIBPatternBrush(HGLOBAL data, uint usage) {CREATE_NATIVE_OBJECT(Brush, ::CreateDIBPatternBrush(data, usage));}

inline std::auto_ptr<Brush> Brush::createDIBPatternBrush(const void* packedDIB, uint usage) {CREATE_NATIVE_OBJECT(Brush, ::CreateDIBPatternBrushPt(packedDIB, usage));}

inline std::auto_ptr<Brush> Brush::createHatchBrush(int index, COLORREF color) {CREATE_NATIVE_OBJECT(Brush, ::CreateHatchBrush(index, color));}

inline std::auto_ptr<Brush> Brush::createPatternBrush(const Bitmap& bitmap) {CREATE_NATIVE_OBJECT(Brush, ::CreatePatternBrush(bitmap.getHandle()));}

inline bool Brush::getLogBrush(::LOGBRUSH& logbrush) const {return ::GetObject(getHandle(), sizeof(HBRUSH), &logbrush) != 0;}

inline Brush Brush::getStockObject(int index) {Brush temp(static_cast<HBRUSH>(::GetStockObject(index))); return temp;}

inline Brush Brush::getSystemColorBrush(int index) {return Brush(::GetSysColorBrush(index));}


// Font /////////////////////////////////////////////////////////////////////

inline std::auto_ptr<Font> Font::create(int width, int height, int escapement, int orientation, int weight,
		bool italic, bool underlined, bool strikeOut, BYTE charset, BYTE outPrecision, BYTE clipPrecision,
		BYTE quality, BYTE pitchAndFamily, const TCHAR* faceName) {
	CREATE_NATIVE_OBJECT(Font, ::CreateFont(width, height, escapement, orientation, weight,
		italic, underlined, strikeOut, charset, outPrecision, clipPrecision, quality, pitchAndFamily, faceName));
}

inline std::auto_ptr<Font> Font::create(const ::LOGFONT& logfont) {CREATE_NATIVE_OBJECT(Font, ::CreateFontIndirect(&logfont));}

inline bool Font::getLogFont(::LOGFONT& logfont) const {return ::GetObject(getHandle(), sizeof(LOGFONT), &logfont) != 0;}

inline Font Font::getStockObject(int index) {return Font(static_cast<HFONT>(::GetStockObject(index)));}


// Palette //////////////////////////////////////////////////////////////////

inline void Palette::animate(uint start, uint count, const ::PALETTEENTRY paletteColors[]) {::AnimatePalette(getHandle(), start, count, paletteColors);}

inline std::auto_ptr<Palette> Palette::create(const ::LOGPALETTE& logpalette) {CREATE_NATIVE_OBJECT(Palette, ::CreatePalette(&logpalette));}

inline std::auto_ptr<Palette> Palette::createHalftonePalette(DC& dc) {CREATE_NATIVE_OBJECT(Palette, ::CreateHalftonePalette(dc.getHandle()));}

inline uint Palette::getEntries(uint start, uint count, ::PALETTEENTRY paletteColors[]) const {return ::GetPaletteEntries(getHandle(), start, count, paletteColors);}

inline int Palette::getEntryCount() const {return ::GetPaletteEntries(getHandle(), 0, 0, 0);}

inline uint Palette::getNearestIndex(COLORREF color) const {return ::GetNearestPaletteIndex(getHandle(), color);}

inline Palette Palette::getStockObject(int index) {return Palette(static_cast<HPALETTE>(::GetStockObject(index)));}

inline bool Palette::resize(uint count) {return toBoolean(::ResizePalette(getHandle(), count));}

inline uint Palette::setEntries(uint start, uint count, const ::PALETTEENTRY paletteColors[]) {return ::SetPaletteEntries(getHandle(), start, count, paletteColors);}


// Pen //////////////////////////////////////////////////////////////////////

inline std::auto_ptr<Pen> Pen::create(int penStyle, int width, COLORREF color) {CREATE_NATIVE_OBJECT(Pen, ::CreatePen(penStyle, width, color));}

inline std::auto_ptr<Pen> Pen::create(int penStyle, int width, const ::LOGBRUSH& logbrush, int styleCount /* = 0 */,
	const DWORD styles[] /* = 0*/) {CREATE_NATIVE_OBJECT(Pen, ::ExtCreatePen(penStyle, width, &logbrush, styleCount, styles));}

inline std::auto_ptr<Pen> Pen::create(const ::LOGPEN& logpen) {CREATE_NATIVE_OBJECT(Pen, ::CreatePenIndirect(&logpen));}

inline bool Pen::getExtLogPen(::EXTLOGPEN& extlogpen) const {return toBoolean(::GetObject(getHandle(), sizeof(::EXTLOGPEN), &extlogpen));}

inline bool Pen::getLogPen(::LOGPEN& logpen) const {return toBoolean(::GetObject(getHandle(), sizeof(::LOGPEN), &logpen));}

inline Pen Pen::getStockObject(int index) {return Pen(static_cast<HPEN>(::GetStockObject(index)));}


// Rgn //////////////////////////////////////////////////////////////////////

inline int Rgn::combine(const Rgn& rgn1, const Rgn& rgn2, int combineMode) {
	if(getHandle() != 0) return false; return ::CombineRgn(getHandle(), rgn1.getHandle(), rgn2.getHandle(), combineMode);}

inline std::auto_ptr<Rgn> Rgn::createElliptic(int left, int top, int right, int bottom) {CREATE_NATIVE_OBJECT(Rgn, ::CreateEllipticRgn(left, top, right, bottom));}

inline std::auto_ptr<Rgn> Rgn::createElliptic(const ::RECT& rect) {CREATE_NATIVE_OBJECT(Rgn, ::CreateEllipticRgnIndirect(&rect));}

inline std::auto_ptr<Rgn> Rgn::createPolygon(const ::POINT points[], int count, int polyFillMode) {
	CREATE_NATIVE_OBJECT(Rgn, ::CreatePolygonRgn(points, count, polyFillMode));}

inline std::auto_ptr<Rgn> Rgn::createPolyPolygon(const ::POINT points[], const int* polyCount, int count, int polyFillMode) {
	CREATE_NATIVE_OBJECT(Rgn, ::CreatePolyPolygonRgn(points, polyCount, count, polyFillMode));}

inline std::auto_ptr<Rgn> Rgn::createRect(int left, int top, int right, int bottom) {CREATE_NATIVE_OBJECT(Rgn, ::CreateRectRgn(left, top, right, bottom));}

inline std::auto_ptr<Rgn> Rgn::createRect(const ::RECT& rect) {CREATE_NATIVE_OBJECT(Rgn, ::CreateRectRgnIndirect(&rect));}

inline std::auto_ptr<Rgn> Rgn::createRoundRect(int x1, int y1, int x2, int y2, int x3, int y3) {CREATE_NATIVE_OBJECT(Rgn, ::CreateRoundRectRgn(x1, y1, x2, y2, x3, y3));}

inline bool Rgn::equals(const Rgn& other) const {return toBoolean(::EqualRgn(getHandle(), other.getHandle()));}

inline std::auto_ptr<Rgn> Rgn::fromData(const ::XFORM* xForm, int count, const ::RGNDATA rgnData[]) {CREATE_NATIVE_OBJECT(Rgn, ::ExtCreateRegion(xForm, count, rgnData));}

inline std::auto_ptr<Rgn> Rgn::fromPath(const DC& dc) {CREATE_NATIVE_OBJECT(Rgn, ::PathToRegion(dc.getHandle()));}

inline int Rgn::getBox(RECT& rect) const {return ::GetRgnBox(getHandle(), &rect);}

inline int Rgn::getData(RGNDATA rgnData[], int count) const {return toBoolean(::GetRegionData(getHandle(), count, rgnData));}

inline bool Rgn::includes(int x, int y) const {return toBoolean(::PtInRegion(getHandle(), x, y));}

inline bool Rgn::includes(const ::POINT& pt) const {return includes(pt.x, pt.y);}

inline bool Rgn::includes(const ::RECT& rect) const {return toBoolean(::RectInRegion(getHandle(), &rect));}

inline int Rgn::offset(int x, int y) {return ::OffsetRgn(getHandle(), x, y);}

inline int Rgn::offset(const ::POINT& pt) {return offset(pt.x, pt.y);}

inline bool Rgn::setRect(int left, int top, int right, int bottom) {return toBoolean(::SetRectRgn(getHandle(), left, top, right, bottom));}

inline bool Rgn::setRect(const ::RECT& rect) {return toBoolean(::SetRectRgn(getHandle(), rect.left, rect.top, rect.right, rect.bottom));}

#undef CREATE_NATIVE_OBJECT

}}} // namespace manah.win32.gdi

#endif /* !MANAH_GDI_OBJECT_HPP */
