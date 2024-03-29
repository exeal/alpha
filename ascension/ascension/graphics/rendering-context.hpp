﻿/**
 * @file rendering-context.hpp
 * @author exeal
 * @date 2011-03-06 created
 * @see rendering-context-options.hpp
 */

#ifndef ASCENSION_RENDERING_CONTEXT_HPP
#define ASCENSION_RENDERING_CONTEXT_HPP
#include <ascension/corelib/basic-types.hpp>	// std.tr1.shared_ptr
#include <ascension/corelib/native-wrappers.hpp>
#include <ascension/corelib/string-piece.hpp>
#include <ascension/graphics/color.hpp>
#include <ascension/graphics/font/font-collection.hpp>
#include <ascension/graphics/font/font-render-context.hpp>
#include <ascension/graphics/geometry/affine-transform.hpp>
#include <ascension/graphics/geometry/dimension.hpp>
#include <ascension/graphics/geometry/named-parameters.hpp>
#include <ascension/graphics/geometry/point.hpp>
#include <ascension/graphics/geometry/rectangle.hpp>
#include <ascension/graphics/rendering-context-options.hpp>
#include <boost/geometry/algorithms/make.hpp>	// boost.geometry.make_zero
#include <boost/optional.hpp>
#include <boost/parameter/preprocessor.hpp>
#include <memory>
#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(CAIRO)
#	include <cairomm/context.h>
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(CORE_GRAPHICS)
#	include <CGContext.h>
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(DIRECT2D)
#	include <d2d1.h>
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(QT)
#	include <QPainter>
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI)
#	include <ascension/win32/handle.hpp>
#	include <stack>
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDIPLUS)
#	include <GdiPlus.h>
#else
	ASCENSION_CANT_DETECT_PLATFORM();
#endif

namespace ascension {
	namespace graphics {
		class ImageData : private boost::noncopyable {
		public:
			/**
			 * Constructor.
			 * @param data The one-dimensional array containing the data in RGBA order, as integers in the range 0 to
			 *             255
			 * @param width The height of the data in device pixels
			 * @param height The height of the data in device pixels
			 */
			ImageData(std::unique_ptr<std::uint8_t[]> data, std::size_t width, std::size_t height)
				 : data_(std::move(data)), width_(width), height_(height) {
			}
			/// Returns the one-dimensional array containing the data in RGBA order, as integers
			/// in the range 0 to 255.
			boost::iterator_range<std::uint8_t*> data() BOOST_NOEXCEPT {
				return boost::make_iterator_range(data_.get(), data_.get() + width() * height());
			}
			/// Returns the one-dimensional array containing the data in RGBA order, as integers
			/// in the range 0 to 255.
			boost::iterator_range<const std::uint8_t*> data() const BOOST_NOEXCEPT {
				return boost::make_iterator_range<const std::uint8_t*>(data_.get(), data_.get() + width() * height());
			}
			/// Returns the actual height of the data in the @c ImageData object, in device pixels.
			std::size_t height() const BOOST_NOEXCEPT {return height_;}
			/// Returns the actual width of the data in the @c ImageData object, in device pixels.
			std::size_t width() const BOOST_NOEXCEPT {return width_;}
		private:
			const std::unique_ptr<std::uint8_t[]> data_;
			const std::size_t width_, height_;
		};

		class Image;
		class Paint;
		class RenderingDevice;

		namespace font {
			class Font;
			template<typename T> class FontMetrics;
		}

