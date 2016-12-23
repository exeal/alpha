/**
 * @file windows.cpp
 * @author exeal
 * @date 2003-2006 was EditView.cpp and EditViewWindowMessages.cpp
 * @date 2006-2011 was viewer.cpp
 * @date 2011-05-16 separated from viewer.cpp
 * @date 2011-2014 was viewer-windows.cpp
 * @date 2014-05-25 separated from viewer-windows.cpp
 */

#include <ascension/platforms.hpp>
#if BOOST_OS_WINDOWS
#include <ascension/win32/handle.hpp>
#include <ascension/win32/windows.hpp>
#include <boost/optional.hpp>

namespace ascension {
	namespace win32 {
		namespace {
			BOOL CALLBACK enumResLangProc(HMODULE, const WCHAR*, const WCHAR* name, WORD langID, LONG_PTR param) {
				if(name == nullptr)
					return false;
				else if(langID != MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US))
					*reinterpret_cast<LANGID*>(param) = langID;
				return true;
			}
		}

		LANGID ascension::win32::userDefaultUILanguage() {
			// references (from Global Dev)
			// - Writing Win32 Multilingual User Interface Applications (http://www.microsoft.com/globaldev/handson/dev/muiapp.mspx)
			// - Ask Dr. International Column #9 (http://www.microsoft.com/globaldev/drintl/columns/009/default.mspx#EPD)
			boost::optional<LANGID> id;
			if(id == boost::none) {
				id = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
				OSVERSIONINFOW version;
				version.dwOSVersionInfoSize = sizeof(decltype(version));
				if(!boole(::GetVersionExW(&version)))
					throw makePlatformError();
				assert(version.dwPlatformId == VER_PLATFORM_WIN32_NT);

				// forward to GetUserDefaultUILanguage (kernel32.dll) if after 2000/XP/Server 2003
				if(version.dwMajorVersion >= 5) {
					const Handle<HMODULE>::Type dll(::LoadLibraryW(L"kernel32.dll"), std::bind(&::FreeLibrary, std::placeholders::_1));
					if(dll.get() != nullptr) {
						if(LANGID(WINAPI *function)(void) = reinterpret_cast<LANGID(WINAPI*)(void)>(::GetProcAddress(dll.get(), "GetUserDefaultUILanguage")))
							id = (*function)();
					}
				}

				// use language of version information of ntdll.dll if on NT 3.51-4.0
				else {
					const Handle<HMODULE>::Type dll(::LoadLibraryW(L"ntdll.dll"), std::bind(&::FreeLibrary, std::placeholders::_1));
					::EnumResourceLanguagesW(dll.get(), MAKEINTRESOURCEW(16)/*RT_VERSION*/,
						MAKEINTRESOURCEW(1), enumResLangProc, reinterpret_cast<LONG_PTR>(&id));
					if(*id == MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)) {	// special cases
						const UINT cp = ::GetACP();
						if(cp == 874)	// Thai
							id = MAKELANGID(LANG_THAI, SUBLANG_DEFAULT);
						else if(cp == 1255)	// Hebrew
							id = MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT);
						else if(cp == 1256)	// Arabic
							id = MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_SAUDI_ARABIA);
					}
				}
			}

			return *id;	// (... or use value of HKCU\Control Panel\Desktop\ResourceLocale if on Win 9x
		}
	}
}

#endif // BOOST_OS_WINDOWS
