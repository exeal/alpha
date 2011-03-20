/**
 * @file geometry.hpp
 * Defines basic data types for geometry.
 * @author exeal
 * @date 2010-11-06 created
 */

#ifndef ASCENSION_GEOMETRY_HPP
#define ASCENSION_GEOMETRY_HPP

#include <ascension/corelib/memory.hpp>	// FastArenaObject
#include <ascension/platforms.hpp>
#ifdef ASCENSION_WINDOWS
#	include <ascension/win32/windows.hpp>
#endif // ASCENSION_WINDOWS

namespace ascension {
	namespace graphics {

#ifdef ASCENSION_WINDOWS
		typedef int Scalar;
		typedef win32::Handle<HRGN> NativePolygon;
#else
#endif

		/**
		 * A @c Dimension represents the size of a two-dimensional primitive.
		 * @note This class does not use words "width" and "height".
		 * @tparam Coordinate The coordinate
		 * @see Scalar, Point, Rect
		 */
		template<typename Coordinate = Scalar>
		struct Dimension {
			Coordinate cx;	///< The size of the primitive in x-coordinate. Can be negative.
			Coordinate cy;	///< The size of the primitive in y-coordinate. Can be negative.
			/// Default constructor does *not* initialize the coordinates.
			Dimension();
			/**
			 * Constructor.
			 * @param cx The size in x-coordinate
			 * @param cy The size in y-coordinate
			 */
			Dimension(Coordinate cx, Coordinate cy) : cx(cx), cy(cy) {}
			Dimension<Coordinate>& operator+=(const Dimension<Coordinate>& other);
			Dimension<Coordinate>& operator-=(const Dimension<Coordinate>& other);
			template<typename Factor> Dimension<Coordinate>& operator*=(Factor other);
			template<typename Divisor> Dimension<Coordinate>& operator/=(Divisor divisor);
			Dimension<Coordinate>& boundTo(const Dimension<Coordinate>& other);
			Dimension<Coordinate>& expandTo(const Dimension<Coordinate>& other);
			bool isNormalized() const;
			/**
			 * Scales this object to the given rectangle, preserving aspect ratio.
			 * @code
			 * Dimension<int> d1(20, 30);
			 * d2.scale(Dimension<int>(60, 60), false); // -> 40x60
			 * Dimension<int> d2(20, 30);
			 * d1.scale(Dimension<int>(60, 60), true);  // -> 60x90
			 * @endcode
			 * @param size
			 * @param keepAspectRatioByExpanding If @c true, this dimension is scaled to a
			 *                                   rectangle as small as possible outside @a size.
			 *                                   If @c false, this dimension is scaled to a
			 *                                   rectangle as large as possible inside @a size
			 * @return This object
			 */
			Dimension<Coordinate>& scale(const Dimension<Coordinate>& size, bool keepAspectRatioByExpanding);
			/**
			 * Swaps the cx and cy values.
			 * @return This object
			 */
			Dimension<Coordinate>& transpose();
		};

		template<typename Coordinate>
		inline bool operator==(const Dimension<Coordinate>& lhs, const Dimension<Coordinate>& rhs);
		template<typename Coordinate>
		bool operator!=(const Dimension<Coordinate>& lhs, const Dimension<Coordinate>& rhs);
		template<typename Coordinate>
		const Dimension<Coordinate> operator+(const Dimension<Coordinate>& lhs, const Dimension<Coordinate>& rhs) {
			Dimension<Coordinate> temp(lhs);
			return temp += rhs;
		}
		template<typename Coordinate>
		inline const Dimension<Coordinate> operator-(const Dimension<Coordinate>& self);
		template<typename Coordinate>
		const Dimension<Coordinate> operator-(const Dimension<Coordinate>& lhs, const Dimension<Coordinate>& rhs) {
			Dimension<Coordinate> temp(lhs);
			return temp += rhs;
		}
		template<typename Coordinate, typename Factor>
		const Dimension<Coordinate> operator*(const Dimension<Coordinate>& point, Factor factor) {
			Dimension<Coordinate> temp(point);
			return temp *= factor;
		}
		template<typename Coordinate, typename Factor>
		const Dimension<Coordinate> operator*(Factor factor, const Dimension<Coordinate>& point) {
			return point * factor;
		}
		template<typename Coordinate, typename Divisor>
		const Dimension<Coordinate> operator/(const Dimension<Coordinate>& point, Divisor divisor) {
			Dimension<Coordinate> temp(point);
			return temp /= divisor;
		}
		template<typename CharType, typename CharTraits, typename Coordinate>
		inline std::basic_ostream<CharType, CharTraits>&
				operator<<(std::basic_ostream<CharType, CharTraits>& out, const Dimension<Coordinate>& v) {
			const std::ctype<CharType>& ct = std::use_facet<std::ctype<CharType> >(out.getloc());
			return out << v.cx << ct.widen('x') << v.cy;
		}