		/**
		 * @c CanvasRenderingContext2D interface defined in "HTML Canvas 2D Context"
		 * (http://www.w3.org/TR/2dcontext/).
		 * The documentation of this class and its members are copied (and arranged) from W3C documents.
		 * Many methods of this class may throw @c PlatformError exception.
		 * @note When drawing with @c RenderingContext2D, we specify points using logical coordinates. The default unit
		 *       is one pixel on pixel-based devices and one point on printing devices.
		 * @see RenderingDevice
		 */
		class RenderingContext2D : public UniqueWrapper<RenderingContext2D> {
		public:
			/// @name Platform-native Interfaces
			/// @{
#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(CAIRO)
			explicit RenderingContext2D(Cairo::RefPtr<Cairo::Context> nativeObject);
			Cairo::RefPtr<Cairo::Context> native();
			Cairo::RefPtr<const Cairo::Context> native() const;
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(CORE_GRAPHICS)
			explicit RenderingContext2D(CGContextRef nativeObject);
			CGContextRef native();
			const CGContextRef native() const;
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(DIRECT2D)
			explicit RenderingContext2D(win32::com::SmartPointer<ID2D1RenderTarget> nativeObject);
			win32::com::SmartPointer<ID2D1RenderTarget> native() const;
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(QT)
			explicit RenderingContext2D(QPainter& nativeObject);	// weak ref.
			explicit RenderingContext2D(std::shared_ptr<QPainter> nativeObject);
			QPainter& native();
			const QPainter& native() const;
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI)
			explicit RenderingContext2D(win32::Handle<HDC> nativeObject);
			win32::Handle<HDC> native() const BOOST_NOEXCEPT;
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDIPLUS)
			explicit RenderingContext2D(Gdiplus::Graphics& nativeObject);	// weak ref.
			explicit RenderingContext2D(std::shared_ptr<Gdiplus::Graphics> nativeObject);
			Gdiplus::Graphics& native();
			const Gdiplus::Graphics& native() const;
#endif
			/// @}
		public:
			/// Move-constructor.
			RenderingContext2D(RenderingContext2D&& other) BOOST_NOEXCEPT;
			/// Returns the available fonts in this rendering context.
			font::FontCollection availableFonts() const;
			/**
			 * Returns the font metrics for the specified font.
			 * @param font The font to get metrics. If this is @c null, this method returns the font metrics for the
			 *             current font
			 * @return The font metrics with values in the user units
			 * @see font
			 */
			std::unique_ptr<const font::FontMetrics<Scalar>> fontMetrics(
				std::shared_ptr<const font::Font> font = nullptr) const;
			font::FontRenderContext fontRenderContext() const;

			/// @name Back-Reference to the Canvas
			/// @{
//			???? canvas() const;
			const RenderingDevice& device() const;
			/// @}

			/// @name 2 The State
			/// @{
			/**
			 * Pushes a copy of the current drawing state onto the drawing state stack.
			 * @return This object
			 * @see restore
			 */
			RenderingContext2D& save();
			/**
			 * Pops the top entry in the drawing state stack, and resets the drawing state it describes.
			 * If there is no saved state, this method does nothing.
			 * @return This object
			 * @see save
			 */
			RenderingContext2D& restore();
			/// @}

