/**
 * @file internal.hpp
 * @brief Private entries used by Ascension internally.
 * @author exeal
 * @date 2006-2007
 */

#ifndef ASCENSION_INTERNAL_HPP
#define ASCENSION_INTERNAL_HPP
#include "common.hpp"
#include <list>
#include <stdexcept>
#include <algorithm>
#include "../../manah/object.hpp"
#ifdef _WIN32
#include <windows.h>
#endif /* _WIN32 */

namespace ascension {

	/**
	 * Defines entities the clients of Ascension do not access.
	 * @internal
	 */
	namespace internal {

		/// Integer constant to type mapping trick from Loki.Int2Type.
		template<int v> struct Int2Type {enum {value = v};};

		/// Returns true if @a T and @a U are same types.
		template<typename T, typename U> struct SameTypes {enum {result = 0};};
		template<typename T> struct SameTypes<T, T> {enum {result = 1};};

		/// Type selector from Loki.Select.
		template<bool expression, typename T, typename U> struct Select {typedef T Result;};
		template<typename T, typename U> struct Select<false, T, U> {typedef U Result;};

		/// Generates signed numeral types.
		template<typename T> struct ToSigned;
		template<> struct ToSigned<unsigned char> {typedef char Result;};
		template<> struct ToSigned<unsigned short> {typedef short Result;};
		template<> struct ToSigned<unsigned int> {typedef int Result;};
		template<> struct ToSigned<unsigned long> {typedef long Result;};
//		template<> struct ToSigned<unsigned __int64> {typedef __int64 Result;};

		/**
		 * Searches upper or lower bound.
		 * @param first the start of the range to search
		 * @param last the end of the range to search
		 * @param value the value to find
		 * @param get the function takes a @c Index parameter and returns a @c Value
		 * @param comp the comparation function takes two @c Value parameters and returns a boolean
		 * @return the bound
		 */
		template<typename Index, typename Value, typename Getter, typename Comparer>
		inline Index searchBound(Index first, Index last, const Value& value, Getter get, Comparer comp) throw() {
			assert(first <= last);
			Index c1 = last - first, c2, mid, p = first;
			while(c1 > 0) {
				c2 = c1 / 2, mid = p + c2;
				if(comp(get(mid), value))
					p = ++mid, c1 -= c2 + 1;
				else
					c1 = c2;
			}
			return (p != first) ? p - 1 : last;
		}

		/**
		 * Searches upper or lower bound.
		 * @param first the start of the range to search
		 * @param last the end of the range to search
		 * @param value the value to find
		 * @param get the function takes a @c Index parameter and returns a @c Value
		 * @return the bound
		 */
		template<typename Index, typename Value, typename Getter>
		inline Index searchBound(Index first, Index last, const Value& value, Getter get) throw() {
			return searchBound(first, last, value, get, std::less_equal<Value>());
		}

		/// Returns absolute difference of two numerals.
		template<typename T> inline std::size_t distance(T i0, T i1) {return (i0 > i1) ? i0 - i1 : i1 - i0;}

		/// Manages a strategy object.
		template<typename Strategy> class StrategyPointer {
			MANAH_NONCOPYABLE_TAG(StrategyPointer);
		public:
			StrategyPointer() throw() : pointee_(0), manages_(false) {}
			StrategyPointer(Strategy* pointee, bool manage) throw() : pointee_(pointee), manages_(manage) {}
			~StrategyPointer() {if(manages_) delete pointee_;}
			Strategy& operator*() const throw() {return *pointee_;}
			Strategy* operator->() const throw() {return get();}
			Strategy* get() const throw() {return pointee_;}
			void reset(Strategy* newValue, bool manage) {
				if(manages_ && newValue != pointee_) delete pointee_; pointee_ = newValue; manages_ = manage;}
			void reset() {reset(0, false);}
		private:
			Strategy* pointee_;
			bool manages_;
		};

