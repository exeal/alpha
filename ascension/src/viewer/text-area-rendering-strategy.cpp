/**
 * @file text-area-rendering-strategy.cpp
 * Implements @c TextAreaRenderingStrategy class.
 * @author exeal
 * @date 2016-07-18 Created.
 */

#include <ascension/graphics/geometry/algorithms/size.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-area-rendering-strategy.hpp>
#include <ascension/viewer/text-viewer.hpp>
//#include <ascension/viewer/widgetapi/widget.hpp>

namespace ascension {
	namespace viewer {
		TextAreaRenderingStrategy::TextAreaRenderingStrategy(TextArea& textArea) :
			textArea_(textArea), fontCollection_(widgetapi::createRenderingContext(textArea_.textViewer())->availableFonts()) {
		}

		const graphics::font::FontCollection& TextAreaRenderingStrategy::fontCollection() const BOOST_NOEXCEPT {
			return fontCollection_;
		}

		graphics::Dimension TextAreaRenderingStrategy::lengthContextViewport() const BOOST_NOEXCEPT {
			return graphics::geometry::size(textArea_.allocationRectangle());
		}

		graphics::Dimension TextAreaRenderingStrategy::parentContentArea() const BOOST_NOEXCEPT {
			return graphics::geometry::size(textArea_.contentRectangle());
		}

		std::unique_ptr<graphics::RenderingContext2D> TextAreaRenderingStrategy::renderingContext() const {
			return widgetapi::createRenderingContext(textArea_.textViewer());
		}
	}
}
