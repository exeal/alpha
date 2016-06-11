/**
 * @file named-parameters.hpp
 * Defines the named parameters used in geometric APIs.
 * @note Clients does not need to include this header file.
 * @author exeal
 * @date 2015-10-24 Created.
 */

#ifndef ASCENSION_GEOMETRY_NAMED_PARAMETERS_HPP
#define ASCENSION_GEOMETRY_NAMED_PARAMETERS_HPP
#include <boost/parameter/name.hpp>

namespace ascension {
	namespace graphics {
		namespace geometry {
#ifndef ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
			// Point
			BOOST_PARAMETER_NAME(x)
			BOOST_PARAMETER_NAME(y)

			// Dimension
			BOOST_PARAMETER_NAME(dx)
			BOOST_PARAMETER_NAME(dy)

			// Rectangle
			BOOST_PARAMETER_NAME(left)
			BOOST_PARAMETER_NAME(top)
			BOOST_PARAMETER_NAME(right)
			BOOST_PARAMETER_NAME(bottom)

			// AffineTransform
			BOOST_PARAMETER_NAME(sx)
			BOOST_PARAMETER_NAME(shy)
			BOOST_PARAMETER_NAME(shx)
			BOOST_PARAMETER_NAME(sy)
			BOOST_PARAMETER_NAME(tx)
			BOOST_PARAMETER_NAME(ty)

			// other
			BOOST_PARAMETER_NAME(from)
			BOOST_PARAMETER_NAME(to)
#endif	// !ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
		}
	}
}

#endif // !ASCENSION_GEOMETRY_NAMED_PARAMETERS_HPP
