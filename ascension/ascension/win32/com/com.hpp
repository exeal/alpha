/**
 * @file com.hpp
 * @author exeal
 * @date 2002-2009 (was manah/com/common.hpp)
 * @date 2010
 */

#ifndef ASCENSION_WIN32_COM_HPP
#define ASCENSION_WIN32_COM_HPP

//#include "../object.hpp"
#include <objbase.h>
#include <objsafe.h>
#include <cassert>
#include <stdexcept>


namespace ascension {
	namespace win32 {
		namespace com {

#define ASCENSION_WIN32_VERIFY_COM_POINTER(p) if((p) == 0) return E_POINTER

			namespace internal {
				class OlestrButNotBstr {
				public:
					explicit OlestrButNotBstr(const OLECHAR* s) : p_(s) {}
					operator const OLECHAR*() const {return p_;}
				private:
					operator BSTR() const;
					const OLECHAR* const p_;
				};
			} // namespace internal

			/**
			 * Converts a @c const @c BSTR into @c const @c OLECHAR*. If @a p is @c null, returns
			 * OLESTR("").
			 */
			inline internal::OlestrButNotBstr safeBSTRtoOLESTR(const BSTR p) /*throw()*/ {
				return internal::OlestrButNotBstr((p != 0) ? p : OLESTR(""));
			}

			/// Returns @c true if @a bstr is an empty string.
			inline bool isEmptyBSTR(const BSTR bstr) /*throw()*/ {
				return bstr == 0 || *bstr == 0;
			}

			/// Converts the given C++ boolean into OLE VariantBool.
			inline VARIANT_BOOL toVariantBoolean(bool b) /*throw()*/ {
				return (b != 0) ? VARIANT_TRUE : VARIANT_FALSE;
			}

			/// A proxy returned by @c ComPtr#operator->.
			template<typename T> class ComPtrProxy : public T {
			private:
				STDMETHOD_(ULONG, AddRef)() = 0;
				STDMETHOD_(ULONG, Release)() = 0;
				T** operator &() const /*throw()*/;	// prohibits &*p
			};

			/**
			 * COM smart pointer.
			 * @tparam T the interface type
			 */
			template<typename T> class ComPtr {
			public:
				/// Interface type.
				typedef T Interface;
				/// Constructor.
				explicit ComPtr(Interface* p = 0) /*throw()*/ : pointee_(p) {
					if(pointee_ != 0)
						pointee_->AddRef();
				}
				/// Constructor creates instance by using @c CoCreateInstance.
				ComPtr<Interface>(REFCLSID clsid, REFIID iid /* = __uuidof(Interface) */,
						DWORD context = CLSCTX_ALL, IUnknown* outer = 0, HRESULT* hr = 0) : pointee_(0) {
					const HRESULT r = ::CoCreateInstance(clsid, outer, context, iid, initializePPV());
					if(hr != 0)
						*hr = r;
				}
				/// Copy-constructor.
				ComPtr(const ComPtr<Interface>& other) /*throw()*/ : pointee_(other.pointee_) {
					if(pointee_ != 0)
						pointee_->AddRef();
				}
				/// Assignment operator.
				ComPtr<Interface>& operator=(const ComPtr<Interface>& other) /*throw()*/ {
					reset(other.get());
					return *this;
				}
				/// Destructor.
				virtual ~ComPtr() /*throw()*/ {
					if(pointee_ != 0)
						pointee_->Release();
				}
				/// Returns the raw pointer.
				ComPtrProxy<Interface>* get() const /*throw()*/ {
					return static_cast<ComPtrProxy<Interface>*>(pointee_);
				}
				/// Returns the output pointer for initialization.
				Interface** initialize() /*throw()*/ {reset(); return &pointee_;}
				/// Returns the output pointer for initialization with type void**.
				void** initializePPV() /*throw*/ {return reinterpret_cast<void**>(initialize());}
				/// Returns true if the pointer addresses the same object.
				bool equals(IUnknown* p) const /*throw()*/ {
					if(pointee_ == 0 && p == 0)
						return true;
					else if(pointee_ == 0 || p == 0)
						return false;
					ComPtr<IUnknown> ps[2];
					pointee_->QueryInterface(IID_IUnknown, ps[0].initialize());
					p->QueryInterface(IID_IUnknown, ps[1].initialize());
					return ps[1].get() == ps[2].get();
				}
				/// Resets the pointer.
				void reset(Interface* p = 0) /*throw()*/ {
					if(pointee_ != 0)
						pointee_->Release();
					pointee_ = p;
					if(p != 0)
						pointee_->AddRef();
				}
				/// Member-access operator.
				ComPtrProxy<Interface>* operator->() const /*throw()*/ {assert(get() != 0); return get();}
				/// Dereference operator.
				ComPtrProxy<Interface>& operator*() const /*throw()*/ {assert(get() != 0); return *get();}
				/// Equality operator.
				bool operator==(const Interface* rhs) const /*throw()*/ {return pointee_ == rhs;}
				/// Inequality operator.
				bool operator!=(const Interface* rhs) const /*throw()*/ {return !(pointee_ == rhs);}
			private:
				ComPtr<Interface>* operator&() const;
				Interface* pointee_;
			};

