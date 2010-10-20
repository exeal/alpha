/**
 * @file windows.hpp
 * @date 2006-2009 exeal
 */

#ifndef MANAH_WINDOWS_HPP
#define MANAH_WINDOWS_HPP

#if defined(_DEBUG) && !defined(MANAH_NO_MEMORY_LEAK_CHECK)
#	define _CRTDBG_MAP_ALLOC
#	include <cstdlib>
#	include <malloc.h>
#	include <crtdbg.h>
#	define _DEBUG_NEW MANAH_DEBUG_NEW
#	define MANAH_DEBUG_NEW ::new(_NORMAL_BLOCK, MANAH_OVERRIDDEN_FILE, __LINE__)
#	define MANAH_OVERRIDDEN_FILE "uknown source file"
#endif // defined(_DEBUG) && !defined(MANAH_NO_MEMORY_LEAK_CHECK)
/*
	... and you should do the follow:
	#ifdef _DEBUG
	#undef MANAH_OVERRIDDEN_FILE
	static const char MANAH_OVERRIDDEN_FILE[] = __FILE__;
	#endif
 */

#include "../object.hpp"
#ifndef STRICT
#	define STRICT
#endif // !STRICT
#ifndef _WIN32_WINNT
#	define _WIN32_WINNT 0x0500	// Windows 2000
#endif // !_WIN32_WINNT
#ifndef WINVER
#	define WINVER 0x0500	// Windows 2000
#endif // !WINVER
#ifndef WIN32_LEAN_AND_MEAN
#	define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#	define NOMINMAX
#endif // !NOMINMAX
#include <cassert>
#include <cstring>	// prevent C header inclusion
#include <cwchar>	// prevent C header inclusion
#include <cstdlib>	// prevent C header inclusion
#undef min
#undef max

#define size_t std::size_t
//#include <winnt.h>
#include <windows.h>
#undef size_t
#include <memory>	// std.auto_ptr
#include <stdexcept>
#include <sstream>

namespace manah {
	namespace win32 {

		/// The specified handle is invalid.
		class InvalidHandleException : public std::invalid_argument {
		public:
			explicit InvalidHandleException(const std::string& message) : std::invalid_argument(message) {}
		};

		/// The specified handle is @c null and not allowed.
		class NullHandleException : public InvalidHandleException {
		public:
			explicit NullHandleException(const std::string& message) : InvalidHandleException(message) {}
		};

		namespace internal {
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

		/**
		 *
		 * @tparam T
		 */
		template<typename T> class Handle {
		public:
			explicit Handle(T handle = 0) : handle_(handle) {}
			template<typename Deleter> Handle(T handle, Deleter deleter) : handle_(handle),
				deleter_((deleter != 0) ? new internal::HandleDeleter<T, Deleter>(handle, deleter) : 0) {}
			~Handle() {reset();}
			Handle(Handle<T>& other) : handle_(other.handle_),
				deleter_(other.deleter_) {other.handle_ = 0; other.deleter_ = 0;}
			Handle<T>& operator=(Handle<T>& other) {
				reset(other.handle_, other.deleter_); other.handle_ = 0; other.deleter_ = 0; return *this;}
			T get() const {return handle_;}
			T release() {delete deleter_; deleter_ = 0; HandleType temp(0); std::swap(handle_, temp); return temp;}
			void reset(T handle = 0) {
//				return reset(handle, 0);
				if(deleter_.get() != 0 && handle != handle_)
					deleter_->destroy();
				handle_ = handle;
				deleter_.reset();
			}
			template<typename Deleter> void reset(T handle, Deleter deleter) {
				std::auto_ptr<internal::HandleDeleterBase> newDeleter((deleter != 0) ?
					new internal::HandleDeleter<T, Deleter>(handle, deleter) : 0);
				if(deleter_.get() != 0 && handle != handle_)
					deleter_->destroy();
				handle_ = handle;
				std::swap(deleter_, newDeleter);
			}
			T use() const {
				if(handle_ == 0)
					throw std::logic_error("handle is null.");
				return handle_;
			}
		private:
			T handle_;
			std::auto_ptr<internal::HandleDeleterBase> deleter_;
		};

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

#	define MANAH_WIN32_OBJECT_CONSTRUCTORS(ClassName)							\
		ClassName() : BaseObject() {}											\
		explicit ClassName(Managed<HandleType>* handle) : BaseObject(handle) {}	\
		explicit ClassName(Borrowed<HandleType>* handle) : BaseObject(handle) {}

