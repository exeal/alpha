/**
 * @file caret-shaper.cpp
 * @author exeal
 * @date 2003-2006 was EditView.cpp and EditViewWindowMessages.cpp
 * @date 2006-2011 was viewer.cpp
 * @date 2011-09-25 separated from viewer.cpp
 * @date 2011-2013
 */

#include <ascension/graphics/font/font-metrics.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/presentation/writing-mode-mappings.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/caret-shaper.hpp>
#include <ascension/viewer/viewer.hpp>

using namespace ascension;
using namespace ascension::viewers;
using namespace ascension::graphics;
using namespace ascension::graphics::font;
using namespace std;


graphics::Rectangle&& viewers::currentCharacterLogicalBounds(const Caret& caret) {
	const TextRenderer& textRenderer = caret.textViewer().textRenderer();
	const TextLayout& layout = textRenderer.layouts().at(line(caret));
	const Index subline = layout.lineAt(offsetInLine(caret));
	const boost::integer_range<Scalar> extent(layout.extent(boost::irange(subline, subline + 1)));

	Scalar trailing;
	if(kernel::locations::isEndOfLine(caret))	// EOL
//		trailing = widgetapi::createRenderingContext(caret.textViewer())->fontMetrics(textRenderer.defaultFont())->averageCharacterWidth();
		trailing = 0;
	else
		trailing = layout.hitToPoint(TextHit<>::trailing(offsetInLine(caret))).ipd() - layout.hitToPoint(TextHit<>::leading(offsetInLine(caret))).ipd();

	using namespace presentation;
	return geometry::make<graphics::Rectangle>(mapFlowRelativeToPhysical(layout.writingMode(),
		FlowRelativeFourSides<Scalar>(_before = *extent.begin(), _after = *extent.end(), _start = static_cast<Scalar>(0), _end = trailing)));
}


// CaretShapeUpdater //////////////////////////////////////////////////////////////////////////////

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
