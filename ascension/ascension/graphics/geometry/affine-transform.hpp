/**
 * @file affine-transform.hpp
 * @author exeal
 * @date 2011-11-20 created
 */

#ifndef ASCENSION_AFFINE_TRANSFORM_HPP
#define ASCENSION_AFFINE_TRANSFORM_HPP

#include <ascension/graphics/geometry/common.hpp>
#include <ascension/graphics/geometry/named-parameters.hpp>
#include <boost/functional/hash.hpp>
#include <boost/geometry/strategies/transform/matrix_transformers.hpp>
#include <boost/operators.hpp>
#include <boost/parameter.hpp>
#include <boost/range/algorithm/equal.hpp>
//#include <boost/math/constants/constants.hpp>	// boost.math.constants.pi
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(CAIRO)
#	include <cairomm/matrix.h>
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(CORE_GRAPHICS)
#	include <CGAffineTransform.h>
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(QT)
#	include <QMatrix>
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(WIN32_GDI)
#	include <ascension/win32/windows.hpp>
#endif

namespace ascension {
	namespace graphics {
		namespace geometry {
			/**
			 * @defgroup affine_transform Affine Transform
			 * @{
			 */

			/// Represents 2D affine transform.
			typedef boost::geometry::strategy::transform::ublas_transformer<double, 2, 2> AffineTransform;

			/**
			 * @defgroup affine_transform_accesors Accesors
			 * @{
			 */
			/// Returns the X coordinate scaling element.
			inline double scaleX(const AffineTransform& tx) BOOST_NOEXCEPT {return tx.matrix()(0, 0);}
			/// Returns the Y coordinate scaling element.
			inline double scaleY(const AffineTransform& tx) BOOST_NOEXCEPT {return tx.matrix()(1, 1);}
			/// Returns the X coordinate shearing element.
			inline double shearX(const AffineTransform& tx) BOOST_NOEXCEPT {return tx.matrix()(0, 1);}
			/// Returns the Y coordinate shearing element.
			inline double shearY(const AffineTransform& tx) BOOST_NOEXCEPT {return tx.matrix()(1, 0);}
			/// Returns the X coordinate translation element.
			inline double translateX(const AffineTransform& tx) BOOST_NOEXCEPT {return tx.matrix()(0, 2);}
			/// Returns the Y coordinate translation element.
			inline double translateY(const AffineTransform& tx) BOOST_NOEXCEPT {return tx.matrix()(1, 2);}
			/// @}

			/**
			 * @defgroup affine_transform_instances Factories For @c AffineTransform
			 * @{
			 */

			/**
			 * Creates a new @c AffineTransform instance with the given coordinates.
			 * @param sx The X coordinate scaling element
			 * @param sy The Y coordinate scaling element
			 * @param shx The X coordinate shearing element
			 * @param shy The Y coordinate shearing element
			 * @param tx The X coordinate translation element
			 * @param ty The Y coordinate translation element
			 * @return A @c AffineTransform instance
			 */
#ifndef ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
			BOOST_PARAMETER_FUNCTION(
					(AffineTransform), makeAffineTransform, tag,
					(required (sx, *) (sy, *) (shx, *) (shy, *) (tx, *) (ty, *))) {
				return AffineTransform(
					sx, shx, tx,	// m00 m01 m02
					shy, sy, ty,	// m10 m11 m12
					0, 0, 1);		// m20 m21 m22
			}
#else
			template<typename T>
			AffineTransform makeAffineTransform(T sx, T sy, T shx, T shy, T tx, T ty);
#endif // !ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING

			/**
			 * @defgroup affine_transform_factories_for_known_transforms Factories For Known Transformations
			 * @{
			 */

			/// Creates a new @c AffineTransform representing Identity transformation.
			inline AffineTransform makeIdentityTransform() {
				return makeAffineTransform(_sx = 1.0, _sy = 1.0, _shx = 0, _shy = 0, _tx = 0, _ty = 0);
			}

