// unknown-impl.hpp
// (c) 2002-2007 exeal

#ifndef MANAH_UNKNOWN_IMPL_HPP
#define MANAH_UNKNOWN_IMPL_HPP
#include "common.hpp"

/**
 *	@file unknown-impl.hpp
 *	IUnknown の3つのメソッドを実装するマクロを定義
 */

namespace manah {
namespace com {

template<bool multiThreaded>
class ReferenceCounter {
public:
	ReferenceCounter() throw() : count_(0) {}
	long get() const throw() {return count_;}
	long increment();
	long decrement();
private:
	long count_;
};

template<> inline long ReferenceCounter<true>::decrement() {return ::InterlockedDecrement(&count_);}
template<> inline long ReferenceCounter<true>::increment() {return ::InterlockedIncrement(&count_);}
template<> inline long ReferenceCounter<false>::decrement() {return --count_;}
template<> inline long ReferenceCounter<false>::increment() {return ++count_;}

/// 参照カウントを使わない実装 (非ヒープオブジェクト)
#define IMPLEMENT_UNKNOWN_NO_REF_COUNT()		\
	STDMETHODIMP_(ULONG) AddRef() {return 2;}	\
	STDMETHODIMP_(ULONG) Release() {return 1;}

/// 参照カウントの増減がスレッドセーフでない実装
#define IMPLEMENT_UNKNOWN_SINGLE_THREADED()		\
private:										\
	manah::com::ReferenceCounter<false> rc_;	\
public:											\
	STDMETHODIMP_(ULONG) AddRef() {				\
		return rc_.increment();					\
	}											\
	STDMETHODIMP_(ULONG) Release() {			\
		if(rc_.decrement() == 0) {				\
			delete this;						\
			return 0;							\
		}										\
		return rc_.get();						\
	}

/// 参照カウントの増減がスレッドセーフな実装
#define IMPLEMENT_UNKNOWN_MULTI_THREADED()		\
private:										\
	manah::com::ReferenceCounter<true>	rc_;	\
public:											\
	STDMETHODIMP_(ULONG) AddRef() {				\
		return rc_.increment();					\
	}											\
	STDMETHODIMP_(ULONG) Release() {			\
		if(rc_.decrement() == 0) {				\
			delete this;						\
			return 0;							\
		}										\
		return rc_.get();						\
	}

#define BEGIN_INTERFACE_TABLE()								\
	STDMETHODIMP QueryInterface(REFIID riid, void** ppv) {	\
		VERIFY_POINTER(ppv);

#define IMPLEMENTS_LEFTMOST_INTERFACE(InterfaceName)			\
		if(riid == IID_##InterfaceName || riid == IID_IUnknown)	\
			*ppv = static_cast<InterfaceName*>(this);

#define IMPLEMENTS_INTERFACE(InterfaceName)		\
		else if(riid == IID_##InterfaceName)	\
			*ppv= static_cast<InterfaceName*>(this);

#define END_INTERFACE_TABLE()							\
		else											\
			return (*ppv = 0), E_NOINTERFACE;			\
		reinterpret_cast<IUnknown*>(*ppv)->AddRef();	\
		return S_OK;									\
	}

}} // namespace manah::com

#endif /* !MANAH_UNKNOWN_IMPL_HPP */
