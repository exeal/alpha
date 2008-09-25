// common.hpp
// (c) 2002-2008 exeal

#ifndef MANAH_COM_COMMON_HPP
#define MANAH_COM_COMMON_HPP

#include "../object.hpp"
#include <objbase.h>
#include <objsafe.h>
#include <cassert>
#include <stdexcept>


namespace manah {
	namespace com {

#define MANAH_VERIFY_POINTER(p)	\
	if((p) == 0)				\
		return E_POINTER

		inline const BSTR safeBSTR(const BSTR bstr) throw() {return (bstr != 0) ? bstr : OLESTR("");}

		/// Returns true if @a bstr is an empty string.
		inline bool isEmptyBSTR(const BSTR bstr) throw() {return bstr == 0 || *bstr == 0;}

		/// Converts the given C++ boolean into OLE VariantBool.
		inline VARIANT_BOOL toVariantBoolean(bool b) throw() {return (b != 0) ? VARIANT_TRUE : VARIANT_FALSE;}

		/// A proxy returned by @c ComPtr#operator->.
		template<typename T> class ComPtrProxy : public T {
		private:
			STDMETHOD_(ULONG, AddRef)() = 0;
			STDMETHOD_(ULONG, Release)() = 0;
			T** operator &() const throw();	// prohibits &*p
		};

		/**
		 * COM smart pointer.
		 * @param T the interface type
		 */
		template<typename T>
		class ComPtr {
		public:
			/// Interface type.
			typedef T Interface;
			/// Constructor.
			explicit ComPtr(Interface* p = 0) throw() : pointee_(p) {if(pointee_ != 0) pointee_->AddRef();}
			/// Copy-constructor.
			ComPtr(const ComPtr<Interface>& rhs) throw() : pointee_(rhs.pointee_) {if(pointee_ != 0) pointee_->AddRef();}
			/// Destructor.
			virtual ~ComPtr() throw() {if(pointee_ != 0) pointee_->Release();}
			/// Initializes with @c CoCreateInstance.
#ifdef _MSC_VER
			HRESULT createInstance(REFCLSID clsid, REFIID riid = __uuidof(Interface), DWORD clsContext = CLSCTX_ALL, IUnknown* unkOuter = 0) {
#else
			HRESULT createInstance(REFCLSID clsid, REFIID riid, DWORD clsContext = CLSCTX_ALL, IUnknown* unkOuter = 0) {
#endif
				assert(get() == 0); return ::CoCreateInstance(clsid, unkOuter, clsContext, riid, reinterpret_cast<void**>(&pointee_));}
			/// Returns the raw pointer.
			ComPtrProxy<Interface>* get() const throw() {return static_cast<ComPtrProxy<Interface>*>(pointee_);}
			/// Returns the output pointer for initialization.
			Interface** initialize() throw() {release(); return &pointee_;}
			/// Returns true if the pointer addresses the same object.
			bool isEqualObject(IUnknown* p) const throw() {
				if(pointee_ == 0 && p == 0) return true;
				else if(pointee_ == 0 || p == 0) return false;
				ComPtr<IUnknown> ps[2];
				pointee_->QueryInterface(IID_IUnknown, ps[0].initialize());
				p->QueryInterface(IID_IUnknown, ps[1].initialize());
				return ps[1].get() == ps[2].get();
			}
			/// Releases the pointer.
			void release() throw() {reset(0);}
			/// Resets the pointer.
			void reset(Interface* p = 0) throw() {if(pointee_ != 0) pointee_->Release(); pointee_ = p; if(p != 0) pointee_->AddRef();}
			/// Addressing operator for initialization.
			Interface** operator&() throw() {return initialize();}
			/// Member-access operator.
			ComPtrProxy<Interface>* operator->() const throw() {assert(get() != 0); return get();}
			/// Dereference operator.
			ComPtrProxy<Interface>& operator*() const throw() {assert(get() != 0); return *get();}
			/// Assignment operator.
			ComPtr<Interface>& operator=(Interface* rhs) throw() {reset(rhs); return *this;}
			/// Equality operator.
			bool operator==(const Interface* rhs) const throw() {return pointee_ == rhs;}
			/// Inequality operator.
			bool operator!=(const Interface* rhs) const throw() {return !(pointee_ == rhs);}
			/// Operator converts into a boolean.
			operator bool() const throw() {return get() != 0;}
		private:
			Interface* pointee_;
		};

		/**
		 * COM smart pointer for initialization with @c IUnknown#QueryInterface.
		 * @param T the interface type
		 * @param iid the IID of the interface
		 */
#ifdef _MSC_VER
		template<typename T, const IID* iid = &__uuidof(T)>
#else
		template<typename T, const IID* iid>
#endif
		class ComQIPtr : public ComPtr<T> {
		public:
			/// Interface type.
			typedef typename ComPtr<T>::Interface Interface;
			/// Constructor.
			explicit ComQIPtr(Interface* p = 0) : ComPtr<Interface>(p) {}
			/// Copy-constructor.
			ComQIPtr(const ComQIPtr<Interface, iid>& rhs) : ComPtr<Interface>(rhs.get()) {}
			/// Returns the output pointer for initialization.
			void** initialize() throw() {ComPtr<T>::release(); return reinterpret_cast<void**>(ComPtr<Interface>::operator &());}
			/// Addressing operator for QI initialization.
			void** operator &() throw() {return initialize();}
		};


