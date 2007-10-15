// dc.hpp
// (c) 2003-2007 exeal

#ifndef MANAH_DC_HPP
#define MANAH_DC_HPP
#include "windows.hpp"
#include <memory>	// std.auto_ptr

namespace manah {
	namespace win32 {

		namespace ui {class Window;}

		namespace gdi {

class DC : public Handle<HDC, ::DeleteDC> {
public:
	// constructors
	DC() : Handle<HDC, ::DeleteDC>(0) {}
	virtual ~DC() {}
	// current objects
	::HBITMAP	getCurrentBitmap() const;
	::HBRUSH	getCurrentBrush() const;
	::HFONT		getCurrentFont() const;
	::HPALETTE	getCurrentPalette() const;
	::HPEN		getCurrentPen() const;
	::HWND		getWindow() const;
	// device context
	std::auto_ptr<DC>	createCompatibleDC() const;
	int					getDeviceCaps(int index) const;
	virtual bool		restore(int savedDC);
	virtual int			save();
	// drawing tools
	int		enumObjects(int objectType, ::GOBJENUMPROC proc, LPARAM data);
	::POINT	getBrushOrg() const;
	::POINT	setBrushOrg(int x, int y);
	::POINT	setBrushOrg(const ::POINT& pt);
	// selection
	::HBITMAP		selectObject(::HBITMAP bitmap);
	::HBRUSH		selectObject(::HBRUSH brush);
	virtual ::HFONT	selectObject(::HFONT font);
	::HPEN			selectObject(::HPEN pen);
	::HGDIOBJ		selectStockObject(int object);
	// color and palette
	::COLORREF	getNearestColor(COLORREF color) const;
	::UINT		realizePalette();
	::HPALETTE	selectPalette(HPALETTE palette, bool forceBackground);
	void		updateColors();
	// attributes
	::COLORREF	getBkColor() const;
	int			getBkMode() const;
	bool		getColorAdjustment(::COLORADJUSTMENT& colorAdjust) const;
	int			getPolyFillMode() const;
	int			getROP2() const;
	int			getStretchBltMode() const;
	::COLORREF	getTextColor() const;
	::COLORREF	setBkColor(::COLORREF color);
	int			setBkMode(int mode);
	bool		setColorAdjustment(const ::COLORADJUSTMENT& colorAdjust);
	int			setPolyFillMode(int mode);
	int			setROP2(int mode);
	int			setStretchBltMode(int mode);
	::COLORREF	setTextColor(::COLORREF color);
	// mapping
	int		getMapMode() const;
	::SIZE	getViewportExt() const;
	::POINT	getViewportOrg() const;
	::SIZE	getWindowExt() const;
	::POINT	getWindowOrg() const;
	bool	offsetViewportOrg(int dx, int dy, ::POINT* original = 0);
	bool	offsetWindowOrg(int dx, int dy, ::POINT* original = 0);
	bool	scaleViewportExt(int xNum, int xDenom, int yNum, int yDenom, ::SIZE* original = 0);
	bool	scaleWindowExt(int xNum, int xDenom, int yNum, int yDenom, ::SIZE* original = 0);
	int		setMapMode(int newMode);
	bool	setViewportExt(int cx, int cy, ::SIZE* original = 0);
	bool	setViewportExt(const ::SIZE& size, ::SIZE* original = 0);
	bool	setViewportOrg(int x, int y, ::POINT* original = 0);
	bool	setViewportOrg(const ::POINT& p, ::POINT* original = 0);
	bool	setWindowExt(int cx, int cy, ::SIZE* original = 0);
	bool	setWindowExt(const ::SIZE& size, ::SIZE* original = 0);
	bool	setWindowOrg(int x, int y, ::POINT* original = 0);
	bool	setWindowOrg(const ::POINT& p, ::POINT* original = 0);
#ifdef LAYOUT_RTL
	// layout
	DWORD	getLayout() const;
	DWORD	setLayout(DWORD layout);
#endif /* LAYOUT_RTL */
	// coordinates
	bool	dpToLP(::POINT ps[], int c) const;
	bool	dpToLP(::POINT& p) const;
	bool	dpToLP(::SIZE& s) const;
	bool	dpToLP(::RECT& rc) const;
	bool	lpToDP(::POINT ps[], int c) const;
	bool	lpToDP(::POINT& p) const;
	bool	lpToDP(::SIZE& s) const;
	bool	lpToDP(::RECT& rc) const;
	// regions
	bool	fillRgn(::HRGN region, ::HBRUSH brush);
	bool	frameRgn(::HRGN region, ::HBRUSH brush, int width, int height);
	bool	invertRgn(::HRGN region);
	bool	paintRgn(::HRGN region);
	// clipping
	int		excludeClipRect(int x1, int y1, int x2, int y2);
	int		excludeClipRect(const ::RECT& rect);
	int		excludeUpdateRgn(::HWND window);
	::UINT	getBoundsRect(::RECT& rect, UINT flags);
	int		getClipBox(::RECT& rect) const;
	int		intersectClipRect(int x1, int y1, int x2, int y2);
	int		intersectClipRect(const ::RECT& rect);
	int		offsetClipRgn(int x, int y);
	int		offsetClipRgn(const ::SIZE& size);
	bool	ptVisible(int x, int y) const;
	bool	ptVisible(const ::POINT& pt) const;
	bool	rectVisible(const ::RECT& rect) const;
	int		selectClipRgn(::HRGN region);
	int		selectClipRgn(::HRGN region, int mode);
	::UINT	setBoundsRect(const ::RECT& rect, ::UINT flags);
	// lines
	bool	angleArc(int x, int y, int radius, float startAngle, float sweepAngle);
	bool	arc(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);
	bool	arc(const ::RECT& rect, const ::POINT& start, const ::POINT& end);
	bool	arcTo(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);
	bool	arcTo(const ::RECT& rect, const ::POINT& start, const ::POINT& end);
	int		getArcDirection() const;
	::POINT	getCurrentPosition() const;
	bool	lineTo(int x, int y);
	bool	lineTo(const ::POINT& pt);
	::POINT	moveTo(int x, int y);
	::POINT	moveTo(const ::POINT& pt);
	bool	polyBezier(const ::POINT* points, int count);
	bool	polyBezierTo(const ::POINT* points, int count);
	bool	polyDraw(const ::POINT* points, const ::BYTE* types, int count);
	bool	polyline(::LPPOINT points, int count);
	bool	polylineTo(const ::POINT* points, int count);
	bool	polyPolyline(const ::POINT* points, const DWORD* polyPoints, int count);
	int		setArcDirection(int direction);
	// simple
//	void	draw3dRect(const ::RECT* rect, COLORREF topLeftColor, COLORREF rightBottomColor);
//	void	draw3dRect(int x, int y, int cx, int cy, COLORREF topLeftColor, COLORREF rightBottomColor);
//	void	drawDragIcon(const ::RECT* rect, ::SIZE size,
//				const ::RECT* lastRect, ::SIZE sizeLast, HBRUSH brush, HBRUSH lastBrush);
	bool	drawEdge(const ::RECT& rect, ::UINT edge, ::UINT flags);
	bool	drawFrameControl(const ::RECT& rect, ::UINT type, ::UINT state);
	bool	drawIcon(int x, int y, ::HICON icon);
	bool	drawIcon(const ::POINT& pt, ::HICON icon);
	bool	drawIconEx(int x, int y, ::HICON icon, int cx, int cy, ::UINT animationCursorStep, ::HBRUSH flicker, ::UINT flags);
	bool	drawIconEx(const ::POINT& pt, ::HICON icon, int cx, int cy, ::UINT animationCursorStep, ::HBRUSH flicker, ::UINT flags);
	bool	drawIconEx(int x, int y, ::HICON icon, const ::SIZE& size, ::UINT animationCursorStep, ::HBRUSH flicker, ::UINT flags);
	bool	drawIconEx(const ::POINT& pt, ::HICON icon, const ::SIZE& size, ::UINT animationCursorStep, ::HBRUSH flicker, ::UINT flags);
	void	fillRect(const ::RECT& rect, ::HBRUSH brush);
	void	fillSolidRect(const ::RECT& rect, ::COLORREF color);
	void	fillSolidRect(int x, int y, int cx, int cy, ::COLORREF color);
	void	frameRect(const ::RECT& rect, ::HBRUSH brush);
	void	invertRect(const ::RECT& rect);
	// ovals and polygons
	bool	chord(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);
	bool	chord(const ::RECT& rect, const ::POINT& start, const ::POINT& end);
	void	drawFocusRect(const ::RECT& rect);
	bool	ellipse(int x1, int y1, int x2, int y2);
	bool	ellipse(const ::RECT& rect);
	bool	pie(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);
	bool	pie(const ::RECT& rect, const ::POINT& start, const ::POINT& end);
	bool	polygon(const ::POINT* points, int count);
	bool	polyPolygon(const ::POINT* points, const int* lpPolyCounts, int count);
	bool	rectangle(int x1, int y1, int x2, int y2);
	bool	rectangle(const ::RECT& rect);
	bool	roundRect(int x1, int y1, int x2, int y2, int x3, int y3);
	bool	roundRect(const ::RECT& rect, const ::POINT& pt);
	// bitmaps
	bool		bitBlt(int x, int y, int width, int height, ::HDC srcDC, int xSrc, int ySrc, DWORD rop);
	bool		extFloodFill(int x, int y, ::COLORREF color, ::UINT fillType);
	bool		floodFill(int x, int y, ::COLORREF color);
	::COLORREF	getPixel(int x, int y) const;
	::COLORREF	getPixel(const ::POINT& pt) const;
	bool		maskBlt(int x, int y, int width, int height,
					::HDC dc, int xSrc, int ySrc, ::HBITMAP bitmap, int xMask, int yMask, ::DWORD rop);
	bool		patBlt(int x, int y, int width, int height, ::DWORD rop);
	bool		patBlt(const ::RECT& rect, ::DWORD rop);
	bool		plgBlt(const ::POINT* point, ::HDC dc,
					int xSrc, int ySrc, int width, int height, ::HBITMAP bitmap, int xMask, int yMask);
	::COLORREF	setPixel(int x, int y, ::COLORREF color);
	::COLORREF	setPixel(const ::POINT& pt, ::COLORREF color);
	bool		setPixelV(int x, int y, ::COLORREF color);
	bool		setPixelV(const ::POINT& pt, ::COLORREF color);
	bool		stretchBlt(int x, int y, int width, int height,
					::HDC srcDC, int xSrc, int ySrc, int srcWidth, int srcHeight, ::DWORD rop);
	// textual
	virtual int		drawText(const ::WCHAR* text, int length, const ::RECT& rect, ::UINT format);
	virtual bool	extTextOut(int x, int y, ::UINT options, const ::RECT* rect, const ::WCHAR* text, ::UINT length, const int* dxWidths);
	::SIZE			getCharacterPlacement(const ::WCHAR* text, int length, int maxExtent, ::GCP_RESULTSW& results, ::DWORD flags) const;
	::SIZE			getTabbedTextExtent(const ::WCHAR* text, int length, int tabCount, int* tabStopPositions) const;
	::UINT			getTextAlign() const;
	int				getTextCharacterExtra() const;
	::SIZE			getTextExtent(const ::WCHAR* text, int length) const;
	bool			getTextExtentExPoint(const ::WCHAR* text, int length, int maxExtent, ::LPINT fit, ::LPINT dx, ::LPSIZE size) const;
#if(_WIN32_WINNT >= 0x0500)
	bool			getTextExtentExPointI(::LPWORD glyphs, int count, int maxExtent, ::LPINT fit, ::LPINT dx, ::LPSIZE size) const;
	bool			getTextExtentPointI(::LPWORD glyphs, int count, ::LPSIZE size) const;
#endif /* _WIN32_WINNT >= 0x0500 */
	int				getTextFace(int faceNameCount, ::WCHAR* faceName) const;
	bool			getTextMetrics(::TEXTMETRICW& metrics) const;
	virtual bool	grayString(::HBRUSH brush, ::GRAYSTRINGPROC outputProc, ::LPARAM data, int length, int x, int y, int width, int height);
	bool			polyTextOut(const ::POLYTEXTW* texts, int count);
	::UINT			setTextAlign(::UINT flags);
	int				setTextCharacterExtra(int charExtra);
	int				setTextJustification(int breakExtra, int breakCount);
	virtual ::SIZE	tabbedTextOut(int x, int y, const ::WCHAR* text, int length, int tabCount, int* tabStopPositions, int tabOrigin);
	virtual bool	textOut(int x, int y, const ::WCHAR* text, int length);
	// fonts
	int		enumFontFamilies(const ::WCHAR* name, ::FONTENUMPROCW proc, ::LPARAM param = 0UL) const;
	int		enumFontFamilies(const ::LOGFONTW& condition, ::FONTENUMPROCW proc, ::LPARAM param = 0UL) const;
	bool	getAspectRatioFilterEx(::SIZE& size) const;
	bool	getCharABCWidths(::UINT firstChar, ::UINT lastChar, ::ABC buffer[]) const;
	bool	getCharABCWidths(::UINT firstChar, ::UINT lastChar, ::ABCFLOAT buffer[]) const;
	bool	getCharWidth(::UINT firstChar, ::UINT lastChar, int* buffer) const;
	bool	getCharWidth(::UINT firstChar, ::UINT lastChar, float* buffer) const;
#if(_WIN32_WINNT >= 0x0500)
	bool	getCharWidthI(::UINT firstGlyph, ::UINT numberOfGlyphs, int buffer[]) const;
	bool	getCharWidthI(::WORD* glyphs, ::UINT numberOfGlyphs, int buffer[]) const;
	bool	getCharABCWidthsI(::UINT firstGlyph, ::UINT numberOfGlyphs, ::ABC buffer[]) const;
	bool	getCharABCWidthsI(const ::WORD glyphs[], ::UINT numberOfGlyphs, ::ABC buffer[]) const;
#endif /* _WIN32_WINNT >= 0x0500 */
	::DWORD	getFontData(DWORD table, DWORD offset, LPVOID data, DWORD bytes) const;
	::DWORD	getFontLanguageInfo() const;
#if(_WIN32_WINNT >= 0x0500)
	::DWORD	getFontUnicodeRanges(::GLYPHSET& glyphSet) const;
	::DWORD	getGlyphIndices(const ::WCHAR* text, int length, ::WORD* indices, ::DWORD flags) const;
#endif /* _WIN32_WINNT >= 0x0500 */
	::DWORD	getGlyphOutline(::UINT ch, ::UINT format, ::LPGLYPHMETRICS gm, ::DWORD bufferSize, ::LPVOID data, const ::MAT2* mat2) const;
	::DWORD	getKerningPairs(::DWORD pairs, ::LPKERNINGPAIR kerningPair) const;
	::UINT	getOutlineTextMetrics(::UINT dataSize, ::LPOUTLINETEXTMETRICW otm) const;
	bool	getRasterizerCaps(::RASTERIZER_STATUS& status, ::UINT cb) const;
	::DWORD	setMapperFlags(::DWORD flag);
	// printer escapement
	int	abortDoc();
	int	drawEscape(int escape, int bytes, const char input[]);
	int	endDoc();
	int	endPage();
	int	escape(int escape, int bytes, const char input[], void* output);
	int	setAbortProc(::ABORTPROC procedure);
	int	startDoc(const ::DOCINFOW& docInfo);
	int	startPage();
	// scroll
	bool	scroll(int dx, int dy, const ::RECT& scrollRect, const ::RECT& clipRect, ::HRGN updateRegion, ::RECT* updateRect);
protected:
	explicit DC(::HDC handle) : Handle<::HDC, ::DeleteDC>(handle) {}
	void	assertValidAsDC() const;
};

namespace internal {
	class WindowRelatedDC : public DC {
	protected:
		WindowRelatedDC(::HWND window, ::HDC handle) : DC(handle), window_(window), refCount_(new ulong) {
			assert(window_ == 0 || toBoolean(::IsWindow(window_))); *refCount_ = 1; assert(getHandle() != 0);}
		WindowRelatedDC(const WindowRelatedDC& rhs) : DC(rhs), window_(rhs.window_), refCount_(rhs.refCount_) {++*refCount_;}
		virtual ~WindowRelatedDC() {::HDC handle = detach(); if(--*refCount_ == 0) {::ReleaseDC(window_, handle); delete refCount_;}}
	private:
		::HWND window_;
		ulong* refCount_;
	};
}

class PaintDC : public DC {
public:
	PaintDC(::HWND window) : data_(new Data) {assert(toBoolean(::IsWindow(window)));
		attach(::BeginPaint(data_->window = window, &data_->paint)); data_->refCount = 1; data_->createdByWindow = false; assert(getHandle() != 0);}
	PaintDC(const PaintDC& rhs) : DC(rhs), data_(rhs.data_) {++data_->refCount;}
	~PaintDC() {if(--data_->refCount == 0) {if(!data_->createdByWindow) ::EndPaint(data_->window, &data_->paint); delete data_;}}
	const ::PAINTSTRUCT& getPaintStruct() const {return data_->paint;}
private:
	PaintDC(::HWND window, ::PAINTSTRUCT& paint) : data_(new Data) {assert(toBoolean(::IsWindow(window)));
		data_->window = window; attach((data_->paint = paint).hdc); data_->refCount = 1; data_->createdByWindow = true; assert(getHandle() != 0);}
	struct Data {
		::HWND window;
		::PAINTSTRUCT paint;
		ulong refCount;
		bool createdByWindow;
	}* data_;
	friend class ui::Window;
};

class ClientDC : public internal::WindowRelatedDC {
public:
	ClientDC(const ClientDC& rhs) : internal::WindowRelatedDC(rhs) {}
	virtual ~ClientDC() {}
protected:
	explicit ClientDC(::HWND window) : internal::WindowRelatedDC(window, ::GetDC(window)) {}
	ClientDC(::HWND window, ::HRGN clipRegion, ::DWORD flags) : internal::WindowRelatedDC(window, ::GetDCEx(window, clipRegion, flags)) {}
	friend class ui::Window;
};

class WindowDC : public internal::WindowRelatedDC {
public:
	WindowDC(const WindowDC& rhs) : internal::WindowRelatedDC(rhs) {}
private:
	WindowDC(::HWND window) : internal::WindowRelatedDC(window, ::GetWindowDC(window)) {}
	friend class ui::Window;
};

class ScreenDC : public ClientDC {
public:
	ScreenDC() : ClientDC(0) {}
};


// DC ///////////////////////////////////////////////////////////////////////

inline int DC::abortDoc() {assertValidAsDC(); return ::AbortDoc(getHandle());}

inline bool DC::angleArc(int x, int y, int radius, float startAngle, float sweepAngle) {
	assertValidAsDC(); return toBoolean(::AngleArc(getHandle(), x, y, radius, startAngle, sweepAngle));}

inline bool DC::arc(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4) {
	assertValidAsDC(); return toBoolean(::Arc(getHandle(), x1, y1, x2, y2, x3, y3, x4, y4));}

inline bool DC::arc(const ::RECT& rect, const ::POINT& start, const ::POINT& end) {
	return arc(rect.left, rect.top, rect.right, rect.bottom, start.x, start.y, end.x, end.y);}

inline bool DC::arcTo(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4) {
	assertValidAsDC(); return toBoolean(::ArcTo(getHandle(), x1, y1, x2, y2, x3, y3, x4, y4));}

inline bool DC::arcTo(const ::RECT& rect, const ::POINT& start, const ::POINT& end) {
	return arcTo(rect.left, rect.top, rect.right, rect.bottom, start.x, start.y, end.x, end.y);}

inline void DC::assertValidAsDC() const {assert(getHandle() != 0);}

inline bool DC::bitBlt(int x, int y, int width, int height, ::HDC srcDC, int xSrc, int ySrc, ::DWORD rop) {
	assertValidAsDC(); return toBoolean(::BitBlt(getHandle(), x, y, width, height, srcDC, xSrc, ySrc, rop));}

inline bool DC::chord(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4) {
	assertValidAsDC(); return toBoolean(::Chord(getHandle(), x1, y1, x2, y2, x3, y3, x4, y4));}

inline bool DC::chord(const ::RECT& rect, const ::POINT& start, const ::POINT& end) {
	return chord(rect.left, rect.top, rect.right, rect.bottom, start.x, start.y, end.x, end.y);}

inline std::auto_ptr<DC> DC::createCompatibleDC() const {assertValidAsDC(); return std::auto_ptr<DC>(new DC(::CreateCompatibleDC(getHandle())));}

inline bool DC::dpToLP(::POINT ps[], int c) const {assertValidAsDC(); return toBoolean(::DPtoLP(getHandle(), ps, c));}

inline bool DC::dpToLP(::POINT& p) const {return dpToLP(&p, 1);}

inline bool DC::dpToLP(::SIZE& s) const {return dpToLP(reinterpret_cast<::POINT*>(&s), 2);}

inline bool DC::dpToLP(::RECT& rc) const {return dpToLP(reinterpret_cast<::POINT*>(&rc), 4);}

inline bool DC::drawEdge(const ::RECT& rect, ::UINT edge, ::UINT flags) {
	assertValidAsDC(); return toBoolean(::DrawEdge(getHandle(), const_cast<RECT*>(&rect), edge, flags));}

inline int DC::drawEscape(int escape, int bytes, const char input[]) {assertValidAsDC(); return ::DrawEscape(getHandle(), escape, bytes, input);}

inline void DC::drawFocusRect(const ::RECT& rect) {assertValidAsDC(); ::DrawFocusRect(getHandle(), &rect);}

inline bool DC::drawFrameControl(const ::RECT& rect, ::UINT type, ::UINT state) {
	assertValidAsDC(); return toBoolean(::DrawFrameControl(getHandle(), const_cast<RECT*>(&rect), type, state));}

inline bool DC::drawIcon(int x, int y, ::HICON icon) {assertValidAsDC(); return toBoolean(::DrawIcon(getHandle(), x, y, icon));}

inline bool DC::drawIcon(const ::POINT& pt, ::HICON icon) {return drawIcon(pt.x, pt.y, icon);}

inline bool DC::drawIconEx(int x, int y, ::HICON icon, int cx, int cy, ::UINT animationCursorStep, ::HBRUSH flicker, ::UINT flags) {
	assertValidAsDC(); return toBoolean(::DrawIconEx(getHandle(), x, y, icon, cx, cy, animationCursorStep, flicker, flags));}

inline bool DC::drawIconEx(const ::POINT& pt, ::HICON icon, int cx, int cy, ::UINT animationCursorStep, ::HBRUSH flicker, ::UINT flags) {
	return drawIconEx(pt.x, pt.y, icon, cx, cy, animationCursorStep, flicker, flags);}

inline bool DC::drawIconEx(int x, int y, ::HICON icon, const ::SIZE& size, ::UINT animationCursorStep, ::HBRUSH flicker, ::UINT flags) {
	return drawIconEx(x, y, icon, size.cx, size.cy, animationCursorStep, flicker, flags);}

inline bool DC::drawIconEx(const ::POINT& pt, ::HICON icon, const ::SIZE& size, ::UINT animationCursorStep, ::HBRUSH flicker, ::UINT flags) {
	return drawIconEx(pt.x, pt.y, icon, size.cx, size.cy, animationCursorStep, flicker, flags);}

inline int DC::drawText(const ::WCHAR* text, int length, const ::RECT& rect, ::UINT format) {
	assertValidAsDC(); return ::DrawTextW(getHandle(), text, length, const_cast<::RECT*>(&rect), format);}

inline bool DC::ellipse(int x1, int y1, int x2, int y2) {assertValidAsDC(); return toBoolean(::Ellipse(getHandle(), x1, y1, x2, y2));}

inline bool DC::ellipse(const ::RECT& rect) {return ellipse(rect.left, rect.top, rect.right, rect.bottom);}

inline int DC::endDoc() {assertValidAsDC(); return ::EndDoc(getHandle());}

inline int DC::endPage() {assertValidAsDC(); return ::EndPage(getHandle());}

inline int DC::enumFontFamilies(const ::WCHAR* name, ::FONTENUMPROCW proc, ::LPARAM param /* = 0UL */) const {
	assertValidAsDC(); return ::EnumFontFamiliesW(getHandle(), name, proc, param);}

inline int DC::enumFontFamilies(const ::LOGFONTW& condition, ::FONTENUMPROCW proc, ::LPARAM param /* = 0UL */) const {
	assertValidAsDC(); return ::EnumFontFamiliesExW(getHandle(), const_cast<::LOGFONTW*>(&condition), proc, param, 0);}

inline int DC::enumObjects(int objectType, ::GOBJENUMPROC proc, ::LPARAM data) {
	assertValidAsDC(); return ::EnumObjects(getHandle(), objectType, proc, data);}

inline int DC::escape(int escape, int bytes, const char input[], void* output) {assertValidAsDC(); return ::Escape(getHandle(), escape, bytes, input, output);}

inline int DC::excludeClipRect(int x1, int y1, int x2, int y2) {assertValidAsDC(); return ::ExcludeClipRect(getHandle(), x1, y1, x2, y2);}

inline int DC::excludeClipRect(const ::RECT& rect) {return excludeClipRect(rect.left, rect.top, rect.right, rect.bottom);}

inline int DC::excludeUpdateRgn(::HWND window) {assertValidAsDC(); return ::ExcludeUpdateRgn(getHandle(), window);}

inline bool DC::extFloodFill(int x, int y, ::COLORREF color, ::UINT fillType) {
	assertValidAsDC(); return toBoolean(::ExtFloodFill(getHandle(), x, y, color, fillType));}

inline bool DC::extTextOut(int x, int y, ::UINT options,
		const ::RECT* rect, const ::WCHAR* text, ::UINT length, const int* dxWidths) {
	assertValidAsDC(); return toBoolean(::ExtTextOutW(getHandle(), x, y, options, rect, text, length, dxWidths));}

inline void DC::fillRect(const ::RECT& rect, ::HBRUSH brush) {assertValidAsDC(); ::FillRect(getHandle(), &rect, brush);}

inline bool DC::fillRgn(::HRGN region, ::HBRUSH brush) {assertValidAsDC(); return toBoolean(::FillRgn(getHandle(), region, brush));}

inline void DC::fillSolidRect(const ::RECT& rect, ::COLORREF color) {
	fillSolidRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, color);}

inline void DC::fillSolidRect(int x, int y, int cx, int cy, ::COLORREF color) {
	assertValidAsDC();
	::COLORREF org = getBkColor();
	::RECT rect;
	::SetRect(&rect, x, y, x + cx, y + cy);
	setBkColor(color);
	extTextOut(0, 0, ETO_IGNORELANGUAGE | ETO_OPAQUE, &rect, L"", 0, 0);
	setBkColor(org);
}

inline bool DC::floodFill(int x, int y, ::COLORREF color) {assertValidAsDC(); return toBoolean(::FloodFill(getHandle(), x, y, color));}

inline bool DC::frameRgn(::HRGN region, ::HBRUSH brush, int width, int height) {
	assertValidAsDC(); return toBoolean(::FrameRgn(getHandle(), region, brush, width, height));}

inline void DC::frameRect(const ::RECT& rect, ::HBRUSH brush) {assertValidAsDC(); ::FrameRect(getHandle(), &rect, brush);}

inline int DC::getArcDirection() const {assertValidAsDC(); return ::GetArcDirection(getHandle());}

inline bool DC::getAspectRatioFilterEx(::SIZE& size) const {
	assertValidAsDC(); return toBoolean(::GetAspectRatioFilterEx(getHandle(), &size));}

inline ::COLORREF DC::getBkColor() const {assertValidAsDC(); return ::GetBkColor(getHandle());}

inline int DC::getBkMode() const {assertValidAsDC(); return ::GetBkMode(getHandle());}

inline ::UINT DC::getBoundsRect(::RECT& rect, ::UINT flags) {assertValidAsDC(); return ::GetBoundsRect(getHandle(), &rect, flags);}

inline ::POINT DC::getBrushOrg() const {assertValidAsDC(); POINT pt; ::GetBrushOrgEx(getHandle(), &pt); return pt;}

inline ::SIZE DC::getCharacterPlacement(const ::WCHAR* text, int length, int maxExtent, ::GCP_RESULTSW& results, ::DWORD flags) const {
	assertValidAsDC();
	const ::DWORD res = ::GetCharacterPlacementW(getHandle(), text, length, maxExtent, &results, flags);
	const ::SIZE size = {LOWORD(res), HIWORD(res)};
	return size;
}

inline bool DC::getCharABCWidths(::UINT firstChar, ::UINT lastChar, ::ABC buffer[]) const {
	assertValidAsDC(); return toBoolean(::GetCharABCWidthsW(getHandle(), firstChar, lastChar, buffer));}

inline bool DC::getCharABCWidths(::UINT firstChar, ::UINT lastChar, ::ABCFLOAT buffer[]) const {
	assertValidAsDC(); return toBoolean(::GetCharABCWidthsFloatW(getHandle(), firstChar, lastChar, buffer));}

#if(_WIN32_WINNT >= 0x0500)
inline bool DC::getCharABCWidthsI(::UINT firstGlyph, ::UINT numberOfGlyphs, ::ABC buffer[]) const {
	assertValidAsDC(); return toBoolean(::GetCharABCWidthsI(getHandle(), firstGlyph, numberOfGlyphs, 0, buffer));}

inline bool DC::getCharABCWidthsI(const ::WORD glyphs[], ::UINT numberOfGlyphs, ::ABC buffer[]) const {
	assertValidAsDC(); return toBoolean(::GetCharABCWidthsI(getHandle(), 0, numberOfGlyphs, const_cast<WORD*>(glyphs), buffer));}
#endif /* _WIN32_WINNT >= 0x0500 */

inline bool DC::getCharWidth(::UINT firstChar, ::UINT lastChar, int* buffer) const {
	assertValidAsDC(); return toBoolean(::GetCharWidth32W(getHandle(), firstChar, lastChar, buffer));}

inline bool DC::getCharWidth(::UINT firstChar, ::UINT lastChar, float* buffer) const {
	assertValidAsDC(); return toBoolean(::GetCharWidthFloatW(getHandle(), firstChar, lastChar, buffer));}

#if(_WIN32_WINNT >= 0x0500)
inline bool DC::getCharWidthI(::UINT firstGlyph, ::UINT numberOfGlyphs, int buffer[]) const {
	assertValidAsDC(); return toBoolean(::GetCharWidthI(getHandle(), firstGlyph, numberOfGlyphs, 0, buffer));}

inline bool DC::getCharWidthI(::WORD* glyphs, ::UINT numberOfGlyphs, int buffer[]) const {
	assertValidAsDC(); return toBoolean(::GetCharWidthI(getHandle(), 0, numberOfGlyphs, const_cast<WORD*>(glyphs), buffer));}
#endif /* _WIN32_WINNT >= 0x0500 */

inline int DC::getClipBox(::RECT& rect) const {assertValidAsDC(); return ::GetClipBox(getHandle(), &rect);}

inline bool DC::getColorAdjustment(::COLORADJUSTMENT& colorAdjust) const {
	assertValidAsDC(); return toBoolean(::GetColorAdjustment(getHandle(), &colorAdjust));}

inline ::HBITMAP DC::getCurrentBitmap() const {
	assertValidAsDC(); return reinterpret_cast<::HBITMAP>(::GetCurrentObject(getHandle(), OBJ_BITMAP));}

inline ::HBRUSH DC::getCurrentBrush() const {
	assertValidAsDC(); return reinterpret_cast<::HBRUSH>(::GetCurrentObject(getHandle(), OBJ_BRUSH));}

inline ::HFONT DC::getCurrentFont() const {assertValidAsDC(); return reinterpret_cast<::HFONT>(::GetCurrentObject(getHandle(), OBJ_FONT));}

inline ::HPALETTE DC::getCurrentPalette() const {assertValidAsDC(); return reinterpret_cast<::HPALETTE>(::GetCurrentObject(getHandle(), OBJ_PAL));}

inline ::HPEN DC::getCurrentPen() const {assertValidAsDC(); return reinterpret_cast<::HPEN>(::GetCurrentObject(getHandle(), OBJ_PEN));}

inline ::POINT DC::getCurrentPosition() const {
	assertValidAsDC();
	::POINT pt;
	::GetCurrentPositionEx(getHandle(), &pt);
	return pt;
}

inline int DC::getDeviceCaps(int index) const {assertValidAsDC(); return ::GetDeviceCaps(getHandle(), index);}

inline ::DWORD DC::getFontData(::DWORD table, ::DWORD offset, ::LPVOID data, ::DWORD bytes) const {
	assertValidAsDC(); return ::GetFontData(getHandle(), table, offset, data, bytes);}

inline ::DWORD DC::getFontLanguageInfo() const {assertValidAsDC(); return ::GetFontLanguageInfo(getHandle());}

#if(_WIN32_WINNT >= 0x0500)
inline ::DWORD DC::getFontUnicodeRanges(::GLYPHSET& glyphSet) const {assertValidAsDC(); return ::GetFontUnicodeRanges(getHandle(), &glyphSet);}

inline ::DWORD DC::getGlyphIndices(const ::WCHAR* text, int length, ::WORD* indices, ::DWORD flags) const {
	assertValidAsDC(); return ::GetGlyphIndicesW(getHandle(), text, length, indices, flags);}
#endif

inline ::DWORD DC::getGlyphOutline(::UINT ch, ::UINT format, ::LPGLYPHMETRICS gm, ::DWORD bufferSize, ::LPVOID data, const ::MAT2* mat2) const {
	assertValidAsDC(); return ::GetGlyphOutlineW(getHandle(), ch, format, gm, bufferSize, data, mat2);}

inline ::DWORD DC::getKerningPairs(::DWORD count, ::LPKERNINGPAIR kerningPairs) const {
	assertValidAsDC(); return ::GetKerningPairs(getHandle(), count, kerningPairs);}

#ifdef LAYOUT_RTL
inline ::DWORD DC::getLayout() const {assertValidAsDC(); return ::GetLayout(getHandle());}
#endif /* LAYOUT_RTL */

inline int DC::getMapMode() const {assertValidAsDC(); return ::GetMapMode(getHandle());}

inline ::COLORREF DC::getNearestColor(::COLORREF color) const {assertValidAsDC(); return ::GetNearestColor(getHandle(), color);}

inline ::UINT DC::getOutlineTextMetrics(::UINT bytes, ::LPOUTLINETEXTMETRICW otm) const {
	assertValidAsDC(); return ::GetOutlineTextMetricsW(getHandle(), bytes, otm);}

inline ::COLORREF DC::getPixel(int x, int y) const {assertValidAsDC(); return ::GetPixel(getHandle(), x, y);}

inline ::COLORREF DC::getPixel(const ::POINT& pt) const {return getPixel(pt.x, pt.y);}

inline int DC::getPolyFillMode() const {assertValidAsDC(); return ::GetPolyFillMode(getHandle());}

inline bool DC::getRasterizerCaps(::RASTERIZER_STATUS& status, ::UINT cb) const {assertValidAsDC(); return toBoolean(::GetRasterizerCaps(&status, cb));}

inline int DC::getROP2() const {assertValidAsDC(); return ::GetROP2(getHandle());}

inline int DC::getStretchBltMode() const {assertValidAsDC(); return ::GetStretchBltMode(getHandle());}

inline SIZE DC::getTabbedTextExtent(const ::WCHAR* text, int length, int tabCount, int* tabStopPositions) const {
	assertValidAsDC();
	const ::DWORD	res = ::GetTabbedTextExtentW(getHandle(), text, length, tabCount, tabStopPositions);
	const ::SIZE size = {LOWORD(res), HIWORD(res)};
	return size;
}

inline ::UINT DC::getTextAlign() const {assertValidAsDC(); return ::GetTextAlign(getHandle());}

inline int DC::getTextCharacterExtra() const {assertValidAsDC(); return ::GetTextCharacterExtra(getHandle());}

inline ::COLORREF DC::getTextColor() const {assertValidAsDC(); return ::GetTextColor(getHandle());}

inline ::SIZE DC::getTextExtent(const ::WCHAR* text, int length) const {
	assertValidAsDC();
	::SIZE size;
	::GetTextExtentPoint32W(getHandle(), text, length, &size);
	return size;
}

inline bool DC::getTextExtentExPoint(const ::WCHAR* text, int length, int maxExtent, ::LPINT fit, ::LPINT dx, ::LPSIZE size) const {
	assertValidAsDC(); return toBoolean(::GetTextExtentExPointW(getHandle(), text, length, maxExtent, fit, dx, size));}

#if(_WIN32_WINNT >= 0x0500)
inline bool DC::getTextExtentExPointI(::LPWORD glyphs, int count, int maxExtent, ::LPINT fit, ::LPINT dx, ::LPSIZE size) const {
	assertValidAsDC(); return toBoolean(::GetTextExtentExPointI(getHandle(), glyphs, count, maxExtent, fit, dx, size));}
#endif /* _WIN32_WINNT >= 0x0500 */

#if(_WIN32_WINNT >= 0x0500)
inline bool DC::getTextExtentPointI(::LPWORD glyphs, int count, ::LPSIZE size) const {
	assertValidAsDC(); return toBoolean(::GetTextExtentPointI(getHandle(), glyphs, count, size));}
#endif /* _WIN32_WINNT >= 0x0500 */

inline int DC::getTextFace(int maxLength, ::WCHAR* faceName) const {assertValidAsDC(); return ::GetTextFaceW(getHandle(), maxLength, faceName);}

inline bool DC::getTextMetrics(::TEXTMETRICW& metrics) const {assertValidAsDC(); return toBoolean(::GetTextMetricsW(getHandle(), &metrics));}

inline ::SIZE DC::getViewportExt() const {assertValidAsDC(); ::SIZE s; ::GetViewportExtEx(getHandle(), &s); return s;}

inline ::POINT DC::getViewportOrg() const {assertValidAsDC(); ::POINT p; ::GetViewportOrgEx(getHandle(), &p); return p;}

inline ::HWND DC::getWindow() const {assertValidAsDC(); return ::WindowFromDC(getHandle());}

inline ::SIZE DC::getWindowExt() const {assertValidAsDC(); ::SIZE s; ::GetWindowExtEx(getHandle(), &s); return s;}

inline ::POINT DC::getWindowOrg() const {assertValidAsDC(); ::POINT p; ::GetWindowOrgEx(getHandle(), &p); return p;}

inline bool DC::grayString(::HBRUSH brush, ::GRAYSTRINGPROC outputProc, ::LPARAM data, int length, int x, int y, int width, int height) {
	assertValidAsDC(); return toBoolean(::GrayStringW(getHandle(), brush, outputProc, data, length, x, y, width, height));}

inline int DC::intersectClipRect(int x1, int y1, int x2, int y2) {assertValidAsDC(); return ::IntersectClipRect(getHandle(), x1, y1, x2, y2);}

inline int DC::intersectClipRect(const ::RECT& rect) {return intersectClipRect(rect.left, rect.top, rect.right, rect.bottom);}

inline void DC::invertRect(const ::RECT& rect) {assertValidAsDC(); ::InvertRect(getHandle(), &rect);}

inline bool DC::invertRgn(::HRGN region) {assertValidAsDC(); return toBoolean(::InvertRgn(getHandle(), region));}

inline bool DC::lineTo(int x, int y) {assertValidAsDC(); return toBoolean(::LineTo(getHandle(), x, y));}

inline bool DC::lineTo(const ::POINT& pt) {return lineTo(pt.x, pt.y);}

inline bool DC::lpToDP(::POINT ps[], int c) const {assertValidAsDC(); return toBoolean(::LPtoDP(getHandle(), ps, c));}

inline bool DC::lpToDP(::POINT& p) const {return lpToDP(&p, 1);}

inline bool DC::lpToDP(::SIZE& s) const {return lpToDP(reinterpret_cast<::POINT*>(&s), 2);}

inline bool DC::lpToDP(::RECT& rc) const {return lpToDP(reinterpret_cast<::POINT*>(&rc), 4);}

inline bool DC::maskBlt(int x, int y, int width, int height, ::HDC dc, int xSrc, int ySrc, ::HBITMAP bitmap, int xMask, int yMask, ::DWORD rop) {
	assertValidAsDC(); return toBoolean(::MaskBlt(getHandle(), x, y, width, height, dc, xSrc, ySrc, bitmap, xMask, yMask, rop));}

inline ::POINT DC::moveTo(int x, int y) {
	assertValidAsDC();
	::POINT pt;
	::MoveToEx(getHandle(), x, y, &pt);
	return pt;
}

inline ::POINT DC::moveTo(const ::POINT& pt) {return moveTo(pt.x, pt.y);}

inline int DC::offsetClipRgn(int x, int y) {assertValidAsDC(); return ::OffsetClipRgn(getHandle(), x, y);}

inline int DC::offsetClipRgn(const ::SIZE& size) {return offsetClipRgn(size.cx, size.cy);}

inline bool DC::offsetViewportOrg(int dx, int dy, ::POINT* original /* = 0 */) {
	assertValidAsDC(); return toBoolean(::OffsetViewportOrgEx(getHandle(), dx, dy, original));}

inline bool DC::offsetWindowOrg(int dx, int dy, ::POINT* original /* = 0 */) {
	assertValidAsDC(); return toBoolean(::OffsetWindowOrgEx(getHandle(), dx, dy, original));}

inline bool DC::paintRgn(::HRGN region) {assertValidAsDC(); return toBoolean(::PaintRgn(getHandle(), region));}

inline bool DC::patBlt(int x, int y, int width, int height, ::DWORD rop) {
	assertValidAsDC(); return toBoolean(::PatBlt(getHandle(), x, y, width, height, rop));}

inline bool DC::patBlt(const ::RECT& rect, ::DWORD rop) {return patBlt(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, rop);}

inline bool DC::pie(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4) {
	assertValidAsDC(); return toBoolean(::Pie(getHandle(), x1, y1, x2, y2, x3, y3, x4, y4));}

inline bool DC::pie(const ::RECT& rect, const ::POINT& start, const ::POINT& end) {
	return toBoolean(pie(rect.left, rect.top, rect.right, rect.bottom, start.x, start.y, end.x, end.y));}

inline bool DC::plgBlt(const ::POINT* point, ::HDC dc, int xSrc, int ySrc, int width, int height, ::HBITMAP bitmap, int xMask, int yMask) {
	assertValidAsDC(); return toBoolean(::PlgBlt(getHandle(), point, dc, xSrc, ySrc, width, height, bitmap, xMask, yMask));}

inline bool DC::polyBezier(const ::POINT* points, int count) {assertValidAsDC(); return toBoolean(::PolyBezier(getHandle(), points, count));}

inline bool DC::polyBezierTo(const ::POINT* points, int count) {assertValidAsDC(); return toBoolean(::PolyBezierTo(getHandle(), points, count));}

inline bool DC::polyDraw(const ::POINT* points, const ::BYTE* types, int count) {
	assertValidAsDC(); return toBoolean(::PolyDraw(getHandle(), points, types, count));}

inline bool DC::polygon(const ::POINT* points, int count) {assertValidAsDC(); return toBoolean(::Polygon(getHandle(), points, count));}

inline bool DC::polyline(::POINT points[], int count) {assertValidAsDC(); return toBoolean(::Polyline(getHandle(), points, count));}

inline bool DC::polylineTo(const ::POINT points[], int count) {assertValidAsDC(); return toBoolean(::PolylineTo(getHandle(), points, count));}

inline bool DC::polyPolygon(const ::POINT points[], const int polyCounts[], int count) {
	assertValidAsDC(); return toBoolean(::PolyPolygon(getHandle(), points, polyCounts, count));}

inline bool DC::polyPolyline(const POINT points[], const ::DWORD polyPoints[], int count) {
	assertValidAsDC(); return toBoolean(::PolyPolyline(getHandle(), points, polyPoints, count));}

inline bool DC::polyTextOut(const ::POLYTEXTW texts[], int count) {assertValidAsDC(); return toBoolean(::PolyTextOutW(getHandle(), texts, count));}

inline bool DC::ptVisible(int x, int y) const {assertValidAsDC(); return toBoolean(::PtVisible(getHandle(), x, y));}

inline bool DC::ptVisible(const ::POINT& pt) const {assertValidAsDC(); return toBoolean(::PtVisible(getHandle(), pt.x, pt.y));}

inline ::UINT DC::realizePalette() {assertValidAsDC(); return ::RealizePalette(getHandle());}

inline bool DC::rectangle(int x1, int y1, int x2, int y2) {assertValidAsDC(); return toBoolean(::Rectangle(getHandle(), x1, y1, x2, y2));}

inline bool DC::rectangle(const ::RECT& rect) {return rectangle(rect.left, rect.top, rect.right, rect.bottom);}

inline bool DC::rectVisible(const ::RECT& rect) const {assertValidAsDC(); return toBoolean(::RectVisible(getHandle(), &rect));}

inline bool DC::restore(int savedDC) {assertValidAsDC(); return toBoolean(::RestoreDC(getHandle(), savedDC));}

inline bool DC::roundRect(int x1, int y1, int x2, int y2, int x3, int y3) {
	assertValidAsDC(); return toBoolean(::RoundRect(getHandle(), x1, y1, x2, y2, x3, y3));}

inline bool DC::roundRect(const ::RECT& rect, const ::POINT& pt) {return roundRect(rect.left, rect.top, rect.right, rect.bottom, pt.x, pt.y);}

inline int DC::save() {assertValidAsDC(); return ::SaveDC(getHandle());}

inline bool DC::scaleViewportExt(int xNum, int xDenom, int yNum, int yDenom, ::SIZE* original /* = 0 */) {
	assertValidAsDC(); return toBoolean(::ScaleViewportExtEx(getHandle(), xNum, xDenom, yNum, yDenom, original));}

inline bool DC::scaleWindowExt(int xNum, int xDenom, int yNum, int yDenom, ::SIZE* original /* = 0 */) {
	assertValidAsDC(); return toBoolean(::ScaleWindowExtEx(getHandle(), xNum, xDenom, yNum, yDenom, original));}

inline bool DC::scroll(int dx, int dy, const ::RECT& scrollRect, const ::RECT& clipRect, ::HRGN updateRegion, ::RECT* updateRect) {
	assertValidAsDC(); return toBoolean(::ScrollDC(getHandle(), dx, dy, &scrollRect, &clipRect, updateRegion, updateRect));}

inline int DC::selectClipRgn(::HRGN region) {assertValidAsDC(); return ::SelectClipRgn(getHandle(), region);}

inline int DC::selectClipRgn(::HRGN region, int mode) {assertValidAsDC(); return ::ExtSelectClipRgn(getHandle(), region, mode);}

inline ::HBITMAP DC::selectObject(::HBITMAP bitmap) {assertValidAsDC(); return reinterpret_cast<::HBITMAP>(::SelectObject(getHandle(), bitmap));}

inline ::HBRUSH DC::selectObject(::HBRUSH brush) {assertValidAsDC(); return reinterpret_cast<::HBRUSH>(::SelectObject(getHandle(), brush));}

inline ::HFONT DC::selectObject(::HFONT font) {assertValidAsDC(); return reinterpret_cast<::HFONT>(::SelectObject(getHandle(), font));}

inline ::HPEN DC::selectObject(::HPEN pen) {assertValidAsDC(); return reinterpret_cast<::HPEN>(::SelectObject(getHandle(), pen));}

inline ::HPALETTE DC::selectPalette(::HPALETTE palette, bool forceBackground) {
	assertValidAsDC(); return ::SelectPalette(getHandle(), palette, forceBackground);}

inline ::HGDIOBJ DC::selectStockObject(int object) {assertValidAsDC(); return ::SelectObject(getHandle(), ::GetStockObject(object));}

inline int DC::setAbortProc(::ABORTPROC procedure) {assertValidAsDC(); return ::SetAbortProc(getHandle(), procedure);}

inline int DC::setArcDirection(int direction) {assertValidAsDC(); return ::SetArcDirection(getHandle(), direction);}

inline ::COLORREF DC::setBkColor(::COLORREF color) {assertValidAsDC(); return ::SetBkColor(getHandle(), color);}

inline int DC::setBkMode(int mode) {assertValidAsDC(); return ::SetBkMode(getHandle(), mode);}

inline ::UINT DC::setBoundsRect(const ::RECT& rect, ::UINT flags) {assertValidAsDC(); return ::SetBoundsRect(getHandle(), &rect, flags);}

inline ::POINT DC::setBrushOrg(int x, int y) {
	assertValidAsDC();
	::POINT pt;
	::SetBrushOrgEx(getHandle(), x, y, &pt);
	return pt;
}

inline ::POINT DC::setBrushOrg(const ::POINT& pt) {return setBrushOrg(pt.x, pt.y);}

inline bool DC::setColorAdjustment(const ::COLORADJUSTMENT& colorAdjust) {
	assertValidAsDC(); return toBoolean(::SetColorAdjustment(getHandle(), &colorAdjust));}

#ifdef LAYOUT_RTL
inline ::DWORD DC::setLayout(::DWORD layout) {assertValidAsDC(); return ::SetLayout(getHandle(), layout);}
#endif /* LAYOUT_RTL */

inline int DC::setMapMode(int newMode) {assertValidAsDC(); return ::SetMapMode(getHandle(), newMode);}

inline ::DWORD DC::setMapperFlags(::DWORD flag) {assertValidAsDC(); return ::SetMapperFlags(getHandle(), flag);}

inline ::COLORREF DC::setPixel(int x, int y, ::COLORREF color) {assertValidAsDC(); return ::SetPixel(getHandle(), x, y, color);}

inline ::COLORREF DC::setPixel(const ::POINT& pt, ::COLORREF color) {return setPixel(pt.x, pt.y, color);}

inline bool DC::setPixelV(int x, int y, ::COLORREF color) {assertValidAsDC(); return toBoolean(::SetPixelV(getHandle(), x, y, color));}

inline bool DC::setPixelV(const ::POINT& pt, ::COLORREF color) {return setPixelV(pt.x, pt.y, color);}

inline int DC::setPolyFillMode(int mode) {assertValidAsDC(); return ::SetPolyFillMode(getHandle(), mode);}

inline int DC::setROP2(int mode) {assertValidAsDC(); return ::SetROP2(getHandle(), mode);}

inline int DC::setStretchBltMode(int mode) {assertValidAsDC(); return ::SetStretchBltMode(getHandle(), mode);}

inline ::UINT DC::setTextAlign(::UINT flags) {assertValidAsDC(); return ::SetTextAlign(getHandle(), flags);}

inline int DC::setTextCharacterExtra(int charExtra) {assertValidAsDC(); return ::SetTextCharacterExtra(getHandle(), charExtra);}

inline ::COLORREF DC::setTextColor(::COLORREF color) {assertValidAsDC(); return ::SetTextColor(getHandle(), color);}

inline int DC::setTextJustification(int breakExtra, int breakCount) {
	assertValidAsDC(); return ::SetTextJustification(getHandle(), breakExtra, breakCount);}

inline bool DC::setViewportExt(int cx, int cy, ::SIZE* original /* = 0 */) {assertValidAsDC(); return toBoolean(::SetViewportExtEx(getHandle(), cx, cy, original));}

inline bool DC::setViewportExt(const ::SIZE& size, ::SIZE* original /* = 0 */) {return setViewportExt(size.cx, size.cy, original);}

inline bool DC::setViewportOrg(int x, int y, ::POINT* original /* = 0 */) {assertValidAsDC(); return toBoolean(::SetViewportOrgEx(getHandle(), x, y, original));}

inline bool DC::setViewportOrg(const ::POINT& p, ::POINT* original /* = 0 */) {return setViewportOrg(p.x, p.y, original);}

inline bool DC::setWindowExt(int cx, int cy, ::SIZE* original /* = 0 */) {assertValidAsDC(); return toBoolean(::SetWindowExtEx(getHandle(), cx, cy, original));}

inline bool DC::setWindowExt(const ::SIZE& size, ::SIZE* original /* = 0 */) {return setWindowExt(size.cx, size.cy, original);}

inline bool DC::setWindowOrg(int x, int y, ::POINT* original /* = 0 */) {assertValidAsDC(); return toBoolean(::SetWindowOrgEx(getHandle(), x, y, original));}

inline bool DC::setWindowOrg(const ::POINT& p, ::POINT* original /* = 0 */) {return setWindowOrg(p.x, p.y, original);}

inline int DC::startDoc(const ::DOCINFOW& docInfo) {assertValidAsDC(); return ::StartDocW(getHandle(), &docInfo);}

inline int DC::startPage() {assertValidAsDC(); return ::StartPage(getHandle());}

inline bool DC::stretchBlt(int x, int y, int width, int height, ::HDC srcDC, int xSrc, int ySrc, int srcWidth, int srcHeight, ::DWORD rop) {
	assertValidAsDC(); return toBoolean(::StretchBlt(getHandle(), x, y, width, height, srcDC, xSrc, ySrc, srcWidth, srcHeight, rop));}

inline SIZE DC::tabbedTextOut(int x, int y, const ::WCHAR* text, int length, int tabCount, int* tabStopPositions, int tabOrigin) {
	assertValidAsDC();
	const long res = ::TabbedTextOutW(getHandle(), x, y, text, length, tabCount, tabStopPositions, tabOrigin);
	const SIZE size = {LOWORD(res), HIWORD(res)};
	return size;
}

inline bool DC::textOut(int x, int y, const ::WCHAR* text, int length) {assertValidAsDC(); return toBoolean(::TextOutW(getHandle(), x, y, text, length));}

inline void DC::updateColors() {assertValidAsDC(); ::UpdateColors(getHandle());}

}}} // namespace manah.win32.gdi

#endif /* MANAH_DC_HPP */
