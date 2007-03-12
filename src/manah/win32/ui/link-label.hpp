// link-lable.hpp
// (c) 2004-2007 exeal

#ifndef MANAH_LINK_LABEL_HPP
#define MANAH_LINK_LABEL_HPP
#include "common-controls.hpp"
#include <stdexcept>

namespace manah {
namespace win32 {
namespace ui {

// 前は色々機能があったけどメンドいんでやーめた

class LinkLabel : public CustomControl<LinkLabel>, public Noncopyable {
	DEFINE_WINDOW_CLASS() {
		name = _T("ManahLinkLabel");
		style = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW | CS_DBLCLKS;
		cursor = MAKEINTRESOURCE(32649);	// IDC_HAND
	}

public:
	LinkLabel();
public:
	bool			create(HWND parent, HINSTANCE hinstance, int id = 0);
	const TCHAR*	getTipText() const;
	void			setTipText(const TCHAR* text);

protected:
	MANAH_DECLEAR_WINDOW_MESSAGE_MAP(LinkLabel);
	UINT	onGetDlgCode();											// WM_GETDLGCODE
	HFONT	onGetFont();											// WM_GETFONT
	void	onKeyDown(UINT vkey, UINT flags, bool& handled);		// WM_KEYDOWN
	void	onKillFocus(HWND newWindow);							// WM_KILLFOCUS
	void	onLButtonDown(UINT flags, const POINT& pt);				// WM_LBUTTONDOWN
	void	onLButtonUp(UINT flags, const POINT& pt);				// WM_LBUTTONUP
	void	onPaint(gdi::PaintDC& dc);								// WM_PAINT
	bool	onSetCursor(HWND window, UINT hitTest, UINT message);	// WM_SETCURSOR
	void	onSetFocus(HWND oldWindow);								// WM_SETFOCUS
	bool	onSetText(const TCHAR* text);							// WM_SETTEXT
	void	onSettingChange(UINT flags, const TCHAR* sectionName);	// WM_SETTINGCHANGE
private:
	void recreateFont();
	AutoBuffer<TCHAR> tipText_;
	HFONT font_;
};


inline MANAH_BEGIN_WINDOW_MESSAGE_MAP(LinkLabel, CustomControl<LinkLabel>)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_GETDLGCODE)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_GETFONT)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_KEYDOWN)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_KILLFOCUS)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_LBUTTONDOWN)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_LBUTTONUP)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_SETCURSOR)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_SETFOCUS)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_SETTEXT)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_SETTINGCHANGE)
MANAH_END_WINDOW_MESSAGE_MAP()

inline LinkLabel::LinkLabel() {}

inline bool LinkLabel::create(HWND parent, HINSTANCE hinstance, int id /* = 0 */) {
	assert(parent == 0 || toBoolean(::IsWindow(parent)));

	if(!CustomControl<LinkLabel>::create(parent, DefaultWindowRect(), 0, WS_CHILD | WS_TABSTOP | WS_VISIBLE))
		return false;
	recreateFont();
	if(id != 0)
#ifdef _WIN64
		setWindowLongPtr(GWLP_ID, id);
#else
		setWindowLong(GWL_ID, id);
#endif /* _WIN64 */
	return true;
}

inline const TCHAR* LinkLabel::getTipText() const {return tipText_.get();}

inline void LinkLabel::setTipText(const TCHAR* text) {
	assert(text != 0);
	tipText_.reset(new TCHAR[std::_tcslen(text) + 1]);
	std::_tcscpy(tipText_.get(), text);
}

inline UINT LinkLabel::onGetDlgCode() {return DLGC_BUTTON | DLGC_UNDEFPUSHBUTTON;}

inline HFONT LinkLabel::onGetFont() {return font_;}

inline void LinkLabel::onKeyDown(UINT vkey, UINT flags, bool&) {
	if(!toBoolean(getStyle() & WS_DISABLED) && vkey == VK_RETURN)
#ifdef _WIN64
		getParent()->sendMessage(WM_COMMAND, getWindowLongPtr(GWLP_ID), reinterpret_cast<LPARAM>(get()));
#else
		getParent()->sendMessage(WM_COMMAND, getWindowLong(GWL_ID), reinterpret_cast<LPARAM>(get()));
#endif /* _WIN64 */
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
	getParent()->sendMessage(WM_COMMAND,
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
	const std::size_t len = getTextLength();
	if(len == 0)
		return;

	TCHAR* const caption = new TCHAR[len + 1];
	HFONT oldFont = dc.selectObject(font_);
	::RECT rect;

	getText(caption, static_cast<int>(len + 1));
	dc.setTextColor(::GetSysColor(toBoolean(getStyle() & WS_DISABLED) ? COLOR_GRAYTEXT : COLOR_HOTLIGHT));
	dc.setBkMode(TRANSPARENT);

	getRect(rect);
	::OffsetRect(&rect, -rect.left, -rect.top);
	::InflateRect(&rect, -1, -1);

	dc.drawText(caption, static_cast<int>(len), rect, DT_LEFT | DT_TOP | DT_SINGLELINE);
	delete[] caption;
	dc.selectObject(oldFont);
}

inline bool LinkLabel::onSetCursor(HWND, UINT, UINT) {
	if(toBoolean(getStyle() & WS_DISABLED)) {
		::SetCursor(::LoadCursor(0, IDC_ARROW));
		return true;
	} else
		return false;
}

inline void LinkLabel::onSetFocus(HWND) {
	::RECT rect;
	getClientRect(rect);
	::OffsetRect(&rect, -rect.left, -rect.top);
	getDC().drawFocusRect(rect);
}

inline bool LinkLabel::onSetText(const TCHAR* text) {
	gdi::ClientDC dc = getDC();
	RECT rect = {0, 0, 0, 0};
	HFONT oldFont = dc.selectObject(getFont());
	dc.drawText(text, -1, rect, DT_CALCRECT);
	dc.selectObject(oldFont);
	rect.right += 2;
	rect.bottom += 2;
	setPosition(0, rect, SWP_NOMOVE | SWP_NOZORDER);
	return false;
}

inline void LinkLabel::onSettingChange(UINT, const TCHAR*) {recreateFont();}

inline void LinkLabel::recreateFont() {
	::DeleteObject(font_);
	font_ = getParent()->getFont();
	::LOGFONT lf;
	if(font_ != 0)
		::GetObject(font_, sizeof(::LOGFONT), &lf);
	else {
		AutoZeroCB<::NONCLIENTMETRICS> ncm;
		::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(::NONCLIENTMETRICS), &ncm, 0);
		lf = ncm.lfStatusFont;
	}
	lf.lfUnderline = true;
	font_ = ::CreateFontIndirect(&lf);
}

}}} // namespace manah.win32.ui

#endif /* !MANAH_LINK_LABEL_HPP */
