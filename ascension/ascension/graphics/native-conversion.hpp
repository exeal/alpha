/**
 * @file native-conversion.hpp
 * Defines basic data types for geometry.
 * @author exeal
 * @date 2010-2014 was geometry.hpp
 * @date 2014-04-29 Separated from geometry.hpp
 */

#ifndef ASCENSION_NATIVE_CONVERSION_HPP
#define ASCENSION_NATIVE_CONVERSION_HPP

namespace ascension {
	namespace graphics {
		/// @defgroup graphics_native_conversion Conversion of Graphics Objects From/To Natives
		/// Provides free functions convert a graphics object between platform-independent and platform-dependent.
		/// @{
		/**
		 * Converts a platform-native graphics object into a platform-independent.
		 * @tparam Model The return type
		 * @tparam Native The type of @a native
		 * @param native The native object to convert
		 * @return The converted platform-independent object
		 * @see #toNative
		 */
		template<typename Model, typename Native> inline Model&& fromNative(const Native& object) {
			return detail::fromNative<Model>(object);
		}

		/**
		 * Converts a platform-independent graphics object into a platform-native.
		 * @tparam Native The return type
		 * @tparam Model The type of @a g
		 * @param object The platform-independent object to convert
		 * @return The converted native object
		 * @see #fromNative
		 */
		template<typename Native, typename Model> inline Native&& toNative(const Model& object) {
			return detail::toNative(object, static_cast<const Native*>(nullptr));
		}
		/// @}
	}
}

#endif // !ASCENSION_GEOMETRY_HPP