			/// @name 3 Line Styles (Part of @c CanvasDrawingStyles Interface)
			/// @{
			/**
			 * Returns the current width of lines, in logical coordinate space units. Initial value is @c 1.0.
			 * @return The current line width
			 * @see setLineWidth
			 */
			Scalar lineWidth() const;
			/**
			 * Sets the width of lines.
			 * @param lineWidth The width of lines, in logical coordinate space units. Values that are not finite
			 *                  values greater than zero are ignored
			 * @return This object
			 * @see lineWidth
			 */
			RenderingContext2D& setLineWidth(Scalar lineWidth);
			/**
			 * Returns the type of endings that the renderer will place on the end of lines. Initial value is
			 * @c LineCap#BUTT.
			 * @return The current line cap style
			 * @see setLineCap
			 */
			LineCap lineCap() const;
			/**
			 * Sets the line cap style.
			 * @param lineCap The line cap style
			 * @return This object
			 * @throw UnknownValueException @a lineCap is unknown
			 * @see lineCap
			 */
			RenderingContext2D& setLineCap(LineCap lineCap);
			/**
			 * Returns the type of corners that the renderer will place where two lines meet. Initial value is
			 * @c LineJoin#MITER.
			 * @return The current line join style
			 * @see setLineJoin
			 */
			LineJoin lineJoin() const;
			/**
			 * Sets the line join style.
			 * @param lineJoin The line join style
			 * @return This object
			 * @throw UnknownValueException @a lineJoin is unknown
			 * @see lineJoin
			 */
			RenderingContext2D& setLineJoin(LineJoin lineJoin);
			/**
			 * Returns the miter limit ratio to decide how to render joins. Initial value is @c 10.0.
			 * @return The current miter limit ratio
			 * @see setMiterLimit
			 */
			double miterLimit() const;
			/**
			 * Sets the miter limit ratio.
			 * @param miterLimit The miter limit ratio. Values that are not finite values greater than zero are ignored
			 * @return This object
			 * @see miterLimit
			 */
			RenderingContext2D& setMiterLimit(double miterLimit);
			/**
			 * @return The current dash list
			 * @see setLineDash
			 */
			const std::vector<Scalar>& lineDash() const;
			/**
			 * @param lineDash
			 * @return This object
			 * @see lineDash
			 */
			RenderingContext2D& setLineDash(const std::vector<Scalar>& lineDash);
			/**
			 * @see setLineDashOffset
			 */
			Scalar lineDashOffset() const;
			/**
			 * @param lineDashOffset
			 * @return This object
			 * @see lineDashOffset
			 */
			RenderingContext2D& setLineDashOffset(Scalar lineDashOffset);
			/// @}

			/// @name 4 Text (Part of @c CanvasDrawingStyles Interface)
			/// @{
			/**
			 * Returns a @c Font object of the current font of the context. The initial value is platform-dependent.
			 * @return The current font settings
			 * @see setFont
			 */
			std::shared_ptr<const font::Font> font() const;
			/**
			 * Sets the font settings by the @c Font object.
			 * @return This object
			 * @see font
			 */
			RenderingContext2D& setFont(std::shared_ptr<const font::Font> font);
			/**
			 * Returns the current text alignment settings. Default value is @c TEXT_ANCHOR_START.
			 * @return The current text alignment settings
			 * @see setTextAlignment, textBaseline
			 */
			TextAlignment textAlignment() const;
			/**
			 * Sets the text alignment settings.
			 * @param textAlignment The new text alignment settings
			 * @return This object
			 * @throw UnknownValueException
			 * @see setTextBaseline, textAlignment
			 */
			RenderingContext2D& setTextAlignment(TextAlignment textAlignment);
			/**
			 * Returns the current baseline alignment settings. Default value is
			 * @c ALIGNMENT_BASELINE_ALPHABETIC.
			 * @return The current baseline alignment settings
			 * @see setTextBaseline, textAlignment
			 */
			font::AlignmentBaseline textBaseline() const;
			/**
			 * Sets the baseline alignment settings.
			 * @param baseline The new baseline alignment settings
			 * @return This object
			 * @throw UnknownValueException
			 * @see setTextAlignment, textBaseline
			 */
			RenderingContext2D& setTextBaseline(font::AlignmentBaseline baseline);
			/// @}

