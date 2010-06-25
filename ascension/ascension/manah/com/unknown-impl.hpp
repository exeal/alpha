// unknown-impl.hpp
// (c) 2002-2009 exeal

#ifndef MANAH_UNKNOWN_IMPL_HPP
#define MANAH_UNKNOWN_IMPL_HPP
#include "common.hpp"
#include "../type-list.hpp"

/**
 * @file unknown-impl.hpp
 * Provides macros implement three methods of @c IUnknown interface.
 */

namespace manah {
	namespace com {

		// Type list and interface list /////////////////////////////////////

		/// A pair of a interface type and its GUID.
		template<typename InterfaceType, const IID* iid> struct InterfaceSignature {
			typedef InterfaceType Interface;
		};

		/// Generates @c InterfaceSignature code.
		#define MANAH_INTERFACE_SIGNATURE(name) manah::com::InterfaceSignature<name, &IID_##name> 

		/// Generates the type list cosists of interface type from the given type list consists of
		/// @c InterfaceSignature.
		template<typename InterfaceList> struct InterfacesFromSignatures;
		template<typename I, const IID* iid, typename Cdr> struct InterfacesFromSignatures<typelist::Cat<InterfaceSignature<I, iid>, Cdr> > {
			typedef typelist::Cat<I, typename InterfacesFromSignatures<Cdr>::Result> Result;
		};
		template<> struct InterfacesFromSignatures<void> {typedef void Result;};

		// Interface hierarchy //////////////////////////////////////////////

		namespace internal {
			template<typename Car, typename Cdr> class GenerateInterfaceHierarchy :
				virtual public Car, public GenerateInterfaceHierarchy<typename Cdr::Car, typename Cdr::Cdr> {};
			template<typename Interface>
			class GenerateInterfaceHierarchy<Interface, void> : virtual public Interface {};
			template<typename Interfaces> struct ImplementsAllBase :
				public GenerateInterfaceHierarchy<typename Interfaces::Car, typename Interfaces::Cdr> {};
		} // namespace internal

		template<typename Interfaces> struct ImplementsAll :
			public internal::ImplementsAllBase<typename typelist::RemoveBases<Interfaces>::Result> {};

		// Threading policy for IUnknownImpl ////////////////////////////////

		struct NoReferenceCounting {};
		struct SingleThreaded {};
		struct MultiThreaded {};

		namespace internal {
			template<typename ThreadingPolicy = MultiThreaded> class ReferenceCounter {
			public:
				ReferenceCounter() /*throw()*/ : c_(0) {}
				ULONG increment() /*throw()*/;
				ULONG decrement() /*throw()*/;
			private:
				long c_;
			};
			template<> class ReferenceCounter<NoReferenceCounting> {
			public:
				ULONG increment() {return 2;}
				ULONG decrement() {return 1;}
			};
			template<> inline ULONG ReferenceCounter<SingleThreaded>::increment() {return ++c_;}
			template<> inline ULONG ReferenceCounter<SingleThreaded>::decrement() {return --c_;}
			template<> inline ULONG ReferenceCounter<MultiThreaded>::increment() {return ::InterlockedIncrement(&c_);}
			template<> inline ULONG ReferenceCounter<MultiThreaded>::decrement() {return ::InterlockedDecrement(&c_);}
		} // namespace internal

		// IUnknownImpl class ///////////////////////////////////////////////

		namespace internal {
			template<typename Self, typename InterfaceSignatures> struct ChainQueryInterface;
			template<typename Self, typename Car, const IID* iid, typename Cdr>
			struct ChainQueryInterface<Self, typelist::Cat<InterfaceSignature<Car, iid>, Cdr> > {
				HRESULT operator()(Self& self, const IID& riid, void** ppv) {
					if(toBoolean(::InlineIsEqualGUID(riid, *iid)))
						return (*ppv = static_cast<Car*>(&self)), self.AddRef(), S_OK;
					return ChainQueryInterface<Self, Cdr>()(self, riid, ppv);
				}
			};
			template<typename Self> struct ChainQueryInterface<Self, void> {
				HRESULT operator()(Self& self, const IID& iid, void** ppv) {
					return (*ppv = 0), E_NOINTERFACE;
				}
			};
		} // namespace internal

		/**
		 * Implements @c IUnknown interface.
		 * @param InterfacesSignatures the interfaces to implement
		 * @param ThreadingPolicy the policy for reference counting. acceptable entities are
		 * @c NoReferenceCounting, @c SingleThreaded and @c MultiThreaded.
		 * @note This class must be base of the other interface implementing classes.
		 */
		template<typename InterfaceSignatures, typename ThreadingPolicy = MultiThreaded>
		class IUnknownImpl : public ImplementsAll<typename InterfacesFromSignatures<InterfaceSignatures>::Result> {
		public:
			IUnknownImpl() /*throw()*/ {}
			virtual ~IUnknownImpl() throw() {}
			STDMETHODIMP_(ULONG) AddRef() {return c_.increment();}
			STDMETHODIMP_(ULONG) Release() {
				if(const ULONG c = c_.decrement())
					return c;
				delete this;
				return 0;
			}
			STDMETHODIMP QueryInterface(REFIID iid, void** ppv) {
				MANAH_VERIFY_POINTER(ppv);
				if(toBoolean(::InlineIsEqualGUID(iid, IID_IUnknown))) {
					*ppv = static_cast<InterfaceSignatures::Car::Interface*>(this);
					return AddRef(), S_OK;
				}
				return internal::ChainQueryInterface<
					IUnknownImpl<InterfaceSignatures, ThreadingPolicy>, InterfaceSignatures>()(*this, iid, ppv);
			}
		private:
			internal::ReferenceCounter<ThreadingPolicy> c_;
		};

}} // namespace manah.com

#endif // !MANAH_UNKNOWN_IMPL_HPP
