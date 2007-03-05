// windows.hpp
// (c) 2006-2007 exeal

#ifndef MANAH_WINDOWS_HPP
#define MANAH_WINDOWS_HPP

#include "../object.hpp"
#define WIN32_LEAN_AND_MEAN
#define size_t std::size_t
//#include <winnt.h>
#include <windows.h>
#include <tchar.h>
#undef size_t
#include <stdexcept>

namespace manah {
	namespace win32 {

		// Win32 structure initializement
		template<class Base> struct AutoZero : public Base {AutoZero() {std::memset(this, 0, sizeof(Base));}};
		template<class Base, typename sizeMemberType, sizeMemberType(Base::*sizeMember)>
		struct AutoZeroS : public AutoZero<Base> {AutoZeroS() {this->*sizeMember = sizeof(Base);}};
#if(_MSC_VER < 1300)
		template<class Base> struct AutoZeroCB : public AutoZeroS<Base, UINT, typename Base::cbSize> {};
		template<class Base> struct AutoZeroLS : public AutoZeroS<Base, DWORD, typename Base::lStructSize> {};
#else
		template<class Base> struct AutoZeroCB : public AutoZeroS<Base, UINT, &Base::cbSize> {};
		template<class Base> struct AutoZeroLS : public AutoZeroS<Base, DWORD, &Base::lStructSize> {};
#endif

		struct ResourceID {
			ResourceID(const TCHAR* nameString) throw() : name(nameString) {}
			ResourceID(::UINT_PTR id) throw() : name(MAKEINTRESOURCE(id)) {}
			const TCHAR* const name;
		};

		// base class for handle-wrapper classes
		template<typename HandleType = HANDLE, BOOL (WINAPI *deleter)(HandleType) = ::CloseHandle>
		class Handle : public Unassignable {
		public:
			Handle(HandleType handle = 0) : handle_(handle), attached_(false) {}
			virtual ~Handle() {release();}
			bool operator!() const throw() {return handle_ == 0;}
			HandleType attach(HandleType handle) {HandleType old = release(); handle_ = handle; attached_ = true; return old;}
			HandleType detach() {if(!attached_) throw std::logic_error("not attched."); return release();}
			HandleType get() const {return handle_;}
			bool isAttached() const throw() {return attached_;}
			HandleType release() {HandleType old = handle_; handle_ = 0; attached_ = false; return old;}
			void reset(HandleType newHandle = 0) {
				if(handle_ != 0 && newHandle != handle_ && !attached_ && deleter != 0)
					(*deleter)(handle_);
				handle_ = newHandle;
				attached_ = false;
			}
		protected:
			Handle(const Handle<HandleType, deleter>& rhs) : handle_(0), attached_(false) {if(rhs.handle_ != 0) attach(rhs.handle_);}
		private:
			HandleType handle_;
			bool attached_;
		};

		// convenient types from ATL
		struct MenuHandleOrControlID {
			MenuHandleOrControlID(HMENU handle) throw() : menu(handle) {}
			MenuHandleOrControlID(UINT_PTR id) throw() : menu(reinterpret_cast<HMENU>(id)) {}
			HMENU menu;
		};

		// others used by (CCustomControl derevied)::GetWindowClass
		struct BrushHandleOrColor {
			BrushHandleOrColor() throw() : brush(0) {}
			BrushHandleOrColor(HBRUSH handle) throw() : brush(handle) {}
			BrushHandleOrColor(COLORREF color) throw() : brush(reinterpret_cast<HBRUSH>(static_cast<HANDLE_PTR>(color + 1))) {}
			BrushHandleOrColor& operator=(HBRUSH rhs) throw() {brush = rhs; return *this;}
			BrushHandleOrColor& operator=(COLORREF rhs) throw() {brush = reinterpret_cast<HBRUSH>(static_cast<HANDLE_PTR>(rhs + 1)); return *this;}
			HBRUSH brush;
		};
		struct CursorHandleOrID {
			CursorHandleOrID() throw() : cursor(0) {}
			CursorHandleOrID(HCURSOR handle) throw() : cursor(handle) {}
			CursorHandleOrID(const TCHAR* systemCursorID) : cursor(::LoadCursor(0, systemCursorID)) {}
			CursorHandleOrID& operator =(HCURSOR rhs) {cursor = rhs; return *this;}
			CursorHandleOrID& operator =(const TCHAR* rhs) {cursor = ::LoadCursor(0, rhs); return *this;}
			HCURSOR cursor;
		};

		class DumpContext {
		public:
			// constructor
			DumpContext(const TCHAR* fileName = 0);
			// operators
			template<class T>
			DumpContext& operator <<(const T& rhs) throw();
			// methods
			void	flush() throw();
			void	hexDump(const TCHAR* line, uchar* pb, int bytes, int width = 0x10) throw();

			// data member
		private:
			TCHAR* fileName_;	// error log
		};

		inline DumpContext::DumpContext(const TCHAR* fileName /* = 0 */) {
			if(fileName != 0)
				throw std::exception("File log is not supported!");
		}

		inline void DumpContext::flush() throw() {/* not implemented */}

		inline void DumpContext::hexDump(const TCHAR* line, uchar* pb, int bytes, int width /* = 0x10 */) throw() {
			TCHAR* const output = new TCHAR[static_cast<std::size_t>(
				(std::_tcslen(line) + 3 * width + 2) * static_cast<float>(bytes / width))];
			std::_tcscpy(output, line);

			TCHAR byte[4];
			for(int i = 0; i < bytes; ++i){
				::wsprintf(byte, _T(" %d"), pb);
				std::_tcscat(output, byte);
				if(i % width == 0){
					std::_tcscat(output, _T("\n"));
					std::_tcscat(output, line);
				}
			}
			::OutputDebugString(_T("\n>----Dump is started"));
			::OutputDebugString(output);
			::OutputDebugString(_T("\n>----Dump is done"));
			delete[] output;
		}

		template <class T> inline DumpContext& DumpContext::operator <<(const T& rhs) throw() {
			std::basic_ostringstream<TCHAR>	ss;
			ss << rhs;
			::OutputDebugString(ss.str().c_str());
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
