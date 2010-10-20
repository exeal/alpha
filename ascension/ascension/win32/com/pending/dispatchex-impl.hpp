// dispatchex-impl.h
// (c) 2004, 2006 exeal

// This file is not maintained currently.

#ifndef DISPATCHEX_IMPL_HPP_
#define DISPATCHEX_IMPL_HPP_

#include <dispex.h>
#include <vector>
#include "com-basic.hpp"


namespace {
	struct TMember {
		TMember() : pwszName(0), bDeleted(false) {
			::VariantInit(&value);
		}
		~TMember() {
			delete[] pwszName;
			::VariantClear(&value);
		}
		DISPID		id;
		wchar_t*	pwszName;
		VARIANT		value;
		bool		bDeleted;
	};
}

namespace armaiti {
namespace ole {

/**
 *	IDispatchEx の実装
 *	@param Interface	IDispatchEx を継承したインターフェイス
 */
template<class Interface>
class IDispatchExImpl : virtual public Interface {
public:
	// コンストラクタ
	IDispatchExImpl(MEMBERID staticMemberMaxId);
	virtual ~IDispatchExImpl();

	// IDispatchEx メソッド
public:
	STDMETHOD(GetDispID)(BSTR bstrName, DWORD grfdex, DISPID* pid);
	STDMETHOD(InvokeEx)(DISPID id, LCID lcid, WORD wFlags,
		DISPPARAMS* pdp, VARIANT* pvarRes, EXCEPINFO* pei, IServiceProvider* pspCaller);
	STDMETHOD(DeleteMemberByName)(BSTR bstrName, DWORD grfdex);
	STDMETHOD(DeleteMemberByDispID)(DISPID id);
	STDMETHOD(GetMemberProperties)(DISPID id, DWORD grfdexFetch, DWORD* pgrfdex);
	STDMETHOD(GetMemberName)(DISPID id, BSTR* pbstrName);
	STDMETHOD(GetNextDispID)(DWORD grfdex, DISPID id, DISPID* pid);
	STDMETHOD(GetNameSpaceParent)(IUnknown** ppunk);

	// コピーの禁止
private:
	IDispatchExImpl(const IDispatchExImpl& rhs);
	operator =(const IDispatchExImpl& rhs);

private:
	TMember*	_FindMember(MEMBERID id) const;
	TMember*	_FindMember(const wchar_t* pwszName, bool bCaseSensitive) const;

