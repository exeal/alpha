/**
 * @file text-run.cpp
 * Implements free functions related to @c TextRun interface.
 * @author exeal
 * @date 2015-03-29 Separated from text-run.hpp.
 */

#include <ascension/graphics/font/actual-text-styles.hpp>
#include <ascension/graphics/font/font.hpp>
#include <ascension/graphics/font/text-run.hpp>
#include <ascension/graphics/geometry/point-xy.hpp>
#include <boost/geometry/algorithms/make.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			/**
			 * Returns the measure of the 'allocation-rectangle' of the specified text run in user units.
			 * @see #measure
			 */
			NumericRange<Scalar> allocationMeasure(const TextRun& textRun) {
				Scalar start = 0, end = 0;
				if(const presentation::FlowRelativeFourSides<ActualBorderSide>* const border = textRun.border()) {
					start += border->start().actualWidth();
					end += border->end().actualWidth();
				}
				if(const presentation::FlowRelativeFourSides<Scalar>* const margin = textRun.margin()) {
					start += margin->start();
					end += margin->end();
				}
				if(const presentation::FlowRelativeFourSides<Scalar>* const padding = textRun.padding()) {
					start += padding->start();
					end += padding->end();
				}
				const NumericRange<Scalar> content(measure(textRun));
				return nrange(*boost::const_begin(content) - start, *boost::const_end(content) + end);
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
					presentation::_blockStart = -lm->ascent(), presentation::_blockEnd = lm->descent());
			}
		}
	}
}
