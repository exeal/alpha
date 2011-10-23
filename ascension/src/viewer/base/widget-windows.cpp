/**
 * @file widget-windows.cpp
 * @author exeal
 * @date 2011-03-27 created
 */

#include <ascension/viewer/base/widget.hpp>
#include <ascension/win32/com/com.hpp>

using namespace ascension;
using namespace ascension::graphics;
using namespace ascension::viewers::base;
using namespace std;


NativeRectangle Widget::bounds(bool includeFrame) const {
	RECT temp;
	if(includeFrame) {
		if(!win32::boole(::GetWindowRect(identifier().get(), &temp)))
			throw PlatformDependentError<>();
	} else {
		if(!win32::boole(::GetClientRect(identifier().get(), &temp)))
			throw PlatformDependentError<>();
	}
	return NativeRectangle(temp);
}

namespace {
	inline DropAction translateDropActions(DWORD effect) {
		DropAction result = DROP_ACTION_IGNORE;
		if(win32::boole(effect & DROPEFFECT_COPY))
			result |= DROP_ACTION_COPY;
		if(win32::boole(effect & DROPEFFECT_MOVE))
			result |= DROP_ACTION_MOVE;
		if(win32::boole(effect & DROPEFFECT_LINK))
			result |= DROP_ACTION_LINK;
		return result;
	}

	inline UserInput::MouseButton translateMouseButton(DWORD keyState) {
		UserInput::MouseButton result = 0;
		if(win32::boole(keyState & MK_LBUTTON))
			result |= UserInput::BUTTON1_DOWN;
		if(win32::boole(keyState & MK_RBUTTON))
			result |= UserInput::BUTTON3_DOWN;
		if(win32::boole(keyState & MK_MBUTTON))
			result |= UserInput::BUTTON2_DOWN;
		if(win32::boole(keyState & MK_XBUTTON1))
			result |= UserInput::BUTTON4_DOWN;
		if(win32::boole(keyState & MK_XBUTTON2))
			result |= UserInput::BUTTON5_DOWN;
		return result;
	}

	inline UserInput::ModifierKey translateModifierKey(DWORD keyState) {
		UserInput::ModifierKey result = 0;
		if(win32::boole(keyState & MK_SHIFT))
			result |= UserInput::SHIFT_DOWN;
		if(win32::boole(keyState & MK_CONTROL))
			result |= UserInput::CONTROL_DOWN;
		if(win32::boole(keyState & MK_ALT))
			result |= UserInput::ALT_DOWN;
		return result;
	}

	template<typename Point>
	inline MouseButtonInput makeMouseButtonInput(const Point& location, DWORD keyState) {
		return MouseButtonInput(
			geometry::make<NativePoint>(location.x, location.y),
			translateMouseButton(keyState), translateModifierKey(keyState));
	}
}

/// Implements @c IDropTarget#DragEnter method.
STDMETHODIMP Widget::DragEnter(IDataObject* data, DWORD keyState, POINTL position, DWORD* effect) {
	if(data == 0)
		return E_INVALIDARG;
	ASCENSION_WIN32_VERIFY_COM_POINTER(effect);

#ifdef _DEBUG
	{
		win32::DumpContext dout;
		win32::com::ComPtr<IEnumFORMATETC> formats;
		HRESULT hr;
		if(SUCCEEDED(hr = data->EnumFormatEtc(DATADIR_GET, formats.initialize()))) {
			FORMATETC format;
			ULONG fetched;
			dout << L"DragEnter received a data object exposes the following formats.\n";
			for(formats->Reset(); formats->Next(1, &format, &fetched) == S_OK; ) {
				WCHAR name[256];
				if(::GetClipboardFormatNameW(format.cfFormat, name, ASCENSION_COUNTOF(name) - 1) != 0)
					dout << L"\t" << name << L"\n";
				else
					dout << L"\t" << L"(unknown format : " << format.cfFormat << L")\n";
				if(format.ptd != 0)
					::CoTaskMemFree(format.ptd);
			}
		}
	}
#endif // _DEBUG

	DragEnterInput input(makeMouseButtonInput(position, keyState), translateDropActions(*effect));
	dragEntered(input);
	return S_OK;
}