		/// Manages the listeners.
		template<class Listener> class Listeners {
			MANAH_NONCOPYABLE_TAG(Listeners);
		public:
			Listeners() throw() {}
			void add(Listener& listener) {
				if(std::find(listeners_.begin(), listeners_.end(), &listener) != listeners_.end())
					throw std::invalid_argument("The listener already has been registered.");
				listeners_.push_back(&listener);
			}
			void remove(Listener& listener) {
				const Iterator i = std::find(listeners_.begin(), listeners_.end(), &listener);
				if(i == listeners_.end()) throw std::invalid_argument("The listener is not registered.");
				listeners_.erase(i);
			}
			void clear() throw() {listeners_.clear();}
			bool isEmpty() const throw() {return listeners_.empty();}
			void notify(void(Listener::*method)()) {
				for(Iterator i(listeners_.begin()), e(listeners_.end()), next; i != e; i = next) {next = i; ++next; ((*i)->*method)();}}
			template<typename Argument> void notify(void(Listener::*method)(Argument), Argument argument) {
				for(Iterator i(listeners_.begin()), e(listeners_.end()), next; i != e; i = next) {next = i; ++next; ((*i)->*method)(argument);}}
			template<typename Arg1, typename Arg2>
			void notify(void(Listener::*method)(Arg1, Arg2), Arg1 arg1, Arg2 arg2) {
				for(Iterator i(listeners_.begin()), e(listeners_.end()), next; i != e; i = next) {next = i; ++next;  ((*i)->*method)(arg1, arg2);}}
			template<typename Arg1, typename Arg2, typename Arg3>
			void notify(void(Listener::*method)(Arg1, Arg2, Arg3), Arg1 arg1, Arg2 arg2, Arg3 arg3) {
				for(Iterator i(listeners_.begin()), e(listeners_.end()), next; i != e; i = next)  {next = i; ++next; ((*i)->*method)(arg1, arg2, arg3);}}
			template<typename Arg1, typename Arg2, typename Arg3, typename Arg4>
			void notify(void(Listener::*method)(Arg1, Arg2, Arg3, Arg4), Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) {
				for(Iterator i(listeners_.begin()), e(listeners_.end()), next; i != e; i = next)  {next = i; ++next; ((*i)->*method)(arg1, arg2, arg3, arg4);}}
			template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
			void notify(void(Listener::*method)(Arg1, Arg2, Arg3, Arg4, Arg5), Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5) {
				for(Iterator i(listeners_.begin()), e(listeners_.end()), next; i != e; i = next) {next = i; ++next;  ((*i)->*method)(arg1, arg2, arg3, arg4, arg5);}}
		private:
			std::list<Listener*> listeners_;
			typedef typename std::list<Listener*>::iterator Iterator;
		};

#ifdef _WIN32
		template<class ProcedureEntries> class SharedLibrary {
			MANAH_NONCOPYABLE_TAG(SharedLibrary);
		public:
			explicit SharedLibrary(const char* fileName) : dll_(::LoadLibraryA(fileName)) {
				if(dll_ == 0)
					throw std::runtime_error("Cannot open the library.");
				std::fill(procedures_, procedures_ + ProcedureEntries::NUMBER_OF_ENTRIES, reinterpret_cast<FARPROC>(1));
			}
			~SharedLibrary() throw() {::FreeLibrary(dll_);}
			template<std::size_t index> typename ProcedureEntries::template Procedure<index>::signature get() const throw() {
				typedef typename ProcedureEntries::template Procedure<index> Procedure;
				if(procedures_[index] == reinterpret_cast<FARPROC>(1))
					procedures_[index] = ::GetProcAddress(dll_, Procedure::name());
				return reinterpret_cast<typename Procedure::signature>(procedures_[index]);
			}
		private:
			::HMODULE dll_;
			mutable ::FARPROC procedures_[ProcedureEntries::NUMBER_OF_ENTRIES];
		};
#else
#endif

#define ASCENSION_DEFINE_SHARED_LIB_ENTRIES(libraryName, numberOfProcedures)	\
	struct libraryName {														\
		enum {NUMBER_OF_ENTRIES = numberOfProcedures};							\
		template<std::size_t index> struct Procedure;							\
	}
#define ASCENSION_SHARED_LIB_ENTRY(libraryName, index, procedureName, procedureSignature)	\
	template<> struct libraryName::Procedure<index> {										\
		static const char* name() {return procedureName;}									\
		typedef procedureSignature;															\
	}

	} // namespace internal

	/// Signed @c length_t
	typedef internal::ToSigned<length_t>::Result signed_length_t;

} // namespace ascension

#endif /* !ASCENSION_INTERNAL_HPP */
