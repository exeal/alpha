/**
 * @file native-conversion.hpp
 * Defines @c fromNative and @c toNative free functions.
 * @author exeal
 * @date 2010-2014 was geometry.hpp
 * @date 2014-04-29 Separated from geometry.hpp
 * @date 2017-01-01 Renamed from graphics/native-conversion.hpp.
 */

#ifndef ASCENSION_NATIVE_CONVERSION_HPP
#define ASCENSION_NATIVE_CONVERSION_HPP

namespace ascension {
	/// @defgroup native_conversion Conversion of Objects From/To Natives
	/// Provides free functions convert objects between platform-independent and platform-dependent.
	/// @see ascension#graphics, ascension#viewer#widgetapi
	/// @{
	/**
	 * Converts a platform-native object into a platform-independent.
	 * @tparam Model The return type
	 * @tparam Native The type of @a native
	 * @param native The native object to convert
	 * @return The converted platform-independent object
	 * @see #toNative
	 */
	template<typename Model, typename Native> inline Model fromNative(const Native& native) {
		return _fromNative(native, static_cast<const Model*>(nullptr));
	}

	/**
	 * Converts a platform-independent object into a platform-native.
	 * @tparam Native The return type
	 * @tparam Model The type of @a g
	 * @param object The platform-independent object to convert
	 * @return The converted native object
	 * @see #fromNative
	 */
	template<typename Native, typename Model> inline Native toNative(const Model& object) {
		return _toNative(object, static_cast<const Native*>(nullptr));
	}
	/// @}
}

#endif // !ASCENSION_NATIVE_CONVERSION_HPP
