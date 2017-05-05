/**
 * @file native-wrappers.hpp
 * Defines trivial @c SharedWrapper and @c UniqueWrapper classes.
 * @author exeal
 * @date 2014-09-14 Created.
 * @date 2017-05-05 Renamed from graphics/object.hpp
 */

#ifndef ASCENSION_NATIVE_WRAPPERS_HPP
#define ASCENSION_NATIVE_WRAPPERS_HPP
#include <boost/config.hpp>

namespace ascension {
	/// @defgroup native_wrappers Native Wrappers
	/// Thin wrappers of platform-native objects.
	/// @{

	/**
	 * Indicates @a Derived is a thin shared wrapper for a platform-native object.
	 * An instance of the derived type is just a pointer or handle to the native object.
	 * @tparam Derived The derived type
	 */
	template<typename Derived>
	class SharedWrapper {};

	/**
	 * Indicates @a Derived is a thin wrapper for a platform-native object.
	 * An instance of the derived type is just a pointer or handle to the native object, but not copyable.
	 * @tparam Derived The derived type
	 */
	template<typename Derived>
	class UniqueWrapper {
	public:
#ifndef BOOST_NO_CXX11_DEFAULTED_FUNCTIONS
		UniqueWrapper() = default;
		UniqueWrapper(UniqueWrapper&&) = default;
		UniqueWrapper& operator=(UniqueWrapper&&) = default;
#else
		UniqueWrapper() {}
		UniqueWrapper(UniqueWrapper&&) {}
		UniqueWrapper& operator=(UniqueWrapper&&) {return *this;}
#endif

#ifndef BOOST_NO_CXX11_DELETED_FUNCTIONS
		UniqueWrapper(const UniqueWrapper&) = delete;
		UniqueWrapper& operator=(const UniqueWrapper&) = delete;
#else
	private:
		UniqueWrapper(const UniqueWrapper&);
		UniqueWrapper& operator=(const UniqueWrapper&);
#endif
	};

	/// @}
}

#endif // !ASCENSION_NATIVE_WRAPPERS_HPP
