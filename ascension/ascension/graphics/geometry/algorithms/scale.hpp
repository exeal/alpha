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
			template<typename Arguments>
			inline bool scale(const Arguments& arguments) {
				return boost::geometry::transform(
					arguments[_from], arguments[_to],
					boost::geometry::strategy::transform::scale_transformer<
						boost::geometry::coordinate_type<std::decay<decltype(arguments[_from])>::type>::type,
						boost::geometry::dimension<decltype(arguments[_from])>::value,
						boost::geometry::dimension<decltype(arguments[_to])>::value
					>(arguments[_dx], arguments[_dy]));
			}

			/**
			 * Calls @c boost#geometry#transform with @c boost#geometry#strategy#transform#scale_transformer.
			 * @tparam Arguments The type of @a arguments
			 * @tparam Delta The type of @a delta
			 * @param arguments The named parameters
			 * @param delta The distance to translate
			 * @return The returned value of @c boost#geometry#transform
			 */
			template<typename Arguments, typename Delta>
			inline bool scale(const Arguments& arguments, const Delta& delta, typename detail::EnableIfTagIs<Delta, DimensionTag>::type* = nullptr) {
				return scale((_from = arguments[_from], _to = arguments[_to], _dx = dx(delta), _dy(delta)));
			}
		}
	}
}

#endif // !ASCENSION_GEOMETRY_TRANSLATE_HPP
