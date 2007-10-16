// windows.hpp
// (c) 2006-2007 exeal

#ifndef MANAH_WINDOWS_HPP
#define MANAH_WINDOWS_HPP

#include "../object.hpp"
#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500	// Windows 2000
#endif /* !_WIN32_WINNT */
#ifndef WINVER
#define WINVER 0x0500	// Windows 2000
#endif /* !WINVER */
#define WIN32_LEAN_AND_MEAN

#define NOMINMAX
#include <cassert>
#include <cstring>	// prevent C header inclusion
#include <cwchar>	// prevent C header inclusion
#include <cstdlib>	// prevent C header inclusion
#undef min
#undef max

#define size_t std::size_t
//#include <winnt.h>
#include <windows.h>
#undef size_t
#include <stdexcept>

namespace manah {
	namespace win32 {

		// Win32 structure initializement
		template<typename T> struct AutoZero : public T {AutoZero() {std::memset(this, 0, sizeof(T));}};
		template<typename T> struct AutoZeroS : public AutoZero<T> {AutoZeroS() {*reinterpret_cast<int*>(this) = sizeof(T);}};

		struct ResourceID : private Noncopyable {
			ResourceID(const ::WCHAR* nameString) throw() : name(nameString) {}
			ResourceID(::UINT_PTR id) throw() : name(MAKEINTRESOURCEW(id)) {}
			const ::WCHAR* const name;
		};

		// base class for handle-wrapper classes
		template<typename HandleType = ::HANDLE, ::BOOL (WINAPI *deleter)(HandleType) = ::CloseHandle>
		class Handle {
		public:
			Handle(HandleType handle = 0) : handle_(handle), attached_(handle != 0) {}
			Handle(const Handle& rhs) : handle_(rhs.handle_), attached_(rhs.attached_) {const_cast<Handle&>(rhs).attached_ = true;}
			virtual ~Handle() {reset();}
			Handle& operator=(const Handle& rhs) {reset();
				handle_ = rhs.handle_; attached_ = rhs.attached_; const_cast<Handle&>(rhs).attached_ = true; return *this;}
			bool operator!() const {return handle_ == 0;}
			HandleType attach(HandleType handle) {if(handle == 0) throw std::invalid_argument("null handle.");
				HandleType old = release(); handle_ = handle; attached_ = true; return old;}
			HandleType detach() {if(!attached_) throw std::logic_error("not attched."); return release();}
			HandleType getHandle() const {return handle_;}
			bool isAttached() const throw() {return attached_;}
			HandleType release() {HandleType old = handle_; handle_ = 0; attached_ = false; return old;}
			void reset(HandleType newHandle = 0) {
				if(handle_ != 0 && newHandle != handle_ && !attached_ && deleter != 0)
					(*deleter)(handle_);
				handle_ = newHandle;
				attached_ = false;
			}
		private:
			HandleType handle_;
			bool attached_;
		};

		// convenient types from ATL
		struct MenuHandleOrControlID {
			MenuHandleOrControlID(::HMENU handle) throw() : menu(handle) {}
			MenuHandleOrControlID(::UINT_PTR id) throw() : menu(reinterpret_cast<::HMENU>(id)) {}
			::HMENU menu;
		};

		// others used by (CCustomControl derevied)::GetWindowClass
		struct BrushHandleOrColor {
			BrushHandleOrColor() throw() : brush(0) {}
			BrushHandleOrColor(::HBRUSH handle) throw() : brush(handle) {}
			BrushHandleOrColor(::COLORREF color) throw() : brush(reinterpret_cast<::HBRUSH>(static_cast<::HANDLE_PTR>(color + 1))) {}
			BrushHandleOrColor& operator=(::HBRUSH rhs) throw() {brush = rhs; return *this;}
			BrushHandleOrColor& operator=(::COLORREF rhs) throw() {brush = reinterpret_cast<::HBRUSH>(static_cast<::HANDLE_PTR>(rhs + 1)); return *this;}
			::HBRUSH brush;
		};
		struct CursorHandleOrID {
			CursorHandleOrID() throw() : cursor(0) {}
			CursorHandleOrID(::HCURSOR handle) throw() : cursor(handle) {}
			CursorHandleOrID(const ::WCHAR* systemCursorID) : cursor(::LoadCursorW(0, systemCursorID)) {}
			CursorHandleOrID& operator=(::HCURSOR rhs) {cursor = rhs; return *this;}
			CursorHandleOrID& operator=(const ::WCHAR* rhs) {cursor = ::LoadCursorW(0, rhs); return *this;}
			HCURSOR cursor;
		};

		class DumpContext {
		public:
			// constructor
			DumpContext(const ::WCHAR* fileName = 0);
			// operators
			template<typename T>
			DumpContext& operator<<(const T& rhs) throw();
			// methods
			void	flush() throw();
			void	hexDump(const ::WCHAR* line, byte* pb, int bytes, int width = 0x10) throw();

			// data member
		private:
			::WCHAR* fileName_;	// error log
		};

		inline DumpContext::DumpContext(const ::WCHAR* fileName /* = 0 */) {
			if(fileName != 0)
				throw std::exception("File log is not supported!");
		}

		inline void DumpContext::flush() throw() {/* not implemented */}

		inline void DumpContext::hexDump(const ::WCHAR* line, byte* pb, int bytes, int width /* = 0x10 */) throw() {
			::WCHAR* const output = new ::WCHAR[static_cast<std::size_t>(
				(std::wcslen(line) + 3 * width + 2) * static_cast<float>(bytes / width))];
			std::wcscpy(output, line);

			::WCHAR buffer[4];
			for(int i = 0; i < bytes; ++i){
				::wsprintfW(buffer, L" %d", pb);
				std::wcscat(output, buffer);
				if(i % width == 0){
					std::wcscat(output, L"\n");
					std::wcscat(output, line);
				}
			}
			::OutputDebugStringW(L"\n>----Dump is started");
			::OutputDebugStringW(output);
			::OutputDebugStringW(L"\n>----Dump is done");
			delete[] output;
		}

		template<typename T> inline DumpContext& DumpContext::operator<<(const T& rhs) throw() {
			std::wostringstream ss;
			ss << rhs;
			::OutputDebugStringW(ss.str().c_str());
			return *this;
		}
	}
}

// sizeof(MENUITEMINFO)
#if(WINVER >= 0x0500 && !defined(MENUITEMINFO_SIZE_VERSION_400))
#define MENUITEMINFO_SIZE_VERSION_400A (offsetof(MENUITEMINFOA, cch) + sizeof(static_cast<MENUITEMINFOA*>(0)->cch))
#define MENUITEMINFO_SIZE_VERSION_400W (offsetof(MENUITEMINFOW, cch) + sizeof(static_cast<MENUITEMINFOW*>(0)->cch))
#ifdef UNICODE
#define MENUITEMINFO_SIZE_VERSION_400 MENUITEMINFO_SIZE_VERSION_400W
#else
#define MENUITEMINFO_SIZE_VERSION_400 MENUITEMINFO_SIZE_VERSION_400A
#endif /* !UNICODE */
#endif /* WINVER >= 0x0500 && !defined(MENUITEMINFO_SIZE_VERSION_400) */

#endif /* !MANAH_WINDOWS_HPP */