		/**
		 * A @c Point represents a point in the (x, y) coordinate plane.
		 * @param Coordinate The coordinate
		 * @see Scalar, Dimension, Rect
		 */
		template<typename Coordinate = Scalar>
		struct Point {
			Coordinate x;	///< The x coordinate.
			Coordinate y;	///< The y coordinate.
			/// Default constructor does *not* initialize the coordinates.
			Point() {}
			/**
			 * Constructor.
			 * @param x The x coordinate
			 * @param y The y coordinate
			 */
			Point(Coordinate x, Coordinate y) : x(x), y(y) {}
			/**
			 * 
			 */
			Point<Coordinate>& operator+=(const Dimension<Coordinate>& other) {
				return translate(other);
			}
			/**
			 * 
			 */
			Point<Coordinate>& operator+=(const Point<Coordinate>& other) {
				return translate(Dimension<Coordinate>(other.x, other.y));
			}
			/**
			 * 
			 */
			Point<Coordinate>& operator-=(const Dimension<Coordinate>& other) {
				return *this += -other;
			}
			/**
			 * 
			 */
			Point<Coordinate>& operator-=(const Point<Coordinate>& other) {
				return *this += -other;
			}
			/**
			 * Multiplies both x and y by the given @a factor.
			 * @tparam Factor A type of @a factor
			 * @param factor The factor
			 * @return This object
			 */
			template<typename Factor> Point<Coordinate>& operator*=(Factor factor) {
				x *= factor;
				y *= factor;
				return *this;
			}
			/**
			 * Divides both x and y by the given @a divisor.
			 * @tparam Divisor A type of @a factor
			 * @param divisor The divisor
			 * @return This object
			 */
			template<typename Divisor> Point<Coordinate>& operator/=(Divisor divisor) {
				x /= divisor;
				y /= divisor;
				return *this;
			}
			/**
			 * Translates the point by @a delta along the x/y axis.
			 * @param delta The distance to move
			 * @return This point
			 */
			template<typename Coordinate2>
			Point<Coordinate>& translate(const Dimension<Coordinate2>& delta) {
				x += delta.cx;
				y += delta.cy;
				return *this;
			}
			/**
			 * Translates the point by @a delta along the x/y axis.
			 * @param delta The distance to move
			 * @return This point
			 */
			template<typename Coordinate2>
			Point<Coordinate>& translate(const Point<Coordinate2>& delta) {
				x += delta.x;
				y += delta.y;
				return *this;
			}
		};

		template<typename Coordinate>
		bool operator==(const Point<Coordinate>& lhs, const Point<Coordinate>& rhs);
		template<typename Coordinate>
		bool operator!=(const Point<Coordinate>& lhs, const Point<Coordinate>& rhs);
		template<typename Coordinate>
		const Point<Coordinate> operator+(const Point<Coordinate>& lhs, const Point<Coordinate>& rhs);
		template<typename Coordinate>
		const Point<Coordinate> operator-(const Point<Coordinate>& point);
		template<typename Coordinate>
		const Point<Coordinate> operator-(const Point<Coordinate>& lhs, const Point<Coordinate>& rhs);
		template<typename Coordinate, typename Factor>
		const Point<Coordinate> operator*(const Point<Coordinate>& point, Factor factor);
		template<typename Coordinate, typename Factor>
		const Point<Coordinate> operator*(Factor factor, const Point<Coordinate>& point);
		template<typename Coordinate, typename Factor>
		const Point<Coordinate> operator/(const Point<Coordinate>& point, Factor factor);
		template<typename CharType, typename CharTraits, typename Coordinate>
		inline std::basic_ostream<CharType, CharTraits>&
				operator<<(std::basic_ostream<CharType, CharTraits>& out, const Point<Coordinate>& v) {
			const std::ctype<CharType>& ct = std::use_facet<std::ctype<CharType> >(out.getloc());
			return out << v.x << ct.widen(',') << v.y;
		}

		template<typename Coordinate> class Rect;

