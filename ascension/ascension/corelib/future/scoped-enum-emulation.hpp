/**
 * @file scoped-enum-emulation.hpp
 * Defines macros emulate C++11 scoped enums.
 * @author exeal
 * @date 2012-05-31 Separated from future.hpp
 */

#ifndef ASCENSION_SCOPED_ENUM_EMULATION_HPP_
#define ASCENSION_SCOPED_ENUM_EMULATION_HPP_
#include <boost/config.hpp>
#include <boost/core/scoped_enum.hpp>
#ifdef BOOST_NO_CXX11_SCOPED_ENUMS
#	include <boost/functional/hash.hpp>
#endif // BOOST_NO_CXX11_SCOPED_ENUMS

namespace ascension {
	namespace detail {
		template<typename T, typename = void>
		struct isBoostScopedEnum : std::false_type {};
		template<typename T>
		struct isBoostScopedEnum<T, typename T::is_boost_scoped_enum_tag> : std::true_type {};
	}
}

/**
 * Forward declares an scoped enum.
 * @param name The name of the scoped enum
 */
#define ASCENSION_SCOPED_ENUM_FORWARD_DECLARE(name)	\
	BOOST_SCOPED_ENUM_FORWARD_DECLARE(name)

/**
 * Starts a declaration of a scoped enum.
 * @param name The name of the scoped enum
 * @param underlyingTypeName The name of the underlying type
 */
#define ASCENSION_SCOPED_ENUM_UT_DECLARE_BEGIN(name, underlyingTypeName)	\
	BOOST_SCOPED_ENUM_UT_DECLARE_BEGIN(name, underlyingTypeName) {

/**
 * Starts a declaration of a scoped enum.
 * @param name The name of the scoped enum
 */
#define ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(name)	\
	BOOST_SCOPED_ENUM_DECLARE_BEGIN(name) {

#ifdef BOOST_NO_CXX11_SCOPED_ENUMS

	/**
	 * Ends a declaration of a scoped enum.
	 * @param name The name of the scoped enum
	 */
#	define ASCENSION_SCOPED_ENUM_DECLARE_END(name)											\
		};																					\
		self_type(enum_type v) BOOST_NOEXCEPT : v_(v) {}									\
		self_type& operator=(underlying_type v) BOOST_NOEXCEPT {return (v_ = v), *this;}	\
		BOOST_SCOPED_ENUM_DECLARE_END2()													\
		inline std::size_t hash_value(const name& v) BOOST_NOEXCEPT {return boost::hash_value(boost::native_value(v));}

#else
#	define ASCENSION_SCOPED_ENUM_DECLARE_END(name) };

#endif // BOOST_NO_CXX11_SCOPED_ENUMS

#endif // !ASCENSION_SCOPED_ENUM_EMULATION_HPP_
