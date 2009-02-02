/**
 * @file ambient.hpp
 */

#ifndef ALPHA_AMBIENT_HPP
#define ALPHA_AMBIENT_HPP
#include <objbase.h>		// interface keyword
#include "ambient-idl.hpp"
#include <manah/memory.hpp>	// manah.AutoBuffer
#include <manah/com/exception.hpp>
#include <manah/com/unknown-impl.hpp>
#include <manah/com/dispatch-impl.hpp>
#include <activscp.h>
#include <map>

namespace alpha {
	namespace ambient {

		class AutoVARIANT : public tagVARIANT {
			MANAH_UNASSIGNABLE_TAG(AutoVARIANT);
		public:
			AutoVARIANT() /*throw()*/ {::VariantInit(this);}
			AutoVARIANT(const AutoVARIANT& other) {::VariantInit(this); ::VariantCopy(this, &other);}
			~AutoVARIANT() throw() {::VariantClear(this);}
			HRESULT coerce(VARTYPE type, USHORT flags) /*throw()*/ {
				AutoVARIANT temp; const HRESULT hr = ::VariantChangeType(&temp, this, flags, type);
				if(FAILED(hr)) return hr; swap(temp); return S_OK;}
			HRESULT coerce(VARTYPE type, USHORT flags, LCID lcid) /*throw()*/ {
				AutoVARIANT temp; const HRESULT hr = ::VariantChangeTypeEx(&temp, this, lcid, flags, type);
				if(FAILED(hr)) return hr; swap(temp); return S_OK;}
			void swap(AutoVARIANT& other) /*throw()*/ {std::swap(static_cast<tagVARIANT&>(*this), static_cast<tagVARIANT&>(other));}
		};

		template<typename InterfaceSignatures, typename TypeInformationProvider, typename ThreadingPolicy = MultiThreaded>
		class IUnknownDispatchImpl : public manah::com::ole::IDispatchImpl<
			manah::com::IUnknownImpl<
				manah::typelist::Cat<MANAH_INTERFACE_SIGNATURE(IDispatch), InterfaceSignatures>, ThreadingPolicy
			>, TypeInformationProvider
		> {};
		
		template<const IID* iid, typename InterfaceSignatures, typename ThreadingPolicy = MultiThreaded>
		class AutomationObject : public IUnknownDispatchImpl<
			InterfaceSignatures, manah::com::ole::TypeInformationFromExecutable<iid>, ThreadingPolicy> {};

		template<typename Interface, const IID* iid /* = __uuidof(Interface) */, typename ThreadingPolicy = manah::com::MultiThreaded>
		class SingleAutomationObject : public AutomationObject<
			iid, manah::typelist::Cat<
				manah::com::InterfaceSignature<Interface, iid>
			>, ThreadingPolicy
		> {};

		template<typename Implementation, typename Base>
		class AutomationProxy : public Base {
		protected:
			explicit AutomationProxy(Implementation& impl) /*throw()*/ : impl_(&impl) {}
			bool check() const /*throw()*/ {return impl_ != 0;}
			void discard() /*throw()*/ {return impl_ = 0;}
			Implementation& impl() {return *impl_;}
			const Implementation& impl() const {return impl_;}
		private:
			Implementation* impl_;
		};

#define AMBIENT_CHECK_PROXY() if(!check()) return CO_E_OBJNOTCONNECTED

		class IEnumVARIANTStaticImpl : public manah::com::IUnknownImpl<manah::typelist::Cat<MANAH_INTERFACE_SIGNATURE(IEnumVARIANT)> > {
			MANAH_UNASSIGNABLE_TAG(IEnumVARIANTStaticImpl);
		public:
			IEnumVARIANTStaticImpl(manah::AutoBuffer<VARIANT> array, std::size_t length);
			~IEnumVARIANTStaticImpl() throw();
			// IEnumVARIANT
			STDMETHODIMP Next(ULONG numberOfElements, VARIANT* values, ULONG* numberOfFetchedElements);
			STDMETHODIMP Skip(ULONG numberOfElements);
			STDMETHODIMP Reset();
			STDMETHODIMP Clone(IEnumVARIANT** enumerator);
		private:
			IEnumVARIANTStaticImpl(const IEnumVARIANTStaticImpl& rhs);
			struct SharedData {
				std::size_t refs;
				VARIANT* array;
				std::size_t length;
			} * data_;
			VARIANT* current_;
		};

		class ScriptSystem : public SingleAutomationObject<IScriptSystem, &IID_IScriptSystem, manah::com::NoReferenceCounting> {
			MANAH_NONCOPYABLE_TAG(ScriptSystem);
		public:
			ScriptSystem();
			~ScriptSystem() throw();
			// IScriptSystem
			STDMETHODIMP get_ActiveBuffer(IBuffer** activeBuffer);
			STDMETHODIMP get_ActiveWindow(IWindow** activeWindow);
			STDMETHODIMP get_Buffers(IBufferList** buffers);
			STDMETHODIMP get_Windows(IWindowList** windows);
			STDMETHODIMP ExecuteFile(BSTR fileName, VARIANT **result);
			STDMETHODIMP GetServiceProvider(IServiceObjectProvider **serviceProvider);
			STDMETHODIMP LoadConstants(VARIANT *libraryNameOrObject, VARIANT* parent);
			STDMETHODIMP LoadScript(BSTR fileName, VARIANT *result);
			STDMETHODIMP Position(SAFEARRAY* parameters, IPosition** newInstance);
			STDMETHODIMP Region(SAFEARRAY* parameters, IRegion** newInstance);
		private:
			HRESULT executeFile(BSTR fileName, VARIANT** result, bool onlyRequire, const OLECHAR* sourceName);
			HRESULT loadConstantsFromTypeLibrary(ITypeLib& typeLibrary, const GUID& guid, IDispatchEx* parent);
		private:
			class ScriptHost;
			ScriptHost* scriptHost_;
		};

		class ServiceProvider :
			public SingleAutomationObject<IServiceObjectProvider, &IID_IServiceObjectProvider, manah::com::NoReferenceCounting> {
		public:
			static ServiceProvider& instance();
			void registerService(const std::basic_string<OLECHAR>& name, IDispatch& object);
			// IServiceObjectProvider
			STDMETHODIMP QueryService(BSTR serviceName, IDispatch **serviceObject);
		private:
			ServiceProvider();
			~ServiceProvider() throw();
		private:
			std::map<std::basic_string<OLECHAR>, IDispatch*> serviceObjects_;
		};
	}
}

#endif // !ALPHA_AMBIENT_HPP