		template<typename Coordinate>
		class RectPartProxy {
		public:
			void operator=(Coordinate other) {
				switch(part_) {
					case 0:	// left
						if(rect_.size().cx >= 0) {
							rect_.size_.cx += rect_.origin().x - other;	// $friendly-access$
							rect_.origin_.x = other;	// $friendly-access$
						} else
							rect_.size_.cx = other - rect_.origin().x;	// $friendly-access$
						break;
					case 1:	// top
						if(rect_.size().cy >= 0) {
							rect_.size_.cy += rect_.origin().y - other;	// $friendly-access$
							rect_.origin_.y = other;	// $friendly-access$
						} else
							rect_.size_.cy = other - rect_.origin().y;	// $friendly-access$
						break;
					case 2:	// right
						if(rect_.size().cx >= 0)
							rect_.size_.cx = other - rect_.origin().x;	// $friendly-access$
						else {
							rect_.size_.cx += rect_.origin().x - other;	// $friendly-access$
							rect_.origin_.x = other - rect_.origin().x;	// $friendly-access$
						}
						break;
					case 3:	// bottom
						if(rect_.size().cy >= 0)
							rect_.size_.cy = other - rect_.origin().y;	// $friendly-access$
						else {
							rect_.size_.cy += rect_.origin().y - other;	// $friendly-access$
							rect_.origin_.y = other - rect_.origin().y;	// $friendly-access$
						}
						break;
				}
			}
			void operator=(const RectPartProxy<Coordinate>& other) {
				return *this = static_cast<Coordinate>(other);
			}
			operator Coordinate() const {
				switch(part_) {
					case 0:	// left
						return std::min(rect_.origin().x, rect_.origin().x + rect_.size().cx);
					case 1:	// top
						return std::min(rect_.origin().y, rect_.origin().y + rect_.size().cy);
					case 2:	// right
						return std::max(rect_.origin().x, rect_.origin().x + rect_.size().cx);
					case 3:	// bottom
						return std::max(rect_.origin().y, rect_.origin().y + rect_.size().cy);
					default:
						assert(false);
				}
			}
			Coordinate operator+() const {return +(*this);}
			Coordinate operator-() const {return -(*this);}
			void operator+=(Coordinate other) {*this = *this + other;}
			void operator-=(Coordinate other) {*this = *this - other;}
			void operator*=(Coordinate other) {*this = *this * other;}
			void operator/=(Coordinate other) {*this = *this / other;}
		private:
			RectPartProxy(const Rect<Coordinate>& rect, int part) : rect_(const_cast<Rect<Coordinate>&>(rect)), part_(part) {}
			RectPartProxy(const RectPartProxy<Coordinate>&);	// noncopyable
			Rect<Coordinate>& rect_;
			const int part_;
			friend class Rect<Coordinate>;
		};

		template<typename Coordinate = Scalar>
		class Rect {
		public:
			Rect();
			explicit Rect(const Point<Coordinate>& origin);
			explicit Rect(const Dimension<Coordinate>& size);
			Rect(const Point<Coordinate>& origin, const Dimension<Coordinate>& size);
			Rect(const Point<Coordinate>& first, const Point<Coordinate>& second);
		public:
			RectPartProxy<Coordinate> bottom() {return RectPartProxy<Coordinate>(*this, 3);}
			const RectPartProxy<Coordinate> bottom() const {return RectPartProxy<Coordinate>(*this, 3);}
			Coordinate height() const {return std::abs(size().cy);}
			RectPartProxy<Coordinate> left() {return RectPartProxy<Coordinate>(*this, 0);}
			const RectPartProxy<Coordinate> left() const {return RectPartProxy<Coordinate>(*this, 0);}
			Point<Coordinate> origin() const;
			RectPartProxy<Coordinate> right() {return RectPartProxy<Coordinate>(*this, 2);}
			const RectPartProxy<Coordinate> right() const {return RectPartProxy<Coordinate>(*this, 2);}
			Dimension<Coordinate> size() const;
			RectPartProxy<Coordinate> top() {return RectPartProxy<Coordinate>(*this, 1);}
			const RectPartProxy<Coordinate> top() const {return RectPartProxy<Coordinate>(*this, 1);}
			Coordinate width() const {return std::abs(size().cx);}
			Range<Coordinate> x() const {return makeRange(origin().x, origin().x + size().cx);}
			Range<Coordinate> y() const {return makeRange(origin().y, origin().y + size().cy);}
		public:
			template<typename Coordinate2>
			bool includes(const Point<Coordinate2>& other) const;
			template<typename Coordinate2>
			bool includes(const Rect<Coordinate2>& other) const;
			template<typename Coordinate2>
			Rect<Coordinate> intersected(const Rect<Coordinate2>& other) const;
			template<typename Coordinate2>
			bool intersects(const Rect<Coordinate2>& other) const;
			template<typename Coordinate2>
			Rect<Coordinate> united(const Rect<Coordinate2>& other) const;
		public:
			Rect<Coordinate>& normalize();
			/**
			 * Sets the size of the rectangle.
			 * @param newSize The size to set
			 * @return This object.
			 */
			Rect<Coordinate>& resize(const Dimension<Coordinate>& newSize) {
				size_ = newSize;
				return *this;
			}
			Rect<Coordinate>& setX(const Range<Coordinate>& newX);
			Rect<Coordinate>& setY(const Range<Coordinate>& newY);
			/**
			 * Moves the rectangle @a offset.x along x-axis and @a offset.y along y-axis,
			 * relative to the current origin.
			 * @tparam Primitive Gives the offset. @c Point or @c Dimension
			 * @param offset The offset to move
			 * @return This object
			 */
			template<typename Primitive>
			Rect<Coordinate>& translate(const Primitive& offset) {
				origin_.translate(offset);
				return *this;
			}
		private:
			friend class RectPartProxy<Coordinate>;
			Point<Coordinate> origin_;
			Dimension<Coordinate> size_;
		};

