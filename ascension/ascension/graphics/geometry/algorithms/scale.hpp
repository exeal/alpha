/**
 * @file scale.hpp
 * Defines @c ascension#graphics#geometry#scale free function.
 * @author exeal
 * @date 2015-10-26 Separated from algorithms.hpp.
 */

#ifndef ASCENSION_GEOMETRY_SCALE_HPP
#define ASCENSION_GEOMETRY_SCALE_HPP
#include <ascension/graphics/geometry/named-parameters.hpp>
#include <boost/geometry/algorithms/transform.hpp>
#include <boost/geometry/strategies/transform/matrix_transformers.hpp>

namespace ascension {
	namespace graphics {
		namespace geometry {
			/**
			 * Calls @c boost#geometry#transform with @c boost#geometry#strategy#transform#scale_transformer.
			 * @tparam Arguments The type of @a arguments
			 * @param arguments The named parameters
			 * @return The returned value of @c boost#geometry#transform
			 */
#ifndef ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
			BOOST_PARAMETER_FUNCTION(
				(bool), scale, tag,
				(required
					(from, *)
					(out(to), *)
					(sx, *, 1)
					(sy, *, 1))) {
				return boost::geometry::transform(
					from, to,
					boost::geometry::strategy::transform::scale_transformer<
						boost::geometry::coordinate_type<std::decay<decltype(from)>::type>::type,
						boost::geometry::dimension<decltype(from)>::value,
						boost::geometry::dimension<decltype(to)>::value
					>(sx, sy));
			}
#else
			template<typename From, typename To, typename Sx, typename Sy>
			inline bool scale(const From& from, To& to, Sx sx = 1, Sy sy = 1);
#endif // !ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING

			/**
			 * Calls @c boost#geometry#transform with @c boost#geometry#strategy#transform#scale_transformer.
			 * @tparam Geometry The type of @a g
			 * @tparam Arguments The type of @a arguments
			 * @param g The geometry to scale
			 * @param arguments The named parameters. 'sx' : The x coordinate of the scale factor. 'sy' : The y
			 *        coordinate of the scale factor.
			 * @return The returned value of @c boost#geometry#transform
			 */
			template<typename Geometry, typename Arguments>
			inline bool scale(Geometry& g, const Arguments& arguments) {
				return scale(_from = g, _to = g, _sx = arguments[_sx | 1], _sy = arguments[_sy | 1]);
			}
		}
	}
}

#endif // !ASCENSION_GEOMETRY_TRANSLATE_HPP
