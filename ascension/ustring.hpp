/**
 * @file ustring.hpp
 */

#ifdef ASCENSION_USE_INTRINSIC_WCHAR_T

#include <cwchar>

namespace ascension {

	inline const Char* ustrchr(const Char* s, Char c) {return std::wcschr(s, c);}

	inline std::size_t ustrlen(const Char* s) {return std::wcslen(s);}

}

#else

#include <algorithm>

namespace ascension {
}

#endif