	// データメンバ
private:
	std::vector<TMember*>	m_members;
	const MEMBERID			m_nStartId;	// 動的メンバの先頭 ID
};

/**
 *	コンストラクタ
 *	@param staticMemberMaxId	IDispatch で使うメンバ ID の最大値。動的メンバの ID はこれより大きな値が割り当てられる
 */
template<class Interface>
inline IDispatchExImpl<Interface>::IDispatchExImpl(MEMBERID staticMemberMaxId) : m_nStartId(staticMemberMaxId + 1) {
}

///	デストラクタ
template<class Interface>
inline IDispatchExImpl<Interface>::~IDispatchExImpl() {
	for(std::size_t i = 0; i < m_members.size(); ++i)
		delete m_members[i];
}

///	@see	IDispatchEx::DeleteMemberByDispID
template<class Interface>
inline STDMETHODIMP IDispatchExImpl<Interface>::DeleteMemberByDispID(DISPID id) {
	if(TMember* pFound = _FindMember(id))
		pFound->bDeleted = true;
	return S_OK;
}

///	@see	IDispatchEx::DeleteMemberByName
template<class Interface>
inline STDMETHODIMP IDispatchExImpl<Interface>::DeleteMemberByName(BSTR bstrName, DWORD grfdex) {
	if(TMember* pFound = _FindMember(bstrName, toBoolean(grfdex & fdexNameCaseSensitive)))
		pFound->bDeleted = true;
	return S_OK;
}

/**
 *	動的メンバリストから指定した ID を持つメンバを検索する
 *	@param id	ID
 *	@return		メンバ。見つからない場合は null
 */
template<class Interface>
inline TMember* IDispatchExImpl<Interface>::_FindMember(MEMBERID id) const {
	if(static_cast<std::size_t>(id - m_nStartId) >= m_members.size())
		return 0;
	return m_members[id - m_nStartId]->bDeleted ? 0 : m_members[id - m_nStartId];
}

/**
 *	動的メンバリストから指定した名前を持つメンバを検索する
 *	@param pwszName			名前
 *	@param bCaseSensitive	大文字小文字を区別するか
 *	@return					メンバ。見つからない場合は null
 */
template<class Interface>
inline TMember* IDispatchExImpl<Interface>::_FindMember(const wchar_t* pwszName, bool bCaseSensitive) const {
	assert(pwszName != 0);
	for(std::size_t i = 0; i < m_members.size(); ++i) {
		if((bCaseSensitive && wcscmp(m_members[i]->pwszName, pwszName) == 0)
				|| (!bCaseSensitive && wcsicmp(m_members[i]->pwszName, pwszName) == 0))
			return m_members[i]->bDeleted ? 0 : m_members[i];
	}
	return 0;
}

///	@see	IDispatchEx::GetDispID
template<class Interface>
inline STDMETHODIMP IDispatchExImpl<Interface>::GetDispID(BSTR bstrName, DWORD grfdex, DISPID* pid) {
	VERIFY_POINTER(pid);
	*pid = DISPID_UNKNOWN;

	if(bstrName == 0)
		return DISP_E_UNKNOWNNAME;
	else if(wcsicmp(bstrName, L"value") == 0) {
		*pid = DISPID_VALUE;
		return S_OK;
	} else if(GetIDsOfNames(IID_NULL, &bstrName, 1, LOCALE_USER_DEFAULT, pid) == S_OK)
		return S_OK;

	if(TMember* pFound = _FindMember(bstrName, toBoolean(grfdex & fdexNameCaseSensitive))) {
		*pid = pFound->id;
		return S_OK;
	} else if(toBoolean(grfdex & fdexNameEnsure)) {
		TMember*	pNewMember = new TMember;
		pNewMember->id = m_nStartId + m_members.size();
		pNewMember->pwszName = new wchar_t[wcslen(bstrName) + 1];
		wcscpy(pNewMember->pwszName, bstrName);
		m_members.push_back(pNewMember);
		*pid = pNewMember->id;
		return S_OK;
	} else
		return DISP_E_UNKNOWNNAME;
}

///	@see	IDispatchEx::GetMemberName
template<class Interface>
inline STDMETHODIMP IDispatchExImpl<Interface>::GetMemberName(DISPID id, BSTR* pbstrName) {
	VERIFY_POINTER(pbstrName);
	if(TMember* pFound = _FindMember(id)) {
		*pbstrName = ::SysAllocString(pFound->pwszName);
		return S_OK;
	}
	return DISP_E_UNKNOWNNAME;
}

///	@see	IDispatchEx::GetMemberProperties
template<class Interface>
inline STDMETHODIMP IDispatchExImpl<Interface>::GetMemberProperties(DISPID id, DWORD grfdexFetch, DWORD* pgrfdex) {
	VERIFY_POINTER(pgrfdex);
	*pgrfdex = 0;
	return E_NOTIMPL;
}

///	@see	IDispatchEx::GetNameSpaceParent
template<class Interface>
inline STDMETHODIMP IDispatchExImpl<Interface>::GetNameSpaceParent(IUnknown** ppunk) {
	VERIFY_POINTER(ppunk);
	*ppunk = 0;
	return E_NOTIMPL;
}

///	@see	IDispatchEx::GetNextDispID
template<class Interface>
inline STDMETHODIMP IDispatchExImpl<Interface>::GetNextDispID(DWORD grfdex, DISPID id, DISPID* pid) {
	VERIFY_POINTER(pid);

	if(m_members.empty() || static_cast<std::size_t>(id - m_nStartId) >= m_members.size() - 1)
		return S_FALSE;
	else if(id == DISPID_STARTENUM)
		*pid = m_nStartId;
	else
		*pid = id + 1;

	while(static_cast<std::size_t>(*pid - m_nStartId) < m_members.size() - 1 && m_members[*pid - m_nStartId]->bDeleted)
		++*pid;
	return (static_cast<std::size_t>(*pid - m_nStartId) < m_members.size() - 1) ? S_OK : S_FALSE;
}

///	@see	IDispatchEx::InvokeEx
template<class Interface>
inline STDMETHODIMP IDispatchExImpl<Interface>::InvokeEx(DISPID id, LCID lcid,
		WORD wFlags, DISPPARAMS* pdp, VARIANT* pvarRes, EXCEPINFO* pei, IServiceProvider* pspCaller) {
	UINT	iArgErr;
	HRESULT	hr = Invoke(id, IID_NULL, lcid, wFlags, pdp, pvarRes, pei, &iArgErr);

	if(hr != DISP_E_MEMBERNOTFOUND)
		return hr;

	if(wFlags == DISPATCH_PROPERTYGET
			|| wFlags == (DISPATCH_PROPERTYGET | DISPATCH_METHOD)) {
		VERIFY_POINTER(pvarRes);
		if(id == DISPID_NEWENUM) {
		} else if(id == DISPID_VALUE) {
			if(pdp == 0)
				return E_INVALIDARG;
			else if(pdp->cArgs != 1)
				return DISP_E_BADPARAMCOUNT;

			VARIANT	propertyName;
			::VariantInit(&propertyName);
			if(FAILED(::VariantChangeType(&propertyName, &pdp->rgvarg[0], 0, VT_BSTR))) {
				::VariantClear(&propertyName);
				return E_INVALIDARG;
			}
			if(TMember* pFound = _FindMember(propertyName.bstrVal, false))
				::VariantCopy(pvarRes, &pFound->value);
			else {
				TMember*	pNewMember = new TMember;
				pNewMember->id = m_nStartId + m_members.size();
				pNewMember->pwszName = new wchar_t[wcslen(propertyName.bstrVal) + 1];
				wcscpy(pNewMember->pwszName, propertyName.bstrVal);
				m_members.push_back(pNewMember);
				::VariantClear(pvarRes);
			}
			::VariantClear(&propertyName);
		} else if(TMember* pFound = _FindMember(id)) {
			::VariantCopy(pvarRes, &pFound->value);
			return S_OK;
		} else
			return DISP_E_MEMBERNOTFOUND;
	} else if(wFlags == DISPATCH_PROPERTYPUT
			|| wFlags == DISPATCH_PROPERTYPUTREF
			|| wFlags == (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF)) {
		if(pdp == 0)
			return E_INVALIDARG;
		if(id != DISPID_VALUE) {
			if(pdp->cArgs != 1)
				return DISP_E_BADPARAMCOUNT;
			if(TMember* pFound = _FindMember(id)) {
				::VariantCopy(&pFound->value, &pdp->rgvarg[0]);
				return S_OK;
			} else
				return DISP_E_MEMBERNOTFOUND;
		} else {
			if(pdp->cArgs != 2)
				return DISP_E_BADPARAMCOUNT;

			VARIANT	propertyName;
			::VariantInit(&propertyName);
			if(FAILED(::VariantChangeType(&propertyName, &pdp->rgvarg[1], 0, VT_BSTR)) || propertyName.bstrVal == 0) {
				::VariantClear(&propertyName);
				return E_INVALIDARG;
			}
			if(TMember* pFound = _FindMember(propertyName.bstrVal, false))
				::VariantCopy(&pFound->value, &pdp->rgvarg[1]);
			else {
				TMember*	pNewMember = new TMember;
				pNewMember->id = m_nStartId + m_members.size();
				pNewMember->pwszName = new wchar_t[wcslen(propertyName.bstrVal) + 1];
				wcscpy(pNewMember->pwszName, propertyName.bstrVal);
				::VariantCopy(&pNewMember->value, &pdp->rgvarg[1]);
				m_members.push_back(pNewMember);
			}
			::VariantClear(&propertyName);
			return S_OK;
		}
	} else if(wFlags == DISPATCH_METHOD) {
		if(TMember* pFound = _FindMember(id)) {
			CComPtr<IDispatch>	pMethod;
			if(pFound->value.vt == VT_DISPATCH)
				pMethod = pFound->value.pdispVal;
			else {
				VARIANT	method;
				::VariantInit(&method);
				hr = ::VariantChangeType(&method, &pFound->value, 0, VT_DISPATCH);
				if(FAILED(hr)) {
					::VariantClear(&method);
					return DISP_E_TYPEMISMATCH;
				}
				pMethod = method.pdispVal;
				::VariantClear(&method);
			}
			return pMethod->Invoke(DISPID_VALUE, IID_NULL, lcid, DISPATCH_METHOD, pdp, pvarRes, pei, &iArgErr);
		}
	}
	return DISP_E_MEMBERNOTFOUND;
}

}} // namespace armaiti::ole

#endif /* DISPATCHEX_IMPL_HPP_ */
