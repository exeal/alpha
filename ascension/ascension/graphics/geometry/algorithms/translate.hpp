/**
 * @file translate.hpp
 * Defines @c ascension#graphics#geometry#translate free function.
 * @author exeal
 * @date 2015-10-26 Separated from algorithms.hpp.
 */

#ifndef ASCENSION_GEOMETRY_TRANSLATE_HPP
#define ASCENSION_GEOMETRY_TRANSLATE_HPP
#include <ascension/graphics/geometry/named-parameters.hpp>
#include <boost/geometry/algorithms/transform.hpp>
#include <boost/geometry/strategies/transform/matrix_transformers.hpp>

namespace ascension {
	namespace graphics {
		namespace geometry {
			/**
			 * Calls @c boost#geometry#transform with @c boost#geometry#strategy#transform#translate_transformer.
			 * @param from The source geometry
			 * @param to The destination geometry
			 * @param tx The x coordinate of the translation, or 0
			 * @param ty The y coordinate of the translation, or 0
			 * @return The returned value of @c boost#geometry#transform
			 */
#ifndef ASCENSION_DOXYGEN_SHOULD_SKIP_THIS
			BOOST_PARAMETER_FUNCTION(
				(bool), translate, tag,
				(required
					(from, *)
					(out(to), *)
					(tx, *, 0)
					(ty, *, 0))) {
				return boost::geometry::transform(
					from, to,
					boost::geometry::strategy::transform::translate_transformer<
						boost::geometry::coordinate_type<std::decay<decltype(from)>::type>::type,
						boost::geometry::dimension<decltype(from)>::value,
						boost::geometry::dimension<decltype(to)>::value
					>(tx, ty));
			}
#else
			template<typename From, typename To, typename Tx, typename Ty>
			bool translate(const From& from, To& to, Tx tx = 0, Ty ty = 0);
#endif

			/**
			 * Calls @c boost#geometry#transform with @c boost#geometry#strategy#transform#translate_transformer.
			 * @tparam Geometry The type of @a g
			 * @tparam Delta The type of @a delta
			 * @param g The geometry to translate
			 * @param delta The distance to translate
			 * @return The returned value of @c boost#geometry#transform
			 */
			template<typename Geometry, typename Delta>
			inline bool translate(Geometry& g, const Delta& delta, typename detail::EnableIfTagIs<Delta, DimensionTag>::type* = nullptr) {
				return translate(_from = g, _to = g, _tx = dx(delta), _ty = dy(delta));
			}

			/**
			 * Calls @c boost#geometry#transform with @c boost#geometry#strategy#transform#translate_transformer.
			 * @tparam Geometry The type of @a g
			 * @tparam Delta The type of @a delta
			 * @param g The geometry to translate
			 * @param delta The named parameters. 'tx' : The x coordinate of the translation. 'ty' : The y coordinate
			 *              of the translation
			 * @return The returned value of @c boost#geometry#transform
			 */
			template<typename Geometry, typename Delta>
			inline bool translate(Geometry& g, const Delta& delta, typename detail::DisableIfTagIs<Delta, DimensionTag>::type* = nullptr) {
				return translate(_from = g, _to = g, _tx = delta[_tx | 0], _ty = delta[_ty | 0]);
			}

			/**
			 * Calls @c boost#geometry#transform with @c boost#geometry#strategy#transform#translate_transformer.
			 * @tparam Geometries The type of @a geometries
			 * @tparam Delta The type of @a delta
			 * @param geometries The named parameters. 'from' : The source geometry. 'to' : The destination geometry
			 * @param delta The distance to translate
			 * @return The returned value of @c boost#geometry#transform
			 */
			template<typename Geometries, typename Delta>
			inline bool translate(const Geometries& geometries, const Delta& delta, typename detail::EnableIfTagIs<Delta, DimensionTag>::type* = nullptr) {
				return translate(_from = geometries[_from], _to = geometries[_to], _tx = dx(delta), _ty = dy(delta));
			}
		}
	}
}

#endif // !ASCENSION_GEOMETRY_TRANSLATE_HPP
