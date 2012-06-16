/**
 * @file rendering-context.hpp
 * @author exeal
 * @date 2011-03-06 created
 */

#ifndef ASCENSION_RENDERING_CONTEXT_HPP
#define ASCENSION_RENDERING_CONTEXT_HPP

#include <ascension/corelib/basic-types.hpp>	// std.tr1.shared_ptr
#include <ascension/corelib/string-piece.hpp>
#include <ascension/graphics/affine-transform.hpp>
#include <ascension/graphics/color.hpp>
#include <ascension/graphics/font.hpp>
#include <ascension/graphics/geometry.hpp>
#include <ascension/presentation/text-line-style.hpp>	// presentation.AlignmentBaseline, presentation.TextAnchor
#include <memory>
#include <boost/optional.hpp>
#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
#	include <cairomm/context.h>
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
#	include <CGContext.h>
#elif defined(ASCENSION_GRAPHICS_SYSTEM_DIRECT2D)
#	include <d2d1.h>
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
#	include <QPainter>
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
#	include <ascension/win32/handle.hpp>
#	include <stack>
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDIPLUS)
#	include <GdiPlus.h>
#endif

namespace ascension {
	namespace graphics {
#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
		typedef Cairo::RefPtr<Cairo::Context> NativeRenderingContext2D;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
		typedef CGContextRef NativeRenderingContext2D;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_DIRECT2D)
		typedef win32::com::SmartPointer<ID2D1RenderTarget> NativeRenderingContext2D;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
		typedef QPainter& NativeRenderingContext2D;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
		typedef win32::Handle<HDC> NativeRenderingContext2D;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDIPLUS)
		typedef Gdiplus::Graphics& NativeRenderingContext2D;