			/// @name 5 Building Paths (@c CanvasPathMethods Interface)
			/// @{
			/**
			 * Creates a new subpath with the specified point.
			 * @param to The point as first (and only) of the new subpath
			 * @return This object
			 * @see lineTo
			 */
			RenderingContext2D& moveTo(const Point& to);
			/**
			 * Marks the current subpath as closed, and starts a new subpath with a point the same as the start and end
			 * of the newly closed subpath.
			 * @return This object
			 * @see beginPath
			 */
			RenderingContext2D& closePath();
			/**
			 * Adds the specified point to the current subpath, connected to the previous one by a straight line. If
			 * the path has no subpaths, ensures there is a subpath for that point.
			 * @param to The point to add to the current subpath
			 * @return This object
			 * @see moveTo
			 */
			RenderingContext2D& lineTo(const Point& to);
			/**
			 * Adds the specified point to the current subpath, connected to the previous one by a quadratic Bézier
			 * curve with the specified control point.
			 * @param cp The control point
			 * @param to The point to add to the current subpath
			 * @return This object
			 * @see bezierCurveTo
			 */
			RenderingContext2D& quadraticCurveTo(const Point& cp, const Point& to);
			/**
			 * Adds the specified point to the current subpath, connected to the previous one by a cubic Bézier curve
			 * with the specified control point.
			 * @param cp1 The first control point
			 * @param cp2 The second control point
			 * @param to The point to add to the current subpath
			 * @return This object
			 * @see bezierCurveTo
			 */
			RenderingContext2D& bezierCurveTo(const Point& cp1, const Point& cp2, const Point& to);
			/**
			 * Adds an arc with the specified control points and radius to the current subpath, connected to the
			 * previous point by a straight line.
			 * @param p1 The start point
			 * @param p2 The destination point
			 * @param radius Radius of the circle gives the arc
			 * @return This object
			 * @throw std#invalid_argument @a radius is negative
			 * @see arc
			 */
			RenderingContext2D& arcTo(const Point& p1, const Point& p2, Scalar radius);
			/**
			 * Adds points to the subpath such that the arc described by the circumference of the circle described by
			 * the arguments, starting at the specified start angle and ending at the given end angle, going in the
			 * given direction, is added to the path, connected to the previous point by a straight line.
			 * @param p The origin of the circle gives the arc
			 * @param radius Radius of the circle gives the arc
			 * @param startAngle Defines the start point 
			 * @param endAngle Defines the end point
			 * @param counterClockwise If @c false, the arc is the path along the circumstance of the circle from the
			 *                         start point to the end point, closewise
			 * @return This object
			 * @throw std#invalid_argument @a radius is negative
			 * @see arcTo
			 */
			RenderingContext2D& arc(const Point& p, Scalar radius,
				double startAngle, double endAngle, bool counterClockwise = false);
			/**
			 * Adds a new closed subpath to the path, representing the specified rectangle.
			 * @param rect The rectangle
			 * @return This object
			 */
			RenderingContext2D& rectangle(const Rectangle& rect);
			/// @}

