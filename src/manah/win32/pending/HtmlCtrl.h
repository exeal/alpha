// HtmlCtrl.h
/////////////////////////////////////////////////////////////////////////////

// !このファイルはまだ使えない!

#ifndef _HTML_CTRL_H_
#define _HTML_CTRL_H_

#include "Window.h"
#include <exdisp.h>
#include "..\Armaiti\OleTypeWrapper.h"


namespace Manah {
namespace Windows {
namespace Controls {


// CHtmlCtrl class definition
/////////////////////////////////////////////////////////////////////////////

class CHtmlCtrl : public CWindow {
	// コンストラクタ
public:
	CHtmlCtrl();
	virtual ~CHtmlCtrl();
private:
	CHtmlCtrl(const CHtmlCtrl& rhs);

	// メソッド
public:
	bool		Create(HWND hwndParent);

	OLECHAR*	GetType() const;
	OLECHAR*	GetLocationName() const;
	READYSTATE	GetReadyState() const;
	bool		IsOffline() const;
	void		SetOffline(bool bOffline);
	bool		IsSilent() const;
	void		SetSilent(bool bSilent);
	bool		IsTopLevelContainer() const;
	OLECHAR*	GetLocationURL() const;
	bool		IsBusy() const;
	HRESULT		GetApplication(IDispatch** ppApplication) const;
	HRESULT		GetParentBrowser(IDispatch** ppParentBrowser) const;
	HRESULT		GetContainer(IDispatch** ppContainer) const;
	HRESULT		GetHtmlDocument(IDispatch** ppDocument) const;
//	TCHAR*		GetFullName() const;
//	bool		IsToolbarVisible() const;
//	void		ShowToolbar(bool bShow = true);
//	bool		IsMenubarVisible() const;
//	void		ShowMenubar(bool bShow = true);
	bool		IsFullScreen() const;
//	void		SetFullScreen(bFullScreen);
	OLECMDF		QueryStatusWB(OLECMDID nCmdId) const;
	bool		IsRegisteredAsBrowser() const;
	void		RegisterAsBrowser(bool bRegister);
	bool		IsRegisteredAsDropTarget() const;
	void		RegisterAsDropTarget(bool bRegister);
	bool		IsTheaterMode() const;
	void		SetTheaterMode(bool bTheaterMode);
//	bool		IsAddressBarVisible() const;
//	void		ShowAddressBar(bool bShow = true);
//	bool		IsStatusBarVisible() const;
//	void		ShowStatusBar(bool bShow = true);

	void		GoBack();
	void		GoForward();
	void		GoHome();
	void		GoSearch();
	void		Navigate(const OLECHAR* lpszURL, DWORD dwFlags = 0, const OLECHAR* lpszTargetFrameName = 0,
					const OLECHAR* lpszHeaders = 0, const void* lpPostData = 0, DWORD dwPostDataLength = 0);
	void		Navigate2(const OLECHAR* lpszURL, DWORD dwFlags = 0, const OLECHAR* lpszTargetFrameName = 0,
					const TCHAR* lpszHeaders = 0, const void* lpPostData = 0, DWORD dwPostDataLength = 0);
	void		Refresh();
	void		Refresh2(int nLevel);
	void		Stop();
	void		PutProperty(const OLECHAR* lpszPropertyName, const VARIANT& vtValue);
	bool		GetProperty(const OLECHAR* lpszPropertyName, VARIANT& vtValue) const;
	void		ExecWB(OLECMDID nCmdId, OLECMDEXECOPT cmdexecopt, const VARIANT& vtIn, VARIANT& vtOut);
	bool		LoadResource(const TCHAR* lpszResource);
	bool		LoadResource(unsigned int nResourceId);

	// オーバーライド
protected:
	virtual void	OnDestory();

	// データメンバ
private:
	IWebBrowser2*	m_pWebBrowser;
};

} /* namespace Controls */
} /* namespace Windows */
} /* namespace Manah */


// CHtmlCtrl class implementation
/////////////////////////////////////////////////////////////////////////////

using Manah::Windows::Controls::CHtmlCtrl;
using Armaiti::OLE::CComVariant;

