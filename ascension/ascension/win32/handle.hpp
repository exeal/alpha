/**
 * @file handle.hpp
 * @date 2006-2011 was windows.hpp
 * @date 2012-04-17 separated from windows.hpp
 * @date 2012, 2014
 * @deprecated This header file was deprecated since version 0.8. Use @c std#shared_ptr and
 *             @c std#unique_ptr class templates. But Win32 handle types are not pointers...
 */

#ifndef ASCENSION_WIN32_HANDLE_HPP
#define ASCENSION_WIN32_HANDLE_HPP
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
#	include <ascension/win32/windows.hpp>
#	include <memory>	// std.unique_ptr
#else
#	include <ascension/win32/windows.hpp>
#	include <memory>
#	include <type_traits>
#endif // ASCENSION_ABANDONED_AT_VERSION_08
#include <functional>	// std.bind, std.placeholders

namespace ascension {
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
	namespace detail {
		class HandleDeleterBase {
		public:
			virtual ~HandleDeleterBase() {}
			virtual void destroy() = 0;
		};

		template<typename Handle, typename Deleter>
		class HandleDeleter : public HandleDeleterBase {
		public:
			HandleDeleter(Handle handle, Deleter deleter) : handle_(handle), deleter_(deleter) {}
			void destroy() {deleter_(handle_);}
		private:
			Handle handle_;
			Deleter deleter_;
		};
	}
#endif // ASCENSION_ABANDONED_AT_VERSION_08

	namespace win32 {
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
		/**
		 *
		 * @tparam T
		 */
		template<typename T> class Handle {
		public:
			typedef T element_type;
		public:
			/// Default constructor.
			Handle() : handle_(element_type()) {}
			/// Null pointer constructer.
			Handle(std::nullptr_t) : handle_(element_type()) {}
			/// Constructor with a handle.
			explicit Handle(element_type h) : handle_(h) {}
			/**
			 * Constructor. A @c Handle initialized by this constructor does not get the ownership
			 * of the handle.
			 * @tpatam U The type of @a handle. Can be not @c T
			 * @param handle The handle to hold
			 */
			template<typename U>
			explicit Handle(U handle) : handle_(handle) {}
			/**
			 * Constructor. A @c Handle initialized by this constructor gets the ownership of the
			 * handle and the destructor destroys the handle.
			 * @tparam U The type of @a handle. Can be not @c T
			 * @tparam D The type of @a deleter
			 * @param handle The handle to hold
			 * @param deleter The function destroys the handle
			 */
			template<typename U, typename D> Handle(U handle, D deleter) :
				handle_(handle), deleter_(new detail::HandleDeleter<T, D>(handle, deleter)) {}
			/// Destructor.
			~Handle() {
				if(deleter_.get() != 0)
					deleter_->destroy();
			}
			/**
			 * Move-constructor.
			 * @param other The source object
			 */
			Handle(Handle<element_type>&& other)
				: handle_(other.release()), deleter_(std::move(other.deleter_)) {}
			/**
			 * Move-constructor.
			 * @tparam U The type of @a other
			 * @param other The source object
			 */
			template<typename U>
			Handle(Handle<U>&& other)
				: handle_(other.release()), deleter_(std::move(other.deleter_)) {}
			/**
			 * Move-assignment operator.
			 * @param other The source object.
			 * @return This object
			 */
			Handle<element_type>& operator=(Handle<element_type>&& other) {
				Handle<element_type>(other).swap(*this);
				return *this;
			}
			/**
			 * Move-assignment operator.
			 * @tparam U The type of @a other
			 * @param other The source object.
			 * @return This object
			 */
			template<typename U>
			Handle<element_type>& operator=(Handle<U>&& other) {
				Handle<element_type>(other).swap(*this);
				return *this;
			}
			/// 
			operator bool() const {return get() != element_type();}
			/// Returns the held handle.
			element_type get() const {return handle_;}
			/***/
			void swap(Handle<element_type>& other) {
				std::swap(handle_, other.handle_);
				std::swap(deleter_, other.deleter_);
			}
			/**
			 * Releases the held handle without destruction.
			 * @return The held handle
			 */
			element_type release() {
				deleter_.reset();
				T temp(0);
				std::swap(handle_, temp);
				return temp;
			}
			/// Resets the handle.
			void reset() {Handle<element_type>().swap(*this);}
			/**
			 * Resets the handle. This method does not get the ownership of the handle.
			 * @param handle The handle to hold. Can be @c null
			 */
			template<typename U>
			void reset(U handle) {
				if(handle != get())
					Handle<element_type>(handle).swap(*this);
			}
			/**
			 * Resets the handle. This method does not get the ownership of the handle.
			 * @tparam Deleter The type of @a deleter
			 * @param handle The handle to hold. Can be @c null
			 * @param deleter The function destroys the handle
			 */
			template<typename U, typename D> void reset(U handle, D deleter) {
				if(handle == element_type() || handle != get())
					Handle<element_type>(handle, deleter).swap(*this);
				else
					deleter_.reset(new detail::HandleDeleter<U, D>(handle, deleter));
			}
		private:
			Handle(const Handle<element_type>&);
			template<typename U> Handle(const Handle<U>&);
			Handle<element_type>& operator=(const Handle<element_type>&);
			template<typename U> Handle<element_type>& operator=(const Handle<U>&);
		private:
			element_type handle_;
			std::unique_ptr<detail::HandleDeleterBase> deleter_;
		};

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
		template<typename Handle> struct Managed {};
		template<typename Handle> struct Borrowed {};
		template<typename Handle> inline Managed<Handle>* managed(Handle handle) {return reinterpret_cast<Managed<Handle>*>(handle);}
		template<typename Handle> inline Borrowed<Handle>* borrowed(Handle handle) {return reinterpret_cast<Borrowed<Handle>*>(handle);}

