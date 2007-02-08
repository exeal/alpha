// link-lable.hpp
// (c) 2004-2007 exeal

#ifndef MANAH_LINK_LABEL_HPP
#define MANAH_LINK_LABEL_HPP
#include "common-controls.hpp"
#include <stdexcept>

namespace manah {
namespace windows {
namespace ui {

// 前は色々機能があったけどメンドいんでやーめた

class LinkLabel : public CustomControl<LinkLabel>, public Noncopyable {
	DEFINE_WINDOW_CLASS() {
		name = _T("ManahLinkLabel");
		style = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW | CS_DBLCLKS;
		cursor = MAKEINTRESOURCE(32649);	// IDC_HAND
	}

	// コンストラクタ
public:
	LinkLabel();
	virtual ~LinkLabel();

	// メソッド
public:
	bool			create(HWND parent, HINSTANCE hinstance, int id = 0);
	const TCHAR*	getTipText() const;
	void			setTipText(const TCHAR* text);
private:
	HFONT	getFont();

	// メッセージハンドラ
protected:
	virtual OVERRIDE_DISPATCH_EVENT(LinkLabel);
	virtual void	onKillFocus(HWND newWindow);							// WM_KILLFOCUS
	virtual void	onLButtonDown(UINT flags, const POINT& pt);				// WM_LBUTTONDOWN
	virtual void	onLButtonUp(UINT flags, const POINT& pt);				// WM_LBUTTONUP
	virtual void	onPaint(gdi::PaintDC& dc);								// WM_PAINT
	virtual bool	onSetCursor(HWND window, UINT hitTest, UINT message);	// WM_SETCURSOR
	virtual void	onSetFocus(HWND oldWindow);								// WM_SETFOCUS
	virtual void	onSetText(const TCHAR* text);							// WM_SETTEXT

	// データメンバ
private:
	TCHAR* tipText_;
};


inline LinkLabel::LinkLabel() : tipText_(0) {}

inline LinkLabel::~LinkLabel() {delete[] tipText_;}

inline bool LinkLabel::create(HWND parent, HINSTANCE hinstance, int id /* = 0 */) {
	assert(parent == 0 || toBoolean(::IsWindow(parent)));

	if(!CustomControl<LinkLabel>::create(parent, DefaultWindowRect(), 0, WS_CHILD | WS_TABSTOP | WS_VISIBLE))
		return false;
	if(id != 0)
#ifdef _WIN64
		setWindowLongPtr(GWLP_ID, id);
#else
		setWindowLong(GWL_ID, id);
#endif /* _WIN64 */
	return true;
}

inline LRESULT LinkLabel::dispatchEvent(UINT message, WPARAM wParam, LPARAM lParam) {
	if(preTranslateMessage(message, wParam, lParam))
		return false;

	switch(message) {
	case WM_GETDLGCODE:
		return DLGC_BUTTON | DLGC_UNDEFPUSHBUTTON;
	case WM_KEYDOWN:
		if(!toBoolean(getStyle() & WS_DISABLED) && wParam == VK_RETURN)
#ifdef _WIN64
			getParent().sendMessage(WM_COMMAND, getWindowLongPtr(GWLP_ID), reinterpret_cast<LPARAM>(get()));
#else
			getParent().sendMessage(WM_COMMAND, getWindowLong(GWL_ID), reinterpret_cast<LPARAM>(get()));
#endif /* _WIN64 */
		break;
	case WM_SETTEXT:
		onSetText(reinterpret_cast<TCHAR*>(lParam));
		break;
	}

	return CustomControl<LinkLabel>::dispatchEvent(message, wParam, lParam);
}

inline HFONT LinkLabel::getFont() {
	LOGFONT lf;
	::GetObject(getParent().getFont(), sizeof(LOGFONT), &lf);
	lf.lfUnderline = true;
	return ::CreateFontIndirect(&lf);	// may return null...
}

inline const TCHAR* LinkLabel::getTipText() const {return tipText_;}

inline void LinkLabel::setTipText(const TCHAR* text) {
	assert(text != 0);
	delete[] tipText_;
	tipText_ = new TCHAR[std::_tcslen(text) + 1];
	std::_tcscpy(tipText_, text);
}

inline void LinkLabel::onKillFocus(HWND) {
	gdi::ClientDC dc = getDC();
	RECT rect;
	getClientRect(rect);
	::OffsetRect(&rect, -rect.left, -rect.top);
	dc.drawFocusRect(rect);
}

inline void LinkLabel::onLButtonDown(UINT, const POINT&) {setFocus();}

inline void LinkLabel::onLButtonUp(UINT, const POINT&) {
	if(toBoolean(getStyle() & WS_DISABLED))
		return;
	getParent().sendMessage(WM_COMMAND,
#ifdef _WIN64
		getWindowLongPtr(GWLP_ID),
#else
		getWindowLong(GWL_ID),
#endif /* _WIN64 */
		reinterpret_cast<LPARAM>(get()));
}

inline void LinkLabel::onPaint(gdi::PaintDC& dc) {
#ifndef COLOR_HOTLIGHT
	const int COLOR_HOTLIGHT = 26;
#endif /* !COLOR_HOTLIGHT */
	const std::size_t len = getWindowTextLength();

	if(len == 0)
		return;

	TCHAR* const caption = new TCHAR[len + 1];
	HFONT oldFont = dc.selectObject(getFont());
	RECT rect;

	getWindowText(caption, static_cast<int>(len + 1));
	dc.setTextColor(::GetSysColor(toBoolean(getStyle() & WS_DISABLED) ? COLOR_GRAYTEXT : COLOR_HOTLIGHT));
	dc.setBkMode(TRANSPARENT);

	getWindowRect(rect);
	::OffsetRect(&rect, -rect.left, -rect.top);
	::InflateRect(&rect, -1, -1);

	dc.drawText(caption, static_cast<int>(len), rect, DT_LEFT | DT_TOP | DT_SINGLELINE);
	delete[] caption;
	::DeleteObject(dc.selectObject(oldFont));
}

inline bool LinkLabel::onSetCursor(HWND, UINT, UINT) {
	if(toBoolean(getStyle() & WS_DISABLED)) {
		::SetCursor(::LoadCursor(0, IDC_ARROW));
		return true;
	} else
		return false;
}

inline void LinkLabel::onSetFocus(HWND) {
	RECT rect;
	getClientRect(rect);
	::OffsetRect(&rect, -rect.left, -rect.top);
	getDC().drawFocusRect(rect);
}

inline void LinkLabel::onSetText(const TCHAR* text) {
	gdi::ClientDC dc = getDC();
	RECT rect = {0, 0, 0, 0};
	HFONT oldFont = dc.selectObject(getFont());
	dc.drawText(text, -1, rect, DT_CALCRECT);
	::DeleteObject(dc.selectObject(oldFont));
	rect.right += 2;
	rect.bottom += 2;
	setWindowPos(0, rect, SWP_NOMOVE | SWP_NOZORDER);
}

}}} // namespace manah::windows::ui

#endif /* !MANAH_LINK_LABEL_HPP */
