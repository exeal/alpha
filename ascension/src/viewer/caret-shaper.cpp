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
		/**
		 * Returns logical bounds of the character the given caret addresses.
		 * @param caret The caret
		 * @return The logical character bounds in user units. (0, 0) is the alignment point
		 */
		graphics::Rectangle&& currentCharacterLogicalBounds(const Caret& caret) {
			const graphics::font::TextRenderer& textRenderer = caret.textViewer().textRenderer();
			const graphics::font::TextLayout& layout = textRenderer.layouts().at(line(caret));
			const Index subline = layout.lineAt(offsetInLine(caret));
			boost::integer_range<graphics::Scalar> extent(layout.extent(boost::irange(subline, subline + 1)));

			const presentation::AbstractTwoAxes<graphics::Scalar> leading(layout.hitToPoint(graphics::font::TextHit<>::leading(offsetInLine(caret))));
			extent = boost::irange(*extent.begin() - leading.bpd(), *extent.end() - leading.bpd());

			graphics::Scalar trailing;
			if(kernel::locations::isEndOfLine(caret))	// EOL
//				trailing = widgetapi::createRenderingContext(caret.textViewer())->fontMetrics(textRenderer.defaultFont())->averageCharacterWidth();
				trailing = 0;
			else
				trailing = layout.hitToPoint(graphics::font::TextHit<>::trailing(offsetInLine(caret))).ipd() - leading.ipd();

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
