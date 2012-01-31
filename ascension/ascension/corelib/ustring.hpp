/**
 * @file ustring.hpp
 */

#ifndef ASCENSION_USTRING_HPP
#define ASCENSION_USTRING_HPP

namespace ascension {
	const Char* umemchr(const Char* s, Char c, std::size_t length);
	int umemcmp(const Char* s1, const Char* s2, std::size_t n);
	const Char* ustrchr(const Char* s, Char c);
	std::size_t ustrlen(const Char* s);
}

#ifdef ASCENSION_USE_INTRINSIC_WCHAR_T

#include <cwchar>

namespace ascension {

	inline const Char* umemchr(const Char* s, Char c, std::size_t length) {return std::wmemchr(s, c, length);}

	inline int umemcmp(const Char* s1, const Char* s2, std::size_t n) {return std::wmemcmp(s1, s2, n);}

	inline const Char* ustrchr(const Char* s, Char c) {return std::wcschr(s, c);}

	inline std::size_t ustrlen(const Char* s) {return std::wcslen(s);}

}

#else

#include <algorithm>

namespace ascension {
	
	inline const Char* umemchr(const Char* s, Char c, std::size_t length) {
		const Char* const p = std::find(s, s + length, c);
		return (p != s + length) ? p : nullptr;
	}
	
	inline int umemcmp(const Char* s1, const Char* s2, std::size_t n) {
		const std::pair<const Char*, const Char*> i(std::mismatch(s1, s1 + n, s2));
		if(i.first == s1 + n)
			return 0;
		return (*i.first < *i.second) ? -1 : +1;
	}

	inline const Char* ustrchr(const Char* s, Char c) {
		for(; *s != 0; ++s) {
			if(*s == c)
				return s;
		}
		return nullptr;
	}

	inline std::size_t ustrlen(const Char* s) {
		std::size_t length = 0;
		while(*s != 0)
			++s, ++length;
		return length;
	}

}

#endif

#endif // ASCENSION_USTRING_HPP