			/// @name 6 Transformations
			/// @{
			/**
			 * Adds the scaling transformation described by @a s to the transformation matrix.
			 * @param sx The scale factor in the horizontal direction. The factor is multiplies
			 * @param sy The scale factor in the vertical direction. The factor is multiplies
			 * @return This object
			 * @see rotate, translate, transform, setTransform
			 */
#ifndef ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
			BOOST_PARAMETER_MEMBER_FUNCTION(
				(RenderingContext2D&), scale, geometry::tag,
				(required
					(sx, (double))
					(sy, (double)))) {
				return transform(geometry::makeScalingTransform(geometry::_sx = sx, geometry::_sy = sy));
			}
#else
			RenderingContext2D& scale(double sx, double sy);
#endif // !ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
			/**
			 * Adds the rotation transformation described by @a angle to the transformation matrix.
			 * @tparam DegreeOrRadian @c boost#geometry#degree or @c boost#geometry#radian
			 * @param angle A clockwise rotation angle measured in units specified by @a DegreeOrRadian
			 * @return This object
			 * @see scale, translate, transform, setTransform
			 */
			template<typename DegreeOrRadian>
			RenderingContext2D& rotate(double angle) {
				return transform(geometry::makeRotationTransform<DegreeOrRadian>(angle));
			}
			/**
			 * Adds the translation transformation described by @a delta to the transformation matrix.
			 * @param delta The translation transformation. @c geometry#dx(delta) represents the translation distance
			 *              in the horizontal direction and @c geometry#dy(delta) represents the translation distance
			 *              in the vertical direction. These are in logical coordinate space units
			 * @return This object
			 * @see scale, rotate, transform, setTransform
			 */
			RenderingContext2D& translate(const Dimension& delta) {
				return translate(geometry::dx(delta), geometry::dy(delta));
			}
			/**
			 * Adds the translation transformation described by @a dx and @a dy to the transformation matrix.
			 * @param tx The translation distance in the horizontal direction in logical coordinate space units
			 * @param ty The translation distance in the vertical direction in logical coordinate space units
			 * @return This object
			 * @see scale, rotate, transform, setTransform
			 */
#ifndef ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
			BOOST_PARAMETER_MEMBER_FUNCTION(
				(RenderingContext2D&), translate, geometry::tag,
				(required
					(tx, (double))
					(ty, (double)))) {
				return transform(geometry::makeTranslationTransform(geometry::_tx = tx, geometry::_ty = ty));
			}
#else
			RenderingContext2D& translate(double tx, double ty);
#endif // !ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
			/**
			 * Replaces the current transformation matrix with the result of multiplying the current transformation
			 * matrix with the matrix described by @a matrix.
			 * @param matrix The matrix to multiply with
			 * @return This object
			 * @see scale, rotate, translate, setTransform
			 */
			RenderingContext2D& transform(const AffineTransform& matrix);
			/**
			 * Resets the current transformation to the identity matrix and calls @c #transform(const AffineTransform&amp;).
			 * @param matrix The new transformation matrix
			 * @return This object
			 * @see scale, rotate, translate, transform
			 */
			RenderingContext2D& setTransform(const AffineTransform& matrix);
			/// @}

			/// @name 7 Fill and Stroke Styles
			/// @{
			/**
			 * Returns the color or style to use inside shapes. Initial value is opaque black.
			 * @return The current fill style
			 * @see setFillStyle, strokeStyle
			 */
			std::shared_ptr<const Paint> fillStyle() const;
			/**
			 * Sets the style used for filling shapes.
			 * @param fillStyle The new fill style to set
			 * @return This object
			 * @see fillStyle, setStrokeStyle
			 */
			RenderingContext2D& setFillStyle(std::shared_ptr<const Paint> fillStyle);
			/**
			 * Returns the color or style to use for the lines around the shapes. Initial value is opaque black.
			 * @return The current stokre style
			 * @see setStrokeStyle, fillStyle
			 */
			std::shared_ptr<const Paint> strokeStyle() const;
			/**
			 * Sets the style used for stroking shapes.
			 * @param strokeStyle The new fill style to set
			 * @return This object
			 * @see strokeStyle, setFillStyle
			 */
			RenderingContext2D& setStrokeStyle(std::shared_ptr<const Paint> strokeStyle);
//			std::unique_ptr<Gradient> createLinearGradient();
//			std::unique_ptr<Gradient> createRadialGradient();
//			std::unique_ptr<Pattern> createPattern();
			/// @}

			/// @name 8 Drawing Rectangles to the Canvas
			/// @{
			/**
			 * Clears all pixels on the canvas in the specified rectangle to transparent black.
			 * @param rectangle The rectangle
			 * @return This object
			 * @see fillRectangle, strokeRectangle
			 */
			RenderingContext2D& clearRectangle(const Rectangle& rectangle);
			/**
			 * Paints the specified rectangle onto the canvas, using the current fill style.
			 * @param rectangle The rectangle
			 * @return This object
			 * @see clarRectangle, strokeRectangle
			 */
			RenderingContext2D& fillRectangle(const Rectangle& rectangle);
			/**
			 * Paints the box that outlines the specified rectangle onto the canvas, using the current stroke style.
			 * @param rectangle The rectangle
			 * @return This object
			 * @see clearRectangle, fillRectangle
			 */
			RenderingContext2D& strokeRectangle(const Rectangle& rectangle);
			/// @}