inline CHtmlCtrl::CHtmlCtrl() : m_pWebBrowser(0) {
}

inline CHtmlCtrl::~CHtmlCtrl() {
	if(m_pWebBrowser != 0)
		m_pWebBrowser->Release();
}

inline bool CHtmlCtrl::Create(HWND hwndParent) {
	AssertValid();

	if(IsWindow())
		return false;

	CLSID	clsid;
	if(FAILED(::CLSIDFromProgID(OLESTR("InternetExplorer.Application"), &clsid)))
		return false;
	if(FAILED(::CoCreateInstance(clsid, 0, CLSCTX_ALL,
			IID_IWebBrowser2, reinterpret_cast<void**>(&m_pWebBrowser))))
		return false;
	m_pWebBrowser->put_FullScreen(VARIANT_TRUE);
	m_pWebBrowser->get_HWND(reinterpret_cast<long*>(&m_hWnd));
	ModifyStyle(0, WS_CHILD);
	SetParent(hwndParent);
	return true;
}

inline void CHtmlCtrl::ExecWB(OLECMDID nCmdId, OLECMDEXECOPT cmdexecopt, const VARIANT& vtIn, VARIANT& vtOut) {
	assert(m_pWebBrowser != 0);
	m_pWebBrowser->ExecWB(nCmdId, cmdexecopt, &const_cast<VARIANT&>(vtIn), &vtOut);
}

inline HRESULT CHtmlCtrl::GetApplication(IDispatch** ppApplication) const {
	assert(m_pWebBrowser != 0);
	if(ppApplication == 0)
		return E_POINTER;
	return m_pWebBrowser->get_Application(ppApplication);
}

inline HRESULT CHtmlCtrl::GetContainer(IDispatch** ppContainer) const {
	assert(m_pWebBrowser != 0);
	if(ppContainer == 0)
		return E_POINTER;
	return m_pWebBrowser->get_Container(ppContainer);
}

inline HRESULT CHtmlCtrl::GetHtmlDocument(IDispatch** ppDocument) const {
	assert(m_pWebBrowser != 0);
	if(ppDocument == 0)
		return E_POINTER;
	return m_pWebBrowser->get_Document(ppDocument);
}

inline OLECHAR* CHtmlCtrl::GetLocationName() const {
	static OLECHAR	szLocationName[1024];
	BSTR			bstrLocationName = 0;
	m_pWebBrowser->get_LocationName(&bstrLocationName);
	wcscpy(szLocationName, (bstrLocationName != 0) ? bstrLocationName : OLESTR(""));
	::SysFreeString(bstrLocationName);
	return szLocationName;
}

inline OLECHAR* CHtmlCtrl::GetLocationURL() const {
	static OLECHAR	szLocationURL[1024];
	BSTR			bstrLocationURL = 0;
	m_pWebBrowser->get_LocationURL(&bstrLocationURL);
	wcscpy(szLocationURL, (bstrLocationURL != 0) ? bstrLocationURL : OLESTR(""));
	::SysFreeString(bstrLocationURL);
	return szLocationURL;
}

inline HRESULT CHtmlCtrl::GetParentBrowser(IDispatch** ppParentBrowser) const {
	assert(m_pWebBrowser != 0);
	if(ppParentBrowser == 0)
		return E_POINTER;
	return m_pWebBrowser->get_Parent(ppParentBrowser);
}

inline bool CHtmlCtrl::GetProperty(const OLECHAR* lpszPropertyName, VARIANT& vtValue) const {
	assert(m_pWebBrowser != 0);
	BSTR	bstrPropertyName = ::SysAllocString(lpszPropertyName);
	bool	bSucceeded = SUCCEEDED(m_pWebBrowser->GetProperty(bstrPropertyName, &vtValue));
	::SysFreeString(bstrPropertyName);
	return bSucceeded;
}

inline READYSTATE CHtmlCtrl::GetReadyState() const {
	assert(m_pWebBrowser != 0);
	READYSTATE	rs;
	m_pWebBrowser->get_ReadyState(&rs);
	return rs;
}

