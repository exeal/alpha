/**
 * @file future.hpp
 * @author exeal
 * @date 2004-2012 was basic-types.hpp
 * @date 2012-02-12 separated from basic-types.hpp
 */

#ifndef ASCENSION_FUTURE_HPP
#define ASCENSION_FUTURE_HPP

#include <cstddef>
//#include <ascension/config.hpp>
//#include <ascension/platforms.hpp>

namespace ascension {
	template<typename T, std::size_t n>
	/*constexpr*/ inline std::size_t countof(T (&a)[n]) {
		return n;
	}
	template<typename T, std::size_t n>
	/*constexpr*/ inline T* endof(T (&a)[n]) {
		return a + countof(a);
	}
	template<typename T, std::size_t n>
	/*constexpr*/ inline const T* endof(const T (&a)[n]) {
		return a + countof(a);
	}
}

/// Returns the number of the elements of the given array.
#define ASCENSION_COUNTOF(array) (sizeof(array) / sizeof((array)[0]))
/// Returns the end of the given array.
#define ASCENSION_ENDOF(array) ((array) + ASCENSION_COUNTOF(array))

/// Starts scoped enum in C++ 11 emulation.
#define ASCENSION_SCOPED_ENUM_PROLOGUE(typeName)	\
	class typeName {								\
		typedef Self;								\
		enum Values
/// Ends scoped enum in C++ 11 emulation.
#define ASCENSION_SCOPED_ENUM_EPILOGUE()											\
	;																				\
		Self() : value_() {}														\
		Self(Values value) : value_(value) {}										\
		operator Values() const {return value_;}									\
		bool operator==(const Self& other) const {return value_ == other.value_;}	\
		bool operator!=(const Self& other) const {return value_ != other.value_;}	\
		bool operator==(const Values& other) const {return value_ == other.value_;}	\
		bool operator!=(const Values& other) const {return value_ != other.value_;}	\
	private:																		\
		Values value_;																\
	};

namespace ascension {
	namespace detail {
		template<unsigned> struct StaticAssertTest {};
		template<int> struct StaticAssertionFailureAtLine;
		template<> struct StaticAssertionFailureAtLine<-1> {};
	} // namespace detail

	#define ASCENSION_STATIC_ASSERT(expression)														\
		typedef ascension::detail::StaticAssertTest<												\
			sizeof(ascension::detail::StaticAssertionFailureAtLine<(expression) ? -1 : __LINE__>)	\
		> oh_static_assertion_shippaidayo_orz
}

#endif // !ASCENSION_FUTURE_HPP