			/// @name 9 Drawing Text to the Canvas
			/// @{
			/**
			 * Fills the given text at the given position. If a maximum measure is provided, the text will be scaled to
			 * fit that measure if necessary.
			 * @param text The text string
			 * @param origin The origin of the text. The alignment of the drawing is calculated by the current
			 *               @c #textAlignment and @c #textBaseline values
			 * @param maximumMeasure If present, this value specifies the maximum measure (width in horizontal writing
			 *                       mode)
			 * @return This object
			 * @throw std#invalid_argument @a maximumMeasure is present but less than or equal to zero
			 * @see strokeText, measureText
			 */
			RenderingContext2D& fillText(const StringPiece& text,
				const Point& origin, boost::optional<Scalar> maximumMeasure = boost::none);
			/**
			 * Strokes the given text at the given position. If a maximum measure is provided, the text will be scaled
			 * to fit that measure if necessary.
			 * @param text The text string
			 * @param origin The origin of the text. The alignment of the drawing is calculated by the current
			 *               @c #textAlignment and @c #textBaseline values
			 * @param maximumMeasure If present, this value specifies the maximum measure (width in horizontal writing
			 *                       mode)
			 * @return This object
			 * @throw std#invalid_argument @a maximumMeasure is present but less than or equal to zero
			 * @see fillText, measureText
			 */
			RenderingContext2D& strokeText(const StringPiece& text,
				const Point& origin, boost::optional<Scalar> maximumMeasure = boost::none);
			/**
			 * Returns a size (measure and extent) of the specified text in the current font.
			 * @param text The text string
			 * @see strokeText, fillText
			 */
			Dimension measureText(const StringPiece& text) const;
			/// @}

			/// @name 10 Drawing Paths to the Canvas
			/// @{
			/**
			 * Resets the current default path.
			 * @return This object
			 * @see closePath
			 */
			RenderingContext2D& beginPath();
			/**
			 * Fills the subpaths of the current default path with the current fill style.
			 * @return This object
			 * @see fillRectangle, fillText, stroke
			 */
			RenderingContext2D& fill();
			/**
			 * Strokes the subpaths of the current default path with the current stroke style.
			 * @return This object
			 * @see fill, strokeRectangle, strokeText
			 */
			RenderingContext2D& stroke();
#if 1
			/**
			 */
			RenderingContext2D& drawFocusIfNeeded(const void* element);
#else
			/**
			 * Draws a focus ring around the current default path, following the platform conventions for focus rings.
			 * @see drawCustomFocusRing
			 */
			void drawSystemFocusRing(/*const NativeRectangle& bounds*/);
			/**
			 * The end user has configured whose system to draw focus rings in a particular manner (for example, high
			 * contrast focus rings), draws a focus ring around the current default path and returns @c false.
			 * Otherwise returns @c true.
			 * @retval true This method did not draw a focus ring
			 * @retval false This method drew a focus ring
			 */
			bool drawCustomFocusRing(/*const NativeRectangle& bounds*/);
			/**
			 * Scrolls the current default path into view. This is especially useful on devices with small screens,
			 * where the whole canvas might not be visible at once.
			 * @return This object
			 */
			RenderingContext2D& scrollPathIntoView();
#endif
			/**
			 * Further constrains the clipping region to the current default path.
			 * @return This object
			 */
			RenderingContext2D& clip();
			/**
			 * Returns @c true if the specified point is in the current default path.
			 * @param point The point to test
			 * @return @a point is in the current default path
			 */
			bool isPointInPath(const Point& point) const;
			/// @}