#endif

		/**
		 * Specifies how shapes and images are drawn onto the existing bitmap.
		 * @see RenderingContext2D#globalCompositeOperation,
		 *      RenderingContext2D#setGlobalCompositeOperation
		 */
		enum CompositeOperation {
			/// Display the source image wherever both images are opaque. Display the destination
			/// image wherever the destination image is opaque but the source image is transparent.
			/// Display transparency elsewhere.
			SOURCE_ATOP,
			/// Display the source image wherever both the source image and destination image are
			/// opaque. Display transparency elsewhere.
			SOURCE_IN,
			/// Display the source image wherever the source image is opaque and the destination
			/// image is transparent. Display transparency elsewhere.
			SOURCE_OUT,
			/// Display the source image wherever the source image is opaque. Display the
			/// destination image elsewhere.
			SOURCE_OVER,
			/// Same as @c SOURCE_ATOP but using the destination image instead of the source image
			/// and vice versa.
			DESTINATION_ATOP,
			/// Same as @c SOURCE_IN but using the destination image instead of the source image
			/// and vice versa.
			DESTINATION_IN,
			/// Same as @c SOURCE_OUT but using the destination image instead of the source image
			/// and vice versa.
			DESTINATION_OUT,
			/// Same as @c SOURCE_OVER but using the destination image instead of the source image
			/// and vice versa.
			DESTINATION_OVER,
			/// Display the sum of the source image and destination image, with color values
			/// approaching 255 (100%) as a limit.
			LIGHTER,
			/// Display the source image instead of the destination image.
			COPY,
			/// Exclusive OR of the source image and destination image.
			XOR
		};

		enum FillRule {NONZERO, EVENODD};

		ASCENSION_BEGIN_SCOPED_ENUM(LineCap)
			BUTT,
			ROUND,
			SQUARE
		ASCENSION_END_SCOPED_ENUM

		ASCENSION_BEGIN_SCOPED_ENUM(LineJoin)
			BEVEL,
			ROUND,
			MITER
		ASCENSION_END_SCOPED_ENUM

		ASCENSION_BEGIN_SCOPED_ENUM(TextAlignment)
			START = presentation::TEXT_ANCHOR_START,
			END = presentation::TEXT_ANCHOR_END,
			LEFT = detail::LEFT,
			RIGHT = detail::RIGHT,
			CENTER = presentation::TEXT_ANCHOR_MIDDLE
		ASCENSION_END_SCOPED_ENUM

		class ImageData {
			ASCENSION_NONCOPYABLE_TAG(ImageData);
		public:
			/**
			 * Constructor.
			 * @param data The one-dimensional array containing the data in RGBA order, as integers
			 *             in the range 0 to 255
			 * @param width The height of the data in device pixels
			 * @param height The height of the data in device pixels
			 */
			ImageData(std::unique_ptr<uint8_t[]>&& data, std::size_t width, std::size_t height)
				 : data_(std::move(data)), width_(width), height_(height) {
			}
			/// Returns the one-dimensional array containing the data in RGBA order, as integers
			/// in the range 0 to 255.
			Range<uint8_t*> data() /*noexcept*/ {
				return makeRange(data_.get(), data_.get() + width() * height());
			}
			/// Returns the one-dimensional array containing the data in RGBA order, as integers
			/// in the range 0 to 255.
			Range<const uint8_t*> data() const /*noexcept*/ {
				return makeRange<const uint8_t*>(data_.get(), data_.get() + width() * height());
			}
			/// Returns the actual height of the data in the @c ImageData object, in device pixels.
			std::size_t height() const /*noexcept*/ {return height_;}
			/// Returns the actual width of the data in the @c ImageData object, in device pixels.
			std::size_t width() const /*noexcept*/ {return width_;}
		private:
			const std::unique_ptr<uint8_t[]> data_;
			const std::size_t width_, height_;
		};

		class Image;
		class Paint;
		class RenderingDevice;

		/**
		 * @c CanvasRenderingContext2D interface defined in "HTML Canvas 2D Context"
		 * (http://dev.w3.org/html5/2dcontext/).
		 * The documentation of this class and its members are copied (and arranged) from W3C
		 * documents.
		 * Many methods of this class may throw @c PlatformError exception.
		 */
		class RenderingContext2D {
			ASCENSION_NONCOPYABLE_TAG(RenderingContext2D);
		public:
			// platform-native interfaces
#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
			explicit RenderingContext2D(Cairo::RefPtr<Cairo::Context> nativeObject);
			Cairo::RefPtr<Cairo::Context> asNativeObject();
			Cairo::RefPtr<const Cairo::Context> asNativeObject() const;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
			CGContextRef asNativeObject();
			const CGContextRef asNativeObject() const;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_DIRECT2D)
			explicit RenderingContext2D(win32::com::SmartPointer<ID2D1RenderTarget> nativeObject);
			win32::com::SmartPointer<ID2D1RenderTarget> asNativeObject() const;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
			explicit RenderingContext2D(QPainter& nativeObject);	// weak ref.
			explicit RenderingContext2D(std::unique_ptr<QPainter> nativeObject);
			explicit RenderingContext2D(std::shared_ptr<QPainter> nativeObject);
			QPainter& asNativeObject();
			const QPainter& asNativeObject() const;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
			explicit RenderingContext2D(win32::Handle<HDC>&& nativeObject);
			explicit RenderingContext2D(const win32::Handle<HDC>& nativeObject);	// weak ref.
			const win32::Handle<HDC>& asNativeObject() const;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDIPLUS)
			explicit RenderingContext2D(Gdiplus::Graphics& nativeObject);	// weak ref.
			explicit RenderingContext2D(std::unique_ptr<Gdiplus::Graphics> nativeObject);
			explicit RenderingContext2D(std::shared_ptr<Gdiplus::Graphics> nativeObject);
			Gdiplus::Graphics& asNativeObject();
			const Gdiplus::Graphics& asNativeObject() const;
