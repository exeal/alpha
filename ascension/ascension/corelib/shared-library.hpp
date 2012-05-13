/**
 * @file shared-library.hpp
 * @author exeal
 * @date 2011-04-01 separated from ../internal.hpp
 */

#ifndef SHARED_LIBRARY_HPP
#define SHARED_LIBRARY_HPP
#include <ascension/platforms.hpp>
#include <algorithm>	// std.fill
#if defined(ASCENSION_OS_WINDOWS)
#	include <ascension/corelib/basic-exceptions.hpp>
#	include <ascension/win32/windows.hpp>	// LoadLibraryA, FreeLibrary, GetProcAddress, HMODULE
#elif defined(ASCENSION_OS_POSIX)
#	include <stdexcept>
#	include <dlfcn.h>
#endif

namespace ascension {
	namespace detail {

		template<typename ProcedureEntries>
		class SharedLibrary {
			ASCENSION_NONCOPYABLE_TAG(SharedLibrary);
		public:
			explicit SharedLibrary(const char* fileName) : library_(
#ifdef ASCENSION_OS_WINDOWS
					::LoadLibraryA(fileName)
#else	// ASCENSION_OS_POSIX
					::dlopen(fileName, RTLD_LAZY)
#endif
					) {
				if(library_ == nullptr)
#ifdef ASCENSION_OS_WINDOWS
					throw makePlatformError();
#else	// ASCENSION_OS_POSIX
					throw std::runtime_error(::dlerror());
#endif
				std::fill(procedures_, procedures_ + ProcedureEntries::NUMBER_OF_ENTRIES, reinterpret_cast<NativeProcedure>(1));
			}
			~SharedLibrary() /*throw()*/ {
#ifdef ASCENSION_OS_WINDOWS
				::FreeLibrary(library_);
#else
				::dlclose(library_);
#endif
			}
			template<std::size_t index>
			typename ProcedureEntries::template Procedure<index>::signature get() const /*throw()*/ {
				typedef typename ProcedureEntries::template Procedure<index> Procedure;
				if(procedures_[index] == reinterpret_cast<NativeProcedure>(1))
					procedures_[index] =
#ifdef ASCENSION_OS_WINDOWS
						::GetProcAddress
#else
						::dlsym
#endif
						(library_, Procedure::name());
				return reinterpret_cast<typename Procedure::signature>(procedures_[index]);
			}
		private:
#ifdef ASCENSION_OS_WINDOWS
			HMODULE library_;
			typedef FARPROC NativeProcedure;
#else
			void* const library_;
			typedef void* NativeProcedure;
#endif
			mutable NativeProcedure procedures_[ProcedureEntries::NUMBER_OF_ENTRIES];
		};

#define ASCENSION_DEFINE_SHARED_LIB_ENTRIES(libraryName, numberOfProcedures)	\
	struct libraryName {														\
		static const std::size_t NUMBER_OF_ENTRIES = numberOfProcedures;		\
		template<std::size_t index> struct Procedure;							\
	}
#define ASCENSION_SHARED_LIB_ENTRY(libraryName, index, procedureName, procedureSignature)	\
	template<> struct libraryName::Procedure<index> {										\
		static const char* name() {return procedureName;}									\
		typedef procedureSignature;															\
	}

	}
}

#endif // !SHARED_LIBRARY_HPP
