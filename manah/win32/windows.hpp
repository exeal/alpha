/**
 * @file windows.hpp
 * @date 2006-2009 exeal
 */

#ifndef MANAH_WINDOWS_HPP
#define MANAH_WINDOWS_HPP

#if defined(_DEBUG) && !defined(MANAH_NO_MEMORY_LEAK_CHECK)
#	define _CRTDBG_MAP_ALLOC
#	include <cstdlib>
#	include <malloc.h>
#	include <crtdbg.h>
#	define _DEBUG_NEW MANAH_DEBUG_NEW
#	define MANAH_DEBUG_NEW ::new(_NORMAL_BLOCK, MANAH_OVERRIDDEN_FILE, __LINE__)
#	define MANAH_OVERRIDDEN_FILE "uknown source file"
#endif // defined(_DEBUG) && !defined(MANAH_NO_MEMORY_LEAK_CHECK)
/*
	... and you should do the follow:
	#ifdef _DEBUG
	#undef MANAH_OVERRIDDEN_FILE
	static const char MANAH_OVERRIDDEN_FILE[] = __FILE__;
	#endif
 */

#include "../object.hpp"
#ifndef STRICT
#	define STRICT
#endif // !STRICT
#ifndef _WIN32_WINNT
#	define _WIN32_WINNT 0x0500	// Windows 2000
#endif // !_WIN32_WINNT
#ifndef WINVER
#	define WINVER 0x0500	// Windows 2000
#endif // !WINVER
#ifndef WIN32_LEAN_AND_MEAN
#	define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#	define NOMINMAX
#endif // !NOMINMAX
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
#include <sstream>

namespace manah {
	namespace win32 {

		/// The specified handle is invalid.
		class InvalidHandleException : public std::invalid_argument {
		public:
			explicit InvalidHandleException(const std::string& message) : std::invalid_argument(message) {}
		};

		/// The specified handle is @c null and not allowed.
		class NullHandleException : public InvalidHandleException {
		public:
			explicit NullHandleException(const std::string& message) : InvalidHandleException(message) {}
		};

		/**
		 * Holds and manages a handle value. The instance has the ownership of the handle value.
		 * The semantics of the copy operations is same as @c std#auto_ptr.
		 * @param T the type of the handle to be held
		 * @param Win32 function used to discard the handle
		 */
		template<typename T = HANDLE, BOOL(WINAPI* deleter)(T) = ::CloseHandle>
		class Handle {
		public:
			typedef T HandleType;	///< The type of the handle to be held.
		public:
			/// Constructor takes a handle as the initial value.
			explicit Handle(HandleType handle = 0) : handle_(handle) {}
			/// Destructor discards the handle.
			virtual ~Handle() {if(handle_ != 0) reset();}
			/// Copy-constructor takes the ownership of the handle away from @a rhs.
			Handle(Handle<HandleType, deleter>& rhs) : handle_(rhs.handle_) {rhs.handle_ = 0;}
			/// Assignment operator takes the ownership of the handle away from @a rhs.
			Handle<HandleType, deleter>& operator=(
				Handle<HandleType, deleter>& rhs) {reset(); std::swap(handle_, rhs.handle_); return *this;}
			/// Returns the raw handle value.
			HandleType get() const {return handle_;}
			/// Sets the internal handle value to @c null.
			HandleType release() {HandleType temp(0); std::swap(handle_, temp); return temp;}
			/// Discards the current handle and takes the new handle's ownership.
			void reset(HandleType newValue = 0) {aboutToReset(newValue); handle_ = newValue;}
			/// Returns the raw handle value. If the handle is @c null, throws @c std#logic_error.
			HandleType use() const {if(handle_ == 0) throw std::logic_error("handle is null.");
				else if(!check()) throw InvalidHandleException("handle is invalid."); return handle_;}
		protected:
			/// Returns false if the handle value is invalid. Called by @c #use method.
			virtual bool check() const {return true;}
		private:
			/// Called by @c #reset method before overwritten by @a newValue.
			virtual void aboutToReset(HandleType newValue) {if(newValue != handle_ && deleter != 0) (*deleter)(handle_);}
		private:
			HandleType handle_;
		};