inline OLECHAR* CHtmlCtrl::GetType() const {
	assert(m_pWebBrowser != 0);
	static OLECHAR	szType[1024];
	BSTR			bstrType = 0;
	m_pWebBrowser->get_Type(&bstrType);
	wcscpy(szType, (bstrType != 0) ? bstrType : OLESTR(""));
	::SysFreeString(bstrType);
	return szType;
}

inline void CHtmlCtrl::OnDestory() {
	if(m_pWebBrowser != 0)
		m_pWebBrowser->Release();
	CWindow::OnDestroy();
}

inline void CHtmlCtrl::GoBack() {
	assert(m_pWebBrowser != 0);
	m_pWebBrowser->GoBack();
}

inline void CHtmlCtrl::GoForward() {
	assert(m_pWebBrowser != 0);
	m_pWebBrowser->GoForward();
}

inline void CHtmlCtrl::GoHome() {
	assert(m_pWebBrowser != 0);
	m_pWebBrowser->GoHome();
}

inline void CHtmlCtrl::GoSearch() {
	assert(m_pWebBrowser != 0);
	m_pWebBrowser->GoSearch();
}

inline bool CHtmlCtrl::IsBusy() const {
	assert(m_pWebBrowser != 0);
	VARIANT_BOOL	bBusy;
	m_pWebBrowser->get_Busy(&bBusy);
	return toBoolean(bBusy);
}

inline bool CHtmlCtrl::IsFullScreen() const {
	assert(m_pWebBrowser != 0);
	VARIANT_BOOL	bFullScreen;
	m_pWebBrowser->get_FullScreen(&bFullScreen);
	return toBoolean(bFullScreen);
}

inline bool CHtmlCtrl::IsOffline() const {
	assert(m_pWebBrowser != 0);
	VARIANT_BOOL	bOffline;
	m_pWebBrowser->get_Offline(&bOffline);
	return toBoolean(bOffline);
}

inline bool CHtmlCtrl::IsRegisteredAsBrowser() const {
	assert(m_pWebBrowser != 0);
	VARIANT_BOOL	bRegistered;
	m_pWebBrowser->get_RegisterAsBrowser(&bRegistered);
	return toBoolean(bRegistered);
}

inline bool CHtmlCtrl::IsRegisteredAsDropTarget() const {
	assert(m_pWebBrowser != 0);
	VARIANT_BOOL	bRegistered;
	m_pWebBrowser->get_RegisterAsDropTarget(&bRegistered);
	return toBoolean(bRegistered);
}

inline bool CHtmlCtrl::IsSilent() const {
	assert(m_pWebBrowser != 0);
	VARIANT_BOOL	bSilent;
	m_pWebBrowser->get_Silent(&bSilent);
	return toBoolean(bSilent);
}

inline bool CHtmlCtrl::IsTheaterMode() const {
	assert(m_pWebBrowser != 0);
	VARIANT_BOOL	bTheaterMode;
	m_pWebBrowser->get_TheaterMode(&bTheaterMode);
	return toBoolean(bTheaterMode);
}

inline bool CHtmlCtrl::IsTopLevelContainer() const {
	assert(m_pWebBrowser != 0);
	VARIANT_BOOL	bTopLevelContainer;
	m_pWebBrowser->get_TopLevelContainer(&bTopLevelContainer);
	return toBoolean(bTopLevelContainer);
}

inline void CHtmlCtrl::Navigate(const OLECHAR* lpszURL, DWORD dwFlags /* = 0 */,
		const OLECHAR* lpszTargetFrameName /* = 0 */, const OLECHAR* lpszHeaders /* = 0 */,
		const void* lpPostData /* = 0 */, DWORD dwPostDataLength /* = 0 */) {
	assert(m_pWebBrowser != 0);
	BSTR		bstrURL = ::SysAllocString(lpszURL);
	CComVariant	vtFlags = dwFlags;
	CComVariant	vtTargetFrameName = lpszTargetFrameName;
	CComVariant	vtHeaders = lpszHeaders;
	SAFEARRAY	sarrPostData = {1, 0, dwPostDataLength, 0, const_cast<void*>(lpPostData)};
	VARIANTARG	vtPostData;
	vtPostData.vt = VT_UI1 | VT_ARRAY;
	vtPostData.parray = &sarrPostData;
	m_pWebBrowser->Navigate(bstrURL, &vtFlags, &vtTargetFrameName, &vtPostData, &vtHeaders);
	::SysFreeString(bstrURL);
}

