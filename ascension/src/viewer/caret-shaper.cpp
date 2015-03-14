/**
 * @file caret-shaper.cpp
 * @author exeal
 * @date 2003-2006 was EditView.cpp and EditViewWindowMessages.cpp
 * @date 2006-2011 was viewer.cpp
 * @date 2011-09-25 separated from viewer.cpp
 * @date 2011-2015
 */

#include <ascension/graphics/font/font-metrics.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/presentation/writing-mode-mappings.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/caret-shaper.hpp>
#include <ascension/viewer/text-viewer.hpp>

namespace ascension {
	namespace viewer {
		/**
		 * Returns logical bounds of the character the given caret addresses.
		 * @param caret The caret
		 * @return The logical character bounds in user units. (0, 0) is the alignment point
		 * @retval boost#none The layout of the line the given caret addresses is not calculated
		 */
		boost::optional<graphics::Rectangle> currentCharacterLogicalBounds(const Caret& caret) {
			const graphics::font::TextRenderer& textRenderer = caret.textViewer().textRenderer();
			if(const graphics::font::TextLayout* const layout = textRenderer.layouts().at(kernel::line(caret))) {
				const Index subline = layout->lineAt(kernel::offsetInLine(caret));
				boost::integer_range<graphics::Scalar> extent(layout->extent(boost::irange(subline, subline + 1)));

				const presentation::FlowRelativeTwoAxes<graphics::Scalar> leading(layout->hitToPoint(graphics::font::TextHit<>::leading(kernel::offsetInLine(caret))));
				extent = boost::irange(*extent.begin() - leading.bpd(), *extent.end() - leading.bpd());

				graphics::Scalar trailing;
				if(kernel::locations::isEndOfLine(caret))	// EOL
//					trailing = widgetapi::createRenderingContext(caret.textViewer())->fontMetrics(textRenderer.defaultFont())->averageCharacterWidth();
					trailing = 0;
				else
					trailing = layout->hitToPoint(graphics::font::TextHit<>::trailing(kernel::offsetInLine(caret))).ipd() - leading.ipd();

				return graphics::geometry::make<graphics::Rectangle>(
					presentation::mapFlowRelativeToPhysical(
						graphics::font::writingMode(*layout),
						presentation::FlowRelativeFourSides<graphics::Scalar>(
							presentation::_blockStart = *extent.begin(), presentation::_blockEnd = *extent.end(),
							presentation::_inlineStart = 0.0f, presentation::_inlineEnd = trailing)));
			}
			return boost::none;
		}

		/// Returns @c StaticShapeChangedSignal signal connector.
		SignalConnector<CaretShaper::StaticShapeChangedSignal> CaretShaper::staticShapeChangedSignal() BOOST_NOEXCEPT {
			return makeSignalConnector(staticShapeChangedSignal_);
		}
	}
}
