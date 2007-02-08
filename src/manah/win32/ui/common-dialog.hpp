// common-dialog.hpp
// (c) 2004-2007 exeal

#ifndef MANAH_COMMON_DIALOG_HPP
#define MANAH_COMMON_DIALOG_HPP
#include "../object.h"
#include <commdlg.h>
#include <shlobj.h>

namespace manah {
namespace windows {
namespace ui {

/* this header contains following classes
CCommonDialog
	CColorDialog
	CFileDialog
	CFindTextDialog
		CReplaceTextDialog
	CFolderDialog
	CFontDialog
	CPageSetupDialog
	CPrintDialog
	CPrintDialogEx
*/

class CCommonDialog : public CSelfAssertable, public CNoncopyable {
	// constructors
public:
	CCommonDialog() : m_hDlg(0) {}
	virtual ~CCommonDialog() {}

	// data member
protected:
	HWND	m_hDlg;
};


class CColorDialog : public CCommonDialog {
	// constructors
public:
	explicit CColorDialog(COLORREF clrInit = RGB(0, 0, 0), DWORD dwFlags = 0, HWND hwndParent = 0);
	virtual ~CColorDialog();

	// methods
public:
	bool				DoModal();
	COLORREF			GetColor() const;
	static COLORREF*	GetCustomColors();
	void				SetCurrentColor(COLORREF clr);
	void				SetTemplate(UINT id);
	void				SetTemplate(const TCHAR* lpszId);
private:
	static UINT				_GetCOLOROKSTRING();
	static UINT				_GetSETRGBSTRING();
	static UINT CALLBACK	_HookProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

	// overridable message handlers
protected:
	virtual bool	OnInitDialog();	// WM_INITDIALOG
	virtual bool	OnColorOk();	// COLOROKSTRING

	// data members
private:
	CHOOSECOLOR	m_cc;
};


class CFileDialog : public CCommonDialog {
	// constructors
public:
	explicit CFileDialog(const TCHAR* lpszDefaultExtensions = 0, const TCHAR* lpszFileName = 0,
				DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
				const TCHAR* lpszFilter = 0, HWND hwndParent = 0);
	virtual ~CFileDialog();

	// methods
public:
	bool			DoModal(bool bForOpen);
	void			GetFileExtension(TCHAR* lpszExtension) const;
	const TCHAR*	GetFileName() const;
	const TCHAR*	GetFileTitle() const;
	std::size_t		GetFolderPath(TCHAR* lpszFolderPath, std::size_t cch) const;
	void			HideControl(int id);
	bool			IsReadOnlyButtonChecked() const;
	void			SetControlText(int id, const TCHAR* lpszText);
	void			SetDefaultExtension(const TCHAR* lpszExtension);
	void			SetTemplate(UINT id);
	void			SetTemplate(const TCHAR* lpszId);
private:
	static UINT CALLBACK	_HookProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

	// overridable message handlers
protected:
	virtual bool	OnFileOk(const OFNOTIFY& ofn);
	virtual bool	OnFolderChange(const OFNOTIFY& ofn);
	virtual bool	OnHelp(const OFNOTIFY& ofn);
	virtual bool	OnInitDialog();
	virtual bool	OnInitDone(const OFNOTIFY& ofn);
	virtual bool	OnSelChange(const OFNOTIFY& ofn);
	virtual bool	OnShareViolation(const OFNOTIFY& ofn);
	virtual bool	OnTypeChange(const OFNOTIFY& ofn);

	// data members
private:
	OPENFILENAME	m_ofn;
	TCHAR			m_szFileName[MAX_PATH];
	TCHAR			m_szFileTitle[MAX_PATH];
};


class CFindTextDialog : public CCommonDialog {
	// constructors
public:
	explicit CFindTextDialog(HWND hwndParent,
		const TCHAR* lpszFindWhat = 0, std::size_t cchFindWhatMax = 128, DWORD dwFlags = FR_DOWN);
	virtual ~CFindTextDialog();

	// methods
public:
	bool					DoesMatchCase() const;
	bool					DoesMatchWholeWord() const;
	bool					DoesSearchDown() const;
	bool					DoModeless();
	static CFindTextDialog*	FromLPARAM(LPARAM lParam);
	static UINT				GetFINDMSGSTRING();
	const TCHAR*			GetFindString() const;
	static UINT				GetHELPMSGSTRING();
	bool					IsCommandFindNext() const;
	bool					IsDialogTerminating() const;
private:
	static UINT CALLBACK	_HookProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