			/// @name 11 Drawing Images to the Canvas
			/// @{
			/**
			 * Draws the specified image onto the canvas.
			 * @param image The image to draw
			 * @param position The destination position
			 * @return This object
			 */
			RenderingContext2D& drawImage(const Image& image, const Point& position);
			/**
			 * Draws the specified image onto the canvas.
			 * @param image The image to draw
			 * @param destinationBounds The destination bounds
			 * @return This object
			 */
			RenderingContext2D& drawImage(const Image& image, const Rectangle& destinationBounds);
			/**
			 * Draws the specified image onto the canvas.
			 * @param image The image to draw
			 * @param sourceBounds The bounds in @a image data to draw
			 * @param destinationBounds The destination bounds
			 * @return This object
			 */
			RenderingContext2D& drawImage(const Image& image, const Rectangle& sourceBounds, const Rectangle& destinationBounds);
			/// @}
#if 0
			/// @name 12 Hit Regions
			/// @{
			struct HitRegionOptions {
				String id;
				const void* control;
			};
			void addHitRegion(const HitRegionOptions& options);
			void removeHitRegion(/*id*/);
			void clearHitRegions();
			/// @}
#endif
			/// @name 13 Pixel Manipulation
			/// @{
			/**
			 * Returns an @c ImageData object with the specified dimensions. All the pixels in the returned object are
			 * transparent black.
			 * @param dimensions The dimensions in pixels
			 * @return An image data
			 */
			std::unique_ptr<ImageData> createImageData(const Dimension& dimensions) const;
			/**
			 * Returns an @c ImageData object with the same dimensions as the argument. All pixels in the returned
			 * object are transparent black.
			 * @param image The image data gives the dimensions
			 * @return An image data
			 */
			std::unique_ptr<ImageData> createImageData(const ImageData& image) const {
		     	return createImageData(Dimension(
					geometry::_dx = static_cast<Scalar>(image.width()),
					geometry::_dy = static_cast<Scalar>(image.height())));
			}
			/**
			 * Returns an @c ImageData object containing the image data for the specified rectangle of the canvas.
			 * @param rectangle The bounds of the image in canvas coordinate space units
			 * @return An image data. Pixels outside the canvas are returned as transparent black
			 * @see putImageData
			 */
			std::unique_ptr<ImageData> getImageData(const Rectangle& rectangle) const;
			/**
			 * Paints the data from the specified @c ImageData object onto the canvas. The @c #globalAlpha and
			 * @c #globalCompositeOperation attributes, as well as the shadow attributes, are ignored for the purposes
			 * of this method call; pixels in the canvas are replaced wholesale, with no composition, alpha blending,
			 * no shadows, etc.
			 * @param image The image data
			 * @param destination The destination position onto where the image is painted in logical coordinate space
			 *                    units
			 * @see getImageData
			 */
			RenderingContext2D& putImageData(const ImageData& image, const Point& destination);
			/**
			 * Paints the data from the specified @c ImageData object onto the canvas. Only the pixels from
			 * @a dirtyRectangle are painted. The @c #globalAlpha and @c #globalCompositeOperation attributes, as well
			 * as the shadow attributes, are ignored for the purposes of this method call; pixels in the canvas are
			 * replaced wholesale, with no composition, alpha blending, no shadows, etc.
			 * @param image The image data
			 * @param destination The destination position onto where the image is painted in the logical coordinate
			 *                    space units
			 * @param dirtyRectangle The bounds to paint in image in device pixels
			 * @see getImageData
			 */
			RenderingContext2D& putImageData(const ImageData& image,
				const Point& destination, const Rectangle& dirtyRectangle);
			/// @}

