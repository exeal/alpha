// dialog.hpp
// (c) 2003-2009 exeal

#ifndef MANAH_DIALOG_HPP
#define MANAH_DIALOG_HPP

#include "common-controls.hpp"
#include <map>

namespace manah {
namespace win32 {
namespace ui {

class BaseDialog {
protected:
	virtual void bindControls() {}
};

class Dialog : public Window, public BaseDialog {
	MANAH_NONCOPYABLE_TAG(Dialog);
public:
	// constructors
	Dialog();
	Dialog(HINSTANCE hinstance, const ResourceID& id);
	virtual ~Dialog();
	void initialize(HINSTANCE instance, const ResourceID& id);
	// open/close
	INT_PTR	doModal(HWND parent);
	INT_PTR	doModal(Window& parent) {return doModal(parent.get());}
	bool doModeless(HWND parent, bool show = true);
	bool doModeless(Window& parent, bool show = true) {return doModeless(parent.get(), show);}
	void end(int result);
	// control attributes
	bool addToolTip(HWND control, const WCHAR* text = LPSTR_TEXTCALLBACKW);
	bool addToolTip(UINT controlID, const WCHAR* text = LPSTR_TEXTCALLBACKW);
	bool check2StateButton(int buttonID, bool check = true);
	bool checkButton(int buttonID, UINT check);
	bool checkRadioButton(int firstButtonID, int lastButtonID, int buttonID);
	int getCheckedRadioButton(int firstButtonID, int lastButtonID) const;
	DWORD getDefaultID() const;
	HWND getItem(int itemID) const;
	int getItemInt(int itemID, bool* translated = 0, bool isSigned = true) const;
	int getItemText(int itemID, WCHAR* text, int maxLength) const;
	UINT isButtonChecked(int buttonID) const;
	LRESULT	sendItemMessage(int itemID, UINT message, WPARAM wParam, LPARAM lParam);
	void setDefaultID(UINT id);
	void setItemInt(int itemID, UINT value, bool isSigned = true);
	void setItemText(int itemID, const WCHAR* text);
	// control iteration
	HWND getNextGroupItem(HWND control, bool previous = false) const;
	HWND getNextGroupItem(int itemID, bool previous = false) const;
	HWND getNextTabItem(HWND control, bool previous = false) const;
	HWND getNextTabItem(int itemID, bool previous = false) const;
	void nextControl() const;
	void previousControl() const;
	// miscellaneous
	bool isDialogMessage(const MSG& msg);
protected:
	virtual INT_PTR	processWindowMessage(UINT, WPARAM, LPARAM) {return false;}
	void setMessageResult(LRESULT result);
private:
	using Window::create;
	using Window::destroy;
	using Window::processWindowMessage;
	using Window::preTranslateWindowMessage;
	static INT_PTR CALLBACK	windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

protected:
	/* ? need to any implement ? */
	virtual void onClose(bool& /* continueDialog */) {}												// WM_CLOSE
	virtual bool onCommand(WORD /* id */, WORD /* notifyCode */, HWND /* control */) {return true;}	// WM_COMMAND
	virtual void onInitDialog(HWND /* focusedWindow */, bool& /* focusDefault */) {}				// WM_INITDIALOG
	virtual void onOK(bool& /* continueDialog */) {}												// IDOK
	virtual void onCancel(bool& /* continueDialog */) {}											// IDCANCEL

private:
	HINSTANCE hinstance_;
	const WCHAR* templateName_;
	bool modeless_;
	ToolTipCtrl toolTips_;
};


// fixed dialog template id version (template is contained in app module)
template<int id> class FixedIDDialog : public Dialog {
public:
	FixedIDDialog() : Dialog(::GetModuleHandleW(0), id) {}
private:
	using Dialog::initialize;
};


// Control binding macros ///////////////////////////////////////////////////

#define MANAH_BEGIN_CONTROL_BINDING()	private: void bindControls() {

#define MANAH_BIND_CONTROL(id, name)	name.reset(getItem(id));

#define MANAH_END_CONTROL_BINDING()	}


// Dialog ///////////////////////////////////////////////////////////////////

inline Dialog::Dialog() : hinstance_(0), templateName_(0), modeless_(false) {}

inline Dialog::Dialog(HINSTANCE hinstance, const ResourceID& id) : hinstance_(hinstance), templateName_(id), modeless_(false) {}

inline Dialog::~Dialog() {}

inline bool Dialog::addToolTip(HWND control, const WCHAR* text /* = LPSTR_TEXTCALLBACK */) {
	if(use() != ::GetParent(control))
		return false;
	return toolTips_.addTool(control, TTF_SUBCLASS, text);
}

inline bool Dialog::addToolTip(UINT controlID, const WCHAR* text /* = LPSTR_TEXTCALLBACK */) {return addToolTip(getItem(controlID), text);}

inline bool Dialog::check2StateButton(int buttonID, bool check /* = true */) {return checkButton(buttonID, check ? BST_CHECKED : BST_UNCHECKED);}

inline bool Dialog::checkButton(int buttonID, UINT check) {return toBoolean(::CheckDlgButton(use(), buttonID, check));}

inline bool Dialog::checkRadioButton(int firstButtonID, int lastButtonID, int buttonID) {return toBoolean(::CheckRadioButton(use(), firstButtonID, lastButtonID, buttonID));}

inline INT_PTR Dialog::doModal(HWND parent) {
	modeless_ = false; return ::DialogBoxParamW(hinstance_, templateName_, parent, Dialog::windowProcedure, reinterpret_cast<LPARAM>(this));}

inline bool Dialog::doModeless(HWND parent, bool show /* = true */) {
	if(HWND handle = ::CreateDialogParamW(hinstance_, templateName_, parent, Dialog::windowProcedure, reinterpret_cast<LPARAM>(this))) {
		modeless_ = true;
		if(show)
			Window::show(SW_SHOW);
		return true;
	}
	return false;
}

inline void Dialog::end(int result) {
	::EndDialog(use(), result);
	if(modeless_)
		destroy();
}

inline int Dialog::getCheckedRadioButton(int firstButtonID, int lastButtonID) const {
	check();
	for(int id = firstButtonID; id <= lastButtonID; ++id) {
		if(isButtonChecked(id) == BST_CHECKED)
			return id;
	}
	return 0;
}

inline DWORD Dialog::getDefaultID() const {return static_cast<DWORD>(const_cast<Dialog*>(this)->sendMessage(DM_GETDEFID));}

inline HWND Dialog::getItem(int itemID) const {return ::GetDlgItem(use(), itemID);}

inline int Dialog::getItemInt(int itemID, bool* translated /* = 0 */, bool isSigned /* = true */) const {
	return ::GetDlgItemInt(use(), itemID, reinterpret_cast<BOOL*>(translated), isSigned);}

inline int Dialog::getItemText(int itemID, WCHAR* text, int maxLength) const {return ::GetDlgItemTextW(use(), itemID, text, maxLength);}

inline HWND Dialog::getNextGroupItem(HWND control, bool previous /* = false */) const {return ::GetNextDlgGroupItem(use(), control, previous);}

inline HWND Dialog::getNextGroupItem(int itemID, bool previous /* = false */) const {return ::GetNextDlgGroupItem(use(), getItem(itemID), previous);}

inline HWND Dialog::getNextTabItem(HWND control, bool previous /* = false */) const {return ::GetNextDlgTabItem(use(), control, previous);}

inline HWND Dialog::getNextTabItem(int itemID, bool previous /* = false */) const {return ::GetNextDlgTabItem(use(), getItem(itemID), previous);}

inline void Dialog::initialize(HINSTANCE hinstance, const ResourceID& id) {hinstance_ = hinstance; templateName_ = id;}

inline bool Dialog::isDialogMessage(const MSG& msg) {return toBoolean(::IsDialogMessage(use(), const_cast<MSG*>(&msg)));}

inline UINT Dialog::isButtonChecked(int buttonID) const {return ::IsDlgButtonChecked(use(), buttonID);}

inline void Dialog::nextControl() const {::SetFocus(getNextTabItem(getFocus().get(), false));}

inline void Dialog::previousControl() const {::SetFocus(getNextTabItem(getFocus().get(), true));}

inline LRESULT Dialog::sendItemMessage(int itemID, UINT message, WPARAM wParam, LPARAM lParam) {return ::SendDlgItemMessageW(use(), itemID, message, wParam, lParam);}

inline void Dialog::setDefaultID(UINT id) {sendMessage(DM_SETDEFID, id);}

inline void Dialog::setItemInt(int itemID, UINT value, bool isSigned /* = true */) {::SetDlgItemInt(use(), itemID, value, isSigned);}

inline void Dialog::setItemText(int itemID, const WCHAR* text) {::SetDlgItemTextW(use(), itemID, text);}

inline void Dialog::setMessageResult(LRESULT result) {
#ifdef _WIN64
	setWindowLongPtr(DWLP_MSGRESULT, result);
#else
	setWindowLong(DWL_MSGRESULT, static_cast<long>(result));
#endif // _WIN64
}

inline INT_PTR CALLBACK Dialog::windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
	if(message == WM_INITDIALOG) {
		Dialog* p = reinterpret_cast<Dialog*>(lParam);
		p->reset(window);
#ifdef _WIN64
		::SetWindowLongPtr(p->get(), DWLP_USER, reinterpret_cast<LONG_PTR>(p));
#else
		::SetWindowLong(p->get(), DWL_USER, static_cast<long>(reinterpret_cast<LONG_PTR>(p)));
#endif // _WIN64
		p->bindControls();
		p->toolTips_.create(window);
		p->toolTips_.activate(true);
		bool defaultFocus = true;
		p->onInitDialog(reinterpret_cast<HWND>(wParam), defaultFocus);
		return defaultFocus;
	} else {
#ifdef _WIN64
		Dialog* p = reinterpret_cast<Dialog*>(::GetWindowLongPtr(window, DWLP_USER));
#else
		Dialog* p = reinterpret_cast<Dialog*>(static_cast<LONG_PTR>(::GetWindowLong(window, DWL_USER)));
#endif // _WIN64
		if(p == 0)
			return false;
		if(message == WM_CLOSE) {
			bool continueDialog = false;
			p->onClose(continueDialog);
			if(!continueDialog) {
				p->toolTips_.destroy();
				p->end(IDCANCEL);
			}
			return true;
		} else if(message == WM_COMMAND) {
			bool continueDialog = false;
			switch(LOWORD(wParam)) {
			case IDOK:
				p->onOK(continueDialog); if(!continueDialog) p->end(IDOK); return true;
			case IDCANCEL:
				p->onCancel(continueDialog); if(!continueDialog) p->end(IDCANCEL); return true;
			default: return p->onCommand(LOWORD(wParam), HIWORD(wParam), reinterpret_cast<HWND>(lParam));
			}
		} else
			return p->processWindowMessage(message, wParam, lParam);
	}
}

}}} // namespace manah.win32.ui

#endif // !MANAH_DIALOG_HPP
