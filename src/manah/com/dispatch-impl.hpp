// dispatch-impl.hpp
// (c) 2002-2007 exeal

#ifndef MANAH_DISPATCH_IMPL_HPP
#define MANAH_DISPATCH_IMPL_HPP
#include "common.hpp"

namespace manah {
namespace com {
namespace ole {

// IProvideClassInfo2Impl ///////////////////////////////////////////////////
	
#ifdef __IProvideClassInfo2_INTERFACE_DEFINED__

/**
 *	IProvideClassInfo2 の基本的な実装
 *	@param clsid		CLSID
 *	@param iid			IID
 *	@param libid		LIBID
 *	@param majorVersion	メジャーバージョン
 *	@param minorVersion	マイナーバージョン
 */
template<const CLSID* clsid, const IID* iid, const GUID* libid, WORD majorVersion = 1, WORD minorVersion = 0>
class IProvideClassInfo2Impl : virtual public IProvideClassInfo2 {
public:
	/// コンストラクタ
	IProvideClassInfo2Impl() {
		ComPtr<ITypeLib> typeLib;
		HRESULT hr = ::LoadRegTypeLib(*libid, majorVersion, minorVersion, 0, &typeLib);
		assert(SUCCEEDED(hr));
		typeLib->GetTypeInfoOfGuid(*iid, &typeInfo_);
		assert(SUCCEEDED(hr));
	}
	///	@see IProvideClassInfo::GetClassInfo
	STDMETHOD(GetClassInfo)(ITypeInfo** ppTypeInfo) {
		VERIFY_POINTER(ppTypeInfo);
		(*ppTypeInfo = typeInfo_.get())->AddRef();
		return S_OK;
	}
	///	@see IProvideClassInfo2::GetGUID
	STDMETHOD(GetGUID)(DWORD dwGuidKind, GUID* pGUID) {
		VERIFY_POINTER(pGUID);
		if(dwGuidKind == GUIDKIND_DEFAULT_SOURCE_DISP_IID && clsid != 0) {
			*pGUID = iid;
			return S_OK;
		}
		*pGUID = 0;
		return E_INVALIDARG;
	}
private:
	ComPtr<ITypeInfo> typeInfo_;
};

#endif /* __IProvideClassInfo2_INTERFACE_DEFINED__ */


// IDispatchImpl クラスのための2つのポリシークラス
/////////////////////////////////////////////////////////////////////////////

/**
 *	LIBID からタイプライブラリをロード
 *	@param libid		タイプライブラリの LIBID
 *	@param iid			IID
 *	@param majorVersion	メジャーバージョン
 *	@param minorVersion	マイナーバージョン
 */
template<const GUID* libid, const IID* iid, WORD majorVersion = 1, WORD minorVersion = 0> class RegTypeLibTypeInfoHolder {
public:
	/// コンストラクタ
	RegTypeLibTypeInfoHolder() {
		ComPtr<ITypeLib> typeLib;
		assert(libid != 0 && iid != 0);
		HRESULT hr = ::LoadRegTypeLib(*libid, majorVersion, minorVersion, 0, &typeLib);
		assert(SUCCEEDED(hr));
		hr = typeLib->GetTypeInfoOfGuid(*iid, &typeInfo_);
		assert(SUCCEEDED(hr));
	}
	/// ITypeInfo インスタンスを返す
	ComPtr<ITypeInfo> getTypeInfo() const throw() {return typeInfo_;}
private:
	ComPtr<ITypeInfo> typeInfo_;
};

/**
 *	直接パスを指定してタイプライブラリをロード
 *	@param TypeLibPath	タイプライブラリのパスを提供するクラス
 *	@param iid			IID
 */
template<class TypeLibPath, const IID* iid> class PathTypeLibTypeInfoHolder {
public:
	/// コンストラクタ
	PathTypeLibTypeInfoHolder() {
		ComPtr<ITypeLib> typeLib;
		HRESULT hr = ::LoadTypeLib(TypeLibPath::getPath(), &typeLib);
		assert(SUCCEEDED(hr));
		hr = typeLib->GetTypeInfoOfGuid(*iid, &typeInfo_);
		assert(SUCCEEDED(hr));
	}
	/// ITypeInfo インスタンスを返す
	ComPtr<ITypeInfo> getTypeInfo() const throw() {return typeInfo_;}
private:
	ComPtr<ITypeInfo> typeInfo_;
};


// IDispatchImpl class definition and implementation
/////////////////////////////////////////////////////////////////////////////

/**
 *	IDispatch の標準的な実装
 *	@param DI				IDispatch を基底とするインターフェイス
 *	@param TypeInfoHolder	ITypeInfo を提供するクラス
 */
template<class DI, class TypeInfoHolder>
class IDispatchImpl : virtual public DI {
public:
	/// @see IDispatch::GetIDsOfNames
	STDMETHOD(GetIDsOfNames)(REFIID riid,
			OLECHAR** rgszNames, unsigned int cNames, LCID lcid, DISPID* rgDispId) {
		assert(riid == IID_NULL);
		return ::DispGetIDsOfNames(typeInfoHolder_.getTypeInfo(), rgszNames, cNames, rgDispId);
	}
	/// @see IDispatch::GetTypeInfo
	STDMETHOD(GetTypeInfo)(unsigned int iTypeInfo, LCID lcid, ITypeInfo** ppTypeInfo) {
		VERIFY_POINTER(ppTypeInfo);
		if(iTypeInfo != 0)
			return DISP_E_BADINDEX;
		(*ppTypeInfo = typeInfoHolder_.getTypeInfo())->AddRef();
		return S_OK;
	}
	/// @see IDispatch::GetTypeInfoCount
	STDMETHOD(GetTypeInfoCount)(unsigned int* pctInfo) {
		VERIFY_POINTER(pctInfo);
		*pctInfo = 1;
		return S_OK;
	}
	/// @see IDispatch::Invoke
	STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID, WORD wFlags,
			DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, unsigned int* puArgErr) {
		if(riid != IID_NULL)
			return DISP_E_UNKNOWNINTERFACE;
		return ::DispInvoke(static_cast<DI*>(this),
				typeInfoHolder_.getTypeInfo(), dispidMember, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
	}
private:
	TypeInfoHolder typeInfoHolder_;
};

}}} // namespace manah::com::ole

#endif /* !MANAH_DISPATCH_IMPL_HPP */