		/// A resource identifier can be initialized by using both a string and a numeric identifier.
		class ResourceID {
			MANAH_NONCOPYABLE_TAG(ResourceID);
		public:
			/// Constructor takes a string identifier.
			ResourceID(const WCHAR* name) /*throw()*/ : name_(name) {}
			/// Constructor takes a numeric identifier.
			ResourceID(UINT_PTR id) /*throw()*/ : name_(MAKEINTRESOURCEW(id)) {}
			/// Returns the string identifier.
			operator const WCHAR*() const /*throw()*/ {return name_;}
		private:
			const WCHAR* const name_;
		};

		/// Defines a structure type automatically fills oneself with zero.
		template<typename Structure> struct AutoZero : public Structure {
			/// Default constructor.
			AutoZero() /*throw()*/ {std::memset(this, 0, sizeof(Structure));}
		};

		/// Defines a structure type automatically fills oneself with zero and sets its size member.
		template<typename Structure, typename SizeType = int> struct AutoZeroSize : public AutoZero<Structure> {
			/// Default constructor.
			AutoZeroSize() /*throw()*/ {*reinterpret_cast<SizeType*>(this) = sizeof(Structure);}
		};

		class DumpContext {
		public:
			template<typename T>
			DumpContext& operator<<(const T& rhs) throw();
			void hexDump(const WCHAR* line, byte* pb, int bytes, int width = 0x10) throw();
		};

		inline void DumpContext::hexDump(const WCHAR* line, byte* pb, int bytes, int width /* = 0x10 */) throw() {
			WCHAR* const output = new WCHAR[static_cast<std::size_t>(
				(std::wcslen(line) + 3 * width + 2) * static_cast<float>(bytes / width))];
			std::wcscpy(output, line);

			WCHAR buffer[4];
			for(int i = 0; i < bytes; ++i){
				::wsprintfW(buffer, L" %d", pb);
				std::wcscat(output, buffer);
				if(i % width == 0){
					std::wcscat(output, L"\n");
					std::wcscat(output, line);
				}
			}
			::OutputDebugStringW(L"\n>----Dump is started");
			::OutputDebugStringW(output);
			::OutputDebugStringW(L"\n>----Dump is done");
			delete[] output;
		}

		template<typename T> inline DumpContext& DumpContext::operator<<(const T& rhs) throw() {
			std::wostringstream ss;
			ss << rhs;
			::OutputDebugStringW(ss.str().c_str());
			return *this;
		}

	}
}


// macros ///////////////////////////////////////////////////////////////////

// sizeof(MENUITEMINFO)
#if(WINVER >= 0x0500 && !defined(MENUITEMINFO_SIZE_VERSION_400))
#	define MENUITEMINFO_SIZE_VERSION_400A (offsetof(MENUITEMINFOA, cch) + sizeof(static_cast<MENUITEMINFOA*>(0)->cch))
#	define MENUITEMINFO_SIZE_VERSION_400W (offsetof(MENUITEMINFOW, cch) + sizeof(static_cast<MENUITEMINFOW*>(0)->cch))
#	ifdef UNICODE
#		define MENUITEMINFO_SIZE_VERSION_400 MENUITEMINFO_SIZE_VERSION_400W
#	else
#		define MENUITEMINFO_SIZE_VERSION_400 MENUITEMINFO_SIZE_VERSION_400A
#	endif // !UNICODE
#endif // WINVER >= 0x0500 && !defined(MENUITEMINFO_SIZE_VERSION_400)

#endif // !MANAH_WINDOWS_HPP
