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
				typedef typename SmartPointer<value_type> pointer;
				typedef typename SmartPointer<const value_type> const_pointer;
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
				template<typename T> struct IsPointer : std::false_type {};
				template<typename T> struct RemovePointer {typedef T Type;};
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				template<typename T> struct IsPointer<typename Glib::RefPtr<T>> : std::true_type {};
				template<typename T> struct RemovePointer<Glib::RefPtr<T>> {typedef T Type;};
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				template<typename T> struct IsPointer<typename std::shared_ptr<T>> : std::true_type {};
				template<typename T> struct RemovePointer<std::shared_ptr<T>> {typedef T Type;};
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
				template<typename T> struct IsPointer<typename ???<T>> : std::true_type {};
				template<typename T> struct RemovePointer<???<T>> {typedef T Type;};
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				template<typename T> struct IsPointer<typename std::shared_ptr<T>> : std::true_type {};
				template<typename T> struct RemovePointer<std::shared_ptr<T>> {typedef T Type;};
#endif
			}

			template<typename WidgetOrWindow>
			class Proxy {
			public:
				typedef typename WidgetOrWindow::pointer pointer;
				template<typename T>
				Proxy(T p, typename std::enable_if<
					detail::IsPointer<T>::value
					&& !std::is_const<typename detail::RemovePointer<T>::Type>::value
					&& std::is_base_of<typename WidgetOrWindow::value_type, typename detail::RemovePointer<T>::Type>::value>::type* = nullptr) : p_(p) {}
				template<typename T>
				Proxy(T& v, typename std::enable_if<
					!std::is_const<T>::value
					&& std::is_base_of<typename WidgetOrWindow::value_type, T>::value>::type* = nullptr) : p_(&v) {}
				template<typename T>
				Proxy(T v, typename std::enable_if<
					std::is_lvalue_reference<T>::value
					&& !std::is_const<typename std::remove_reference<T>::type>::value
					&& std::is_base_of<typename WidgetOrWindow::value_type, typename std::remove_reference<T>::type>::value>::type* = nullptr) : p_(&v) {}
//				typename std::add_lvalue_reference<typename WidgetOrWindow::value_type>::type operator*() const {return *get();}
				pointer operator->() const {return p_;}
				operator Proxy<const WidgetOrWindow>() const {return Proxy<const WidgetOrWindow>(get());}
#ifndef BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS
				explicit operator bool() const {return p_;}
#else
			private:
				typedef void(Proxy::*safeBool)() const;
			public:
				operator safeBool() const {return p_ ? &Proxy::uncallable : nullptr;}
#endif
				pointer get() const {return p_;}

			private:
#ifdef BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS
				void uncallable() const {}
#endif
				pointer p_;
			};

			template<typename WidgetOrWindow>
			class Proxy<const WidgetOrWindow> {
			public:
				typedef typename WidgetOrWindow::const_pointer pointer;
				template<typename T>
				Proxy(T p, typename std::enable_if<
					detail::IsPointer<T>::value
//					&& std::is_const<typename detail::RemovePointer<T>::Type>::value
					&& std::is_base_of<typename WidgetOrWindow::value_type, typename detail::RemovePointer<T>::Type>::value>::type* = nullptr) : p_(p) {}
				template<typename T>
				Proxy(T& v, typename std::enable_if<
/*					std::is_const<T>::value
					&&*/ std::is_base_of<typename WidgetOrWindow::value_type, T>::value>::type* = nullptr) : p_(&v) {}
				template<typename T>
				Proxy(T v, typename std::enable_if<
					std::is_lvalue_reference<T>::value
//					&& std::is_const<typename std::remove_reference<T>::type>::value
					&& std::is_base_of<typename WidgetOrWindow::value_type, typename std::remove_reference<T>::type>::value>::type* = nullptr) : p_(&v) {}
//				typename std::add_lvalue_reference<typename WidgetOrWindow::value_type>::type operator*() const {return *get();}
				pointer operator->() const {return p_;}
				operator Proxy<const WidgetOrWindow>() const {return Proxy<const WidgetOrWindow>(get());}
#ifndef BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS
				explicit operator bool() const {return p_;}
#else
			private:
				typedef void(Proxy::*safeBool)() const;
			public:
				operator safeBool() const {return p_ ? &Proxy::uncallable : nullptr;}
#endif
				pointer get() const {return p_;}

			private:
#ifdef BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS
				void uncallable() const {}
#endif
				pointer p_;
			};
		}
	}
}

#endif // !ASCENSION_WIDGET_PROXY_HPP