#endif
#ifdef ASCENSION_COMPILER_MSVC
			RenderingContext2D(RenderingContext2D&& other) /*noexcept*/ : nativeObject_(std::move(other.nativeObject_)) {}
			RenderingContext2D& operator=(RenderingContext2D&& other) {
				nativeObject_ = std::move(other.nativeObject_);
				return *this;
			}
#endif // ASCENSION_COMPILER_MSVC
		public:
			// back-reference to the canvas
//			???? canvas() const;
			const RenderingDevice& device() const;
			// state
			/**
			 * Pushes the current drawing context onto the drawing state stack.
			 * @return This object
			 * @see #restore
			 */
			RenderingContext2D& save();
			/**
			 * Pops the top state on the drawing state stack, restoring the context to that state.
			 * If there is no saved state, this method does nothing.
			 * @return This object
			 * @see #save
			 */
			RenderingContext2D& restore();
			// compositing
			/**
			 * Returns the current alpha value applied to rendering operations. Initial value is
			 * @c 1.0.
			 * @return The current alpha value in the range from 0.0 (fully transparent) to 1.0 (no
			 *         additional transparency)
			 * @see #globalCompositeOperation, #setGlobalAlpha
			 */
			double globalAlpha() const;
			/**
			 * Sets the current alpha value applied to rendering operations.
			 * @param globalAlpha The alpha value in the range from 0.0 (fully transparent) to 1.0
			 *                   (no additional transparency)
			 * @throw std#invalid_argument @a globalAlpha is out of range from 0.0 to 1.0
			 * @see #globalAlpha, #setGlobalCompositeOperation
			 */
			RenderingContext2D& setGlobalAlpha(double globalAlpha);
			/**
			 * Returns the current composition operation. Initial value is @c SOURCE_OVER.
			 * @return The current composition operation
			 * @see #globalAlpha, #setGlobalCompositeOperation
			 */
			CompositeOperation globalCompositeOperation() const;
			/**
			 * Sets the current composition operation.
			 * @param compositeOperation The new composition operation
			 * @throw UnknownValueException @a compositeOperation is unknown
			 * @see #globalCompositeOperation, #setGlobalAlpha
			 */
			RenderingContext2D& setGlobalCompositeOperation(CompositeOperation compositeOperation);
			// colors and styles
			/**
			 * Returns the current style used for stroking shapes. Initial value is opaque black.
			 * @return The current stokre style
			 * @see #setStrokeStyle, #fillStyle
			 */
			std::shared_ptr<Paint> strokeStyle() const {return strokeStyle_.first;}
			/**
			 * Sets the style used for stroking shapes.
			 * @param strokeStyle The new fill style to set
			 * @return This object
			 * @see #strokeStyle, #setFillStyle
			 */
			RenderingContext2D& setStrokeStyle(std::shared_ptr<Paint> strokeStyle);
			/**
			 * Returns the current style used for filling shapes. Initial value is opaque black.
			 * @return The current fill style
			 * @see #setFillStyle, #strokeStyle
			 */
			std::shared_ptr<Paint> fillStyle() const {return fillStyle_.first;}
			/**
			 * Sets the style used for filling shapes.
			 * @param fillStyle The new fill style to set
			 * @return This object
			 * @see #fillStyle, #setStrokeStyle
			 */
			RenderingContext2D& setFillStyle(std::shared_ptr<Paint> fillStyle);