			/**
			 * Creates a transform that rotates coordinates by the specified number of quadrants.
			 * @param numberOfQuadrants The number of 90 degree arcs to rotate by
			 */
			inline AffineTransform makeQuadrantRotationTransform(int numberOfQuadrants) {
				switch(numberOfQuadrants % 3) {
			 		case 0:
						return makeIdentityTransform();
					case 1:
						return makeAffineTransform(_sx = 0.0, _sy = 0.0, _shx = -1.0, _shy = 1.0, _tx = 0.0, _ty = 0.0);
					case 2:
						return makeAffineTransform(_sx = -1.0, _sy = -1.0, _shx = 0.0, _shy = 0.0, _tx = 0.0, _ty = 0.0);
					case 3:
						return makeAffineTransform(_sx = 0.0, _sy = 0.0, _shx = 1.0, _shy = -1.0, _tx = 0.0, _ty = 0.0);
				}
				ASCENSION_ASSERT_NOT_REACHED();
			}
#if 0
			/**
			 * Creates a transform that rotates coordinates by the specified number of quadrants around the specified
			 * anchor point.
			 * @tparam Geometry The type of @a anchor
			 * @param numberOfQuadrants The number of 90 degree arcs to rotate by
			 * @param anchor The rotation anchor point
			 */ 
			template<typename Geometry>
			inline AffineTransform makeQuadrantRotationTransform(int numberOfQuadrants, const Geometry& anchor,
				typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr);
#endif
			/**
			 * Creates a transform representing a rotation transformation.
			 * @tparam DegreeOrRadian @c boost#geometry#degree or @c boost#geometry#radian
			 * @param thetaInRadians The angle of rotation measured in units specified by @a DegreeOrRadian
			 * @return An @c AffineTransform object that is a rotation transform, created with the specified angle of rotation
			 */
			template<typename DegreeOrRadian>
			inline AffineTransform makeRotationTransform(double thetaInRadians) {
				return AffineTransform(boost::geometry::strategy::transform::rotate_transformer<DegreeOrRadian, double, 2, 2>(thetaInRadians));
			}
#if 0
			/**
			 * Creates a transform that rotates coordinates around an anchor point.
			 * @tparam DegreeOrRadian @c boost#geometry#degree or @c boost#geometry#radian
			 * @tparam Geometry The type of @a anchor
			 * @param thetaInRadians The angle of rotation measured in units specified by @a DegreeOrRadian
			 * @param anchor The rotation anchor point
			 * @return An @c AffineTransform object that rotates coordinates around the specified angle of rotation
			 */
			template<typename DegreeOrRadian, typename Geometry>
			inline AffineTransform makeRotationTransform(double thetaInRadians, const Geometry& anchor,
				typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr);

			/**
			 * Creates a transform that rotates coorinates according to a rotation vector.
			 * @param rotationVector The rotation vector
			 * @return An @c AffineTransform object that rotates coordinates according to the specified rotation vector
			 */
			inline AffineTransform makeRotationTransform(const BasicDimension<double>& rotationVector);

			/**
			 * Creates a transform that rotates coorinates around an anchor point according to a rotation vector.
			 * @tparam Geometry The type of @a anchor
			 * @param rotationVector The rotation vector
			 * @param anchor The rotation anchor point
			 * @return An @c AffineTransform object that rotates coordinates around the specified point according to the specified rotation vector
			 */
			template<typename Geometry>
			inline AffineTransform makeRotationTransform(
				const BasicDimension<double>& rotationVector, const Geometry& anchor,
				typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr);
#endif
			/**
			 * @fn ascension::graphics::geometry::makeScalingTransform
			 * Creates a transform representing a scaling transformation.
			 * @param sx The factor by which coordinates are scaled along the X axis direction
			 * @param sy The factor by which coordinates are scaled along the Y axis direction
			 * @return An @c AffineTransform object that scales coordinates by the specified factors
			 */
#ifndef ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
			BOOST_PARAMETER_FUNCTION(
					(AffineTransform), makeScalingTransform, tag,
					(required (sx, *) (sy, *))) {
				return AffineTransform(boost::geometry::strategy::transform::scale_transformer<double, 2, 2>(sx, sy));
			}
#else
			template<typename T>
			AffineTransform makeScalingTransform(T sx, T sy);
#endif // !ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING

			/**
			 * @fn ascension::graphics::geometry::makeShearingTransform
			 * Creates a transform representing a shearing transformation.
			 * @param shx The multiplier by which coordinates are shifted in the direction of the positive X axis as a
			 *            factor of their Y coordinate
			 * @param shy The multiplier by which coordinates are shifted in the direction of the positive Y axis as a
			 *            factor of their X coordinate
			 * @return An @c AffineTransform object that shears coordinates by the specified multipliers
			 */
#ifndef ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
			BOOST_PARAMETER_FUNCTION(
					(AffineTransform), makeShearingTransform, tag,
					(required (shx, *) (shy, *))) {
				return makeAffineTransform(_sx = 1.0, _shx = shx, _shy = shy, _sy = 1.0, _tx = 0.0, _ty = 0.0);
			}
#else
			template<typename T>
			AffineTransform makeShearingTransform(T shx, T shy);
#endif // !ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING

