// PropertySheet.h
// (c) 2004-2006 exeal

#ifndef PROPERTY_SHEET_H_
#define PROPERTY_SHEET_H_

#include "Dialog.hpp"
#include <prsht.h>
#include <vector>
#include <map>


namespace Manah {
namespace Windows {
namespace Controls {

class PropertySheet;

class PropertyPage : public Dialog {
public:
	// コンストラクタ
	PropertyPage();
	PropertyPage(const ResourceID& templateName, const TCHAR* caption);
	// メソッド
	void	cancelToClose();
	void	create(const ResourceID& templateName, const TCHAR* caption);
	void	setModified(bool changed = true);
	LRESULT	querySiblings(WPARAM wParam, LPARAM lParam);
private:
	INT_PTR	doModal(HWND parent);
	bool	doModeless(HWND parent, bool show);
	void	endDialog(int result);
#ifdef _DEBUG
#define assertValidAsPropertyPage() assertValidAsWindow(); assert(getParent().isWindow())
#else
#define assertValidAsPropertyPage()
#endif

private:
	PROPSHEETPAGE page_;
};

class PropertySheet : public Window {
public:
	// コンストラクタ
	PropertySheet();
	PropertySheet(const TCHAR* caption, HWND parent = 0, UINT activePage = 0);
	// メソッド
public:
	void	Construct(const TCHAR* lpszCaption, HWND hwndParent = 0, UINT iSelectPage = 0);
	int		GetActiveIndex() const;
	int		GetPageIndex(const CPropertyPage* pPage) const;
	int		GetPageCount() const;
	CPropertyPage*	GetPage(int nPage) const;
	CPropertyPage*	GetActivePage() const;
	bool	SetActivePage(int nPage);
	bool	SetActivePage(const CPropertyPage* pPage);
	void	SetTitle(const TCHAR* lpszCaption, UINT nStyle = 0);
	HWND	GetTabControl() const;
	void	SetFinishText(const TCHAR* lpszText);
	void	SetWizardButtons(DWORD dwFlags);
	void	SetWizardMode();
	void	EndStackedTabs(bool bStacked);
	void	SetCallbackProc(PFNPROPSHEETCALLBACK pfn);
	virtual int	DoModal();
	bool		Create(HWND hwndParent = 0, DWORD dwStyle = static_cast<DWORD>(-1), DWORD dwExStyle = 0);
	void		AddPage(const CPropertyPage* pPage);
	void		RemovePage(int nPage);
	void		RemovePage(const CPropertyPage* pPage);
	bool		PressButton(int nButton);
	void		EndDialog(int nEndID);
private:
	void	CommonConstruct(HWND hwndParent, UINT iSelectPage);

private:
	std::vector<CPropertyPage*>	m_listPages;
	PFNPROPSHEETCALLBACK		m_pfnCallback;
public:
	PROPSHEETHEADER	m_psh;
};


// PropertyPage class implementation
/////////////////////////////////////////////////////////////////////////////

inline PropertyPage::PropertyPage() {create(0, 0);}

inline PropertyPage::PropertyPage(const ResourceID& templateName, const TCHAR* caption) {create(templateName, caption);}

inline void PropertyPage::cancelToClose() {assertValidAsPropertyPage(); getParent().sendMessage(PSM_CANCELTOCLOSE);}

inline void PropertyPage::create(const ResourceID& templateName, const TCHAR* caption) {
	assertValid();
	STD_::memset(&page_, 0, sizeof(PROPSHEETPAGE));
	page_.dwSize = sizeof(PROPSHEETPAGE);
	page_.dwFlags = PSP_USETITLE;
	page_.hInstance = ::GetModuleHandle(0);
	page_.pszTemplate = templateName.name_;
	page_.pszTitle = caption;
	page_.pfnDlgProc = Dialog::windowProcedure;
	page_.lParam = reinterpret_cast<LPARAM>(this);
}

inline LRESULT PropertyPage::querySiblings(WPARAM wParam, LPARAM lParam) {
	assertValidAsPropertyPage(); return getParent().sendMessage(PSM_QUERYSIBLINGS, wParam, lParam);}

inline void PropertyPage::setModified(bool changed /* = true */) {
	assertValidAsPropertyPage(); getParent().sendMessage((changed ? PSM_CHANGED : PSM_UNCHANGED), reinterpret_cast<WPARAM>(getHandle()));}

inline LRESULT CALLBACK CPropertyPage::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	LPPROPSHEETPAGE						lpPropSheetPage;
	CPropertyPage*						pPropertyPage;
	static std::map<HWND, MPARAM>		mapMessages;
	std::map<HWND, MPARAM>::iterator	itMessages;
	MPARAM								mParam;

