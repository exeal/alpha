// theme.hpp
// (c) 2006-2007 exeal

#ifndef MANAH_THEME_HPP
#define MANAH_THEME_HPP
#include <uxtheme.h>

namespace manah {
namespace win32 {
namespace ui {

	class Theme : public Handle<HTHEME, ::CloseThemeData> {
	public:
		// constructors
		explicit Theme(HTHEME handle = 0) throw();
		// constructions
		HRESULT	close();
		bool	open(HWND window, const WCHAR* classList);
		// attributes
		HRESULT	getBackgroundContentRect(HDC dc, int partID, int stateID, const ::RECT& bounding, ::RECT& contentRect) const;
		HRESULT	getBackgroundExtent(HDC dc, int partID, int stateID, const ::RECT& contentRect, ::RECT& extentRect) const;
		HRESULT	getBackgroundRegion(HDC dc, int partID, int stateID, const ::RECT& rect, HRGN& region) const;
		HRESULT	getBool(int partID, int stateID, int propertyID, bool& value) const;
		HRESULT	getColor(int partID, int stateID, int propertyID, COLORREF& color) const;
		HRESULT	getEnumValue(int partID, int stateID, int propertyID, int& value) const;
		HRESULT	getInt(int partID, int stateID, int propertyID, int& value) const;
		HRESULT	getMetric(HDC dc, int partID, int stateID, int propertyID, int& value) const;
		HRESULT	getPartSize(HDC dc, int partID, int stateID, ::RECT* rect, ::THEMESIZE type, ::SIZE& size) const;
		HRESULT	getPosition(int partID, int stateID, int propertyID, ::POINT& pt) const;
		HRESULT	getString(int partID, int stateID, int propertyID, WCHAR* buffer, int maxBufferChars) const;
		HRESULT	getTextExtent(HDC dc, int partID, int stateID,
			const WCHAR* text, int charCount, DWORD flags, const ::RECT* bounding, ::RECT& extentRect) const;
		HRESULT	getTextMetrics(HDC dc, int partID, int stateID, ::TEXTMETRICW& textMetric) const;
		bool	isBackgroundPartiallyTransparent(int partID, int stateID) const;
		bool	isPartDefined(int partID, int stateID) const;
		// operations
		HRESULT	drawBackground(HDC dc, int partID, int stateID, const ::RECT& rect, const ::RECT* clipRect = 0);
		HRESULT	drawBackground(HDC dc, int partID, int stateID, const ::RECT& rect, const ::DTBGOPTS* options = 0);
		HRESULT	drawEdge(HDC dc, int partID, int stateID, const ::RECT& destRect, UINT edge, UINT flags, ::RECT* contentRect = 0);
		HRESULT	drawIcon(HDC dc, int partID, int stateID, const ::RECT& rect, HIMAGELIST imageList, int index);
		HRESULT	drawText(HDC dc, int partID, int stateID, const WCHAR* text, int charCount, DWORD flags, DWORD flags2, const ::RECT& rect);
		HRESULT	hitTestBackground(HDC dc, int partID, int stateID,
			DWORD options, const ::RECT& rect, HRGN region, ::POINT pt, WORD& result) const;
//	private:
//		static uchar dllLoaded_;
	};


	Theme::Theme(HTHEME handle /* = 0 */) throw() : HandleHolder<HTHEME, ::CloseThemeData>(handle) {}

	HRESULT Theme::close() {
		HRESULT hr = ::CloseThemeData(get());
		if(SUCCEEDED(hr))
			release();
		return hr;
	}

	HRESULT Theme::drawBackground(HDC dc, int partID, int stateID, const ::RECT& rect, const ::RECT* clipRect /* = 0 */) {
		assert(get() != 0); return ::DrawBackground(get(), dc, partID, stateID, &rect, clipRect);}

	HRESULT Theme::drawBackground(HDC dc, int partID, int stateID, const ::RECT& rect, const ::DTBGOPTS* options /* = 0 */) {
		assert(get() != 0); return ::DrawBackgroundEx(get(), dc, partID, stateID, &rect, options);}

	HRESULT Theme::drawEdge(HDC dc, int partID, int stateID, const ::RECT& destRect, UINT edge, UINT flags, ::RECT* contentRect /* = 0 */) {
		assert(get() != 0); return ::DrawThemeEdge(get(), dc, partID, stateID, &destRect, edge, flags, contentRect);}

	HRESULT Theme::drawIcon(HDC dc, int partID, int stateID, const ::RECT& rect, HIMAGELIST imageList, int index) {
		assert(get() != 0); return ::DrawThemeIcon(get(), dc, partID, stateID, &rect, imageList, index);}

