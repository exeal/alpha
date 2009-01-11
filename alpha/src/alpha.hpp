/**
 * @file alpha.hpp
 */

#include <manah/win32/windows.hpp>

// std.min �� std.max �̕⊮ (VC6 + NOMINMAX �p)
#if(_MSC_VER < 1300)
namespace std {
	template<typename T> inline const T& max(const T& a1, const T& a2) {return (a1 < a2) ? a2 : a1;}
	template<typename T, typename Pr> inline const T& max(const T& a1, const T& a2, Pr pred) {return pred(a1, a2) ? a2 : a1;}
	template<typename T> inline const T& min(const T& a1, const T& a2) {return (a2 < a1) ? a2 : a1;}
	template<typename T, typename Pr> inline const T& min(const T& a1, const T& a2, Pr pred) {return pred(a2, a1) ? a2 : a1;}
	typedef unsigned int size_t;
}
#endif /* _MSC_VER < 1300 */

#pragma warning(disable: 4503)	// �u�������ꂽ���O�̒��������E���z���܂����v(1)
#pragma warning(disable: 4786)	// �u���ʎq�� 'number' �����ɐ؂�l�߂��܂����v(3)

#ifdef _DEBUG
#	define ALERT(msg)	::MessageBoxW(0, (msg), L"Debug alert", MB_ICONEXCLAMATION);
#else
#	define ALERT(msg)
#endif /* _DEBUG */

// for �X�R�[�v (VC6 �p�BVC7 �ł� /Zc:forScope ���g��)
#if(_MSC_VER < 1300)
#	define for if(0); else for 
#endif
