/**
 * @c file standard-caret-painter.hpp
 * Defines @c StandardCaretPainter class.
 * @author exeal
 * @date 2015-11-28 Created.
 */

#ifndef ASCENSION_STANDARD_CARET_PAINTER_HPP
#define ASCENSION_STANDARD_CARET_PAINTER_HPP
#include <ascension/viewer/caret-painter.hpp>

namespace ascension {
	namespace viewer {
		class StandardCaretPainter : public CaretPainter {
		private:
			void paint(graphics::PaintContext& context,
				const graphics::font::TextLayout& layout, const graphics::Point& alignmentPoint);
		};
	}
}

#endif // !ASCENSION_STANDARD_CARET_PAINTER_HPP
