/**
 * @file affine-transform.hpp
 * @author exeal
 * @date 2011-11-20 created
 */

#ifndef ASCENSION_AFFINE_TRANSFORM_HPP
#define ASCENSION_AFFINE_TRANSFORM_HPP
#include <ascension/graphics/geometry.hpp>
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

			/// Affine transform identifying tag.
			struct AffineTransformTag {};

			const std::size_t SCALE_X_ELEMENT = 2;
			const std::size_t SCALE_Y_ELEMENT = 3;
			const std::size_t SHEAR_X_ELEMENT = 4;
			const std::size_t SHEAR_Y_ELEMENT = 5;
			const std::size_t TRANSLATE_X_ELEMENT = 6;
			const std::size_t TRANSLATE_Y_ELEMENT = 7;

#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
			namespace nativetypes {typedef Cairo::Matrix NativeAffineTransform;}
			template<> struct Coordinate<Cairo::Matrix> {typedef double Type;};
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
			namespace nativetypes {typedef CGAffineTransform NativeAffineTransform;}
			template<> struct Coordinate<CGAffineTransform> {typedef CGFloat Type;};
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
			namespace nativetypes {typedef QMatrix NativeAffineTransform;}
			template<> struct Coordinate<QMatrix> {typedef qreal Type;};
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
			namespace nativetypes {typedef XFORM NativeAffineTransform;}
			template<> struct Coordinate<XFORM> {typedef FLOAT Type;};