	// overridable message handler
	virtual bool	OnInitDialog();

	// data members
private:
	FINDREPLACE			m_fr;
	TCHAR*				m_pszFindWhat;
	const std::size_t	m_cchFindWhat;
	friend class CReplaceTextDialog;
};


class CReplaceTextDialog : public CFindTextDialog {
	// constructors
public:
	explicit CReplaceTextDialog(HWND hwndParent,
		const TCHAR* lpszFindWhat = 0, const TCHAR* lpszReplaceWith = 0,
		std::size_t cchFindWhatMax = 128, std::size_t cchReplaceWithMax = 128, DWORD dwFlags = FR_DOWN);
	virtual ~CReplaceTextDialog();

	// methods
public:
	bool			DoModeless();
	const TCHAR*	GetReplaceString() const;
	bool			IsCommandReplaceAll() const;
	bool			IsCommandReplaceCurrent() const;

	// data members
private:
	TCHAR*				m_pszReplaceWith;
	const std::size_t	m_cchReplaceWith;
};


// CColorDialog class implementation
/////////////////////////////////////////////////////////////////////////////

inline CColorDialog::CColorDialog(COLORREF clrInit /* = RGB(0, 0, 0) */, DWORD dwFlags /* = 0 */, HWND hwndParent /* = 0 */) {
	assert(hwndParent == 0 || toBoolean(::IsWindow(hwndParent)));
	ZeroMemory(&m_cc, sizeof(CHOOSECOLOR));
	m_cc.lStructSize = sizeof(CHOOSECOLOR);
	m_cc.Flags = dwFlags | CC_ENABLEHOOK;
	m_cc.hInstance = reinterpret_cast<HWND>(::GetModuleHandle(0));
	m_cc.hwndOwner = hwndParent;
	m_cc.lCustData = reinterpret_cast<LPARAM>(this);
	m_cc.lpCustColors = CColorDialog::GetCustomColors();
	m_cc.lpfnHook = CColorDialog::_HookProc;
	if(clrInit != RGB(0, 0, 0)) {
		m_cc.Flags |= CC_RGBINIT;
		m_cc.rgbResult = clrInit;
	}
}

inline CColorDialog::~CColorDialog() {
}

inline bool CColorDialog::DoModal() {
	AssertValid();
	assert(m_hDlg == 0);
	if(m_cc.hwndOwner == 0)
		m_cc.hwndOwner = ::GetActiveWindow();
	const bool	b = toBoolean(::ChooseColor(&m_cc));
	m_hDlg = 0;
	return b;
}

inline COLORREF CColorDialog::GetColor() const {
	AssertValid();
	return m_cc.rgbResult;
}

inline UINT CColorDialog::_GetCOLOROKSTRING() {
	static UINT	CCN_COLOROK = 0;
	if(CCN_COLOROK == 0)
		CCN_COLOROK = ::RegisterWindowMessage(COLOROKSTRING);
	return CCN_COLOROK;
}

inline COLORREF* CColorDialog::GetCustomColors() {
	static COLORREF	clrs[16] = {
		RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF),
		RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF),
		RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF),
		RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF)
	};
	return clrs;
}

inline UINT CColorDialog::_GetSETRGBSTRING() {
	static UINT	CCN_SETRGB = 0;
	if(CCN_SETRGB == 0)
		CCN_SETRGB = ::RegisterWindowMessage(SETRGBSTRING);
	return CCN_SETRGB;
}

inline UINT CALLBACK CColorDialog::_HookProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	if(msg == WM_INITDIALOG) {
		CColorDialog*	pDlg = reinterpret_cast<CColorDialog*>(reinterpret_cast<CHOOSECOLOR*>(lParam)->lCustData);
		pDlg->m_hDlg = hDlg;
		return pDlg->OnInitDialog();
	} else if(msg == _GetCOLOROKSTRING())
		return reinterpret_cast<CColorDialog*>(reinterpret_cast<CHOOSECOLOR*>(lParam)->lCustData)->OnColorOk();
	else
		return false;
}

inline void CColorDialog::SetCurrentColor(COLORREF clr) {
	AssertValid();
	assert(toBoolean(::IsWindow(m_hDlg)));
	::SendMessage(m_hDlg, CColorDialog::_GetSETRGBSTRING(), 0, clr);
}

