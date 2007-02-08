// dc.hpp
// (c) 2003-2007 exeal

#ifndef MANAH_DC_HPP
#define MANAH_DC_HPP
#include "windows.hpp"

namespace {
	struct WindowRelatedDCHolder {
		HWND window_;
		HDC dc_;
		ulong c_;
		WindowRelatedDCHolder(HWND window, HDC dc) : window_(window), dc_(dc), c_(1) {assert(dc != 0);}
		~WindowRelatedDCHolder() {assert(c_ == 0); ::ReleaseDC(window_, dc_);}
	};
}

namespace manah {
	namespace windows {

		namespace ui {class Window;}

		namespace gdi {

class DC : public HandleHolder<HDC> {
public:
	// コンストラクタ
	DC() : HandleHolder<HDC>(0) {}
	virtual ~DC() {}
	// 作成
	HBITMAP		getCurrentBitmap() const;
	HBRUSH		getCurrentBrush() const;
	HFONT		getCurrentFont() const;
	HPALETTE	getCurrentPalette() const;
	HPEN		getCurrentPen() const;
	HWND		getWindow() const;
	// デバイスコンテキスト
	int				getDeviceCaps(int index) const;
	HDC				getSafeHdc() const;
	virtual bool	restoreDC(int savedDC);
	virtual int		saveDC();
	// 描画ツール
	int		enumObjects(int objectType, GOBJENUMPROC proc, LPARAM data);
	POINT	getBrushOrg() const;
	POINT	setBrushOrg(int x, int y);
	POINT	setBrushOrg(const POINT& pt);
	// 選択
	HBITMAP			selectObject(HBITMAP bitmap);
	HBRUSH			selectObject(HBRUSH brush);
	virtual HFONT	selectObject(HFONT font);
	HPEN			selectObject(HPEN pen);
	HGDIOBJ			selectStockObject(int object);
	// 色とパレット
	COLORREF	getNearestColor(COLORREF color) const;
	UINT		realizePalette();
	HPALETTE	selectPalette(HPALETTE palette, bool forceBackground);
	void		updateColors();
	// 属性
	COLORREF	getBkColor() const;
	int			getBkMode() const;
	bool		getColorAdjustment(COLORADJUSTMENT& colorAdjust) const;
	int			getPolyFillMode() const;
	int			getROP2() const;
	int			getStretchBltMode() const;
	COLORREF	getTextColor() const;
	COLORREF	setBkColor(COLORREF color);
	int			setBkMode(int mode);
	bool		setColorAdjustment(const COLORADJUSTMENT& colorAdjust);
	int			setPolyFillMode(int mode);
	int			setROP2(int mode);
	int			setStretchBltMode(int mode);
	COLORREF	setTextColor(COLORREF color);
	// 領域
	bool	fillRgn(HRGN region, HBRUSH brush);
	bool	frameRgn(HRGN region, HBRUSH brush, int width, int height);
	bool	invertRgn(HRGN region);
	bool	paintRgn(HRGN region);
	// クリッピング
	int		excludeClipRect(int x1, int y1, int x2, int y2);
	int		excludeClipRect(const RECT& rect);
	int		excludeUpdateRgn(HWND window);
	UINT	getBoundsRect(RECT& rect, UINT flags);
	int		getClipBox(RECT& rect) const;
	int		intersectClipRect(int x1, int y1, int x2, int y2);
	int		intersectClipRect(const RECT& rect);
	int		offsetClipRgn(int x, int y);
	int		offsetClipRgn(const SIZE& size);
	bool	ptVisible(int x, int y) const;
	bool	ptVisible(const POINT& pt) const;
	bool	rectVisible(const RECT& rect) const;
	int		selectClipRgn(HRGN region);
	int		selectClipRgn(HRGN region, int mode);
	UINT	setBoundsRect(const RECT& rect, UINT flags);
	// 線出力
	bool	angleArc(int x, int y, int radius, float startAngle, float sweepAngle);
	bool	arc(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);
	bool	arc(const RECT& rect, const POINT& start, const POINT& end);
	bool	arcTo(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);
	bool	arcTo(const RECT& rect, const POINT& start, const POINT& end);
	int		getArcDirection() const;
	POINT	getCurrentPosition() const;
	bool	lineTo(int x, int y);
	bool	lineTo(const POINT& pt);
	POINT	moveTo(int x, int y);
	POINT	moveTo(const POINT& pt);
	bool	polyBezier(const POINT* points, int count);
	bool	polyBezierTo(const POINT* points, int count);
	bool	polyDraw(const POINT* points, const BYTE* types, int count);
	bool	polyline(LPPOINT points, int count);
	bool	polylineTo(const POINT* points, int count);
	bool	polyPolyline(const POINT* points, const DWORD* polyPoints, int count);
	int		setArcDirection(int direction);
	// 単純描画
//	void	draw3dRect(const RECT* rect, COLORREF topLeftColor, COLORREF rightBottomColor);
//	void	draw3dRect(int x, int y, int cx, int cy, COLORREF topLeftColor, COLORREF rightBottomColor);
//	void	drawDragIcon(const RECT* rect, SIZE size,
//				const RECT* lastRect, SIZE sizeLast, HBRUSH brush, HBRUSH lastBrush);
	bool	drawEdge(const RECT& rect, UINT edge, UINT flags);
	bool	drawFrameControl(const RECT& rect, UINT type, UINT state);
	bool	drawIcon(int x, int y, HICON icon);
	bool	drawIcon(const POINT& pt, HICON icon);
	bool	drawIconEx(int x, int y, HICON icon, int cx, int cy, UINT animationCursorStep, HBRUSH flicker, UINT flags);
	bool	drawIconEx(const POINT& pt, HICON icon, int cx, int cy, UINT animationCursorStep, HBRUSH flicker, UINT flags);
	bool	drawIconEx(int x, int y, HICON icon, const SIZE& size, UINT animationCursorStep, HBRUSH flicker, UINT flags);
	bool	drawIconEx(const POINT& pt, HICON icon, const SIZE& size, UINT animationCursorStep, HBRUSH flicker, UINT flags);
	void	fillRect(const RECT& rect, HBRUSH brush);
	void	fillSolidRect(const RECT& rect, COLORREF color);
	void	fillSolidRect(int x, int y, int cx, int cy, COLORREF color);
	void	frameRect(const RECT& rect, HBRUSH brush);
	void	invertRect(const RECT& rect);
	// 楕円と多角形
	bool	chord(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);
	bool	chord(const RECT& rect, const POINT& start, const POINT& end);
	void	drawFocusRect(const RECT& rect);
	bool	ellipse(int x1, int y1, int x2, int y2);
	bool	ellipse(const RECT& rect);
	bool	pie(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);
	bool	pie(const RECT& rect, const POINT& start, const POINT& end);
	bool	polygon(const POINT* points, int count);
	bool	polyPolygon(const POINT* points, const int* lpPolyCounts, int count);
	bool	rectangle(int x1, int y1, int x2, int y2);
	bool	rectangle(const RECT& rect);
	bool	roundRect(int x1, int y1, int x2, int y2, int x3, int y3);
	bool	roundRect(const RECT& rect, const POINT& pt);
	// ビットマップ
	bool		bitBlt(int x, int y, int width, int height, HDC srcDC, int xSrc, int ySrc, DWORD rop);
	bool		extFloodFill(int x, int y, COLORREF color, UINT fillType);
	bool		floodFill(int x, int y, COLORREF color);
	COLORREF	getPixel(int x, int y) const;
	COLORREF	getPixel(const POINT& pt) const;
	bool		maskBlt(int x, int y, int width, int height,
					HDC dc, int xSrc, int ySrc, HBITMAP bitmap, int xMask, int yMask, DWORD rop);
	bool		patBlt(int x, int y, int width, int height, DWORD rop);
	bool		patBlt(const RECT& rect, DWORD rop);
	bool		plgBlt(const POINT* point, HDC dc,
					int xSrc, int ySrc, int width, int height, HBITMAP bitmap, int xMask, int yMask);
	COLORREF	setPixel(int x, int y, COLORREF color);
	COLORREF	setPixel(const POINT& pt, COLORREF color);
	bool		setPixelV(int x, int y, COLORREF color);
	bool		setPixelV(const POINT& pt, COLORREF color);
	bool		stretchBlt(int x, int y, int width, int height,
					HDC srcDC, int xSrc, int ySrc, int srcWidth, int srcHeight, DWORD rop);
	// テキスト
	virtual int		drawText(const TCHAR* text, int length, const RECT& rect, UINT format);
	virtual bool	extTextOut(int x, int y, UINT options, const RECT* rect, const TCHAR* text, UINT length, const int* dxWidths);
	SIZE			getCharacterPlacement(const TCHAR* text, int length, int maxExtent, GCP_RESULTS& results, DWORD flags) const;
	SIZE			getTabbedTextExtent(const TCHAR* text, int length, int tabCount, int* tabStopPositions) const;
	UINT			getTextAlign() const;
	int				getTextCharacterExtra() const;
	SIZE			getTextExtent(const TCHAR* text, int length) const;
	bool			getTextExtentExPoint(const TCHAR* text, int length, int maxExtent, LPINT fit, LPINT dx, LPSIZE size) const;
	bool			getTextExtentExPointI(LPWORD glyphs, int count, int maxExtent, LPINT fit, LPINT dx, LPSIZE size) const;
	bool			getTextExtentPointI(LPWORD glyphs, int count, LPSIZE size) const;
	int				getTextFace(int faceNameCount, TCHAR* faceName) const;
	bool			getTextMetrics(TEXTMETRIC& metrics) const;
	virtual bool	grayString(HBRUSH brush, GRAYSTRINGPROC outputProc, LPARAM data, int length, int x, int y, int width, int height);
	bool			polyTextOut(const POLYTEXT* texts, int count);
	UINT			setTextAlign(UINT flags);
	int				setTextCharacterExtra(int charExtra);
	int				setTextJustification(int breakExtra, int breakCount);
	virtual SIZE	tabbedTextOut(int x, int y, const TCHAR* text, int length, int tabCount, int* tabStopPositions, int tabOrigin);
	virtual bool	textOut(int x, int y, const TCHAR* text, int length);
	// フォント
	int		enumFontFamilies(const TCHAR* name, FONTENUMPROC proc, LPARAM param = 0UL) const;
	int		enumFontFamilies(const LOGFONT& condition, FONTENUMPROC proc, LPARAM param = 0UL) const;
	bool	getAspectRatioFilterEx(SIZE& size) const;
	bool	getCharABCWidths(UINT firstChar, UINT lastChar, ABC buffer[]) const;
	bool	getCharABCWidths(UINT firstChar, UINT lastChar, ABCFLOAT buffer[]) const;
	bool	getCharWidth(UINT firstChar, UINT lastChar, int* buffer) const;
	bool	getCharWidth(UINT firstChar, UINT lastChar, float* buffer) const;
	DWORD	getFontData(DWORD table, DWORD offset, LPVOID data, DWORD bytes) const;
	DWORD	getFontLanguageInfo() const;
#if(_WIN32_WINNT >= 0x0500)
	DWORD	getFontUnicodeRanges(GLYPHSET& glyphSet) const;
	DWORD	getGlyphIndices(const TCHAR* text, int length, WORD* indices, DWORD flags) const;
#endif /* _WIN32_WINNT >= 0x0500 */
	DWORD	getGlyphOutline(UINT ch, UINT format, LPGLYPHMETRICS gm, DWORD bufferSize, LPVOID data, const MAT2* mat2) const;
	DWORD	getKerningPairs(DWORD pairs, LPKERNINGPAIR kerningPair) const;
	UINT	getOutlineTextMetrics(UINT dataSize, LPOUTLINETEXTMETRIC otm) const;
	bool	getRasterizerCaps(RASTERIZER_STATUS& status, UINT cb) const;
	DWORD	setMapperFlags(DWORD flag);
	// スクロール
	bool	scrollDC(int dx, int dy, const RECT& scrollRect, const RECT& clipRect, HRGN updateRegion, RECT* updateRect);
protected:
	DC(const DC& rhs) : HandleHolder<HDC>(rhs) {}
	void	assertValidAsDC() const;
};


class AutoDC : public DC {
public:
	AutoDC(HDC handle = 0) {if(handle != 0) attach(handle);}
	AutoDC(const AutoDC& rhs) {if(rhs.get() != 0) attach(rhs.get());}
	virtual ~AutoDC() {if(get() != 0 && !isAttached()) ::DeleteDC(detach());}
	bool createCompatibleDC(HDC dc) {
		assert(get() == 0);
		setHandle(::CreateCompatibleDC(dc));
		return get() != 0;
	}
	bool deleteDC() {
		if(get() == 0)
			return false;
		return toBoolean(::DeleteDC(detach()));
	}
};


class PaintDC : public DC {
public:
	PaintDC(HWND window) : window_(window), createdByWindow_(false) {
		assert(toBoolean(::IsWindow(window)));
		setHandle(::BeginPaint(window, &paint_));
		assert(get() != 0);
	}
	PaintDC(const PaintDC& rhs) :
		DC(rhs), window_(rhs.window_), createdByWindow_(rhs.createdByWindow_) {const_cast<PaintDC&>(rhs).window_ = 0;}
private:
	PaintDC(HWND window, PAINTSTRUCT& paint) : window_(window), paint_(paint), createdByWindow_(true) {
		setHandle(paint_.hdc);
		assert(::IsWindow(window_));
		assert(get() != 0);
	}
public:
	~PaintDC() {if(!createdByWindow_) ::EndPaint(window_, &paint_);}
private:
	operator HDC() const;
public:
	const PAINTSTRUCT& getPaintStruct() const {return paint_;}
private:
	HWND		window_;
	PAINTSTRUCT	paint_;
	bool		createdByWindow_;
	friend class ui::Window;	// for Window::beginPaint
};


class ClientDC : public DC {
protected:
	ClientDC(HWND window) : holder_(new WindowRelatedDCHolder(window, ::GetDC(window))) {setHandle(holder_->dc_);}
	ClientDC(HWND window, HRGN clipRegion, DWORD flags) :
		holder_(new WindowRelatedDCHolder(window, ::GetDCEx(window, clipRegion, flags))) {setHandle(holder_->dc_);}
public:
	ClientDC(const ClientDC& rhs) : DC(rhs), holder_(rhs.holder_) {++holder_->c_; setHandle(rhs.get());}
	virtual ~ClientDC() {if(--holder_->c_ == 0) delete holder_;}
private:
	operator HDC() const;
private:
	WindowRelatedDCHolder* holder_;
	friend class ui::Window;	// for Window::getDC
};


class WindowDC : public DC {
protected:
	WindowDC(HWND window) : holder_(new WindowRelatedDCHolder(window, ::GetWindowDC(window))) {setHandle(holder_->dc_);}
public:
	WindowDC(const WindowDC& rhs) : DC(rhs), holder_(rhs.holder_) {++holder_->c_; setHandle(rhs.get());}
	virtual ~WindowDC() {if(--holder_->c_ == 0) delete holder_;}
private:
	operator HDC() const;
private:
	WindowRelatedDCHolder* holder_;
	friend class ui::Window;	// for Window::getWindowDC
};


class ScreenDC : public ClientDC {
public:
	ScreenDC() : ClientDC(0) {}
private:
	operator HDC() const;
};


// DC ///////////////////////////////////////////////////////////////////////

inline bool DC::angleArc(int x, int y, int radius, float startAngle, float sweepAngle) {
	assertValidAsDC(); return toBoolean(::AngleArc(get(), x, y, radius, startAngle, sweepAngle));}

inline bool DC::arc(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4) {
	assertValidAsDC(); return toBoolean(::Arc(get(), x1, y1, x2, y2, x3, y3, x4, y4));}

inline bool DC::arc(const RECT& rect, const POINT& start, const POINT& end) {
	return arc(rect.left, rect.top, rect.right, rect.bottom, start.x, start.y, end.x, end.y);}

inline bool DC::arcTo(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4) {
	assertValidAsDC(); return toBoolean(::ArcTo(get(), x1, y1, x2, y2, x3, y3, x4, y4));}

inline bool DC::arcTo(const RECT& rect, const POINT& start, const POINT& end) {
	return arcTo(rect.left, rect.top, rect.right, rect.bottom, start.x, start.y, end.x, end.y);}

inline void DC::assertValidAsDC() const {assert(get() != 0);}

inline bool DC::bitBlt(int x, int y, int width, int height, HDC srcDC, int xSrc, int ySrc, DWORD rop) {
	assertValidAsDC(); return toBoolean(::BitBlt(get(), x, y, width, height, srcDC, xSrc, ySrc, rop));}

inline bool DC::chord(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4) {
	assertValidAsDC(); return toBoolean(::Chord(get(), x1, y1, x2, y2, x3, y3, x4, y4));}

inline bool DC::chord(const RECT& rect, const POINT& start, const POINT& end) {
	return chord(rect.left, rect.top, rect.right, rect.bottom, start.x, start.y, end.x, end.y);}

inline bool DC::drawEdge(const RECT& rect, UINT edge, UINT flags) {
	assertValidAsDC(); return toBoolean(::DrawEdge(get(), const_cast<RECT*>(&rect), edge, flags));}

inline void DC::drawFocusRect(const RECT& rect) {assertValidAsDC(); ::DrawFocusRect(get(), &rect);}

inline bool DC::drawFrameControl(const RECT& rect, UINT type, UINT state) {
	assertValidAsDC(); return toBoolean(::DrawFrameControl(get(), const_cast<RECT*>(&rect), type, state));}

inline bool DC::drawIcon(int x, int y, HICON icon) {assertValidAsDC(); return toBoolean(::DrawIcon(get(), x, y, icon));}

inline bool DC::drawIcon(const POINT& pt, HICON icon) {return drawIcon(pt.x, pt.y, icon);}

inline bool DC::drawIconEx(int x, int y, HICON icon, int cx, int cy, UINT animationCursorStep, HBRUSH flicker, UINT flags) {
	assertValidAsDC(); return toBoolean(::DrawIconEx(get(), x, y, icon, cx, cy, animationCursorStep, flicker, flags));}

inline bool DC::drawIconEx(const POINT& pt, HICON icon, int cx, int cy, UINT animationCursorStep, HBRUSH flicker, UINT flags) {
	return drawIconEx(pt.x, pt.y, icon, cx, cy, animationCursorStep, flicker, flags);}

inline bool DC::drawIconEx(int x, int y, HICON icon, const SIZE& size, UINT animationCursorStep, HBRUSH flicker, UINT flags) {
	return drawIconEx(x, y, icon, size.cx, size.cy, animationCursorStep, flicker, flags);}

inline bool DC::drawIconEx(const POINT& pt, HICON icon, const SIZE& size, UINT animationCursorStep, HBRUSH flicker, UINT flags) {
	return drawIconEx(pt.x, pt.y, icon, size.cx, size.cy, animationCursorStep, flicker, flags);}

inline int DC::drawText(const TCHAR* text, int length, const RECT& rect, UINT format) {
	assertValidAsDC(); return ::DrawText(get(), text, length, const_cast<RECT*>(&rect), format);}

inline bool DC::ellipse(int x1, int y1, int x2, int y2) {assertValidAsDC(); return toBoolean(::Ellipse(get(), x1, y1, x2, y2));}

inline bool DC::ellipse(const RECT& rect) {return ellipse(rect.left, rect.top, rect.right, rect.bottom);}

inline int DC::enumFontFamilies(const TCHAR* name, FONTENUMPROC proc, LPARAM param /* = 0UL */) const {
	assertValidAsDC(); return ::EnumFontFamilies(get(), name, proc, param);}

inline int DC::enumFontFamilies(const LOGFONT& condition, FONTENUMPROC proc, LPARAM param /* = 0UL */) const {
	assertValidAsDC(); return ::EnumFontFamiliesEx(get(), const_cast<LOGFONT*>(&condition), proc, param, 0);}

inline int DC::enumObjects(int objectType, GOBJENUMPROC proc, LPARAM data) {
	assertValidAsDC(); return ::EnumObjects(get(), objectType, proc, data);}

inline int DC::excludeClipRect(int x1, int y1, int x2, int y2) {assertValidAsDC(); return ::ExcludeClipRect(get(), x1, y1, x2, y2);}

inline int DC::excludeClipRect(const RECT& rect) {return excludeClipRect(rect.left, rect.top, rect.right, rect.bottom);}

inline int DC::excludeUpdateRgn(HWND window) {assertValidAsDC(); return ::ExcludeUpdateRgn(get(), window);}

inline bool DC::extFloodFill(int x, int y, COLORREF color, UINT fillType) {
	assertValidAsDC(); return toBoolean(::ExtFloodFill(get(), x, y, color, fillType));}

inline bool DC::extTextOut(int x, int y, UINT options,
		const RECT* rect, const TCHAR* text, UINT length, const int* dxWidths) {
	assertValidAsDC(); return toBoolean(::ExtTextOut(get(), x, y, options, rect, text, length, dxWidths));}

inline void DC::fillRect(const RECT& rect, HBRUSH brush) {assertValidAsDC(); ::FillRect(get(), &rect, brush);}

inline bool DC::fillRgn(HRGN region, HBRUSH brush) {assertValidAsDC(); return toBoolean(::FillRgn(get(), region, brush));}

inline void DC::fillSolidRect(const RECT& rect, COLORREF color) {
	fillSolidRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, color);}

inline void DC::fillSolidRect(int x, int y, int cx, int cy, COLORREF color) {
	assertValidAsDC();
	COLORREF org = getBkColor();
	RECT rect;
	::SetRect(&rect, x, y, x + cx, y + cy);
	setBkColor(color);
	extTextOut(0, 0, ETO_IGNORELANGUAGE | ETO_OPAQUE, &rect, _T(""), 0, 0);
	setBkColor(org);
}

inline bool DC::floodFill(int x, int y, COLORREF color) {
	assertValidAsDC(); return toBoolean(::FloodFill(get(), x, y, color));}

inline bool DC::frameRgn(HRGN region, HBRUSH brush, int width, int height) {
	assertValidAsDC(); return toBoolean(::FrameRgn(get(), region, brush, width, height));}

inline void DC::frameRect(const RECT& rect, HBRUSH brush) {assertValidAsDC(); ::FrameRect(get(), &rect, brush);}

inline int DC::getArcDirection() const {assertValidAsDC(); return ::GetArcDirection(get());}

inline bool DC::getAspectRatioFilterEx(SIZE& size) const {
	assertValidAsDC(); return toBoolean(::GetAspectRatioFilterEx(get(), &size));}

inline COLORREF DC::getBkColor() const {assertValidAsDC(); return ::GetBkColor(get());}

inline int DC::getBkMode() const {assertValidAsDC(); return ::GetBkMode(get());}

inline UINT DC::getBoundsRect(RECT& rect, UINT flags) {assertValidAsDC(); return ::GetBoundsRect(get(), &rect, flags);}

inline POINT DC::getBrushOrg() const {assertValidAsDC(); POINT pt; ::GetBrushOrgEx(get(), &pt); return pt;}

inline SIZE DC::getCharacterPlacement(const TCHAR* text, int length, int maxExtent, GCP_RESULTS& results, DWORD flags) const {
	assertValidAsDC();
	const DWORD res = ::GetCharacterPlacement(get(), text, length, maxExtent, &results, flags);
	const SIZE size = {LOWORD(res), HIWORD(res)};
	return size;
}

inline bool DC::getCharABCWidths(UINT firstChar, UINT lastChar, ABC buffer[]) const {
	assertValidAsDC(); return toBoolean(::GetCharABCWidths(get(), firstChar, lastChar, buffer));}

inline bool DC::getCharABCWidths(UINT firstChar, UINT lastChar, ABCFLOAT buffer[]) const {
	assertValidAsDC(); return toBoolean(::GetCharABCWidthsFloat(get(), firstChar, lastChar, buffer));}

inline bool DC::getCharWidth(UINT firstChar, UINT lastChar, int* buffer) const {
	assertValidAsDC(); return toBoolean(::GetCharWidth32(get(), firstChar, lastChar, buffer));}

inline bool DC::getCharWidth(UINT firstChar, UINT lastChar, float* buffer) const {
	assertValidAsDC(); return toBoolean(::GetCharWidthFloat(get(), firstChar, lastChar, buffer));}

inline int DC::getClipBox(RECT& rect) const {assertValidAsDC(); return ::GetClipBox(get(), &rect);}

inline bool DC::getColorAdjustment(COLORADJUSTMENT& colorAdjust) const {
	assertValidAsDC(); return toBoolean(::GetColorAdjustment(get(), &colorAdjust));}

inline HBITMAP DC::getCurrentBitmap() const {
	assertValidAsDC(); return reinterpret_cast<HBITMAP>(::GetCurrentObject(get(), OBJ_BITMAP));}

inline HBRUSH DC::getCurrentBrush() const {
	assertValidAsDC(); return reinterpret_cast<HBRUSH>(::GetCurrentObject(get(), OBJ_BRUSH));}

inline HFONT DC::getCurrentFont() const {assertValidAsDC(); return reinterpret_cast<HFONT>(::GetCurrentObject(get(), OBJ_FONT));}

inline HPALETTE DC::getCurrentPalette() const {assertValidAsDC(); return reinterpret_cast<HPALETTE>(::GetCurrentObject(get(), OBJ_PAL));}

inline HPEN DC::getCurrentPen() const {assertValidAsDC(); return reinterpret_cast<HPEN>(::GetCurrentObject(get(), OBJ_PEN));}

inline POINT DC::getCurrentPosition() const {
	assertValidAsDC();
	POINT pt;
	::GetCurrentPositionEx(get(), &pt);
	return pt;
}

inline int DC::getDeviceCaps(int index) const {assertValidAsDC(); return ::GetDeviceCaps(get(), index);}

inline DWORD DC::getFontData(DWORD table, DWORD offset, LPVOID data, DWORD bytes) const {
	assertValidAsDC(); return ::GetFontData(get(), table, offset, data, bytes);}

inline DWORD DC::getFontLanguageInfo() const {assertValidAsDC(); return ::GetFontLanguageInfo(get());}

#if(_WIN32_WINNT >= 0x0500)
inline DWORD DC::getFontUnicodeRanges(GLYPHSET& glyphSet) const {assertValidAsDC(); return ::GetFontUnicodeRanges(get(), &glyphSet);}

inline DWORD DC::getGlyphIndices(const TCHAR* text, int length, WORD* indices, DWORD flags) const {
	assertValidAsDC(); return ::GetGlyphIndices(get(), text, length, indices, flags);}
#endif

inline DWORD DC::getGlyphOutline(UINT ch, UINT format, LPGLYPHMETRICS gm, DWORD bufferSize, LPVOID data, const MAT2* mat2) const {
	assertValidAsDC(); return ::GetGlyphOutline(get(), ch, format, gm, bufferSize, data, mat2);}

inline DWORD DC::getKerningPairs(DWORD count, LPKERNINGPAIR kerningPairs) const {
	assertValidAsDC(); return ::GetKerningPairs(get(), count, kerningPairs);}

inline COLORREF DC::getNearestColor(COLORREF color) const {assertValidAsDC(); return ::GetNearestColor(get(), color);}

inline UINT DC::getOutlineTextMetrics(UINT bytes, LPOUTLINETEXTMETRIC otm) const {
	assertValidAsDC(); return ::GetOutlineTextMetrics(get(), bytes, otm);}

inline COLORREF DC::getPixel(int x, int y) const {assertValidAsDC(); return ::GetPixel(get(), x, y);}

inline COLORREF DC::getPixel(const POINT& pt) const {return getPixel(pt.x, pt.y);}

inline int DC::getPolyFillMode() const {assertValidAsDC(); return ::GetPolyFillMode(get());}

inline bool DC::getRasterizerCaps(RASTERIZER_STATUS& status, UINT cb) const {assertValidAsDC(); return toBoolean(::GetRasterizerCaps(&status, cb));}

inline int DC::getROP2() const {assertValidAsDC(); return ::GetROP2(get());}

inline HDC DC::getSafeHdc() const {return (this == 0) ? 0 : get();}

inline int DC::getStretchBltMode() const {assertValidAsDC(); return ::GetStretchBltMode(get());}

inline SIZE DC::getTabbedTextExtent(const TCHAR* text, int length, int tabCount, int* tabStopPositions) const {
	assertValidAsDC();
	const DWORD	res = ::GetTabbedTextExtent(get(), text, length, tabCount, tabStopPositions);
	const SIZE size = {LOWORD(res), HIWORD(res)};
	return size;
}

inline UINT DC::getTextAlign() const {assertValidAsDC(); return ::GetTextAlign(get());}

inline int DC::getTextCharacterExtra() const {assertValidAsDC(); return ::GetTextCharacterExtra(get());}

inline COLORREF DC::getTextColor() const {assertValidAsDC(); return ::GetTextColor(get());}

inline SIZE DC::getTextExtent(const TCHAR* text, int length) const {
	assertValidAsDC();
	SIZE size;
	::GetTextExtentPoint32(get(), text, length, &size);
	return size;
}

inline bool DC::getTextExtentExPoint(const TCHAR* text, int length, int maxExtent, LPINT fit, LPINT dx, LPSIZE size) const {
	assertValidAsDC(); return toBoolean(::GetTextExtentExPoint(get(), text, length, maxExtent, fit, dx, size));}

#if(_WIN32_WINNT >= 0x0500)
inline bool DC::getTextExtentExPointI(LPWORD glyphs, int count, int maxExtent, LPINT fit, LPINT dx, LPSIZE size) const {
	assertValidAsDC(); return toBoolean(::GetTextExtentExPointI(get(), glyphs, count, maxExtent, fit, dx, size));}
#endif /* _WIN32_WINNT >= 0x0500 */

#if(_WIN32_WINNT >= 0x0500)
inline bool DC::getTextExtentPointI(LPWORD glyphs, int count, LPSIZE size) const {
	assertValidAsDC(); return toBoolean(::GetTextExtentPointI(get(), glyphs, count, size));}
#endif /* _WIN32_WINNT >= 0x0500 */

inline int DC::getTextFace(int maxLength, TCHAR* faceName) const {assertValidAsDC(); return ::GetTextFace(get(), maxLength, faceName);}

inline bool DC::getTextMetrics(TEXTMETRIC& metrics) const {assertValidAsDC(); return toBoolean(::GetTextMetrics(get(), &metrics));}

inline HWND DC::getWindow() const {assertValidAsDC(); return ::WindowFromDC(get());}

inline bool DC::grayString(HBRUSH brush, GRAYSTRINGPROC outputProc, LPARAM data, int length, int x, int y, int width, int height) {
	assertValidAsDC(); return toBoolean(::GrayString(get(), brush, outputProc, data, length, x, y, width, height));}

inline int DC::intersectClipRect(int x1, int y1, int x2, int y2) {assertValidAsDC(); return ::IntersectClipRect(get(), x1, y1, x2, y2);}

inline int DC::intersectClipRect(const RECT& rect) {return intersectClipRect(rect.left, rect.top, rect.right, rect.bottom);}

inline void DC::invertRect(const RECT& rect) {assertValidAsDC(); ::InvertRect(get(), &rect);}

inline bool DC::invertRgn(HRGN region) {assertValidAsDC(); return toBoolean(::InvertRgn(get(), region));}

inline bool DC::lineTo(int x, int y) {assertValidAsDC(); return toBoolean(::LineTo(get(), x, y));}

inline bool DC::lineTo(const POINT& pt) {return lineTo(pt.x, pt.y);}

inline bool DC::maskBlt(int x, int y, int width, int height, HDC dc, int xSrc, int ySrc, HBITMAP bitmap, int xMask, int yMask, DWORD rop) {
	assertValidAsDC(); return toBoolean(::MaskBlt(get(), x, y, width, height, dc, xSrc, ySrc, bitmap, xMask, yMask, rop));}

inline POINT DC::moveTo(int x, int y) {
	assertValidAsDC();
	POINT pt;
	::MoveToEx(get(), x, y, &pt);
	return pt;
}

inline POINT DC::moveTo(const POINT& pt) {return moveTo(pt.x, pt.y);}

inline int DC::offsetClipRgn(int x, int y) {assertValidAsDC(); return ::OffsetClipRgn(get(), x, y);}

inline int DC::offsetClipRgn(const SIZE& size) {return offsetClipRgn(size.cx, size.cy);}

inline bool DC::paintRgn(HRGN region) {assertValidAsDC(); return toBoolean(::PaintRgn(get(), region));}

inline bool DC::patBlt(int x, int y, int width, int height, DWORD rop) {
	assertValidAsDC(); return toBoolean(::PatBlt(get(), x, y, width, height, rop));}

inline bool DC::patBlt(const RECT& rect, DWORD rop) {return patBlt(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, rop);}

inline bool DC::pie(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4) {
	assertValidAsDC(); return toBoolean(::Pie(get(), x1, y1, x2, y2, x3, y3, x4, y4));}

inline bool DC::pie(const RECT& rect, const POINT& start, const POINT& end) {
	return toBoolean(pie(rect.left, rect.top, rect.right, rect.bottom, start.x, start.y, end.x, end.y));}

inline bool DC::plgBlt(const POINT* point, HDC dc, int xSrc, int ySrc, int width, int height, HBITMAP bitmap, int xMask, int yMask) {
	assertValidAsDC(); return toBoolean(::PlgBlt(get(), point, dc, xSrc, ySrc, width, height, bitmap, xMask, yMask));}

inline bool DC::polyBezier(const POINT* points, int count) {assertValidAsDC(); return toBoolean(::PolyBezier(get(), points, count));}

inline bool DC::polyBezierTo(const POINT* points, int count) {assertValidAsDC(); return toBoolean(::PolyBezierTo(get(), points, count));}

inline bool DC::polyDraw(const POINT* points, const BYTE* types, int count) {
	assertValidAsDC(); return toBoolean(::PolyDraw(get(), points, types, count));}

inline bool DC::polygon(const POINT* points, int count) {assertValidAsDC(); return toBoolean(::Polygon(get(), points, count));}

inline bool DC::polyline(LPPOINT points, int count) {assertValidAsDC(); return toBoolean(::Polyline(get(), points, count));}

inline bool DC::polylineTo(const POINT* points, int count) {assertValidAsDC(); return toBoolean(::PolylineTo(get(), points, count));}

inline bool DC::polyPolygon(const POINT* points, const int* polyCounts, int count) {
	assertValidAsDC(); return toBoolean(::PolyPolygon(get(), points, polyCounts, count));}

inline bool DC::polyPolyline(const POINT* points, const DWORD *polyPoints, int count) {
	assertValidAsDC(); return toBoolean(::PolyPolyline(get(), points, polyPoints, count));}

inline bool DC::polyTextOut(const POLYTEXT* texts, int count) {assertValidAsDC(); return toBoolean(::PolyTextOut(get(), texts, count));}

inline bool DC::ptVisible(int x, int y) const {assertValidAsDC(); return toBoolean(::PtVisible(get(), x, y));}

inline bool DC::ptVisible(const POINT& pt) const {assertValidAsDC(); return toBoolean(::PtVisible(get(), pt.x, pt.y));}

inline UINT DC::realizePalette() {assertValidAsDC(); return ::RealizePalette(get());}

inline bool DC::rectangle(int x1, int y1, int x2, int y2) {assertValidAsDC(); return toBoolean(::Rectangle(get(), x1, y1, x2, y2));}

inline bool DC::rectangle(const RECT& rect) {return rectangle(rect.left, rect.top, rect.right, rect.bottom);}

inline bool DC::rectVisible(const RECT& rect) const {assertValidAsDC(); return toBoolean(::RectVisible(get(), &rect));}

inline bool DC::restoreDC(int savedDC) {assertValidAsDC(); return toBoolean(::RestoreDC(get(), savedDC));}

inline bool DC::roundRect(int x1, int y1, int x2, int y2, int x3, int y3) {
	assertValidAsDC(); return toBoolean(::RoundRect(get(), x1, y1, x2, y2, x3, y3));}

inline bool DC::roundRect(const RECT& rect, const POINT& pt) {return roundRect(rect.left, rect.top, rect.right, rect.bottom, pt.x, pt.y);}

inline int DC::saveDC() {assertValidAsDC(); return ::SaveDC(get());}

inline bool DC::scrollDC(int dx, int dy, const RECT& scrollRect, const RECT& clipRect, HRGN updateRegion, RECT* updateRect) {
	assertValidAsDC(); return toBoolean(::ScrollDC(get(), dx, dy, &scrollRect, &clipRect, updateRegion, updateRect));}

inline int DC::selectClipRgn(HRGN region) {assertValidAsDC(); return ::SelectClipRgn(get(), region);}

inline int DC::selectClipRgn(HRGN region, int mode) {assertValidAsDC(); return ::ExtSelectClipRgn(get(), region, mode);}

inline HBITMAP DC::selectObject(HBITMAP bitmap) {assertValidAsDC(); return reinterpret_cast<HBITMAP>(::SelectObject(get(), bitmap));}

inline HBRUSH DC::selectObject(HBRUSH brush) {assertValidAsDC(); return reinterpret_cast<HBRUSH>(::SelectObject(get(), brush));}

inline HFONT DC::selectObject(HFONT font) {assertValidAsDC(); return reinterpret_cast<HFONT>(::SelectObject(get(), font));}

inline HPEN DC::selectObject(HPEN pen) {assertValidAsDC(); return reinterpret_cast<HPEN>(::SelectObject(get(), pen));}

inline HPALETTE DC::selectPalette(HPALETTE palette, bool forceBackground) {
	assertValidAsDC(); return ::SelectPalette(get(), palette, forceBackground);}

inline HGDIOBJ DC::selectStockObject(int object) {assertValidAsDC(); return ::SelectObject(get(), ::GetStockObject(object));}

inline int DC::setArcDirection(int direction) {assertValidAsDC(); return ::SetArcDirection(get(), direction);}

inline COLORREF DC::setBkColor(COLORREF color) {assertValidAsDC(); return ::SetBkColor(get(), color);}

inline int DC::setBkMode(int mode) {assertValidAsDC(); return ::SetBkMode(get(), mode);}

inline UINT DC::setBoundsRect(const RECT& rect, UINT flags) {assertValidAsDC(); return ::SetBoundsRect(get(), &rect, flags);}

inline POINT DC::setBrushOrg(int x, int y) {
	assertValidAsDC();
	POINT pt;
	::SetBrushOrgEx(get(), x, y, &pt);
	return pt;
}

inline POINT DC::setBrushOrg(const POINT& pt) {return setBrushOrg(pt.x, pt.y);}

inline bool DC::setColorAdjustment(const COLORADJUSTMENT& colorAdjust) {
	assertValidAsDC(); return toBoolean(::SetColorAdjustment(get(), &colorAdjust));}

inline DWORD DC::setMapperFlags(DWORD flag) {assertValidAsDC(); return ::SetMapperFlags(get(), flag);}

inline COLORREF DC::setPixel(int x, int y, COLORREF color) {assertValidAsDC(); return ::SetPixel(get(), x, y, color);}

inline COLORREF DC::setPixel(const POINT& pt, COLORREF color) {return setPixel(pt.x, pt.y, color);}

inline bool DC::setPixelV(int x, int y, COLORREF color) {assertValidAsDC(); return toBoolean(::SetPixelV(get(), x, y, color));}

inline bool DC::setPixelV(const POINT& pt, COLORREF color) {return setPixelV(pt.x, pt.y, color);}

inline int DC::setPolyFillMode(int mode) {assertValidAsDC(); return ::SetPolyFillMode(get(), mode);}

inline int DC::setROP2(int mode) {assertValidAsDC(); return ::SetROP2(get(), mode);}

inline int DC::setStretchBltMode(int mode) {assertValidAsDC(); return ::SetStretchBltMode(get(), mode);}

inline UINT DC::setTextAlign(UINT flags) {assertValidAsDC(); return ::SetTextAlign(get(), flags);}

inline int DC::setTextCharacterExtra(int charExtra) {assertValidAsDC(); return ::SetTextCharacterExtra(get(), charExtra);}

inline COLORREF DC::setTextColor(COLORREF color) {assertValidAsDC(); return ::SetTextColor(get(), color);}

inline int DC::setTextJustification(int breakExtra, int breakCount) {
	assertValidAsDC(); return ::SetTextJustification(get(), breakExtra, breakExtra);}

inline bool DC::stretchBlt(int x, int y, int width, int height, HDC srcDC, int xSrc, int ySrc, int srcWidth, int srcHeight, DWORD rop) {
	assertValidAsDC(); return toBoolean(::StretchBlt(get(), x, y, width, height, srcDC, xSrc, ySrc, srcWidth, srcHeight, rop));}

inline SIZE DC::tabbedTextOut(int x, int y, const TCHAR* text, int length, int tabCount, int* tabStopPositions, int tabOrigin) {
	assertValidAsDC();
	const long res = ::TabbedTextOut(get(), x, y, text, length, tabCount, tabStopPositions, tabOrigin);
	const SIZE size = {LOWORD(res), HIWORD(res)};
	return size;
}

inline bool DC::textOut(int x, int y, const TCHAR* text, int length) {
	assertValidAsDC(); return toBoolean(::TextOut(get(), x, y, text, length));}

inline void DC::updateColors() {assertValidAsDC(); ::UpdateColors(get());}

}}} // namespace manah::windows::gdi

#endif /* MANAH_DC_HPP */
