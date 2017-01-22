/***/

#ifndef ALPHA_WIN32_RESOURCE_ID_HPP
#define ALPHA_WIN32_RESOURCE_ID_HPP
#include <ascension/win32/windows.hpp>
#include <boost/utility/string_ref.hpp>

namespace alpha {
	namespace win32 {
		/// Creates a resource identifier from both a string and integer.
		class ResourceID {
		public:
			/**
			 * Creates a @c ResourceID instance with a string identifier.
			 * @param name The identifier
			 */
			ResourceID(const boost::basic_string_ref<WCHAR, std::char_traits<WCHAR>>& name) BOOST_NOEXCEPT : name_(name.data()) {}
			/**
			 * Creates a @c ResourceID instance with a numeric identifier.
			 * @param id The identifier
			 */
			ResourceID(UINT_PTR id) BOOST_NOEXCEPT : name_(MAKEINTRESOURCEW(id)) {
				if(!IS_INTRESOURCE(id))
					throw std::invalid_argument("id");
			}
			/// Returns the string identifier.
			operator const WCHAR*() const BOOST_NOEXCEPT {
				return name_;
			}

		private:
			const WCHAR* const name_;
		};
	}
}

#endif // !ALPHA_WIN32_RESOURCE_ID_HPP
