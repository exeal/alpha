/**
 * @file custom-control.hpp
 * @author exeal
 * @date 2011-03-26 Separated from window-windows.hpp.
 * @date 2012-03-07 Renamed from widget-windows.hpp.
 * @date 2016-12-24 Separated from window.hpp.
 */

#ifndef ASCENSION_CUSTOM_CONTROL_HPP
#define ASCENSION_CUSTOM_CONTROL_HPP
#include <ascension/win32/window/window.hpp>
#include <ascension/win32/window/detail/message-dispatcher.hpp>

namespace ascension {
	namespace win32 {
		/// A window class. See @c CustomControl#classInformation.
		struct WindowClass {
			/// The class styles. Same as @c WNDCLASSEXW#style.
			UINT styles;
			/// The class name. Sane as @c WNDCLASSEXW#lpszClassName.
			std::basic_string<WCHAR> name;
			/// Makes a brush handle parameter from either a brush handle or @c COLORREF value. 
			class Background {
			public:
				/// Creates a @c Background instance with @c null @c HBRUSH value.
				Background() BOOST_NOEXCEPT : brush_(nullptr) {}
				/// Creates a @c Background instance with a brush handle.
				Background(Handle<HBRUSH> handle) BOOST_NOEXCEPT : brush_(handle) {}
				/// Creates a @c Background instance with a @c COLORREF value used to make the brush handle.
				Background(int systemColor) BOOST_NOEXCEPT
					: brush_(borrowed(reinterpret_cast<HBRUSH>(static_cast<HANDLE_PTR>(systemColor + 1)))) {}
				/// Move-constructor.
				Background(Background&& other) BOOST_NOEXCEPT : brush_(std::move(other.brush_)) {}
				/// Move-assignment operator.
				Background& operator=(Background&& other) BOOST_NOEXCEPT {
					return (brush_ = std::move(other.brush_)), *this;
				}
				/// Returns the brush handle.
				Handle<HBRUSH> get() const BOOST_NOEXCEPT {return brush_;}
			private:
				Handle<HBRUSH> brush_;
			} background;	///< The background.
			Handle<HICON> icon, smallIcon;
			/// Makes a cursor handle parameter from either a cursor handle or numeric identifier.
			class Cursor {
			public:
				/// Creates a @ Cursor instance with @c null @c HCURSOR value.
				Cursor() BOOST_NOEXCEPT : cursor_(nullptr) {}
				/// Creates a @c Cursor instance wirh a cursor handle.
				Cursor(Handle<HCURSOR> handle) BOOST_NOEXCEPT : cursor_(handle) {}
				/// Creates a @c Cursor instance with a numeric identifier for system cursor.
				Cursor(const std::basic_string<WCHAR>& systemCursorID) BOOST_NOEXCEPT :
					cursor_(static_cast<HCURSOR>(::LoadImageW(nullptr, systemCursorID.c_str(), IMAGE_CURSOR, 0, 0, LR_DEFAULTCOLOR | LR_SHARED)), &::DestroyCursor) {}
				/// Move-constructor.
				Cursor(Cursor&& other) BOOST_NOEXCEPT : cursor_(std::move(other.cursor_)) {}
				/// Move-assignment operator.
				Cursor& operator=(Cursor&& other) BOOST_NOEXCEPT {
					return (cursor_ = std::move(other.cursor_)), *this;
				}
				/// Returns the cursor handle.
				Handle<HCURSOR> get() const BOOST_NOEXCEPT {return cursor_;}
			private:
				Handle<HCURSOR> cursor_;
			} cursor;	///< The cursor.
			WindowClass() : styles(0) {}
		};

		template<typename Derived> class CustomControl;

		template<typename Derived>
		void realize(CustomControl<Derived>& widget, const Window::Type& type);

		/**
		 * A @c CustomControl has unique window class and window message procedure.
		 * @tparam Derived The derived class
		 * @see SubclassedWindow
		 */
		template<typename Derived>
		class CustomControl : public Window {
		protected:
			/**
			 * Creates a @c CustomControl instance.
			 * @note This constructor does not create a window. You should call @c realize(widget) after the object
			 *       construction.
			 * @post handle().get() == nullptr
			 */
			CustomControl() : Window(Handle<HWND>()) {}
			/**
			 * The window procedure.
			 * @param message The window message
			 * @param wp The first parameter
			 * @param lp The second parameter
			 * @param[out] consumed
			 * @note The default implementation calls @c DefWindowProcW.
			 */
			virtual LRESULT processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed) {
				return (consumed = true), ::DefWindowProcW(handle().get(), message, wp, lp);
			}
			/**
			 * This custom control was realized.
			 * @param type The window type
			 * @note The default implementation does nothing.
			 * @see #realize
			 */
			virtual void realized(const Type& type) {}
			/**
			 * Returns the window class data.
			 * @param[out] out The window class data
			 */
			virtual void windowClass(WindowClass& out) const BOOST_NOEXCEPT = 0;

		private:
			void realize(const Type& type) {
				if(handle().get() != nullptr)
					throw IllegalStateException("The CustomControl is already realized.");
				static std::basic_string<WCHAR> windowClassName;
				if(windowClassName.empty()) {
					WindowClass klassData;
					windowClass(klassData);
					assert(!klassData.name.empty());

					WNDCLASSEXW classData;
					classData.cbSize = sizeof(WNDCLASSEXW);
					classData.style = klassData.styles;
					classData.lpfnWndProc = windowProcedure;
					classData.cbClsExtra = classData.cbWndExtra = 0;
					classData.hInstance = ::GetModuleHandleW(nullptr);
					classData.hIcon = klassData.icon.get();
					classData.hCursor = klassData.cursor.get().get();
					classData.hbrBackground = klassData.background.get().get();
					classData.lpszMenuName = nullptr;
					classData.lpszClassName = klassData.name.c_str();
					classData.hIconSm = klassData.smallIcon.get();
					if(::RegisterClassExW(&classData) == 0)
						throw makePlatformError();
					windowClassName = klassData.name;
				}

				::CreateWindowExW(
					0, windowClassName.c_str(), nullptr, type.styles(),
					CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
					type.parent().get(), nullptr, ::GetModuleHandleW(nullptr), this);
				if(handle().get() == nullptr)
					throw makePlatformError();
				realized(type);
			}
			static LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wp, LPARAM lp) {
				return messageDispatcher_.dispatch(window, message, wp, lp);
			}
		private:
			static detail::MessageDispatcher<CustomControl<Derived>> messageDispatcher_;
			friend class detail::MessageDispatcher<CustomControl<Derived>>;
			template<typename D> friend void realize(CustomControl<D>& widget, const Type& type);
		};

		template<typename Derived>
		detail::MessageDispatcher<CustomControl<Derived>> CustomControl<Derived>::messageDispatcher_;

		/**
		 * Realizes the custom control.
		 * @tparam Derived The template parameter of @c CustomControl
		 * @param widget The custom control to realize
		 * @param type The window type
		 */
		template<typename Derived>
		inline void realize(CustomControl<Derived>& widget, const Window::Type& type) {
			return widget.realize(type);
		}
	}
}

#endif // !ASCENSION_CUSTOM_CONTROL_HPP