		/**
		 * Holds and manages a handle value. The instance has the ownership of the handle value.
		 * The semantics of the copy operations is same as @c std#auto_ptr.
		 * @tparam Handle the type of the handle to be held
		 * @tparam deleter Win32 function used to discard the handle
		 * @tparam HeldType the type of the handle to be held actually
		 */
		template<typename Handle = HANDLE, BOOL(WINAPI* deleter)(Handle) = ::CloseHandle, typename HeldType = Handle>
		class Object {
		public:
			/// The type of the handle to be held.
			typedef HeldType HandleType;
			/// Alias of this type.
			typedef Object<Handle, deleter, HeldType> BaseObject;
		public:
			/// Constructor takes a handle as the initial value and manages it.
			explicit Object(Managed<HandleType>* handle) : handle_(reinterpret_cast<HandleType>(handle)), manages_(true) {
				if(handle_ != 0 && !check(handle_))
					throw InvalidHandleException("handle");
			}
			/// Constructor takes a handle as the initial value.
			explicit Object(Borrowed<HandleType>* handle = 0) : handle_(reinterpret_cast<HandleType>(handle)), manages_(false) {
				if(handle_ != 0 && !check(handle_))
					throw InvalidHandleException("handle");
			}
			/// Destructor discards the handle.
			virtual ~Object() {
				reset();
			}
			/// Copy-constructor takes the ownership of the handle away from @a other.
			Object(Object<Handle, deleter, HandleType>& other) : handle_(other.handle_), manages_(other.manages_) {
				other.handle_ = 0;
			}
			/// Assignment operator takes the ownership of the handle away from @a other.
			Object<Handle, deleter, HandleType>& operator=(Object<Handle, deleter, HandleType>& other) {
				reset();
				std::swap(handle_, other.handle_);
				manages_ = other.manages_;
				return *this;
			}
			/// Returns the raw handle value.
			HandleType get() const {return handle_;}
			/// Sets the internal handle value to @c null.
			HandleType release() {
				HandleType temp(0);
				std::swap(handle_, temp);
				return temp;
			}
			/// Discards or release the current handle, holds the new handle value and manages it.
			void reset(Managed<HandleType>* newValue) {
				resetHandle(reinterpret_cast<HandleType>(newValue));
				manages_ = true;
			}
			/// Discards or release the current handle and holds the new handle value.
			void reset(Borrowed<HandleType>* newValue = 0) {
				resetHandle(reinterpret_cast<HandleType>(newValue));
				manages_ = false;
			}
			/// Returns the raw handle value. If the handle is @c null, throws @c std#logic_error.
			HandleType use() const {
				if(handle_ == 0)
					throw std::logic_error("handle is null.");
				else if(!check(handle_))
					throw InvalidHandleException("handle is invalid.");
				return handle_;
			}
		protected:
			/// Returns @c false if @a handle is invalid. Called by @c #use method.
			virtual bool check(HandleType handle) const {return true;}
		private:
			void resetHandle(HandleType newHandle) {
				if(newHandle != 0 && !check(newHandle))
					throw InvalidHandleException("newValue");
				if(handle_ == 0)
					handle_ = newHandle;
				else if(newHandle == 0) {
					if(deleter != 0 && manages_)
						(*deleter)(handle_);
					handle_ = 0;
				} else {
					if(newHandle != handle_ && deleter != 0 && manages_)
						(*deleter)(handle_);
					handle_ = newHandle;
				}
			}
			HandleType handle_;
			bool manages_;
		};

#	define ASCENSION_WIN32_OBJECT_CONSTRUCTORS(ClassName)						\
		ClassName() : BaseObject() {}											\
		explicit ClassName(Managed<HandleType>* handle) : BaseObject(handle) {}	\
		explicit ClassName(Borrowed<HandleType>* handle) : BaseObject(handle) {}

#endif // ASCENSION_ABANDONED_AT_VERSION_08
#else
#	if 0
		template<typename HeldType>
		using Handle = std::shared_ptr<std::remove_pointer<HeldType>::type>;
#	else
		template<typename HeldType>
		struct Handle {
			typedef std::shared_ptr<typename std::remove_pointer<HeldType>::type> Type;
		};
#	endif
#endif // ASCENSION_ABANDONED_AT_VERSION_08

		namespace detail {
			inline win32::Handle<HDC>::Type screenDC() {
				HDC dc = ::GetDC(nullptr);
				if(dc == nullptr)
					throw makePlatformError();
				return win32::Handle<HDC>::Type(dc,
					std::bind(&::ReleaseDC, static_cast<HWND>(nullptr), std::placeholders::_1));
			}
		}
	}
}

#endif // !ASCENSION_WIN32_HANDLE_HPP
