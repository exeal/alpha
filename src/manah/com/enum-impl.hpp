// enum-impl.hpp
// (c) 2005-2007 exeal

#ifndef MANAH_ENUM_IMPL_HPP
#define MANAH_ENUM_IMPL_HPP

#include "unknown-impl.hpp"


namespace manah {
namespace com {

// IEnumXXXX::Clone の実装ポリシー ////////////////////////////////////////

///	IEnumXXXX::Clone を実装する
struct AllowCloning {enum {allow = true};};
///	IEnumXXXX::Clone を実装しない
struct DisallowCloning {enum {allow = false};};


/**
 *	IEnumXXXX の実装。列挙要素が構造体へのポインタであるケースは未サポート
 *	@param T				列挙要素の型
 *	@param IEnum			列挙インターフェイス
 *	@param CloningPolicy	IEnumXXXX::Clone を実装するか
 */
template<class T, class IEnum, class CloningPolicy = DisallowCloning>
class IEnumImpl : virtual public IEnum {
private:
	typedef char Small;
	class Big {char dummy[2];};
	static Small test(IUnknown*);
	static Big test(...);
	static T makeT();
	enum {IS_INTERFACE_POINTER = sizeof(test(makeT())) == sizeof(Small)};
	template<typename U0, typename U1> struct isSameType {enum {res = false};};
	template<typename U0> struct isSameType<U0, U0> {enum {res = true};};
	enum {
		isPrimitive = 0,
		isOLESTR = 1,
		isBSTR = 2,
		isVARIANT = 3,
		isInterfacePointer = 4,
		typeSpec = (isSameType<T, OLECHAR*>::res ? isOLESTR : isPrimitive)
			| (isSameType<T, BSTR>::res ? isBSTR : isPrimitive)
			| (isSameType<T, VARIANT>::res ? isVARIANT : isPrimitive)
			| (IS_INTERFACE_POINTER ? isInterfacePointer : isPrimitive)
	};
	template<int TypeSpec> struct Copier;
	template<> struct Copier<0/*isPrimitive*/> {
		static void copy(T& lhs, const T& rhs) {lhs = rhs;}
		static void release(T& v) {}
	};
	template<> struct Copier<1/*isOLESTR*/> {
		static void copy(T& lhs, const T& rhs) {
			lhs = ::CoTaskMemAlloc(sizeof(OLECHAR) * (wcslen(rhs) + 1));
			wcscpy(lhs, rhs);
		}
		static void release(T& v) {::CoTaskMemFree(v);}
	};
	template<> struct Copier<2/*isBSTR*/> {
		static void copy(T& lhs, const T& rhs) {lhs = ::SysAllocString(rhs);}
		static void release(T& v) {::SysFreeString(v);}
	};
	template<> struct Copier<3/*isVARIANT*/> {
		static void copy(T& lhs, const T& rhs) {
			::VariantInit(&lhs);
			::VariantCopy(&lhs, const_cast<VARIANT*>(&rhs));
		}
		static void release(T& v) {::VariantClear(&v);}
	};
	template<> struct Copier<4/*isInterfacePointer*/> {
		static void copy(T& lhs, const T& rhs) {lhs = rhs; lhs->AddRef();}
		static void release(T& v) {v->Release();}
	};

private:
	IEnumImpl();
	IEnumImpl(const IEnumImpl&);
	operator =(const IEnumImpl&);
public:
	/**
	 *	コンストラクタ
	 *	@param first, last	要素のイテレータ
	 */
	template<class ForwardIterator> IEnumImpl(ForwardIterator first, ForwardIterator last) : cursor_(0) {
		while(first < last) {
			T temp;
			Copier<typeSpec>::copy(temp, *(first++));
			elements_.push_back(temp);
		}
	}
	///	デストラクタ
	virtual ~IEnumImpl() {
//		std::for_each(elements_.begin(), elements_.end(), &Copier::Release);
		for(std::size_t i = 0; i < elements_.size(); ++i)
			Copier<typeSpec>::release(elements_[i]);
	}
public:
	IMPLEMENT_UNKNOWN_MULTI_THREADED()
	///	@see	IUnknown::QueryInterface
	STDMETHODIMP QueryInterface(REFIID riid, void** ppv) {
		VERIFY_POINTER(ppv);
		if(riid == __uuidof(IEnum) || riid == IID_IUnknown)
			*ppv = static_cast<IEnum*>(this);
		else
			return (*ppv = 0), E_NOINTERFACE;
		reinterpret_cast<IUnknown*>(*ppv)->AddRef();
		return S_OK;
	}
	///	@see	IEnumXXXX::Next
	STDMETHODIMP Next(unsigned long celt, T* rgelt, unsigned long* pcFetched) {
		if(celt == 0)
			return S_OK;
		else if(rgelt == 0 || (celt != 1 && pcFetched == 0))
			return E_INVALIDARG;
		while(celt-- != 0) {
			if(cursor_ == elements_.size())
				return S_FALSE;
			Copier<typeSpec>::copy(*(rgelt++), elements_[cursor_++]);
			++rgelt;
		}
		return (celt + 1 == 0) ? S_OK : S_FALSE;
	}
	///	@see	IEnumXXXX::Skip
	STDMETHODIMP Skip(unsigned long celt) {
		while(celt-- != 0) {
			if(cursor_ == elements_.size())
				return S_FALSE;
			++cursor_;
		}
		return (celt + 1 == 0) ? S_OK : S_FALSE;
	}
	///	@see	IEnumXXXX::Reset
	STDMETHODIMP Reset() {
		cursor_ = 0;
		return S_OK;
	}
	///	@see	IEnumXXXX::Clone
	STDMETHODIMP Clone(IEnum** ppEnum) {
		VERIFY_POINTER(ppEnum);

		if(!CloningPolicy::allow)
			return E_NOTIMPL;

		IEnumImpl* clone = new IEnumImpl<T, IEnum, CloningPolicy>(elements_.begin(), elements_.end());
		if(clone == 0)
			return E_OUTOFMEMORY;
		clone->cursor_ = cursor_;
		(*ppEnum)->AddRef();
		return S_OK;
	}
private:
	std::vector<T>	elements_;
	std::size_t		cursor_;
};

///	IEnumString の実装
typedef IEnumImpl<OLECHAR*, IEnumString> IEnumStringImpl;
///	IEnumVARIANT の実装
typedef IEnumImpl<VARIANT, IEnumVARIANT> IEnumVARIANTImpl;

}} // namespace manah::com

#endif /* !MANAH_ENUM_IMPL_HPP */
