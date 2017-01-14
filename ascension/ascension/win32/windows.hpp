/**
 * @file windows.hpp
 * @author exeal
 * @date 2006-2014
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
#	define ASCENSION_OVERRIDDEN_FILE "unknown source file"
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
#undef STRICT
#undef size_t
#include <utility>									// std.swap
#include <ascension/corelib/basic-exceptions.hpp>	// makePlatformError
#include <ascension/corelib/string-piece.hpp>		// BasicStringPiece
#include <boost/noncopyable.hpp>

namespace ascension {
	namespace win32 {
		/**
		 * Converts between 16-bit character strings, such as `WCHAR*`.
		 * @tparam To The destination character type
		 * @tparam From The source character type
		 * @param p The pointer to the source character string
		 * @return The converted pointer
		 */
		template<typename To, typename From>
		inline BOOST_CONSTEXPR To* wideString(From* p) BOOST_NOEXCEPT {
			static_assert(sizeof(From) == 2, "The source is not 16-bit character string.");
			static_assert(sizeof(To) == 2, "The destination is not 16-bit character string.");
			return reinterpret_cast<To*>(p);
		}

		/**
		 * Converts Win32 @c BOOL value to C++ standard @c bool one.
		 * @param v The source value
		 * @retval true @a is not @c FALSE (may be @c TRUE)
		 * @retval false @a v is @c FALSE
		 */
		inline bool boole(BOOL v) BOOST_NOEXCEPT {return v != FALSE;}

		/**
		 * Returns the default UI language.
		 * Wrapper for Win32 @c GetUserDefaultUILanguage API.
		 */
		LANGID userDefaultUILanguage();

		inline LONG_PTR getWindowLong(HWND window, int index) {
			const DWORD lastError = ::GetLastError();
			::SetLastError(ERROR_SUCCESS);
			const LONG_PTR result = ::GetWindowLongPtrW(window, index);
			if(result == 0 && ::GetLastError() != ERROR_SUCCESS)
				throw makePlatformError();
			::SetLastError(lastError);
			return result;
		}

		inline void setWindowLong(HWND window, int index, LONG_PTR value) {
			const DWORD lastError = ::GetLastError();
			::SetLastError(0);
			if(::SetWindowLongPtrW(window, index, value) == 0 && ::GetLastError() != ERROR_SUCCESS)
				throw makePlatformError();
			::SetLastError(lastError);
		}

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
		/// Specialization of @c BasicStringPiece for @c WCHAR type.
		typedef BasicStringPiece<WCHAR> StringPiece;
#else
		/// Specialization of @c boost#basic_string_ref for @c WCHAR type.
		typedef boost::basic_string_ref<WCHAR, std::char_traits<WCHAR>> StringPiece;
#endif

#	define ASCENSION_WIN32_OBJECT_CONSTRUCTORS(ClassName)						\
		ClassName() : BaseObject() {}											\
		explicit ClassName(Managed<HandleType>* handle) : BaseObject(handle) {}	\
		explicit ClassName(Borrowed<HandleType>* handle) : BaseObject(handle) {}

		/// A resource identifier can be initialized by using both a string and a numeric identifier.
		class ResourceID : private boost::noncopyable {
		public:
			/// Constructor takes a string identifier.
			ResourceID(const win32::StringPiece& name) BOOST_NOEXCEPT : name_(name.data()) {}
			/// Constructor takes a numeric identifier.
			ResourceID(UINT_PTR id) BOOST_NOEXCEPT : name_(MAKEINTRESOURCEW(id)) {}
			/// Returns the string identifier.
			operator const WCHAR*() const BOOST_NOEXCEPT {return name_;}
		private:
			const WCHAR* const name_;
		};

		/**
		 * Creates an object and fills with zero.
		 * @tparam T The object type
		 * @return The created object
		 */
		template<typename T> inline T makeZero() {
			T object;
			std::memset(&object, 0, sizeof(T));
			return object;
		}

		/**
		 * Creates an object, fills with zero and sets its size member.
		 * @tparam T The object type
		 * @tparam SizeType The type of the size member
		 * @return The created object
		 */
		template<typename T, typename SizeType = int> inline T makeZeroSize() {
			T object;
			std::memset(&object, 0, sizeof(T));
			*reinterpret_cast<SizeType*>(&object) = sizeof(T);
			return object;
		}

	}
}


// macros ///////////////////////////////////////////////////////////////////

#if(_MSC_VER < 1300 && 0)
// for MSVC6 + NOMINMAX
namespace std {
	template<typename T> inline const T& max(const T& a1, const T& a2) {return (a1 < a2) ? a2 : a1;}
	template<typename T, typename Pr> inline const T& max(const T& a1, const T& a2, Pr pred) {return pred(a1, a2) ? a2 : a1;}
	template<typename T> inline const T& min(const T& a1, const T& a2) {return (a2 < a1) ? a2 : a1;}
	template<typename T, typename Pr> inline const T& min(const T& a1, const T& a2, Pr pred) {return pred(a2, a1) ? a2 : a1;}
	typedef unsigned int size_t;
}

// for MSVC6 which can't use "/Zc:forScope"
#	define for if(0); else for 
#endif

// sizeof(MENUITEMINFO)
#if(WINVER >= 0x0500 && !defined(MENUITEMINFO_SIZE_VERSION_400))
#	define MENUITEMINFO_SIZE_VERSION_400A (offsetof(MENUITEMINFOA, cch) + sizeof(static_cast<MENUITEMINFOA*>(nullptr)->cch))
#	define MENUITEMINFO_SIZE_VERSION_400W (offsetof(MENUITEMINFOW, cch) + sizeof(static_cast<MENUITEMINFOW*>(nullptr)->cch))
#	ifdef UNICODE
#		define MENUITEMINFO_SIZE_VERSION_400 MENUITEMINFO_SIZE_VERSION_400W
#	else
#		define MENUITEMINFO_SIZE_VERSION_400 MENUITEMINFO_SIZE_VERSION_400A
#	endif // !UNICODE
#endif // WINVER >= 0x0500 && !defined(MENUITEMINFO_SIZE_VERSION_400)

#endif // !ASCENSION_WIN32_WINDOWS_HPP
