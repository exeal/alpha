/**
 * @file common.hpp
 * Defines common data types for geometry.
 * @author exeal
 * @date 2010-11-06 Created (as geometry.hpp)
 * @date 2014-09-07 Separated from geometry.hpp
 */

#ifndef ASCENSION_GEOMETRY_COMMON_HPP
#define ASCENSION_GEOMETRY_COMMON_HPP

#include <boost/geometry/core/tag.hpp>
#include <type_traits>

namespace ascension {
	namespace graphics {
		namespace geometry {

			/**
			 * @defgroup geometric_primitives Geometric Primitives
			 * Basic primitives of @c ascension#graphics#geometry.
			 * @{
			 */
			/// A scalar value is a length in user space which is "logical coordinates" in Win32 GDI.
			typedef float Scalar;
			/// @}

			namespace detail {
				template<typename Geometry, typename GeometryTag, typename T = void>
				struct EnableIfTagIs : std::enable_if<
					std::is_same<
						typename boost::geometry::tag<typename std::remove_cv<Geometry>::type>::type,
						GeometryTag
					>::value, T> {};
			}
		}

		using geometry::Scalar;
	}
}

#endif // !ASCENSION_GEOMETRY_COMMON_HPP
