/**
 * @file object.hpp
 * Defines base classes of graphics objects.
 */

#ifndef ASCENSION_GRAPHICS_OBJECT_HPP
#define ASCENSION_GRAPHICS_OBJECT_HPP
#include <boost/noncopyable.hpp>

namespace ascension {
	namespace graphics {
		/**
		 * Indicates @a Derived is a wrapper for a native graphics object.
		 * @tparam Derived The derived type
		 */
		template<typename Derived> class Wrapper : private boost::noncopyable {};
	}
}

#endif // !ASCENSION_GRAPHICS_OBJECT_HPP