inline void CColorDialog::SetTemplate(UINT id) {
	SetTemplate(MAKEINTRESOURCE(id));
}

inline void CColorDialog::SetTemplate(const TCHAR* lpszId) {
	AssertValid();
	assert(lpszId != 0);
	assert(toBoolean(m_cc.Flags & CC_ENABLETEMPLATE));
	m_cc.lpTemplateName = const_cast<TCHAR*>(lpszId);
}

inline bool CColorDialog::OnInitDialog() {
	return false;
}

inline bool CColorDialog::OnColorOk() {
	return false;
}


// CFileDialog class implementation
/////////////////////////////////////////////////////////////////////////////

inline CFileDialog::CFileDialog(const TCHAR* lpszDefaultExtensions /* = 0 */,
		const TCHAR* lpszFileName /* = 0 */, DWORD dwFlags /* = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT */,
		const TCHAR* lpszFilter /* = 0 */, HWND hwndParent /* = 0 */) {
	STD_::_tcscpy(m_szFileName, (lpszFileName != 0) ? lpszFileName : _T(""));
	m_szFileTitle[0] = 0;
	ZeroMemory(&m_ofn, sizeof(OPENFILENAME));
	m_ofn.lStructSize = sizeof(OPENFILENAME);
	m_ofn.Flags = dwFlags | OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLESIZING;
	m_ofn.hInstance = ::GetModuleHandle(0);
	m_ofn.hwndOwner = hwndParent;
	m_ofn.lCustData = reinterpret_cast<LPARAM>(this);
	m_ofn.lpfnHook = CFileDialog::_HookProc;
	m_ofn.lpstrDefExt = lpszDefaultExtensions;
	m_ofn.lpstrFile = m_szFileName;
	m_ofn.lpstrFileTitle = m_szFileTitle;
	m_ofn.lpstrFilter = lpszFilter;
	m_ofn.nMaxFile = MAX_PATH;
	m_ofn.nMaxFileTitle = MAX_PATH;
}

inline CFileDialog::~CFileDialog() {
}

inline bool CFileDialog::DoModal(bool bForOpen) {
	AssertValid();
	assert(m_hDlg == 0);
	if(m_ofn.hwndOwner == 0)
		m_ofn.hwndOwner = ::GetActiveWindow();
	const bool	b = toBoolean(bForOpen ? ::GetOpenFileName(&m_ofn) : ::GetSaveFileName(&m_ofn));
	m_hDlg = 0;
	return b;
}

inline const TCHAR* CFileDialog::GetFileName() const {
	AssertValid();
	return m_szFileName;
}

inline const TCHAR* CFileDialog::GetFileTitle() const {
	AssertValid();
	return m_szFileTitle;
}

inline std::size_t CFileDialog::GetFolderPath(TCHAR* lpszFolderPath, std::size_t cch) const {
	AssertValid();
	assert(toBoolean(::IsWindow(m_hDlg)));
	assert(toBoolean(m_ofn.Flags & OFN_EXPLORER));
	return ::SendMessage(m_hDlg, CDM_GETFOLDERPATH, cch, reinterpret_cast<LPARAM>(lpszFolderPath));
}

inline void CFileDialog::HideControl(int id) {
	AssertValid();
	assert(toBoolean(::IsWindow(m_hDlg)));
	assert(toBoolean(m_ofn.Flags & OFN_EXPLORER));
	::SendMessage(m_hDlg, CDM_HIDECONTROL, id, 0L);
}

inline UINT CALLBACK CFileDialog::_HookProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	if(msg == WM_INITDIALOG) {
		CFileDialog*	pDlg = reinterpret_cast<CFileDialog*>(reinterpret_cast<OPENFILENAME*>(lParam)->lCustData);
		pDlg->m_hDlg = hDlg;
		return pDlg->OnInitDialog();
	} else if(msg == WM_NOTIFY) {
		OFNOTIFY*		lpOFN = reinterpret_cast<OFNOTIFY*>(lParam);
		CFileDialog*	pDlg = reinterpret_cast<CFileDialog*>(lpOFN->lpOFN->lCustData);
		switch(lpOFN->hdr.code) {
		case CDN_FILEOK:			return pDlg->OnFileOk(*lpOFN);
		case CDN_FOLDERCHANGE:		return pDlg->OnFolderChange(*lpOFN);
		case CDN_HELP:				return pDlg->OnHelp(*lpOFN);
		case CDN_INITDONE:			return pDlg->OnInitDone(*lpOFN);
		case CDN_SELCHANGE:			return pDlg->OnSelChange(*lpOFN);
		case CDN_SHAREVIOLATION:	return pDlg->OnShareViolation(*lpOFN);
		case CDN_TYPECHANGE:		return pDlg->OnTypeChange(*lpOFN);
		default:					return false;
		}
	} else
		return false;
}

