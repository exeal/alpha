/**
 * @file text-run.cpp
 * Implements free functions related to @c TextRun interface.
 * @author exeal
 * @date 2015-03-29 Separated from text-run.hpp.
 */

#include <ascension/graphics/font/font.hpp>
#include <ascension/graphics/font/text-run.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			/**
			 * Returns the 'content-box' of the specified text run in user units.
			 * @see #borderBox, #marginBox, #paddingBox
			 */
			presentation::FlowRelativeFourSides<Scalar> contentBox(const TextRun& textRun) BOOST_NOEXCEPT {
				const Point lastGlyphPosition(textRun.glyphPosition(textRun.numberOfGlyphs()));
				Scalar measure;
		     	if(geometry::y(lastGlyphPosition) == 0)
					measure = geometry::x(lastGlyphPosition);
		     	else if(geometry::x(lastGlyphPosition) == 0)
					measure = geometry::y(lastGlyphPosition);
				else
					measure = static_cast<Scalar>(boost::geometry::distance(boost::geometry::make_zero<Point>(), lastGlyphPosition));
				std::unique_ptr<const LineMetrics> lm(textRun.font()->lineMetrics(String(), textRun.fontRenderContext()));
				return presentation::FlowRelativeFourSides<Scalar>(
					presentation::_inlineStart = 0.0f, presentation::_inlineEnd = measure,
					presentation::_blockStart = -lm->ascent(), presentation::_blockEnd = lm->descent() + lm->leading());
			}
		}
	}
}