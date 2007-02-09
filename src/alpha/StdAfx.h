// stdafx.h : 標準のシステム インクルード ファイル、
//            または参照回数が多く、かつあまり変更されない
//            プロジェクト専用のインクルード ファイルを記述します。
//

#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#if _MSC_VER > 1000
#	pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN		// Windows ヘッダーから殆ど使用されないスタッフを除外します

// Win32 用シンボル
#define STRICT
#define UNICODE
#define _UNICODE
#define WINVER			0x0501	// Windows XP
//#define _WIN32_WINDOWS  0x0401
#define _WIN32_WINNT	0x0501	// Windows XP
#define _WIN32_IE		0x0600	// Internet Explorer 6
#define NOMINMAX

#ifdef _DEBUG
#	define _CRTDBG_MAP_ALLOC
#	include <cstdlib>
#	include <malloc.h>
#	include <crtdbg.h>
#endif /* _DEBUG */

// std::min と std::max の補完 (VC6 + NOMINMAX 用)
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
#	define _DEBUG_NEW	new(_NORMAL_BLOCK, __FILE__, __LINE__)
#else
#	define ALERT(msg)
#	define _DEBUG_NEW
#endif /* _DEBUG */

// for スコープ (VC6 用。VC7 では /Zc:forScope を使う)
#if(_MSC_VER < 1300)
#	define for if(0); else for 
#endif


// TODO: プログラムで必要なヘッダー参照を追加してください。

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
