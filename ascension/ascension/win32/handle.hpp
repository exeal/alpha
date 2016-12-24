/**
 * @file handle.hpp
 * Defines @c win32#Handle class.
 * @date 2006-2011 was windows.hpp
 * @date 2012-04-17 separated from windows.hpp
 * @date 2012, 2014
 */

#ifndef ASCENSION_WIN32_HANDLE_HPP
#define ASCENSION_WIN32_HANDLE_HPP
#include <ascension/win32/windows.hpp>
#include <boost/core/null_deleter.hpp>
#include <functional>	// std.bind, std.placeholders
#include <memory>		// std.shared_ptr
#include <type_traits>	// std.remove_pointer

namespace ascension {
	namespace win32 {
		/**
		 * Safe handle type using @c std#shared_ptr.
		 * @tparam T The raw handle type
		 */
		template<typename T> class Handle : public std::shared_ptr<typename std::remove_pointer<T>::type> {
		public:
			typedef std::shared_ptr<typename std::remove_pointer<T>::type> Super;
		public:
			/// Creates an empty @c Handle.
			BOOST_CONSTEXPR Handle() : Super() {}
			/// Creates an empty @c Handle.
			BOOST_CONSTEXPR Handle(std::nullptr_t) : Super(nullptr) {}
			/**
			 * Creates a @c Handle which holds the given borrowed handle.
			 * @param handle The handle to hold
			 * @note A @c Handle created by this constructor does not destroy the handle.
			 */
			explicit Handle(T handle) : Super(handle, boost::null_deleter()) {}
			/**
			 * Creates a @c Handle which holds the given handle.
			 * @tparam Deleter The type of @a deleter
			 * @param handle The handle to hold
			 * @param deleter The function destroys the handle. This is passed to @c std#shared_ptr
			 */
			template<typename Deleter> Handle(T handle, Deleter deleter) : Super(handle, deleter) {}
			/**
			 * Move-constructor.
			 * @param other The source object
			 */
			Handle(Handle<element_type>&& other) : Super(std::move(other)) {}
			/**
			 * Move-assignment operator.
			 * @param other The source object.
			 * @return This object
			 */
			Handle<element_type>& operator=(Handle<element_type>&& other) {
				Super::operator=(std::move(other));
				return *this;
			}
		};

		/**
		 * Returns a @c Handle created with the given borrowed handle.
		 * @tparam T The type of @a handle
		 * @param handle The borrowed handle
		 * @return `Handle&lt;T&gt;(handle)`
		 */
		template<typename T>
		inline Handle<T> borrowed(T handle) {
			return Handle<T>(handle);
		}

		/**
		 * Returns a @c Handle created with the given @a handle and @a deleter.
		 * @tparam T The type of @a handle
		 * @tparam Deleter The type of @a deleter
		 * @param handle The handle
		 * @param deleter The deleter
		 * @return `Handle&lt;T&gt;(handle, deleter)`
		 */
		template<typename T, typename Deleter>
		inline Handle<T> makeHandle(T handle, Deleter deleter) {
			return Handle<T>(handle, deleter);
		}

		namespace detail {
			inline Handle<HDC> screenDC() {
				HDC dc = ::GetDC(nullptr);
				if(dc == nullptr)
					throw makePlatformError();
				return Handle<HDC>(dc, std::bind(&::ReleaseDC, static_cast<HWND>(nullptr), std::placeholders::_1));
			}
		}
	}
}

#endif // !ASCENSION_WIN32_HANDLE_HPP
