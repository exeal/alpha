/**
 * @file shared-library.hpp
 * @author exeal
 * @date 2011-04-01 separated from ../internal.hpp
 */

#ifndef SHARED_LIBRARY_HPP
#define SHARED_LIBRARY_HPP
#include <ascension/platforms.hpp>

#ifdef ASCENSION_OS_WINDOWS
#	include <ascension/win32/windows.hpp>	// LoadLibraryA, FreeLibrary, GetProcAddress, HMODULE
#	include <algorithm>	// std.fill
#	include <stdexcept>	// std.runtime_error
#else
#endif

namespace ascension {
	namespace detail {

#ifdef ASCENSION_OS_WINDOWS
		template<class ProcedureEntries>
		class SharedLibrary {
			ASCENSION_NONCOPYABLE_TAG(SharedLibrary);
		public:
			explicit SharedLibrary(const char* fileName) : dll_(::LoadLibraryA(fileName)) {
				if(dll_ == 0)
					throw std::runtime_error("Cannot open the library.");
				std::fill(procedures_, procedures_ + ProcedureEntries::NUMBER_OF_ENTRIES, reinterpret_cast<FARPROC>(1));
			}
			~SharedLibrary() /*throw()*/ {::FreeLibrary(dll_);}
			template<std::size_t index>
			typename ProcedureEntries::template Procedure<index>::signature get() const /*throw()*/ {
				typedef typename ProcedureEntries::template Procedure<index> Procedure;
				if(procedures_[index] == reinterpret_cast<FARPROC>(1))
					procedures_[index] = ::GetProcAddress(dll_, Procedure::name());
				return reinterpret_cast<typename Procedure::signature>(procedures_[index]);
			}
		private:
			HMODULE dll_;
			mutable FARPROC procedures_[ProcedureEntries::NUMBER_OF_ENTRIES];
		};
#else
#endif

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
