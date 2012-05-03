/**
 * @file dispatch-impl.hpp
 * @author exeal
 * @date 2002-2012
 */

#ifndef ASCENSION_WIN32_DISPATCH_IMPL_HPP
#define ASCENSION_WIN32_DISPATCH_IMPL_HPP
#include "com.hpp"

namespace ascension {
	namespace win32 {
		namespace com {
			namespace ole {

// IProvideClassInfo2Impl ///////////////////////////////////////////////////
	
#ifdef __IProvideClassInfo2_INTERFACE_DEFINED__

/**
 * Standard implementation of @c IProvideClassInfo2 interface.
 * @tparam clsid The CLSID
 * @tparam iid The IID
 * @tparam libid The LIBID
 * @tparam majorVersion The major version
 * @tparam minorVersion The minor version
 */
template<const CLSID* clsid, const IID* iid, const GUID* libid, WORD majorVersion = 1, WORD minorVersion = 0>
class IProvideClassInfo2Impl : /*virtual*/ public IProvideClassInfo2 {
public:
	/// Constructor.
	IProvideClassInfo2Impl() {
		ComPtr<ITypeLib> typeLib;
		HRESULT hr = ::LoadRegTypeLib(*libid, majorVersion, minorVersion, 0, typeLib.initialize());
		assert(SUCCEEDED(hr));
		typeLib->GetTypeInfoOfGuid(*iid, typeInfo_.initialize());
		assert(SUCCEEDED(hr));
	}
	///	@see IProvideClassInfo#GetClassInfo
	STDMETHOD(GetClassInfo)(ITypeInfo** ppTypeInfo) {
		MANAH_VERIFY_POINTER(ppTypeInfo);
		(*ppTypeInfo = typeInfo_.get())->AddRef();
		return S_OK;
	}
	///	@see IProvideClassInfo2#GetGUID
	STDMETHOD(GetGUID)(DWORD dwGuidKind, GUID* pGUID) {
		MANAH_VERIFY_POINTER(pGUID);
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

#endif // __IProvideClassInfo2_INTERFACE_DEFINED__


/**
 * Loads the type library from LIBID.
 * @tparam libid The LIBID of the type library
 * @tparam iid The IID of the type to load
 * @tparam majorVersion The major version
 * @tparam minorVersion The minor version
 */
template<const GUID* libid, const IID* iid, WORD majorVersion = 1, WORD minorVersion = 0> class TypeInformationFromRegistry {
public:
	TypeInformationFromRegistry() {
		ComPtr<ITypeLib> typeLibrary;
		assert(libid != nullptr && iid != nullptr);
		HRESULT hr = ::LoadRegTypeLib(*libid, majorVersion, minorVersion, 0, typeLibrary.initialize());
		assert(SUCCEEDED(hr));
		hr = typeLibrary->GetTypeInfoOfGuid(*iid, typeInformation_.initialize());
		assert(SUCCEEDED(hr));
	}
	ComPtr<ITypeInfo> get() const /*throw()*/ {return typeInformation_;}
private:
	ComPtr<ITypeInfo> typeInformation_;
};

/**
 * Loads the type library from file.
 * @tparam TypeLibPath The class provides the path name of the type library.
 * @tparam iid The IID of the type to load
 */
template<typename TypeLibraryPath, const IID* iid> class TypeInformationFromPath {
public:
	TypeInformationFromPath() {
		ComPtr<ITypeLib> typeLibrary;
		HRESULT hr = ::LoadTypeLib(TypeLibraryPath::get(), typeLibrary.initialize());
		assert(SUCCEEDED(hr));
		hr = typeLibrary->GetTypeInfoOfGuid(*iid, typeInformation_.initialize());
		assert(SUCCEEDED(hr));
	}
	/// Returns an @c ITypeInfo instance.
	ComPtr<ITypeInfo> get() const /*throw()*/ {return typeInformation_;}
private:
	ComPtr<ITypeInfo> typeInformation_;
};

/**
 * Loads the type library from program module.
 * @param iid The IID of the type to load
 */
template<const IID* iid> class TypeInformationFromExecutable {
public:
	TypeInformationFromExecutable() {
		WCHAR programName[MAX_PATH];
		const DWORD n = ::GetModuleFileNameW(nullptr, programName, MANAH_COUNTOF(programName));
		if(n != 0 && n != MANAH_COUNTOF(programName)) {
			ComPtr<ITypeLib> typeLibrary;
			if(SUCCEEDED(::LoadTypeLib(programName, typeLibrary.initialize())))
				typeLibrary->GetTypeInfoOfGuid(*iid, typeInformation_.initialize());
		}
	}
	ComPtr<ITypeInfo> get() const /*throw()*/ {return typeInformation_;}
private:
	ComPtr<ITypeInfo> typeInformation_;
};


// IDispatchImpl class definition and implementation
/////////////////////////////////////////////////////////////////////////////

/**
 * Standard implementation of @c IDispatch interface.
 * @tparam Base The base class derived from dispatch interfaces to implement
 * @tparam TypeInformationProvider Provides @c ITypeInfo
 */
template<typename Base, typename TypeInformationProvider>
class IDispatchImpl : public Base {
public:
	/// @see IDispatch#GetIDsOfNames
	STDMETHODIMP GetIDsOfNames(REFIID iid, OLECHAR** names, unsigned int numberOfNames, LCID, DISPID* id) {
		return (iid == IID_NULL) ? ::DispGetIDsOfNames(tip_.get().get(), names, numberOfNames, id) : E_INVALIDARG;}
	/// @see IDispatch#GetTypeInfo
	STDMETHODIMP GetTypeInfo(unsigned int index, LCID, ITypeInfo** typeInformation) {
		MANAH_VERIFY_POINTER(typeInformation);
		return (index == 0) ? ((*typeInformation = tip_.get().get())->AddRef(), S_OK) : DISP_E_BADINDEX;}
	/// @see IDispatch#GetTypeInfoCount
	STDMETHODIMP GetTypeInfoCount(unsigned int* number) {MANAH_VERIFY_POINTER(number); *number = 1; return S_OK;}
	/// @see IDispatch#Invoke
	STDMETHODIMP Invoke(DISPID id, REFIID iid, LCID, WORD flags, DISPPARAMS* parameters, VARIANT* result,
		EXCEPINFO* exception, unsigned int* argErr) {return (iid == IID_NULL) ? ::DispInvoke(static_cast<IDispatch*>(this),
		tip_.get().get(), id, flags, parameters, result, exception, argErr) : DISP_E_UNKNOWNINTERFACE;}
private:
	TypeInformationProvider tip_;
};

			}
		}
	}
} // namespace ascension.win32.com.ole

#endif // !ASCENSION_WIN32_DISPATCH_IMPL_HPP