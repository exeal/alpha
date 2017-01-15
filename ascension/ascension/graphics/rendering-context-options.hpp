/**
 * @file rendering-context-options.hpp
 * @author exeal
 * @date 2011-03-06 created
 * @date 2017-01-15 Separated from rendering-context.hpp.
 */

#ifndef ASCENSION_RENDERING_CONTEXT_OPTIONS_HPP
#define ASCENSION_RENDERING_CONTEXT_OPTIONS_HPP
#include <ascension/graphics/font/text-alignment.hpp>

namespace ascension {
	namespace graphics {
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

		/// Valid values for 'lineCap' attribute.
		/// Each point has a flat edge perpendicular to the direction of the line coming out of it.
		ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(LineCap)
			/// No additional line cap is added.
			BUTT,
			/// A semi-circle with the diameter equal to the styles 'lineWidth' width must additionally be placed on to
			/// the line coming out of each point.
			ROUND,
			/// A rectangle with the length of the style 'lineWidth' width and the width of half the styles 'lineWidth'
			/// width, placed flat against the edge perpendicular to the direction of the line coming out of the point,
			/// must be added at each point.
			SQUARE
		ASCENSION_SCOPED_ENUM_DECLARE_END(LineCap)

		/// Valid values for 'lineJoin' attribute.
		/// In addition to the point where a join occurs, two additional points are relevant to each join, one for each
		/// line: the two corners found half the line width away from the join point, one perpendicular to each line,
		/// each on the side furthest from the other line.
		ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(LineJoin)
			/// This is all that is rendered at joins.
			BEVEL,
			/// A filled arc connecting the two aforementioned corners of the join, abutting (and not overlapping) the
			/// aforementioned triangle, with the diameter equal to the line width and the origin at the point of the
			/// join, must be added at joins.
			ROUND,
			/// A second filled triangle must (if it can given the miter length) be added at the join, with one line
			/// being the line between the two aforementioned corners, abutting the first triangle, and the other two
			/// being continuations of the outside edges of the two joining lines, as long as required to intersect
			/// without going over the miter length.
			MITER
		ASCENSION_SCOPED_ENUM_DECLARE_END(LineJoin)

		/// Valid values for 'textAlignment' attribute.
		ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(TextAlignment)
			START = font::TextAlignment::START,
			END = font::TextAlignment::END,
			LEFT = font::TextAlignment::LEFT,
			RIGHT = font::TextAlignment::RIGHT,
			CENTER = font::TextAlignment::CENTER
		ASCENSION_SCOPED_ENUM_DECLARE_END(TextAlignment)
	}
}

#endif // !ASCENSION_RENDERING_CONTEXT_OPTIONS_HPP