	if(message == WM_INITDIALOG) {
		lpPropSheetPage = reinterpret_cast<LPPROPSHEETPAGE>(lParam);
		pPropertyPage = reinterpret_cast<CPropertyPage*>(lpPropSheetPage->lParam);
		pPropertyPage->m_hWnd = hWnd;
		pPropertyPage->SetWindowLong(GWL_USERDATA, reinterpret_cast<LPARAM>(pPropertyPage));
		itMessages = mapMessages.find(hWnd);
		if(itMessages != mapMessages.end()){
			::SendMessage(hWnd, WM_MEASUREITEM, (*itMessages).second.wParam, (*itMessages).second.lParam);
			mapMessages.erase(itMessages);
		}
		return pPropertyPage->DispatchEvent(message, wParam, lParam);
	} else if(message == WM_MEASUREITEM) {
		pPropertyPage = reinterpret_cast<CPropertyPage*>(::GetWindowLong(hWnd, GWL_USERDATA));
		if(pPropertyPage == 0){
			mParam.wParam = wParam;
			mParam.lParam = lParam;
			mapMessages.insert(std::map<HWND, MPARAM>::value_type(hWnd, mParam));
			return false;
		} else
			return pPropertyPage->DispatchEvent(WM_MEASUREITEM, wParam, lParam);
	} else {
		pPropertyPage = reinterpret_cast<CPropertyPage*>(::GetWindowLong(hWnd, GWL_USERDATA));
		if(pPropertyPage != 0)
			return pPropertyPage->DispatchEvent(message, wParam, lParam);
		else
			return false;
	}
}


// CPropertySheet class implementation
/////////////////////////////////////////////////////////////////////////////

inline CPropertySheet::CPropertySheet() : m_pfnCallback(0) {
	CommonConstruct(0, 0);
	m_listPages.clear();
}

inline CPropertySheet::CPropertySheet(
		const TCHAR* lpszCaption, HWND hwndParent /* = 0 */, UINT iSelectPage /* = 0 */) : m_pfnCallback(0) {
	CommonConstruct(hwndParent, iSelectPage);
	m_psh.pszCaption = lpszCaption;
	m_listPages.clear();
}

inline void CPropertySheet::AddPage(const CPropertyPage* pPage) {
	AssertValid();
	assert(pPage != 0);

	if(IsWindow())
		SendMessage(PSM_ADDPAGE, 0, reinterpret_cast<LPARAM>(pPage));
	m_listPages.push_back(const_cast<CPropertyPage*>(pPage));
}

inline void CPropertySheet::CommonConstruct(HWND hwndParent, UINT iSelectPage) {
	ZeroMemory(&m_psh, sizeof(m_psh));
	m_psh.dwSize = sizeof(PROPSHEETHEADER);
	m_psh.dwFlags = PSH_HASHELP | PSH_NOAPPLYNOW | PSH_PROPSHEETPAGE;
	m_psh.pszCaption = _T("");
	m_psh.nStartPage = iSelectPage;
	m_psh.pfnCallback = m_pfnCallback;
	m_psh.hwndParent = hwndParent;
	m_psh.hInstance = ::GetModuleHandle(0);
}

inline int CPropertySheet::DoModal() {
	AssertValid();

	int nReturn;
	int nPageCount;

	nPageCount = m_listPages.size();
	if(m_pfnCallback != 0) {
		m_psh.dwFlags |= PSH_USECALLBACK;
		m_psh.pfnCallback = m_pfnCallback;
	}
	m_psh.dwFlags &= ~PSH_MODELESS;
	m_psh.nPages = nPageCount;
	m_psh.ppsp = 0;
	m_psh.ppsp = new PROPSHEETPAGE[nPageCount];
	for(int i = 0; i < nPageCount; i++)
		memcpy(const_cast<LPPROPSHEETPAGE>(m_psh.ppsp + i), &(m_listPages.at(i)->m_psp), sizeof(PROPSHEETPAGE));

	nReturn = ::PropertySheet(&m_psh);

	delete[] const_cast<PROPSHEETPAGE*>(m_psh.ppsp);
	m_psh.ppsp = 0;

	return nReturn;
}

inline int CPropertySheet::GetActiveIndex() const {
	AssertValidAsWindow();

	HWND hwndTabCtrl;

	hwndTabCtrl = GetTabControl();
	assert(::IsWindow(hwndTabCtrl));

	return static_cast<int>(::SendMessage(hwndTabCtrl, TCM_GETCURSEL, 0, 0L));
}

inline CPropertyPage* CPropertySheet::GetActivePage() const {
	AssertValidAsWindow();
	return GetPage(GetActiveIndex());
}

inline CPropertyPage* CPropertySheet::GetPage(int nPage) const {
	AssertValid();

	if(nPage >= GetPageCount())
		return 0;
	return m_listPages.at(nPage);
}

inline int CPropertySheet::GetPageCount() const {
	AssertValid();

	HWND hwndTabCtrl;

	if(IsWindow()){
		hwndTabCtrl = GetTabControl();
		assert(::IsWindow(hwndTabCtrl));
		return static_cast<int>(::SendMessage(hwndTabCtrl, TCM_GETITEMCOUNT, 0, 0L));
	}else
		return m_listPages.size();
}

inline int CPropertySheet::GetPageIndex(const CPropertyPage* pPage) const {
	AssertValid();
	assert(pPage != 0);

	int nPageCount;

	nPageCount = GetPageCount();
	for(int i = 0; i < nPageCount; i++){
		if(m_listPages.at(i) == pPage)
			return i;
	}
	return static_cast<int>(-1);
}

inline HWND CPropertySheet::GetTabControl() const {
	AssertValidAsWindow();
	return reinterpret_cast<HWND>(::SendMessage(m_hWnd, PSM_GETTABCONTROL, 0, 0L));
}

inline void CPropertySheet::RemovePage(int nPage) {
	AssertValidAsWindow();

	std::vector<CPropertyPage*>::iterator	it;
	
	if(nPage >= GetPageCount())
		return;
	if(IsWindow())
		SendMessage(PSM_REMOVEPAGE, nPage);
	for(int i = 0; i < nPage; i++)	it++;
	m_listPages.erase(it);
}

inline void CPropertySheet::RemovePage(const CPropertyPage* pPage) {
	AssertValidAsWindow();
	assert(pPage != 0);
	RemovePage(GetPageIndex(pPage));
}

inline void CPropertySheet::SetCallbackProc(PFNPROPSHEETCALLBACK pfn) {
	m_pfnCallback = pfn;
}

} /* namespace Controls */
} /* namespace Windows */
} /* namespace Manah */

#endif /* _PROPERTY_SHEET_H_ */

/* [EOF] */