/**
 * @file widget-proxy.hpp
 * @author exeal
 * @date 2011-03-27
 * @date 2014-08-30 Separated from widget.hpp
 */

#ifndef ASCENSION_WIDGET_PROXY_HPP
#define ASCENSION_WIDGET_PROXY_HPP
#include <ascension/platforms.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <gtkmm/widget.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#	include <QWidget>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <ascension/win32/window.hpp>	// win32.Window
#endif

namespace ascension {
	namespace viewers {
		namespace widgetapi {
			template<typename Entity, template<class> class SmartPointer>
			struct WidgetOrWindowBase {
				typedef Entity value_type;
				typedef value_type& reference;
				typedef const value_type& const_reference;
				typedef value_type* pointer;
				typedef const value_type* const_pointer;
				typedef typename SmartPointer<value_type> SmartPointerType;
				typedef typename SmartPointer<const value_type> ConstSmartPointerType;
			};

			struct Widget : WidgetOrWindowBase<
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				Gtk::Widget, Glib::RefPtr
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				QWidget, std::shared_ptr
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
				NSView, ???
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				win32::Window, std::shared_ptr
#endif
			> {};

			struct Window : WidgetOrWindowBase<
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				Gdk::Window, Glib::RefPtr
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				QWidget, std::shared_ptr
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
				NSWindow, ???
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				win32::Window, std::shared_ptr
#endif
			> {};

			namespace detail {
				template<typename T> struct IsSmartPointer : std::false_type {};
				template<typename T> struct RemoveSmartPointer {typedef T Type;};
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				template<typename T> struct IsSmartPointer<typename Glib::RefPtr<T>> : std::true_type {};
				template<typename T> struct RemoveSmartPointer<Glib::RefPtr<T>> {typedef T Type;};
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				template<typename T> struct IsSmartPointer<typename std::shared_ptr<T>> : std::true_type {};
				template<typename T> struct RemoveSmartPointer<std::shared_ptr<T>> {typedef T Type;};
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
				template<typename T> struct IsSmartPointer<typename ???<T>> : std::true_type {};
				template<typename T> struct RemoveSmartPointer<???<T>> {typedef T Type;};
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				template<typename T> struct IsSmartPointer<typename std::shared_ptr<T>> : std::true_type {};
				template<typename T> struct RemoveSmartPointer<std::shared_ptr<T>> {typedef T Type;};
#endif
			}

			template<typename WidgetOrWindow>
			class ProxyBase {
			public:
				typedef typename std::conditional<std::is_const<WidgetOrWindow>::value,
					typename WidgetOrWindow::const_pointer, typename WidgetOrWindow::pointer>::type pointer;
				typedef typename std::conditional<std::is_const<WidgetOrWindow>::value,
					typename WidgetOrWindow::ConstSmartPointerType, typename WidgetOrWindow::SmartPointerType>::type SmartPointerType;
				explicit ProxyBase(pointer p = nullptr) : p_(p) {assert(!sp_);}
				explicit ProxyBase(SmartPointerType p) : p_(nullptr), sp_(p) {}
				pointer operator->() const {return get();}
				pointer get() const {
					if(sp_)
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
						return sp_.operator->();
#else
						return sp_.get();
#endif
					else
						return p_;
				}
				SmartPointerType sp() const {return sp_;}
#ifndef BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS
				explicit operator bool() const {return p_;}
#else
			private:
				typedef void(ProxyBase::*safeBool)() const;
			public:
				operator safeBool() const {return p_ ? &ProxyBase::uncallable : nullptr;}
#endif

			private:
#ifdef BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS
				void uncallable() const {}
#endif
				pointer p_;
				SmartPointerType sp_;
			};

			template<typename WidgetOrWindow>
			class Proxy : public ProxyBase<WidgetOrWindow> {
			public:
				/// Creates an instance from a native smart pointer.
				template<typename T>
				Proxy(T p, typename std::enable_if<
					detail::IsSmartPointer<T>::value
					&& !std::is_const<typename detail::RemoveSmartPointer<T>::Type>::value
					&& std::is_base_of<typename WidgetOrWindow::value_type, typename detail::RemoveSmartPointer<T>::Type>::value>::type* = nullptr) : ProxyBase(p) {}
				/// Creates an instance from a reference to a C++ object.
				template<typename T>
				Proxy(T& v, typename std::enable_if<
					!std::is_const<T>::value
					&& std::is_base_of<typename WidgetOrWindow::value_type, T>::value>::type* = nullptr) : ProxyBase(&v) {}
				template<typename T>
				Proxy(T v, typename std::enable_if<
					std::is_lvalue_reference<T>::value
					&& !std::is_const<typename std::remove_reference<T>::type>::value
					&& std::is_base_of<typename WidgetOrWindow::value_type, typename std::remove_reference<T>::type>::value>::type* = nullptr) : ProxyBase(&v) {}
//				typename std::add_lvalue_reference<typename WidgetOrWindow::value_type>::type operator*() const {return *get();}
				/// Converts into const version.
				operator Proxy<const WidgetOrWindow>() const {
					if(sp())
						return Proxy<const WidgetOrWindow>(sp());
					else if(get() != nullptr)
						return Proxy<const WidgetOrWindow>(*get());
					else
						return Proxy<const WidgetOrWindow>(SmartPointerType(nullptr));
				}
			};

			template<typename WidgetOrWindow>
			class Proxy<const WidgetOrWindow> : public ProxyBase<const WidgetOrWindow> {
			public:
				/// Creates an instance from a native smart pointer.
				template<typename T>
				Proxy(T p, typename std::enable_if<
					detail::IsSmartPointer<T>::value
//					&& std::is_const<typename detail::RemoveSmartPointer<T>::Type>::value
					&& std::is_base_of<typename WidgetOrWindow::value_type, typename detail::RemoveSmartPointer<T>::Type>::value>::type* = nullptr) : ProxyBase(p) {}
				/// Creates an instance from a reference to a C++ object.
				template<typename T>
				Proxy(T& v, typename std::enable_if<
/*					std::is_const<T>::value
					&&*/ std::is_base_of<typename WidgetOrWindow::value_type, T>::value>::type* = nullptr) : ProxyBase(&v) {}
				template<typename T>
				Proxy(T v, typename std::enable_if<
					std::is_lvalue_reference<T>::value
//					&& std::is_const<typename std::remove_reference<T>::type>::value
					&& std::is_base_of<typename WidgetOrWindow::value_type, typename std::remove_reference<T>::type>::value>::type* = nullptr) : ProxyBase(&v) {}
//				typename std::add_lvalue_reference<typename WidgetOrWindow::value_type>::type operator*() const {return *get();}
			};
		}
	}
}

#endif // !ASCENSION_WIDGET_PROXY_HPP