inline bool CFileDialog::IsReadOnlyButtonChecked() const {
	AssertValid();
	assert(toBoolean(::IsWindow(m_hDlg)));
	return toBoolean(m_ofn.Flags & OFN_READONLY);
}

inline bool CFileDialog::OnFileOk(const OFNOTIFY& ofn) {
	return false;
}

inline bool CFileDialog::OnFolderChange(const OFNOTIFY& ofn) {
	return false;
}

inline bool CFileDialog::OnHelp(const OFNOTIFY& ofn) {
	return false;
}

inline bool CFileDialog::OnInitDialog() {
	return false;
}

inline bool CFileDialog::OnInitDone(const OFNOTIFY& ofn) {
	return false;
}

inline bool CFileDialog::OnSelChange(const OFNOTIFY& ofn) {
	return false;
}

inline bool CFileDialog::OnShareViolation(const OFNOTIFY& ofn) {
	return false;
}

inline bool CFileDialog::OnTypeChange(const OFNOTIFY& ofn) {
	return false;
}

inline void CFileDialog::SetControlText(int id, const TCHAR* lpszText) {
	AssertValid();
	assert(toBoolean(::IsWindow(m_hDlg)));
	assert(toBoolean(m_ofn.Flags & OFN_EXPLORER));
	::SendMessage(m_hDlg, CDM_SETCONTROLTEXT, id, reinterpret_cast<LPARAM>(lpszText));
}

inline void CFileDialog::SetDefaultExtension(const TCHAR* lpszExtension) {
	AssertValid();
	assert(toBoolean(::IsWindow(m_hDlg)));
	assert(toBoolean(m_ofn.Flags & OFN_EXPLORER));
	::SendMessage(m_hDlg, CDM_SETDEFEXT, 0, reinterpret_cast<LPARAM>(lpszExtension));
}

inline void CFileDialog::SetTemplate(UINT id) {
	SetTemplate(MAKEINTRESOURCE(id));
}

inline void CFileDialog::SetTemplate(const TCHAR* lpszId) {
	AssertValid();
	assert(lpszId != 0);
	assert(toBoolean(m_ofn.Flags & OFN_ENABLETEMPLATE));
	m_ofn.lpTemplateName = const_cast<TCHAR*>(lpszId);
}


// CFindTextDialog class implementation
/////////////////////////////////////////////////////////////////////////////

inline CFindTextDialog::CFindTextDialog(HWND hwndParent,
		const TCHAR* lpszFindWhat /* = 0 */, std::size_t cchFindWhatMax /* = 128 */, DWORD dwFlags /* = FR_DOWN */)
		: m_pszFindWhat(new TCHAR[cchFindWhatMax + 1]), m_cchFindWhat(std::max<std::size_t>(cchFindWhatMax, 80)) {
	assert(lpszFindWhat == 0 || STD_::_tcslen(lpszFindWhat) <= cchFindWhatMax);
	assert(toBoolean(::IsWindow(hwndParent)));

	STD_::_tcscpy(m_pszFindWhat, (lpszFindWhat != 0) ? lpszFindWhat : _T(""));
	ZeroMemory(&m_fr, sizeof(FINDREPLACE));
	m_fr.lStructSize = sizeof(FINDREPLACE);
	m_fr.Flags = dwFlags | FR_ENABLEHOOK;
	m_fr.hInstance = ::GetModuleHandle(0);
	m_fr.hwndOwner = hwndParent;
	m_fr.lCustData = reinterpret_cast<LPARAM>(this);
	m_fr.lpfnHook = CFindTextDialog::_HookProc;
	m_fr.lpstrFindWhat = m_pszFindWhat;
	m_fr.wFindWhatLen = m_cchFindWhat;
}

inline CFindTextDialog::~CFindTextDialog() {
	delete[] m_pszFindWhat;
}