			/**
			 * @fn ascension::graphics::geometry::makeTranslationTransform
			 * Creates a transform representing a translation transformation.
			 * @param tx The distance by which coordinates are translated in the X axis direction
			 * @param ty The distance by which coordinates are translated in the Y axis direction
			 * @return An @c AffineTransform object that represents a translation transformation, created with the specified vector
			 */
#ifndef ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
			BOOST_PARAMETER_FUNCTION(
					(AffineTransform), makeTranslationTransform, tag,
					(required (tx, *) (ty, *))) {
				return makeAffineTransform(_sx = 1.0, _sy = 1.0, _shx = 0.0, _shy = 0.0, _tx = tx, _ty = ty);
			}
#else
			template<typename T>
			AffineTransform makeTranslationTransform(T tx, T ty);
#endif // !ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
			/// @}
			/// @}

			/// @defgroup affine_transform_attributes Attributes
			/// @{
			/**
			 * Returns the determinant of the matrix representation of the transform.
			 * @see #inverseTransform
			 */
			inline double determinant(const AffineTransform& tx) BOOST_NOEXCEPT {
				return scaleX(tx) * scaleY(tx) - shearX(tx) * shearY(tx);
			}

			/// Returns @c true if the two represents the same affine coordinate transform.
			inline bool equals(const AffineTransform& lhs, const AffineTransform& rhs) {
				return boost::equal(lhs.matrix().data(), rhs.matrix().data());
			}

			/// Implements the hash function for @c AffineTransform.
			inline std::size_t hash_value(const AffineTransform& tx) BOOST_NOEXCEPT {
				std::size_t v = boost::hash_value(scaleX(tx));
				boost::hash_combine(v, scaleY(tx));
				boost::hash_combine(v, shearX(tx));
				boost::hash_combine(v, shearY(tx));
				boost::hash_combine(v, translateX(tx));
				boost::hash_combine(v, translateY(tx));
				return v;
			}

			/// Returns @c true if this is an identity transform.
			inline bool isIdentity(const AffineTransform& tx) BOOST_NOEXCEPT {
				return equals(tx, makeIdentityTransform());
			}
			/// @}
#if 0
			/**
			 * @defgroup affine_transform_inverse Inverse Transformations
			 * @{
			 */
			/**
			 * Sets this transform to the inverse of itself.
			 * @see #determinant
			 */
			AffineTransform& invert(AffineTransform& tx) {
				const double d = determinant(tx);
				if(std::abs(d) <= std::numeric_limits<double>::min())
					throw NoninvertibleTransformException();
				return tx = makeAffineTransform(
					_sx = scaleY(tx) / d, _sy = scaleX(tx) / d,
					_shx = -shearX(tx) / d, _shy = -shearY(tx) / d,
					_tx = (shearX(tx) * translateY(tx) - scaleY(tx) * translateX(tx)) / d,
					_ty = (shearY(tx) * translateX(tx) - scaleX(tx) * translateY(tx)) / d);
			}
			bool isInvertible() const;
			/// @}

			/// @defgroup affine_transform_generic_concatenations Generic Concatenations
			/// @{
			AffineTransform& concatenate(AffineTransform& lhs, const AffineTransform& rhs);
			AffineTransform& preConcatenate(AffineTransform& lhs, const AffineTransform& rhs);
			/// @}

			/// @defgroup affine_transform_known_concatenations Known Concatenations
			/// @{
			AffineTransform& quadrantRotate(AffineTransform& tx, int numberOfQuadrants);
			template<typename Geometry>
			AffineTransform& quadrantRotate(AffineTransform& tx, int numberOfQuadrants, const Geometry& anchor,
				typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr);
			AffineTransform& rotate(AffineTransform& tx, double thetaInRadians);
			template<typename Geometry>
			AffineTransform& rotate(AffineTransform& tx, double thetaInRadians, const Geometry& anchor,
				typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr);
			inline AffineTransform& rotate(AffineTransform& tx, const BasicDimension<double>& rotationVector);
			template<typename Geometry>
			inline AffineTransform& rotate(AffineTransform& tx,
				const BasicDimension<double>& rotationVector, const Geometry& anchor,
				typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr);
			AffineTransform& scale(AffineTransform& tx, double sx, double sy);
//			AffineTransform& scaleNonUniform(AffineTransform& tx, double sx, double sy);
			AffineTransform& shear(AffineTransform& tx, double shx, double shy);
			AffineTransform& translate(AffineTransform& tx, double tx, double ty);
			/// @}