/// Implements @c IDropTarget#DragOver method.
STDMETHODIMP Widget::DragOver(DWORD keyState, POINTL position, DWORD* effect) {
	ASCENSION_WIN32_VERIFY_COM_POINTER(effect);

	DragMoveInput input(makeMouseButtonInput(position, keyState), translateDropActions(*effect));
	dragMoved(input);
	return S_OK;
}

/// Implements @c IDropTarget#DragLeave method.
STDMETHODIMP Widget::DragLeave() {
	try {
		dragLeft(DragLeaveInput());
	} catch(const bad_alloc&) {
		return E_OUTOFMEMORY;
	}
	return S_OK;
}

/// Implements @c IDropTarget#Drop method.
STDMETHODIMP Widget::Drop(IDataObject* data, DWORD keyState, POINTL position, DWORD* effect) {
	if(data == 0)
		return E_INVALIDARG;
	ASCENSION_WIN32_VERIFY_COM_POINTER(effect);

	DropInput input(makeMouseButtonInput(position, keyState), translateDropActions(*effect));
	dropped(input);
	return S_OK;
}

auto_ptr<Widget::InputGrabLocker> Widget::grabInput() {
	::SetCapture(identifier().get());
	return auto_ptr<InputGrabLocker>(new InputGrabLocker(*this));
}

bool Widget::hasFocus() const /*throw()*/ {
	return ::GetFocus() == identifier().get();
}

void Widget::hide() {
	if(!win32::boole(::SetWindowPos(identifier().get(), 0, 0, 0, 0, 0,
			SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOSIZE | SWP_NOZORDER)))
		throw PlatformDependentError<>();
}

void WidgetBase::initialize(const Handle<HWND>& parent,
			const Point<>& position /* = Point<>(CW_USEDEFAULT, CW_USEDEFAULT) */,
			const Dimension<>& size /* = Dimension<>(CW_USEDEFAULT, CW_USEDEFAULT) */,
			DWORD style /* = 0 */, DWORD extendedStyle /* = 0 */) {
	if(handle().get() != 0)
		throw IllegalStateException("this object already has a window handle.");
	AutoZeroSize<WNDCLASSEXW> klass;
	const basic_string<WCHAR> className(provideClassName());
	if(!boole(::GetClassInfoExW(klass.hInstance = ::GetModuleHandle(0), className.c_str(), &klass))) {
		ClassInformation ci;
		provideClassInformation(ci);
		klass.style = ci.style;
		klass.lpfnWndProc = &windowProcedure;
		klass.hIcon = ci.icon.get();
		klass.hCursor = ci.cursor.get();
		klass.hbrBackground = ci.background.get();
		klass.lpszClassName = className.c_str();
		klass.hIconSm = ci.smallIcon.get();
		if(::RegisterClassExW(&klass) == 0)
			throw PlatformDependentError<>();
	}

	HWND borrowed = ::CreateWindowExW(extendedStyle, className.c_str(), 0,
		style, position.x, position.y, size.cx, size.cy, parent.get(), 0, 0, this);
	if(borrowed == 0)
		throw PlatformDependentError<>();
	assert(borrowed == handle().get());
	const WidgetBase* const self = reinterpret_cast<WidgetBase*>(
#ifdef _WIN64
		::GetWindowLongPtrW(borrowed, GWLP_USERDATA));
#else
		static_cast<LONG_PTR>(::GetWindowLongW(borrowed, GWL_USERDATA)));
#endif // _WIN64
	assert(self == this);
}

bool Widget::isVisible() const /*throw()*/ {
	return win32::boole(::IsWindowVisible(identifier().get()));
}

bool Widget::isWindow() const /*throw()*/ {
	return win32::boole(::IsWindow(identifier().get()));
}

NativePoint Widget::mapFromGlobal(const NativePoint& position) const {
	POINT temp(position);
	if(!win32::boole(::ScreenToClient(identifier().get(), &temp)))
		throw PlatformDependentError<>();
	return NativePoint(temp);
}

NativePoint Widget::mapToGlobal(const NativePoint& position) const {
	POINT temp(position);
	if(!win32::boole(::ClientToScreen(identifier().get(), &temp)))
		throw PlatformDependentError<>();
	return NativePoint(temp);
}

