/**
 * @file widget-windows.cpp
 * @author exeal
 * @date 2011-03-27 created
 */

#include <ascension/viewer/base/widget-windows.hpp>

using namespace ascension;
using namespace ascension::graphics;
using namespace ascension::win32;
using namespace std;


graphics::Rect<> WidgetBase::bounds(bool includeFrame) const {
	RECT temp;
	if(includeFrame) {
		if(!boole(::GetWindowRect(handle().get(), &temp)))
			throw PlatformDependentError<>();
	} else {
		if(!boole(::GetClientRect(handle().get(), &temp)))
			throw PlatformDependentError<>();
	}
	return graphics::fromNative(temp);
}

bool WidgetBase::hasFocus() const /*throw()*/ {
	return ::GetFocus() == handle().get();
}

void WidgetBase::hide() {
	if(!boole(::SetWindowPos(handle().get(), 0, 0, 0, 0, 0,
			SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOSIZE | SWP_NOZORDER)))
		throw PlatformDependentError<>();
}

Point<> WidgetBase::clientToScreen(const Point<>& p) const {
	POINT temp(toNative(p));
	if(!boole(::ClientToScreen(handle().get(), &temp)))
		throw PlatformDependentError<>();
	return graphics::fromNative(temp);
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

bool WidgetBase::isVisible() const /*throw()*/ {
	return boole(::IsWindowVisible(handle().get()));
}

bool WidgetBase::isWindow() const /*throw()*/ {
	return boole(::IsWindow(handle().get()));
}

void WidgetBase::redrawScheduledRegion() {
	if(!boole(::UpdateWindow(handle().get())))
		throw PlatformDependentError<>();
}

void WidgetBase::setBounds(const graphics::Rect<>& bounds) {
	if(!boole(::SetWindowPos(handle().get(), 0,
			bounds.origin().x, bounds.origin().y,
			bounds.size().cx, bounds.size().cy, SWP_NOACTIVATE | SWP_NOZORDER)))
		throw PlatformDependentError<>();
}

void WidgetBase::scheduleRedraw(bool eraseBackground) {
	if(!boole(::InvalidateRect(handle().get(), 0, eraseBackground)))
		throw PlatformDependentError<>();
}

void WidgetBase::scheduleRedraw(const graphics::Rect<>& rect, bool eraseBackground) {
	RECT temp = toNative(rect);
	if(!boole(::InvalidateRect(handle().get(), &temp, eraseBackground)))
		throw PlatformDependentError<>();
}

Point<> WidgetBase::screenToClient(const Point<>& p) const {
	POINT temp(toNative(p));
	if(!boole(::ScreenToClient(handle().get(), &temp)))
		throw PlatformDependentError<>();
	return graphics::fromNative(temp);
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

void WidgetBase::show() {
	if(!boole(::ShowWindow(handle().get(), SW_SHOWNOACTIVATE)))
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
