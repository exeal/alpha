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


WidgetBase::WidgetBase(LRESULT (*messageDispatcher)(WidgetBase&, UINT, WPARAM, LPARAM, bool&)) : messageDispatcher_(messageDispatcher) {
	if(messageDispatcher == 0)
		throw NullPointerException("windowProcedure");
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

	LRESULT result = (self->messageDispatcher_)(*self, message, wp, lp, consumed);
	if(!consumed)
		result = ::CallWindowProcW(::DefWindowProcW, window, message, wp, lp);
	return result;
}
