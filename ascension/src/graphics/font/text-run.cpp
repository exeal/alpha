/**
 * @file text-run.cpp
 * Implements free functions related to @c TextRun interface.
 * @author exeal
 * @date 2015-03-29 Separated from text-run.hpp.
 */

#include <ascension/graphics/font/actual-text-styles.hpp>
#include <ascension/graphics/font/font.hpp>
#include <ascension/graphics/font/text-run.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			/**
			 * Returns the measure of the 'allocation-rectangle' of the specified text run in user units.
			 * @see #measure
			 */
			Scalar allocationMeasure(const TextRun& textRun) {
				const presentation::FlowRelativeFourSides<ActualBorderSide>* const border = textRun.border();
				const presentation::FlowRelativeFourSides<Scalar>* const margin = textRun.margin();
				const presentation::FlowRelativeFourSides<Scalar>* const padding = textRun.padding();
				return measure(textRun)
					+ ((border != nullptr) ? border->start().actualWidth() + border->end().actualWidth() : 0)
					+ ((margin != nullptr) ? margin->start() + margin->end() : 0)
					+ ((padding != nullptr) ? padding->start() + padding->end() : 0);
			}

			/**
			 * Returns the 'border-box' of the specified text run in user units.
			 * @see #contentBox, #marginBox, #paddingBox, TextRun#border
			 */
			presentation::FlowRelativeFourSides<Scalar> borderBox(const TextRun& textRun) BOOST_NOEXCEPT {
				presentation::FlowRelativeFourSides<Scalar> bounds(paddingBox(textRun));
				if(const presentation::FlowRelativeFourSides<ActualBorderSide>* const borders = textRun.border()) {
					bounds.before() -= borders->before().actualWidth();
					bounds.after() += borders->after().actualWidth();
					bounds.start() -= borders->start().actualWidth();
					bounds.end() += borders->end().actualWidth();
				}
				return bounds;
			}

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
