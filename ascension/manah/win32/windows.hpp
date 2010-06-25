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

		template<typename Handle> struct Managed {};
		template<typename Handle> struct Borrowed {};
		template<typename Handle> inline Managed<Handle>* managed(Handle handle) {return reinterpret_cast<Managed<Handle>*>(handle);}
		template<typename Handle> inline Borrowed<Handle>* borrowed(Handle handle) {return reinterpret_cast<Borrowed<Handle>*>(handle);}

		/**
		 * Holds and manages a handle value. The instance has the ownership of the handle value.
		 * The semantics of the copy operations is same as @c std#auto_ptr.
		 * @tparam HandleType the type of the handle to be held
		 * @tparam deleter Win32 function used to discard the handle
		 * @tparam HeldType the type of the handle to be held actually
		 */
		template<typename HandleType = HANDLE, BOOL(WINAPI* deleter)(HandleType) = ::CloseHandle, typename HeldType = HandleType>
		class Object {
		public:
			/// The type of the handle to be held.
			typedef HeldType Handle;
			/// Alias of this type.
			typedef Object<HandleType, deleter, Handle> BaseObject;
		public:
			explicit Object(Managed<Handle>* handle);
			explicit Object(Borrowed<Handle>* handle = 0);
			virtual ~Object();
			Object(Object<HandleType, deleter, Handle>& other);
			Object<HandleType, deleter, Handle>& operator=(Object<HandleType, deleter, Handle>& other);
			Handle get() const;
			Handle release();
			void reset(Managed<Handle>* newValue);
			void reset(Borrowed<Handle>* newValue = 0);
			Handle use() const;
		protected:
			/// Returns @c false if @a handle is invalid. Called by @c #use method.
			virtual bool check(Handle handle) const {return true;}
		private:
			void resetHandle(Handle newHandle);
			Handle handle_;
			bool manages_;
		};

#define MANAH_WIN32_OBJECT_CONSTRUCTORS(ClassName)									\
	ClassName() : BaseObject() {}													\
	explicit ClassName(Managed<Handle>* handle) : BaseObject(handle) {}	\
	explicit ClassName(Borrowed<Handle>* handle) : BaseObject(handle) {}

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


		/// Constructor takes a handle as the initial value and manages it.
		template<typename HandleType, BOOL(WINAPI* deleter)(HandleType), typename HeldType>
		inline Object<HandleType, deleter, HeldType>::Object(Managed<Handle>* handle) : handle_(reinterpret_cast<Handle>(handle)), manages_(true) {
			if(handle_ != 0 && !check(handle_))
				throw InvalidHandleException("handle");
		}

		/// Constructor takes a handle as the initial value.
		template<typename HandleType, BOOL(WINAPI* deleter)(HandleType), typename HeldType>
		inline Object<HandleType, deleter, HeldType>::Object(Borrowed<Handle>* handle /* = 0 */) : handle_(reinterpret_cast<Handle>(handle)), manages_(false) {
			if(handle_ != 0 && !check(handle_))
				throw InvalidHandleException("handle");
		}

		/// Destructor discards the handle.
		template<typename HandleType, BOOL(WINAPI* deleter)(HandleType), typename HeldType>
		inline Object<HandleType, deleter, HeldType>::~Object() {
			reset();
		}

		/// Copy-constructor takes the ownership of the handle away from @a other.
		template<typename HandleType, BOOL(WINAPI* deleter)(HandleType), typename HeldType>
		inline Object<HandleType, deleter, HeldType>::Object(Object<HandleType, deleter, HeldType>& other) : handle_(other.handle_), manages_(other.manages_) {
			other.handle_ = 0;
		}

		/// Assignment operator takes the ownership of the handle away from @a other.
		template<typename HandleType, BOOL(WINAPI* deleter)(HandleType), typename HeldType>
		inline Object<HandleType, deleter, HeldType>& Object<HandleType, deleter, HeldType>::operator=(Object<HandleType, deleter, HeldType>& other) {
			reset();
			std::swap(handle_, other.handle_);
			manages_ = other.manages_;
			return *this;
		}

		/// Returns the raw handle value.
		template<typename HandleType, BOOL(WINAPI* deleter)(HandleType), typename HeldType>
		inline typename Object<HandleType, deleter, HeldType>::Handle Object<HandleType, deleter, HeldType>::get() const {
			return handle_;
		}

		/// Sets the internal handle value to @c null.
		template<typename HandleType, BOOL(WINAPI* deleter)(HandleType), typename HeldType>
		inline typename Object<HandleType, deleter, HeldType>::Handle Object<HandleType, deleter, HeldType>::release() {
			Handle temp(0);
			std::swap(handle_, temp);
			return temp;
		}

		/// Discards or release the current handle, holds the new handle value and manages it.
		template<typename HandleType, BOOL(WINAPI* deleter)(HandleType), typename HeldType>
		inline void Object<HandleType, deleter, HeldType>::reset(Managed<Handle>* newValue) {
			resetHandle(reinterpret_cast<Handle>(newValue));
			manages_ = true;
		}

		/// Discards or release the current handle and holds the new handle value.
		template<typename HandleType, BOOL(WINAPI* deleter)(HandleType), typename HeldType>
		inline void Object<HandleType, deleter, HeldType>::reset(Borrowed<Handle>* newValue /* = 0 */) {
			resetHandle(reinterpret_cast<Handle>(newValue));
			manages_ = false;
		}

		template<typename HandleType, BOOL(WINAPI* deleter)(HandleType), typename HeldType>
		inline void Object<HandleType, deleter, HeldType>::resetHandle(Handle newHandle) {
			if(newHandle != 0 && !check(newHandle))
				throw InvalidHandleException("newValue");
			if(handle_ == 0)
				handle_ = newHandle;
			else if(newHandle == 0) {
				if(deleter != 0 && manages_)
					(*deleter)(handle_);
				handle_ = 0;
			} else {
				if(newHandle != handle_ && deleter != 0 && manages_)
					(*deleter)(handle_);
				handle_ = newHandle;
			}
		}

		/// Returns the raw handle value. If the handle is @c null, throws @c std#logic_error.
		template<typename HandleType, BOOL(WINAPI* deleter)(HandleType), typename HeldType>
		inline typename Object<HandleType, deleter, HeldType>::Handle Object<HandleType, deleter, HeldType>::use() const {
			if(handle_ == 0)
				throw std::logic_error("handle is null.");
			else if(!check(handle_))
				throw InvalidHandleException("handle is invalid.");
			return handle_;
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