inline void CHtmlCtrl::Navigate2(const OLECHAR* lpszURL, DWORD dwFlags /* = 0 */,
		const OLECHAR* lpszTargetFrameName /* = 0 */, const OLECHAR* lpszHeaders /* = 0 */,
		const void* lpPostData /* = 0 */, DWORD dwPostDataLength /* = 0 */) {
	assert(m_pWebBrowser != 0);
	BSTR		bstrURL = ::SysAllocString(lpszURL);
	CComVariant	vtURL = bstrURL;
	CComVariant	vtFlags = dwFlags;
	CComVariant	vtTargetFrameName = lpszTargetFrameName;
	CComVariant	vtHeaders = lpszHeaders;
	SAFEARRAY	sarrPostData = {1, 0, dwPostDataLength, 0, const_cast<void*>(lpPostData)};
	VARIANTARG	vtPostData;
	vtPostData.vt = VT_UI1 | VT_ARRAY;
	vtPostData.parray = &sarrPostData;
	m_pWebBrowser->Navigate2(&vtURL, &vtFlags, &vtTargetFrameName, &vtPostData, &vtHeaders);
	::SysFreeString(bstrURL);
}

inline void CHtmlCtrl::PutProperty(const OLECHAR* lpszPropertyName, const VARIANT& vtValue) {
	assert(m_pWebBrowser != 0);
	BSTR	bstrPropertyName = ::SysAllocString(lpszPropertyName);
	m_pWebBrowser->PutProperty(bstrPropertyName, const_cast<VARIANT&>(vtValue));
	::SysFreeString(bstrPropertyName);
}

inline OLECMDF CHtmlCtrl::QueryStatusWB(OLECMDID nCmdId) const {
	assert(m_pWebBrowser != 0);
	OLECMDF	cmdf;
	m_pWebBrowser->QueryStatusWB(nCmdId, &cmdf);
	return cmdf;
}

inline void CHtmlCtrl::Refresh() {
	assert(m_pWebBrowser != 0);
	m_pWebBrowser->Refresh();
}

inline void CHtmlCtrl::Refresh2(int nLevel) {
	assert(m_pWebBrowser != 0);
	CComVariant	vtLevel = nLevel;
	m_pWebBrowser->Refresh2(&vtLevel);
}

inline void CHtmlCtrl::RegisterAsBrowser(bool bRegister) {
	assert(m_pWebBrowser != 0);
	m_pWebBrowser->put_RegisterAsBrowser(bRegister ? VARIANT_TRUE : VARIANT_FALSE);
}

inline void CHtmlCtrl::RegisterAsDropTarget(bool bRegister) {
	assert(m_pWebBrowser != 0);
	m_pWebBrowser->put_RegisterAsDropTarget(bRegister ? VARIANT_TRUE : VARIANT_FALSE);
}

inline void CHtmlCtrl::SetOffline(bool bOffline) {
	assert(m_pWebBrowser != 0);
	m_pWebBrowser->put_Offline(bOffline ? VARIANT_TRUE : VARIANT_FALSE);
}

inline void CHtmlCtrl::SetSilent(bool bSilent) {
	assert(m_pWebBrowser != 0);
	m_pWebBrowser->put_Silent(bSilent ? VARIANT_TRUE : VARIANT_FALSE);
}

inline void CHtmlCtrl::SetTheaterMode(bool bTheaterMode) {
	assert(m_pWebBrowser != 0);
	m_pWebBrowser->put_TheaterMode(bTheaterMode ? VARIANT_TRUE : VARIANT_FALSE);
}

inline void CHtmlCtrl::Stop() {
	assert(m_pWebBrowser != 0);
	m_pWebBrowser->Stop();
}

#endif /* _HTML_CTRL_H_ */

/* [EOF] */