/**
 * @file text-hit.hpp
 * Defines @c viewer#TextHit type and related free functions.
 * @author exeal
 * @see graphics/font/text-hit.hpp
 * @date 2003-xx-xx Created.
 * @date 2008-xx-xx Separated from point.hpp.
 * @date 2011-10-02 Separated from caret.hpp.
 * @date 2016-05-22 Separated from visual-point.hpp.
 * @date 2016-06-05 Separated from visual-destination-proxy.hpp.
 */

#ifndef ASCENSION_VIEWER_TEXT_HIT_HPP
#define ASCENSION_VIEWER_TEXT_HIT_HPP
#include <ascension/graphics/font/text-hit.hpp>
#include <ascension/kernel/position.hpp>

namespace ascension {
	namespace kernel {
		class Document;
	}

	namespace viewer {
		/**
		 * Used by procedures which move @c VisualPoint.
		 * @see graphics#font#TextHit
		 */
		typedef graphics::font::TextHit<kernel::Position> TextHit;

		kernel::Position insertionPosition(const kernel::Document& document, const TextHit& hit);
		TextHit otherHit(const kernel::Document& document, const TextHit& hit);
	}
}

#endif // !ASCENSION_VIEWER_TEXT_HIT_HPP