	HRESULT	Theme::drawText(HDC dc, int partID, int stateID, const WCHAR* text, int charCount, DWORD flags, DWORD flags2, const ::RECT& rect) {
		assert(get() != 0); return ::DrawThemeText(get(), dc, partID, stateID, text, charCount, flags, flags2, &rect);}

	HRESULT Theme::getBackgroundContentRect(HDC dc, int partID, int stateID, const ::RECT& bounding, ::RECT& contentRect) const {
		assert(get() != 0); return ::GetThemeBackgroundContentRect(get(), dc, partID, stateID, &bounding, &contentRect);}

	HRESULT Theme::getBackgroundExtent(HDC dc, int partID, int stateID, const ::RECT& contentRect, ::RECT& extentRect) const {
		assert(get() != 0); return ::GetThemeBackgroundExtent(get(), dc, partID, stateID, &contentRect, &extentRect);}

	HRESULT Theme::getBackgroundRegion(HDC dc, int partID, int stateID, const ::RECT& rect, HRGN& region) const {
		assert(get() != 0); return ::GetThemeBackgroundRegion(get(), dc, partID, stateID, &rect, &region);}

	HRESULT Theme::getBool(int partID, int stateID, int propertyID, bool& value) const {
		assert(get() != 0);
		BOOL temp;
		const HRESULT hr = ::GetThemeBool(partID, stateID, propertyID, &value);
		value = toBoolean(temp);
		return hr;
	}

	HRESULT Theme::getColor(int partID, int stateID, int propertyID, COLORREF& color) const {
		assert(get() != 0); return ::GetThemeColor(get(), partID, stateID, propertyID, &color);}

	HRESULT Theme::getEnumValue(int partID, int stateID, int propertyID, int& value) const {
		assert(get() != 0); return ::GetThemeEnumValue(get(), partID, stateID, propertyID, &value);}

	HRESULT Theme::getInt(int partID, int stateID, int propertyID, int& value) const {
		assert(get() != 0); return ::GetThemeInt(get(), partID, stateID, propertyID, &color);}

	HRESULT Theme::getMetric(HDC dc, int partID, int stateID, int propertyID, int& value) const {
		assert(get() != 0); return ::GetThemeMetric(get(), dc, partID, stateID, propertyID, &value);}

	HRESULT Theme::getPartSize(HDC dc, int partID, int stateID, ::RECT* rect, ::THEMESIZE type, ::SIZE& size) const {
		assert(get() != 0); return ::GetThemePartSize(get(), dc, partID, stateID, rect, type, &size);}

	HRESULT Theme::getPosition(int partID, int stateID, int propertyID, ::POINT& pt) const {
		assert(get() != 0); return ::GetThemePosition(get(), partID, stateID, propertyID, &pt);}

	HRESULT Theme::getString(int partID, int stateID, int propertyID, WCHAR* buffer, int maxBufferChars) const {
		assert(get() != 0); return ::GetThemeString(get(), partID, stateID, propertyID, buffer, maxBufferChars);}

	HRESULT Theme::getTextExtent(HDC dc, int partID, int stateID,
			const WCHAR* text, int charCount, DWORD flags, const ::RECT* bounding, ::RECT& extentRect) const {
		assert(get() != 0); return ::GetThemeTextExtent(dc, partID, stateID, text, charCount, flags, bounding, &extentRect);}

	HRESULT Theme::getTextMetrics(HDC dc, int partID, int stateID, ::TEXTMETRICW& textMetric) const {
#ifdef UNICODE
		assert(get() != 0); return ::GetThemeTextMetrics(get(), dc, partID, stateID, &textMetric);
#else
		assert(get() != 0); return ::GetThemeTextMetrics(get(), dc, partID, stateID, reinterpret_cast<::TEXTMETRIC>(&textMetric));
#endif
	}

	HRESULT Theme::hitTestBackground(HDC dc, int partID, int stateID,
			DWORD options, const ::RECT& rect, HRGN region, ::POINT pt, WORD& result) const {
		assert(get() != 0); return HitTestThemeBackground(get(), dc, partID, stateID, options, &rect, region, pt, &result);}

	bool Theme::isBackgroundPartiallyTransparent(int partID, int stateID) const {
		assert(get() != 0); return toBoolean(::IsThemeBackgroundPartiallyTransparent(get(), partID, stateID));}

	bool Theme::isPartDefined(int partID, int stateID) const {
		assert(get() != 0); return toBoolean(::IsThemePartDefined(get(), partID, stateID));}

	bool Theme::open(HWND window, const WCHAR* classList) {
		if(get() != 0)
			return false;
		else if(HTHEME handle = ::OpenThemeData(window, classList)) {
			setHandle(handle);
			return true;
		}
		return false;
	}

}}}	// namespace manah.win32.ui

#endif /* !MANAH_THEME_HPP */
