/**
 * @file caret-shaper.cpp
 * @author exeal
 * @date 2003-2006 was EditView.cpp and EditViewWindowMessages.cpp
 * @date 2006-2011 was viewer.cpp
 * @date 2011-09-25 separated from viewer.cpp
 * @date 2011-2014
 */

#include <ascension/graphics/font/font-metrics.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/presentation/writing-mode-mappings.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/caret-shaper.hpp>
#include <ascension/viewer/viewer.hpp>

namespace ascension {
	namespace viewers {
		graphics::Rectangle&& viewers::currentCharacterLogicalBounds(const Caret& caret) {
			const graphics::font::TextRenderer& textRenderer = caret.textViewer().textRenderer();
			const graphics::font::TextLayout& layout = textRenderer.layouts().at(line(caret));
			const Index subline = layout.lineAt(offsetInLine(caret));
			const boost::integer_range<graphics::Scalar> extent(layout.extent(boost::irange(subline, subline + 1)));

			graphics::Scalar trailing;
			if(kernel::locations::isEndOfLine(caret))	// EOL
//				trailing = widgetapi::createRenderingContext(caret.textViewer())->fontMetrics(textRenderer.defaultFont())->averageCharacterWidth();
				trailing = 0;
			else
				trailing = layout.hitToPoint(graphics::font::TextHit<>::trailing(offsetInLine(caret))).ipd()
					- layout.hitToPoint(graphics::font::TextHit<>::leading(offsetInLine(caret))).ipd();

			return graphics::geometry::make<graphics::Rectangle>(presentation::mapFlowRelativeToPhysical(layout.writingMode(),
				presentation::FlowRelativeFourSides<graphics::Scalar>(
					presentation::_before = *extent.begin(), presentation::_after = *extent.end(), presentation::_start = 0.0f, presentation::_end = trailing)));
		}


		// CaretShapeUpdater //////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * Private constructor.
		 * @param caret The caret
		 */
		CaretShapeUpdater::CaretShapeUpdater(Caret& caret) BOOST_NOEXCEPT : caret_(caret) {
		}

		/// Returns the caret.
		Caret& CaretShapeUpdater::caret() BOOST_NOEXCEPT {
			return caret_;
		}

		/// Returns the caret.
		const Caret& CaretShapeUpdater::caret() const BOOST_NOEXCEPT {
			return caret_;
		}

		/// Notifies the text viewer to update the shape of the caret.
		void CaretShapeUpdater::update() BOOST_NOEXCEPT {
			caret().resetVisualization();	// $friendly-access
		}
	}
}
