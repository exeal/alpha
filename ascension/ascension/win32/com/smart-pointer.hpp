/**
 * @file smart-pointer.hpp
 * @author exeal
 * @date 2002-2009 was manah/com/common.hpp
 * @date 2010-2012 was com.hpp
 * @date 2012-05-04 separated from com.hpp
 */

#ifndef ASCENSION_WIN32_SMART_POINTER_HPP
#define ASCENSION_WIN32_SMART_POINTER_HPP

#include <objbase.h>	// CoCreateInstance
#include <cassert>
#include <utility>		// std.swap
#include <boost/operators.hpp>


namespace ascension {
	namespace win32 {
		namespace com {
			/// A proxy returned by @c SmartPointer#operator->.
			template<typename T> class SmartPointerProxy : public T {
			private:
				STDMETHOD_(ULONG, AddRef)() = 0;
				STDMETHOD_(ULONG, Release)() = 0;
				T** operator &() const /*throw()*/;	// prohibits &*p
			};

			/**
			 * COM smart pointer.
			 * @tparam T The interface type
			 * @tparam iid The GUID of @a T
			 */
			template<typename T, const IID* iid = &__uuidof(T)>
			class SmartPointer :
				public boost::totally_ordered<T*, SmartPointer<T, iid>>,
				public boost::totally_ordered<SmartPointer<T, iid>, SmartPointer<T, iid>> {
			public:
				typedef T element_type;	///< The interface type.
			public:
				/// Default constructor.
				SmartPointer() /*noexcept*/ : pointee_(nullptr) {}
				/// Null pointer constructor.
				SmartPointer(std::nullptr_t) /*noexcept*/ : pointee_(nullptr) {}
				/// Constructor with an interface pointer.
				template<typename U>
				explicit SmartPointer(U* p) /*noexcept*/ : pointee_(nullptr) {
					if(p != nullptr)
						p->QueryInterface(*iid, initializePPV());
				}
				/// Constructor with an interface pointer.
				template<>
				explicit SmartPointer<element_type, iid>(element_type* p) /*noexcept*/ : pointee_(p) {
					if(pointee_ != nullptr)
						pointee_->AddRef();
				}
				/// Copy-constructor.
				SmartPointer(const SmartPointer<element_type, iid>& other) /*noexcept*/ : pointee_(other.pointee_) {
					if(pointee_ != nullptr)
						pointee_->AddRef();
				}
				/// Copy-constructor.
				template<typename U, const IID* riid>
				SmartPointer(const SmartPointer<U, riid>& other) /*noexcept*/ : pointee_(nullptr) {
					if(other.pointee_ != nullptr)
						other.pointee_->QueryInterface(iid, initializePPV());
				}
				/// Move-constructor.
				SmartPointer(SmartPointer<element_type, iid>&& other) /*noexcept*/ : pointee_(other.pointee_) {
					if(pointee_ != nullptr) {
						pointee_->AddRef();
						other.reset();
					}
				}
				/// Move-constructor.
				template<typename U, const IID* riid>
				SmartPointer(SmartPointer<U, riid>&& other) /*noexcept*/ : pointee_(nullptr) {
					if(pointee_ != nullptr) {
						if(SUCCEEDED(other.pointee_->QueryInterface(iid, initializePPV())))
							other.reset();
					}
				}
				/// Copy-assignment operator.
				template<typename U, const IID* riid>
				SmartPointer<element_type, iid>& operator=(const SmartPointer<U, riid>& other) /*noexcept*/ {
					SmartPointer<element_type, iid>(other).swap(*this);
					return *this;
				}
				/// Move-assignment operator.
				template<typename U, const IID* riid>
				SmartPointer<element_type, iid>& operator=(SmartPointer<U, riid>&& other) /*noexcept*/ {
					SmartPointer<element_type, iid>(other).swap(*this);
					return *this;
				}
				/// Destructor.
				~SmartPointer() /*noexcept*/ {
					if(pointee_ != nullptr)
						pointee_->Release();
				}
				/// 
				operator bool() const /*noexcept*/ {return get() != nullptr;}
				/// Constructor creates instance by using @c CoCreateInstance.
				static SmartPointer<element_type, iid> create(REFCLSID clsid, REFIID iid /* = __uuidof(Interface) */,
						DWORD context = CLSCTX_ALL, IUnknown* outer = nullptr, HRESULT* hr = nullptr) {
					SmartPointer<element_type, iid> p;
					const HRESULT r = ::CoCreateInstance(clsid, outer, context, iid, p.initializePPV());
					if(hr != 0)
						*hr = r;
					return p;
				}
				/// Returns the interface pointer.
				SmartPointerProxy<element_type>* get() const /*noexcept*/ {
					return static_cast<SmartPointerProxy<element_type>*>(pointee_);
				}
				/// Returns the output pointer for initialization.
				element_type** initialize() /*noexcept*/ {
					reset();
					return &pointee_;
				}
				/// Returns the output pointer for initialization with type @c void**.
				void** initializePPV() /*noexcept*/ {
					return reinterpret_cast<void**>(initialize());
				}
				/// Returns true if the pointer addresses the same object.
				bool equals(IUnknown* p) const /*noexcept*/ {
					if(pointee_ == nullptr && p == nullptr)
						return true;
					else if(pointee_ == nullptr || p == nullptr)
						return false;
					SmartPointerProxy<IUnknown> ps[2];
					pointee_->QueryInterface(IID_IUnknown, ps[0].initialize());
					p->QueryInterface(IID_IUnknown, ps[1].initialize());
					return ps[1].get() == ps[2].get();
				}
				/// Resets the pointer.
				void reset() /*noexcept*/ {
					SmartPointer<element_type, iid>().swap(*this);
				}
				/// Resets the pointer.
				template<typename U>
				void reset(U* p) /*noexcept*/ {
					if(p != pointee_)
						SmartPointer<element_type, iid>(p).swap(*this);
				}
				/// Swaps the two objects.
				void swap(SmartPointer<element_type>& other) /*noexcept*/ {
					std::swap(pointee_, other.pointee_);
				}
				/// Swaps the two objects.
				void swap(SmartPointerProxy<element_type>& other) /*noexcept*/ {
					std::swap(pointee_, other.pointee_);
				}
				/// Member-access operator.
				SmartPointerProxy<element_type>* operator->() const /*noexcept*/ {
					assert(get() != nullptr);
					return get();
				}
				/// Dereference operator.
				SmartPointerProxy<element_type>& operator*() const /*noexcept*/ {
					assert(get() != nullptr);
					return *get();
				}
				/// Equality operator.
				friend bool operator==(const element_type* lhs, const SmartPointer<element_type, iid>& rhs) /*noexcept*/ {
					return operator==(rhs, lhs);
				}
				/// Equality operator.
				friend bool operator==(const SmartPointer<element_type, iid>& lhs, const SmartPointer<element_type, iid>& rhs) /*noexcept*/ {
					return lhs.pointee_ == rhs.pointee_;
				}
				/// Less than operator.
				friend bool operator<(const element_type* lhs, const SmartPointer<element_type, iid>& rhs) /*noexcept*/ {
					return lhs < rhs.pointee_;
				}
				/// Less than operator.
				friend bool operator<(const SmartPointer<element_type, iid> lhs, const SmartPointer<element_type, iid>& rhs) /*noexcept*/ {
					return lhs.pointee_ < rhs.pointee_;
				}
				/// Greater than operator.
				friend bool operator>(const element_type* lhs, const SmartPointer<element_type, iid>& rhs) /*noexcept*/ {
					return lhs > rhs.pointee_;
				}
				/// Greater than operator.
				friend bool operator>(const SmartPointer<element_type, iid>& lhs, const SmartPointer<element_type, iid>& rhs) /*noexcept*/ {
					return lhs.pointee_ > rhs.pointee_;
				}
			private:
				SmartPointerProxy<element_type>* operator&() const;
				element_type* pointee_;
			};
		}
	}
} // namespace ascension.win32.com

#endif // !ASCENSION_WIN32_SMART_POINTER_HPP