			/// @defgroup affine_transform_transformations Transformations
			/// @{
			template<typename Geometry>
			Geometry deltaTransform(const AffineTransform& tx, const Geometry& g);
			template<typename Geometry>
			Geometry inverseTransform(const AffineTransform& tx, const Geometry& g);
			template<typename Geometry>
			Geometry transform(const AffineTransform& tx, const Geometry& g);
			/// @}
#endif

//			/// Affine transform identifying tag.
//			struct AffineTransformTag {};
		}

		// platform-dependent conversions
		namespace detail {
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(CAIRO)
			template<typename Geometry>
			inline Geometry fromNative(const Cairo::Matrix& native,
					typename std::enable_if<std::is_same<Geometry, geometry::AffineTransform>::value>::type* = nullptr) {
				return geometry::AffineTransform(
					native.xx, native.xy, native.x0,	// m00 m01 m02
					native.yx, native.yy, native.y0,	// m10 m11 m12
					0, 0, 1);							// m20 m21 m22
			}
			inline Cairo::Matrix toNative(const geometry::AffineTransform& tx, const Cairo::Matrix* = nullptr) {
				return Cairo::Matrix(
					geometry::scaleX(tx), geometry::shearY(tx),				// xx yx
					geometry::shearX(tx), geometry::scaleY(tx),				// xy yy
					geometry::translateX(tx), geometry::translateY(tx));	// x0 y0
			}
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(CORE_GRAPHICS)
			template<typename Geometry>
			inline Geometry fromNative(const CGAffineTransform& native,
					typename std::enable_if<std::is_same<Geometry, geometry::AffineTransform>::value>::type* = nullptr) {
				return geometry::AffineTransform(
					native.a, native.c, native.tx,	// m00 m01 m02
					native.b, native.d, native.ty,	// m10 m11 m12
					0, 0, 1);						// m20 m21 m22
			}
			inline CGAffineTransform toNative(const geometry::AffineTransform& tx, const CGAffineTransform* = nullptr) {
				return ::CGAffineTransformMake(
					geometry::scaleX(tx), geometry::shearY(tx),				// a  b
					geometry::shearX(tx), geometry::scaleY(tx),				// c  d
					geometry::translateX(tx), geometry::translateY(tx));	// tx ty
			}
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(QT)
			template<typename Geometry>
			inline Geometry fromNative(const QMatrix& native,
					typename std::enable_if<std::is_same<Geometry, geometry::AffineTransform>::value>::type* = nullptr) {
				return geometry::AffineTransform(
					native.m11(), native.m12(), native.dx(),	// m00 m01 m02
					native.m21(), native.m22(), native.dy(),	// m10 m11 m12
					0, 0, 1);									// m20 m21 m22
			}
			inline QMatrix toNative(const geometry::AffineTransform& tx, const QMatrix* = nullptr) {
				return QMatrix(
					geometry::scaleX(tx), geometry::shearY(tx),				// m11 m12
					geometry::shearX(tx), geometry::scaleY(tx),				// m21 m22
					geometry::translateX(tx), geometry::translateY(tx));	// dx  dy
			}
#endif
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(WIN32_GDI)
			template<typename Geometry>
			inline Geometry fromNative(const XFORM& native,
					typename std::enable_if<std::is_same<Geometry, geometry::AffineTransform>::value>::type* = nullptr) {
				return geometry::AffineTransform(
					native.eM11, native.eM21, native.eDx,	// m00 m01 m02
					native.eM12, native.eM22, native.eDy,	// m10 m11 m12
					0, 0, 1);								// m20 m21 m22
			}
			inline XFORM toNative(const geometry::AffineTransform& tx, const XFORM* = nullptr) {
				XFORM native;
				native.eM11 = static_cast<FLOAT>(geometry::scaleX(tx));
				native.eM12 = static_cast<FLOAT>(geometry::shearY(tx));
				native.eM21 = static_cast<FLOAT>(geometry::shearX(tx));
				native.eM22 = static_cast<FLOAT>(geometry::scaleY(tx));
				native.eDx = static_cast<FLOAT>(geometry::translateX(tx));
				native.eDy = static_cast<FLOAT>(geometry::translateY(tx));
				return native;
			}
#endif
		}

		using geometry::AffineTransform;
	}
}

#endif // !ASCENSION_AFFINE_TRANSFORM_HPP