			template<typename T, const IID* iid /* = &__uuidof(T) */>
			class ComQIPtr : public ComPtr<T> {
			public:
				typedef typename ComPtr<T>::Interface Interface;
			public:
				/// Constructor.
				explicit ComQIPtr(Interface* p = 0) /*throw()*/ : ComPtr<Interface>(p) {}
				/// Constructor takes IUnknown pointer.
				ComQIPtr(IUnknown* p) /*throw()*/ {if(p != 0) p->QueryInterface(*iid, initializePPV());}
				/// Constructor takes other typed ComPtr.
				template<typename Other>
				ComQIPtr(const ComPtr<Other>& other) /*throw()*/ {
					if(other.get() != 0)
						other->QueryInterface(*iid, initializePPV());
				}
				/// Copy-constructor.
				ComQIPtr(const ComQIPtr<Interface, iid>& other) /*throw()*/ : ComPtr<Interface>(other) {}
				/// Assignment operator.
				ComQIPtr<Interface, iid>& operator=(IUnknown* other) /*throw()*/ {
					ComPtr<Interface> temp;
					if(other != 0)
						other->QueryInterface(*iid, temp.initializePPV());
					ComPtr<Interface>::operator=(temp);
					return *this;
				}
				/// Assignment operator.
				template<typename Other> ComQIPtr<Interface, iid>& operator=(const ComPtr<Other>& rhs) /*throw()*/ {return operator=(rhs.get());}
				/// Assignment operator.
				ComQIPtr<Interface, iid>& operator=(const ComQIPtr<Interface, iid>& rhs) /*throw()*/ {return ComPtr<Interface>::operator=(rhs);}
			};


			/**
			 * A critical section.
			 * @tparam automatic set true to initialize/terminate the critical section automatically
			 * when the instance is created/deleted.
			 */
			template<bool automatic = true> class ComCriticalSection {
			public:
				/// Default constructor.
				ComCriticalSection() {
					if(automatic && FAILED(doInitialize()))
						throw std::runtime_error("Failed to initialize critical section!");
				}
				/// Destructor.
				~ComCriticalSection() {if(automatic) doTerminate();}
				/// Enters the critical section.
				void lock() {::EnterCriticalSection(&cs_);}
				/// Leaves the critical section.
				void unlock() {::LeaveCriticalSection(&cs_);}
				/// Initializes the critical section.
				HRESULT initialize();
				/// Terminates the critical section.
				void terminate();

			private:
				ComCriticalSection(const ComCriticalSection&);
				ComCriticalSection& operator=(const ComCriticalSection&);
				HRESULT doInitialize() {
#ifdef _MSC_VER
					__try {
#endif // _MSC_VER
						::InitializeCriticalSection(&cs_);
#ifdef _MSC_VER
					} __except(EXCEPTION_EXECUTE_HANDLER) {
						return (STATUS_NO_MEMORY == ::GetExceptionCode()) ? E_OUTOFMEMORY : E_FAIL;
					}
#endif // _MSC_VER
					return S_OK;
				}
				void doTerminate() {::DeleteCriticalSection(&cs_);}
			private:
				CRITICAL_SECTION cs_;
			};

			template<> inline HRESULT ComCriticalSection<false>::initialize() {return doInitialize();}

			template<> inline void ComCriticalSection<false>::terminate() {doTerminate();}


			/**
			 * Simple implementation of @c IObjectSafety interface.
			 * @tparam supportedSafety the options to support
			 * @tparam initialSafety the initial value
			 * @tparam Base the base type inherits @c IObjectSafety
			 */
			template<DWORD supportedSafety, DWORD initialSafety, typename Base> class IObjectSafetyImpl : public Base {
			public:
				IObjectSafetyImpl() : enabledSafety_(supportedSafety & initialSafety) {}
				virtual ~IObjectSafetyImpl() throw() {}
			public:
				STDMETHODIMP GetInterfaceSafetyOptions(REFIID iid, DWORD* supportedOptions, DWORD* enabledOptions) {
					MANAH_VERIFY_POINTER(supportedOptions);
					MANAH_VERIFY_POINTER(enabledOptions);
					ComPtr<IUnknown> p;
					if(SUCCEEDED(QueryInterface(iid, reinterpret_cast<void**>(p.initialize())))) {
						*supportedOptions = supportedSafety;
						*enabledOptions = enabledSafety_;
						return S_OK;
					} else {
						*supportedOptions = *enabledOptions = 0;
						return E_NOINTERFACE;
					}
				}
				STDMETHODIMP SetInterfaceSafetyOptions(REFIID iid, DWORD optionSetMask, DWORD enabledOptions) {
					ComPtr<IUnknown> p;
					if(FAILED(QueryInterface(iid, reinterpret_cast<void**>(p.initialize()))))
						return E_NOINTERFACE;
					else if(toBoolean(optionSetMask & ~supportedSafety))
						return E_FAIL;
					enabledSafety_ = (enabledSafety_ & ~optionSetMask) | (optionSetMask & enabledOptions);
					return S_OK;
				}
			protected:
				DWORD safetyOptions() const /*throw()*/ {return enabledSafety_;}
				void setSafetyOptions(DWORD options) /*throw()*/ {enabledSafety_ = (options & supportedSafety);}
			private:
				DWORD enabledSafety_;
			};

		}
	}
} // namespace ascension.win32.com

#endif // !ASCENSION_WIN32_COM_HPP
