/**
 * @file unknown-impl.hpp
 * Provides macros implement three methods of @c IUnknown interface.
 * @author exeal
 * @date 2002-2012
 */

#ifndef ASCENSION_WIN32_UNKNOWN_IMPL_HPP
#define ASCENSION_WIN32_UNKNOWN_IMPL_HPP

#include <ascension/win32/com/com.hpp>
#include <ascension/corelib/type-list.hpp>

namespace ascension {

	namespace win32 {
		namespace com {
			/**
			 * A pair of a interface type and its GUID.
			 * @tparam I The interface type
			 * @tparam iid The GUID of @a Interface
			 */
			template<typename I, const IID* iid> struct Interface {
				typedef I Type;
			};

			// Threading policy for IUnknownImpl ////////////////////////////////

			/// A policy for @c IUnknownImpl does not manage the reference count.
			struct NoReferenceCounting {};
			/// A policy for @c IUnknownImpl uses C++ standard operators for @c long type to manage
			/// the reference count.
			struct SingleThreaded {};
			/// A policy for @c IUnknownImpl uses Win32 @c InterlockedXxcrement functions to manage
			/// the reference count.
			struct MultiThreaded {};
		}
	}

	namespace detail {
		template<typename Car, typename Cdr>
		class GenerateInterfaceHierarchy : virtual public Car,
			public GenerateInterfaceHierarchy<
				typename typelist::Car<Cdr>::Type, typename typelist::Cdr<Cdr>::Type> {};
		template<typename I>
		class GenerateInterfaceHierarchy<I, void> : virtual public I {};

		template<typename Interfaces> struct TypelistFromInterfaces;
		template<typename I, const IID* iid, typename Cdr>
		struct TypelistFromInterfaces<typelist::Cat<win32::com::Interface<I, iid>, Cdr>> {
			typedef typelist::Cat<I, typename TypelistFromInterfaces<Cdr>::Type> Type;
		};
		template<typename I, const IID* iid>
		struct TypelistFromInterfaces<win32::com::Interface<I, iid>> {
//			typedef I Type;
			typedef typelist::Cat<I> Type;
		};
		template<> struct TypelistFromInterfaces<void> {typedef void Type;};

		template<typename Interfaces>
		struct ImplementsAllBase : public GenerateInterfaceHierarchy<
			typename typelist::Car<Interfaces>::Type, typename typelist::Cdr<Interfaces>::Type> {};

		template<typename Interfaces> struct ImplementsAll :
			public detail::ImplementsAllBase<typename typelist::RemoveBases<Interfaces>::Type> {};

		template<typename Self, typename Interfaces> struct ChainQueryInterface;
		template<typename Self, typename Car, const IID* iid, typename Cdr>
		struct ChainQueryInterface<Self, typelist::Cat<win32::com::Interface<Car, iid>, Cdr>> {
			HRESULT operator()(Self& self, const IID& riid, void** ppv) {
				if(boole(::InlineIsEqualGUID(riid, *iid)))
					return (*ppv = static_cast<Car*>(&self)), self.AddRef(), S_OK;
				return ChainQueryInterface<Self, Cdr>()(self, riid, ppv);
			}
		};
		template<typename Self> struct ChainQueryInterface<Self, void> {
			HRESULT operator()(Self& self, const IID& iid, void** ppv) {
				return (*ppv = 0), E_NOINTERFACE;
			}
		};

		template<typename ThreadingPolicy> class ReferenceCounter;
		template<> class ReferenceCounter<win32::com::NoReferenceCounting> {
		public:
			ULONG increment() {return 2;}
			ULONG decrement() {return 1;}
		};
		template<> class ReferenceCounter<win32::com::SingleThreaded> {
		public:
			ReferenceCounter() /*noexcept*/ : c_(0) {}
			ULONG increment() /*noexcept*/ {return ++c_;}
			ULONG decrement() /*noexcept*/{return --c_;}
		private:
			ULONG c_;
		};
		template<> class ReferenceCounter<win32::com::MultiThreaded> {
		public:
			ReferenceCounter() /*noexcept*/ : c_(0) {}
			ULONG increment() /*noexcept*/ {return ::InterlockedIncrement(&c_);}
			ULONG decrement() /*noexcept*/{return ::InterlockedDecrement(&c_);}
		private:
			long c_;
		};
	}

	namespace win32 {
		namespace com {
			/// Generates @c Interface code from interface's @a name.
			#define ASCENSION_WIN32_COM_INTERFACE(name)	\
				ascension::win32::com::Interface<name, &IID_##name>

			/**
			 * Implements @c IUnknown interface.
			 * @tparam Interfaces The type list of @c Interface class templates
			 * @tparam ThreadingPolicy The policy for reference counting. Acceptable entities are
			 *                         @c NoReferenceCounting, @c SingleThreaded and
			 *                         @c MultiThreaded
			 * @note This class must be base of the other interface implementing classes.
			 */
			template<typename Interfaces, typename ThreadingPolicy = MultiThreaded>
			class IUnknownImpl : public detail::ImplementsAll<typename detail::TypelistFromInterfaces<Interfaces>::Type> {
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
					ASCENSION_WIN32_VERIFY_COM_POINTER(ppv);
					if(boole(::InlineIsEqualGUID(iid, IID_IUnknown))) {
						*ppv = static_cast<typename typelist::Car<Interfaces>::Type::Type*>(this);
						return AddRef(), S_OK;
					}
					return detail::ChainQueryInterface<
						IUnknownImpl<Interfaces, ThreadingPolicy>, Interfaces>()(*this, iid, ppv);
				}
			private:
				detail::ReferenceCounter<ThreadingPolicy> c_;
			};

		}
	}
} // namespace ascension.win32.com

#endif // !ASCENSION_WIN32_UNKNOWN_IMPL_HPP