		template<typename Coordinate>
		inline Rect<Coordinate> operator|(const Rect<Coordinate>& lhs, const Rect<Coordinate>& rhs);
		template<typename Coordinate>
		inline Rect<Coordinate> operator&(const Rect<Coordinate>& lhs, const Rect<Coordinate>& rhs);
		template<typename CharType, typename CharTraits, typename Coordinate>
		inline std::basic_ostream<CharType, CharTraits>&
				operator<<(std::basic_ostream<CharType, CharTraits>& out, const Rect<Coordinate>& v) {
			const std::ctype<CharType>& ct = std::use_facet<std::ctype<CharType> >(out.getloc());
			return out << v.origin() << ct.widen(' ') << v.size();
		}

		// conversions from/to platform-native primitives
#if defined(ASCENSION_WINDOWS)
		inline Point<> fromNative(const POINT& source) /*throw()*/ {
			return Point<>(source.x, source.y);
		}
		inline Dimension<> fromNative(const SIZE& source) /*throw()*/ {
			return Dimension<>(source.cx, source.cy);
		}
		inline Rect<> fromNative(const RECT& source) /*throw()*/ {
			return Rect<>(
				Point<>(std::min(source.left, source.right), std::min(source.top, source.bottom)),
				Point<>(std::max(source.left, source.right), std::max(source.top, source.bottom)));
		}
		inline POINT toNative(const Point<>& source) /*throw()*/ {
			const POINT temp = {source.x, source.y};
			return temp;
		}
		inline SIZE toNative(const Dimension<>& source) /*throw()*/ {
			const SIZE temp = {source.cx, source.cy};
			return temp;
		}
		inline RECT toNative(const Rect<>& source) /*throw()*/ {
			RECT temp;
			::SetRect(&temp, source.left(), source.top(), source.right(), source.bottom());
			return temp;
		}
#elif defined(ASCENSION_CORE_GRAPHICS)
		inline Point<> fromNative(const CGPoint& source) /*throw()*/ {
			return Point<>(source.x, source.y);
		}
		inline Dimension<> fromNative(const CGSize& source) /*throw()*/ {
			return Dimension<>(source.width, source.height);
		}
		inline Rect<> fromNative(const CGRect& source) /*throw()*/ {
			return Rect<>(fromNative(source.origin), fromNative(source.size));
		}
		inline CGPoint toNative(const Point<>& source) /*throw()*/ {
			return CGPointMake(source.x, source.y);
		}
		inline CGSize toNative(const Dimension<>& source) /*throw()*/ {
			return CGSizeMake(source.cx, source.cy);
		}
		inline CGRect toNative(const Rect<>& source) /*throw()*/ {
			return CGRectMake(source.origin().x, source.origin().y, source.size().cx, source.size().cy);
		}
#elif defined(ASCENSION_X11)
		inline Rect<> fromNative(const GdkRectangle& source) /*throw()*/ {
			return Rect<>(Point<>(source.x, source.y), Dimension<>(source.width, source.height));
		}
		inline GdkRectangle toNative(const Rect<>& source) /*throw()*/ {
			const GdkRectangle temp = {source.origin().x, source.origin().y, source.size().cx, source.size().cy};
			return temp;
		}
#endif

	}
}

#endif // !ASCENSION_GEOMETRY_HPP
