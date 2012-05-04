/**
 * @file com.hpp
 * @author exeal
 * @date 2002-2009 (was manah/com/common.hpp)
 * @date 2010-2012
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

#define ASCENSION_WIN32_VERIFY_COM_POINTER(p) if((p) == nullptr) return E_POINTER

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
				return internal::OlestrButNotBstr((p != nullptr) ? p : OLESTR(""));
			}

			/// Returns @c true if @a bstr is an empty string.
			inline bool isEmptyBSTR(const BSTR bstr) /*throw()*/ {
				return bstr == nullptr || *bstr == 0;
			}

			/// Converts the given C++ boolean into OLE VariantBool.
			inline VARIANT_BOOL toVariantBoolean(bool b) /*throw()*/ {
				return b ? VARIANT_TRUE : VARIANT_FALSE;
			}

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