			/// @name 14 Compositing
			/// @{
			/**
			 * Returns the current alpha value applied to rendering operations. Initial value is @c 1.0.
			 * @return The current alpha value in the range from 0.0 (fully transparent) to 1.0 (no additional
			 *         transparency)
			 * @see globalCompositeOperation, setGlobalAlpha
			 */
			double globalAlpha() const;
			/**
			 * Sets the current alpha value applied to rendering operations.
			 * @param globalAlpha The alpha value in the range from 0.0 (fully transparent) to 1.0
			 *                   (no additional transparency)
			 * @throw std#invalid_argument @a globalAlpha is out of range from 0.0 to 1.0
			 * @see globalAlpha, setGlobalCompositeOperation
			 */
			RenderingContext2D& setGlobalAlpha(double globalAlpha);
			/**
			 * Returns the current composition operation. Initial value is @c SOURCE_OVER.
			 * @return The current composition operation
			 * @see globalAlpha, setGlobalCompositeOperation
			 */
			CompositeOperation globalCompositeOperation() const;
			/**
			 * Sets the current composition operation.
			 * @param compositeOperation The new composition operation
			 * @throw UnknownValueException @a compositeOperation is unknown
			 * @see globalCompositeOperation, setGlobalAlpha
			 */
			RenderingContext2D& setGlobalCompositeOperation(CompositeOperation compositeOperation);
			/// @}

			/// @name 15 Shadows
			/// @{
			/**
			 * Returns the current shadow color. Initial value is fully-transparent black.
			 * @return The current shadow color
			 * @see setShadowColor
			 */
			Color shadowColor() const;
			/**
			 * Sets the shadow color.
			 * @param shadowColor The new shadow color to set
			 * @return This object
			 * @see shadowColor
			 */
			RenderingContext2D& setShadowColor(const Color& shadowColor);
			/**
			 * Returns the current shadow offset. Initial value is <code>(0, 0)</code>.
			 * @return The current shadow offset
			 * @see setShadowOffset
			 */
			Dimension shadowOffset() const;
			/**
			 * Sets the shadow offset.
			 * @return The new shadow offset to set
			 * @see shadowOffset
			 */
			RenderingContext2D& setShadowOffset(const Dimension& shadowOffset);
			/**
			 * Returns the current level of blur applied to shadows. Initial value is @c 0.
			 * @return The current level of blur applied to shadows
			 * @see setShadowBlur
			 */
			Scalar shadowBlur() const;
			/**
			 * Sets the shadow color.
			 * @param shadowColor The new shadow color to set
			 * @return This object
			 * @see shadowBlur
			 */
			RenderingContext2D& setShadowBlur(Scalar shadowColor);
			/// @}

		private:
#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(CAIRO)
			Cairo::RefPtr<Cairo::Context> nativeObject_;
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(CORE_GRAPHICS)
			CGContextRef nativeObject_;
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(DIRECT2D)
			win32::com::SmartPointer<ID2D1RenderTarget> nativeObject_;
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(QT)
			std::shared_ptr<QPainter> nativeObject_;
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI)
			win32::Handle<HDC> nativeObject_;
			struct State {
				std::pair<std::shared_ptr<const Paint>, std::size_t> fillStyle, strokeStyle;
				win32::Handle<HPEN> pen, previousPen;
				win32::Handle<HBRUSH> brush, previousBrush;
				std::shared_ptr<const font::Font> font;
			} currentState_;
			struct SavedState {
				SavedState(const State& state, int cookie) BOOST_NOEXCEPT : state(state), cookie(cookie) {}
				const State state;
				const int cookie;
			};
			std::stack<SavedState> savedStates_;
			bool hasCurrentSubpath_;
			RenderingContext2D& changePen(win32::Handle<HPEN> newPen);
			win32::Handle<HPEN> createModifiedPen(
				const LOGBRUSH* patternBrush, boost::optional<Scalar> lineWidth,
				boost::optional<LineCap> lineCap, boost::optional<LineJoin> lineJoin) const;
			bool endPath();
			bool ensureThereIsASubpathFor(const Point& p);
			void updatePenAndBrush();
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDIPLUS)
			std::shared_ptr<Gdiplus::Graphics> nativeObject_;
#endif
		};
	}
}

#endif // !ASCENSION_RENDERING_CONTEXT_HPP
