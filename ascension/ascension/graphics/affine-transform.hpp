/**
 * @file affine-transform.hpp
 * @author exeal
 * @date 2011-11-20 created
 */

#ifndef ASCENSION_AFFINE_TRANSFORM_HPP
#define ASCENSION_AFFINE_TRANSFORM_HPP
#include <ascension/graphics/geometry.hpp>
//#include <boost/math/constants/constants.hpp>	// boost.math.constants.pi
#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
#	include <cairomm/matrix.h>
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
#	include <CGAffineTransform.h>
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
#	include <QMatrix>
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
#	include <ascension/win32/windows.hpp>
#endif

namespace ascension {
	namespace graphics {
		namespace geometry {
#ifndef ASCENSION_DOXYGEN_SHOULD_SKIP_THIS
			BOOST_PARAMETER_NAME(sx)
			BOOST_PARAMETER_NAME(shy)
			BOOST_PARAMETER_NAME(shx)
			BOOST_PARAMETER_NAME(sy)
			BOOST_PARAMETER_NAME(tx)
			BOOST_PARAMETER_NAME(ty)
#endif	// !ASCENSION_DOXYGEN_SHOULD_SKIP_THIS

			/// Base type for @c AffineTransform class.
			class AffineTransformBase {
			public:
				typedef double value_type;
			public:
				/// @name Coordinate Accesses
				/// @{
				value_type scaleX() const BOOST_NOEXCEPT;
				value_type scaleY() const BOOST_NOEXCEPT;
				value_type shearX() const BOOST_NOEXCEPT;
				value_type shearY() const BOOST_NOEXCEPT;
				value_type translateX() const BOOST_NOEXCEPT;
				value_type translateY() const BOOST_NOEXCEPT;
				/// @}
			protected:
				AffineTransformBase() BOOST_NOEXCEPT;
				AffineTransformBase(value_type sx, value_type shy, value_type shx,
					value_type sy, value_type tx, value_type ty) BOOST_NOEXCEPT;
				template<typename Arguments>
				AffineTransformBase(const Arguments& arguments);
			private:
				value_type scaleX_, scaleY_, shearX_, shearY_, translateX_, translateY_;
			};

			/// Represents a 2D affine transform.
			class AffineTransform : public AffineTransformBase {
			public:
				/// Constructs a new @c AffineTransform representing the Identity transformation.
				AffineTransform() {}
				BOOST_PARAMETER_CONSTRUCTOR(
					AffineTransform, (AffineTransformBase), tag,
					(required
						(sx, (value_type))
						(shy, (value_type))
						(shx, (value_type))
						(sy, (value_type))
						(tx, (value_type))
						(ty, (value_type))))

#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
				AffineTransform(const Cairo::Matrix& native) :
					AffineTransformBase(native.xx, native.yx, native.xy, native.yy, native.x0, native.y0) {}
				operator Cairo::Matrix() const {
					return Cairo::Matrix(scaleX(), shearY(), shearX(), scaleY(), translateX(), translateY());
				}
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
				AffineTransform(const CGAffineTransform& native) :
					AffineTransformBase(native.a, native.b, native.c, native.d, native.tx, native.ty) {}
				operator CGAffineTransform() const {
					return ::CGAffineTransformMake(scaleX(), shearY(), shearX(), scaleY(), translateX(), translateY());
				}
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
				AffineTransform(const QMatrix& native) :
					AffineTransformBase(native.m11(), native.m21(), native.m12(), native.m22, native.dx(), native.dy()) {}
				operator QMatrix() const {
					return QMatrix(scaleX(), shearY(), shearX(), scaleY(), translateX(), translateY());
				}
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
				AffineTransform(const XFORM& native) :
					AffineTransformBase(native.eM11, native.eM21, native.eM12, native.eM22, native.eDx, native.eDy) {}
				operator XFORM() const {
					XFORM temp = {
						static_cast<FLOAT>(scaleX()), static_cast<FLOAT>(shearX()),
						static_cast<FLOAT>(shearY()), static_cast<FLOAT>(scaleY()),
						static_cast<FLOAT>(translateX()), static_cast<FLOAT>(translateY())
					};
					return temp;
				}
#else
#endif

				/// @name Factories For Known Transformations
				/// @{
				static AffineTransform&& quadrantRotation(int numberOfQuadrants);
				template<typename Geometry>
				static AffineTransform&& quadrantRotation(int numberOfQuadrants, const Geometry& anchor,
					typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr);
				static AffineTransform&& rotation(value_type thetaInRadians);
				template<typename Geometry>
				static AffineTransform&& rotation(value_type thetaInRadians, const Geometry& anchor,
					typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr);
				static AffineTransform&& rotation(const BasicDimension<value_type>& rotationVector);
				template<typename Geometry>
				static AffineTransform&& rotation(
					const BasicDimension<value_type>& rotationVector, const Geometry& anchor,
					typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr);
				static AffineTransform&& scaling(value_type sx, value_type sy) BOOST_NOEXCEPT;
				static AffineTransform&& shearing(value_type shx, value_type shy) BOOST_NOEXCEPT;
				static AffineTransform&& translation(value_type tx, value_type ty) BOOST_NOEXCEPT;
				/// @}

				/// @name Attributes
				/// @{
				value_type determinant() const BOOST_NOEXCEPT;
				bool isIdentity() const BOOST_NOEXCEPT;
				/// @}

				/// @name Inverse Transformations
				/// @{
				AffineTransform& invert();
//				bool isInvertible() const;
				/// @}

				/// @name Generic Concatenations
				/// @{
				AffineTransform& concatenate(const AffineTransform& tx);
				AffineTransform& preConcatenate(const AffineTransform& tx);
				/// @}

				/// @name Known Concatenations
				/// @{
				AffineTransform& quadrantRotate(int numberOfQuadrants);
				template<typename Geometry>
				AffineTransform& quadrantRotate(int numberOfQuadrants, const Geometry& anchor,
					typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr);
				AffineTransform& rotate(value_type thetaInRadians);
				template<typename Geometry>
				AffineTransform& rotate(value_type thetaInRadians, const Geometry& anchor,
					typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr);
				inline AffineTransform& rotate(const BasicDimension<value_type>& rotationVector);
				template<typename Geometry>
				inline AffineTransform& rotate(
					const BasicDimension<value_type>& rotationVector, const Geometry& anchor,
					typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr);
				AffineTransform& scale(value_type sx, value_type sy);
//				AffineTransform& scaleNonUniform(value_type sx, value_type sy);
				AffineTransform& shear(value_type shx, value_type shy);
				AffineTransform& translate(value_type tx, value_type ty);
				/// @}

				/// @name Transformations
				/// @{
				template<typename Geometry>
				Geometry&& deltaTransform(const Geometry& p,
					typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) const;
				template<typename Geometry>
				Geometry&& inverseTransform(const Geometry& p,
					typename detail::EnableIfTagIs<Geometry, boost::geometry::point_tag>::type* = nullptr) const;
				template<typename Geometry>
				Geometry& transformShape(const AffineTransform& tx, Geometry& shape);
				template<typename Geometry>
				Geometry&& transformedShape(const AffineTransform& tx, const Geometry& shape);
				/// @}
			};

//			/// Affine transform identifying tag.
//			struct AffineTransformTag {};
		}

		using geometry::AffineTransform;
	}
}

#endif // !ASCENSION_AFFINE_TRANSFORM_HPP
