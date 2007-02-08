// ole-type-wrapper.hpp
// (c) 2002-2007 exeal

#ifndef MANAH_OLE_TYPE_WRAPPER_HPP
#define MANAH_OLE_TYPE_WRAPPER_HPP
#include "common.hpp"

namespace manah {
namespace com {
namespace ole {

// ComBSTR //////////////////////////////////////////////////////////////////

/// wrapper for BSTR
/// @see _bstr_t class (comutil.h)
template<bool allowConversion = true> class ComBSTR {
public:
	///	constructor
	explicit ComBSTR(const OLECHAR* p = 0) throw() : bstr_(::SysAllocString(p)) {}
	///	constructor
	explicit ComBSTR(const char* p) throw() {
		if(p != 0) {
			int len = ::MultiByteToWideChar(CP_ACP, 0, p, -1, 0, 0);
			wchar_t* buffer = new wchar_t[len + 1];
			::MultiByteToWideChar(CP_ACP, 0, p, -1, buffer, len);
			bstr_ = ::SysAllocString(buffer);
			delete[] buffer;
		} else
			bstr_ = 0;
	}
	/// constructor
	explicit ComBSTR(const unsigned char* p) throw() : ComBSTR(reinterpret_cast<char*>(p)) {}
	///	constructor
	explicit ComBSTR(const VARIANT& value) throw() {
		if(value.vt == VT_BSTR)
			bstr_ = ::SysAllocString(value.bstrVal);
		else {
			VARIANT temp;
			::VariantInit(&temp);
			::VariantChangeType(&temp, const_cast<VARIANT*>(&value), 0, VT_BSTR);
			bstr_ = ::SysAllocString(temp.bstrVal);
			::VariantClear(&temp);
		}
	}
	/// copy-constructor
	explicit ComBSTR(const ComBSTR& rhs) : bstr_(::SysAllocString(rhs.bstr_)) throw() {}
	/// destructor
	~ComBSTR() throw() {if(!isNull()) ::SysFreeString(bstr_);}
	/// assignment operator
	ComBSTR& operator =(const OLECHAR* p) throw() {assign(p); return *this;}
	/// implicit conversion operator
	operator const BSTR() const throw();
	/// assign
	void assign(const OLECHAR* p) {assert(!isNull()); if(!::SysReAllocString(&bstr_, p)) throw std::bad_alloc("SysReAllocString failed.");}
	/// return raw BSTR string
	const BSTR get() const throw() {return bstr_;}
	/// if the string is empty
	bool isEmpty() const throw() {return getLength() == 0;}
	/// if the string is null
	bool isNull() const throw() {return bstr_ == 0;}
	/// return length of the string
	UINT getLength() const throw() {return ::SysStringLen(bstr_);}
	/// return byte count of the string
	UINT getByteLength() const throw() {return ::SysStringByteLen(bstr_);}

private:
	BSTR bstr_;
};

template<> inline ComBSTR<true>::operator const BSTR() const throw() {return get();}

/// equal operator
template<bool allowConversion>
inline bool operator ==(const ComBSTR<allowConversion>& lhs, const OLECHAR* rhs) throw() {return std::wcscmp(safeBSTR(lhs.getBSTR()), rhs) == 0;}

/// equal operator
template<bool allowConversion>
inline bool operator ==(const OLECHAR* lhs, const ComBSTR<allowConversion>& rhs) throw() {return rhs == lhs;}

/// non-equal operator
template<bool allowConversion>
inline bool operator !=(const ComBSTR<allowConversion>& lhs, const OLECHAR* rhs) throw() {return !(lhs == rhs);}

/// non-equal operator
template<bool allowConversion>
inline bool operator !=(const OLECHAR* lhs, const ComBSTR<allowConversion>& rhs) throw() {return !(rhs == lhs);}


// ComVariant ///////////////////////////////////////////////////////////////

/// convert VARTYPE to C++ type
template<VARTYPE type> struct VarType2CppType;

/// convert C++ type to VARTYPE
template<class type> struct CppType2VarType;

/// descriminator for VARTYPE
template<VARTYPE type> struct VarTypeDescriminator {
	typedef typename VarType2CppType<type>::result CppType;
	typedef CppType(VARIANT::*MemberType);
	static MemberType member;
	static CppType& value(VARIANT& var);
};
template<VARTYPE type> typename VarTypeDescriminator<type>::MemberType VarTypeDescriminator<type>::member = 0;

#define MAP_TYPE_V2C(var_type, cpp_type)	\
	template<> struct VarType2CppType<var_type> {typedef cpp_type result;};
#define MAP_TYPE(var_type, cpp_type)	\
	MAP_TYPE_V2C(var_type, cpp_type)	\
	template<> struct CppType2VarType<cpp_type> {enum{result = var_type};};
#define DESCRIMINATE_TYPE(var_type, member_name)				\
	template<> inline VarTypeDescriminator<var_type>::CppType&	\
	VarTypeDescriminator<var_type>::value(VARIANT& var) {return (member = &VARIANT::member_name), var.*member;}
#define VARTYPE_TABLE_V2C(var_type, cpp_type, member_name)	\
	MAP_TYPE_V2C(var_type, cpp_type)						\
	DESCRIMINATE_TYPE(var_type, member_name)
#define VARTYPE_TABLE(var_type, cpp_type, member_name)	\
	MAP_TYPE(var_type, cpp_type)						\
	DESCRIMINATE_TYPE(var_type, member_name)

// define all to error
VARTYPE_TABLE(VT_I8, LONGLONG, llVal)
VARTYPE_TABLE(VT_I4, LONG, lVal)	// conflicts with VT_ERROR/SCODE
VARTYPE_TABLE(VT_UI1, BYTE, bVal)
VARTYPE_TABLE_V2C(VT_I2, SHORT, iVal)	// conflicts with VT_BOOL/VARIANT_BOOL
VARTYPE_TABLE(VT_R4, FLOAT, fltVal)
VARTYPE_TABLE(VT_R8, DOUBLE, dblVal)	// conflicts with VT_DATE/DATE
VARTYPE_TABLE(VT_BOOL, VARIANT_BOOL, boolVal)	// conflicts with VT_I2/SHORT
VARTYPE_TABLE_V2C(VT_ERROR, SCODE, scode)	// conflicts with VT_I4/LONG
VARTYPE_TABLE(VT_CY, CY, cyVal)
VARTYPE_TABLE_V2C(VT_DATE, DATE, date)	// conflicts with VT_R8/DOUBLE
VARTYPE_TABLE(VT_BSTR, BSTR, bstrVal)
VARTYPE_TABLE(VT_UNKNOWN, IUnknown*, punkVal)
VARTYPE_TABLE(VT_DISPATCH, IDispatch*, pdispVal)
VARTYPE_TABLE(VT_ARRAY, SAFEARRAY*, parray)
VARTYPE_TABLE(VT_BYREF, VOID*, byref)
VARTYPE_TABLE(VT_I1, CHAR, cVal)
VARTYPE_TABLE(VT_UI2, USHORT, uiVal)
VARTYPE_TABLE(VT_UI4, ULONG, ulVal)
VARTYPE_TABLE(VT_UI8, ULONGLONG, ullVal)
VARTYPE_TABLE(VT_INT, INT, intVal)
VARTYPE_TABLE(VT_UINT, UINT, uintVal)
MAP_TYPE(VT_VARIANT, VARIANT)
template<> inline VarTypeDescriminator<VT_VARIANT>::CppType&
VarTypeDescriminator<VT_VARIANT>::value(VARIANT& var) {return var;}

#undef MAP_TYPE
#undef DESCRIMINATE_TYPE
#undef VARTYPE_TABLE

/// wrapper for VARIANT and VARIANTARG
/// @see _variant_t class (comutil.h)
/*template<bool allowConversion>*/ class ComVariant : public tagVARIANT {
public:
	/// constructor
	ComVariant() {::VariantInit(this);}
	/// copy-constructor
	ComVariant(const ComVariant& rhs) {vt = VT_EMPTY; copy(rhs);}
	/// constructor
	explicit ComVariant(const tagVARIANT& value) {vt = VT_EMPTY; copy(value);}
	/// constructor
	template<class T>
	explicit ComVariant(const T&) {vt = CppType2VarType<T>::result; *VarTypeDescriminator<CppType2VarType<T>::result>(*this) = value;}
	/// constructor
	ComVariant(const OLECHAR* value) {vt = VT_EMPTY; *this = value;}
	/// constructor
	ComVariant(const BSTR value) {vt = VT_EMPTY; *this = value;}
	/// constructor
	ComVariant(bool value) {vt = VT_BOOL; boolVal = toVariantBoolean(value);}
	/// constructor
	ComVariant(const IDispatch& value) {
		vt = VT_DISPATCH;
		pdispVal = const_cast<IDispatch*>(&value);
		if(pdispVal != 0)
			pdispVal->AddRef();
	}
	/// constructor
	ComVariant(const IUnknown& value) {
		vt = VT_UNKNOWN;
		punkVal = const_cast<IUnknown*>(&value);
		if(punkVal != 0)
			punkVal->AddRef();
	}
	/// destructor
	virtual ~ComVariant() {clear();}
	///	attach raw VARIANT
	HRESULT attach(const VARIANT& var) {
		const HRESULT hr = clear();
		if(SUCCEEDED(hr)) {
			std::memcpy(this, &var, sizeof(VARIANT));
			vt = VT_EMPTY;
		}
		return hr;
	}
	///	change type of the value
	HRESULT changeType(VARTYPE type, const VARIANT* src = 0) {
		return ::VariantChangeType(this, (src == 0) ? this : const_cast<VARIANT*>(src), 0, type);}
	///	clear
	HRESULT clear() {return ::VariantClear(this);}
	///	copy from @a src
	HRESULT copy(const VARIANT& src) {
		HRESULT hr = ::VariantCopy(this, const_cast<VARIANT*>(&src));
		if(FAILED(hr))
			vt = VT_ERROR, scode = hr;
		return hr;
	}
	///	revoke attachment
	HRESULT detach(VARIANT& var) {
		const HRESULT hr = ::VariantClear(&var);
		if(SUCCEEDED(hr)) {
			std::memcpy(&var, this, sizeof(VARIANT));
			vt = VT_EMPTY;
		}
		return hr;
	}
//	HRESULT readFromStream(IStream& stream) {return E_NOTIMPL;}
//	HRESULT writeToStream(IStream& stream) {return E_NOTIMPL;}

	// ‰‰ŽZŽq
public:
	ComVariant& operator =(const ComVariant& rhs) {copy(rhs); return *this;}
	ComVariant& operator =(const tagVARIANT& rhs) {copy(rhs); return *this;}
	ComVariant& operator =(const OLECHAR* rhs) {
		clear();
		vt = VT_BSTR;
		bstrVal = ::SysAllocString(rhs);
		if(bstrVal == 0 && rhs != 0)
			vt = VT_ERROR, scode = E_OUTOFMEMORY;
		return *this;
	}
	ComVariant& operator =(const BSTR rhs) {
		clear();
		vt = VT_BSTR;
		bstrVal = ::SysAllocString(rhs);
		if(bstrVal == 0 && rhs != 0)
			vt = VT_ERROR, scode = E_OUTOFMEMORY;
		return *this;
	}
	ComVariant& operator =(bool rhs) {
		if(vt != VT_BOOL)
			clear(), vt = VT_BOOL;
		boolVal = toVariantBoolean(rhs);
		return *this;
	}
	template<class T> ComVariant& operator =(const T& rhs) {
		if(vt != CppType2VarType<T>::result)
			clear(), vt = CppType2VarType<T>::result;
		VarTypeDescriminator<CppType2VarType<T>::result>::value(*this) = rhs;
		return *this;
	}
	ComVariant& operator =(const IDispatch& rhs) {
		clear(), vt = VT_DISPATCH;
		pdispVal = const_cast<IDispatch*>(&rhs);
		if(pdispVal != 0)
			pdispVal->AddRef();
		return *this;
	}
	ComVariant& operator =(const IUnknown& rhs) {
		clear(), vt = VT_UNKNOWN;
		punkVal = const_cast<IUnknown*>(&rhs);
		if(punkVal != 0)
			punkVal->AddRef();
		return *this;
	}
	template<class T> operator T() const {
		if(vt == CppType2VarType<T>::result)
			return VarTypeDescriminator<CppType2VarType<T>::result>::value(*this);
		VARIANT temp;
		::VariantInit(&temp);
		::VariantChangeType(&temp, const_cast<ComVariant*>(this), 0, CppType2VarType<T>::result);
		return VarTypeDescriminator<CppType2VarType<T>::result>::value(temp);
	}
};

}}} // namespace manah::com::ole

#endif /* !MANAH_OLE_TYPE_WRAPPER_HPP */