//			std::unique_ptr<Gradient> createLinearGradient();
//			std::unique_ptr<Gradient> createRadialGradient();
//			std::unique_ptr<Pattern> createPattern();
			// shadows
			/**
			 * Returns the current shadow offset. Initial value is <code>(0, 0)</code>.
			 * @return The current shadow offset
			 * @see #setShadowOffset
			 */
			NativeSize shadowOffset() const;
			/**
			 * Sets the shadow offset.
			 * @return The new shadow offset to set
			 * @see #shadowOffset
			 */
			RenderingContext2D& setShadowOffset(const NativeSize& shadowOffset);
			/**
			 * Returns the current level of blur applied to shadows. Initial value is @c 0.
			 * @return The current level of blur applied to shadows
			 * @see #setShadowBlur
			 */
			Scalar shadowBlur() const;
			/**
			 * Sets the shadow color.
			 * @param shadowColor The new shadow color to set
			 * @return This object
			 * @see #shadowBlur
			 */
			RenderingContext2D& setShadowBlur(Scalar shadowBlur);
			/**
			 * Returns the current shadow color. Initial value is fully-transparent black.
			 * @return The current shadow color
			 * @see #setShadowColor
			 */
			Color shadowColor() const;
			/**
			 * Sets the shadow color.
			 * @param shadowColor The new shadow color to set
			 * @return This object
			 * @see #shadowColor
			 */
			RenderingContext2D& setShadowColor(const Color& shadowColor);
			// rects
			/**
			 * Clears all pixels on the canvas in the specified rectangle to transparent black.
			 * @param rectangle The rectangle
			 * @return This object
			 * @see #fillRectangle, #strokeRectangle
			 */
			RenderingContext2D& clearRectangle(const NativeRectangle& rectangle);
			/**
			 * Paints the specified rectangle onto the canvas, using the current fill style.
			 * @param rectangle The rectangle
			 * @return This object
			 * @see #clarRectangle, #strokeRectangle
			 */
			RenderingContext2D& fillRectangle(const NativeRectangle& rectangle);
			/**
			 * Paints the box that outlines the specified rectangle onto the canvas, using the
			 * current stroke style.
			 * @param rectangle The rectangle
			 * @return This object
			 * @see #clearRectangle, #fillRectangle
			 */
			RenderingContext2D& strokeRectangle(const NativeRectangle& rectangle);
			// current default path API
			/**
			 * Resets the current default path.
			 * @return This object
			 * @see #closePath
			 */
			RenderingContext2D& beginPath();
			/**
			 * Fills the subpaths of the current default path with the current fill style.
			 * @return This object
			 * @see #fillRectangle, #fillText, #stroke
			 */
			RenderingContext2D& fill();
			/**
			 * Strokes the subpaths of the current default path with the current stroke style.
			 * @return This object
			 * @see #fill, #strokeRectangle, #strokeText
			 */
			RenderingContext2D& stroke();
			/**
			 * Draws a focus ring around the current default path, following the platform
			 * conventions for focus rings.
			 * @see #drawCustomFocusRing
			 */
			void drawSystemFocusRing(/*const NativeRectangle& bounds*/);
			/**
			 * The end user has configured whose system to draw focus rings in a particular manner
			 * (for example, high contrast focus rings), draws a focus ring around the current
			 * default path and returns @c false. Otherwise returns @c true.
			 * @retval true This method did not draw a focus ring
			 * @retval false This method drew a focus ring
			 */
			bool drawCustomFocusRing(/*const NativeRectangle& bounds*/);
			/**
			 * Scrolls the current default path into view. This is especially useful on devices
			 * with small screens, where the whole canvas might not be visible at once.
			 * @return This object
			 */
			RenderingContext2D& scrollPathIntoView();
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
			bool isPointInPath(const NativePoint& point) const;
			// text
			/**
			 * Fills the given text at the given position. If a maximum measure is provided, the
			 * text will be scaled to fit that measure if necessary.
			 * @param text The text string
			 * @param origin The origin of the text. The alignment of the drawing is calculated by
			 *               the current @c #textAlign and @c #textBaseline values
			 * @param maximumMeasure If present, this value specifies the maximum measure (width
			 *                       in horizontal writing mode)
			 * @return This object
			 * @throw std#invalid_argument @a maximumMeasure is present but less than or equal to zero
			 * @see #strokeText, #measureText
			 */
			RenderingContext2D& fillText(const StringPiece& text,
				const NativePoint& origin, boost::optional<Scalar> maximumMeasure = boost::none);
			/**
			 * Strokes the given text at the given position. If a maximum measure is provided, the
			 * text will be scaled to fit that measure if necessary.
			 * @param text The text string
			 * @param origin The origin of the text. The alignment of the drawing is calculated by
			 *               the current @c #textAlign and @c #textBaseline values
			 * @param maximumMeasure If present, this value specifies the maximum measure (width
			 *                       in horizontal writing mode)
			 * @return This object
			 * @throw std#invalid_argument @a maximumMeasure is present but less than or equal to zero
			 * @see #fillText, #measureText
			 */
			RenderingContext2D& strokeText(const StringPiece& text,
				const NativePoint& origin, boost::optional<Scalar> maximumMeasure = boost::none);
			/**
			 * Returns a size (measure and extent) of the specified text in the current font.
			 * @param text The text string
			 * @see #strokeText, #fillText
			 */
			NativeSize measureText(const StringPiece& text) const;
			// drawing images
			/**
			 * Draws the specified image onto the canvas.
			 * @param image The image to draw
			 * @param position The destination position
			 * @return This object
			 */
			RenderingContext2D& drawImage(const Image& image, const NativePoint& position);
			/**
			 * Draws the specified image onto the canvas.
			 * @param image The image to draw
			 * @param destinationBounds The destination bounds
			 * @return This object
			 */
			RenderingContext2D& drawImage(const Image& image, const NativeRectangle& destinationBounds);
			/**
			 * Draws the specified image onto the canvas.
			 * @param image The image to draw
			 * @param sourceBounds The bounds in @a image data to draw
			 * @param destinationBounds The destination bounds
			 * @return This object
			 */
			RenderingContext2D& drawImage(const Image& image, const NativeRectangle& sourceBounds, const NativeRectangle& destinationBounds);
			// pixel manipulation
			/**
			 * Returns an @c ImageData object with the specified dimensions. All the pixels in the
			 * returned object are transparent black.
			 * @param dimensions The dimensions in pixels
			 * @return An image data
			 */
			std::unique_ptr<ImageData> createImageData(const NativeSize& dimensions) const;
			/**
			 * Returns an @c ImageData object with the same dimensions as the argument. All pixels
			 * in the returned object are transparent black.
			 * @param image The image data gives the dimensions
			 * @return An image data
			 */
			std::unique_ptr<ImageData> createImageData(const ImageData& image) const {
		     	return createImageData(geometry::make<NativeSize>(image.width(), image.height()));
			}
			/**
			 * Returns an @c ImageData object containing the image data for the specified rectangle
			 * of the canvas.
			 * @param rectangle The bounds of the image in canvas coordinate space units
			 * @return An image data. Pixels outside the canvas are returned as transparent black
			 * @see #putImageData
			 */
			std::unique_ptr<ImageData> getImageData(const NativeRectangle& rectangle) const;
			/**
			 * Paints the data from the specified @c ImageData object onto the canvas. The
			 * @c #globalAlpha and @c #globalCompositeOperation attributes, as well as the shadow
			 * attributes, are ignored for the purposes of this method call; pixels in the canvas
			 * are replaced wholesale, with no composition, alpha blending, no shadows, etc.
			 * @param image The image data
			 * @param destination The destination position onto where the image is painted in the
			 *                    canvas coordinate space units
			 * @see #getImageData
			 */
			RenderingContext2D& putImageData(const ImageData& image, const NativePoint& destination) {
				return putImageData(image, destination, geometry::make<NativeRectangle>(
					geometry::make<NativePoint>(0, 0), geometry::make<NativeSize>(image.width(), image.height())));
			}
			/**
			 * Paints the data from the specified @c ImageData object onto the canvas. Only the
			 * pixels from @a dirtyRectangle are painted. The @c #globalAlpha and
			 * @c #globalCompositeOperation attributes, as well as the shadow attributes, are
			 * ignored for the purposes of this method call; pixels in the canvas are replaced
			 * wholesale, with no composition, alpha blending, no shadows, etc.
			 * @param image The image data
			 * @param destination The destination position onto where the image is painted in the
			 *                    canvas coordinate space units
			 * @param dirtyRectangle The bounds to paint in image in device pixels
			 * @see #getImageData
			 */
			RenderingContext2D& putImageData(const ImageData& image,
				const NativePoint& destination, const NativeRectangle& dirtyRectangle);
			// transformations (CanvasTransformation interface)
			/**
			 * Adds the scaling transformation described by @a s to the transformation matrix.
			 * @param sx The scale factor in the horizontal direction. The factor is multiplies
			 * @param sy The scale factor in the vertical direction. The factor is multiplies
			 * @return This object
			 * @see #rotate, #translate, #transform, #setTransform
			 */
			RenderingContext2D& scale(
				geometry::Coordinate<NativeAffineTransform>::Type sx,
				geometry::Coordinate<NativeAffineTransform>::Type sy);
			/**
			 * Adds the rotation transformation described by @a angle to the transformation matrix.
			 * @param angle A clockwise rotation angle in radians
			 * @return This object
			 * @see #scale, #translate, #transform, #setTransform
			 */
			RenderingContext2D& rotate(double angle);
			/**
			 * Adds the translation transformation described by @a delta to the transformation
			 * matrix.
			 * @param delta The translation transformation. @c geometry#dx(delta) represents the
			 *              translation distance in the horizontal direction and
			 *              @c geometry#dy(delta) represents the translation distance in the
			 *              vertical direction. These are in xxx units
			 * @return This object
			 * @see #scale, #rotate, #transform, #setTransform
			 */
			RenderingContext2D& translate(const NativeSize& delta);
			/**
			 * Adds the translation transformation described by @a dx and @a dy to the
			 * transformation matrix.
			 * @param dx The translation distance in the horizontal direction in xxx units
			 * @param dy The translation distance in the vertical direction in xxx units
			 * @return This object
			 * @see #scale, #rotate, #transform, #setTransform
			 */
			RenderingContext2D& translate(
				geometry::Coordinate<NativeAffineTransform>::Type dx,
				geometry::Coordinate<NativeAffineTransform>::Type dy);
			/**
			 * Replaces the current transformation matrix with the result of multiplying the
			 * current transformation matrix with the matrix described by @a matrix.
			 * @param matrix The matrix to multiply with
			 * @return This object
			 * @see #scale, #rotate, #translate, #setTransform
			 */
			RenderingContext2D& transform(const NativeAffineTransform& matrix);
			/**
			 * Resets the current transformation to the identity matrix and calls @c #transform(matrix).
			 * @param matrix The new transformation matrix
			 * @return This object
			 * @see #scale, #rotate, #translate, #transform
			 */
			RenderingContext2D& setTransform(const NativeAffineTransform& matrix);
			// line caps/joins (CanvasLineStyles interface)
			/**
			 * Returns the current width of lines, in xxx units. Initial value is @c 1.0.
			 * @return The current width of lines
			 * @see #setLineWidth
			 */
			Scalar lineWidth() const;
			/**
			 * Sets the width of lines.
			 * @param lineWidth The width of lines, in xxx units. Values that are not finite values
			 *                  greater than zero are ignored
			 * @return This object
			 * @see #lineWidth
			 */
			RenderingContext2D& setLineWidth(Scalar lineWidth);
			/**
			 * Returns the current line cap style. Initial value is @c LineCap#BUTT.
			 * @return The current line cap style
			 * @see #setLineCap
			 */
			LineCap lineCap() const;
			/**
			 * Sets the line cap style.
			 * @param lineCap The line cap style
			 * @return This object
			 * @throw UnknownValueException @a lineCap is unknown
			 * @see #lineCap
			 */
			RenderingContext2D& setLineCap(LineCap lineCap);
			/**
			 * Returns the current line join style. Initial value is @c LineJoin#MITER.
			 * @return The current line join style
			 * @see #setLineJoin
			 */
			LineJoin lineJoin() const;
			/**
			 * Sets the line join style.
			 * @param lineJoin The line join style
			 * @return This object
			 * @throw UnknownValueException @a lineJoin is unknown
			 * @see #lineJoin
			 */
			RenderingContext2D& setLineJoin(LineJoin lineJoin);
			/**
			 * Returns the current miter limit ratio. Initial value is @c 10.0.
			 * @return The current miter limit ratio
			 * @see #setMiterLimit
			 */
			double miterLimit() const;
			/**
			 * Sets the miter limit ratio.
			 * @param miterLimit The miter limit ratio. Values that are not finite values greater
			 *                   than zero are ignored
			 * @return This object
			 * @see #miterLimit
			 */
			RenderingContext2D& setMiterLimit(double miterLimit);
			// text (CanvasText interface)
			std::shared_ptr<const font::Font> font() const;
			RenderingContext2D& setFont(std::shared_ptr<const font::Font> font);
			/**
			 * Returns the current text alignment settings. Default value is @c TEXT_ANCHOR_START.
			 * @return The current text alignment settings
			 * @see #setTextAlignment, #textBaseline
			 */
			TextAlignment textAlignment() const;
			/**
			 * Sets the text alignment settings.
			 * @param textAlignment The new text alignment settings
			 * @return This object
			 * @throw UnknownValueException
			 * @see #setTextBaseline, #textAlignment
			 */
			RenderingContext2D& setTextAlignment(TextAlignment textAlignment);
			/**
			 * Returns the current baseline alignment settings. Default value is
			 * @c ALIGNMENT_BASELINE_ALPHABETIC.
			 * @return The current baseline alignment settings
			 * @see #setTextBaseline, #textAlignment
			 */
			presentation::AlignmentBaseline textBaseline() const;
			/**
			 * Sets the baseline alignment settings.
			 * @param textAlignment The new baseline alignment settings
			 * @return This object
			 * @throw UnknownValueException
			 * @see #setTextAlignment, #textBaseline
			 */
			RenderingContext2D& setTextBaseline(presentation::AlignmentBaseline baseline);
			// shared path API methods (CanvasPathMethods interface)
			/**
			 * Marks the current subpath as closed, and starts a new subpath with a point the same
			 * as the start and end of the newly closed subpath.
			 * @return This object
			 * @see #beginPath
			 */
			RenderingContext2D& closePath();
			/**
			 * Creates a new subpath with the specified point.
			 * @param to The point as first (and only) of the new subpath
			 * @return This object
			 * @see #lineTo
			 */
			RenderingContext2D& moveTo(const NativePoint& to);
			/**
			 * Adds the specified point to the current subpath, connected to the previous one by a
			 * straight line. If the path has no subpaths, ensures there is a subpath for that
			 * point.
			 * @param to The point to add to the current subpath
			 * @return This object
			 * @see #moveTo
			 */
			RenderingContext2D& lineTo(const NativePoint& to);
			/**
			 * Adds the specified point to the current subpath, connected to the previous one by a
			 * quadratic Bézier curve with the specified control point.
			 * @param cp The control point
			 * @param to The point to add to the current subpath
			 * @return This object
			 * @see #bezierCurveTo
			 */
			RenderingContext2D& quadraticCurveTo(const NativePoint& cp, const NativePoint& to);
			/**
			 * Adds the specified point to the current subpath, connected to the previous one by a
			 * cubic Bézier curve with the specified control point.
			 * @param cp1 The first control point
			 * @param cp2 The second control point
			 * @param to The point to add to the current subpath
			 * @return This object
			 * @see #bezierCurveTo
			 */
			RenderingContext2D& bezierCurveTo(const NativePoint& cp1, const NativePoint& cp2, const NativePoint& to);
			/**
			 * Adds an arc with the specified control points and radius to the current subpath,
			 * connected to the previous point by a straight line.
			 * @param p1 The start point
			 * @param p2 The destination point
			 * @param radius Radius of the circle gives the arc
			 * @return This object
			 * @throw std#invalid_argument @a radius is negative
			 * @see #arc
			 */
			RenderingContext2D& arcTo(const NativePoint& p1, const NativePoint& p2, Scalar radius);
			/**
			 * Adds a new closed subpath to the path, representing the specified rectangle.
			 * @param rect The rectangle
			 * @return This object
			 */
			RenderingContext2D& rectangle(const NativeRectangle& rect);
			/**
			 * Adds points to the subpath such that the arc described by the circumference of the
			 * circle described by the arguments, starting at the specified start angle and ending
			 * at the given end angle, going in the given direction, is added to the path,
			 * connected to the previous point by a straight line.
			 * @param p The origin of the circle gives the arc
			 * @param radius Radius of the circle gives the arc
			 * @param startAngle Defines the start point 
			 * @param endAngle Defines the end point
			 * @param counterClosewise If @c false, the arc is the path along the circumstance of
			 *                         the circle from the start point to the end point, closewise
			 * @return This object
			 * @throw std#invalid_argument @a radius is negative
			 * @see #arc
			 */
			RenderingContext2D& arc(const NativePoint& p, Scalar radius,
				double startAngle, double endAngle, bool counterClockwise = false);
		private:
#if defined(ASCENSION_GRAPHICS_SYSTEM_CAIRO)
			Cairo::RefPtr<Cairo::Context> nativeObject_;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_CORE_GRAPHICS)
			CGContextRef nativeObject_;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_DIRECT2D)
			win32::com::SmartPointer<ID2D1RenderTarget> nativeObject_;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_QT)
			std::shared_ptr<QPainter> nativeObject_;
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI)
			win32::Handle<HDC> nativeObject_;
			win32::Handle<HPEN> currentPen_, oldPen_;
			win32::Handle<HBRUSH> oldBrush_;
			std::stack<int> savedStates_;
			bool hasCurrentSubpath_;
			RenderingContext2D& changePen(
				const LOGBRUSH* patternBrush, boost::optional<Scalar> lineWidth,
				boost::optional<LineCap> lineCap, boost::optional<LineJoin> lineJoin);
			bool endPath();
			bool ensureThereIsASubpathFor(const NativePoint& p);
#elif defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDIPLUS)
			std::shared_ptr<Gdiplus::Graphics> nativeObject_;
#endif
			std::pair<std::shared_ptr<Paint>, std::size_t> fillStyle_, strokeStyle_;
		};

		class PaintContext : public RenderingContext2D {
		public:
			/**
			 * Constructor.
			 * @param context The rendering context
			 * @param boundsToPaint The rectangle in which the painting is requested
			 */
			PaintContext(RenderingContext2D&& context, const NativeRectangle& boundsToPaint)
				: RenderingContext2D(std::move(context)), boundsToPaint_(boundsToPaint) {}
			/// Returns the rendering context.
//			RenderingContext2D& operator*() /*noexcept*/ {return context_;}
			/// Returns the rendering context.
//			const RenderingContext2D& operator*() const /*noexcept*/ {return context_;}
			/// Returns the rendering context.
//			RenderingContext2D* operator->() /*noexcept*/ {return &context_;}
			/// Returns the rendering context.
//			const RenderingContext2D* operator->() const /*noexcept*/ {return &context_;}
			/// Returns a rectangle in which the painting is requested.
			const NativeRectangle& boundsToPaint() const /*noexcept*/ {return boundsToPaint_;}
		private:
			const NativeRectangle boundsToPaint_;
		};
	}

	namespace detail {
		inline win32::Handle<HDC> screenDC() {
			return win32::Handle<HDC>(::GetDC(nullptr),
				std::bind(&::ReleaseDC, static_cast<HWND>(nullptr), std::placeholders::_1));
		}
	}
}

#endif //!ASCENSION_RENDERING_CONTEXT_HPP
