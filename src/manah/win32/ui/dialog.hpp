// dialog.hpp
// (c) 2003-2007 exeal

#ifndef MANAH_DIALOG_HPP
#define MANAH_DIALOG_HPP

#include "common-controls.hpp"
#include <map>

namespace manah {
namespace windows {
namespace ui {

class BaseDialog {
protected:
	virtual void bindControls() {}
};

class Dialog : public Window, public BaseDialog, public Noncopyable {
	// コンストラクタ
public:
	Dialog();
	Dialog(HINSTANCE hinstance, const ResourceID& id);
	virtual ~Dialog();
	
	// メソッド
public:
	bool	addToolTip(HWND control, const TCHAR* text = LPSTR_TEXTCALLBACK);
	bool	addToolTip(UINT controlID, const TCHAR* text = LPSTR_TEXTCALLBACK);
	bool	checkDlg2StateButton(int buttonID, bool check = true);
	bool	checkDlgButton(int buttonID, UINT check);
	bool	checkRadioButton(int firstButtonID, int lastButtonID, int buttonID);
	void	create(HINSTANCE instance, const ResourceID& id);
	INT_PTR	doModal(HWND parent);
	bool	doModeless(HWND parent, bool show = true);
	void	endDialog(int result);
	int		getCheckedRadioButton(int firstButtonID, int lastButtonID) const;
	DWORD	getDefID() const;
	HWND	getDlgItem(int itemID) const;
	int		getDlgItemInt(int itemID, bool* translated = 0, bool isSigned = true) const;
	int		getDlgItemText(int itemID, TCHAR* text, int maxLength) const;
	HWND	getNextDlgGroupItem(HWND control, bool previous = false) const;
	HWND	getNextDlgGroupItem(int itemID, bool previous = false) const;
	HWND	getNextDlgTabItem(HWND control, bool previous = false) const;
	HWND	getNextDlgTabItem(int itemID, bool previous = false) const;
	bool	isDialogMessage(const MSG& msg);
	UINT	isDlgButtonChecked(int buttonID) const;
	void	nextDlgCtrl() const;
	void	prevDlgCtrl() const;
	LRESULT	sendDlgItemMessage(int itemID, UINT message, WPARAM wParam, LPARAM lParam);
	void	setDefID(UINT id);
	void	setDlgItemInt(int itemID, UINT value, bool isSigned = true);
	void	setDlgItemText(int itemID, const TCHAR* text);
protected:
	virtual LRESULT	dispatchEvent(UINT message, WPARAM wParam, LPARAM lParam);
private:
	static INT_PTR CALLBACK	windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK	dummyProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {return false;}

	// メッセージハンドラ
protected:
	/* ? need to any implement ? */
	virtual void	onClose() {toolTips_.destroyWindow(); endDialog(IDCLOSE);}			// WM_CLOSE
	virtual bool	onCommand(WORD id, WORD notifyCode, HWND control);					// WM_COMMAND
	virtual bool	onInitDialog(HWND focusedWindow, LPARAM initParam) {return true;}	// WM_INITDIALOG
	virtual void	onOK() {endDialog(IDOK);}											// IDOK
	virtual void	onCancel() {endDialog(IDCANCEL);}									// IDCANCEL
	/* ? noneed to implement ? */
	virtual void	onActivate(UINT state, HWND previousWindow, bool minimized) {}		// WM_ACTIVATE

