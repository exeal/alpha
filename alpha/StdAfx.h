// stdafx.h : �W���̃V�X�e�� �C���N���[�h �t�@�C���A
//            �܂��͎Q�Ɖ񐔂������A�����܂�ύX����Ȃ�
//            �v���W�F�N�g��p�̃C���N���[�h �t�@�C�����L�q���܂��B
//

#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#if _MSC_VER > 1000
#	pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN		// Windows �w�b�_�[����w�ǎg�p����Ȃ��X�^�b�t�����O���܂�

// Win32 �p�V���{��
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

// std::min �� std::max �̕⊮ (VC6 + NOMINMAX �p)
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
#	define _DEBUG_NEW	new(_NORMAL_BLOCK, __FILE__, __LINE__)
#else
#	define ALERT(msg)
#	define _DEBUG_NEW
#endif /* _DEBUG */

// for �X�R�[�v (VC6 �p�BVC7 �ł� /Zc:forScope ���g��)
#if(_MSC_VER < 1300)
#	define for if(0); else for 
#endif


// TODO: �v���O�����ŕK�v�ȃw�b�_�[�Q�Ƃ�ǉ����Ă��������B

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ �͑O�s�̒��O�ɒǉ��̐錾��}�����܂��B

#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