inline bool CFindTextDialog::DoesMatchCase() const {
	AssertValid();
	return toBoolean(m_fr.Flags & FR_MATCHCASE);
}

inline bool CFindTextDialog::DoesMatchWholeWord() const {
	AssertValid();
	return toBoolean(m_fr.Flags & FR_WHOLEWORD);
}

inline bool CFindTextDialog::DoesSearchDown() const {
	AssertValid();
	return toBoolean(m_fr.Flags & FR_DOWN);
}

inline bool CFindTextDialog::DoModeless() {
	AssertValid();
	return ::FindText(&m_fr) != 0;
}

inline CFindTextDialog* CFindTextDialog::FromLPARAM(LPARAM lParam) {
	reinterpret_cast<CFindTextDialog*>(reinterpret_cast<FINDREPLACE*>(lParam)->lCustData);
}

inline UINT CFindTextDialog::GetFINDMSGSTRING() {
	static UINT	FRN_FINDMSG = 0;
	if(FRN_FINDMSG == 0)
		FRN_FINDMSG = ::RegisterWindowMessage(FINDMSGSTRING);
	return FRN_FINDMSG;
}

inline const TCHAR* CFindTextDialog::GetFindString() const {
	AssertValid();
	return m_pszFindWhat;
}

inline UINT CFindTextDialog::GetHELPMSGSTRING() {
	static UINT	FRN_HELPMSG = 0;
	if(FRN_HELPMSG == 0)
		FRN_HELPMSG = ::RegisterWindowMessage(HELPMSGSTRING);
	return FRN_HELPMSG;
}

inline UINT CALLBACK CFindTextDialog::_HookProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	if(msg == WM_INITDIALOG) {
		CFindTextDialog*	pDlg = reinterpret_cast<CFindTextDialog*>(reinterpret_cast<FINDREPLACE*>(lParam)->lCustData);
		pDlg->m_hDlg = hDlg;
		return pDlg->OnInitDialog();
	}
	return false;
}

inline bool CFindTextDialog::IsCommandFindNext() const {
	AssertValid();
	return toBoolean(m_fr.Flags & FR_FINDNEXT);
}

inline bool CFindTextDialog::IsDialogTerminating() const {
	AssertValid();
	return toBoolean(m_fr.Flags & FR_DIALOGTERM);
}

inline bool CFindTextDialog::OnInitDialog() {
	return false;
}


// CReplaceTextDialog class implementation
/////////////////////////////////////////////////////////////////////////////

inline CReplaceTextDialog::CReplaceTextDialog(HWND hwndParent,
		const TCHAR* lpszFindWhat /* = 0 */, const TCHAR* lpszReplaceWith /* = 0 */,
		std::size_t cchFindWhatMax /* = 128 */, std::size_t cchReplaceWithMax /* = 128 */, DWORD dwFlags /* = FR_DOWN */)
		: CFindTextDialog(hwndParent, lpszFindWhat, cchFindWhatMax, dwFlags),
		m_pszReplaceWith(new TCHAR[cchReplaceWithMax + 1]), m_cchReplaceWith(std::max<std::size_t>(cchReplaceWithMax, 80)) {
	assert(lpszReplaceWith == 0 || STD_::_tcslen(lpszReplaceWith) <= cchReplaceWithMax);
	STD_::_tcscpy(m_pszReplaceWith, (lpszReplaceWith != 0) ? lpszReplaceWith : _T(""));
}

inline CReplaceTextDialog::~CReplaceTextDialog() {
	delete[] m_pszReplaceWith;
}

inline bool CReplaceTextDialog::DoModeless() {
	AssertValid();
	return ::ReplaceText(&m_fr) != 0;
}

inline const TCHAR* CReplaceTextDialog::GetReplaceString() const {
	AssertValid();
	return m_pszReplaceWith;
}

inline bool CReplaceTextDialog::IsCommandReplaceAll() const {
	AssertValid();
	assert(m_hDlg != 0);
	return toBoolean(m_fr.Flags & FR_REPLACEALL);
}

inline bool CReplaceTextDialog::IsCommandReplaceCurrent() const {
	AssertValid();
	assert(m_hDlg != 0);
	return toBoolean(m_fr.Flags & FR_REPLACE);
}

} // namespace Controls
} // namespace Windows
} // namespace Manah

#endif /* COMMON_DIALOG_H_ */

/* [EOF] */