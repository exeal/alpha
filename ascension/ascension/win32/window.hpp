/**
 * @file window.hpp
 * @author exeal
 * @date 2011-03-26 separated from window-windows.hpp
 * @date 2012-03-07 renamed from widget-windows.hpp
 */

#ifndef ASCENSION_WIN32_WINDOW_HPP
#define ASCENSION_WIN32_WINDOW_HPP
#include <ascension/corelib/basic-exceptions.hpp>
#include <ascension/graphics/geometry.hpp>
#include <ascension/win32/handle.hpp>
#include <map>
#include <memory>	// std.shared_ptr

namespace ascension {
	namespace detail {
		template<typename Window>
		class MessageDispatcher {
			ASCENSION_NONCOPYABLE_TAG(MessageDispatcher);
		public:
			void addExplicitly(HWND handle, Window& object) {
				handleToObjects_.insert(std::make_pair(handle, &object));
			}
			LRESULT dispatch(HWND window, UINT message, WPARAM wp, LPARAM lp) {
				bool dummy;
				return dispatch(window, message, wp, lp, dummy);
			}
			template<typename DefaultProcedure>
			LRESULT dispatch(HWND window, UINT message, WPARAM wp, LPARAM lp, DefaultProcedure defaultProcedure) {
				bool consumed;
				const LRESULT result = dispatch(window, message, wp, lp, consumed);
				if(consumed)
					return result;
				std::map<HWND, Window*>::iterator i(handleToObjects_.find(window));
				assert(i != handleToObjects_.end());
				return ::CallWindowProcW(i->second->*defaultProcedure, window, message, wp, lp);
			}
			LRESULT dispatch(HWND window, UINT message, WPARAM wp, LPARAM lp, bool& consumed) {
				if(message == WM_NCCREATE) {
					void* const p = reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams;
					assert(p != nullptr);
					addExplicitly(window, *static_cast<Window*>(p));
				}
				const std::map<HWND, Window*>::iterator i(handleToObjects_.find(window));
				const LRESULT result = (i != handleToObjects_.end()) ?
					i->second->processMessage(message, wp, lp, consumed)
					: (::DefWindowProcW(window, message, wp, lp), consumed = true);
				if(message == WM_NCDESTROY)
					removeExplicitly(window);
				return result;
			}
			void removeExplicitly(HWND handle) {
				handleToObjects_.erase(handle);
			}
		private:
			std::map<HWND, Window*> handleToObjects_;
		};
	}

	namespace win32 {
		class Window {
			ASCENSION_NONCOPYABLE_TAG(Window);
		public:
			static const DWORD defaultStyle = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE;
			/// Constructor takes a borrowed window handle.
			explicit Window(const Handle<HWND>::Type& handle) BOOST_NOEXCEPT : handle_(handle.get()) {}
			/// Move-constructor.
			Window(Window&& other) BOOST_NOEXCEPT : handle_(std::move(other.handle_)) {}
			/// Move-assignment operator.
			Window& operator=(Window&& other) BOOST_NOEXCEPT {std::swap(*this, Window(other));}
			/// Returns the held window handle.
			Handle<HWND>::Type handle() const {return handle_;}
		protected:
			/// Constructor takes a window handle.
			explicit Window(HWND&& handle) : handle_(handle) {
				if(handle_.get() == nullptr)
					throw NullPointerException("handle");
			}
		private:
			Handle<HWND>::Type handle_;
		};

		class SubclassedWindow : public Window {
			ASCENSION_NONCOPYABLE_TAG(SubclassedWindow);
		public:
			/// Move-constructor.
			SubclassedWindow(SubclassedWindow&& other) BOOST_NOEXCEPT :
					Window(std::move(other)), originalWindowProcedure_(other.originalWindowProcedure_) {
				other.originalWindowProcedure_ = nullptr;
			}
			/// Move-assignment operator.
			SubclassedWindow& operator=(SubclassedWindow&& other) BOOST_NOEXCEPT {
				Window::operator=(std::move(other));
				originalWindowProcedure_ = other.originalWindowProcedure_;
				other.originalWindowProcedure_ = nullptr;
			}
		protected:
			SubclassedWindow(const Handle<HWND>::Type& parent, const WCHAR className[],
					const graphics::Point* position = nullptr, const graphics::Dimension* size = nullptr,
					DWORD style = 0, DWORD extendedStyle = 0) : Window(::CreateWindowExW(
						extendedStyle, className, nullptr, style,
						(position != nullptr) ? static_cast<int>(graphics::geometry::x(*position)) : CW_USEDEFAULT,
						(position != nullptr) ? static_cast<int>(graphics::geometry::y(*position)) : CW_USEDEFAULT,
						(size != nullptr) ? static_cast<int>(graphics::geometry::dx(*size)) : CW_USEDEFAULT,
						(size != nullptr) ? static_cast<int>(graphics::geometry::dy(*size)) : CW_USEDEFAULT,
						parent.get(), nullptr, ::GetModuleHandleW(nullptr), nullptr)) {
				originalWindowProcedure_ = reinterpret_cast<WNDPROC>(::GetWindowLongPtrW(handle().get(), GWLP_WNDPROC));
				::SetWindowLongPtrW(handle().get(), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
				::SetWindowLongPtrW(handle().get(), GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(windowProcedure));
			}
			virtual LRESULT processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed) {
				return 0;
			}
		private:
			static LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wp, LPARAM lp) {
				return messageDispatcher_.dispatch(window, message, wp, lp, &SubclassedWindow::originalWindowProcedure_);
			}
		private:
			static detail::MessageDispatcher<SubclassedWindow> messageDispatcher_;
			WNDPROC originalWindowProcedure_;
			friend class detail::MessageDispatcher<SubclassedWindow>;
		};