void Widget::redrawScheduledRegion() {
	if(!win32::boole(::UpdateWindow(identifier().get())))
		throw PlatformDependentError<>();
}

void Widget::releaseInput() {
	if(!win32::boole(::ReleaseCapture()))
		throw PlatformDependentError<>();
}

void Widget::setBounds(const NativeRectangle& bounds) {
	if(!win32::boole(::SetWindowPos(identifier().get(), 0,
			geometry::left(bounds), geometry::top(bounds),
			geometry::dx(bounds), geometry::dy(bounds), SWP_NOACTIVATE | SWP_NOZORDER)))
		throw PlatformDependentError<>();
}

void Widget::scheduleRedraw(bool eraseBackground) {
	if(!win32::boole(::InvalidateRect(identifier().get(), 0, eraseBackground)))
		throw PlatformDependentError<>();
}

void Widget::scheduleRedraw(const NativeRectangle& rect, bool eraseBackground) {
	RECT temp(rect);
	if(!win32::boole(::InvalidateRect(identifier().get(), &temp, eraseBackground)))
		throw PlatformDependentError<>();
}

void WidgetBase::scrollInformation(int bar, SCROLLINFO& scrollInfo, UINT mask /* = SIF_ALL */) const {
	scrollInfo.cbSize = sizeof(SCROLLINFO);
	scrollInfo.fMask = mask;
	if(!boole(::GetScrollInfo(handle().get(), bar, &scrollInfo)))
		throw PlatformDependentError<>();
}

int WidgetBase::scrollPosition(int bar) const {
	return ::GetScrollPos(handle().get(), bar);
}

Range<int> WidgetBase::scrollRange(int bar) const {
	int minPos, maxPos;
	if(!boole(::GetScrollRange(handle().get(), bar, &minPos, &maxPos)))
		throw PlatformDependentError<>();
	return makeRange(minPos, maxPos);
}

int WidgetBase::scrollTrackPosition(int bar) const {
	SCROLLINFO si;
	scrollInformation(bar, si, SIF_TRACKPOS);
	return si.nTrackPos;
}

void WidgetBase::setScrollInformation(int bar, const SCROLLINFO& scrollInfo, bool redraw /* = true */) {
	if(!boole(::SetScrollInfo(handle().get(), bar, &scrollInfo, redraw)))
		throw PlatformDependentError<>();
}

int WidgetBase::setScrollPosition(int bar, int pos, bool redraw /* = true */) {
	return ::SetScrollPos(handle().get(), bar, pos, redraw);
}

void WidgetBase::setScrollRange(int bar, const Range<int>& range, bool redraw /* = true */) {
	::SetScrollRange(handle().get(), bar, range.beginning(), range.end(), redraw);
}

void Widget::show() {
	if(!win32::boole(::ShowWindow(identifier().get(), SW_SHOWNOACTIVATE)))
		throw PlatformDependentError<>();
}

LRESULT CALLBACK WidgetBase::windowProcedure(HWND window, UINT message, WPARAM wp, LPARAM lp) {
	WidgetBase* self;
	bool consumed = false;
	if(message == WM_NCCREATE) {
		self = reinterpret_cast<WidgetBase*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
		assert(self != 0);
#ifdef _WIN64
		::SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
#else
		::SetWindowLong(window, GWL_USERDATA, static_cast<long>(reinterpret_cast<LONG_PTR>(self)));
#endif // _WIN64
		self->handle_.reset(window, &::DestroyWindow);
	} else {
		self = reinterpret_cast<WidgetBase*>(
#ifdef _WIN64
			::GetWindowLongPtrW(window, GWLP_USERDATA));
#else
			static_cast<LONG_PTR>(::GetWindowLongW(window, GWL_USERDATA)));
#endif // _WIN64
		if(self == 0)
			return TRUE;
/*		const LRESULT r = self->preTranslateWindowMessage(message, wp, lp, consumed);
		if(consumed)
			return r;
		else*/ if(message == WM_PAINT) {
			Handle<HWND> temp(window);
			PaintContext context(temp);
			self->paint(context);
			return FALSE;
		}
	}

	LRESULT result = self->processMessage(message, wp, lp, consumed);
	if(!consumed)
		result = ::CallWindowProcW(::DefWindowProcW, window, message, wp, lp);
	return result;
}