		template<typename T> class Borrowed : private T {
		public:
			/// Default constructor.
			Borrowed() : T() {}
//			/// Constructor.
//			explicit Borrowed(const T& t) : T(t.get()) {}
			/// Constructor.
			explicit Borrowed(typename T::HandleType handle) : T(handle) {}
			/// Destructor.
			~Borrowed() throw() {T::release();}
			/// Copy-constructor just copies the handle value.
			Borrowed(const Borrowed<T>& rhs) : T() {reset(rhs.get());}
			/// Member-access operator returns @c T object.
			T* operator->() {return this;}
			/// Member-access operator returns @c const @c T object.
			const T* operator->() const {return this;}
		private:
			void aboutToReset(typename T::HandleType newValue) {}
		};

		/// A resource identifier can be initialized by using both a string and a numeric identifier.
		class ResourceID {
			MANAH_NONCOPYABLE_TAG(ResourceID);
		public:
			/// Constructor takes a string identifier.
			ResourceID(const WCHAR* name) /*throw()*/ : name_(name) {}
			/// Constructor takes a numeric identifier.
			ResourceID(UINT_PTR id) /*throw()*/ : name_(MAKEINTRESOURCEW(id)) {}
			/// Returns the string identifier.
			operator const WCHAR*() const /*throw()*/ {return name_;}
		private:
			const WCHAR* const name_;
		};

		/// Defines a structure type automatically fills oneself with zero.
		template<typename Structure> struct AutoZero : public Structure {
			/// Default constructor.
			AutoZero() /*throw()*/ {std::memset(this, 0, sizeof(Structure));}
		};

		/// Defines a structure type automatically fills oneself with zero and sets its size member.
		template<typename Structure, typename SizeType = int> struct AutoZeroSize : public AutoZero<Structure> {
			/// Default constructor.
			AutoZeroSize() /*throw()*/ {*reinterpret_cast<SizeType*>(this) = sizeof(Structure);}
		};

		class DumpContext {
		public:
			template<typename T>
			DumpContext& operator<<(const T& rhs) throw();
			void hexDump(const WCHAR* line, byte* pb, int bytes, int width = 0x10) throw();
		};

		inline void DumpContext::hexDump(const WCHAR* line, byte* pb, int bytes, int width /* = 0x10 */) throw() {
			WCHAR* const output = new WCHAR[static_cast<std::size_t>(
				(std::wcslen(line) + 3 * width + 2) * static_cast<float>(bytes / width))];
			std::wcscpy(output, line);

			WCHAR buffer[4];
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


// macros ///////////////////////////////////////////////////////////////////

// sizeof(MENUITEMINFO)
#if(WINVER >= 0x0500 && !defined(MENUITEMINFO_SIZE_VERSION_400))
#	define MENUITEMINFO_SIZE_VERSION_400A (offsetof(MENUITEMINFOA, cch) + sizeof(static_cast<MENUITEMINFOA*>(0)->cch))
#	define MENUITEMINFO_SIZE_VERSION_400W (offsetof(MENUITEMINFOW, cch) + sizeof(static_cast<MENUITEMINFOW*>(0)->cch))
#	ifdef UNICODE
#		define MENUITEMINFO_SIZE_VERSION_400 MENUITEMINFO_SIZE_VERSION_400W
#	else
#		define MENUITEMINFO_SIZE_VERSION_400 MENUITEMINFO_SIZE_VERSION_400A
#	endif // !UNICODE
#endif // WINVER >= 0x0500 && !defined(MENUITEMINFO_SIZE_VERSION_400)

#endif // !MANAH_WINDOWS_HPP