	// データメンバ
private:
	HINSTANCE		hinstance_;
	const TCHAR*	templateName_;
	bool			modeless_;
	ToolTipCtrl		toolTips_;
};


// fixed dialog template id version (template is contained in app module)
template<int id> class FixedIDDialog : public Dialog {
public:
	FixedIDDialog() : Dialog(::GetModuleHandle(0), id) {}
private:
	bool	create();
};


// Control binding macros ///////////////////////////////////////////////////

#define BEGIN_CONTROL_BINDING()	private: void bindControls() {

#define BIND_CONTROL(id, name)	name.attach(getDlgItem(id));

#define END_CONTROL_BINDING()	}


// Dialog ///////////////////////////////////////////////////////////////////

inline Dialog::Dialog() : hinstance_(0), templateName_(0), modeless_(false) {}

inline Dialog::Dialog(HINSTANCE hinstance, const ResourceID& id) : hinstance_(hinstance), templateName_(id.name), modeless_(false) {}

inline Dialog::~Dialog() {if(isWindow()) destroyWindow(); setHandle(0);}

inline bool Dialog::addToolTip(HWND control, const TCHAR* text /* = LPSTR_TEXTCALLBACK */) {
	assertValidAsWindow();
	if(get() != ::GetParent(control))
		return false;
	return toolTips_.addTool(control, TTF_SUBCLASS, text);
}

inline bool Dialog::addToolTip(UINT controlID, const TCHAR* text /* = LPSTR_TEXTCALLBACK */) {
	assertValidAsWindow(); return addToolTip(getDlgItem(controlID), text);}

inline bool Dialog::checkDlg2StateButton(int buttonID, bool check /* = true */) {
	return checkDlgButton(buttonID, check ? BST_CHECKED : BST_UNCHECKED);}

inline bool Dialog::checkDlgButton(int buttonID, UINT check) {
	assertValidAsWindow(); return toBoolean(::CheckDlgButton(get(), buttonID, check));}

inline bool Dialog::checkRadioButton(int firstButtonID, int lastButtonID, int buttonID) {
	assertValidAsWindow(); return toBoolean(::CheckRadioButton(get(), firstButtonID, lastButtonID, buttonID));}

inline void Dialog::create(HINSTANCE hinstance, const ResourceID& id) {
	hinstance_ = hinstance;
	templateName_ = id.name;
}

inline LRESULT Dialog::dispatchEvent(UINT message, WPARAM wParam, LPARAM lParam) {
	if(preTranslateMessage(message, wParam, lParam))
		return true;

	switch(message) {
	case WM_ACTIVATE:
		onActivate(LOWORD(wParam), reinterpret_cast<HWND>(lParam), toBoolean(HIWORD(wParam)));
		break;
	case WM_CLOSE:
		onClose();
		return true;
	case WM_COMMAND:
		return onCommand(LOWORD(wParam), HIWORD(wParam), reinterpret_cast<HWND>(lParam));
	case WM_CONTEXTMENU: {
			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
			if(onContextMenu(reinterpret_cast<HWND>(wParam), pt))
				return true;
		}
		break;
	case WM_DESTROY:
		onDestroy();
		return true;
	case WM_DRAWITEM:
		onDrawItem(static_cast<UINT>(wParam), reinterpret_cast<LPDRAWITEMSTRUCT>(lParam));
		break;
	case WM_INITDIALOG:
		return onInitDialog(reinterpret_cast<HWND>(wParam), lParam);
	case WM_KILLFOCUS:
		onKillFocus(reinterpret_cast<HWND>(wParam));
		return true;
	case WM_LBUTTONDOWN: {
			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
			onLButtonDown(static_cast<UINT>(wParam), pt);
		}
		break;
	case WM_LBUTTONDBLCLK: {
			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
			onLButtonDblClk(static_cast<UINT>(wParam), pt);
		}
		break;
	case WM_LBUTTONUP: {
			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
			onLButtonUp(static_cast<UINT>(wParam), pt);
		}
		break;
	case WM_MEASUREITEM:
		onMeasureItem(static_cast<UINT>(wParam), reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam));
		break;
	case WM_MOUSEMOVE: {
			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
			onMouseMove(static_cast<UINT>(wParam), pt);
		}
		break;
	case WM_NOTIFY:
		return onNotify(static_cast<UINT>(wParam), reinterpret_cast<LPNMHDR>(lParam));
	case WM_SETCURSOR:
		if(onSetCursor(reinterpret_cast<HWND>(wParam),
			static_cast<UINT>(LOWORD(lParam)), static_cast<UINT>(HIWORD(lParam))))
			return false;
		break;
	case WM_SETFOCUS:
		onSetFocus(reinterpret_cast<HWND>(wParam));
		return true;
	case WM_SHOWWINDOW:
		onShowWindow(toBoolean(wParam), static_cast<UINT>(lParam));
		break;
	case WM_SIZE:
		onSize(static_cast<UINT>(wParam), LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_SYSCOLORCHANGE:
		onSysColorChange();
		break;
	case WM_TIMER:
		onTimer(static_cast<UINT>(wParam));
		break;
	}

	return false;
}

inline INT_PTR Dialog::doModal(HWND parent) {
	modeless_ = false;
	return ::DialogBoxParam(hinstance_, templateName_, parent, Dialog::windowProcedure, reinterpret_cast<LPARAM>(this));
}

inline bool Dialog::doModeless(HWND parent, bool show /* = true */) {
	if(HWND handle = ::CreateDialogParam(hinstance_, templateName_, parent, Dialog::windowProcedure, reinterpret_cast<LPARAM>(this))) {
		modeless_ = true;
		if(show)
			showWindow(SW_SHOW);
		return true;
	}
	return false;
}

inline void Dialog::endDialog(int result) {
	assertValidAsWindow();
	::EndDialog(get(), result);
	if(modeless_)
		destroyWindow();
}

inline int Dialog::getCheckedRadioButton(int firstButtonID, int lastButtonID) const {
	assertValidAsWindow();
	for(int id = firstButtonID; id <= lastButtonID; ++id) {
		if(isDlgButtonChecked(id) == BST_CHECKED)
			return id;
	}
	return 0;
}

inline DWORD Dialog::getDefID() const {return static_cast<DWORD>(const_cast<Dialog*>(this)->sendMessage(DM_GETDEFID));}

inline HWND Dialog::getDlgItem(int itemID) const {assertValidAsWindow(); return ::GetDlgItem(get(), itemID);}

inline int Dialog::getDlgItemInt(int itemID, bool* translated /* = 0 */, bool isSigned /* = true */) const {
	assertValidAsWindow(); return ::GetDlgItemInt(get(), itemID, reinterpret_cast<BOOL*>(translated), isSigned);}

inline int Dialog::getDlgItemText(int itemID, TCHAR* text, int maxLength) const {
	assertValidAsWindow(); return ::GetDlgItemText(get(), itemID, text, maxLength);}

inline HWND Dialog::getNextDlgGroupItem(HWND control, bool previous /* = false */) const {
	assertValidAsWindow(); return ::GetNextDlgGroupItem(get(), control, previous);}

inline HWND Dialog::getNextDlgGroupItem(int itemID, bool previous /* = false */) const {
	assertValidAsWindow(); return ::GetNextDlgGroupItem(get(), getDlgItem(itemID), previous);
}

inline HWND Dialog::getNextDlgTabItem(HWND control, bool previous /* = false */) const {
	assertValidAsWindow(); return ::GetNextDlgTabItem(get(), control, previous);}

inline HWND Dialog::getNextDlgTabItem(int itemID, bool previous /* = false */) const {
	assertValidAsWindow(); return ::GetNextDlgTabItem(get(), getDlgItem(itemID), previous);}

inline bool Dialog::isDialogMessage(const MSG& msg) {
	assertValidAsWindow(); return toBoolean(::IsDialogMessage(get(), const_cast<MSG*>(&msg)));}

inline UINT Dialog::isDlgButtonChecked(int buttonID) const {assertValidAsWindow(); return ::IsDlgButtonChecked(get(), buttonID);}

inline void Dialog::nextDlgCtrl() const {assertValidAsWindow(); ::SetFocus(getNextDlgTabItem(getFocus(), false));}

inline void Dialog::prevDlgCtrl() const {assertValidAsWindow(); ::SetFocus(getNextDlgTabItem(getFocus(), true));}

inline LRESULT Dialog::sendDlgItemMessage(int itemID, UINT message, WPARAM wParam, LPARAM lParam) {
	assertValidAsWindow(); return ::SendDlgItemMessage(get(), itemID, message, wParam, lParam);}

inline void Dialog::setDefID(UINT id) {assertValidAsWindow(); sendMessage(DM_SETDEFID, id);}

inline void Dialog::setDlgItemInt(int itemID, UINT value, bool isSigned /* = true */) {
	assertValidAsWindow(); ::SetDlgItemInt(get(), itemID, value, isSigned);}

inline void Dialog::setDlgItemText(int itemID, const TCHAR* text) {assertValidAsWindow(); ::SetDlgItemText(get(), itemID, text);}

inline INT_PTR CALLBACK Dialog::windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
	if(message == WM_INITDIALOG) {
		Dialog* instance = reinterpret_cast<Dialog*>(lParam);
		instance->setHandle(window);
#ifdef _WIN64
		::SetWindowLongPtr(instance->get(), DWLP_USER, reinterpret_cast<LONG_PTR>(instance));
#else
		::SetWindowLong(instance->get(), DWL_USER, static_cast<long>(reinterpret_cast<LONG_PTR>(instance)));
#endif /* _WIN64 */
		instance->bindControls();
		instance->toolTips_.create(window);
		instance->toolTips_.activate(true);
		return instance->dispatchEvent(message, wParam, lParam);
	} else {
#ifdef _WIN64
		Dialog* instance = reinterpret_cast<Dialog*>(::GetWindowLongPtr(window, DWLP_USER));
#else
		Dialog* instance = reinterpret_cast<Dialog*>(static_cast<LONG_PTR>(::GetWindowLong(window, DWL_USER)));
#endif /* _WIN64 */
		return (instance != 0) ? instance->dispatchEvent(message, wParam, lParam) : false;
	}
}

inline bool Dialog::onCommand(WORD id, WORD notifyCode, HWND control) {
	switch(id) {
	case IDOK:		onOK();		return true;
	case IDCANCEL:	onCancel();	return true;
	default:					return false;
	}
}

}}} // namespace manah::windows::ui

#endif /* MANAH_DIALOG_HPP */