		class CustomControl : public Window {
		public:
			struct ClassInformation {
				UINT style;	// corresponds to WNDCLASSEXW.style
				/// Makes a brush handle parameter from either a brush handle or @c COLORREF value. 
				class Background {
					ASCENSION_NONCOPYABLE_TAG(Background);
				public:
					/// Constructor makes @c null @c HBRUSH value.
					Background() BOOST_NOEXCEPT : brush_(nullptr) {}
					/// Constructor takes a brush handle.
					Background(Handle<HBRUSH>::Type handle) BOOST_NOEXCEPT : brush_(handle) {}
					/// Constructor takes a @c COLORREF value used to make the brush handle.
					Background(int systemColor) BOOST_NOEXCEPT
						: brush_(reinterpret_cast<HBRUSH>(static_cast<HANDLE_PTR>(systemColor + 1))) {}
					/// Move-constructor.
					Background(Background&& other) BOOST_NOEXCEPT : brush_(std::move(other.brush_)) {}
					/// Move-assignment operator.
					Background& operator=(Background&& other) BOOST_NOEXCEPT {
						return (brush_ = std::move(other.brush_)), *this;
					}
					/// Returns the brush handle.
					Handle<HBRUSH>::Type get() const BOOST_NOEXCEPT {return brush_;}
				private:
					Handle<HBRUSH>::Type brush_;
				} background;
				Handle<HICON> icon, smallIcon;
				/// Makes a cursor handle parameter from either a cursor handle or numeric identifier.
				class CursorHandleOrID {
					ASCENSION_NONCOPYABLE_TAG(CursorHandleOrID);
				public:
					/// Constructor makes @c null @c HCURSOR value.
					CursorHandleOrID() BOOST_NOEXCEPT : cursor_(nullptr) {}
					/// Constructor takes a cursor handle.
					CursorHandleOrID(Handle<HCURSOR>::Type handle) BOOST_NOEXCEPT : cursor_(handle) {}
					/// Constructor takes a numeric identifier for system cursor.
					CursorHandleOrID(const WCHAR* systemCursorID) BOOST_NOEXCEPT : cursor_(::LoadCursorW(nullptr, systemCursorID)) {}
					/// Move-constructor.
					CursorHandleOrID(CursorHandleOrID&& other) BOOST_NOEXCEPT : cursor_(std::move(other.cursor_)) {}
					/// Move-assignment operator.
					CursorHandleOrID& operator=(CursorHandleOrID&& other) BOOST_NOEXCEPT {
						return (cursor_ = std::move(other.cursor_)), *this;
					}
					/// Returns the cursor handle.
					Handle<HCURSOR>::Type get() const BOOST_NOEXCEPT {return cursor_;}
				private:
					Handle<HCURSOR>::Type cursor_;
				} cursor;
				ClassInformation() : style(0) {}
			};
			CustomControl();
		protected:
			virtual LRESULT processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed) = 0;
			virtual void provideClassInformation(ClassInformation& classInfomation) const {}
			virtual std::basic_string<WCHAR> provideClassName() const = 0;
		private:
			static LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wp, LPARAM lp) {
				return messageDispatcher_.dispatch(window, message, wp, lp);
			}
		private:
			static detail::MessageDispatcher<CustomControl> messageDispatcher_;
			friend class detail::MessageDispatcher<CustomControl>;
		};
	}
}

#endif // !ASCENSION_WIN32_WINDOW_HPP