		/// A wrapper for treatment an @c IErrorInfo as a C++ exception (from Essential COM (Don Box)).
		class ComException {
		public:
			/**
			 * Constructor.
			 * @param scode the SCODE
			 * @param riid the IID
			 * @param source the class thrown this exception
			 * @param description the description of the exception. if @c null, retrieved by @a scode
			 * @param helpFile the help file
			 * @param helpContext the number of the help topic
			 */
			ComException(HRESULT scode, REFIID riid,
					const OLECHAR* source, const OLECHAR* description = 0, const OLECHAR* helpFile = 0, DWORD helpContext = 0) {
				ICreateErrorInfo* pcei = 0;

				assert(FAILED(scode));

				HRESULT hr = ::CreateErrorInfo(&pcei);
				assert(SUCCEEDED(hr));

				hr = pcei->SetGUID(riid);
				assert(SUCCEEDED(hr));
				if(source != 0) {
					hr = pcei->SetSource(const_cast<OLECHAR*>(source));
					assert(SUCCEEDED(hr));
				}
				if(description != 0) {
					hr = pcei->SetDescription(const_cast<OLECHAR*>(description));
					assert(SUCCEEDED(hr));
				} else {
					BSTR bstrDescription = 0;
					ComException::getDescriptionOfSCode(scode, bstrDescription);
					hr = pcei->SetDescription(bstrDescription);
					::SysFreeString(bstrDescription);
					assert(SUCCEEDED(hr));
				}
				if(helpFile != 0) {
					hr = pcei->SetHelpFile(const_cast<OLECHAR*>(helpFile));
					assert(SUCCEEDED(hr));
				}
				hr = pcei->SetHelpContext(helpContext);
				assert(SUCCEEDED(hr));

				hr_ = scode;
				hr = pcei->QueryInterface(IID_IErrorInfo, reinterpret_cast<void**>(&errorInfo_));
				assert(SUCCEEDED(hr));
				pcei->Release();
			}
			/// Destructor.
			virtual ~ComException() {if(errorInfo_ != 0) errorInfo_->Release();}
			/// Returns an @c IErrorInfo interface.
			void getErrorInfo(IErrorInfo*& errorInfo) const {errorInfo = errorInfo_; errorInfo->AddRef();}
			/// Returns the @c HRESULT value.
			HRESULT getSCode() const throw() {return hr_;}
			/// Throws the exception object as a logical thread exception.
			void throwLogicalThreadError() {::SetErrorInfo(0, errorInfo_);}
			/**
			 * Returns an error message corresponding to the given @c HRESULT.
			 * @param[in] hr the HRESULT value
			 * @param[out] description the error message
			 * @param[in] language the language identifier
			 */
			static void getDescriptionOfSCode(HRESULT hr, BSTR& description, DWORD language = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)) {
				void* buffer = 0;
				::FormatMessageW(
					FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					0, hr, language, reinterpret_cast<wchar_t*>(&buffer), 0, 0);
				description = ::SysAllocString(reinterpret_cast<OLECHAR*>(buffer));
				::LocalFree(buffer);
			}

		private:
			HRESULT hr_;
			IErrorInfo* errorInfo_;
		};


		/**
		 * A critical section.
		 * @param automatic set true to initialize/terminate the critical section automatically
		 * when the instance is created/deleted.
		 */
		template<bool automatic = true> class ComCriticalSection {
		public:
			/// Default constructor.
			ComCriticalSection() {if(automatic) if(FAILED(doInitialize())) throw std::runtime_error("Failed to initialize critical section!");}
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
			ComCriticalSection& operator =(const ComCriticalSection&);
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
		 * Standard implementation of @c ISupportErrorInfo interface.
		 * @note This supports only one interface.
		 */
		template<const IID* iid> class ISupportErrorInfoImpl : virtual public ISupportErrorInfo {
		public:
			virtual ~ISupportErrorInfoImpl() {}
			STDMETHODIMP InterfaceSupportsErrorInfo(REFIID riid) {return (riid == *iid) ? S_OK : S_FALSE;}
		};


		/**
		 * Simple implementation of @c IObjectSafety interface.
		 * @note This supports only one interface.
		 * @param supportedSafety the options to support
		 * @param initialSafety the initial value
		 */
		template<DWORD supportedSafety, DWORD initialSafety> class IObjectSafetyImpl : virtual public IObjectSafety {
		public:
			IObjectSafetyImpl() : enabledSafety_(supportedSafety & initialSafety) {}
			virtual ~IObjectSafetyImpl() {}
		public:
			STDMETHODIMP GetInterfaceSafetyOptions(REFIID riid, DWORD* pdwSupportedOptions, DWORD* pdwEnabledOptions) {
				MANAH_VERIFY_POINTER(pdwSupportedOptions);
				MANAH_VERIFY_POINTER(pdwEnabledOptions);
				ComQIPtr<IUnknown, &IID_IUnknown> p;
				if(SUCCEEDED(QueryInterface(riid, &p))) {
					*pdwSupportedOptions = supportedSafety;
					*pdwEnabledOptions = enabledSafety_;
					return S_OK;
				} else {
					*pdwSupportedOptions = *pdwEnabledOptions = 0;
					return E_NOINTERFACE;
				}
			}
			STDMETHODIMP SetInterfaceSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions) {
				ComQIPtr<IUnknown, &IID_IUnknown> p;
				if(FAILED(QueryInterface(riid, &p)))
					return E_NOINTERFACE;
				else if(toBoolean(dwOptionSetMask & ~supportedSafety))
					return E_FAIL;
				enabledSafety_ = (enabledSafety_ & ~dwOptionSetMask) | (dwOptionSetMask & dwEnabledOptions);
				return S_OK;
			}
		protected:
			DWORD getSafetyOptions() const throw() {return enabledSafety_;}
			void setSafetyOptions(DWORD options) throw() {enabledSafety_ = (options & supportedSafety);}
		private:
			DWORD enabledSafety_;
		};

}} // namespace manah.com

#endif // !MANAH_COM_COMMON_HPP
