/**
 * @file windows.hpp
 * @date 2006-2012 exeal
 */

#ifndef ASCENSION_WIN32_WINDOWS_HPP
#define ASCENSION_WIN32_WINDOWS_HPP

#if defined(_DEBUG) && !defined(ASCENSION_NO_MEMORY_LEAK_CHECK)
#	define _CRTDBG_MAP_ALLOC
#	include <cstdlib>
#	include <malloc.h>
#	include <crtdbg.h>
#	define _DEBUG_NEW ASCENSION_DEBUG_NEW
#	define ASCENSION_DEBUG_NEW ::new(_NORMAL_BLOCK, ASCENSION_OVERRIDDEN_FILE, __LINE__)
#	define ASCENSION_OVERRIDDEN_FILE "uknown source file"
#endif // defined(_DEBUG) && !defined(ASCENSION_NO_MEMORY_LEAK_CHECK)
/*
	... and you should do the follow:
	#ifdef _DEBUG
	#undef ASCENSION_OVERRIDDEN_FILE
	static const char ASCENSION_OVERRIDDEN_FILE[] = __FILE__;
	#endif
 */

//#include <ascension/common.hpp>
//#include <ascension/basic-types.hpp>
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
#include <utility>								// std.swap
#include <ascension/corelib/basic-types.hpp>	// ASCENSION_NON_COPYABLE_TAG

namespace ascension {
	namespace win32 {

		/**
		 * Converts Win32 @c BOOL value to C++ standard @c bool one.
		 * @param v The source value
		 * @retval true @a is not @c FALSE (may be @c TRUE)
		 * @retval false @a v is @c FALSE
		 */
		inline bool boole(BOOL v) /*throw()*/ {return v != FALSE;}

		/**
		 * Returns the default UI language.
		 * Wrapper for Win32 @c GetUserDefaultUILanguage API.
		 */
		LANGID ASCENSION_FASTCALL userDefaultUILanguage() /*throw()*/;

#	define ASCENSION_WIN32_OBJECT_CONSTRUCTORS(ClassName)						\
		ClassName() : BaseObject() {}											\
		explicit ClassName(Managed<HandleType>* handle) : BaseObject(handle) {}	\
		explicit ClassName(Borrowed<HandleType>* handle) : BaseObject(handle) {}

		/// A resource identifier can be initialized by using both a string and a numeric identifier.
		class ResourceID {
			ASCENSION_NONCOPYABLE_TAG(ResourceID);
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
			void hexDump(const WCHAR* line, BYTE* pb, int bytes, int width = 0x10) throw();
		};

		inline void DumpContext::hexDump(const WCHAR* line, BYTE* pb, int bytes, int width /* = 0x10 */) throw() {
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

#endif // !ASCENSION_WIN32_WINDOWS_HPP
