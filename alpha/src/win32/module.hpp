/**
 * @file win32/module.hpp
 * Defines @c win32#Module class.
 * @author exeal
 * @date 2003-2007, 2013
 */

#ifndef ALPHA_WIN32_MODULE_HPP
#define ALPHA_WIN32_MODULE_HPP
#include "win32/resource-id.hpp"
#include <ascension/win32/handle.hpp>
#include <string>
#include <type_traits>
#include <utility>


namespace alpha {
	namespace win32 {
		class Module {
		public:
			/**
			 * Constructor.
			 * @param handle A handle to the module
			 * @throw ascension#NullPointerException @a handle is @c null
			 */
			explicit Module(ascension::win32::Handle<HMODULE> handle) : handle_(handle), accelerators_(nullptr) {
				if(handle_ == nullptr)
					throw ascension::NullPointerException("handle");
				WCHAR temp[MAX_PATH];
				if(0 == ::GetModuleFileNameW(handle_.get(), temp, std::extent<decltype(temp)>::value))
					throw ascension::makePlatformError();
				fileName_.assign(temp);
			}

			/// Returns the module file name.
			const std::basic_string<WCHAR>& fileName() const BOOST_NOEXCEPT {
				return fileName_;
			}
#if 0
			/**
			 * Calls @c FindResourceExW.
			 * @param id The resource identifier
			 * @param type The resource type
			 * @param language The language of the resource
			 * @return A handle to the specified resource's information block
			 */
			ascension::win32::Handle<HRSRC> findResource(const ResourceID& id, const WCHAR* type, WORD language = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)) {
				if(HRSRC temp = ::FindResourceExW(handle_.get(), type, id, language))
					return ascension::win32::borrowed(temp);
				throw ascension::makePlatformError();
			}
#endif
			/**
			 * Calls @c LoadAcceleratorsW.
			 * @param id The resource identifier
			 */
			void loadAccelerators(const ResourceID& id) {
				auto newAccelerators(ascension::win32::borrowed(::LoadAcceleratorsW(handle_.get(), id)));	// MSDN says "are freed automatically when the application terminates."
				if(newAccelerators.get() == nullptr)
					throw ascension::makePlatformError();
				std::swap(accelerators_, newAccelerators);
			}
#if 0
			/**
			 * Calls @c LoadResource.
			 * @param resource A handle to the resource to be loaded
			 * @return A handle to the data associated with the resource
			 * @throw ascension#NullPointerException @a resource is @c null
			 */
			ascension::win32::Handle<HGLOBAL> loadResource(ascension::win32::Handle<HRSRC> resource) {
				if(resource.get() == nullptr)
					throw ascension::NullPointerException("resource");
				else if(auto temp = ascension::win32::borrowed(::LoadResource(handle_.get(), resource.get())))
					return temp;
				throw ascension::makePlatformError();
			}

			/**
			 * Calls @c LoadCursorW with @c null @c HMODULE.
			 * @param id The resource identifier
			 * @return A handle to the loaded cursor
			 */
			static ascension::win32::Handle<HCURSOR> loadStandardCursor(const ResourceID& id) {
				if(auto temp = ascension::win32::borrowed(::LoadCursorW(nullptr, id)))
					return temp;
				throw ascension::makePlatformError();
			}

			/**
			 * Calls @c LoadIconW with @c null @c HMODULE.
			 * @param id The resource identifier
			 * @return A handle to the loaded icon
			 */
			static ascension::win32::Handle<HICON> loadStandardIcon(const ResourceID& id) {
				if(auto temp = ascension::win32::borrowed(::LoadIconW(nullptr, id)))
					return temp;
				throw ascension::makePlatformError();
			}

			/**
			 * Calls @c SizeofResource.
			 * @param resource A handle to the resource
			 * @return The number of bytes in the resource
			 * @throw ascension#NullPointerException @a resource is @c null
			 */
			DWORD sizeofResource(ascension::win32::Handle<HRSRC> resource) {
				if(resource.get() == nullptr)
					throw ascension::NullPointerException("resource");
				else if(const DWORD temp = ::SizeofResource(handle_.get(), resource.get()))
					return temp;
				throw ascension::makePlatformError();
			}
#endif
		protected:
			/// Returns a handle to the loaded accelerators, or @c null if not loaded.
			ascension::win32::Handle<HACCEL> accelerators() const BOOST_NOEXCEPT {return accelerators_;}

		private:
			ascension::win32::Handle<HMODULE> handle_;
			std::basic_string<WCHAR> fileName_;
			ascension::win32::Handle<HACCEL> accelerators_;
		};
	}
}

#endif // !ALPHA_WIN32_MODULE_HPP