#endif
			template<> struct Tag<nativetypes::NativeAffineTransform> {
				typedef AffineTransformTag Type;
			};

			namespace traits {
				template<typename AffineTransform> struct Maker<AffineTransformTag, AffineTransform> {
					static AffineTransform make(
							typename Coordinate<AffineTransform>::Type scaleX,
							typename Coordinate<AffineTransform>::Type scaleY,
							typename Coordinate<AffineTransform>::Type shearX,
							typename Coordinate<AffineTransform>::Type shearY,
							typename Coordinate<AffineTransform>::Type translateX,
							typename Coordinate<AffineTransform>::Type translateY) {
#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
						return Cairomm::Matrix(scaleX, shearY, shearX, scaleY, translateX, translateY);
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
						return ::CGAffineTransformMake(scaleX, shearY, shearX, scaleY, translateX, translateY);
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
						return QMatrix(scaleX, shearY, shearX, scaleY, translateX, translateY);
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
						XFORM temp = {scaleX, shearX, shearY, scaleY, translateX, translateY};
						return temp;
#endif
					}
				};
				template<typename AffineTransform>
				struct Accessor<AffineTransformTag, AffineTransform, SCALE_X_ELEMENT> {
					static typename Coordinate<AffineTransform>::Type get(const AffineTransform& a) {
#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
						return a.xx;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
						return a.a;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
						return a.m11();
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
						return a.eM11;
#endif
					}
				};
				template<typename AffineTransform>
				struct Accessor<AffineTransformTag, AffineTransform, SCALE_Y_ELEMENT> {
					static typename Coordinate<AffineTransform>::Type get(const AffineTransform& a) {
#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
						return a.yy;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
						return a.d;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
						return a.m22();
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
						return a.eM22;
#endif
					}
				};
				template<typename AffineTransform>
				struct Accessor<AffineTransformTag, AffineTransform, SHEAR_X_ELEMENT> {
					static typename Coordinate<AffineTransform>::Type get(const AffineTransform& a) {
#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
						return a.xy;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
						return a.c;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
						return a.m12();
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
						return a.eM12;
#endif
					}
				};
				template<typename AffineTransform>
				struct Accessor<AffineTransformTag, AffineTransform, SHEAR_Y_ELEMENT> {
					static typename Coordinate<AffineTransform>::Type get(const AffineTransform& a) {
#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
						return a.yx;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
						return a.b;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
						return a.m21();
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
						return a.eM21;
#endif
					}
				};
				template<typename AffineTransform>
				struct Accessor<AffineTransformTag, AffineTransform, TRANSLATE_X_ELEMENT> {
					static typename Coordinate<AffineTransform>::Type get(const AffineTransform& a) {
#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
						return a.x0;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
						return a.tx;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
						return a.dx();
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
						return a.eDx;
#endif
					}
				};
				template<typename AffineTransform>
				struct Accessor<AffineTransformTag, AffineTransform, TRANSLATE_Y_ELEMENT> {
					static typename Coordinate<AffineTransform>::Type get(const AffineTransform& a) {
#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
						return a.y0;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
						return a.ty;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
						return a.dy();
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
						return a.eDy;
#endif
					}
				};
			} // namespace traits

			// known transformations //////////////////////////////////////////////////////////////

			template<typename AffineTransform>
			inline AffineTransform identityTransform();
			template<typename AffineTransform>
			inline AffineTransform quadrantRotationTransform(int numberOfQuadrants);
			template<typename AffineTransform, typename Point>
			inline AffineTransform quadrantRotationTransform(int numberOfQuadrants, const Point& anchor);
			template<typename AffineTransform>
			inline AffineTransform rotationTransform(
				typename Coordinate<AffineTransform>::Type thetaInRadians);
			template<typename AffineTransform, typename Point>
			inline AffineTransform rotationTransform(
				typename Coordinate<AffineTransform>::Type thetaInRadians, const Point& anchor);
			template<typename AffineTransform>
			inline AffineTransform rotationTransform(
				typename Coordinate<AffineTransform>::Type vectorX,
				typename Coordinate<AffineTransform>::Type vectorY);
			template<typename AffineTransform, typename Point>
			inline AffineTransform rotationTransform(
				typename Coordinate<AffineTransform>::Type vectorX,
				typename Coordinate<AffineTransform>::Type vectorY, const Point& anchor);
			template<typename AffineTransform>
			inline AffineTransform scalingTransform(
				typename Coordinate<AffineTransform>::Type sx,
				typename Coordinate<AffineTransform>::Type sy);
			template<typename AffineTransform>
			inline AffineTransform shearingTransform(
				typename Coordinate<AffineTransform>::Type shx,
				typename Coordinate<AffineTransform>::Type shy);
			template<typename AffineTransform>
			inline AffineTransform translationTransform(
				typename Coordinate<AffineTransform>::Type tx,
				typename Coordinate<AffineTransform>::Type ty);

			// attributes /////////////////////////////////////////////////////////////////////////

			template<typename AffineTransform>
			inline NativeSize deltaTransform(const AffineTransform& a);
			template<typename AffineTransform>
			inline typename Coordinate<AffineTransform>::Type determinant(const AffineTransform& a);
			template<typename AffineTransform>
			inline bool isIdentity(const AffineTransform& a,
				typename detail::EnableIfTagIs<AffineTransform, AffineTransformTag>::type* = nullptr);

			// inverse transformations ////////////////////////////////////////////////////////////

			template<typename AffineTransform>
			inline AffineTransform inverted(const AffineTransform& a,
				typename detail::EnableIfTagIs<AffineTransform, AffineTransformTag>::type* = nullptr);
			template<typename AffineTransform>
			inline bool isInvertible(const AffineTransform& a,
				typename detail::EnableIfTagIs<AffineTransform, AffineTransformTag>::type* = nullptr);

			// generic concatenations /////////////////////////////////////////////////////////////

			template<typename AffineTransform>
			inline AffineTransform concatenate(
				AffineTransform& lhs, const AffineTransform& rhs,
				typename detail::EnableIfTagIs<AffineTransform, AffineTransformTag>::type* = nullptr);
			template<typename AffineTransform>
			inline AffineTransform preConcatenate(
				const AffineTransform& lhs, AffineTransform& rhs,
				typename detail::EnableIfTagIs<AffineTransform, AffineTransformTag>::type* = nullptr);

			// known concatenations ///////////////////////////////////////////////////////////////

			template<typename AffineTransform>
			inline AffineTransform& quadrantRotate(AffineTransform& a, int numberOfQuadrants,
				typename detail::EnableIfTagIs<AffineTransform, AffineTransformTag>::type* = nullptr);
			template<typename AffineTransform>
			inline AffineTransform& quadrantRotate(AffineTransform& a, int numberOfQuadrants,
				typename Coordinate<AffineTransform>::Type x,
				typename Coordinate<AffineTransform>::Type y,
				typename detail::EnableIfTagIs<AffineTransform, AffineTransformTag>::type* = nullptr);
			template<typename AffineTransform>
			inline AffineTransform& rotate(AffineTransform& a,
				typename Coordinate<AffineTransform>::Type thetaInRadians,
				typename detail::EnableIfTagIs<AffineTransform, AffineTransformTag>::type* = nullptr);
			template<typename AffineTransform>
			inline AffineTransform& rotate(AffineTransform& a,
				typename Coordinate<AffineTransform>::Type thetaInRadians,
				typename Coordinate<AffineTransform>::Type x,
				typename Coordinate<AffineTransform>::Type y,
				typename detail::EnableIfTagIs<AffineTransform, AffineTransformTag>::type* = nullptr);
			template<typename AffineTransform>
			inline AffineTransform& scale(AffineTransform& a,
				typename Coordinate<AffineTransform>::Type sx,
				typename Coordinate<AffineTransform>::Type sy,
				typename detail::EnableIfTagIs<AffineTransform, AffineTransformTag>::type* = nullptr);
			template<typename AffineTransform>
			inline AffineTransform& scaleNonUniform(AffineTransform& a,
				typename Coordinate<AffineTransform>::Type sx,
				typename Coordinate<AffineTransform>::Type sy,
				typename detail::EnableIfTagIs<AffineTransform, AffineTransformTag>::type* = nullptr);
			template<typename AffineTransform>
			inline AffineTransform& scale(AffineTransform& a,
				typename Coordinate<AffineTransform>::Type factor,
				typename detail::EnableIfTagIs<AffineTransform, AffineTransformTag>::type* = nullptr);
			template<typename AffineTransform>
			inline AffineTransform& shear(AffineTransform& a,
				typename Coordinate<AffineTransform>::Type shx,
				typename Coordinate<AffineTransform>::Type shy,
				typename detail::EnableIfTagIs<AffineTransform, AffineTransformTag>::type* = nullptr);
			template<typename AffineTransform>
			inline AffineTransform& translate(AffineTransform& a,
				typename Coordinate<AffineTransform>::Type tx,
				typename Coordinate<AffineTransform>::Type ty,
				typename detail::EnableIfTagIs<AffineTransform, AffineTransformTag>::type* = nullptr);

			// transformations ////////////////////////////////////////////////////////////////////

			template<typename AffineTransform, typename Geometry>
			inline Geometry& transform(const AffineTransform& a, Geometry& g,
				typename detail::EnableIfTagIs<AffineTransform, AffineTransformTag>::type* = nullptr);

		}
	}
}

#endif // !ASCENSION_AFFINE_TRANSFORM_HPP
