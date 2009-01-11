/**
 * @file alpha.hpp
 */

#include <manah/win32/windows.hpp>

// std.min と std.max の補完 (VC6 + NOMINMAX 用)
#if(_MSC_VER < 1300)
namespace std {
	template<typename T> inline const T& max(const T& a1, const T& a2) {return (a1 < a2) ? a2 : a1;}
	template<typename T, typename Pr> inline const T& max(const T& a1, const T& a2, Pr pred) {return pred(a1, a2) ? a2 : a1;}
	template<typename T> inline const T& min(const T& a1, const T& a2) {return (a2 < a1) ? a2 : a1;}
	template<typename T, typename Pr> inline const T& min(const T& a1, const T& a2, Pr pred) {return pred(a2, a1) ? a2 : a1;}
	typedef unsigned int size_t;
}
#endif /* _MSC_VER < 1300 */

#pragma warning(disable: 4503)	// 「装飾された名前の長さが限界を越えました」(1)
#pragma warning(disable: 4786)	// 「識別子は 'number' 文字に切り詰められました」(3)

#ifdef _DEBUG
#	define ALERT(msg)	::MessageBoxW(0, (msg), L"Debug alert", MB_ICONEXCLAMATION);
#else
#	define ALERT(msg)
#endif /* _DEBUG */

// for スコープ (VC6 用。VC7 では /Zc:forScope を使う)
#if(_MSC_VER < 1300)
#	define for if(0); else for 
#endif
