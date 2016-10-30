/**
 * @file viewer/text-hit.hpp
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

		graphics::font::TextHit<> inlineHit(const TextHit& hit) BOOST_NOEXCEPT;
		kernel::Position insertionPosition(const kernel::Document& document, const TextHit& hit);
		TextHit otherHit(const kernel::Document& document, const TextHit& hit);

		/**
		 * Transforms the given @c TextHit into @c graphics#font#TextHit&lt;&gt; by using @c kernel#offsetInLine.
		 * @param hit The @c TextHit object to transform
		 * @return The transformed @c graphics#font#TextHit&lt;&gt; object
		 */
		inline graphics::font::TextHit<> inlineHit(const TextHit& hit) BOOST_NOEXCEPT {
			return graphics::font::transformTextHit(hit, [](const kernel::Position& p) {
				return kernel::offsetInLine(p);
			});
		}
	}
}

#endif // !ASCENSION_VIEWER_TEXT_HIT_HPP
